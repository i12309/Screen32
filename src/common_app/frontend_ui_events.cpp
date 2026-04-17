#include "common_app/frontend_ui_events.h"

#include <lvgl.h>

#include "common_app/generated/element_descriptors.generated.h"

namespace demo {

namespace {

const Screen32BoundElement* g_trackedElements = nullptr;
size_t g_trackedCount = 0;
FrontendUiEventSink g_sink = {};

const Screen32BoundElement* find_tracked_element_by_id(uint32_t elementId) {
    return screen32_find_bound_element(g_trackedElements, g_trackedCount, elementId);
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
    const Screen32BoundElement* tracked = find_tracked_element_by_id(elementId);
    const Screen32ElementDescriptor* descriptor =
        (tracked != nullptr) ? tracked->descriptor : screen32_find_element_descriptor(elementId);
    if (descriptor == nullptr) {
        return;
    }

    const uint32_t pageId = screen32_current_page_id();
    const lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));

    if (code == LV_EVENT_CLICKED) {
        if (descriptor->emits_button_event) {
            if (g_sink.onButtonEvent != nullptr) {
                g_sink.onButtonEvent(g_sink.userData, elementId, pageId);
            }
        } else if (g_sink.onObjectClick != nullptr) {
            g_sink.onObjectClick(g_sink.userData, elementId, pageId);
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

    for (size_t i = 0; i < trackedCount; ++i) {
        const Screen32BoundElement& tracked = trackedElements[i];
        if (tracked.obj == nullptr || tracked.descriptor == nullptr) {
            continue;
        }

        const bool needsObjectClicks = g_sink.onObjectClick != nullptr;
        if (tracked.descriptor->emits_button_event || needsObjectClicks) {
            if (!tracked.descriptor->emits_button_event && needsObjectClicks) {
                lv_obj_add_flag(tracked.obj, LV_OBJ_FLAG_CLICKABLE);
            }
            lv_obj_add_event_cb(
                tracked.obj,
                on_ui_event_cb,
                LV_EVENT_CLICKED,
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
