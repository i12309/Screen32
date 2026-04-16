#ifndef LV_CONF_H
#define LV_CONF_H

/* clang-format отключен */
/* Файл demo_web/lv_conf.h
 * Назначение: конфигурация LVGL для web-сборки (Emscripten/SDL). */

/*=========================
   ГЛУБИНА ЦВЕТА
 *=========================*/
#define LV_COLOR_DEPTH 16

/*=========================
   НАСТРОЙКИ ПАМЯТИ
 *=========================*/
/* Используем стандартный malloc из C-библиотеки */
#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB

/*=========================
   МОНИТОРЫ
 *=========================*/
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

/*=========================
   ДРАЙВЕРЫ ОТОБРАЖЕНИЯ
 *=========================*/
#define LV_USE_SDL 1

/* Размер SDL-окна/канваса для web-версии */
#define LV_SDL_WINDOWED 1
#define LV_SDL_WINDOW_WIDTH  800
#define LV_SDL_WINDOW_HEIGHT 480

/*=========================
   ШРИФТЫ
 *=========================*/
#define LV_FONT_DEFAULT &lv_font_montserrat_14
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1

/*=========================
   ВИДЖЕТЫ
 *=========================*/
/* Включаем базовые виджеты, которые использует EEZ UI */
#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  1
#define LV_USE_CANVAS     1
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   1
#define LV_USE_IMG        1
#define LV_USE_LABEL      1
#define LV_USE_LINE       1
#define LV_USE_ROLLER     1
#define LV_USE_SLIDER     1
#define LV_USE_SWITCH     1
#define LV_USE_TABLE      1
#define LV_USE_TEXTAREA   1

#endif /* LV_CONF_H */
