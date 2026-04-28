#include "demo/offline_demo_scenario.h"

#include "element_ids.generated.h"
#include "page_ids.generated.h"

namespace demo {

namespace {

constexpr uint32_t kDefaultPageOrder[] = {
    scr_LOAD0,
    scr_LOAD,
    scr_MAIN,
    scr_TASK_RUN,
    scr_TASK_PROCESS,
    scr_INFO,
    scr_INPUT,
    scr_INIT,
    scr_WAIT,
    scr_KEYBOARD,
    scr_SERVICE,
    scr_SERVICE2
};

constexpr OfflineDemoButtonRoute kDefaultButtonRoutes[] = {
    {btn_MAIN_TASK, OfflineDemoNavigationAction::Goto, scr_TASK_RUN},
    {btn_MAIN_PROFILE, OfflineDemoNavigationAction::Goto, scr_INFO},
    {btn_MAIN_NET, OfflineDemoNavigationAction::Goto, scr_INIT},
    {btn_MAIN_SERVICE, OfflineDemoNavigationAction::Goto, scr_SERVICE},
    {btn_MAIN_STATS, OfflineDemoNavigationAction::Goto, scr_WAIT},

    {btn_TASK_RUN_BACK, OfflineDemoNavigationAction::Back, 0},
    {btn_TASK_RUN_LIST_TASK, OfflineDemoNavigationAction::Goto, scr_INFO},
    {btn_TASK_RUN_LIST_PROFILE, OfflineDemoNavigationAction::Goto, scr_INPUT},
    {btn_TASK_RUN_LABEL, OfflineDemoNavigationAction::Goto, scr_INFO},
    {btn_TASK_RUN_CYCLES, OfflineDemoNavigationAction::Goto, scr_INPUT},
    {btn_TASK_RUN_PLUS, OfflineDemoNavigationAction::Next, 0},
    {btn_TASK_RUN_MINUS, OfflineDemoNavigationAction::Prev, 0},
    {btn_TASK_RUN_START, OfflineDemoNavigationAction::Goto, scr_TASK_PROCESS},

    {btn_INFO_BACK, OfflineDemoNavigationAction::Back, 0},
    {btn_INFO_NEXT, OfflineDemoNavigationAction::Goto, scr_INPUT},
    {btn_INFO_FIELD1, OfflineDemoNavigationAction::Goto, scr_INPUT},
    {btn_INFO_FIELD2, OfflineDemoNavigationAction::Goto, scr_INPUT},
    {btn_INFO_FIELD3, OfflineDemoNavigationAction::Goto, scr_INPUT},
    {btn_INFO_CANCEL, OfflineDemoNavigationAction::Back, 0},
    {btn_INFO_OK, OfflineDemoNavigationAction::Goto, scr_WAIT},

    {btn_INPUT_FIELD1, OfflineDemoNavigationAction::Goto, scr_KEYBOARD},
    {btn_INPUT_FIELD2, OfflineDemoNavigationAction::Goto, scr_KEYBOARD},
    {btn_INPUT_FIELD3, OfflineDemoNavigationAction::Goto, scr_KEYBOARD},
    {btn_INPUT_FIELD4, OfflineDemoNavigationAction::Goto, scr_KEYBOARD},
    {btn_INPUT_CANCEL, OfflineDemoNavigationAction::Back, 0},
    {btn_INPUT_OK, OfflineDemoNavigationAction::Back, 0},

    {btn_INIT_HTTP, OfflineDemoNavigationAction::Goto, scr_WAIT},
    {btn_INIT_OK, OfflineDemoNavigationAction::Goto, scr_MAIN},
    {btn_INIT_GROUP, OfflineDemoNavigationAction::Goto, scr_KEYBOARD},
    {btn_INIT_NAME, OfflineDemoNavigationAction::Goto, scr_KEYBOARD},
    {btn_INIT_ACCESS_POINT, OfflineDemoNavigationAction::Goto, scr_KEYBOARD},
    {btn_INIT_TEST, OfflineDemoNavigationAction::Goto, scr_WAIT},

    {btn_WAIT_TEXT1, OfflineDemoNavigationAction::Next, 0},
    {btn_WAIT_TEXT2, OfflineDemoNavigationAction::Next, 0},
    {btn_WAIT_TEXT3, OfflineDemoNavigationAction::Next, 0},

    {btn_SERVICE_BACK, OfflineDemoNavigationAction::Back, 0},
    {btn_NEXT_2, OfflineDemoNavigationAction::Goto, scr_SERVICE2},
    {btn_SERVICE_SLICE, OfflineDemoNavigationAction::Goto, scr_WAIT},
    {btn_SERVICE_CALIBRATION, OfflineDemoNavigationAction::Goto, scr_INFO},
    {btn_SERVICE_PROBA, OfflineDemoNavigationAction::Goto, scr_INPUT},

    {btn_SERVICE2_BACK, OfflineDemoNavigationAction::Back, 0}
};

constexpr OfflineDemoPageTapRoute kDefaultPageTapRoutes[] = {
    {scr_LOAD0, OfflineDemoNavigationAction::Next, 0},
    {scr_LOAD, OfflineDemoNavigationAction::Next, 0},
    //{scr_TASK_PROCESS, OfflineDemoNavigationAction::Back, 0},
    {scr_WAIT, OfflineDemoNavigationAction::Next, 0},
    {scr_KEYBOARD, OfflineDemoNavigationAction::Back, 0},
};

constexpr OfflineDemoScenario kDefaultScenario = {
    kDefaultPageOrder,
    sizeof(kDefaultPageOrder) / sizeof(kDefaultPageOrder[0]),
    kDefaultButtonRoutes,
    sizeof(kDefaultButtonRoutes) / sizeof(kDefaultButtonRoutes[0]),
    kDefaultPageTapRoutes,
    sizeof(kDefaultPageTapRoutes) / sizeof(kDefaultPageTapRoutes[0]),
};

} // namespace

const OfflineDemoScenario& screen32_default_offline_demo_scenario() {
    return kDefaultScenario;
}

} // namespace demo
