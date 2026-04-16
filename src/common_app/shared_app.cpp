#include "shared_app.h"
#include "app_core.h"
#include "frontend_config.h"
#include "frontend_runtime.h"
#include "navigation.h"

extern "C" {
#include "ui/screens.h"
#include "ui/ui.h"
}

namespace demo {

namespace {

/*
 * Файл shared_app.cpp
 * Назначение: содержит общую прикладную логику поверх app_core/navigation.
 * Файл не зависит от конкретной платформы и используется как web, так и ESP32
 * обвязкой.
 */

bool g_fallback_mode = false;
uint32_t g_fallback_last_tick = 0;
FrontendConfig g_frontend_config = frontend_default_config();
bool g_offline_navigation_mode = true;

bool ui_objects_ready() {
    return objects.load != nullptr &&
           objects.main_menu != nullptr &&
           objects.def_page1 != nullptr &&
           objects.def_page2 != nullptr &&
           objects.def_page3 != nullptr &&
           objects.def_page4 != nullptr;
}

void log_ui_objects() {
    // При необходимости здесь можно включить платформенный лог состояния UI-объектов.
    // platform_log("[DEMO] obj load=%p main=%p d1=%p d2=%p d3=%p d4=%p\n",
    //             objects.load, objects.main_menu,
    //             objects.def_page1, objects.def_page2,
    //             objects.def_page3, objects.def_page4);
}

void show_fallback_screen() {
    lv_obj_t *screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x20252c), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "EEZ UI allocation failed");
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(label);

    lv_scr_load(screen);
    lv_refr_now(nullptr);
    g_fallback_mode = true;
}

void bind_direct_routes() {
    const nav::DirectRoute routes[] = {
        {&objects.b_main_task, SCREEN_ID_DEF_PAGE1},
        {&objects.next_2, SCREEN_ID_DEF_PAGE2},
        {&objects.next_5, SCREEN_ID_DEF_PAGE3},
        {&objects.next_9, SCREEN_ID_DEF_PAGE4},
        {&objects.next_12, SCREEN_ID_MAIN_MENU},
    };

    for (const auto &route : routes) {
        if (*route.button != nullptr) {
            nav::bind_button(*route.button, nav::goto_action(route.target, true));
        }
    }
}

void bind_back_buttons() {
    lv_obj_t *back_buttons[] = {
        objects.back,
        objects.back_1,
        objects.back_3,
        objects.back_4
    };

    for (lv_obj_t *button : back_buttons) {
        if (button != nullptr) {
            nav::bind_button(button, nav::back_action());
        }
    }
}

void configure_navigation() {
    // Собираем все необходимые привязки кнопок к действиям навигации.
    nav::init();
    bind_direct_routes();
    bind_back_buttons();
    // platform_log("[DEMO] nav bindings=%u\n", (unsigned)nav::binding_count()); // Отладка.
}

ScreensEnum sanitize_start_page(uint32_t pageId) {
    if (pageId < static_cast<uint32_t>(_SCREEN_ID_FIRST) ||
        pageId > static_cast<uint32_t>(_SCREEN_ID_LAST)) {
        return SCREEN_ID_MAIN_MENU;
    }
    return static_cast<ScreensEnum>(pageId);
}

void apply_start_page(ScreensEnum page) {
    if (g_offline_navigation_mode) {
        nav::go_to(page, false);
        return;
    }
    loadScreen(page);
}

} // namespace

void app_setup() {
    // Платформенная инициализация выполняется на уровне платформенной точки входа.
    // platform_log("[DEMO] setup begin\n");

    app_core_init();
    g_fallback_last_tick = platform_tick_ms();
    log_ui_objects();

    if (!ui_objects_ready()) {
        // Если экраны не создались, показываем безопасный fallback-экран.
        // platform_log("[DEMO] ERROR: screen objects are null, switching to fallback\n");
        show_fallback_screen();
        return;
    }

    // Загружаем frontend-config и поднимаем общий runtime screen client.
    frontend_load_config(g_frontend_config);
    frontend_runtime_init(g_frontend_config);
    g_offline_navigation_mode = frontend_runtime_is_offline_demo();

    if (g_offline_navigation_mode) {
        // В offline_demo локальная навигация живет в текущем navigation.*
        configure_navigation();
    }

    apply_start_page(sanitize_start_page(g_frontend_config.startPage));
    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(nullptr);

    // platform_log("[DEMO] setup done\n");
}

void app_loop() {
    if (!g_fallback_mode) {
        app_core_tick();
        frontend_runtime_tick();
    } else {
        // Даже в fallback режиме нужно обслуживать таймеры LVGL с корректным delta.
        const uint32_t now = platform_tick_ms();
        const uint32_t delta = now - g_fallback_last_tick;
        if (delta > 0) {
            lv_tick_inc(delta);
        }
        g_fallback_last_tick = now;
        lv_timer_handler();
    }
    // Платформенная задержка делается на стороне платформенной обвязки.
}

} // namespace demo
