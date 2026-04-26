#include "common_app/FrontApp.h"

#include <memory>
#include <stdio.h>
#include <string.h>
#include <vector>

#include <lvgl.h>

#include "common_app/app_core.h"
#include "common_app/frontend_platform.h"
#include "common_app/KeyboardController.h"
#include "chunk/TextChunkAssembler.h"
#include "chunk/TextChunkSender.h"
#include "demo/offline_demo_controller.h"
#include "demo/offline_demo_ui_events.h"
#include "element_descriptors.generated.h"
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
constexpr const char* kTrafficLogTag = "frontapp.traffic";

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
    Envelope txEnvelope = {};
    screenlib::chunk::TextChunkAssembler textAssembler;
    uint32_t nextTransferId = 1;

    State()
        : objectMap(objectBindings, kMaxObjectBindings, pageBindings, kMaxPageBindings),
          adapter(&objectMap) {}
};

State g_state;

void reset_envelope(Envelope& env) {
    memset(&env, 0, sizeof(env));
}

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
        case Envelope_button_event_tag: return "button_event";
        case Envelope_input_event_tag: return "input_event";
        case Envelope_heartbeat_tag: return "heartbeat";
        case Envelope_hello_tag: return "hello";
        case Envelope_request_device_info_tag: return "request_device_info";
        case Envelope_device_info_tag: return "device_info";
        case Envelope_request_current_page_tag: return "request_current_page";
        case Envelope_current_page_tag: return "current_page";
        case Envelope_set_element_attribute_tag: return "set_element_attribute";
        case Envelope_request_element_attribute_tag: return "request_element_attribute";
        case Envelope_element_attribute_state_tag: return "element_attribute_state";
        case Envelope_page_snapshot_tag: return "page_snapshot";
        case Envelope_attribute_changed_tag: return "attribute_changed";
        case Envelope_text_chunk_tag: return "text_chunk";
        case Envelope_text_chunk_abort_tag: return "text_chunk_abort";
        default: return "unknown";
    }
}

const char* button_action_name(ButtonAction action) {
    switch (action) {
        case ButtonAction_PUSH: return "push";
        case ButtonAction_POP: return "pop";
        case ButtonAction_REPEAT: return "repeat";
        case ButtonAction_CLICK:
        default:
            return "click";
    }
}

void describe_attribute_value(const ElementAttributeValue& value, char* out, size_t outSize) {
    if (out == nullptr || outSize == 0) {
        return;
    }

    switch (value.which_value) {
        case ElementAttributeValue_int_value_tag:
            snprintf(out,
                     outSize,
                     "attr=%d int=%ld",
                     static_cast<int>(value.attribute),
                     static_cast<long>(value.value.int_value));
            break;
        case ElementAttributeValue_color_value_tag:
            snprintf(out,
                     outSize,
                     "attr=%d color=#%06lX",
                     static_cast<int>(value.attribute),
                     static_cast<unsigned long>(value.value.color_value & 0xFFFFFFU));
            break;
        case ElementAttributeValue_font_value_tag:
            snprintf(out,
                     outSize,
                     "attr=%d font=%d",
                     static_cast<int>(value.attribute),
                     static_cast<int>(value.value.font_value));
            break;
        case ElementAttributeValue_bool_value_tag:
            snprintf(out,
                     outSize,
                     "attr=%d bool=%d",
                     static_cast<int>(value.attribute),
                     value.value.bool_value ? 1 : 0);
            break;
        case ElementAttributeValue_string_value_tag:
            snprintf(out,
                     outSize,
                     "attr=%d text=\"%s\"",
                     static_cast<int>(value.attribute),
                     value.value.string_value);
            break;
        default:
            snprintf(out, outSize, "attr=%d value=<none>", static_cast<int>(value.attribute));
            break;
    }
}

