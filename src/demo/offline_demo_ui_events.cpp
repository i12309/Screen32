#include "demo/offline_demo_ui_events.h"

#include <lvgl.h>

#include "page_descriptors.generated.h"

extern "C" {
#include "ui/screens.h"
}

namespace demo {

namespace {

constexpr uint32_t kSyntheticPageTapElementId = 0;

const Screen32BoundElement* g_trackedElements = nullptr;
size_t g_trackedCount = 0;
OfflineDemoObjectClickHandler g_objectClickHandler = nullptr;
void* g_userData = nullptr;
bool g_enabled = false;
bool g_trackedObjectClickHandlersAttached = false;
bool g_fallbackHandlersAttached = false;

const Screen32BoundElement* find_tracked_element_by_id(uint32_t elementId) {
    return screen32_find_bound_element(g_trackedElements, g_trackedCount, elementId);
}

const Screen32BoundElement* find_tracked_element_by_obj(const lv_obj_t* obj) {
    if (obj == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < g_trackedCount; ++i) {
        if (g_trackedElements[i].obj == obj) {
            return &g_trackedElements[i];
        }
    }
    return nullptr;
}

lv_obj_t* find_page_root_by_id(uint32_t pageId) {
    switch (pageId) {
        case scr_LOAD:
            return objects.load;
        case scr_MAIN:
            return objects.main;
        case scr_KEYBOARD:
            return objects.keyboard;
        case scr_TASK_RUN:
            return objects.task_run;
        case scr_TASK_PROCESS:
            return objects.task_process;
        case scr_INFO:
            return objects.info;
        case scr_INPUT:
            return objects.input;
        case scr_INIT:
            return objects.init;
        case scr_WAIT:
            return objects.wait;
        case scr_SERVICE:
            return objects.service;
        case scr_SERVICE2:
            return objects.service2;
        case scr_DEF_PAGE:
            return objects.def_page;
        case scr_DEF_PAGE2:
            return objects.def_page2;
        case scr_DEF_PAGE3:
            return objects.def_page3;
        case scr_DEF_PAGE4:
            return objects.def_page4;
        default:
            return nullptr;
    }
}

void on_demo_object_click_cb(lv_event_t* e) {
    if (e == nullptr) {
        return;
    }

    const uint32_t elementId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    const lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED && code != LV_EVENT_READY) {
        return;
    }

    if (!g_enabled || g_objectClickHandler == nullptr) {
        return;
    }

    const uint32_t pageId = screen32_current_page_id();
    lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));

    if (elementId == kSyntheticPageTapElementId) {
        // Do not duplicate clicks for tracked objects with dedicated handlers.
        if (find_tracked_element_by_obj(target) != nullptr) {
            return;
        }
        g_objectClickHandler(g_userData, elementId, pageId);
        return;
    }

    const Screen32BoundElement* tracked = find_tracked_element_by_id(elementId);
    if (tracked == nullptr || tracked->descriptor == nullptr) {
        return;
    }
    if (tracked->descriptor->emits_button_event) {
        return;
    }

    g_objectClickHandler(g_userData, elementId, pageId);
}

void attach_fallback_click_handlers_recursive(lv_obj_t* obj) {
    if (obj == nullptr || !lv_obj_is_valid(obj)) {
        return;
    }

    if (find_tracked_element_by_obj(obj) == nullptr) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(
            obj,
            on_demo_object_click_cb,
            LV_EVENT_CLICKED,
            reinterpret_cast<void*>(static_cast<uintptr_t>(kSyntheticPageTapElementId)));
        lv_obj_add_event_cb(
            obj,
            on_demo_object_click_cb,
            LV_EVENT_READY,
            reinterpret_cast<void*>(static_cast<uintptr_t>(kSyntheticPageTapElementId)));
    }

    const uint32_t childCount = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < childCount; ++i) {
        attach_fallback_click_handlers_recursive(lv_obj_get_child(obj, static_cast<int32_t>(i)));
    }
}

void attach_fallback_handlers_for_all_pages() {
    if (g_fallbackHandlersAttached) {
        return;
    }

    for (size_t i = 0; i < screen32_page_descriptor_count(); ++i) {
        const uint32_t pageId = g_screen32_page_descriptors[i].page_id;
        attach_fallback_click_handlers_recursive(find_page_root_by_id(pageId));
    }
    g_fallbackHandlersAttached = true;
}

void attach_tracked_object_click_handlers() {
    if (g_trackedObjectClickHandlersAttached) {
        return;
    }

    for (size_t i = 0; i < g_trackedCount; ++i) {
        const Screen32BoundElement& tracked = g_trackedElements[i];
        if (tracked.obj == nullptr || tracked.descriptor == nullptr) {
            continue;
        }
        if (tracked.descriptor->emits_button_event) {
            continue;
        }

        lv_obj_add_flag(tracked.obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(
            tracked.obj,
            on_demo_object_click_cb,
            LV_EVENT_CLICKED,
            reinterpret_cast<void*>(static_cast<uintptr_t>(tracked.elementId)));
    }

    g_trackedObjectClickHandlersAttached = true;
}

} // namespace

void offline_demo_ui_events_init(const Screen32BoundElement* trackedElements,
                                 size_t trackedCount,
                                 OfflineDemoObjectClickHandler handler,
                                 void* userData) {
    g_trackedElements = trackedElements;
    g_trackedCount = trackedCount;
    g_objectClickHandler = handler;
    g_userData = userData;
    g_enabled = false;
    g_trackedObjectClickHandlersAttached = false;
    g_fallbackHandlersAttached = false;
}

void offline_demo_ui_events_set_enabled(bool enabled) {
    g_enabled = enabled;
    if (!enabled || g_objectClickHandler == nullptr) {
        return;
    }

    attach_tracked_object_click_handlers();
    attach_fallback_handlers_for_all_pages();
}

} // namespace demo
