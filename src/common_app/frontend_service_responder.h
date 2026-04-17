#pragma once

#include <stddef.h>

#include "common_app/frontend_config.h"
#include "ui_object_map.generated.h"
#include "runtime/ScreenClient.h"

namespace demo {

/*
 * Роль файла:
 * - Обрабатывает входящие service-запросы от backend и отправляет protobuf-ответы.
 * Вызывается:
 * - из обработчика клиента в frontend_runtime при входящих envelope.
 * Должен содержать:
 * - логику ответов request_device_info/current_page/page_state/element_state.
 * НЕ должен содержать:
 * - оркестрацию init/tick runtime, offline demo-правила, регистрацию LVGL-обработчиков.
 */

struct FrontendServiceResponderContext {
    screenlib::client::ScreenClient* client = nullptr;
    const Screen32BoundElement* trackedElements = nullptr;
    size_t trackedCount = 0;
    FrontendMode mode = FrontendMode::Wasm;
};

// Собирает payload метаданных устройства для ответов на hello/device_info.
DeviceInfo frontend_build_device_info(FrontendMode mode);

// Обрабатывает один входящий envelope и при необходимости отправляет ответ через ctx.client.
void frontend_handle_service_request(const Envelope& env, const FrontendServiceResponderContext& ctx);

} // namespace demo