void describe_set_attribute_value(const SetElementAttribute& cmd, char* out, size_t outSize) {
    if (out == nullptr || outSize == 0) {
        return;
    }

    switch (cmd.which_value) {
        case SetElementAttribute_int_value_tag:
            snprintf(out, outSize, "int=%ld", static_cast<long>(cmd.value.int_value));
            break;
        case SetElementAttribute_color_value_tag:
            snprintf(out,
                     outSize,
                     "color=#%06lX",
                     static_cast<unsigned long>(cmd.value.color_value & 0xFFFFFFU));
            break;
        case SetElementAttribute_font_value_tag:
            snprintf(out, outSize, "font=%d", static_cast<int>(cmd.value.font_value));
            break;
        case SetElementAttribute_bool_value_tag:
            snprintf(out, outSize, "bool=%d", cmd.value.bool_value ? 1 : 0);
            break;
        default:
            snprintf(out, outSize, "value=<none>");
            break;
    }
}

void describe_envelope(const Envelope& env, char* out, size_t outSize) {
    if (out == nullptr || outSize == 0) {
        return;
    }

    switch (env.which_payload) {
        case Envelope_show_page_tag:
            snprintf(out,
                     outSize,
                     "page=%lu session=%lu",
                     static_cast<unsigned long>(env.payload.show_page.page_id),
                     static_cast<unsigned long>(env.payload.show_page.session_id));
            break;
        case Envelope_button_event_tag:
            snprintf(out,
                     outSize,
                     "element=%lu page=%lu session=%lu action=%s",
                     static_cast<unsigned long>(env.payload.button_event.element_id),
                     static_cast<unsigned long>(env.payload.button_event.page_id),
                     static_cast<unsigned long>(env.payload.button_event.session_id),
                     button_action_name(env.payload.button_event.action));
            break;
        case Envelope_input_event_tag:
            if (env.payload.input_event.which_value == InputEvent_int_value_tag) {
                snprintf(out,
                         outSize,
                         "element=%lu page=%lu session=%lu int=%ld",
                         static_cast<unsigned long>(env.payload.input_event.element_id),
                         static_cast<unsigned long>(env.payload.input_event.page_id),
                         static_cast<unsigned long>(env.payload.input_event.session_id),
                         static_cast<long>(env.payload.input_event.value.int_value));
            } else {
                snprintf(out,
                         outSize,
                         "element=%lu page=%lu session=%lu value=<none>",
                         static_cast<unsigned long>(env.payload.input_event.element_id),
                         static_cast<unsigned long>(env.payload.input_event.page_id),
                         static_cast<unsigned long>(env.payload.input_event.session_id));
            }
            break;
        case Envelope_heartbeat_tag:
            snprintf(out,
                     outSize,
                     "uptime_ms=%lu",
                     static_cast<unsigned long>(env.payload.heartbeat.uptime_ms));
            break;
        case Envelope_hello_tag:
            snprintf(out,
                     outSize,
                     "device=%s type=%s",
                     env.payload.hello.device_info.device_id,
                     env.payload.hello.device_info.client_type);
            break;
        case Envelope_request_device_info_tag:
            snprintf(out,
                     outSize,
                     "request=%lu",
                     static_cast<unsigned long>(env.payload.request_device_info.request_id));
            break;
        case Envelope_device_info_tag:
            snprintf(out,
                     outSize,
                     "device=%s type=%s instance=%s",
                     env.payload.device_info.device_id,
                     env.payload.device_info.client_type,
                     env.payload.device_info.instance_id);
            break;
        case Envelope_request_current_page_tag:
            snprintf(out,
                     outSize,
                     "request=%lu",
                     static_cast<unsigned long>(env.payload.request_current_page.request_id));
            break;
        case Envelope_current_page_tag:
            snprintf(out,
                     outSize,
                     "request=%lu page=%lu",
                     static_cast<unsigned long>(env.payload.current_page.request_id),
                     static_cast<unsigned long>(env.payload.current_page.page_id));
            break;
        case Envelope_set_element_attribute_tag: {
            char valueText[80] = {};
            describe_set_attribute_value(env.payload.set_element_attribute, valueText, sizeof(valueText));
            snprintf(out,
                     outSize,
                     "element=%lu attr=%d request=%lu session=%lu %s",
                     static_cast<unsigned long>(env.payload.set_element_attribute.element_id),
                     static_cast<int>(env.payload.set_element_attribute.attribute),
                     static_cast<unsigned long>(env.payload.set_element_attribute.request_id),
                     static_cast<unsigned long>(env.payload.set_element_attribute.session_id),
                     valueText);
            break;
        }
        case Envelope_request_element_attribute_tag:
            snprintf(out,
                     outSize,
                     "request=%lu page=%lu element=%lu attr=%d",
                     static_cast<unsigned long>(env.payload.request_element_attribute.request_id),
                     static_cast<unsigned long>(env.payload.request_element_attribute.page_id),
                     static_cast<unsigned long>(env.payload.request_element_attribute.element_id),
                     static_cast<int>(env.payload.request_element_attribute.attribute));
            break;
        case Envelope_element_attribute_state_tag:
            snprintf(out,
                     outSize,
                     "request=%lu page=%lu element=%lu attr=%d found=%d",
                     static_cast<unsigned long>(env.payload.element_attribute_state.request_id),
                     static_cast<unsigned long>(env.payload.element_attribute_state.page_id),
                     static_cast<unsigned long>(env.payload.element_attribute_state.element_id),
                     static_cast<int>(env.payload.element_attribute_state.attribute),
                     env.payload.element_attribute_state.found ? 1 : 0);
            break;
        case Envelope_page_snapshot_tag:
            snprintf(out,
                     outSize,
                     "page=%lu session=%lu elements=%u",
                     static_cast<unsigned long>(env.payload.page_snapshot.page_id),
                     static_cast<unsigned long>(env.payload.page_snapshot.session_id),
                     static_cast<unsigned>(env.payload.page_snapshot.elements_count));
            break;
        case Envelope_attribute_changed_tag: {
            char valueText[80] = {};
            if (env.payload.attribute_changed.has_value) {
                describe_attribute_value(env.payload.attribute_changed.value, valueText, sizeof(valueText));
            } else {
                snprintf(valueText, sizeof(valueText), "value=<none>");
            }
            snprintf(out,
                     outSize,
                     "element=%lu page=%lu session=%lu reason=%d reply=%lu %s",
                     static_cast<unsigned long>(env.payload.attribute_changed.element_id),
                     static_cast<unsigned long>(env.payload.attribute_changed.page_id),
                     static_cast<unsigned long>(env.payload.attribute_changed.session_id),
                     static_cast<int>(env.payload.attribute_changed.reason),
                     static_cast<unsigned long>(env.payload.attribute_changed.in_reply_to_request),
                     valueText);
            break;
        }
        case Envelope_text_chunk_tag:
            snprintf(out,
                     outSize,
                     "transfer=%lu element=%lu page=%lu session=%lu index=%lu/%lu kind=%d bytes=%u request=%lu",
                     static_cast<unsigned long>(env.payload.text_chunk.transfer_id),
                     static_cast<unsigned long>(env.payload.text_chunk.element_id),
                     static_cast<unsigned long>(env.payload.text_chunk.page_id),
                     static_cast<unsigned long>(env.payload.text_chunk.session_id),
                     static_cast<unsigned long>(env.payload.text_chunk.chunk_index),
                     static_cast<unsigned long>(env.payload.text_chunk.chunk_count),
                     static_cast<int>(env.payload.text_chunk.kind),
                     static_cast<unsigned>(env.payload.text_chunk.chunk_data.size),
                     static_cast<unsigned long>(env.payload.text_chunk.request_id));
            break;
        case Envelope_text_chunk_abort_tag:
            snprintf(out,
                     outSize,
                     "transfer=%lu request=%lu reason=%d",
                     static_cast<unsigned long>(env.payload.text_chunk_abort.transfer_id),
                     static_cast<unsigned long>(env.payload.text_chunk_abort.request_id),
                     static_cast<int>(env.payload.text_chunk_abort.reason));
            break;
        default:
            snprintf(out, outSize, "tag=%u", static_cast<unsigned>(env.which_payload));
            break;
    }
}

