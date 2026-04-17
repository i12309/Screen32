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

Стартовая offline-страница по умолчанию — `scr_LOAD` с id `1`.
Это значение задано в `demo_web/frontend_config.json` через:

```json
"firstOfflinePage": 1
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
