#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Файл src/platform_web/platform.h
 * Назначение: web-платформенный API (Emscripten/SDL) для общего app-слоя.
 * Файл объявляет тот же набор платформенных функций, что и ESP32-слой,
 * чтобы `common_app` можно было собирать без изменений для обеих платформ.
 */

// Инициализирует LVGL и web-драйверы отображения/ввода через SDL.
void platform_init(void);

// Возвращает текущее время в миллисекундах.
uint32_t platform_tick_ms(void);

// Платформенная задержка (для web обычно no-op, так как цикл управляется браузером).
void platform_delay_ms(uint32_t ms);

// Пишет форматированный лог в консоль браузера.
void platform_log(const char *fmt, ...);

#ifdef __cplusplus
}
#endif