void log_traffic_envelope(const Envelope& env, screenlib::client::ScreenClient::EventDirection direction) {
    if (!g_state.config.logTraffic) {
        return;
    }

    char details[192] = {};
    describe_envelope(env, details, sizeof(details));
    SCREENLIB_LOGI(kTrafficLogTag,
                   "%s %s %s",
                   direction == screenlib::client::ScreenClient::EventDirection::Incoming ? "RX" : "TX",
                   envelope_payload_name(env.which_payload),
                   details);
}

enum class FrontendButtonAction : uint8_t {
    Click = 0,
    Push = 1,
    Pop = 2,
    Repeat = 3
};

struct FrontendUiEventSink {
    void* userData = nullptr;
    void (*onButtonEvent)(void* userData,
                          uint32_t elementId,
                          uint32_t pageId,
                          FrontendButtonAction action) = nullptr;
    void (*onInputEventInt)(void* userData, uint32_t elementId, uint32_t pageId, int32_t value) = nullptr;
    void (*onInputEventText)(void* userData, uint32_t elementId, uint32_t pageId, const char* value) = nullptr;
};

ButtonAction to_proto_button_action(FrontendButtonAction action) {
    switch (action) {
        case FrontendButtonAction::Push:
            return ButtonAction_PUSH;
        case FrontendButtonAction::Pop:
            return ButtonAction_POP;
        case FrontendButtonAction::Repeat:
            return ButtonAction_REPEAT;
        case FrontendButtonAction::Click:
        default:
            return ButtonAction_CLICK;
    }
}

