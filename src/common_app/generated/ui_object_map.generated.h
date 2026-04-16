// AUTO-GENERATED FILE.
// Source: src/ui/screens.h + src/ui/screens.c
// DO NOT EDIT MANUALLY. Run: python scripts/generate_ui_meta.py

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <lvgl.h>

#include "common_app/generated/element_descriptors.generated.h"
#include "common_app/generated/page_descriptors.generated.h"
#include "lvgl_eez/UiObjectMap.h"

namespace demo {

struct Screen32BoundElement {
    uint32_t elementId = 0;
    uint32_t pageId = 0;
    lv_obj_t* obj = nullptr;
    const Screen32ElementDescriptor* descriptor = nullptr;
};

bool screen32_bind_generated_ui_map(screenlib::adapter::UiObjectMap& objectMap,
                                  Screen32BoundElement* outElements,
                                  size_t outCapacity,
                                  size_t* outCount);

const Screen32BoundElement* screen32_find_bound_element(const Screen32BoundElement* elements,
                                                     size_t count,
                                                     uint32_t elementId);

uint32_t screen32_current_page_id();
bool screen32_load_page_by_target(void* pageTarget);

}  // namespace demo
