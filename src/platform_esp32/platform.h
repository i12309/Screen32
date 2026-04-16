#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Файл src/platform_esp32/platform.h
 * Назначение: ESP32-платформенный API для ветки исходников в `src/`.
 * Интерфейс используется общим кодом для времени, логов и инициализации.
 */

// Инициализация ESP32-платформы: Serial, smartdisplay, подсветка и диагностика.
void platform_init(void);

// Возвращает время с момента запуска в миллисекундах.
uint32_t platform_tick_ms(void);

// Добавляет небольшую задержку в основном цикле.
void platform_delay_ms(uint32_t ms);

// Пишет форматированный лог в UART.
void platform_log(const char *fmt, ...);

// Выводит статистику кучи (в т.ч. PSRAM).
void platform_log_heap(const char *tag);

#ifdef __cplusplus
}
#endif
