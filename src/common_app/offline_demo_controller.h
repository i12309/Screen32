#pragma once

#include <stddef.h>
#include <stdint.h>

#include "lvgl_eez/EezLvglAdapter.h"

namespace demo {

class OfflineDemoController {
public:
    OfflineDemoController() = default;

    void init(screenlib::adapter::EezLvglAdapter* adapter);
    void reset();

    bool setPageOrder(const uint32_t* pageIds, size_t count);
    bool bindButtonToNext(uint32_t elementId);
    bool bindButtonToPrev(uint32_t elementId);
    bool bindButtonToGoto(uint32_t elementId, uint32_t targetPageId);

    bool start(uint32_t startPageId);
    bool onButtonEvent(uint32_t elementId, uint32_t sourcePageId);

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

    static constexpr size_t kMaxPages = 16;
    static constexpr size_t kMaxBindings = 64;

    screenlib::adapter::EezLvglAdapter* _adapter = nullptr;
    uint32_t _pageOrder[kMaxPages] = {};
    size_t _pageOrderCount = 0;
    Binding _bindings[kMaxBindings] = {};
    size_t _bindingCount = 0;
    uint32_t _currentPageId = 0;

    bool setBinding(uint32_t elementId, BindingActionType action, uint32_t targetPageId);
    const Binding* findBinding(uint32_t elementId) const;
    bool showPage(uint32_t pageId);
    int findPageIndex(uint32_t pageId) const;
    bool pickStartPage(uint32_t requestedPageId, uint32_t& outPageId) const;
};

} // namespace demo

