#include "UartClientLink.h"

#ifdef ARDUINO

bool UartClientLink::begin(const Config& cfg) {
    if (cfg.serial == nullptr || cfg.baud == 0) {
        _started = false;
        _serial = nullptr;
        return false;
    }

    _serial = cfg.serial;
    _serial->begin(cfg.baud, SERIAL_8N1, cfg.rxPin, cfg.txPin);
    _started = true;
    return true;
}

bool UartClientLink::write(const uint8_t* data, size_t len) {
    if (!_started || _serial == nullptr || data == nullptr || len == 0) {
        return false;
    }
    return _serial->write(data, len) == len;
}

size_t UartClientLink::read(uint8_t* dst, size_t max_len) {
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

#endif

