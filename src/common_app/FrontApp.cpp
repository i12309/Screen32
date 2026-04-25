#include "common_app/FrontApp.h"

#include <memory>
#include <stdio.h>
#include <string.h>

#include <lvgl.h>

#include "common_app/app_core.h"
#include "common_app/frontend_platform.h"
#include "common_app/frontend_ui_events.h"
#include "demo/offline_demo_controller.h"
#include "demo/offline_demo_ui_events.h"
#include "log/ScreenLibLogger.h"
#include "lvgl_eez/EezLvglAdapter.h"
#include "lvgl_eez/UiObjectMap.h"
#include "page_descriptors.generated.h"
#include "runtime/ScreenClient.h"
#include "ui_object_map.generated.h"

#if defined(ARDUINO)
#include <Arduino.h>
#include <esp_system.h>
#endif

namespace frontapp {
namespace {

constexpr size_t kMaxObjectBindings = SCREEN32_ELEMENT_DESCRIPTOR_COUNT;
constexpr size_t kMaxPageBindings = SCREEN32_PAGE_DESCRIPTOR_COUNT;
constexpr size_t kMaxTrackedElements = SCREEN32_ELEMENT_DESCRIPTOR_COUNT;
constexpr uint32_t kHelloRetryPeriodMs = 5000;
constexpr uint32_t kWaitLogPeriodMs = 5000;
constexpr const char* kLogTag = "frontapp";

struct State {
    demo::FrontendConfig config = demo::frontend_default_config();
    bool initialized = false;
    bool offlineDemo = true;
    bool online = false;
    bool backendConnected = false;
    uint32_t currentSessionId = 0;
    uint32_t currentPageId = 0;
    uint32_t lastHeartbeatMs = 0;
    uint32_t lastHelloMs = 0;
    uint32_t waitStartMs = 0;
    uint32_t lastWaitLogMs = 0;

    std::unique_ptr<ITransport> transport;
    std::unique_ptr<screenlib::client::ScreenClient> client;

    screenlib::adapter::UiObjectBinding objectBindings[kMaxObjectBindings] = {};
    screenlib::adapter::UiPageBinding pageBindings[kMaxPageBindings] = {};
    screenlib::adapter::UiObjectMap objectMap;
    screenlib::adapter::EezLvglAdapter adapter;
    demo::OfflineDemoController offlineController;

    demo::Screen32BoundElement tracked[kMaxTrackedElements] = {};
    size_t trackedCount = 0;

