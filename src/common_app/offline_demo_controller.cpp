#include "common_app/offline_demo_controller.h"

#include "common_app/generated/element_descriptors.generated.h"
#include "common_app/generated/element_ids.generated.h"
#include "common_app/generated/page_descriptors.generated.h"
#include "common_app/generated/page_ids.generated.h"

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

bool OfflineDemoController::configureDefaultDemo() {
    const uint32_t pageOrder[] = {
        SCREEN32_PAGE_ID_MAIN_MENU,
        SCREEN32_PAGE_ID_DEF_PAGE1,
        SCREEN32_PAGE_ID_DEF_PAGE2,
        SCREEN32_PAGE_ID_DEF_PAGE3,
        SCREEN32_PAGE_ID_DEF_PAGE4,
    };
    if (!setPageOrder(pageOrder, sizeof(pageOrder) / sizeof(pageOrder[0]))) {
        return false;
    }

    bool ok = true;
    ok = bindButtonToGoto(SCREEN32_ELEMENT_ID_B_MAIN_TASK, SCREEN32_PAGE_ID_DEF_PAGE1) && ok;
    ok = bindButtonToGoto(SCREEN32_ELEMENT_ID_NEXT_2, SCREEN32_PAGE_ID_DEF_PAGE2) && ok;
    ok = bindButtonToGoto(SCREEN32_ELEMENT_ID_NEXT_5, SCREEN32_PAGE_ID_DEF_PAGE3) && ok;
    ok = bindButtonToGoto(SCREEN32_ELEMENT_ID_NEXT_9, SCREEN32_PAGE_ID_DEF_PAGE4) && ok;
    ok = bindButtonToGoto(SCREEN32_ELEMENT_ID_NEXT_12, SCREEN32_PAGE_ID_MAIN_MENU) && ok;

    ok = bindButtonToPrev(SCREEN32_ELEMENT_ID_BACK) && ok;
    ok = bindButtonToPrev(SCREEN32_ELEMENT_ID_BACK_1) && ok;
    ok = bindButtonToPrev(SCREEN32_ELEMENT_ID_BACK_3) && ok;
    ok = bindButtonToPrev(SCREEN32_ELEMENT_ID_BACK_4) && ok;

    return ok;
}

bool OfflineDemoController::start(uint32_t startPageId) {
    uint32_t resolvedPageId = 0;
    if (!pickStartPage(startPageId, resolvedPageId)) {
        return false;
    }
    return showPage(resolvedPageId);
}

bool OfflineDemoController::onButtonEvent(uint32_t elementId, uint32_t sourcePageId) {
    (void)sourcePageId;
    const Binding* binding = findBinding(elementId);
    if (binding == nullptr) {
        return false;
    }

    if (_pageOrderCount == 0) {
        return false;
    }

    int currentIndex = findPageIndex(_currentPageId);
    if (currentIndex < 0) {
        currentIndex = 0;
    }

    if (binding->action == BindingActionType::Goto) {
        return showPage(binding->targetPageId);
    }

    if (binding->action == BindingActionType::Next) {
        const size_t nextIndex = (static_cast<size_t>(currentIndex) + 1) % _pageOrderCount;
        return showPage(_pageOrder[nextIndex]);
    }

    const size_t prevIndex = (static_cast<size_t>(currentIndex) + _pageOrderCount - 1) % _pageOrderCount;
    return showPage(_pageOrder[prevIndex]);
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
