#include "common_app/frontend_config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace demo {

namespace {

bool copy_string(char* dst, size_t dstSize, const char* src) {
    if (dst == nullptr || dstSize == 0 || src == nullptr) {
        return false;
    }
    strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
    return true;
}

const char* find_json_key_value(const char* json, const char* key) {
    if (json == nullptr || key == nullptr) {
        return nullptr;
    }

    char pattern[64] = {};
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* keyPos = strstr(json, pattern);
    if (keyPos == nullptr) {
        return nullptr;
    }

    const char* colon = strchr(keyPos + strlen(pattern), ':');
    if (colon == nullptr) {
        return nullptr;
    }

    return colon + 1;
}

const char* skip_ws(const char* p) {
    while (p != nullptr && *p != '\0' && isspace(static_cast<unsigned char>(*p))) {
        ++p;
    }
    return p;
}

bool parse_json_string_value(const char* p, char* out, size_t outSize) {
    if (p == nullptr || out == nullptr || outSize == 0) {
        return false;
    }

    p = skip_ws(p);
    if (p == nullptr || *p != '"') {
        return false;
    }
    ++p;

    size_t n = 0;
    while (*p != '\0' && *p != '"' && n + 1 < outSize) {
        out[n++] = *p++;
    }
    out[n] = '\0';
    return *p == '"';
}

bool parse_json_int_value(const char* p, int32_t& out) {
    if (p == nullptr) {
        return false;
    }
    p = skip_ws(p);
    if (p == nullptr || *p == '\0') {
        return false;
    }

    char* end = nullptr;
    const long value = strtol(p, &end, 10);
    if (end == p) {
        return false;
    }
    out = static_cast<int32_t>(value);
    return true;
}

bool parse_json_bool_like_value(const char* p, bool& out) {
    if (p == nullptr) {
        return false;
    }
    p = skip_ws(p);
    if (p == nullptr || *p == '\0') {
        return false;
    }

    if (strncmp(p, "true", 4) == 0) {
        out = true;
        return true;
    }
    if (strncmp(p, "false", 5) == 0) {
        out = false;
        return true;
    }

    int32_t asInt = 0;
    if (!parse_json_int_value(p, asInt)) {
        return false;
    }
    out = (asInt != 0);
    return true;
}

bool parse_mode(const char* value, FrontendMode& outMode) {
    if (value == nullptr) {
        return false;
    }
    if (strcmp(value, "esp32") == 0) {
        outMode = FrontendMode::Esp32;
        return true;
    }
    if (strcmp(value, "wasm") == 0 || strcmp(value, "web") == 0) {
        outMode = FrontendMode::Wasm;
        return true;
    }
    return false;
}

bool parse_transport_type(const char* value, FrontendTransportType& outType) {
    if (value == nullptr) {
        return false;
    }
    if (strcmp(value, "none") == 0 || strcmp(value, "null") == 0 || strcmp(value, "fake") == 0) {
        outType = FrontendTransportType::None;
        return true;
    }
    if (strcmp(value, "uart") == 0) {
        outType = FrontendTransportType::Uart;
        return true;
    }
    if (strcmp(value, "ws_client") == 0 || strcmp(value, "ws") == 0) {
        outType = FrontendTransportType::WsClient;
        return true;
    }
    return false;
}

} // namespace

FrontendConfig frontend_default_config() {
    FrontendConfig cfg{};
#ifdef ARDUINO
    cfg.mode = FrontendMode::Esp32;
    cfg.transport.type = FrontendTransportType::Uart;
#else
    cfg.mode = FrontendMode::Wasm;
    cfg.transport.type = FrontendTransportType::WsClient;
#endif
    cfg.offlineDemo = true;
    cfg.firstOnlinePage = 1;
    cfg.firstOfflinePage = 1;
    copy_string(cfg.transport.url, sizeof(cfg.transport.url), "ws://127.0.0.1:81");
    cfg.transport.baud = 115200;
    cfg.transport.rxPin = 16;
    cfg.transport.txPin = 17;
    return cfg;
}

