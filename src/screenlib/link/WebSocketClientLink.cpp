#include "WebSocketClientLink.h"

#if defined(ARDUINO)

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

bool WebSocketClientLink::begin(const char* url) {
    char host[96] = {};
    char path[96] = {};
    uint16_t port = 0;

    if (!parseUrl(url, host, sizeof(host), port, path, sizeof(path))) {
        return false;
    }
    return begin(host, port, path);
}

bool WebSocketClientLink::begin(const char* host, uint16_t port, const char* path) {
    if (host == nullptr || host[0] == '\0' || port == 0) {
        return false;
    }
    if (path == nullptr || path[0] == '\0') {
        path = "/";
    }

    _connected = false;
    rxClear();

    _ws.begin(host, port, path);
    _ws.setReconnectInterval(_cfg.reconnectIntervalMs);
    _ws.onEvent([this](WStype_t type, uint8_t* payload, size_t length) {
        this->onEvent(type, payload, length);
    });

    return true;
}

void WebSocketClientLink::onEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            _connected = true;
            rxClear();
            break;

        case WStype_DISCONNECTED:
            _connected = false;
            rxClear();
            break;

        case WStype_BIN:
            if (!_connected || payload == nullptr || length == 0) {
                break;
            }
            for (size_t i = 0; i < length; ++i) {
                rxPush(payload[i]);
            }
            break;

        default:
            break;
    }
}

bool WebSocketClientLink::parseUrl(const char* url,
                                   char* hostOut,
                                   size_t hostOutSize,
                                   uint16_t& portOut,
                                   char* pathOut,
                                   size_t pathOutSize) {
    if (url == nullptr || hostOut == nullptr || pathOut == nullptr ||
        hostOutSize == 0 || pathOutSize == 0) {
        return false;
    }

    hostOut[0] = '\0';
    pathOut[0] = '\0';
    portOut = 0;

    const char* p = url;
    if (strncmp(p, "ws://", 5) == 0) {
        p += 5;
    } else if (strncmp(p, "wss://", 6) == 0) {
        // В текущей версии поддерживаем только ws://.
        return false;
    }

    const char* slash = strchr(p, '/');
    const char* hostPortEnd = (slash != nullptr) ? slash : (p + strlen(p));
    if (hostPortEnd <= p) {
        return false;
    }

    const char* colon = nullptr;
    for (const char* it = p; it < hostPortEnd; ++it) {
        if (*it == ':') {
            colon = it;
            break;
        }
    }
    if (colon == nullptr) {
        return false;
    }

    const size_t hostLen = static_cast<size_t>(colon - p);
    if (hostLen == 0 || hostLen >= hostOutSize) {
        return false;
    }
    memcpy(hostOut, p, hostLen);
    hostOut[hostLen] = '\0';

    const char* portStart = colon + 1;
    if (portStart >= hostPortEnd) {
        return false;
    }

    char portText[8] = {};
    const size_t portLen = static_cast<size_t>(hostPortEnd - portStart);
    if (portLen == 0 || portLen >= sizeof(portText)) {
        return false;
    }
    memcpy(portText, portStart, portLen);
    portText[portLen] = '\0';
    for (size_t i = 0; i < portLen; ++i) {
        if (!isdigit(static_cast<unsigned char>(portText[i]))) {
            return false;
        }
    }

    const long parsedPort = strtol(portText, nullptr, 10);
    if (parsedPort <= 0 || parsedPort > 65535) {
        return false;
    }
    portOut = static_cast<uint16_t>(parsedPort);

    if (slash != nullptr) {
        strncpy(pathOut, slash, pathOutSize - 1);
        pathOut[pathOutSize - 1] = '\0';
    } else {
        strncpy(pathOut, "/", pathOutSize - 1);
        pathOut[pathOutSize - 1] = '\0';
    }

    return true;
}

#endif

#if defined(__EMSCRIPTEN__)

#include <emscripten/emscripten.h>

#include <stdio.h>
#include <string.h>