FrontendUiEventSink g_uiEventSink = {};
const demo::Screen32BoundElement* g_trackedElements = nullptr;
size_t g_trackedCount = 0;

struct ButtonEventState {
    uint32_t elementId = 0;
    bool isPressed = false;
    bool suppressClick = false;
};

ButtonEventState g_buttonStates[SCREEN32_ELEMENT_DESCRIPTOR_COUNT] = {};
size_t g_buttonStateCount = 0;
KeyboardController g_keyboardController;

const demo::Screen32BoundElement* find_tracked_element_by_id(uint32_t elementId) {
    return demo::screen32_find_bound_element(g_trackedElements, g_trackedCount, elementId);
}

ButtonEventState* find_button_state(uint32_t elementId) {
    for (size_t i = 0; i < g_buttonStateCount; ++i) {
        if (g_buttonStates[i].elementId == elementId) {
            return &g_buttonStates[i];
        }
    }
    return nullptr;
}

void emit_button_action(uint32_t elementId, uint32_t pageId, FrontendButtonAction action) {
    if (g_uiEventSink.onButtonEvent == nullptr) {
        return;
    }
    g_uiEventSink.onButtonEvent(g_uiEventSink.userData, elementId, pageId, action);
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

void on_generated_ui_event_cb(lv_event_t* e) {
    if (e == nullptr) {
        return;
    }

    const uint32_t elementId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    const lv_event_code_t code = lv_event_get_code(e);
    const uint32_t pageId = demo::screen32_current_page_id();
    lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));

    const demo::Screen32BoundElement* tracked = find_tracked_element_by_id(elementId);
    const Screen32ElementDescriptor* descriptor =
        (tracked != nullptr) ? tracked->descriptor : screen32_find_element_descriptor(elementId);
    if (descriptor == nullptr) {
        return;
    }

    if (g_keyboardController.shouldSuppressGeneratedEvent(descriptor->page_id)) {
        return;
    }

    if (descriptor->opens_keyboard) {
        if (code == LV_EVENT_CLICKED) {
            lv_obj_t* source = tracked != nullptr ? tracked->obj : target;
            g_keyboardController.open(
                descriptor->page_id,
                elementId,
                source,
                descriptor->keyboard_kind,
                descriptor->keyboard_max_length);
        }
        return;
    }

    if (descriptor->emits_button_event) {
        ButtonEventState* state = find_button_state(elementId);

        if (code == LV_EVENT_PRESSED) {
            if (state != nullptr) {
                if (state->isPressed) {
                    return;
                }
                state->isPressed = true;
                state->suppressClick = false;
            }
            emit_button_action(elementId, pageId, FrontendButtonAction::Push);
            return;
        }

        if (code == LV_EVENT_LONG_PRESSED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
            if (state != nullptr && !state->isPressed) {
                return;
            }
            if (state != nullptr) {
                state->suppressClick = true;
            }
            emit_button_action(elementId, pageId, FrontendButtonAction::Repeat);
            return;
        }

        if (code == LV_EVENT_RELEASED) {
            if (state != nullptr) {
                if (!state->isPressed) {
                    return;
                }
                state->isPressed = false;
            }
            emit_button_action(elementId, pageId, FrontendButtonAction::Pop);
            return;
        }

        if (code == LV_EVENT_CLICKED) {
            if (state != nullptr && state->suppressClick) {
                state->suppressClick = false;
                return;
            }
            emit_button_action(elementId, pageId, FrontendButtonAction::Click);
        }
        return;
    }

    if (code != LV_EVENT_VALUE_CHANGED || target == nullptr || !descriptor->emits_input_event) {
        return;
    }

