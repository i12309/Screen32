#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ui_object_map.generated.h"

namespace demo {

using OfflineDemoObjectClickHandler = void (*)(void* userData, uint32_t elementId, uint32_t pageId);

// Инициализирует demo-only обработчики object-click событий.
void offline_demo_ui_events_init(const Screen32BoundElement* trackedElements,
                                 size_t trackedCount,
                                 OfflineDemoObjectClickHandler handler,
                                 void* userData);

// Включает или выключает demo-only обработку тапа по объекту или странице.
void offline_demo_ui_events_set_enabled(bool enabled);

} // namespace demo
