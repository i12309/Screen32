#pragma once

#include <stddef.h>
#include <stdint.h>

namespace demo {

/*
 * Роль файла:
 * - Описывает сценарий offline demo в одном месте: какие страницы участвуют в demo,
 *   в каком порядке работает Next и какие элементы куда ведут.
 * Вызывается:
 * - из OfflineDemoController при конфигурации demo по умолчанию.
 * Должен содержать:
 * - только декларативное описание маршрутов и порядка страниц.
 * НЕ должен содержать:
 * - LVGL-вызовы, transport/runtime-логику и обработку UI-событий.
 */

enum class OfflineDemoNavigationAction : uint8_t {
    Next = 1,
    Prev = 2,
    Goto = 3,
    Back = 4,
};

struct OfflineDemoButtonRoute {
    constexpr OfflineDemoButtonRoute(
        uint32_t elementId_ = 0,
        OfflineDemoNavigationAction action_ = OfflineDemoNavigationAction::Goto,
        uint32_t targetPageId_ = 0)
        : elementId(elementId_), action(action_), targetPageId(targetPageId_) {}

    uint32_t elementId;
    OfflineDemoNavigationAction action;
    uint32_t targetPageId;
};

struct OfflineDemoPageTapRoute {
    constexpr OfflineDemoPageTapRoute(
        uint32_t sourcePageId_ = 0,
        OfflineDemoNavigationAction action_ = OfflineDemoNavigationAction::Goto,
        uint32_t targetPageId_ = 0)
        : sourcePageId(sourcePageId_), action(action_), targetPageId(targetPageId_) {}

    uint32_t sourcePageId;
    OfflineDemoNavigationAction action;
    uint32_t targetPageId;
};

struct OfflineDemoScenario {
    constexpr OfflineDemoScenario(
        const uint32_t* pageOrder_ = nullptr,
        size_t pageOrderCount_ = 0,
        const OfflineDemoButtonRoute* buttonRoutes_ = nullptr,
        size_t buttonRouteCount_ = 0,
        const OfflineDemoPageTapRoute* pageTapRoutes_ = nullptr,
        size_t pageTapRouteCount_ = 0)
        : pageOrder(pageOrder_),
          pageOrderCount(pageOrderCount_),
          buttonRoutes(buttonRoutes_),
          buttonRouteCount(buttonRouteCount_),
          pageTapRoutes(pageTapRoutes_),
          pageTapRouteCount(pageTapRouteCount_) {}

    const uint32_t* pageOrder;
    size_t pageOrderCount;
    const OfflineDemoButtonRoute* buttonRoutes;
    size_t buttonRouteCount;
    const OfflineDemoPageTapRoute* pageTapRoutes;
    size_t pageTapRouteCount;
};

// Возвращает основной сценарий offline demo для Screen32.
// Это главное место, где удобно задавать маршрут demo-экрана.
const OfflineDemoScenario& screen32_default_offline_demo_scenario();

} // namespace demo
