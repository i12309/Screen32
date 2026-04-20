#include "shared_app.h"

#include "app_core.h"
#include "frontend_config.h"
#include "frontend_runtime.h"

#include <stdio.h>

extern "C" {
#include "ui/screens.h"
}

namespace demo {

namespace {

bool g_fallback_mode = false;
uint32_t g_fallback_last_tick = 0;
FrontendConfig g_frontend_config = frontend_default_config();

bool ui_objects_ready() {
    return objects.load != nullptr &&
           objects.main != nullptr &&
           objects.def_page != nullptr &&
           objects.def_page2 != nullptr &&
           objects.def_page3 != nullptr &&
           objects.def_page4 != nullptr;
}

void show_fallback_screen() {
    lv_obj_t* screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x20252c), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* label = lv_label_create(screen);
    lv_label_set_text(label, "EEZ UI allocation failed");
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(label);

    lv_scr_load(screen);
    lv_refr_now(nullptr);
    g_fallback_mode = true;
}

} // namespace

void app_setup() {
    app_core_init();
    g_fallback_last_tick = platform_tick_ms();

    if (!ui_objects_ready()) {
        show_fallback_screen();
        return;
    }

    const bool cfgLoaded = frontend_load_config(g_frontend_config);
    printf("[app] frontend config: loaded=%d mode=%s transport=%s offline_demo=%d online_page=%lu offline_page=%lu\n",
           cfgLoaded ? 1 : 0,
           frontend_mode_name(g_frontend_config.mode),
           frontend_transport_name(g_frontend_config.transport.type),
           g_frontend_config.offlineDemo ? 1 : 0,
           static_cast<unsigned long>(g_frontend_config.firstOnlinePage),
           static_cast<unsigned long>(g_frontend_config.firstOfflinePage));

    const bool runtimeReady = frontend_runtime_init(g_frontend_config);
    printf("[app] frontend runtime init: %s\n", runtimeReady ? "ok" : "fail");

    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(nullptr);
}

void app_loop() {
    if (!g_fallback_mode) {
        app_core_tick();
        frontend_runtime_tick();
        return;
    }

    const uint32_t now = platform_tick_ms();
    const uint32_t delta = now - g_fallback_last_tick;
    if (delta > 0) {
        lv_tick_inc(delta);
    }
    g_fallback_last_tick = now;
    lv_timer_handler();
}

} // namespace demo
