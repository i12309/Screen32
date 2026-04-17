#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ui_object_map.generated.h"
#include "runtime/ScreenClient.h"

namespace demo {

/*
 * Роль файла:
 * - Собирает protobuf-снимки page/element state из generated descriptors и live LVGL-объектов.
 * Вызывается:
 * - из frontend_service_responder при обработке request_page_state / request_element_state.
 * Должен содержать:
 * - helper-функции read-only чтения UI и сборку state для запросов.
 * НЕ должен содержать:
 * - оркестрацию transport/client, регистрацию LVGL-обработчиков, offline demo navigation rules.
 */

// Заполняет один PageElementState по tracked UI-элементу.
// Возвращает false, если объект невалидный или не представим поддерживаемыми типами state.
bool frontend_fill_page_element_state(const Screen32BoundElement& tracked, PageElementState& outState);

// Собирает полный payload ответа PageState для requestedPageId (0 = текущая страница).
void frontend_build_page_state(const Screen32BoundElement* trackedElements,
                               size_t trackedCount,
                               uint32_t requestedPageId,
                               uint32_t requestId,
                               PageState& outState);

// Собирает payload ответа ElementState для запроса одного элемента.
void frontend_build_element_state(const Screen32BoundElement* trackedElements,
                                  size_t trackedCount,
                                  uint32_t requestedPageId,
                                  uint32_t elementId,
                                  uint32_t requestId,
                                  ElementState& outState);

} // namespace demo


