#include "platform_web/platform.h"
#include "common_app/frontend_config.h"
#include "common_app/frontend_platform.h"
#include "link/WebSocketClientLink.h"

#include <lvgl.h>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <cstring>

#include <emscripten/emscripten.h>

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

} // namespace

void platform_init(void) {
    if (g_platform_initialized) {
        return;
    }

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
    vprintf(fmt, args);
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
        "  \"start_page\": 2\n"
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
        auto transport = std::make_unique<WebSocketClientLink>();
        if (!transport->begin(config.transport.url)) {
            return nullptr;
        }
        return transport;
    }

    return nullptr;
}

} // namespace demo
