#pragma once

#include <stddef.h>
#include <stdint.h>

#include "common_app/generated/ui_object_map.generated.h"

namespace demo {

/*
 * Роль файла:
 * - Мост между LVGL-обработчиками сгенерированных UI-объектов и событиями кнопок/ввода frontend.
 * Вызывается:
 * - из frontend_runtime после привязки сгенерированной карты объектов.
 * Должен содержать:
 * - регистрацию LVGL-обработчиков и декодирование событий.
 * НЕ должен содержать:
 * - логику service-ответов, wiring transport runtime, правила offline demo по страницам.
 */

struct FrontendUiEventSink {
    void* userData = nullptr;
    void (*onButtonEvent)(void* userData, uint32_t elementId, uint32_t pageId) = nullptr;
    void (*onInputEventInt)(void* userData, uint32_t elementId, uint32_t pageId, int32_t value) = nullptr;
    void (*onInputEventText)(void* userData, uint32_t elementId, uint32_t pageId, const char* value) = nullptr;
};

// Подключает LVGL-обработчики для всех tracked-элементов и прокидывает события в sink.
void frontend_ui_events_attach_generated(const Screen32BoundElement* trackedElements,
                                         size_t trackedCount,
                                         const FrontendUiEventSink& sink);

} // namespace demo
