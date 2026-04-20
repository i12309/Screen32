#include "common_app/frontend_runtime.h"

#include <memory>
#include <stdio.h>

#include <lvgl.h>

#include "common_app/app_core.h"
#include "common_app/frontend_platform.h"
#include "common_app/frontend_service_responder.h"
#include "common_app/frontend_ui_events.h"
#include "page_descriptors.generated.h"
#include "ui_object_map.generated.h"
#include "common_app/offline_demo_controller.h"
#include "lvgl_eez/EezLvglAdapter.h"
#include "lvgl_eez/UiObjectMap.h"
#include "runtime/ScreenClient.h"

namespace demo {

namespace {

constexpr size_t kMaxObjectBindings = SCREEN32_ELEMENT_DESCRIPTOR_COUNT;
constexpr size_t kMaxPageBindings = SCREEN32_PAGE_DESCRIPTOR_COUNT;
constexpr size_t kMaxTrackedElements = SCREEN32_ELEMENT_DESCRIPTOR_COUNT;
constexpr uint32_t kHeartbeatPeriodMs = 1000;

class NullTransport : public ITransport {
public:
    bool connected() const override { return false; }
    bool write(const uint8_t* data, size_t len) override {
        (void)data;
        (void)len;
        return false;
    }
    size_t read(uint8_t* dst, size_t max_len) override {
        (void)dst;
        (void)max_len;
        return 0;
    }
    void tick() override {}
};

// RuntimeState принадлежит слою оркестрации frontend_runtime и связывает
// все части, зависящие от режима: transport, client, adapter и offline-контроллер.
struct RuntimeState {
    FrontendConfig config = frontend_default_config();
    bool initialized = false;
    bool offlineDemo = true;
    bool online = false;
    uint32_t lastHeartbeatMs = 0;

    NullTransport nullTransport;
    std::unique_ptr<ITransport> transport;
    std::unique_ptr<screenlib::client::ScreenClient> client;

    screenlib::adapter::UiObjectBinding objectBindings[kMaxObjectBindings] = {};
    screenlib::adapter::UiPageBinding pageBindings[kMaxPageBindings] = {};
    screenlib::adapter::UiObjectMap objectMap;
    screenlib::adapter::EezLvglAdapter adapter;
    OfflineDemoController offlineController;

    Screen32BoundElement tracked[kMaxTrackedElements] = {};
    size_t trackedCount = 0;

