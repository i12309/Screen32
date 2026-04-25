#pragma once

#include <stddef.h>
#include <stdint.h>

namespace demo {

// Режим frontend определяет профиль устройства в payload для service/device_info.
enum class FrontendMode : uint8_t {
    Esp32 = 0,
    Wasm
};

// Тип transport для online-режима runtime.
enum class FrontendTransportType : uint8_t {
    None = 0,
    Uart,
    WsClient
};

struct FrontendTransportConfig {
    FrontendTransportType type = FrontendTransportType::None;
    char url[128] = "ws://127.0.0.1:81";
    uint32_t baud = 115200;
    int32_t rxPin = 44;
    int32_t txPin = 43;
};

struct FrontendConfig {
    FrontendMode mode = FrontendMode::Wasm;
    FrontendTransportConfig transport{};
    bool offlineDemo = true;
    // Первая страница для показа в online-режиме до команд backend.
    uint32_t firstOnlinePage = 1;
    // Первая страница для показа в offline demo-режиме.
    uint32_t firstOfflinePage = 1;
    // Через сколько миллисекунд ожидания backend пытаться перейти в offline demo.
    // JSON-параметр: offline_timeout_ms.
    uint32_t offlineTimeoutMs = 30000;
    // Heartbeat period in online mode (ms). 0 disables heartbeat sending.
    uint32_t heartbeatPeriodMs = 1000;
    // Логи входящих и исходящих protocol Envelope. JSON: log_traffic.
    bool logTraffic = true;
};

// Возвращает конфиг frontend по умолчанию для текущей платформы.
FrontendConfig frontend_default_config();
// Парсит JSON-текст конфига в FrontendConfig.
bool frontend_parse_config_json(const char* json, FrontendConfig& outConfig);
// Загружает JSON-конфиг через платформенный хук и парсит его.
bool frontend_load_config(FrontendConfig& outConfig);

// Возвращает текстовое имя режима для метаданных.
const char* frontend_mode_name(FrontendMode mode);
// Возвращает текстовое имя transport-типа для логов и метаданных.
const char* frontend_transport_name(FrontendTransportType type);

// Платформенный хук: платформенный слой должен вернуть JSON-текст frontend-конфига.
bool platform_load_frontend_config_json(char* outJson, size_t outSize);

} // namespace demo
