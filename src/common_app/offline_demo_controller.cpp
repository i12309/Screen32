#include "common_app/offline_demo_controller.h"

#include <ctype.h>
#include <string.h>

#include <lvgl.h>

#include "element_descriptors.generated.h"
#include "ui_object_map.generated.h"
#include "page_descriptors.generated.h"

namespace demo {

namespace {

constexpr size_t kMaxButtonsPerPage = 32;
constexpr size_t kMaxBarsPerPage = 8;

bool is_known_page_id(uint32_t pageId) {
    return screen32_find_page_descriptor(pageId) != nullptr;
}

bool is_known_element_id(uint32_t elementId) {
    return screen32_find_element_descriptor(elementId) != nullptr;
}

char to_lower_ascii(char ch) {
    return static_cast<char>(tolower(static_cast<unsigned char>(ch)));
}

bool starts_with_ignore_case(const char* text, const char* prefix) {
    if (text == nullptr || prefix == nullptr) {
        return false;
    }

    while (*prefix != '\0') {
        if (*text == '\0') {
            return false;
        }
        if (to_lower_ascii(*text) != to_lower_ascii(*prefix)) {
            return false;
        }
        ++text;
        ++prefix;
    }

    return true;
}

bool contains_ignore_case(const char* text, const char* token) {
    if (text == nullptr || token == nullptr || *token == '\0') {
        return false;
    }

    const size_t textLen = strlen(text);
    const size_t tokenLen = strlen(token);
    if (tokenLen > textLen) {
        return false;
    }

    for (size_t i = 0; i + tokenLen <= textLen; ++i) {
        bool match = true;
        for (size_t j = 0; j < tokenLen; ++j) {
            if (to_lower_ascii(text[i + j]) != to_lower_ascii(token[j])) {
                match = false;
                break;
            }
        }
        if (match) {
            return true;
        }
    }

    return false;
}

bool is_bar_container_descriptor(const Screen32ElementDescriptor& descriptor) {
    if (descriptor.element_type != TYPE_CONTAINER) {
        return false;
    }
    return starts_with_ignore_case(descriptor.object_name, "c_bar") ||
           starts_with_ignore_case(descriptor.element_name, "cnt_BAR");
}

bool is_back_button_descriptor(const Screen32ElementDescriptor* descriptor) {
    if (descriptor == nullptr) {
        return false;
    }
    return contains_ignore_case(descriptor->element_name, "back") ||
           contains_ignore_case(descriptor->object_name, "back");
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
    uint32_t pageOrder[kMaxPages] = {};
    const size_t availablePageCount = screen32_page_descriptor_count();
    if (availablePageCount == 0 || availablePageCount > kMaxPages) {
        return false;
    }

    for (size_t i = 0; i < availablePageCount; ++i) {
        pageOrder[i] = g_screen32_page_descriptors[i].page_id;
    }

    if (!setPageOrder(pageOrder, availablePageCount)) {
        return false;
    }

    bool ok = true;

    for (size_t pageIndex = 0; pageIndex < availablePageCount; ++pageIndex) {
        const uint32_t pageId = pageOrder[pageIndex];

        uint32_t pageButtons[kMaxButtonsPerPage] = {};
        size_t pageButtonCount = 0;
        uint32_t barElements[kMaxBarsPerPage] = {};
        size_t barCount = 0;
        bool hasBar = false;

        const size_t elementCount = screen32_element_descriptor_count();
        for (size_t i = 0; i < elementCount; ++i) {
            const Screen32ElementDescriptor& descriptor = g_screen32_element_descriptors[i];
            if (descriptor.page_id != pageId) {
                continue;
            }

            if (is_bar_container_descriptor(descriptor)) {
                hasBar = true;
                if (barCount < kMaxBarsPerPage) {
                    barElements[barCount++] = descriptor.element_id;
                }
            }

            if (descriptor.emits_button_event && pageButtonCount < kMaxButtonsPerPage) {
                pageButtons[pageButtonCount++] = descriptor.element_id;
            }
        }

        uint32_t navButtons[kMaxButtonsPerPage] = {};
        size_t navButtonCount = 0;

        if (hasBar && pageButtonCount > 0) {
            for (size_t i = 0; i < pageButtonCount; ++i) {
                if (isButtonInsideBar(pageButtons[i], barElements, barCount)) {
                    navButtons[navButtonCount++] = pageButtons[i];
                }
            }
        }

        if (navButtonCount == 0) {
            for (size_t i = 0; i < pageButtonCount; ++i) {
                navButtons[navButtonCount++] = pageButtons[i];
            }
        }

        bool hasPageBinding = false;

        if (hasBar) {
            bool hasBack = false;
            uint32_t firstNonBackButton = 0;
            for (size_t i = 0; i < navButtonCount; ++i) {
                const uint32_t buttonId = navButtons[i];
                const Screen32ElementDescriptor* descriptor = screen32_find_element_descriptor(buttonId);
                if (is_back_button_descriptor(descriptor)) {
                    ok = bindButtonToPrev(buttonId) && ok;
                    hasBack = true;
                    hasPageBinding = true;
                    continue;
                }
                if (firstNonBackButton == 0) {
                    firstNonBackButton = buttonId;
                }
            }

            if (hasBack) {
                if (firstNonBackButton == 0) {
                    for (size_t i = 0; i < pageButtonCount; ++i) {
                        const uint32_t buttonId = pageButtons[i];
                        const Screen32ElementDescriptor* descriptor = screen32_find_element_descriptor(buttonId);
                        if (!is_back_button_descriptor(descriptor)) {
                            firstNonBackButton = buttonId;
                            break;
                        }
                    }
                }
                if (firstNonBackButton != 0) {
                    ok = bindButtonToNext(firstNonBackButton) && ok;
                    hasPageBinding = true;
                }
            } else {
                uint32_t prevCandidate = 0;
                uint32_t nextCandidate = 0;

                if (navButtonCount >= 1) {
                    prevCandidate = navButtons[0];
                } else if (pageButtonCount >= 1) {
                    prevCandidate = pageButtons[0];
                }

                if (navButtonCount >= 2) {
                    nextCandidate = navButtons[1];
                } else {
                    for (size_t i = 0; i < pageButtonCount; ++i) {
                        if (pageButtons[i] != prevCandidate) {
                            nextCandidate = pageButtons[i];
                            break;
                        }
                    }
                }

                if (prevCandidate != 0) {
                    ok = bindButtonToPrev(prevCandidate) && ok;
                    hasPageBinding = true;
                }
                if (nextCandidate != 0) {
                    ok = bindButtonToNext(nextCandidate) && ok;
                    hasPageBinding = true;
                }
            }
        } else if (pageButtonCount > 0) {
            ok = bindButtonToNext(pageButtons[0]) && ok;
            hasPageBinding = true;
        }

        if (!hasPageBinding) {
            ok = bindPageTapToNext(pageId) && ok;
        }
    }

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

} // namespace demo


