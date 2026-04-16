#include "common_app/app_core.h"

#include <lvgl.h>

extern "C" {
#include "ui/ui.h"
}

/*
 * Файл src/common_app/app_core.cpp
 * Назначение: реализация общего цикла UI для ветки `src/`.
 */

// Предыдущее значение времени для расчёта delta.
static uint32_t g_last_tick_ms = 0;

// Флаг аварийного режима, в котором пропускается пользовательский ui_tick().
static bool g_fallback_mode = false;

void app_core_init(void) {
    // Инициализация сгенерированного EEZ UI.
    ui_init();
    g_last_tick_ms = platform_tick_ms();
}

void app_core_tick(void) {
    // Обновляем LVGL-тик на прошедшее время.
    const uint32_t now = platform_tick_ms();
    const uint32_t delta = now - g_last_tick_ms;
    if (delta > 0) {
        lv_tick_inc(delta);
    }
    g_last_tick_ms = now;

    if (!g_fallback_mode) {
        ui_tick();
    }
    // Выполняем таймеры и задачи LVGL.
    lv_timer_handler();
}
