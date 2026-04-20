#include "platform_web/platform.h"
#include "common_app/frontend_config.h"
#include "common_app/frontend_platform.h"
#include "link/ITransport.h"
#include "log/ScreenLibLogger.h"

#include <lvgl.h>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <cstring>

#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>

#include "src/drivers/sdl/lv_sdl_keyboard.h"
#include "src/drivers/sdl/lv_sdl_mouse.h"
#include "src/drivers/sdl/lv_sdl_mousewheel.h"
#include "src/drivers/sdl/lv_sdl_window.h"

namespace {

/*
 * Файл src/platform_web/platform.cpp
 * Назначение: реализация web-платформенного слоя для Emscripten-сборки.
 * Файл поднимает LVGL + SDL-окно 800x480 и создает устройства ввода,
 * а также предоставляет таймер/логирование для общего app-слоя.
 */

bool g_platform_initialized = false;
lv_display_t *g_display = nullptr;

class EmscriptenWsClientTransport : public ITransport {
public:
    struct Config {
        uint32_t reconnectIntervalMs = 2000;
    };

    EmscriptenWsClientTransport() = default;
    explicit EmscriptenWsClientTransport(const Config& cfg) : _cfg(cfg) {}

    bool begin(const char* url) {
        if (url == nullptr || url[0] == '\0') {
            return false;
        }
        strncpy(_url, url, sizeof(_url) - 1);
        _url[sizeof(_url) - 1] = '\0';
        _started = true;
        return openSocket();
    }

    bool connected() const override {
        return _connected;
    }

    bool write(const uint8_t* data, size_t len) override {
        if (!_connected || _socket <= 0 || data == nullptr || len == 0) {
            return false;
        }
        return emscripten_websocket_send_binary(
                   _socket,
                   const_cast<void*>(static_cast<const void*>(data)),
                   static_cast<uint32_t>(len)) == EMSCRIPTEN_RESULT_SUCCESS;
    }

    size_t read(uint8_t* dst, size_t max_len) override {
        if (dst == nullptr || max_len == 0) {
            return 0;
        }

        size_t n = 0;
        while (n < max_len && !rxEmpty()) {
            dst[n++] = rxPop();
        }
        return n;
    }

    void tick() override {
        if (!_started || _connected) {
            return;
        }

        const double now = emscripten_get_now();
        if (_lastConnectAttemptMs == 0.0 ||
            (now - _lastConnectAttemptMs) >= static_cast<double>(_cfg.reconnectIntervalMs)) {
            openSocket();
        }
    }

private:
    static constexpr size_t kRxBufSize = 1024;

    Config _cfg{};
    bool _connected = false;
    bool _started = false;
    EMSCRIPTEN_WEBSOCKET_T _socket = 0;
    char _url[160] = {};
    double _lastConnectAttemptMs = 0.0;

    uint8_t _rxBuf[kRxBufSize] = {};
    size_t _rxHead = 0;
    size_t _rxTail = 0;

    bool rxFull() const {
        return ((_rxHead + 1) & (kRxBufSize - 1)) == _rxTail;
    }

    bool rxEmpty() const {
        return _rxHead == _rxTail;
    }

    void rxPush(uint8_t b) {
        if (rxFull()) {
            return;
        }
        _rxBuf[_rxHead] = b;
        _rxHead = (_rxHead + 1) & (kRxBufSize - 1);
    }

    uint8_t rxPop() {
        const uint8_t b = _rxBuf[_rxTail];
        _rxTail = (_rxTail + 1) & (kRxBufSize - 1);
        return b;
    }

    void rxClear() {
        _rxHead = 0;
        _rxTail = 0;
    }

    bool openSocket() {
        if (!emscripten_websocket_is_supported()) {
            _connected = false;
            return false;
        }

        closeSocket();
        rxClear();
        _connected = false;
        _lastConnectAttemptMs = emscripten_get_now();

        EmscriptenWebSocketCreateAttributes attrs = {};
        attrs.url = _url;
        attrs.protocols = nullptr;
        attrs.createOnMainThread = EM_TRUE;

        _socket = emscripten_websocket_new(&attrs);
        if (_socket <= 0) {
            _socket = 0;
            return false;
        }

        emscripten_websocket_set_onopen_callback(_socket, this, &EmscriptenWsClientTransport::onOpen);
        emscripten_websocket_set_onclose_callback(_socket, this, &EmscriptenWsClientTransport::onClose);
        emscripten_websocket_set_onerror_callback(_socket, this, &EmscriptenWsClientTransport::onError);
        emscripten_websocket_set_onmessage_callback(_socket, this, &EmscriptenWsClientTransport::onMessage);
        return true;
    }

