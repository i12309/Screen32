#include "platform_esp32/platform.h"
#include "common_app/frontend_config.h"
#include "common_app/frontend_platform.h"
#include "link/ITransport.h"
#include "link/WebSocketClientLink.h"
#include "log/ScreenLibLogger.h"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp32_smartdisplay.h>
#include <lvgl.h>
#include <cstdarg>
#include <cstdio>
#include <memory>
#include <cstring>

/*
 * Файл src/platform_esp32/platform.cpp
 * Назначение: реализация платформенного слоя ESP32 для ветки `src/`.
 * Файл отвечает за инициализацию железа и сервисные функции платформы.
 */

void platform_init(void) {
    // Запуск последовательного лога и инициализация экрана/тача.
    Serial.begin(115200);
    screenlib::log::Logger::init(screenlib::log::Level::Debug);
    SCREENLIB_LOGI("platform.esp32", "platform init");

    smartdisplay_init();
    smartdisplay_lcd_set_backlight(1.0f);

    // Фиксируем стартовые значения памяти после инициализации.
    SCREENLIB_LOGI("platform.esp32",
                   "heap after init: 8bit=%lu, psram=%lu",
                   static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_8BIT)),
                   static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
}

uint32_t platform_tick_ms(void) {
    return millis();
}

void platform_delay_ms(uint32_t ms) {
    delay(ms);
}

void platform_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    screenlib::log::Logger::vlog(screenlib::log::Level::Info, "platform.esp32", fmt, args);
    va_end(args);
}

void platform_log_heap(const char *tag) {
    // Диагностический снимок состояния кучи.
    const uint32_t free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    const uint32_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    const uint32_t largest_8bit = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    const uint32_t largest_psram = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    SCREENLIB_LOGI("platform.esp32",
                   "heap %s: 8bit=%lu (largest=%lu), psram=%lu (largest=%lu)",
                   tag,
                   static_cast<unsigned long>(free_8bit),
                   static_cast<unsigned long>(largest_8bit),
                   static_cast<unsigned long>(free_psram),
                   static_cast<unsigned long>(largest_psram));
}

namespace demo {

namespace {

class Esp32UartTransport : public ITransport {
public:
    bool begin(HardwareSerial* serial, uint32_t baud, int8_t rxPin, int8_t txPin) {
        if (serial == nullptr || baud == 0) {
            _serial = nullptr;
            _started = false;
            return false;
        }
        _serial = serial;
        _serial->begin(baud, SERIAL_8N1, rxPin, txPin);
        _started = true;
        return true;
    }

    bool connected() const override {
        return _started;
    }

    bool write(const uint8_t* data, size_t len) override {
        if (!_started || _serial == nullptr || data == nullptr || len == 0) {
            return false;
        }
        return _serial->write(data, len) == len;
    }

    size_t read(uint8_t* dst, size_t max_len) override {
        if (!_started || _serial == nullptr || dst == nullptr || max_len == 0) {
            return 0;
        }

        size_t n = 0;
        while (n < max_len && _serial->available() > 0) {
            const int ch = _serial->read();
            if (ch < 0) {
                break;
            }
            dst[n++] = static_cast<uint8_t>(ch);
        }
        return n;
    }

    void tick() override {}

private:
    HardwareSerial* _serial = nullptr;
    bool _started = false;
};

} // namespace

bool platform_load_frontend_config_json(char* outJson, size_t outSize) {
    if (outJson == nullptr || outSize == 0) {
        return false;
    }

    const char* json =
        "{\n"
        "  \"mode\": \"esp32\",\n"
        "  \"transport\": {\n"
        "    \"type\": \"uart\",\n"
        "    \"baud\": 115200,\n"
        "    \"rxPin\": 17,\n"
        "    \"txPin\": 18\n"
        "  },\n"
        "  \"offline_demo\": 1,\n"
        "  \"offline_timeout_ms\": 30000,\n"
        "  \"heartbeatPeriodMs\": 0,\n"
        "  \"firstOnlinePage\": 1,\n"
        "  \"firstOfflinePage\": 1\n"
        "}\n";

    strncpy(outJson, json, outSize - 1);
    outJson[outSize - 1] = '\0';
    return true;
}

std::unique_ptr<ITransport> platform_create_transport(const FrontendConfig& config) {
    if (config.transport.type == FrontendTransportType::None) {
        return nullptr;
    }

    if (config.transport.type == FrontendTransportType::Uart) {
        std::unique_ptr<Esp32UartTransport> transport(new Esp32UartTransport());
        if (!transport->begin(&Serial1,
                              config.transport.baud,
                              static_cast<int8_t>(config.transport.rxPin),
                              static_cast<int8_t>(config.transport.txPin))) {
            return nullptr;
        }
        return transport;
    }

    if (config.transport.type == FrontendTransportType::WsClient) {
        std::unique_ptr<WebSocketClientLink> transport(new WebSocketClientLink());
        if (!transport->begin(config.transport.url)) {
            return nullptr;
        }
        return transport;
    }

    return nullptr;
}

} // namespace demo
