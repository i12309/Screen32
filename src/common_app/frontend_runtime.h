#pragma once

#include <stdint.h>

#include "common_app/frontend_config.h"

namespace demo {

// Инициализирует слой оркестрации frontend runtime:
// - связывает сгенерированную карту UI и мост событий,
// - выбирает режим online/offline,
// - связывает transport/client/adapter/offline-контроллер.
bool frontend_runtime_init(const FrontendConfig& config);

// Инициализирует runtime в принудительном online-режиме.
// Если transport не поднялся, вернет false без авто-перехода в demo.
bool frontend_runtime_init_online(const FrontendConfig& config);

// Инициализирует runtime в принудительном offline demo-режиме.
bool frontend_runtime_init_offline_demo(const FrontendConfig& config);

// Выполняет один шаг runtime. В online-режиме продвигает ScreenClient и heartbeat.
// В offline_demo режиме оркестрация событийная, поэтому tick не выполняет действий.
void frontend_runtime_tick();

// Возвращает true, если runtime работает в online-режиме (управляется backend).
bool frontend_runtime_is_online();

// Возвращает true, если runtime работает в локальном offline demo режиме.
bool frontend_runtime_is_offline_demo();

// Возвращает true, если demo-модуль подключен в текущей сборке.
bool frontend_runtime_demo_available();

// Возвращает true после первого входящего сообщения от backend.
bool frontend_runtime_backend_connected();

// Принудительно переключает уже инициализированный runtime в offline demo.
bool frontend_runtime_switch_to_offline_demo();

// Возвращает текущий сгенерированный page id активного LVGL-экрана.
uint32_t frontend_runtime_current_page();

} // namespace demo
