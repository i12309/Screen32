#include "demo/offline_demo_controller.h"

#include <lvgl.h>

#include "element_descriptors.generated.h"
#include "page_descriptors.generated.h"
#include "ui_object_map.generated.h"

namespace demo {

namespace {

bool is_known_page_id(uint32_t pageId) {
    return screen32_find_page_descriptor(pageId) != nullptr;
}

bool is_known_element_id(uint32_t elementId) {
    return screen32_find_element_descriptor(elementId) != nullptr;
}

} // namespace

void OfflineDemoController::init(screenlib::adapter::EezLvglAdapter* adapter,
                                 const Screen32BoundElement* trackedElements,
                                 size_t trackedCount) {
    reset();
    _adapter = adapter;
    _trackedElements = trackedElements;
    _trackedCount = trackedCount;
}

void OfflineDemoController::reset() {
    _pageOrderCount = 0;
    _bindingCount = 0;
    _pageTapBindingCount = 0;
    _historyCount = 0;
    _currentPageId = 0;
    _trackedElements = nullptr;
    _trackedCount = 0;
}

bool OfflineDemoController::setPageOrder(const uint32_t* pageIds, size_t count) {
    if (pageIds == nullptr || count == 0 || count > kMaxPages) {
        return false;
    }

    for (size_t i = 0; i < count; ++i) {
        if (!is_known_page_id(pageIds[i])) {
            return false;
        }
        for (size_t j = i + 1; j < count; ++j) {
            if (pageIds[i] == pageIds[j]) {
                return false;
            }
        }
    }

    for (size_t i = 0; i < count; ++i) {
        _pageOrder[i] = pageIds[i];
    }
    _pageOrderCount = count;
    return true;
}

bool OfflineDemoController::bindButtonToNext(uint32_t elementId) {
    return setBinding(elementId, BindingActionType::Next, 0);
}

bool OfflineDemoController::bindButtonToPrev(uint32_t elementId) {
    return setBinding(elementId, BindingActionType::Prev, 0);
}

bool OfflineDemoController::bindButtonToBack(uint32_t elementId) {
    return setBinding(elementId, BindingActionType::Back, 0);
}

bool OfflineDemoController::bindButtonToGoto(uint32_t elementId, uint32_t targetPageId) {
    if (!is_known_page_id(targetPageId)) {
        return false;
    }
    return setBinding(elementId, BindingActionType::Goto, targetPageId);
}

bool OfflineDemoController::bindPageTapToNext(uint32_t sourcePageId) {
    return setPageTapBinding(sourcePageId, BindingActionType::Next, 0);
}

bool OfflineDemoController::bindPageTapToPrev(uint32_t sourcePageId) {
    return setPageTapBinding(sourcePageId, BindingActionType::Prev, 0);
}

bool OfflineDemoController::bindPageTapToBack(uint32_t sourcePageId) {
    return setPageTapBinding(sourcePageId, BindingActionType::Back, 0);
}

bool OfflineDemoController::bindPageTapToGoto(uint32_t sourcePageId, uint32_t targetPageId) {
    if (!is_known_page_id(targetPageId)) {
        return false;
    }
    return setPageTapBinding(sourcePageId, BindingActionType::Goto, targetPageId);
}

bool OfflineDemoController::configureScenario(const OfflineDemoScenario& scenario) {
    if (scenario.pageOrder == nullptr || scenario.pageOrderCount == 0) {
        return false;
    }

    if (!setPageOrder(scenario.pageOrder, scenario.pageOrderCount)) {
        return false;
    }

    bool ok = true;

    for (size_t i = 0; i < scenario.buttonRouteCount; ++i) {
        const OfflineDemoButtonRoute& route = scenario.buttonRoutes[i];
        switch (route.action) {
            case OfflineDemoNavigationAction::Next:
                ok = bindButtonToNext(route.elementId) && ok;
                break;
            case OfflineDemoNavigationAction::Prev:
                ok = bindButtonToPrev(route.elementId) && ok;
                break;
            case OfflineDemoNavigationAction::Back:
                ok = bindButtonToBack(route.elementId) && ok;
                break;
            case OfflineDemoNavigationAction::Goto:
            default:
                ok = bindButtonToGoto(route.elementId, route.targetPageId) && ok;
                break;
        }
    }

    for (size_t i = 0; i < scenario.pageTapRouteCount; ++i) {
        const OfflineDemoPageTapRoute& route = scenario.pageTapRoutes[i];
        switch (route.action) {
            case OfflineDemoNavigationAction::Next:
                ok = bindPageTapToNext(route.sourcePageId) && ok;
                break;
            case OfflineDemoNavigationAction::Prev:
                ok = bindPageTapToPrev(route.sourcePageId) && ok;
                break;
            case OfflineDemoNavigationAction::Back:
                ok = bindPageTapToBack(route.sourcePageId) && ok;
                break;
            case OfflineDemoNavigationAction::Goto:
            default:
                ok = bindPageTapToGoto(route.sourcePageId, route.targetPageId) && ok;
                break;
        }
    }

    return ok;
}

bool OfflineDemoController::configureDefaultDemo() {
    return configureScenario(screen32_default_offline_demo_scenario());
}

bool OfflineDemoController::start(uint32_t startPageId) {
    if (_pageOrderCount == 0) {
        if (!configureDefaultDemo()) {
            return false;
        }
    }

    uint32_t resolvedPageId = 0;
    if (!pickStartPage(startPageId, resolvedPageId)) {
        return false;
    }

    return showPage(resolvedPageId, false);
}

bool OfflineDemoController::onButtonEvent(uint32_t elementId, uint32_t sourcePageId) {
    const Binding* binding = findBinding(elementId);
    if (binding == nullptr) {
        return false;
    }
    return applyAction(binding->action, binding->targetPageId, sourcePageId);
}

bool OfflineDemoController::onObjectClick(uint32_t sourcePageId) {
    const PageTapBinding* binding = findPageTapBinding(sourcePageId);
    if (binding == nullptr) {
        return false;
    }
    return applyAction(binding->action, binding->targetPageId, sourcePageId);
}

bool OfflineDemoController::onInputEventInt(uint32_t elementId, uint32_t sourcePageId, int32_t value) {
    (void)value;
    return onButtonEvent(elementId, sourcePageId);
}

bool OfflineDemoController::onInputEventText(uint32_t elementId, uint32_t sourcePageId, const char* value) {
    (void)value;
    return onButtonEvent(elementId, sourcePageId);
}

uint32_t OfflineDemoController::currentPage() const {
    return _currentPageId;
}

bool OfflineDemoController::setBinding(uint32_t elementId, BindingActionType action, uint32_t targetPageId) {
    if (!is_known_element_id(elementId)) {
        return false;
    }

    for (size_t i = 0; i < _bindingCount; ++i) {
        if (_bindings[i].elementId == elementId) {
            _bindings[i].action = action;
            _bindings[i].targetPageId = targetPageId;
            return true;
        }
    }

    if (_bindingCount >= kMaxBindings) {
        return false;
    }

    _bindings[_bindingCount].elementId = elementId;
    _bindings[_bindingCount].action = action;
    _bindings[_bindingCount].targetPageId = targetPageId;
    _bindingCount++;
    return true;
}

const OfflineDemoController::Binding* OfflineDemoController::findBinding(uint32_t elementId) const {
    for (size_t i = 0; i < _bindingCount; ++i) {
        if (_bindings[i].elementId == elementId) {
            return &_bindings[i];
        }
    }
    return nullptr;
}

bool OfflineDemoController::setPageTapBinding(uint32_t sourcePageId,
                                              BindingActionType action,
                                              uint32_t targetPageId) {
    if (!is_known_page_id(sourcePageId)) {
        return false;
    }

    for (size_t i = 0; i < _pageTapBindingCount; ++i) {
        if (_pageTapBindings[i].sourcePageId == sourcePageId) {
            _pageTapBindings[i].action = action;
            _pageTapBindings[i].targetPageId = targetPageId;
            return true;
        }
    }

    if (_pageTapBindingCount >= kMaxPageTapBindings) {
        return false;
    }

    _pageTapBindings[_pageTapBindingCount].sourcePageId = sourcePageId;
    _pageTapBindings[_pageTapBindingCount].action = action;
    _pageTapBindings[_pageTapBindingCount].targetPageId = targetPageId;
    _pageTapBindingCount++;
    return true;
}

const OfflineDemoController::PageTapBinding* OfflineDemoController::findPageTapBinding(
    uint32_t sourcePageId) const {
    for (size_t i = 0; i < _pageTapBindingCount; ++i) {
        if (_pageTapBindings[i].sourcePageId == sourcePageId) {
            return &_pageTapBindings[i];
        }
    }
    return nullptr;
}

bool OfflineDemoController::applyAction(BindingActionType action,
                                        uint32_t targetPageId,
                                        uint32_t sourcePageId) {
    if (_pageOrderCount == 0) {
        return false;
    }

    if (action == BindingActionType::Back) {
        uint32_t previousPageId = 0;
        if (!popHistory(previousPageId)) {
            return false;
        }
        return showPage(previousPageId, false);
    }

    if (action == BindingActionType::Goto) {
        return showPage(targetPageId);
    }

    int currentIndex = findPageIndex(_currentPageId);
    if (currentIndex < 0) {
        currentIndex = findPageIndex(sourcePageId);
    }
    if (currentIndex < 0) {
        currentIndex = 0;
    }

    if (action == BindingActionType::Next) {
        const size_t nextIndex = (static_cast<size_t>(currentIndex) + 1) % _pageOrderCount;
        return showPage(_pageOrder[nextIndex]);
    }

    const size_t prevIndex = (static_cast<size_t>(currentIndex) + _pageOrderCount - 1) % _pageOrderCount;
    return showPage(_pageOrder[prevIndex]);
}

bool OfflineDemoController::showPage(uint32_t pageId, bool rememberCurrentPage) {
    if (_adapter == nullptr) {
        return false;
    }
    if (!_adapter->showPage(pageId)) {
        return false;
    }
    if (rememberCurrentPage && _currentPageId != 0 && _currentPageId != pageId) {
        pushHistory(_currentPageId);
    }
    _currentPageId = pageId;
    return true;
}

int OfflineDemoController::findPageIndex(uint32_t pageId) const {
    for (size_t i = 0; i < _pageOrderCount; ++i) {
        if (_pageOrder[i] == pageId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool OfflineDemoController::pickStartPage(uint32_t requestedPageId, uint32_t& outPageId) const {
    if (_pageOrderCount == 0) {
        return false;
    }

    if (requestedPageId != 0 && findPageIndex(requestedPageId) >= 0) {
        outPageId = requestedPageId;
        return true;
    }

    outPageId = _pageOrder[0];
    return true;
}

const Screen32BoundElement* OfflineDemoController::findTrackedElement(uint32_t elementId) const {
    if (_trackedElements == nullptr || _trackedCount == 0) {
        return nullptr;
    }
    return screen32_find_bound_element(_trackedElements, _trackedCount, elementId);
}

bool OfflineDemoController::isButtonInsideBar(uint32_t buttonElementId,
                                              const uint32_t* barElementIds,
                                              size_t barCount) const {
    if (barElementIds == nullptr || barCount == 0) {
        return false;
    }

    const Screen32BoundElement* button = findTrackedElement(buttonElementId);
    if (button == nullptr || button->obj == nullptr || !lv_obj_is_valid(button->obj)) {
        return false;
    }

    for (size_t i = 0; i < barCount; ++i) {
        const Screen32BoundElement* bar = findTrackedElement(barElementIds[i]);
        if (bar == nullptr || bar->obj == nullptr || !lv_obj_is_valid(bar->obj)) {
            continue;
        }

        lv_obj_t* parent = lv_obj_get_parent(button->obj);
        while (parent != nullptr) {
            if (parent == bar->obj) {
                return true;
            }
            if (!lv_obj_is_valid(parent)) {
                break;
            }
            parent = lv_obj_get_parent(parent);
        }
    }

    return false;
}

void OfflineDemoController::pushHistory(uint32_t pageId) {
    if (pageId == 0) {
        return;
    }

    if (_historyCount > 0 && _history[_historyCount - 1] == pageId) {
        return;
    }

    if (_historyCount >= kMaxHistoryDepth) {
        for (size_t i = 1; i < _historyCount; ++i) {
            _history[i - 1] = _history[i];
        }
        _historyCount = kMaxHistoryDepth - 1;
    }

    _history[_historyCount++] = pageId;
}

bool OfflineDemoController::popHistory(uint32_t& outPageId) {
    if (_historyCount == 0) {
        return false;
    }

    outPageId = _history[--_historyCount];
    return true;
}

} // namespace demo
