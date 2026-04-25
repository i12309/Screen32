#pragma once

#include <stdint.h>

#include "common_app/frontend_config.h"

namespace frontapp {

bool init(const demo::FrontendConfig& config);
void tick();

bool is_online();
bool is_offline_demo();
bool switch_to_offline_demo();
bool backend_connected();

uint32_t current_page();
uint32_t current_session_id();

} // namespace frontapp
