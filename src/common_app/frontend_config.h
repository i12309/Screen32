#pragma once

#include <stddef.h>
#include <stdint.h>

namespace demo {

enum class FrontendMode : uint8_t {
    Esp32 = 0,
    Wasm
};

enum class FrontendTransportType : uint8_t {
    None = 0,
    Uart,
    WsClient
};

struct FrontendTransportConfig {
    FrontendTransportType type = FrontendTransportType::None;
    char url[128] = "ws://127.0.0.1:81";
    uint32_t baud = 115200;
    int32_t rxPin = 16;
    int32_t txPin = 17;
};

struct FrontendConfig {
    FrontendMode mode = FrontendMode::Wasm;
    FrontendTransportConfig transport{};
    bool offlineDemo = true;
    uint32_t startPage = 2;
};

FrontendConfig frontend_default_config();
bool frontend_parse_config_json(const char* json, FrontendConfig& outConfig);
bool frontend_load_config(FrontendConfig& outConfig);

const char* frontend_mode_name(FrontendMode mode);
const char* frontend_transport_name(FrontendTransportType type);

// Platform hook: platform layer must return JSON text for frontend config.
bool platform_load_frontend_config_json(char* outJson, size_t outSize);

} // namespace demo