bool WebSocketClientLink::begin(const char* url) {
    if (url == nullptr || url[0] == '\0') {
        return false;
    }

    strncpy(_url, url, sizeof(_url) - 1);
    _url[sizeof(_url) - 1] = '\0';
    _started = true;
    return openSocket();
}

bool WebSocketClientLink::begin(const char* host, uint16_t port, const char* path) {
    if (host == nullptr || host[0] == '\0' || port == 0) {
        return false;
    }

    if (path == nullptr || path[0] == '\0') {
        path = "/";
    }

    char url[160] = {};
    snprintf(url, sizeof(url), "ws://%s:%u%s", host, static_cast<unsigned>(port), path);
    return begin(url);
}

bool WebSocketClientLink::write(const uint8_t* data, size_t len) {
    if (!_connected || _socket <= 0 || data == nullptr || len == 0) {
        return false;
    }
    const EMSCRIPTEN_RESULT rc = emscripten_websocket_send_binary(_socket, data, len);
    return rc == EMSCRIPTEN_RESULT_SUCCESS;
}

size_t WebSocketClientLink::read(uint8_t* dst, size_t max_len) {
    if (dst == nullptr || max_len == 0) {
        return 0;
    }

    size_t n = 0;
    while (n < max_len && !rxEmpty()) {
        dst[n++] = rxPop();
    }
    return n;
}

void WebSocketClientLink::tick() {
    if (!_started) {
        return;
    }

    if (_connected) {
        return;
    }

    const double now = emscripten_get_now();
    if (_lastConnectAttemptMs == 0.0 ||
        (now - _lastConnectAttemptMs) >= static_cast<double>(_cfg.reconnectIntervalMs)) {
        openSocket();
    }
}

bool WebSocketClientLink::openSocket() {
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

    emscripten_websocket_set_onopen_callback(_socket, this, &WebSocketClientLink::onOpen);
    emscripten_websocket_set_onclose_callback(_socket, this, &WebSocketClientLink::onClose);
    emscripten_websocket_set_onerror_callback(_socket, this, &WebSocketClientLink::onError);
    emscripten_websocket_set_onmessage_callback(_socket, this, &WebSocketClientLink::onMessage);
    return true;
}

void WebSocketClientLink::closeSocket() {
    if (_socket <= 0) {
        return;
    }
    emscripten_websocket_close(_socket, 1000, "");
    emscripten_websocket_delete(_socket);
    _socket = 0;
}

EM_BOOL WebSocketClientLink::onOpen(int eventType, const EmscriptenWebSocketOpenEvent* e, void* userData) {
    (void)eventType;
    (void)e;
    WebSocketClientLink* self = static_cast<WebSocketClientLink*>(userData);
    if (self == nullptr) {
        return EM_FALSE;
    }
    self->_connected = true;
    self->rxClear();
    return EM_TRUE;
}

EM_BOOL WebSocketClientLink::onClose(int eventType, const EmscriptenWebSocketCloseEvent* e, void* userData) {
    (void)eventType;
    (void)e;
    WebSocketClientLink* self = static_cast<WebSocketClientLink*>(userData);
    if (self == nullptr) {
        return EM_FALSE;
    }
    self->_connected = false;
    self->_socket = 0;
    self->rxClear();
    return EM_TRUE;
}

EM_BOOL WebSocketClientLink::onError(int eventType, const EmscriptenWebSocketErrorEvent* e, void* userData) {
    (void)eventType;
    (void)e;
    WebSocketClientLink* self = static_cast<WebSocketClientLink*>(userData);
    if (self == nullptr) {
        return EM_FALSE;
    }
    self->_connected = false;
    return EM_TRUE;
}

EM_BOOL WebSocketClientLink::onMessage(int eventType, const EmscriptenWebSocketMessageEvent* e, void* userData) {
    (void)eventType;
    WebSocketClientLink* self = static_cast<WebSocketClientLink*>(userData);
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

#endif
