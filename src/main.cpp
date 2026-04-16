#include "platform_esp32/platform.h"
#include "common_app/shared_app.h"

/*
 * Файл src/main.cpp
 * Назначение: Arduino-точка входа для ESP32-сборки.
 * Файл соединяет платформенный слой ESP32 и общий app-слой (`common_app/shared_app`):
 * инициализирует платформу, запускает общее приложение и крутит основной цикл.
 */

void setup() {
    // Сначала поднимаем платформу ESP32 (дисплей, тач, UART и т.п.).
    platform_init();
    // Затем запускаем общий прикладной слой, одинаковый для Web и ESP32.
    demo::app_setup();
}

void loop() {
    // Один шаг общего прикладного цикла.
    demo::app_loop();
    // Небольшая пауза для снижения нагрузки на CPU в цикле Arduino.
    platform_delay_ms(5);
}
