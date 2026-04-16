# Инструмент Генерации UI-Меты

`tools/ui_meta_gen` — это build/codegen-инструмент.
Он не линкуется в runtime прошивки или wasm.

Вход:
- `src/ui/screens.h`
- `src/ui/screens.c`

Выход:
- Общие заголовки:
  - `src/common_app/generated/page_ids.generated.h`
  - `src/common_app/generated/element_ids.generated.h`
  - `src/common_app/generated/page_descriptors.generated.h`
  - `src/common_app/generated/element_descriptors.generated.h`
- Файлы только для frontend:
  - `src/common_app/generated/ui_object_map.generated.h`
  - `src/common_app/generated/ui_object_map.generated.cpp`
  - `src/common_app/generated/eez_page_meta.generated.cpp`

Запуск:

```powershell
python tools/ui_meta_gen/generate_ui_meta.py
```

Совместимый wrapper:

```powershell
python scripts/generate_ui_meta.py
```
