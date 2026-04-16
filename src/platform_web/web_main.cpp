#include "platform_web/platform.h"
#include "common_app/shared_app.h"

#include <emscripten/emscripten.h>

namespace {

/*
 * Файл src/platform_web/web_main.cpp
 * Назначение: точка входа web-сборки.
 * Файл инициализирует web-платформу, запускает общий app-слой
 * и передает основной цикл в `emscripten_set_main_loop_arg`.
 */

void web_loop(void *) {
    demo::app_loop();
}

} // namespace

int main() {
    platform_log("[WEB] main begin\n");
    platform_init();
    platform_log("[WEB] platform init\n");

    demo::app_setup();
    platform_log("[WEB] LVGL v9 initialized\n");

    emscripten_set_main_loop_arg(web_loop, nullptr, 0, 1);
    return 0;
}
