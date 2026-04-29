#include "platform_esp32/platform.h"
#include "common_app/frontend_config.h"
#include "common_app/frontend_platform.h"
#include "link/ITransport.h"
#include "link/WebSocketClientLink.h"
#include "log/ScreenLibLogger.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_err.h>
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

namespace {

void on_failed_alloc(size_t size, uint32_t caps, const char* function_name) {
    SCREENLIB_LOGE("heap",
                   "alloc failed: size=%lu caps=0x%lx function=%s",
                   static_cast<unsigned long>(size),
                   static_cast<unsigned long>(caps),
                   function_name != nullptr ? function_name : "?");
}

} // namespace

void platform_init(void) {
    // Запуск последовательного лога и инициализация экрана/тача.
    Serial.begin(115200);
    screenlib::log::Logger::init(screenlib::log::Level::Debug);
    SCREENLIB_LOGI("platform.esp32", "platform init");
    heap_caps_register_failed_alloc_callback(on_failed_alloc);
    SCREENLIB_LOGI("platform.esp32",
                   "psram found=%d size=%lu free=%lu",
                   psramFound() ? 1 : 0,
                   static_cast<unsigned long>(ESP.getPsramSize()),
                   static_cast<unsigned long>(ESP.getFreePsram()));

    platform_log_heap("platform_init: before smartdisplay_init");
    smartdisplay_init();
    platform_log_heap("platform_init: after smartdisplay_init");
    smartdisplay_lcd_set_backlight(1.0f);
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
    const uint32_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    const uint32_t largest_internal = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    const uint32_t free_dma = heap_caps_get_free_size(MALLOC_CAP_DMA);
    const uint32_t largest_dma = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);
    const uint32_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    const uint32_t largest_psram = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    SCREENLIB_LOGI("platform.esp32",
                   "heap %s: INTERNAL=%lu largest=%lu, DMA=%lu largest=%lu, PSRAM=%lu largest=%lu",
                   tag,
                   static_cast<unsigned long>(free_internal),
                   static_cast<unsigned long>(largest_internal),
                   static_cast<unsigned long>(free_dma),
                   static_cast<unsigned long>(largest_dma),
                   static_cast<unsigned long>(free_psram),
                   static_cast<unsigned long>(largest_psram));
}

namespace demo {

namespace {

// UART-транспорт через IDF API напрямую.
// Используем UART_NUM_2; UART_NUM_0 занят USB-CDC (логи).
// HardwareSerial обходим — у используемой версии Arduino-esp32 баг
// в обработчике ошибок uartBegin (NPE при отказе uartSetPins/driver_install),
// и нет способа получить код ошибки.
class Esp32UartTransport : public ITransport {
public:
    static constexpr uart_port_t kPort = UART_NUM_2;
    static constexpr size_t kRxBufferSize = 1024;
    static constexpr size_t kTxBufferSize = 0;

    bool begin(uint32_t baud, int8_t rxPin, int8_t txPin) {
        if (baud == 0) {
            return false;
        }
        if (!GPIO_IS_VALID_GPIO(rxPin) || !GPIO_IS_VALID_OUTPUT_GPIO(txPin)) {
            SCREENLIB_LOGE("platform.esp32",
                           "uart pins invalid: rx=%d tx=%d",
                           static_cast<int>(rxPin),
                           static_cast<int>(txPin));
            return false;
        }
        if (uart_is_driver_installed(kPort)) {
            uart_driver_delete(kPort);
        }

        uart_config_t cfg = {};
        cfg.baud_rate = static_cast<int>(baud);
        cfg.data_bits = UART_DATA_8_BITS;
        cfg.parity = UART_PARITY_DISABLE;
        cfg.stop_bits = UART_STOP_BITS_1;
        cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
        cfg.source_clk = UART_SCLK_APB;

        esp_err_t err = uart_param_config(kPort, &cfg);
        if (err != ESP_OK) {
            SCREENLIB_LOGE("platform.esp32", "uart_param_config -> %s",
                           esp_err_to_name(err));
            return false;
        }
        err = uart_set_pin(kPort, txPin, rxPin,
                           UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        if (err != ESP_OK) {
            SCREENLIB_LOGE("platform.esp32", "uart_set_pin -> %s",
                           esp_err_to_name(err));
            return false;
        }
        platform_log_heap("platform_create_transport: before uart_driver_install");
        err = uart_driver_install(kPort,
                                  static_cast<int>(kRxBufferSize),
                                  static_cast<int>(kTxBufferSize),
                                  0, nullptr, 0);
        platform_log_heap("platform_create_transport: after uart_driver_install");
        if (err != ESP_OK) {
            SCREENLIB_LOGE("platform.esp32", "uart_driver_install -> %s",
                           esp_err_to_name(err));
            return false;
        }

        _started = true;
        SCREENLIB_LOGI("platform.esp32",
                       "uart begin: baud=%lu rx=%d tx=%d",
                       static_cast<unsigned long>(baud),
                       static_cast<int>(rxPin),
                       static_cast<int>(txPin));
        return true;
    }

    bool connected() const override { return _started; }

    bool write(const uint8_t* data, size_t len) override {
        if (!_started || data == nullptr || len == 0) {
            return false;
        }
        const int n = uart_write_bytes(kPort,
                                       reinterpret_cast<const char*>(data),
                                       len);
        return n == static_cast<int>(len);
    }

    size_t read(uint8_t* dst, size_t max_len) override {
        if (!_started || dst == nullptr || max_len == 0) {
            return 0;
        }
        const int n = uart_read_bytes(kPort, dst, max_len, 0);
        return n > 0 ? static_cast<size_t>(n) : 0;
    }

    void tick() override {}

private:
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
        "  \"offline_demo\": 0,\n"
        "  \"offline_timeout_ms\": 30000,\n"
        "  \"heartbeatPeriodMs\": 0,\n"
        "  \"log_traffic\": 1,\n"
        "  \"firstOnlinePage\": 1,\n"
        "  \"firstOfflinePage\": 0\n"
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
        if (!transport->begin(config.transport.baud,
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
