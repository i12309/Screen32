# Offline-demo страницы

Эта папка содержит шаблон для собственных offline-страниц, которые показываются, когда нет связи с backend.

## Файлы

- `offline_demo_controller.cpp/.h` - runtime demo-режима. Вызывается из `FrontApp` при старте в offline demo или при `switch_to_offline_demo()`.
- `offline_demo_scenario.cpp/.h` - декларативный сценарий: порядок страниц, переходы по кнопкам и переходы по тапу по странице.
- `offline_demo_ui_events.cpp/.h` - demo-only обработчики кликов по объектам и страницам. Они работают локально и не отправляют события backend.

## Как добавить свою demo-страницу

1. Открой `offline_demo_scenario.cpp`.
2. Добавь нужную страницу в `kDefaultPageOrder`, если она должна участвовать в переходах Next/Prev.
3. Добавь маршруты в `kDefaultButtonRoutes` или `kDefaultPageTapRoutes`.
4. Пересобери: `pio run -e JC8048W550C`.
5. Прошей и проверь offline-сценарий на экране.

Demo-режим не общается с backend. Любое изменение UI в этом режиме применяется напрямую к LVGL через `OfflineDemoController` и `EezLvglAdapter`.
