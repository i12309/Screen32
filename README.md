# EEZ Studio + LVGL Cross-Platform Demo

Этот проект собирает один и тот же UI (EEZ + LVGL) для двух платформ:
- ESP32 (`platformio`)
- WebAssembly (`emscripten + cmake`)

## Актуальная структура

```text
DEMO/
├── src/
│   ├── common_app/          # Общий app-слой для ESP32 и Web
│   │   ├── app_core.*
│   │   ├── navigation.*
│   │   └── shared_app.*
│   ├── platform_esp32/      # Платформенный слой ESP32
│   │   └── platform.*
│   ├── platform_web/        # Платформенный слой Web (Emscripten/SDL)
│   │   ├── platform.*
│   │   └── web_main.cpp
│   ├── ui/                  # Сгенерированный EEZ UI (read-only)
│   └── main.cpp             # Arduino entry point для ESP32
├── include/
│   └── lv_conf.h            # LVGL конфиг для ESP32
├── demo_web/
│   ├── CMakeLists.txt       # Web build (LVGL как полноценная CMake-библиотека)
│   ├── lv_conf.h            # LVGL конфиг для Web
│   └── shell.html           # HTML-обертка canvas
├── lib/
│   ├── lvgl/                # Единый LVGL для обеих платформ
│   └── esp32-smartdisplay/  # Локальная библиотека дисплея ESP32
├── platformio.ini
└── extra_script.py          # Цель `pio run -t build_web`
```

## Ключевые принципы

- Один общий app-код: `src/common_app`.
- Один LVGL для всех сборок: `lib/lvgl`.
- Разделены только платформенные адаптеры:
  - `src/platform_esp32`
  - `src/platform_web`

## Требования

- PlatformIO
- Emscripten SDK (доступны `emcmake`, `emcc`)
- CMake + Ninja

## Сборка

### ESP32

```powershell
pio run -e JC8048W550C
```

### Web

```powershell
pio run -t build_web
python -m http.server 8080 --directory demo_web/build
```

Открыть: `http://localhost:8080/demo_web.html`

## Примечания по Web UI

- Рендер идет в canvas 800x480.
- Масштабирование в `shell.html` ограничено:
  - больше 800x480 не растягивается,
  - на меньших экранах масштабируется вниз с сохранением пропорций.
