#pragma once

#include <stddef.h>
#include <stdint.h>

#include "link/ITransport.h"

#ifdef ARDUINO

#include <HardwareSerial.h>

class UartClientLink : public ITransport {
public:
    struct Config {
        HardwareSerial* serial = nullptr;
        uint32_t baud = 115200;
        int8_t rxPin = -1;
        int8_t txPin = -1;
    };

    UartClientLink() = default;
    explicit UartClientLink(const Config& cfg) { begin(cfg); }

    bool begin(const Config& cfg);

    bool connected() const override { return _started; }
    bool write(const uint8_t* data, size_t len) override;
    size_t read(uint8_t* dst, size_t max_len) override;
    void tick() override {}

private:
    HardwareSerial* _serial = nullptr;
    bool _started = false;
};

#else

class UartClientLink : public ITransport {
public:
    struct Config {
        uint32_t baud = 115200;
        int8_t rxPin = -1;
        int8_t txPin = -1;
    };

    bool begin(const Config& cfg) {
        (void)cfg;
        return false;
    }

    bool connected() const override { return false; }
    bool write(const uint8_t* data, size_t len) override {
        (void)data;
        (void)len;
        return false;
    }
    size_t read(uint8_t* dst, size_t max_len) override {
        (void)dst;
        (void)max_len;
        return 0;
    }
    void tick() override {}
};

#endif

