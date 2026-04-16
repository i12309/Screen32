#pragma once

#include <stdint.h>

#include "common_app/frontend_config.h"

namespace demo {

bool frontend_runtime_init(const FrontendConfig& config);
void frontend_runtime_tick();

bool frontend_runtime_is_online();
bool frontend_runtime_is_offline_demo();
uint32_t frontend_runtime_current_page();

} // namespace demo

