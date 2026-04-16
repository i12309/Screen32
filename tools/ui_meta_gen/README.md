# UI Meta Generator Tool

`tools/ui_meta_gen` is a build/codegen tool.
It is not linked into firmware or wasm runtime.

Input:
- `src/ui/screens.h`
- `src/ui/screens.c`

Output:
- Shared headers:
  - `src/common_app/generated/page_ids.generated.h`
  - `src/common_app/generated/element_ids.generated.h`
  - `src/common_app/generated/page_descriptors.generated.h`
  - `src/common_app/generated/element_descriptors.generated.h`
- Frontend-only files:
  - `src/common_app/generated/ui_object_map.generated.h`
  - `src/common_app/generated/ui_object_map.generated.cpp`
  - `src/common_app/generated/eez_page_meta.generated.cpp`

Run:

```powershell
python tools/ui_meta_gen/generate_ui_meta.py
```

Compatibility wrapper:

```powershell
python scripts/generate_ui_meta.py
```