    State()
        : objectMap(objectBindings, kMaxObjectBindings, pageBindings, kMaxPageBindings),
          adapter(&objectMap) {}
};

State g_state;

void copy_text_safe(char* dst, size_t dstSize, const char* src) {
    if (dst == nullptr || dstSize == 0) {
        return;
    }
    dst[0] = '\0';
    if (src == nullptr) {
        return;
    }
    strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
}

bool is_valid_page_id(uint32_t pageId) {
    return screen32_find_page_descriptor(pageId) != nullptr;
}

uint32_t resolve_start_page(uint32_t requestedPage, uint32_t fallbackPage) {
    if (is_valid_page_id(requestedPage)) {
        return requestedPage;
    }
    if (is_valid_page_id(fallbackPage)) {
        return fallbackPage;
    }
    return scr_LOAD;
}

uint32_t resolve_online_start_page(const demo::FrontendConfig& config) {
    return resolve_start_page(config.firstOnlinePage, scr_LOAD);
}

uint32_t resolve_offline_start_page(const demo::FrontendConfig& config) {
    return resolve_start_page(config.firstOfflinePage, scr_LOAD);
}

uint32_t normalize_color(uint32_t value) {
    if (value <= 0xFFFF) {
        const uint8_t r = static_cast<uint8_t>(((value >> 11) & 0x1F) * 255 / 31);
        const uint8_t g = static_cast<uint8_t>(((value >> 5) & 0x3F) * 255 / 63);
        const uint8_t b = static_cast<uint8_t>((value & 0x1F) * 255 / 31);
        return (static_cast<uint32_t>(r) << 16) |
               (static_cast<uint32_t>(g) << 8) |
               static_cast<uint32_t>(b);
    }
    return value & 0xFFFFFF;
}

const char* envelope_payload_name(pb_size_t whichPayload) {
    switch (whichPayload) {
        case Envelope_show_page_tag: return "show_page";
        case Envelope_set_text_tag: return "set_text";
        case Envelope_set_color_tag: return "set_color";
        case Envelope_set_visible_tag: return "set_visible";
        case Envelope_set_value_tag: return "set_value";
        case Envelope_set_batch_tag: return "set_batch";
        case Envelope_button_event_tag: return "button_event";
        case Envelope_input_event_tag: return "input_event";
        case Envelope_heartbeat_tag: return "heartbeat";
        case Envelope_hello_tag: return "hello";
        case Envelope_request_device_info_tag: return "request_device_info";
        case Envelope_request_current_page_tag: return "request_current_page";
        case Envelope_set_element_attribute_tag: return "set_element_attribute";
        case Envelope_page_snapshot_tag: return "page_snapshot";
        case Envelope_attribute_changed_tag: return "attribute_changed";
        default: return "unknown";
    }
}

ButtonAction to_proto_button_action(demo::FrontendButtonAction action) {
    switch (action) {
        case demo::FrontendButtonAction::Push:
            return ButtonAction_PUSH;
        case demo::FrontendButtonAction::Pop:
            return ButtonAction_POP;
        case demo::FrontendButtonAction::Repeat:
            return ButtonAction_REPEAT;
        case demo::FrontendButtonAction::Click:
        default:
            return ButtonAction_CLICK;
    }
}

bool get_label_for_object(lv_obj_t* obj, lv_obj_t*& outLabel) {
    if (obj == nullptr || !lv_obj_is_valid(obj)) {
        return false;
    }

#if LV_USE_LABEL
    if (lv_obj_check_type(obj, &lv_label_class)) {
        outLabel = obj;
        return true;
    }
#endif

    const uint32_t childCount = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < childCount; ++i) {
        lv_obj_t* child = lv_obj_get_child(obj, static_cast<int32_t>(i));
#if LV_USE_LABEL
        if (lv_obj_check_type(child, &lv_label_class)) {
            outLabel = child;
            return true;
        }
#endif
    }

