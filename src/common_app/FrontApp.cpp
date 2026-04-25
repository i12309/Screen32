#include "common_app/FrontApp.h"

#include "common_app/frontend_runtime.h"

namespace frontapp {

bool init(const demo::FrontendConfig& config) {
    return demo::frontend_runtime_init(config);
}

void tick() {
    demo::frontend_runtime_tick();
}

bool is_online() {
    return demo::frontend_runtime_is_online();
}

bool is_offline_demo() {
    return demo::frontend_runtime_is_offline_demo();
}

bool switch_to_offline_demo() {
    return demo::frontend_runtime_switch_to_offline_demo();
}

bool backend_connected() {
    return demo::frontend_runtime_backend_connected();
}

uint32_t current_page() {
    return demo::frontend_runtime_current_page();
}

uint32_t current_session_id() {
    return 0;
}

} // namespace frontapp
