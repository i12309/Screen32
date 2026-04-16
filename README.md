# Screen32 Frontend (EEZ + LVGL + screenLIB)

`Screen32` — frontend/screen-client проект.
`screenLIB` подключен как внешняя зависимость (submodule), без копирования исходников в `src/`.

## Зависимости

- `lib/screenLIB` — git submodule (`https://github.com/i12309/screenLIB.git`)
- LVGL и `esp32-smartdisplay` в `lib/`
- `nanopb`:
  - ESP32/PlatformIO: через `lib_deps` (`nanopb/Nanopb`)
  - Web/CMake: через `FetchContent` (github `nanopb`)

## Что находится в Screen32

Только integration-код:

- `src/common_app/frontend_runtime.*`
- `src/common_app/frontend_config.*`
- `src/common_app/frontend_platform.h`
- `src/platform_esp32/platform.cpp`
- `src/platform_web/platform.cpp`
- `demo_web/frontend_config.json`
- build glue (`platformio.ini`, `demo_web/CMakeLists.txt`)

Локальных копий `screenLIB` и `nanopb` в `src/` нет.

## Страницы demo (6)

- `SCREEN_ID_LOAD`
- `SCREEN_ID_MAIN_MENU`
- `SCREEN_ID_DEF_PAGE1`
- `SCREEN_ID_DEF_PAGE2`
- `SCREEN_ID_DEF_PAGE3`
- `SCREEN_ID_DEF_PAGE4`

## Frontend Config JSON

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

Где лежит:

- Web: `demo_web/frontend_config.json` (preload в `/frontend_config.json`)
- ESP32: embedded JSON в `src/platform_esp32/platform.cpp`

## Режимы

- `offline_demo=1`: UI работает локально через `navigation.*`, backend не обязателен.
- `offline_demo=0`: frontend работает как screen-client через transport из конфига.

## Сборка

Перед сборкой подтянуть submodule:

```powershell
git submodule update --init --recursive
```

ESP32:

```powershell
pio run -e JC8048W550C
```

Web:

```powershell
pio run -t build_web
python -m http.server 8080 --directory demo_web/build
```

Открыть: `http://localhost:8080/demo_web.html`

## Windows local web build

Для локальной web-сборки на Windows добавлены helper-скрипты:

- `scripts\env_web.bat` — активирует emsdk и добавляет CMake в `PATH`
- `scripts\build_web.bat` — поднимает окружение и запускает `pio run -t build_web`
- `scripts\serve_web.bat` — запускает локальный `http.server` для `demo_web/build`

Рекомендуемый запуск:

```bat
scripts\build_web.bat
```

Скрипты поддерживают переменные окружения:

- `SCREEN32_EMSDK_DIR`
- `SCREEN32_CMAKE_DIR`

Если переменные не заданы, используются локальные fallback-пути:

- `C:\Users\sign\CODE\Emsdk\emsdk`
- `C:\Users\sign\CODE\Emsdk\cmake-4.3.1-windows-x86_64`

Важно: machine-specific paths используются только в `scripts/*.bat`.
`platformio.ini`, `demo_web/CMakeLists.txt` и исходники проекта остаются machine-agnostic.
