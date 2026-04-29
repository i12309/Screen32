#ifndef LV_CONF_H
#define LV_CONF_H

/* clang-format off */
/* Файл include/lv_conf.h
 * Назначение: минимальная конфигурация LVGL для ESP32-сборки. */

// Для панели 800x480 используется цветовой формат RGB565.
#define LV_COLOR_DEPTH 16

// LVGL-аллокатор: встроенный пул, размещаемый в PSRAM.
// Системный malloc() на ESP32 берёт из internal DRAM, а её мало (~290 KB
// под драйверы железа). Большое количество lv_obj/lv_label на сложных EEZ
// страницах быстро её исчерпывает. Поэтому держим LVGL-кучу в PSRAM
// (8 МБ), а DRAM оставляем драйверам (UART/DMA/прерывания).
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB

// Размер пула под lv_malloc. Выделяется один раз через LV_MEM_POOL_ALLOC
// в PSRAM (см. ниже). 512 KB — с запасом под все 26 страниц с 180+ элементами.
#define LV_MEM_SIZE (512 * 1024U)
#define LV_MEM_POOL_EXPAND_SIZE 0

// Используем кастомный аллокатор пула: heap_caps_malloc(SPIRAM).
// LV_MEM_POOL_INCLUDE подключается до использования, поэтому объявим
// функцию через extern "C" заголовок esp_heap_caps.h.
#define LV_MEM_POOL_INCLUDE "esp_heap_caps.h"
#define LV_MEM_POOL_ALLOC(size) heap_caps_malloc((size), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)

// Отключаем служебные мониторы для упрощённой тестовой конфигурации.
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

// Экран Keyboard из EEZ использует встроенный шрифт Montserrat 26.
//#define LV_FONT_MONTSERRAT_26 1

#endif /* LV_CONF_H */
