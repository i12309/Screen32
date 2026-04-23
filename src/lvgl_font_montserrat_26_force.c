#include <lvgl.h>

/*
 * Явно подключаем built-in font 26 px в основную сборку.
 * EEZ-сгенерированный screens.c ссылается на lv_font_montserrat_26,
 * а в текущей схеме сборки зависимостей символ не доезжает до линковки.
 */
//#include "../lib/ScreenUI/vendor/lvgl-release-v9.4/lvgl-release-v9.4/src/font/lv_font_montserrat_26.c"
