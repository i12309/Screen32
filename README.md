# Screen32 Frontend (EEZ + LVGL + screenLIB client)

`Screen32` — это frontend/screen-client проект.
Интеграция выполнена через `screenLIB` (только client-side части):

- `core`: `FrameCodec`, `ProtoCodec`, `machine.pb.*`, `ScreenBridge`, `ITransport`
- `client`: `ScreenClient`, `CommandDispatcher`, `WebSocketClientLink`, `UartClientLink`
- `adapter`: `IUiAdapter`, `EezLvglAdapter`, `UiObjectMap`

`host` модуль (`ScreenSystem`, `ScreenManager`, `PageRegistry`) в этот проект не подключается.

## Структура

- `src/common_app/`
  - `app_core.*` — общий LVGL цикл
  - `navigation.*` — локальная offline-навигация
  - `frontend_config.*` — загрузка/parsing frontend JSON
  - `frontend_runtime.*` — общий runtime для online/offline
  - `shared_app.*` — общий entrypoint `app_setup/app_loop`
- `src/platform_esp32/` — ESP32 startup + config hook + transport hook
- `src/platform_web/` — Web startup + config hook + transport hook
- `src/screenlib/` — встроенные `core/client/adapter` части screenLIB
- `src/third_party/nanopb/` — protobuf runtime (`pb_*`)
- `demo_web/` — CMake target web-сборки

## Участвующие страницы (6)

- `SCREEN_ID_LOAD`
- `SCREEN_ID_MAIN_MENU`
- `SCREEN_ID_DEF_PAGE1`
- `SCREEN_ID_DEF_PAGE2`
- `SCREEN_ID_DEF_PAGE3`
- `SCREEN_ID_DEF_PAGE4`

## Frontend Config JSON

Минимальный формат:

```json
{
  "mode": "esp32",
  "transport": {
    "type": "uart",
    "url": "ws://127.0.0.1:81",
    "baud": 115200,
    "rxPin": 16,
    "txPin": 17
  },
  "offline_demo": 1,
  "start_page": 2
}
```

Где лежит конфиг:

- Web: `demo_web/frontend_config.json` (preload в WASM FS как `/frontend_config.json`)
- ESP32: compile-time embedded JSON в `src/platform_esp32/platform.cpp` (`platform_load_frontend_config_json`)

## Режимы

`offline_demo=1`:

- transport может быть `none`
- frontend стартует без backend
- UI интерактивен
- локальная навигация работает через `navigation.*`

`offline_demo=0`:

- frontend работает как реальный screen client
- transport поднимается из `transport.type`:
  - ESP32: `uart` (или `ws_client`, если явно задано)
  - Web: `ws_client`
- команды от backend (`show_page`, `set_text`, `set_value`, `set_visible`, `set_color`, `set_batch`) применяются через `ScreenClient -> EezLvglAdapter`

## Сборка ESP32

```powershell
pio run -e JC8048W550C
```

## Сборка Web

```powershell
pio run -t build_web
python -m http.server 8080 --directory demo_web/build
```

Открыть: `http://localhost:8080/demo_web.html`

