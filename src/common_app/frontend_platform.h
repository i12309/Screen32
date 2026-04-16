#pragma once

#include <memory>

#include "common_app/frontend_config.h"
#include "link/ITransport.h"

namespace demo {

// Платформенный хук: создает экземпляр transport по распарсенному frontend-конфигу.
// Возвращает nullptr для режима offline/none или при ошибке инициализации transport.
std::unique_ptr<ITransport> platform_create_transport(const FrontendConfig& config);

} // namespace demo
