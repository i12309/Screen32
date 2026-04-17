#include "common_app/page_state_builder.h"

#include <string.h>

#include <lvgl.h>

#include "element_descriptors.generated.h"

namespace demo {

namespace {

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

uint32_t resolve_page_id(uint32_t requestedPageId) {
    return requestedPageId == 0 ? screen32_current_page_id() : requestedPageId;
}

const Screen32BoundElement* find_tracked_by_id(const Screen32BoundElement* trackedElements,
                                               size_t trackedCount,
                                               uint32_t elementId) {
    return screen32_find_bound_element(trackedElements, trackedCount, elementId);
}

} // namespace

bool frontend_fill_page_element_state(const Screen32BoundElement& tracked, PageElementState& outState) {
    if (tracked.obj == nullptr || !lv_obj_is_valid(tracked.obj)) {
        return false;
    }

    PageElementState zeroState = PageElementState_init_zero;
    outState = zeroState;
    outState.element_id = tracked.elementId;
    const Screen32ElementDescriptor* descriptor =
        tracked.descriptor != nullptr ? tracked.descriptor : screen32_find_element_descriptor(tracked.elementId);
    if (descriptor == nullptr) {
        return false;
    }

    char text[65] = {};
    if (descriptor->supports_text && read_element_text(tracked.obj, text, sizeof(text))) {
        outState.type = ElementStateType_ELEMENT_STATE_TEXT;
        outState.which_value = PageElementState_text_value_tag;
        copy_text_safe(outState.value.text_value, sizeof(outState.value.text_value), text);
        return true;
    }

    int32_t value = 0;
    if (descriptor->supports_value && read_element_int_value(tracked.obj, value)) {
        outState.type = ElementStateType_ELEMENT_STATE_INT;
        outState.which_value = PageElementState_int_value_tag;
        outState.value.int_value = value;
        return true;
    }

    if (!descriptor->supports_visible) {
        return false;
    }

    outState.type = ElementStateType_ELEMENT_STATE_VISIBLE;
    outState.which_value = PageElementState_visible_tag;
    outState.value.visible = !lv_obj_has_flag(tracked.obj, LV_OBJ_FLAG_HIDDEN);
    return true;
}

void frontend_build_page_state(const Screen32BoundElement* trackedElements,
                               size_t trackedCount,
                               uint32_t requestedPageId,
                               uint32_t requestId,
                               PageState& outState) {
    PageState zeroState = PageState_init_zero;
    outState = zeroState;
    outState.request_id = requestId;
    outState.page_id = resolve_page_id(requestedPageId);
    outState.elements_count = 0;

    for (size_t i = 0; i < trackedCount && outState.elements_count < 8; ++i) {
        if (trackedElements[i].pageId != outState.page_id) {
            continue;
        }
        if (frontend_fill_page_element_state(trackedElements[i], outState.elements[outState.elements_count])) {
            outState.elements_count++;
        }
    }
}

void frontend_build_element_state(const Screen32BoundElement* trackedElements,
                                  size_t trackedCount,
                                  uint32_t requestedPageId,
                                  uint32_t elementId,
                                  uint32_t requestId,
                                  ElementState& outState) {
    ElementState zeroState = ElementState_init_zero;
    outState = zeroState;
    outState.request_id = requestId;
    outState.page_id = resolve_page_id(requestedPageId);

    const Screen32BoundElement* tracked = find_tracked_by_id(trackedElements, trackedCount, elementId);
    if (tracked != nullptr && tracked->pageId == outState.page_id) {
        outState.found = frontend_fill_page_element_state(*tracked, outState.element);
        outState.has_element = outState.found;
        return;
    }

    outState.found = false;
    outState.has_element = false;
}

} // namespace demo


