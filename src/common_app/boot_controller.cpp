#include "common_app/boot_controller.h"

#include "common_app/app_core.h"
#include "common_app/frontend_runtime.h"
#include "log/ScreenLibLogger.h"

#if defined(ARDUINO)
#include <Arduino.h>
#include <esp_system.h>
#endif

namespace demo {

namespace {

constexpr const char* kLogTag = "boot";

const char* state_name(BootController::State state) {
    switch (state) {
        case BootController::State::Idle:
            return "idle";
        case BootController::State::WaitingBackend:
            return "waiting_backend";
        case BootController::State::Running:
            return "running";
        case BootController::State::Fault:
            return "fault";
        default:
            return "unknown";
    }
}

} // namespace

BootController& boot_controller() {
    static BootController controller;
    return controller;
}

void BootController::begin(const FrontendConfig& config, bool configLoaded) {
    _config = config;
    _configLoaded = configLoaded;
    _started = true;
    _state = State::Idle;
    _bootStartMs = platform_tick_ms();
    _waitStartMs = 0;
    _lastWaitLogMs = _bootStartMs;

    print_boot_banner();
    print_config();

    if (_config.mode != FrontendMode::Esp32) {
        const bool ready = frontend_runtime_init(_config);
        if (!ready) {
            enter_fault("runtime init failed");
            return;
        }
        _state = State::Running;
        SCREENLIB_LOGI(kLogTag, "boot complete (mode=%s)", state_name(_state));
        return;
    }

    if (_config.transport.type == FrontendTransportType::None) {
        enter_fault("transport=none in online boot path");
        return;
    }

    const bool onlineReady = frontend_runtime_init_online(_config);
    if (!onlineReady) {
        enter_fault("online init failed");
        return;
    }

    enter_waiting_backend();
}

void BootController::tick() {
    if (!_started) {
        return;
    }

    switch (_state) {
        case State::WaitingBackend: {
            frontend_runtime_tick();

            if (frontend_runtime_backend_connected()) {
                promote_backend_ready();
                return;
            }

            const uint32_t now = platform_tick_ms();
            if ((now - _lastWaitLogMs) >= kWaitLogPeriodMs) {
                const uint32_t elapsedSec = (now - _waitStartMs) / 1000U;
                SCREENLIB_LOGI(kLogTag,
                               "waiting backend... %lus elapsed",
                               static_cast<unsigned long>(elapsedSec));
                _lastWaitLogMs = now;
            }

            if (_config.offlineTimeoutMs == 0) {
                // Timeout disabled: keep waiting backend forever.
                return;
            }

            if ((now - _waitStartMs) < _config.offlineTimeoutMs) {
                return;
            }

            if (!fallback_to_demo("backend wait timeout reached")) {
                // Demo disabled or failed to start: keep waiting in the next timeout window.
                _waitStartMs = now;
                _lastWaitLogMs = now;
                SCREENLIB_LOGI(kLogTag,
                               "backend still unavailable; continue waiting (next timeout=%lus)",
                               static_cast<unsigned long>(_config.offlineTimeoutMs / 1000U));
            }
            return;
        }
        case State::Running:
            frontend_runtime_tick();
            return;
        case State::Idle:
        case State::Fault:
        default:
            return;
    }
}

void BootController::print_boot_banner() const {
    SCREENLIB_LOGI(kLogTag, "=== Screen32 boot start ===");
    SCREENLIB_LOGI(kLogTag, "config source: %s", _configLoaded ? "platform json" : "defaults");

#if defined(ARDUINO)
    SCREENLIB_LOGI(kLogTag,
                   "chip=%s rev=%d cores=%d cpu=%luMHz",
                   ESP.getChipModel(),
                   ESP.getChipRevision(),
                   ESP.getChipCores(),
                   static_cast<unsigned long>(ESP.getCpuFreqMHz()));
    SCREENLIB_LOGI(kLogTag,
                   "flash=%lu bytes psram=%s free_heap=%lu",
                   static_cast<unsigned long>(ESP.getFlashChipSize()),
                   psramFound() ? "yes" : "no",
                   static_cast<unsigned long>(ESP.getFreeHeap()));
    SCREENLIB_LOGI(kLogTag, "reset_reason=%d", static_cast<int>(esp_reset_reason()));
#endif
}

void BootController::print_config() const {
    SCREENLIB_LOGI(kLogTag,
                   "frontend mode=%s transport=%s first_online_page=%lu first_offline_page=%lu",
                   frontend_mode_name(_config.mode),
                   frontend_transport_name(_config.transport.type),
                   static_cast<unsigned long>(_config.firstOnlinePage),
                   static_cast<unsigned long>(_config.firstOfflinePage));

    if (_config.transport.type == FrontendTransportType::Uart) {
        SCREENLIB_LOGI(kLogTag,
                       "uart baud=%lu rx=%ld tx=%ld",
                       static_cast<unsigned long>(_config.transport.baud),
                       static_cast<long>(_config.transport.rxPin),
                       static_cast<long>(_config.transport.txPin));
    } else if (_config.transport.type == FrontendTransportType::WsClient) {
        SCREENLIB_LOGI(kLogTag, "ws url=%s", _config.transport.url);
    }
}

void BootController::enter_fault(const char* reason) {
    _state = State::Fault;
    SCREENLIB_LOGE(kLogTag, "boot fault: %s", reason != nullptr ? reason : "unknown");
}

void BootController::enter_waiting_backend() {
    _state = State::WaitingBackend;
    _waitStartMs = platform_tick_ms();
    _lastWaitLogMs = _waitStartMs;
    if (_config.offlineTimeoutMs == 0) {
        SCREENLIB_LOGI(kLogTag, "waiting backend connection (timeout=disabled)");
    } else {
        SCREENLIB_LOGI(kLogTag,
                       "waiting backend connection (timeout=%lus)",
                       static_cast<unsigned long>(_config.offlineTimeoutMs / 1000U));
    }
}

void BootController::promote_backend_ready() {
    _state = State::Running;
    const uint32_t now = platform_tick_ms();
    const uint32_t elapsedMs = now - _waitStartMs;
    SCREENLIB_LOGI(kLogTag,
                   "backend connected after %lu ms; continue runtime",
                   static_cast<unsigned long>(elapsedMs));
}

bool BootController::fallback_to_demo(const char* reason) {
    SCREENLIB_LOGW(kLogTag, "fallback to demo: %s", reason != nullptr ? reason : "timeout");
    if (!frontend_runtime_switch_to_offline_demo()) {
        SCREENLIB_LOGW(kLogTag, "fallback to demo failed; keep waiting backend");
        return false;
    }
    _state = State::Running;
    SCREENLIB_LOGI(kLogTag, "offline demo mode started");
    return true;
}

} // namespace demo
