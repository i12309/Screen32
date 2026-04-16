#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Файл src/common_app/app_core.h
 * Назначение: интерфейс платформо-независимого ядра цикла UI (ветка `src/`).
 */

// Инициализация UI и внутренних таймерных меток.
void app_core_init(void);

// Основной шаг цикла: обновление тика LVGL, ui_tick и lv_timer_handler.
void app_core_tick(void);

// Текущее время в миллисекундах из платформенного слоя.
uint32_t platform_tick_ms(void);

#ifdef __cplusplus
}
#endif