    return false;
}

bool hook_show_page(void* userData, void* pageTarget) {
    (void)userData;
    return demo::screen32_load_page_by_target(pageTarget);
}

bool hook_set_text(void* userData, void* uiObject, const char* text) {
    (void)userData;
    lv_obj_t* obj = static_cast<lv_obj_t*>(uiObject);
    if (obj == nullptr || !lv_obj_is_valid(obj)) {
        return false;
    }

#if LV_USE_DROPDOWN
    if (lv_obj_check_type(obj, &lv_dropdown_class)) {
        lv_dropdown_set_options(obj, text != nullptr ? text : "");
        return true;
    }
#endif

#if LV_USE_CHECKBOX
    if (lv_obj_check_type(obj, &lv_checkbox_class)) {
        lv_checkbox_set_text(obj, text != nullptr ? text : "");
        return true;
    }
#endif

    lv_obj_t* label = nullptr;
    if (!get_label_for_object(obj, label)) {
        return false;
    }

#if LV_USE_LABEL
    lv_label_set_text(label, text != nullptr ? text : "");
    return true;
#else
    (void)text;
    return false;
#endif
}

bool hook_set_value(void* userData, void* uiObject, int32_t value) {
    (void)userData;
    lv_obj_t* obj = static_cast<lv_obj_t*>(uiObject);
    if (obj == nullptr || !lv_obj_is_valid(obj)) {
        return false;
    }

#if LV_USE_SLIDER
    if (lv_obj_check_type(obj, &lv_slider_class)) {
        lv_slider_set_value(obj, value, LV_ANIM_OFF);
        return true;
    }
#endif

#if LV_USE_BAR
    if (lv_obj_check_type(obj, &lv_bar_class)) {
        lv_bar_set_value(obj, value, LV_ANIM_OFF);
        return true;
    }
#endif

#if LV_USE_ARC
    if (lv_obj_check_type(obj, &lv_arc_class)) {
        lv_arc_set_value(obj, value);
        return true;
    }
#endif

#if LV_USE_DROPDOWN
    if (lv_obj_check_type(obj, &lv_dropdown_class)) {
        lv_dropdown_set_selected(obj, value < 0 ? 0U : static_cast<uint32_t>(value));
        return true;
    }
#endif

#if LV_USE_CHECKBOX
    if (lv_obj_check_type(obj, &lv_checkbox_class)) {
        if (value != 0) {
            lv_obj_add_state(obj, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(obj, LV_STATE_CHECKED);
        }
        return true;
    }
#endif

    char text[24] = {};
    snprintf(text, sizeof(text), "%ld", static_cast<long>(value));
    return hook_set_text(nullptr, uiObject, text);
}

bool hook_set_visible(void* userData, void* uiObject, bool visible) {
    (void)userData;
    lv_obj_t* obj = static_cast<lv_obj_t*>(uiObject);
    if (obj == nullptr || !lv_obj_is_valid(obj)) {
        return false;
    }

    if (visible) {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
    return true;
}

bool hook_set_color(void* userData, void* uiObject, uint32_t bgColor, uint32_t fgColor) {
    (void)userData;
    lv_obj_t* obj = static_cast<lv_obj_t*>(uiObject);
    if (obj == nullptr || !lv_obj_is_valid(obj)) {
        return false;
    }

    lv_obj_set_style_bg_color(obj, lv_color_hex(normalize_color(bgColor)), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(obj, lv_color_hex(normalize_color(fgColor)), LV_PART_MAIN | LV_STATE_DEFAULT);
    return true;
}

DeviceInfo build_device_info(demo::FrontendMode mode) {
    DeviceInfo info = DeviceInfo_init_zero;
    char deviceId[sizeof(info.device_id)] = {0};
    char instanceId[sizeof(info.instance_id)] = {0};

#if defined(ARDUINO)
    const uint64_t mac = ESP.getEfuseMac();
    snprintf(deviceId,
             sizeof(deviceId),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             static_cast<unsigned int>((mac >> 40) & 0xFFU),
             static_cast<unsigned int>((mac >> 32) & 0xFFU),
             static_cast<unsigned int>((mac >> 24) & 0xFFU),
             static_cast<unsigned int>((mac >> 16) & 0xFFU),
             static_cast<unsigned int>((mac >> 8) & 0xFFU),
             static_cast<unsigned int>(mac & 0xFFU));
    snprintf(instanceId, sizeof(instanceId), "%s-%d", ESP.getChipModel(), ESP.getChipRevision());
#else
    copy_text_safe(deviceId, sizeof(deviceId), "ip-address");
    copy_text_safe(instanceId, sizeof(instanceId), "screen32");
#endif

    info.protocol_version = 1;
    copy_text_safe(info.fw_version, sizeof(info.fw_version), "1");
    copy_text_safe(info.ui_version, sizeof(info.ui_version), "1");
    copy_text_safe(info.screen_type, sizeof(info.screen_type), "JC8048W550C");
    copy_text_safe(info.client_type, sizeof(info.client_type), demo::frontend_mode_name(mode));
    copy_text_safe(info.device_id, sizeof(info.device_id), deviceId);
    copy_text_safe(info.instance_id, sizeof(info.instance_id), instanceId);
    info.capabilities = 0;
    return info;
}

bool send_snapshot_for_current_page() {
    if (g_state.client == nullptr) {
        return false;
    }

    Envelope env = Envelope_init_zero;
    env.which_payload = Envelope_page_snapshot_tag;
    if (!g_state.adapter.buildPageSnapshot(
            g_state.currentPageId,
            g_state.currentSessionId,
            env.payload.page_snapshot)) {
        SCREENLIB_LOGW(kLogTag,
                       "buildPageSnapshot returned false for page=%lu session=%lu",
                       static_cast<unsigned long>(g_state.currentPageId),
                       static_cast<unsigned long>(g_state.currentSessionId));
    }
    return g_state.client->sendEnvelope(env);
}

bool send_attribute_change(uint32_t elementId,
                           const ElementAttributeValue& value,
                           AttributeChangeReason reason,
                           uint32_t inReplyToRequest) {
    if (g_state.client == nullptr || g_state.offlineDemo || !g_state.client->connected()) {
        return false;
    }

    Envelope env = Envelope_init_zero;
    env.which_payload = Envelope_attribute_changed_tag;
    env.payload.attribute_changed.session_id = g_state.currentSessionId;
    env.payload.attribute_changed.page_id = g_state.currentPageId;
    env.payload.attribute_changed.element_id = elementId;
    env.payload.attribute_changed.has_value = true;
    env.payload.attribute_changed.value = value;
    env.payload.attribute_changed.reason = reason;
    env.payload.attribute_changed.in_reply_to_request = inReplyToRequest;
    return g_state.client->sendEnvelope(env);
}

void on_attribute_change(uint32_t elementId,
                         const ElementAttributeValue& value,
                         AttributeChangeReason reason,
                         void* userData) {
    (void)userData;
    send_attribute_change(elementId, value, reason, 0);
}

bool convert_set_cmd_to_attribute_value(const SetElementAttribute& cmd, ElementAttributeValue& out) {
    out = ElementAttributeValue_init_zero;
    out.attribute = cmd.attribute;

    switch (cmd.which_value) {
        case SetElementAttribute_int_value_tag:
            out.which_value = ElementAttributeValue_int_value_tag;
            out.value.int_value = cmd.value.int_value;
            return true;
        case SetElementAttribute_color_value_tag:
            out.which_value = ElementAttributeValue_color_value_tag;
            out.value.color_value = cmd.value.color_value;
            return true;
        case SetElementAttribute_font_value_tag:
            out.which_value = ElementAttributeValue_font_value_tag;
            out.value.font_value = cmd.value.font_value;
            return true;
        case SetElementAttribute_bool_value_tag:
            out.which_value = ElementAttributeValue_bool_value_tag;
            out.value.bool_value = cmd.value.bool_value;
            return true;
        case SetElementAttribute_string_value_tag:
            out.which_value = ElementAttributeValue_string_value_tag;
            copy_text_safe(out.value.string_value, sizeof(out.value.string_value), cmd.value.string_value);
            return true;
        default:
            return false;
    }
}

void handle_set_element_attribute(const SetElementAttribute& cmd) {
    ElementAttributeValue requested = ElementAttributeValue_init_zero;
    ElementAttributeValue applied = ElementAttributeValue_init_zero;
    if (!convert_set_cmd_to_attribute_value(cmd, requested)) {
        SCREENLIB_LOGW(kLogTag,
                       "set_element_attribute convert failed element=%lu attr=%d",
                       static_cast<unsigned long>(cmd.element_id),
                       static_cast<int>(cmd.attribute));
        return;
    }

    if (!g_state.adapter.applyAttributeValue(cmd.element_id, requested, applied)) {
        SCREENLIB_LOGW(kLogTag,
                       "set_element_attribute apply failed element=%lu attr=%d",
                       static_cast<unsigned long>(cmd.element_id),
                       static_cast<int>(cmd.attribute));
        return;
    }

    if (cmd.request_id != 0) {
        send_attribute_change(
            cmd.element_id,
            applied,
            AttributeChangeReason_REASON_COMMAND_APPLIED,
            cmd.request_id);
    }
}

bool on_adapter_event(const Envelope& source, void* userData) {
    (void)userData;
    if (g_state.client == nullptr || g_state.offlineDemo || !g_state.client->connected()) {
        return false;
    }

    Envelope env = source;
    switch (env.which_payload) {
        case Envelope_button_event_tag:
            env.payload.button_event.session_id = g_state.currentSessionId;
            break;
        case Envelope_input_event_tag:
            env.payload.input_event.session_id = g_state.currentSessionId;
            break;
        default:
            break;
    }

    return g_state.client->sendEnvelope(env);
}

void on_ui_button_event(void* userData, uint32_t elementId, uint32_t pageId, demo::FrontendButtonAction action) {
    State* state = static_cast<State*>(userData);
    if (state == nullptr || !state->initialized) {
        return;
    }

    const ButtonAction protoAction = to_proto_button_action(action);
    if (state->offlineDemo) {
        if (protoAction == ButtonAction_CLICK) {
            state->offlineController.onButtonEvent(elementId, pageId);
        }
        return;
    }

    state->adapter.emitButtonEvent(elementId, pageId, protoAction);
}

void on_ui_object_click(void* userData, uint32_t elementId, uint32_t pageId) {
    State* state = static_cast<State*>(userData);
    (void)elementId;
    if (state == nullptr || !state->initialized) {
        return;
    }

    if (state->offlineDemo) {
        state->offlineController.onObjectClick(pageId);
    }
}

void on_ui_input_event_int(void* userData, uint32_t elementId, uint32_t pageId, int32_t value) {
    State* state = static_cast<State*>(userData);
    if (state == nullptr || !state->initialized) {
        return;
    }

    if (state->offlineDemo) {
        state->offlineController.onInputEventInt(elementId, pageId, value);
        return;
    }

    state->adapter.emitInputEventInt(elementId, pageId, value);
}

void on_ui_input_event_text(void* userData, uint32_t elementId, uint32_t pageId, const char* value) {
    State* state = static_cast<State*>(userData);
    if (state == nullptr || !state->initialized) {
        return;
    }

    if (state->offlineDemo) {
        state->offlineController.onInputEventText(elementId, pageId, value);
        return;
    }

    state->adapter.emitInputEventString(elementId, pageId, value != nullptr ? value : "");
}

void handle_incoming_envelope(const Envelope& env) {
    switch (env.which_payload) {
        case Envelope_show_page_tag: {
            const ShowPage& msg = env.payload.show_page;
            g_state.currentPageId = msg.page_id;
            g_state.currentSessionId = msg.session_id;
            g_state.backendConnected = true;
            g_state.online = true;
            g_state.offlineDemo = false;

            g_state.adapter.showPage(msg.page_id);
            g_state.adapter.installChangeListeners(msg.page_id, &on_attribute_change, nullptr);
            send_snapshot_for_current_page();
            break;
        }
        case Envelope_set_element_attribute_tag:
            handle_set_element_attribute(env.payload.set_element_attribute);
            break;
        case Envelope_set_text_tag:
            g_state.adapter.setText(env.payload.set_text.element_id, env.payload.set_text.text);
            break;
        case Envelope_set_value_tag:
            g_state.adapter.setValue(env.payload.set_value.element_id, env.payload.set_value.value);
            break;
        case Envelope_set_visible_tag:
            g_state.adapter.setVisible(env.payload.set_visible.element_id, env.payload.set_visible.visible);
            break;
        case Envelope_set_color_tag:
            g_state.adapter.setColor(
                env.payload.set_color.element_id,
                env.payload.set_color.bg_color,
                env.payload.set_color.fg_color);
            break;
        case Envelope_set_batch_tag:
            g_state.adapter.applyBatch(env.payload.set_batch);
            break;
        case Envelope_request_device_info_tag:
            if (g_state.client != nullptr) {
                g_state.client->sendHello(build_device_info(g_state.config.mode));
            }
            break;
        case Envelope_request_current_page_tag:
            if (g_state.client != nullptr) {
                g_state.client->sendCurrentPage(
                    demo::screen32_current_page_id(),
                    env.payload.request_current_page.request_id);
            }
            break;
        default:
            break;
    }
}

void on_client_event(const Envelope& env, screenlib::client::ScreenClient::EventDirection direction, void* userData) {
    State* state = static_cast<State*>(userData);
    if (state == nullptr || direction != screenlib::client::ScreenClient::EventDirection::Incoming) {
        return;
    }

    if (!state->backendConnected) {
        state->backendConnected = true;
        SCREENLIB_LOGI(kLogTag, "backend connected: first incoming payload=%s", envelope_payload_name(env.which_payload));
    }

    handle_incoming_envelope(env);
}

void configure_adapter() {
    screenlib::adapter::EezLvglHooks hooks{};
    hooks.showPage = &hook_show_page;
    hooks.setText = &hook_set_text;
    hooks.setValue = &hook_set_value;
    hooks.setVisible = &hook_set_visible;
    hooks.setColor = &hook_set_color;
    g_state.adapter.setHooks(hooks, nullptr);
    g_state.adapter.setObjectMap(&g_state.objectMap);
    g_state.adapter.setEventSink(&on_adapter_event, nullptr);
}

bool bind_generated_ui() {
    const bool mapBound = demo::screen32_bind_generated_ui_map(
        g_state.objectMap,
        g_state.tracked,
        kMaxTrackedElements,
        &g_state.trackedCount);
    if (!mapBound) {
        SCREENLIB_LOGW(kLogTag, "generated UI map has unbound entries");
    }
    SCREENLIB_LOGI(kLogTag, "tracked elements: %u", static_cast<unsigned>(g_state.trackedCount));
    return mapBound;
}

bool start_online_mode(const demo::FrontendConfig& config) {
    if (g_state.transport == nullptr) {
        return false;
    }

    demo::offline_demo_ui_events_set_enabled(false);
    g_state.offlineDemo = false;
    g_state.online = true;
    g_state.backendConnected = false;
    g_state.client.reset(new screenlib::client::ScreenClient(*g_state.transport));
    g_state.client->setEventHandler(&on_client_event, &g_state);
    g_state.client->init();

    const uint32_t startPage = resolve_online_start_page(config);
    g_state.currentPageId = startPage;
    g_state.adapter.showPage(startPage);
    SCREENLIB_LOGI(kLogTag, "online mode enabled; start page=%lu", static_cast<unsigned long>(startPage));

    const bool helloOk = g_state.client->sendHello(build_device_info(g_state.config.mode));
    g_state.lastHelloMs = ::platform_tick_ms();
    g_state.waitStartMs = g_state.lastHelloMs;
    g_state.lastWaitLogMs = g_state.waitStartMs;
    SCREENLIB_LOGI(kLogTag, "hello sent: %s", helloOk ? "ok" : "fail");
    return true;
}

bool start_offline_demo_mode(const demo::FrontendConfig& config) {
    g_state.client.reset();
    g_state.transport.reset();
    demo::offline_demo_ui_events_set_enabled(true);
    g_state.offlineDemo = true;
    g_state.online = false;
    g_state.backendConnected = false;
    g_state.currentSessionId = 0;
    g_state.waitStartMs = 0;
    g_state.lastWaitLogMs = 0;

    g_state.offlineController.init(&g_state.adapter, g_state.tracked, g_state.trackedCount);
    const uint32_t startPage = resolve_offline_start_page(config);
    g_state.currentPageId = startPage;
    const bool started = g_state.offlineController.start(startPage);
    if (!started) {
        const uint32_t fallbackPage = resolve_offline_start_page(demo::frontend_default_config());
        g_state.currentPageId = fallbackPage;
        SCREENLIB_LOGW(kLogTag,
                       "offline start failed, fallback page=%lu",
                       static_cast<unsigned long>(fallbackPage));
        g_state.adapter.showPage(fallbackPage);
    } else {
        SCREENLIB_LOGI(kLogTag, "offline demo mode enabled; start page=%lu", static_cast<unsigned long>(startPage));
    }
    return true;
}

} // namespace

bool init(const demo::FrontendConfig& config) {
    if (g_state.initialized) {
        return true;
    }

    g_state.config = config;
    g_state.lastHeartbeatMs = ::platform_tick_ms();
    g_state.lastHelloMs = g_state.lastHeartbeatMs;
    g_state.backendConnected = false;

    SCREENLIB_LOGI(kLogTag,
                   "init mode=%s transport=%s offline_demo=%d online_page=%lu offline_page=%lu hb_ms=%lu",
                   demo::frontend_mode_name(config.mode),
                   demo::frontend_transport_name(config.transport.type),
                   config.offlineDemo ? 1 : 0,
                   static_cast<unsigned long>(config.firstOnlinePage),
                   static_cast<unsigned long>(config.firstOfflinePage),
                   static_cast<unsigned long>(config.heartbeatPeriodMs));

    configure_adapter();
    bind_generated_ui();

    demo::FrontendUiEventSink sink{};
    sink.userData = &g_state;
    sink.onButtonEvent = &on_ui_button_event;
    sink.onInputEventInt = &on_ui_input_event_int;
    sink.onInputEventText = &on_ui_input_event_text;
    demo::frontend_ui_events_attach_generated(g_state.tracked, g_state.trackedCount, sink);
    demo::offline_demo_ui_events_init(g_state.tracked, g_state.trackedCount, &on_ui_object_click, &g_state);

    bool useOfflineDemo = config.offlineDemo || config.transport.type == demo::FrontendTransportType::None;
    if (!useOfflineDemo) {
        g_state.transport = demo::platform_create_transport(config);
        if (!g_state.transport) {
            SCREENLIB_LOGW(kLogTag, "transport init failed, fallback to offline demo");
            useOfflineDemo = true;
        }
    }

    const bool ready = useOfflineDemo ? start_offline_demo_mode(config) : start_online_mode(config);
    g_state.initialized = ready;
    return ready;
}

void tick() {
    if (!g_state.initialized) {
        return;
    }

    if (g_state.online && g_state.client != nullptr) {
        g_state.client->tick();

        const uint32_t now = ::platform_tick_ms();
        if (!g_state.backendConnected && (now - g_state.lastHelloMs) >= kHelloRetryPeriodMs) {
            const bool helloOk = g_state.client->sendHello(build_device_info(g_state.config.mode));
            g_state.lastHelloMs = now;
            SCREENLIB_LOGD(kLogTag, "hello retry: %s", helloOk ? "ok" : "fail");
        }

        if (!g_state.backendConnected && g_state.config.offlineTimeoutMs > 0) {
            if ((now - g_state.lastWaitLogMs) >= kWaitLogPeriodMs) {
                const uint32_t elapsedSec = (now - g_state.waitStartMs) / 1000U;
                SCREENLIB_LOGI(kLogTag,
                               "waiting backend... %lus elapsed",
                               static_cast<unsigned long>(elapsedSec));
                g_state.lastWaitLogMs = now;
            }

            if ((now - g_state.waitStartMs) >= g_state.config.offlineTimeoutMs) {
                SCREENLIB_LOGW(kLogTag, "fallback to demo: backend wait timeout reached");
                if (!switch_to_offline_demo()) {
                    g_state.waitStartMs = now;
                    g_state.lastWaitLogMs = now;
                    SCREENLIB_LOGI(kLogTag,
                                   "backend still unavailable; continue waiting (next timeout=%lus)",
                                   static_cast<unsigned long>(g_state.config.offlineTimeoutMs / 1000U));
                }
                return;
            }
        }

        if (g_state.config.heartbeatPeriodMs > 0 &&
            (now - g_state.lastHeartbeatMs) >= g_state.config.heartbeatPeriodMs) {
            g_state.client->sendHeartbeat(now);
            g_state.lastHeartbeatMs = now;
        }
    }

    g_state.adapter.flushPendingChanges();
}

bool is_online() {
    return g_state.online;
}

bool is_offline_demo() {
    return g_state.offlineDemo;
}

bool switch_to_offline_demo() {
    if (!g_state.initialized) {
        return false;
    }
    if (g_state.offlineDemo) {
        return true;
    }
    if (!g_state.config.offlineDemo) {
        SCREENLIB_LOGW(kLogTag, "offline demo disabled by config");
        return false;
    }

    SCREENLIB_LOGW(kLogTag, "switching runtime to offline demo mode");
    return start_offline_demo_mode(g_state.config);
}

bool backend_connected() {
    return g_state.backendConnected;
}

uint32_t current_page() {
    return g_state.currentPageId != 0 ? g_state.currentPageId : demo::screen32_current_page_id();
}

uint32_t current_session_id() {
    return g_state.currentSessionId;
}

} // namespace frontapp
