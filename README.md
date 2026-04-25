# Screen32

`Screen32` — frontend-интеграционный и demo/runtime проект.

Репозиторий использует:
- `screenLIB` как библиотеку протокола и runtime-слоев;
- `ScreenUI` как пакет UI с LVGL, generated-артефактами и concrete-адаптером.

Репозиторий владеет только:
- orchestration runtime-логики в `src/common_app`;
- платформенным стартом и transport wiring в `src/platform_esp32` и `src/platform_web`;
- glue-кодом сборки для demo/web в `demo_web`, `scripts`, `platformio.ini`.

Репозиторий не должен владеть:
- concrete LVGL/EEZ adapter implementation;
- generated UI sources;
- generated shared/frontend meta;
- vendor-кодом LVGL.

Все эти части должны жить в `ScreenUI`.

## Зависимости

Сборка использует зависимости только из submodule внутри этого репозитория:
1. `lib/screenLIB`
2. `lib/ScreenUI`

После клонирования инициализируйте их командой:

```bash
git submodule update --init --recursive
```

## Offline demo

Стартовая offline-страница по умолчанию — `scr_LOAD0` с id `1`.
Это значение задано в `demo_web/frontend_config.json` через:

```json
"firstOfflinePage": 1
```

Основной сценарий demo теперь задается отдельно в файле:

```cpp
src/demo/offline_demo_scenario.cpp
```

Там задаются:
- список страниц, которые участвуют в demo, и их порядок для `Next`/`Prev`;
- маршруты конкретных кнопок;
- кнопки и страницы, которые должны работать как `Back()` по истории переходов.

## Логи обмена

Логи входящих и исходящих protocol-сообщений включаются ключом `log_traffic` в frontend-конфиге:

```json
"log_traffic": 1
```

Чтобы выключить их, задайте:

```json
"log_traffic": 0
```

В мониторе они идут с тегом `frontapp.traffic` и направлением `RX`/`TX`, например:

```text
TX hello device=... type=esp32
RX show_page page=1 session=42
TX button_event element=10 page=1 session=42 action=click
```

## Сборка

ESP32:

```bash
pio run
```

Web target:

```bash
pio run -t build_web
```

Вспомогательные скрипты:
- Windows: `scripts/build_web.bat`, `scripts/serve_web.bat`
- Linux/macOS: `build_web.sh`
