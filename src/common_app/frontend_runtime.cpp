#include "common_app/frontend_runtime.h"

#include <memory>
#include <stdio.h>
#include <string.h>

#include <lvgl.h>

#include "common_app/app_core.h"
#include "common_app/frontend_platform.h"
#include "runtime/ScreenClient.h"
#include "lvgl_eez/EezLvglAdapter.h"
#include "lvgl_eez/UiObjectMap.h"

extern "C" {
#include "ui/screens.h"
#include "ui/ui.h"
}

namespace demo {

namespace {

constexpr size_t kMaxObjectBindings = 96;
constexpr size_t kMaxPageBindings = 8;
constexpr size_t kMaxTrackedElements = 96;
constexpr uint32_t kHeartbeatPeriodMs = 1000;

enum ElementId : uint32_t {
    E_MAIN_TASK = 1001,
    E_MAIN_PROFILE = 1002,
    E_MAIN_NET = 1003,
    E_MAIN_SERVICE = 1004,
    E_MAIN_STATS = 1005,
    E_MAIN_SUPPORT = 1006,

    E_PAGE1_BACK = 1101,
    E_PAGE1_NEXT = 1102,
    E_PAGE1_TITLE = 1103,
    E_PAGE1_B1 = 1104,
    E_PAGE1_B2 = 1105,
    E_PAGE1_B3 = 1106,
    E_PAGE1_B4 = 1107,
    E_PAGE1_B5 = 1108,
    E_PAGE1_B6 = 1109,

    E_PAGE2_BACK = 1201,
    E_PAGE2_NEXT3 = 1202,
    E_PAGE2_NEXT4 = 1203,
    E_PAGE2_NEXT5 = 1204,
    E_PAGE2_TITLE = 1205,

    E_PAGE3_BACK = 1301,
    E_PAGE3_NEXT7 = 1302,
    E_PAGE3_NEXT8 = 1303,
    E_PAGE3_NEXT9 = 1304,
    E_PAGE3_TITLE = 1305,

    E_PAGE4_BACK = 1401,
    E_PAGE4_NEXT10 = 1402,
    E_PAGE4_NEXT11 = 1403,
    E_PAGE4_NEXT12 = 1404,
    E_PAGE4_TITLE = 1405,
};

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

struct TrackedElement {
    uint32_t elementId = 0;
    uint32_t pageId = 0;
    lv_obj_t* obj = nullptr;
};

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

    TrackedElement tracked[kMaxTrackedElements] = {};
    size_t trackedCount = 0;