bool frontend_parse_config_json(const char* json, FrontendConfig& outConfig) {
    if (json == nullptr || json[0] == '\0') {
        return false;
    }

    FrontendConfig cfg = frontend_default_config();

    const char* modePos = find_json_key_value(json, "mode");
    if (modePos != nullptr) {
        char modeText[16] = {};
        if (parse_json_string_value(modePos, modeText, sizeof(modeText))) {
            parse_mode(modeText, cfg.mode);
        }
    }

    const char* typePos = find_json_key_value(json, "type");
    if (typePos != nullptr) {
        char typeText[24] = {};
        if (parse_json_string_value(typePos, typeText, sizeof(typeText))) {
            parse_transport_type(typeText, cfg.transport.type);
        }
    }

    const char* urlPos = find_json_key_value(json, "url");
    if (urlPos != nullptr) {
        char urlText[128] = {};
        if (parse_json_string_value(urlPos, urlText, sizeof(urlText))) {
            copy_string(cfg.transport.url, sizeof(cfg.transport.url), urlText);
        }
    }

    const char* baudPos = find_json_key_value(json, "baud");
    if (baudPos != nullptr) {
        int32_t value = 0;
        if (parse_json_int_value(baudPos, value) && value > 0) {
            cfg.transport.baud = static_cast<uint32_t>(value);
        }
    }

    const char* rxPos = find_json_key_value(json, "rxPin");
    if (rxPos != nullptr) {
        int32_t value = 0;
        if (parse_json_int_value(rxPos, value)) {
            cfg.transport.rxPin = value;
        }
    }

    const char* txPos = find_json_key_value(json, "txPin");
    if (txPos != nullptr) {
        int32_t value = 0;
        if (parse_json_int_value(txPos, value)) {
            cfg.transport.txPin = value;
        }
    }

    const char* offlinePos = find_json_key_value(json, "offline_demo");
    if (offlinePos != nullptr) {
        bool value = false;
        if (parse_json_bool_like_value(offlinePos, value)) {
            cfg.offlineDemo = value;
        }
    }

    bool hasOnlinePage = false;
    bool hasOfflinePage = false;
    int32_t value = 0;

    const char* onlinePos = find_json_key_value(json, "firstOnlinePage");
    if (onlinePos == nullptr) {
        onlinePos = find_json_key_value(json, "first_online_page");
    }
    if (onlinePos != nullptr && parse_json_int_value(onlinePos, value) && value > 0) {
        cfg.firstOnlinePage = static_cast<uint32_t>(value);
        hasOnlinePage = true;
    }

    const char* offlinePagePos = find_json_key_value(json, "firstOfflinePage");
    if (offlinePagePos == nullptr) {
        offlinePagePos = find_json_key_value(json, "first_offline_page");
    }
    if (offlinePagePos != nullptr && parse_json_int_value(offlinePagePos, value) && value > 0) {
        cfg.firstOfflinePage = static_cast<uint32_t>(value);
        hasOfflinePage = true;
    }

    // Обратная совместимость: единый start_page обновляет обе mode-страницы
    // только если явно не заданы firstOnlinePage/firstOfflinePage.
    const char* startPos = find_json_key_value(json, "start_page");
    if (startPos != nullptr && parse_json_int_value(startPos, value) && value > 0) {
        if (!hasOnlinePage) {
            cfg.firstOnlinePage = static_cast<uint32_t>(value);
        }
        if (!hasOfflinePage) {
            cfg.firstOfflinePage = static_cast<uint32_t>(value);
        }
    }

    outConfig = cfg;
    return true;
}

bool frontend_load_config(FrontendConfig& outConfig) {
    char json[1024] = {};
    if (!platform_load_frontend_config_json(json, sizeof(json))) {
        outConfig = frontend_default_config();
        return false;
    }

    if (!frontend_parse_config_json(json, outConfig)) {
        outConfig = frontend_default_config();
        return false;
    }

    return true;
}

const char* frontend_mode_name(FrontendMode mode) {
    return mode == FrontendMode::Esp32 ? "esp32" : "wasm";
}

const char* frontend_transport_name(FrontendTransportType type) {
    switch (type) {
        case FrontendTransportType::None:
            return "none";
        case FrontendTransportType::Uart:
            return "uart";
        case FrontendTransportType::WsClient:
            return "ws_client";
        default:
            return "unknown";
    }
}

} // namespace demo
