#include "common_app/frontend_ui_events.h"

#include <lvgl.h>

#include "element_descriptors.generated.h"

namespace demo {

namespace {

const Screen32BoundElement* g_trackedElements = nullptr;
size_t g_trackedCount = 0;
FrontendUiEventSink g_sink = {};

struct ButtonEventState {
    uint32_t elementId = 0;
    bool isPressed = false;
    bool suppressClick = false;
};

ButtonEventState g_buttonStates[SCREEN32_ELEMENT_DESCRIPTOR_COUNT] = {};
size_t g_buttonStateCount = 0;

const Screen32BoundElement* find_tracked_element_by_id(uint32_t elementId) {
    return screen32_find_bound_element(g_trackedElements, g_trackedCount, elementId);
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
    if (g_sink.onButtonEvent == nullptr) {
        return;
    }
    g_sink.onButtonEvent(g_sink.userData, elementId, pageId, action);
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

void on_ui_event_cb(lv_event_t* e) {
    if (e == nullptr) {
        return;
    }

    const uint32_t elementId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    const lv_event_code_t code = lv_event_get_code(e);
    const uint32_t pageId = screen32_current_page_id();
    lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));

    const Screen32BoundElement* tracked = find_tracked_element_by_id(elementId);
    const Screen32ElementDescriptor* descriptor =
        (tracked != nullptr) ? tracked->descriptor : screen32_find_element_descriptor(elementId);
    if (descriptor == nullptr) {
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
        if (g_sink.onInputEventText != nullptr) {
            g_sink.onInputEventText(g_sink.userData, elementId, pageId, lv_textarea_get_text(target));
        }
        return;
    }
#endif

    int32_t numericValue = 0;
    if (read_element_int_value(target, numericValue) && g_sink.onInputEventInt != nullptr) {
        g_sink.onInputEventInt(g_sink.userData, elementId, pageId, numericValue);
    }
}

} // namespace

void frontend_ui_events_attach_generated(const Screen32BoundElement* trackedElements,
                                         size_t trackedCount,
                                         const FrontendUiEventSink& sink) {
    g_trackedElements = trackedElements;
    g_trackedCount = trackedCount;
    g_sink = sink;
    g_buttonStateCount = 0;

    for (size_t i = 0; i < trackedCount; ++i) {
        const Screen32BoundElement& tracked = trackedElements[i];
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
                on_ui_event_cb,
                LV_EVENT_CLICKED,
                reinterpret_cast<void*>(static_cast<uintptr_t>(tracked.elementId)));
            lv_obj_add_event_cb(
                tracked.obj,
                on_ui_event_cb,
                LV_EVENT_PRESSED,
                reinterpret_cast<void*>(static_cast<uintptr_t>(tracked.elementId)));
            lv_obj_add_event_cb(
                tracked.obj,
                on_ui_event_cb,
                LV_EVENT_RELEASED,
                reinterpret_cast<void*>(static_cast<uintptr_t>(tracked.elementId)));
            lv_obj_add_event_cb(
                tracked.obj,
                on_ui_event_cb,
                LV_EVENT_LONG_PRESSED,
                reinterpret_cast<void*>(static_cast<uintptr_t>(tracked.elementId)));
            lv_obj_add_event_cb(
                tracked.obj,
                on_ui_event_cb,
                LV_EVENT_LONG_PRESSED_REPEAT,
                reinterpret_cast<void*>(static_cast<uintptr_t>(tracked.elementId)));
        }

        if (tracked.descriptor->emits_input_event) {
            lv_obj_add_event_cb(
                tracked.obj,
                on_ui_event_cb,
                LV_EVENT_VALUE_CHANGED,
                reinterpret_cast<void*>(static_cast<uintptr_t>(tracked.elementId)));
        }
    }
}

} // namespace demo
