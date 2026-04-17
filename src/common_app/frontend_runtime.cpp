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

    if (!g_state.offlineDemo) {
        g_state.transport = platform_create_transport(config);
        if (!g_state.transport) {
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
        ITransport* transport = g_state.transport ? g_state.transport.get() : static_cast<ITransport*>(&g_state.nullTransport);
        g_state.client.reset(new screenlib::client::ScreenClient(*transport));
        g_state.client->setUiAdapter(&g_state.adapter);
        g_state.client->setEventHandler(&on_client_event, &g_state);
        g_state.client->init();

        g_state.adapter.showPage(resolve_online_start_page(config));
        g_state.client->sendHello(frontend_build_device_info(g_state.config.mode));
    } else {
        g_state.client.reset();
        g_state.offlineController.init(&g_state.adapter);
        const bool started = g_state.offlineController.start(resolve_offline_start_page(config));
        if (!started) {
            g_state.adapter.showPage(resolve_offline_start_page(frontend_default_config()));
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


