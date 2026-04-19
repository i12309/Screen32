#include "common_app/offline_demo_controller.h"

#include "element_descriptors.generated.h"
#include "element_ids.generated.h"
#include "page_descriptors.generated.h"
#include "page_ids.generated.h"

namespace demo {

namespace {

bool is_known_page_id(uint32_t pageId) {
    return screen32_find_page_descriptor(pageId) != nullptr;
}

bool is_known_element_id(uint32_t elementId) {
    return screen32_find_element_descriptor(elementId) != nullptr;
}

} // namespace

void OfflineDemoController::init(screenlib::adapter::EezLvglAdapter* adapter) {
    reset();
    _adapter = adapter;
}

void OfflineDemoController::reset() {
    _pageOrderCount = 0;
    _bindingCount = 0;
    _pageTapBindingCount = 0;
    _currentPageId = 0;
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

bool OfflineDemoController::bindPageTapToGoto(uint32_t sourcePageId, uint32_t targetPageId) {
    if (!is_known_page_id(targetPageId)) {
        return false;
    }
    return setPageTapBinding(sourcePageId, BindingActionType::Goto, targetPageId);
}

bool OfflineDemoController::configureDefaultDemo() {
    const uint32_t pageOrder[] = {
        scr_LOAD,
        scr_MAIN,
        scr_DEF_PAGE,
        scr_DEF_PAGE2,
        scr_DEF_PAGE3,
        scr_DEF_PAGE4,
    };
    if (!setPageOrder(pageOrder, sizeof(pageOrder) / sizeof(pageOrder[0]))) {
        return false;
    }

    bool ok = true;
    ok = bindPageTapToNext(scr_LOAD) && ok;
    ok = bindButtonToGoto(btn_MAIN_TASK, scr_DEF_PAGE) && ok;
    ok = bindButtonToGoto(btn_NEXT_2, scr_DEF_PAGE2) && ok;
    ok = bindButtonToGoto(btn_NEXT_13, scr_DEF_PAGE3) && ok;
    ok = bindButtonToGoto(btn_NEXT_7, scr_DEF_PAGE4) && ok;
    ok = bindButtonToGoto(btn_NEXT_10, scr_MAIN) && ok;

    ok = bindButtonToPrev(btn_BACK_5) && ok;
    ok = bindButtonToPrev(btn_BACK_3) && ok;
    ok = bindButtonToPrev(btn_BACK_4) && ok;

    return ok;
}

bool OfflineDemoController::start(uint32_t startPageId) {
    // Держим demo-правила самодостаточными: если вызывающий код не задал свой порядок и bindings,
    // контроллер применяет встроенные настройки до первого запуска.
    if (_pageOrderCount == 0) {
        if (!configureDefaultDemo()) {
            return false;
        }
    }

    uint32_t resolvedPageId = 0;
    if (!pickStartPage(startPageId, resolvedPageId)) {
        return false;
    }
    return showPage(resolvedPageId);
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

bool OfflineDemoController::showPage(uint32_t pageId) {
    if (_adapter == nullptr) {
        return false;
    }
    if (!_adapter->showPage(pageId)) {
        return false;
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

} // namespace demo