    void closeSocket() {
        if (_socket <= 0) {
            return;
        }
        emscripten_websocket_close(_socket, 1000, "");
        emscripten_websocket_delete(_socket);
        _socket = 0;
    }

    static EM_BOOL onOpen(int eventType, const EmscriptenWebSocketOpenEvent* e, void* userData) {
        (void)eventType;
        (void)e;
        auto* self = static_cast<EmscriptenWsClientTransport*>(userData);
        if (self == nullptr) {
            return EM_FALSE;
        }
        self->_connected = true;
        self->rxClear();
        return EM_TRUE;
    }

    static EM_BOOL onClose(int eventType, const EmscriptenWebSocketCloseEvent* e, void* userData) {
        (void)eventType;
        (void)e;
        auto* self = static_cast<EmscriptenWsClientTransport*>(userData);
        if (self == nullptr) {
            return EM_FALSE;
        }
        self->_connected = false;
        self->_socket = 0;
        self->rxClear();
        return EM_TRUE;
    }

    static EM_BOOL onError(int eventType, const EmscriptenWebSocketErrorEvent* e, void* userData) {
        (void)eventType;
        (void)e;
        auto* self = static_cast<EmscriptenWsClientTransport*>(userData);
        if (self == nullptr) {
            return EM_FALSE;
        }
        self->_connected = false;
        return EM_TRUE;
    }

    static EM_BOOL onMessage(int eventType, const EmscriptenWebSocketMessageEvent* e, void* userData) {
        (void)eventType;
        auto* self = static_cast<EmscriptenWsClientTransport*>(userData);
        if (self == nullptr || e == nullptr || e->data == nullptr || e->numBytes == 0) {
            return EM_FALSE;
        }
        if (e->isText) {
            return EM_TRUE;
        }

        const uint8_t* bytes = static_cast<const uint8_t*>(e->data);
        for (uint32_t i = 0; i < e->numBytes; ++i) {
            self->rxPush(bytes[i]);
        }
        return EM_TRUE;
    }
};

} // namespace

void platform_init(void) {
    if (g_platform_initialized) {
        return;
    }

    screenlib::log::Logger::init(screenlib::log::Level::Debug);

    lv_init();

    g_display = lv_sdl_window_create(800, 480);
    if (g_display != nullptr) {
        lv_sdl_window_set_title(g_display, "LVGL Simulator");
    }

    lv_sdl_mouse_create();
    lv_sdl_keyboard_create();
#if LV_SDL_MOUSEWHEEL_MODE == LV_SDL_MOUSEWHEEL_MODE_ENCODER
    lv_sdl_mousewheel_create();
#endif

    g_platform_initialized = true;
}

uint32_t platform_tick_ms(void) {
    return (uint32_t)emscripten_get_now();
}

void platform_delay_ms(uint32_t ms) {
    LV_UNUSED(ms);
}

void platform_log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    screenlib::log::Logger::vlog(screenlib::log::Level::Info, "platform.web", fmt, args);
    va_end(args);
}

namespace demo {

namespace {

bool read_json_file(const char* path, char* outJson, size_t outSize) {
    if (path == nullptr || outJson == nullptr || outSize == 0) {
        return false;
    }

    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }

    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (text.empty()) {
        return false;
    }

    strncpy(outJson, text.c_str(), outSize - 1);
    outJson[outSize - 1] = '\0';
    return true;
}

} // namespace

bool platform_load_frontend_config_json(char* outJson, size_t outSize) {
    if (outJson == nullptr || outSize == 0) {
        return false;
    }

    if (read_json_file("/frontend_config.json", outJson, outSize)) {
        return true;
    }
    if (read_json_file("frontend_config.json", outJson, outSize)) {
        return true;
    }
    if (read_json_file("../frontend_config.json", outJson, outSize)) {
        return true;
    }

    const char* fallback =
        "{\n"
        "  \"mode\": \"wasm\",\n"
        "  \"transport\": {\n"
        "    \"type\": \"ws_client\",\n"
        "    \"url\": \"ws://127.0.0.1:81\"\n"
        "  },\n"
        "  \"offline_demo\": 1,\n"
        "  \"firstOnlinePage\": 1,\n"
        "  \"firstOfflinePage\": 1\n"
        "}\n";

    strncpy(outJson, fallback, outSize - 1);
    outJson[outSize - 1] = '\0';
    return true;
}

std::unique_ptr<ITransport> platform_create_transport(const FrontendConfig& config) {
    if (config.transport.type == FrontendTransportType::None || config.offlineDemo) {
        return nullptr;
    }

    if (config.transport.type == FrontendTransportType::WsClient) {
        auto transport = std::make_unique<EmscriptenWsClientTransport>();
        if (!transport->begin(config.transport.url)) {
            return nullptr;
        }
        return transport;
    }

    return nullptr;
}

} // namespace demo
