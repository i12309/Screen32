# Screen32: Фронтенд На EEZ + LVGL + screenLIB

`Screen32` — frontend/screen-client проект.
`screenLIB` подключается как внешняя зависимость (submodule `lib/screenLIB`), а не копируется в `src/`.

## Границы Репозитория

В Screen32 находится только integration/runtime/config/demo код frontend:

- `src/common_app/frontend_runtime.*`
- `src/common_app/frontend_service_responder.*`
- `src/common_app/frontend_ui_events.*`
- `src/common_app/page_state_builder.*`
- `src/common_app/offline_demo_controller.*`
- `src/common_app/frontend_config.*`
- `src/platform_web/platform.cpp`
- `src/platform_esp32/platform.cpp`
- `demo_web/frontend_config.json`
- build-склейка (`platformio.ini`, `demo_web/CMakeLists.txt`, `scripts/*.bat`)

Копий `screenLIB` или отдельного runtime `nanopb` в `src/` нет.

## Цепочка Запуска

Старт frontend проходит так:

1. `shared_app/app_setup()`
2. `app_core_init()` инициализирует сгенерированный EEZ/LVGL UI
3. `frontend_load_config()` загружает JSON-конфиг
4. `frontend_runtime_init(config)` выбирает режим и связывает runtime-слои

Ответственность слоев runtime:

- `frontend_runtime`: только orchestration (выбор режима, wiring, tick)
- `frontend_ui_events`: LVGL callbacks -> frontend button/input события
- `frontend_service_responder`: обработчики service-запросов backend
- `page_state_builder`: live UI -> protobuf `PageState` / `ElementState`
- `offline_demo_controller`: все правила offline demo-навигации

## Режимы Работы

### Online (`offline_demo = 0`)

1. Создать transport
2. Создать `ScreenClient`
3. Создать `EezLvglAdapter`
4. Показать `firstOnlinePage` (или fallback)
5. Отправить `hello/device_info`
6. Ждать команды backend и отвечать на service-запросы

В online-режиме локальной demo-навигации нет.

### Offline Demo (`offline_demo = 1`)

1. Поднять тот же UI и те же generated bindings
2. Инициализировать `OfflineDemoController`
3. Стартовать с `firstOfflinePage` (или fallback)
4. Локально обрабатывать button/input события (backend transport не обязателен)

Все demo-правила навигации (order, next/prev/goto bindings) находятся в `OfflineDemoController`.

## Конфиг Frontend

Поддерживаемые поля:

- `mode`
- `transport` (`type`, `url`, `baud`, `rxPin`, `txPin`)
- `offline_demo`
- `firstOnlinePage`
- `firstOfflinePage`

Пример:

```json
{
  "mode": "wasm",
  "transport": {
    "type": "ws_client",
    "url": "ws://127.0.0.1:81",
    "baud": 115200,
    "rxPin": 16,
    "txPin": 17
  },
  "offline_demo": 1,
  "firstOnlinePage": 1,
  "firstOfflinePage": 2
}
```

Поле `start_page` всё ещё парсится для обратной совместимости, но лучше использовать mode-specific поля.

## Сгенерированный Meta-Layer

UI meta генерируется из EEZ/LVGL UI и является источником истины для метаданных страниц/элементов.

Общие generated-файлы (можно использовать и на frontend, и на backend):

- `src/common_app/generated/page_ids.generated.h`
- `src/common_app/generated/element_ids.generated.h`
- `src/common_app/generated/page_descriptors.generated.h`
- `src/common_app/generated/element_descriptors.generated.h`

Generated-файлы только для frontend:

- `src/common_app/generated/ui_object_map.generated.h`
- `src/common_app/generated/ui_object_map.generated.cpp`
- `src/common_app/generated/eez_page_meta.generated.cpp`

Generator — build-time tool, а не runtime-библиотека.

Ручной запуск:

```powershell
python tools/ui_meta_gen/generate_ui_meta.py
```

Совместимый wrapper:

```powershell
python scripts/generate_ui_meta.py
```

Сборка PlatformIO также запускает генерацию через `extra_script.py`.

## Сборка

Инициализация submodule:

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
```

Запуск локального просмотра:

```bat
scripts\serve_web.bat
```

Если запускать вручную через `python -m http.server`, делайте это из корня проекта
или передавайте абсолютный путь в `--directory`.

Открыть: `http://localhost:8080/demo_web.html`.

### Требования Для Web

- установленный `emsdk`
- установленный `cmake`

Отдельный desktop/system пакет SDL2 для web-сборки не нужен: используется SDL2-порт Emscripten (`-sUSE_SDL=2`).

## Локальная Web-Сборка На Windows

Для удобного локального запуска на Windows (без machine-specific путей в общих project files):

- `scripts\env_web.bat`: активирует emsdk и добавляет `cmake`
- `scripts\build_web.bat`: поднимает окружение и запускает `pio run -t build_web`
- `scripts\serve_web.bat`: запускает локальный web-сервер для `demo_web/build_web`

Рекомендуемая команда:

```bat
scripts\build_web.bat
```

Опциональные переменные окружения:

- `SCREEN32_EMSDK_DIR`
- `SCREEN32_CMAKE_DIR`

Если они не заданы, скрипты используют локальные fallback-пути:

- `C:\Users\sign\CODE\Emsdk\emsdk`
- `C:\Users\sign\CODE\Emsdk\cmake-4.3.1-windows-x86_64`

Machine-specific пути используются только в локальных `scripts/*.bat`, общие project files остаются machine-agnostic.
