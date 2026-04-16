#include "platform_web/platform.h"

#include <lvgl.h>
#include <cstdarg>
#include <cstdio>

#include <emscripten/emscripten.h>

#include "src/drivers/sdl/lv_sdl_keyboard.h"
#include "src/drivers/sdl/lv_sdl_mouse.h"
#include "src/drivers/sdl/lv_sdl_mousewheel.h"
#include "src/drivers/sdl/lv_sdl_window.h"

namespace {

/*
 * Файл src/platform_web/platform.cpp
 * Назначение: реализация web-платформенного слоя для Emscripten-сборки.
 * Файл поднимает LVGL + SDL-окно 800x480 и создает устройства ввода,
 * а также предоставляет таймер/логирование для общего app-слоя.
 */

bool g_platform_initialized = false;
lv_display_t *g_display = nullptr;

} // namespace

void platform_init(void) {
    if (g_platform_initialized) {
        return;
    }

    lv_init();

    g_display = lv_sdl_window_create(800, 480);
    if (g_display != nullptr) {
        lv_sdl_window_set_title(g_display, "LVGL Simulator");
    }

    lv_sdl_mouse_create();
    lv_sdl_keyboard_create();
#if LV_SDL_MOUSEWHEEL_MODE == LV_SDL_MOUSEWHEEL_MODE_ENCODER
    lv_sdl_mousewheel_create();
#endif

    g_platform_initialized = true;
}

uint32_t platform_tick_ms(void) {
    return (uint32_t)emscripten_get_now();
}

void platform_delay_ms(uint32_t ms) {
    LV_UNUSED(ms);
}

void platform_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