    RuntimeState()
        : objectMap(objectBindings, kMaxObjectBindings, pageBindings, kMaxPageBindings),
          adapter(&objectMap) {}
};

RuntimeState g_state;

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

uint32_t resolve_online_start_page(const FrontendConfig& config) {
    return resolve_start_page(config.firstOnlinePage, scr_LOAD);
}

uint32_t resolve_offline_start_page(const FrontendConfig& config) {
    return resolve_start_page(config.firstOfflinePage, scr_LOAD);
}

const char* envelope_payload_name(pb_size_t whichPayload) {
    switch (whichPayload) {
        case Envelope_show_page_tag:
            return "show_page";
        case Envelope_set_text_tag:
            return "set_text";
        case Envelope_set_color_tag:
            return "set_color";
        case Envelope_set_visible_tag:
            return "set_visible";
        case Envelope_set_value_tag:
            return "set_value";
        case Envelope_set_batch_tag:
            return "set_batch";
        case Envelope_button_event_tag:
            return "button_event";
        case Envelope_input_event_tag:
            return "input_event";
        case Envelope_heartbeat_tag:
            return "heartbeat";
        case Envelope_hello_tag:
            return "hello";
        case Envelope_request_device_info_tag:
            return "request_device_info";
        case Envelope_device_info_tag:
            return "device_info";
        case Envelope_request_current_page_tag:
            return "request_current_page";
        case Envelope_current_page_tag:
            return "current_page";
        case Envelope_request_page_state_tag:
            return "request_page_state";
        case Envelope_page_state_tag:
            return "page_state";
        case Envelope_request_element_state_tag:
            return "request_element_state";
        case Envelope_element_state_tag:
            return "element_state";
        default:
            return "unknown";
    }
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
    return screen32_load_page_by_target(pageTarget);
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

void on_ui_button_event(void* userData, uint32_t elementId, uint32_t pageId) {
    RuntimeState* state = static_cast<RuntimeState*>(userData);
    if (state == nullptr || !state->initialized) {
        return;
    }

    if (state->offlineDemo) {
        state->offlineController.onButtonEvent(elementId, pageId);
        return;
    }

    printf("[frontend_runtime] tx button_event page=%lu element=%lu\n",
           static_cast<unsigned long>(pageId),
           static_cast<unsigned long>(elementId));
    state->adapter.emitButtonEvent(elementId, pageId);
}

void on_ui_object_click(void* userData, uint32_t elementId, uint32_t pageId) {
    RuntimeState* state = static_cast<RuntimeState*>(userData);
    (void)elementId;
    if (state == nullptr || !state->initialized) {
        return;
    }

    if (state->offlineDemo) {
        state->offlineController.onObjectClick(pageId);
    }
}

void on_ui_input_event_int(void* userData, uint32_t elementId, uint32_t pageId, int32_t value) {
    RuntimeState* state = static_cast<RuntimeState*>(userData);
    if (state == nullptr || !state->initialized) {
        return;
    }

    if (state->offlineDemo) {
        state->offlineController.onInputEventInt(elementId, pageId, value);
        return;
    }

    printf("[frontend_runtime] tx input_event[int] page=%lu element=%lu value=%ld\n",
           static_cast<unsigned long>(pageId),
           static_cast<unsigned long>(elementId),
           static_cast<long>(value));
    state->adapter.emitInputEventInt(elementId, pageId, value);
}

void on_ui_input_event_text(void* userData, uint32_t elementId, uint32_t pageId, const char* value) {
    RuntimeState* state = static_cast<RuntimeState*>(userData);
    if (state == nullptr || !state->initialized) {
        return;
    }

    if (state->offlineDemo) {
        state->offlineController.onInputEventText(elementId, pageId, value);
        return;
    }

    printf("[frontend_runtime] tx input_event[text] page=%lu element=%lu value=\"%s\"\n",
           static_cast<unsigned long>(pageId),
           static_cast<unsigned long>(elementId),
           value != nullptr ? value : "");
    state->adapter.emitInputEventString(elementId, pageId, value != nullptr ? value : "");
}

void on_client_event(const Envelope& env, screenlib::client::ScreenClient::EventDirection direction, void* userData) {
    RuntimeState* state = static_cast<RuntimeState*>(userData);
    if (state == nullptr || state->client == nullptr) {
        return;
    }
    if (direction != screenlib::client::ScreenClient::EventDirection::Incoming) {
        return;
    }

    switch (env.which_payload) {
        case Envelope_show_page_tag:
            printf("[frontend_runtime] rx show_page page=%lu\n",
                   static_cast<unsigned long>(env.payload.show_page.page_id));
            break;
        case Envelope_set_text_tag:
            printf("[frontend_runtime] rx set_text element=%lu text=\"%s\"\n",
                   static_cast<unsigned long>(env.payload.set_text.element_id),
                   env.payload.set_text.text);
            break;
        case Envelope_set_value_tag:
            printf("[frontend_runtime] rx set_value element=%lu value=%ld\n",
                   static_cast<unsigned long>(env.payload.set_value.element_id),
                   static_cast<long>(env.payload.set_value.value));
            break;
        case Envelope_set_visible_tag:
            printf("[frontend_runtime] rx set_visible element=%lu visible=%d\n",
                   static_cast<unsigned long>(env.payload.set_visible.element_id),
                   env.payload.set_visible.visible ? 1 : 0);
            break;
        case Envelope_set_batch_tag:
            printf("[frontend_runtime] rx set_batch texts=%u colors=%u visibles=%u values=%u\n",
                   static_cast<unsigned>(env.payload.set_batch.texts_count),
                   static_cast<unsigned>(env.payload.set_batch.colors_count),
                   static_cast<unsigned>(env.payload.set_batch.visibles_count),
                   static_cast<unsigned>(env.payload.set_batch.values_count));
            break;
        case Envelope_heartbeat_tag:
            break;
        default:
            printf("[frontend_runtime] rx %s (%u)\n",
                   envelope_payload_name(env.which_payload),
                   static_cast<unsigned>(env.which_payload));
            break;
    }

    FrontendServiceResponderContext responder{};
    responder.client = state->client.get();
    responder.trackedElements = state->tracked;
    responder.trackedCount = state->trackedCount;
    responder.mode = state->config.mode;
    frontend_handle_service_request(env, responder);
}

} // namespace

bool frontend_runtime_init(const FrontendConfig& config) {
    if (g_state.initialized) {
        return true;
    }

    g_state.config = config;
    g_state.offlineDemo = config.offlineDemo || config.transport.type == FrontendTransportType::None;
    g_state.online = !g_state.offlineDemo;
    g_state.lastHeartbeatMs = platform_tick_ms();
    printf("[frontend_runtime] init mode=%s transport=%s offline_demo=%d online_page=%lu offline_page=%lu\n",
           frontend_mode_name(config.mode),
           frontend_transport_name(config.transport.type),
           g_state.offlineDemo ? 1 : 0,
           static_cast<unsigned long>(config.firstOnlinePage),
           static_cast<unsigned long>(config.firstOfflinePage));

    screenlib::adapter::EezLvglHooks hooks{};
    hooks.showPage = &hook_show_page;
    hooks.setText = &hook_set_text;
    hooks.setValue = &hook_set_value;
    hooks.setVisible = &hook_set_visible;
    hooks.setColor = &hook_set_color;
    g_state.adapter.setHooks(hooks, nullptr);
    g_state.adapter.setObjectMap(&g_state.objectMap);

    const bool mapBound = screen32_bind_generated_ui_map(
        g_state.objectMap,
        g_state.tracked,
        kMaxTrackedElements,
        &g_state.trackedCount);
    if (!mapBound) {
        printf("[frontend_runtime] warning: generated UI map has unbound entries\n");
    }
    printf("[frontend_runtime] tracked elements: %u\n", static_cast<unsigned>(g_state.trackedCount));

    if (!g_state.offlineDemo) {
        g_state.transport = platform_create_transport(config);
        if (!g_state.transport) {
            printf("[frontend_runtime] transport init failed, fallback to offline demo\n");
            g_state.offlineDemo = true;
            g_state.online = false;
        }
    }

    FrontendUiEventSink sink{};
    sink.userData = &g_state;
    sink.onButtonEvent = &on_ui_button_event;
    sink.onObjectClick = g_state.offlineDemo ? &on_ui_object_click : nullptr;
    sink.onInputEventInt = &on_ui_input_event_int;
    sink.onInputEventText = &on_ui_input_event_text;
    frontend_ui_events_attach_generated(g_state.tracked, g_state.trackedCount, sink);

    if (g_state.online) {
        printf("[frontend_runtime] online mode enabled\n");
        ITransport* transport = g_state.transport ? g_state.transport.get() : static_cast<ITransport*>(&g_state.nullTransport);
        g_state.client.reset(new screenlib::client::ScreenClient(*transport));
        g_state.client->setUiAdapter(&g_state.adapter);
        g_state.client->setEventHandler(&on_client_event, &g_state);
        g_state.client->init();

        const uint32_t startPage = resolve_online_start_page(config);
        g_state.adapter.showPage(startPage);
        printf("[frontend_runtime] online start page=%lu\n", static_cast<unsigned long>(startPage));
        g_state.client->sendHello(frontend_build_device_info(g_state.config.mode));
        printf("[frontend_runtime] hello sent\n");
    } else {
        printf("[frontend_runtime] offline demo mode enabled\n");
        g_state.client.reset();
        g_state.offlineController.init(&g_state.adapter, g_state.tracked, g_state.trackedCount);
        const uint32_t startPage = resolve_offline_start_page(config);
        const bool started = g_state.offlineController.start(startPage);
        if (!started) {
            const uint32_t fallbackPage = resolve_offline_start_page(frontend_default_config());
            printf("[frontend_runtime] offline start failed, fallback page=%lu\n",
                   static_cast<unsigned long>(fallbackPage));
            g_state.adapter.showPage(fallbackPage);
        } else {
            printf("[frontend_runtime] offline start page=%lu\n", static_cast<unsigned long>(startPage));
        }
    }

    g_state.initialized = true;
    return true;
}

void frontend_runtime_tick() {
    if (!g_state.initialized || !g_state.online || g_state.client == nullptr) {
        return;
    }

    g_state.client->tick();

    const uint32_t now = platform_tick_ms();
    if (now - g_state.lastHeartbeatMs >= kHeartbeatPeriodMs) {
        g_state.client->sendHeartbeat(now);
        g_state.lastHeartbeatMs = now;
    }
}

bool frontend_runtime_is_online() {
    return g_state.online;
}

bool frontend_runtime_is_offline_demo() {
    return g_state.offlineDemo;
}

uint32_t frontend_runtime_current_page() {
    return screen32_current_page_id();
}

} // namespace demo


