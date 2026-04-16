#include "platform_esp32/platform.h"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp32_smartdisplay.h>
#include <lvgl.h>
#include <cstdarg>
#include <cstdio>

/*
 * Файл src/platform_esp32/platform.cpp
 * Назначение: реализация платформенного слоя ESP32 для ветки `src/`.
 * Файл отвечает за инициализацию железа и сервисные функции платформы.
 */

void platform_init(void) {
    // Запуск последовательного лога и инициализация экрана/тача.
    Serial.begin(115200);
    Serial.println("[ESP32] platform init");

    smartdisplay_init();
    smartdisplay_lcd_set_backlight(1.0f);

    // Фиксируем стартовые значения памяти после инициализации.
    platform_log("[ESP32] heap after init: 8bit=%lu, psram=%lu\n",
                 heap_caps_get_free_size(MALLOC_CAP_8BIT),
                 heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

uint32_t platform_tick_ms(void) {
    return millis();
}

void platform_delay_ms(uint32_t ms) {
    delay(ms);
}

void platform_log(const char *fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Serial.print(buf);
}

void platform_log_heap(const char *tag) {
    // Диагностический снимок состояния кучи.
    const uint32_t free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    const uint32_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    const uint32_t largest_8bit = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    const uint32_t largest_psram = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    Serial.printf("[ESP32] heap %s: 8bit=%lu (largest=%lu), psram=%lu (largest=%lu)\n",
                  tag,
                  (unsigned long)free_8bit,
                  (unsigned long)largest_8bit,
                  (unsigned long)free_psram,
                  (unsigned long)largest_psram);
}
