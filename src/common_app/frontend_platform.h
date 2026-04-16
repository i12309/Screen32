#pragma once

#include <memory>

#include "common_app/frontend_config.h"
#include "link/ITransport.h"

namespace demo {

// Platform hook: create transport instance according to parsed frontend config.
// Returns nullptr for offline/none mode or when transport bootstrap fails.
std::unique_ptr<ITransport> platform_create_transport(const FrontendConfig& config);

} // namespace demo