#if LV_USE_TEXTAREA
    if (lv_obj_check_type(target, &lv_textarea_class)) {
        if (g_uiEventSink.onInputEventText != nullptr) {
            g_uiEventSink.onInputEventText(g_uiEventSink.userData, elementId, pageId, lv_textarea_get_text(target));
        }
        return;
    }
#endif

    int32_t numericValue = 0;
    if (read_element_int_value(target, numericValue) && g_uiEventSink.onInputEventInt != nullptr) {
        g_uiEventSink.onInputEventInt(g_uiEventSink.userData, elementId, pageId, numericValue);
    }
}

void attach_generated_ui_events(const demo::Screen32BoundElement* trackedElements,
                                size_t trackedCount,
                                const FrontendUiEventSink& sink) {
    g_trackedElements = trackedElements;
    g_trackedCount = trackedCount;
    g_uiEventSink = sink;
    g_buttonStateCount = 0;

    for (size_t i = 0; i < trackedCount; ++i) {
        const demo::Screen32BoundElement& tracked = trackedElements[i];
        if (tracked.obj == nullptr || tracked.descriptor == nullptr) {
            continue;
        }

        if (tracked.descriptor->emits_button_event) {
            if (g_buttonStateCount < SCREEN32_ELEMENT_DESCRIPTOR_COUNT) {
                ButtonEventState& state = g_buttonStates[g_buttonStateCount++];
                state.elementId = tracked.elementId;
                state.isPressed = false;
                state.suppressClick = false;
            }

            lv_obj_add_event_cb(
                tracked.obj,
                on_generated_ui_event_cb,
                LV_EVENT_CLICKED,
                reinterpret_cast<void*>(static_cast<uintptr_t>(tracked.elementId)));
            lv_obj_add_event_cb(
                tracked.obj,
                on_generated_ui_event_cb,
                LV_EVENT_PRESSED,
                reinterpret_cast<void*>(static_cast<uintptr_t>(tracked.elementId)));
            lv_obj_add_event_cb(
                tracked.obj,
                on_generated_ui_event_cb,
                LV_EVENT_RELEASED,
                reinterpret_cast<void*>(static_cast<uintptr_t>(tracked.elementId)));
            lv_obj_add_event_cb(
                tracked.obj,
                on_generated_ui_event_cb,
                LV_EVENT_LONG_PRESSED,
                reinterpret_cast<void*>(static_cast<uintptr_t>(tracked.elementId)));
            lv_obj_add_event_cb(
                tracked.obj,
                on_generated_ui_event_cb,
                LV_EVENT_LONG_PRESSED_REPEAT,
                reinterpret_cast<void*>(static_cast<uintptr_t>(tracked.elementId)));
        }

        if (tracked.descriptor->emits_input_event) {
            lv_obj_add_event_cb(
                tracked.obj,
                on_generated_ui_event_cb,
                LV_EVENT_VALUE_CHANGED,
                reinterpret_cast<void*>(static_cast<uintptr_t>(tracked.elementId)));
        }
    }
}