    RuntimeState()
        : objectMap(objectBindings, kMaxObjectBindings, pageBindings, kMaxPageBindings),
          adapter(&objectMap) {}
};

RuntimeState g_state;

uint32_t current_page_id() {
    lv_obj_t* active = lv_scr_act();
    if (active == objects.load) return SCREEN_ID_LOAD;
    if (active == objects.main_menu) return SCREEN_ID_MAIN_MENU;
    if (active == objects.def_page1) return SCREEN_ID_DEF_PAGE1;
    if (active == objects.def_page2) return SCREEN_ID_DEF_PAGE2;
    if (active == objects.def_page3) return SCREEN_ID_DEF_PAGE3;
    if (active == objects.def_page4) return SCREEN_ID_DEF_PAGE4;
    return SCREEN_ID_MAIN_MENU;
}

void copy_text_safe(char* dst, size_t dstSize, const char* src) {
    if (dst == nullptr || dstSize == 0) return;
    dst[0] = '\0';
    if (src == nullptr) return;
    strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
}

uint32_t normalize_color(uint32_t value) {
    // Accept both rgb565 and 24-bit RGB inputs.
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

bool read_element_text(lv_obj_t* obj, char* outText, size_t outSize) {
    if (obj == nullptr || outText == nullptr || outSize == 0) {
        return false;
    }

    lv_obj_t* label = nullptr;
    if (!get_label_for_object(obj, label)) {
        return false;
    }

#if LV_USE_LABEL
    const char* text = lv_label_get_text(label);
    copy_text_safe(outText, outSize, text);
    return true;
#else
    (void)label;
    return false;
#endif
}

bool read_element_int_value(lv_obj_t* obj, int32_t& outValue) {
    if (obj == nullptr || !lv_obj_is_valid(obj)) {
        return false;
    }

#if LV_USE_SLIDER
    if (lv_obj_check_type(obj, &lv_slider_class)) {
        outValue = lv_slider_get_value(obj);
        return true;
    }
#endif

#if LV_USE_BAR
    if (lv_obj_check_type(obj, &lv_bar_class)) {
        outValue = lv_bar_get_value(obj);
        return true;
    }
#endif

#if LV_USE_ARC
    if (lv_obj_check_type(obj, &lv_arc_class)) {
        outValue = lv_arc_get_value(obj);
        return true;
    }
#endif

    return false;
}

bool hook_show_page(void* userData, void* pageTarget) {
    (void)userData;
    lv_obj_t* target = static_cast<lv_obj_t*>(pageTarget);
    if (target == nullptr) {
        return false;
    }

    if (target == objects.load) {
        loadScreen(SCREEN_ID_LOAD);
        return true;
    }
    if (target == objects.main_menu) {
        loadScreen(SCREEN_ID_MAIN_MENU);
        return true;
    }
    if (target == objects.def_page1) {
        loadScreen(SCREEN_ID_DEF_PAGE1);
        return true;
    }
    if (target == objects.def_page2) {
        loadScreen(SCREEN_ID_DEF_PAGE2);
        return true;
    }
    if (target == objects.def_page3) {
        loadScreen(SCREEN_ID_DEF_PAGE3);
        return true;
    }
    if (target == objects.def_page4) {
        loadScreen(SCREEN_ID_DEF_PAGE4);
        return true;
    }

    return false;
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

TrackedElement* find_tracked_element_by_id(uint32_t elementId) {
    for (size_t i = 0; i < g_state.trackedCount; ++i) {
        if (g_state.tracked[i].elementId == elementId) {
            return &g_state.tracked[i];
        }
    }
    return nullptr;
}

void on_ui_event_cb(lv_event_t* e) {
    if (e == nullptr || g_state.client == nullptr) {
        return;
    }

    const uint32_t elementId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    const uint32_t pageId = current_page_id();
    const lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));

    if (code == LV_EVENT_CLICKED) {
        g_state.adapter.emitButtonEvent(elementId, pageId);
        return;
    }

    if (code != LV_EVENT_VALUE_CHANGED || target == nullptr) {
        return;
    }

#if LV_USE_TEXTAREA
    if (lv_obj_check_type(target, &lv_textarea_class)) {
        g_state.adapter.emitInputEventString(elementId, pageId, lv_textarea_get_text(target));
        return;
    }
#endif

    int32_t numericValue = 0;
    if (read_element_int_value(target, numericValue)) {
        g_state.adapter.emitInputEventInt(elementId, pageId, numericValue);
    }
}

bool bind_page(uint32_t pageId, lv_obj_t* pageObj) {
    return pageObj != nullptr && g_state.objectMap.bindPage(pageId, pageObj);
}

bool bind_element(uint32_t elementId, uint32_t pageId, lv_obj_t* obj) {
    if (obj == nullptr) {
        return false;
    }
    if (!g_state.objectMap.bindElement(elementId, obj)) {
        return false;
    }
    if (g_state.trackedCount < kMaxTrackedElements) {
        g_state.tracked[g_state.trackedCount++] = TrackedElement{elementId, pageId, obj};
    }

    lv_obj_add_event_cb(obj, on_ui_event_cb, LV_EVENT_CLICKED, reinterpret_cast<void*>(static_cast<uintptr_t>(elementId)));
    lv_obj_add_event_cb(obj, on_ui_event_cb, LV_EVENT_VALUE_CHANGED, reinterpret_cast<void*>(static_cast<uintptr_t>(elementId)));
    return true;
}

void bind_default_ui_map() {
    g_state.objectMap.clear();
    g_state.trackedCount = 0;

    bind_page(SCREEN_ID_LOAD, objects.load);
    bind_page(SCREEN_ID_MAIN_MENU, objects.main_menu);
    bind_page(SCREEN_ID_DEF_PAGE1, objects.def_page1);
    bind_page(SCREEN_ID_DEF_PAGE2, objects.def_page2);
    bind_page(SCREEN_ID_DEF_PAGE3, objects.def_page3);
    bind_page(SCREEN_ID_DEF_PAGE4, objects.def_page4);

    bind_element(E_MAIN_TASK, SCREEN_ID_MAIN_MENU, objects.b_main_task);
    bind_element(E_MAIN_PROFILE, SCREEN_ID_MAIN_MENU, objects.b_main_profile);
    bind_element(E_MAIN_NET, SCREEN_ID_MAIN_MENU, objects.b_main_net);
    bind_element(E_MAIN_SERVICE, SCREEN_ID_MAIN_MENU, objects.b_main_service);
    bind_element(E_MAIN_STATS, SCREEN_ID_MAIN_MENU, objects.b_main_stats);
    bind_element(E_MAIN_SUPPORT, SCREEN_ID_MAIN_MENU, objects.b_main_support);

    bind_element(E_PAGE1_BACK, SCREEN_ID_DEF_PAGE1, objects.back);
    bind_element(E_PAGE1_NEXT, SCREEN_ID_DEF_PAGE1, objects.next_2);
    bind_element(E_PAGE1_TITLE, SCREEN_ID_DEF_PAGE1, objects.title);
    bind_element(E_PAGE1_B1, SCREEN_ID_DEF_PAGE1, objects.b1);
    bind_element(E_PAGE1_B2, SCREEN_ID_DEF_PAGE1, objects.b2);
    bind_element(E_PAGE1_B3, SCREEN_ID_DEF_PAGE1, objects.b3);
    bind_element(E_PAGE1_B4, SCREEN_ID_DEF_PAGE1, objects.b4);
    bind_element(E_PAGE1_B5, SCREEN_ID_DEF_PAGE1, objects.b5);
    bind_element(E_PAGE1_B6, SCREEN_ID_DEF_PAGE1, objects.b6);

    bind_element(E_PAGE2_BACK, SCREEN_ID_DEF_PAGE2, objects.back_1);
    bind_element(E_PAGE2_NEXT3, SCREEN_ID_DEF_PAGE2, objects.next_3);
    bind_element(E_PAGE2_NEXT4, SCREEN_ID_DEF_PAGE2, objects.next_4);
    bind_element(E_PAGE2_NEXT5, SCREEN_ID_DEF_PAGE2, objects.next_5);
    bind_element(E_PAGE2_TITLE, SCREEN_ID_DEF_PAGE2, objects.title_1);

    bind_element(E_PAGE3_BACK, SCREEN_ID_DEF_PAGE3, objects.back_3);
    bind_element(E_PAGE3_NEXT7, SCREEN_ID_DEF_PAGE3, objects.next_7);
    bind_element(E_PAGE3_NEXT8, SCREEN_ID_DEF_PAGE3, objects.next_8);
    bind_element(E_PAGE3_NEXT9, SCREEN_ID_DEF_PAGE3, objects.next_9);
    bind_element(E_PAGE3_TITLE, SCREEN_ID_DEF_PAGE3, objects.title_3);

    bind_element(E_PAGE4_BACK, SCREEN_ID_DEF_PAGE4, objects.back_4);
    bind_element(E_PAGE4_NEXT10, SCREEN_ID_DEF_PAGE4, objects.next_10);
    bind_element(E_PAGE4_NEXT11, SCREEN_ID_DEF_PAGE4, objects.next_11);
    bind_element(E_PAGE4_NEXT12, SCREEN_ID_DEF_PAGE4, objects.next_12);
    bind_element(E_PAGE4_TITLE, SCREEN_ID_DEF_PAGE4, objects.title_4);
}

DeviceInfo make_device_info() {
    DeviceInfo info = DeviceInfo_init_zero;
    info.protocol_version = 1;
    copy_text_safe(info.fw_version, sizeof(info.fw_version), "screen32-frontend");
    copy_text_safe(info.ui_version, sizeof(info.ui_version), "eez-lvgl");
    copy_text_safe(info.screen_type, sizeof(info.screen_type), "screen32");
    copy_text_safe(info.client_type, sizeof(info.client_type), frontend_mode_name(g_state.config.mode));
    copy_text_safe(info.device_id, sizeof(info.device_id), "screen32-demo");
    copy_text_safe(info.instance_id, sizeof(info.instance_id), "default");
    info.capabilities = 0;
    return info;
}

bool fill_page_element_state(const TrackedElement& tracked, PageElementState& outState) {
    if (tracked.obj == nullptr || !lv_obj_is_valid(tracked.obj)) {
        return false;
    }

    outState = PageElementState_init_zero;
    outState.element_id = tracked.elementId;

    char text[65] = {};
    if (read_element_text(tracked.obj, text, sizeof(text))) {
        outState.type = ElementStateType_ELEMENT_STATE_TEXT;
        outState.which_value = PageElementState_text_value_tag;
        copy_text_safe(outState.value.text_value, sizeof(outState.value.text_value), text);
        return true;
    }

    int32_t value = 0;
    if (read_element_int_value(tracked.obj, value)) {
        outState.type = ElementStateType_ELEMENT_STATE_INT;
        outState.which_value = PageElementState_int_value_tag;
        outState.value.int_value = value;
        return true;
    }

    outState.type = ElementStateType_ELEMENT_STATE_VISIBLE;
    outState.which_value = PageElementState_visible_tag;
    outState.value.visible = !lv_obj_has_flag(tracked.obj, LV_OBJ_FLAG_HIDDEN);
    return true;
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
        case Envelope_request_device_info_tag: {
            state->client->sendDeviceInfo(make_device_info());
            break;
        }
        case Envelope_request_current_page_tag: {
            const uint32_t requestId = env.payload.request_current_page.request_id;
            state->client->sendCurrentPage(current_page_id(), requestId);
            break;
        }
        case Envelope_request_page_state_tag: {
            PageState pageState = PageState_init_zero;
            pageState.request_id = env.payload.request_page_state.request_id;
            const uint32_t requestedPage = env.payload.request_page_state.page_id;
            pageState.page_id = (requestedPage == 0) ? current_page_id() : requestedPage;
            pageState.elements_count = 0;

            for (size_t i = 0; i < state->trackedCount && pageState.elements_count < 8; ++i) {
                if (state->tracked[i].pageId != pageState.page_id) {
                    continue;
                }
                if (fill_page_element_state(state->tracked[i], pageState.elements[pageState.elements_count])) {
                    pageState.elements_count++;
                }
            }
            state->client->sendPageState(pageState);
            break;
        }
        case Envelope_request_element_state_tag: {
            ElementState elementState = ElementState_init_zero;
            elementState.request_id = env.payload.request_element_state.request_id;
            elementState.page_id = env.payload.request_element_state.page_id;
            if (elementState.page_id == 0) {
                elementState.page_id = current_page_id();
            }

            TrackedElement* tracked = find_tracked_element_by_id(env.payload.request_element_state.element_id);
            if (tracked != nullptr && (tracked->pageId == elementState.page_id)) {
                elementState.found = fill_page_element_state(*tracked, elementState.element);
                elementState.has_element = elementState.found;
            } else {
                elementState.found = false;
                elementState.has_element = false;
            }

            state->client->sendElementState(elementState);
            break;
        }
        default:
            break;
    }
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

    bind_default_ui_map();

    if (!g_state.offlineDemo) {
        g_state.transport = platform_create_transport(config);
        if (!g_state.transport) {
            g_state.offlineDemo = true;
            g_state.online = false;
        }
    }

    ITransport* transport = g_state.transport ? g_state.transport.get() : static_cast<ITransport*>(&g_state.nullTransport);
    g_state.client.reset(new screenlib::client::ScreenClient(*transport));
    g_state.client->setUiAdapter(&g_state.adapter);
    g_state.client->setEventHandler(&on_client_event, &g_state);
    g_state.client->init();

    if (g_state.online) {
        g_state.client->sendHello(make_device_info());
    }

    g_state.initialized = true;
    return true;
}

void frontend_runtime_tick() {
    if (!g_state.initialized || g_state.client == nullptr) {
        return;
    }

    g_state.client->tick();

    if (!g_state.online) {
        return;
    }

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
    return current_page_id();
}

} // namespace demo
