#ifndef LV_CONF_H
#define LV_CONF_H

/* clang-format off */
/* Файл include/lv_conf.h
 * Назначение: минимальная конфигурация LVGL для ESP32-сборки. */

// Для панели 800x480 используется цветовой формат RGB565.
#define LV_COLOR_DEPTH 16

// Используем системный аллокатор вместо фиксированного пула LVGL.
// Это снижает риск нехватки памяти на сложных экранах EEZ.
#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB

// Отключаем служебные мониторы для упрощённой тестовой конфигурации.
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

// Экран Keyboard из EEZ использует встроенный шрифт Montserrat 26.
//#define LV_FONT_MONTSERRAT_26 1

#endif /* LV_CONF_H */