bool hook_show_page(void* userData, void* pageTarget) {
    (void)userData;
    return demo::screen32_load_page_by_target(pageTarget);
}

bool hook_set_text(void* userData, void* uiObject, const char* text) {
    (void)userData;
    return KeyboardController::setTextToObject(static_cast<lv_obj_t*>(uiObject), text);
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

bool send_hello(const DeviceInfo& deviceInfo) {
    if (g_state.client == nullptr) {
        return false;
    }

    Envelope& env = g_state.txEnvelope;
    reset_envelope(env);
    env.which_payload = Envelope_hello_tag;
    env.payload.hello.has_device_info = true;
    env.payload.hello.device_info = deviceInfo;
    return g_state.client->sendEnvelope(env);
}

bool send_device_info(const DeviceInfo& deviceInfo) {
    if (g_state.client == nullptr) {
        return false;
    }

    Envelope& env = g_state.txEnvelope;
    reset_envelope(env);
    env.which_payload = Envelope_device_info_tag;
    env.payload.device_info = deviceInfo;
    return g_state.client->sendEnvelope(env);
}

bool send_current_page(uint32_t pageId, uint32_t requestId) {
    if (g_state.client == nullptr) {
        return false;
    }

    Envelope& env = g_state.txEnvelope;
    reset_envelope(env);
    env.which_payload = Envelope_current_page_tag;
    env.payload.current_page.page_id = pageId;
    env.payload.current_page.request_id = requestId;
    return g_state.client->sendEnvelope(env);
}

uint32_t next_transfer_id() {
    const uint32_t transferId = g_state.nextTransferId++;
    if (g_state.nextTransferId == 0) {
        g_state.nextTransferId = 1;
    }
    return transferId;
}

bool send_client_envelope(const Envelope& env, void* userData) {
    (void)userData;
    return g_state.client != nullptr && g_state.client->sendEnvelope(env);
}

bool send_snapshot_for_current_page() {
    if (g_state.client == nullptr) {
        return false;
    }

    std::vector<screenlib::adapter::SnapshotLongTextField> longTextFields;
    Envelope& env = g_state.txEnvelope;
    reset_envelope(env);
    env.which_payload = Envelope_page_snapshot_tag;
    if (!g_state.adapter.buildPageSnapshot(
            g_state.currentPageId,
            g_state.currentSessionId,
            env.payload.page_snapshot,
            longTextFields)) {
        SCREENLIB_LOGW(kLogTag,
                       "buildPageSnapshot returned false for page=%lu session=%lu",
                       static_cast<unsigned long>(g_state.currentPageId),
                       static_cast<unsigned long>(g_state.currentSessionId));
    }

    if (!g_state.client->sendEnvelope(env)) {
        return false;
    }

    bool allOk = true;
    for (const screenlib::adapter::SnapshotLongTextField& field : longTextFields) {
        const uint32_t transferId = next_transfer_id();
        if (!screenlib::chunk::sendTextChunks(&send_client_envelope,
                                              nullptr,
                                              TextChunkKind_TEXT_CHUNK_ATTRIBUTE_CHANGED,
                                              transferId,
                                              g_state.currentSessionId,
                                              g_state.currentPageId,
                                              field.elementId,
                                              field.attribute,
                                              0,
                                              field.text.c_str())) {
            SCREENLIB_LOGW(kLogTag,
                           "snapshot long text chunks send failed element=%lu attr=%d",
                           static_cast<unsigned long>(field.elementId),
                           static_cast<int>(field.attribute));
            allOk = false;
        }
    }

    return allOk;
}

bool send_attribute_change(uint32_t elementId,
                           const ElementAttributeValue& value,
                           AttributeChangeReason reason,
                           uint32_t inReplyToRequest) {
    if (g_state.client == nullptr || g_state.offlineDemo || !g_state.client->connected()) {
        return false;
    }

    Envelope& env = g_state.txEnvelope;
    reset_envelope(env);
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

bool send_text_chunk_abort(const TextChunkAbort& abort) {
    if (g_state.client == nullptr || g_state.offlineDemo || !g_state.client->connected()) {
        return false;
    }

    Envelope& env = g_state.txEnvelope;
    reset_envelope(env);
    env.which_payload = Envelope_text_chunk_abort_tag;
    env.payload.text_chunk_abort = abort;
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

void handle_text_chunk(const TextChunk& chunkMsg) {
    screenlib::chunk::AssembledText text;
    TextChunkAbort abort = TextChunkAbort_init_zero;
    if (!g_state.textAssembler.push(chunkMsg, ::platform_tick_ms(), text, abort)) {
        if (abort.transfer_id != 0) {
            SCREENLIB_LOGW(kLogTag,
                           "text chunk rejected transfer=%lu reason=%d",
                           static_cast<unsigned long>(abort.transfer_id),
                           static_cast<int>(abort.reason));
            send_text_chunk_abort(abort);
        }
        return;
    }

    if (text.kind != TextChunkKind_TEXT_CHUNK_SET_ATTRIBUTE ||
        text.attribute != ElementAttribute_ELEMENT_ATTRIBUTE_TEXT) {
        SCREENLIB_LOGW(kLogTag,
                       "unsupported text chunk kind=%d attr=%d transfer=%lu",
                       static_cast<int>(text.kind),
                       static_cast<int>(text.attribute),
                       static_cast<unsigned long>(text.transferId));
        return;
    }

    if (text.sessionId != 0 && text.sessionId != g_state.currentSessionId) {
        SCREENLIB_LOGW(kLogTag,
                       "stale text chunk ignored: session=%lu/%lu element=%lu",
                       static_cast<unsigned long>(text.sessionId),
                       static_cast<unsigned long>(g_state.currentSessionId),
                       static_cast<unsigned long>(text.elementId));
        return;
    }

    ElementAttributeValue applied = ElementAttributeValue_init_zero;
    if (!g_state.adapter.applyTextAttribute(text.elementId, text.text.c_str(), applied)) {
        SCREENLIB_LOGW(kLogTag,
                       "text chunk apply failed element=%lu",
                       static_cast<unsigned long>(text.elementId));
        return;
    }

    if (text.requestId != 0) {
        send_attribute_change(text.elementId,
                              applied,
                              AttributeChangeReason_REASON_COMMAND_APPLIED,
                              text.requestId);
    }
}

bool on_adapter_event(const Envelope& source, void* userData) {
    (void)userData;
    if (g_state.client == nullptr || g_state.offlineDemo || !g_state.client->connected()) {
        return false;
    }

    Envelope& env = g_state.txEnvelope;
    env = source;
    switch (env.which_payload) {
        case Envelope_button_event_tag:
            env.payload.button_event.session_id = g_state.currentSessionId;
            break;
        case Envelope_input_event_tag:
            env.payload.input_event.session_id = g_state.currentSessionId;
            break;
        case Envelope_text_chunk_tag:
            env.payload.text_chunk.session_id = g_state.currentSessionId;
            break;
        default:
            break;
    }

    return g_state.client->sendEnvelope(env);
}

void on_ui_button_event(void* userData, uint32_t elementId, uint32_t pageId, FrontendButtonAction action) {
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
        case Envelope_text_chunk_tag:
            handle_text_chunk(env.payload.text_chunk);
            break;
        case Envelope_text_chunk_abort_tag:
            SCREENLIB_LOGW(kLogTag,
                           "peer aborted text chunk transfer=%lu request=%lu reason=%d",
                           static_cast<unsigned long>(env.payload.text_chunk_abort.transfer_id),
                           static_cast<unsigned long>(env.payload.text_chunk_abort.request_id),
                           static_cast<int>(env.payload.text_chunk_abort.reason));
            break;
        case Envelope_request_device_info_tag:
            send_device_info(build_device_info(g_state.config.mode));
            break;
        case Envelope_request_current_page_tag:
            send_current_page(
                g_keyboardController.logicalPageId(demo::screen32_current_page_id()),
                env.payload.request_current_page.request_id);
            break;
        default:
            break;
    }
}

void on_client_event(const Envelope& env, screenlib::client::ScreenClient::EventDirection direction, void* userData) {
    State* state = static_cast<State*>(userData);
    if (state == nullptr) {
        return;
    }

    log_traffic_envelope(env, direction);

    if (direction != screenlib::client::ScreenClient::EventDirection::Incoming) {
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
    g_state.textAssembler.reset();
    g_state.nextTransferId = 1;
    g_state.client->setEventHandler(&on_client_event, &g_state);
    g_state.client->init();

    const uint32_t startPage = resolve_online_start_page(config);
    g_state.currentPageId = startPage;
    g_state.adapter.showPage(startPage);
    SCREENLIB_LOGI(kLogTag, "online mode enabled; start page=%lu", static_cast<unsigned long>(startPage));

    const bool helloOk = send_hello(build_device_info(g_state.config.mode));
    g_state.lastHelloMs = ::platform_tick_ms();
    g_state.waitStartMs = g_state.lastHelloMs;
    g_state.lastWaitLogMs = g_state.waitStartMs;
    SCREENLIB_LOGI(kLogTag, "hello sent: %s", helloOk ? "ok" : "fail");
    return true;
}

bool start_offline_demo_mode(const demo::FrontendConfig& config) {
    g_state.client.reset();
    g_state.transport.reset();
    g_state.textAssembler.reset();
    g_state.nextTransferId = 1;
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
                   "init mode=%s transport=%s offline_demo=%d online_page=%lu offline_page=%lu hb_ms=%lu log_traffic=%d",
                   demo::frontend_mode_name(config.mode),
                   demo::frontend_transport_name(config.transport.type),
                   config.offlineDemo ? 1 : 0,
                   static_cast<unsigned long>(config.firstOnlinePage),
                   static_cast<unsigned long>(config.firstOfflinePage),
                   static_cast<unsigned long>(config.heartbeatPeriodMs),
                   config.logTraffic ? 1 : 0);

    configure_adapter();
    bind_generated_ui();

    FrontendUiEventSink sink{};
    sink.userData = &g_state;
    sink.onButtonEvent = &on_ui_button_event;
    sink.onInputEventInt = &on_ui_input_event_int;
    sink.onInputEventText = &on_ui_input_event_text;
    g_keyboardController.setInputEventSink(sink.userData, sink.onInputEventText);
    attach_generated_ui_events(g_state.tracked, g_state.trackedCount, sink);
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
        TextChunkAbort abort = TextChunkAbort_init_zero;
        while (g_state.textAssembler.pollTimeout(now, abort)) {
            SCREENLIB_LOGW(kLogTag,
                           "text chunk assembly timeout transfer=%lu request=%lu",
                           static_cast<unsigned long>(abort.transfer_id),
                           static_cast<unsigned long>(abort.request_id));
            send_text_chunk_abort(abort);
        }

        if (!g_state.backendConnected && (now - g_state.lastHelloMs) >= kHelloRetryPeriodMs) {
            const bool helloOk = send_hello(build_device_info(g_state.config.mode));
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
