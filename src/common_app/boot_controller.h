#pragma once

#include <stdint.h>

#include "common_app/frontend_config.h"

namespace demo {

class BootController {
public:
    enum class State : uint8_t {
        Idle = 0,
        WaitingBackend,
        Running,
        Fault
    };

    void begin(const FrontendConfig& config, bool configLoaded);
    void tick();

    State state() const { return _state; }

private:
    static constexpr uint32_t kWaitLogPeriodMs = 5000;

    void print_boot_banner() const;
    void print_config() const;
    void enter_fault(const char* reason);
    void enter_waiting_backend();
    void promote_backend_ready();
    void fallback_to_demo(const char* reason);

    FrontendConfig _config = frontend_default_config();
    bool _started = false;
    bool _configLoaded = false;
    bool _waitTimeoutReachedLogged = false;
    uint32_t _bootStartMs = 0;
    uint32_t _waitStartMs = 0;
    uint32_t _lastWaitLogMs = 0;
    State _state = State::Idle;
};

BootController& boot_controller();

} // namespace demo
