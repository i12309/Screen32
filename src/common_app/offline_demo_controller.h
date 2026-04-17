#pragma once

#include <stddef.h>
#include <stdint.h>

#include "lvgl_eez/EezLvglAdapter.h"

namespace demo {

/*
 * Роль файла:
 * - Содержит все локальное поведение offline demo: порядок страниц, привязки и правила next/prev/goto.
 * Вызывается:
 * - из frontend_runtime в режиме offline_demo и из offline-веток обработки UI-событий.
 * Должен содержать:
 * - только переходы навигации/состояний offline demo.
 * НЕ должен содержать:
 * - логику transport/proto/service-ответов.
 */

class OfflineDemoController {
public:
    OfflineDemoController() = default;

    // Инициализирует контроллер и адаптер для локального переключения страниц.
    void init(screenlib::adapter::EezLvglAdapter* adapter);
    // Сбрасывает текущую страницу, порядок страниц и привязки.
    void reset();

    // Задает упорядоченный список страниц для переходов next/prev.
    bool setPageOrder(const uint32_t* pageIds, size_t count);
    // Привязывает id кнопки к переходу на следующую страницу по порядку.
    bool bindButtonToNext(uint32_t elementId);
    // Привязывает id кнопки к переходу на предыдущую страницу по порядку.
    bool bindButtonToPrev(uint32_t elementId);
    // Привязывает id кнопки к явной целевой странице.
    bool bindButtonToGoto(uint32_t elementId, uint32_t targetPageId);
    // Привязывает клик по любому объекту страницы к переходу next/prev/goto.
    bool bindPageTapToNext(uint32_t sourcePageId);
    bool bindPageTapToPrev(uint32_t sourcePageId);
    bool bindPageTapToGoto(uint32_t sourcePageId, uint32_t targetPageId);
    // Применяет встроенный порядок demo-страниц и привязки по умолчанию.
    bool configureDefaultDemo();

    // Запускает demo-поток с requested page id или с первой страницы в порядке как fallback.
    bool start(uint32_t startPageId);
    // Обрабатывает событие кнопки в offline-режиме.
    bool onButtonEvent(uint32_t elementId, uint32_t sourcePageId);
    // Обрабатывает клик по некнопочному объекту страницы в offline-режиме.
    bool onObjectClick(uint32_t sourcePageId);
    // Обрабатывает numeric input-событие в offline-режиме.
    bool onInputEventInt(uint32_t elementId, uint32_t sourcePageId, int32_t value);
    // Обрабатывает text input-событие в offline-режиме.
    bool onInputEventText(uint32_t elementId, uint32_t sourcePageId, const char* value);

    // Возвращает текущий локальный page id, которым управляет контроллер.
    uint32_t currentPage() const;

private:
    enum class BindingActionType : uint8_t {
        Next = 1,
        Prev = 2,
        Goto = 3
    };

    struct Binding {
        uint32_t elementId = 0;
        BindingActionType action = BindingActionType::Goto;
        uint32_t targetPageId = 0;
    };

    struct PageTapBinding {
        uint32_t sourcePageId = 0;
        BindingActionType action = BindingActionType::Goto;
        uint32_t targetPageId = 0;
    };

    static constexpr size_t kMaxPages = 16;
    static constexpr size_t kMaxBindings = 64;
    static constexpr size_t kMaxPageTapBindings = 16;

    screenlib::adapter::EezLvglAdapter* _adapter = nullptr;
    uint32_t _pageOrder[kMaxPages] = {};
    size_t _pageOrderCount = 0;
    Binding _bindings[kMaxBindings] = {};
    size_t _bindingCount = 0;
    PageTapBinding _pageTapBindings[kMaxPageTapBindings] = {};
    size_t _pageTapBindingCount = 0;
    uint32_t _currentPageId = 0;

    bool setBinding(uint32_t elementId, BindingActionType action, uint32_t targetPageId);
    const Binding* findBinding(uint32_t elementId) const;
    bool setPageTapBinding(uint32_t sourcePageId, BindingActionType action, uint32_t targetPageId);
    const PageTapBinding* findPageTapBinding(uint32_t sourcePageId) const;
    bool applyAction(BindingActionType action, uint32_t targetPageId, uint32_t sourcePageId);
    bool showPage(uint32_t pageId);
    int findPageIndex(uint32_t pageId) const;
    bool pickStartPage(uint32_t requestedPageId, uint32_t& outPageId) const;
};

} // namespace demo


