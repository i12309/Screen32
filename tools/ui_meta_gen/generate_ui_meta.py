#!/usr/bin/env python3
"""Генерирует UI meta-layer из сгенерированных EEZ/LVGL исходников UI.

Вход:
- src/ui/screens.h
- src/ui/screens.c

Выход (в src/common_app/generated):
- page_ids.generated.h
- element_ids.generated.h
- page_descriptors.generated.h
- element_descriptors.generated.h
- ui_object_map.generated.h
- ui_object_map.generated.cpp
- eez_page_meta.generated.cpp
"""

from __future__ import annotations

import hashlib
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List

REPO_ROOT = Path(__file__).resolve().parents[2]
UI_DIR = REPO_ROOT / "src" / "ui"
OUT_DIR = REPO_ROOT / "src" / "common_app" / "generated"

SCREENS_H = UI_DIR / "screens.h"
SCREENS_C = UI_DIR / "screens.c"


@dataclass(frozen=True)
class PageInfo:
    enum_name: str
    page_id: int
    object_name: str


@dataclass(frozen=True)
class ElementInfo:
    object_name: str
    page_enum: str
    page_id: int
    creator: str


BUTTON_CREATORS = {"lv_button", "lv_btn"}
VALUE_CREATORS = {"lv_slider", "lv_bar", "lv_arc", "lv_spinbox", "lv_roller", "lv_dropdown", "lv_switch"}
INPUT_CREATORS = {"lv_textarea"}
TEXT_CREATORS = {"lv_label"}


def read_text(path: Path) -> str:
    if not path.exists():
        raise FileNotFoundError(f"Missing required input file: {path}")
    return path.read_text(encoding="utf-8")


def to_object_name(page_enum_name: str) -> str:
    if not page_enum_name.startswith("SCREEN_ID_"):
        raise ValueError(f"Invalid page enum name: {page_enum_name}")
    return page_enum_name[len("SCREEN_ID_") :].lower()


def to_create_function(page_enum_name: str) -> str:
    return f"create_screen_{to_object_name(page_enum_name)}"


def parse_pages_and_objects(screens_h: str) -> tuple[List[PageInfo], List[str]]:
    pages: List[PageInfo] = []
    objects: List[str] = []

    enum_re = re.compile(r"^\s*(SCREEN_ID_[A-Z0-9_]+)\s*=\s*(\d+)\s*,\s*$")
    object_re = re.compile(r"^\s*lv_obj_t\s*\*\s*([A-Za-z_][A-Za-z0-9_]*)\s*;\s*$")

    in_enum = False
    in_struct = False
    for raw_line in screens_h.splitlines():
        line = raw_line.strip()

        if line.startswith("enum ScreensEnum"):
            in_enum = True
            continue
        if in_enum:
            if line.startswith("};"):
                in_enum = False
                continue
            match = enum_re.match(raw_line)
            if match:
                enum_name = match.group(1)
                if enum_name.startswith("_SCREEN_ID_"):
                    continue
                page_id = int(match.group(2))
                pages.append(PageInfo(enum_name=enum_name, page_id=page_id, object_name=to_object_name(enum_name)))
            continue

        if line.startswith("typedef struct _objects_t"):
            in_struct = True
            continue
        if in_struct:
            if line.startswith("} objects_t;"):
                in_struct = False
                continue
            match = object_re.match(raw_line)
            if match:
                objects.append(match.group(1))

    if not pages:
        raise RuntimeError("Failed to parse page enum from screens.h")
    if not objects:
        raise RuntimeError("Failed to parse objects struct from screens.h")

    return pages, objects


def parse_object_assignments(screens_c: str, pages: List[PageInfo]) -> Dict[str, ElementInfo]:
    func_to_page: Dict[str, PageInfo] = {to_create_function(p.enum_name): p for p in pages}

    current_page: PageInfo | None = None
    current_creator = "lv_obj"

    fn_re = re.compile(r"^\s*void\s+(create_screen_[a-zA-Z0-9_]+)\s*\(")
    creator_re = re.compile(r"\blv_obj_t\s*\*\s*obj\s*=\s*([a-zA-Z0-9_]+)_create\s*\(")
    assign_re = re.compile(r"\bobjects\.([A-Za-z_][A-Za-z0-9_]*)\s*=\s*obj\s*;")

    out: Dict[str, ElementInfo] = {}
    for raw_line in screens_c.splitlines():
        fn_match = fn_re.match(raw_line)
        if fn_match:
            fn_name = fn_match.group(1)
            current_page = func_to_page.get(fn_name)
            current_creator = "lv_obj"
            continue

        creator_match = creator_re.search(raw_line)
        if creator_match:
            current_creator = creator_match.group(1)

        assign_match = assign_re.search(raw_line)
        if assign_match and current_page is not None:
            obj_name = assign_match.group(1)
            out[obj_name] = ElementInfo(
                object_name=obj_name,
                page_enum=current_page.enum_name,
                page_id=current_page.page_id,
                creator=current_creator,
            )

    return out


def normalize_element_name(object_name: str) -> str:
    return re.sub(r"[^A-Za-z0-9]+", "_", object_name).strip("_").upper()


def make_element_enum_name(object_name: str) -> str:
    return f"SCREEN32_ELEMENT_ID_{normalize_element_name(object_name)}"


def stable_element_id(object_name: str, used_ids: set[int]) -> int:
    # Стабилизируем ID по хэшу имени объекта; редкие коллизии разрешаем детерминированно.
    seed = f"screen32:element:{object_name}"
    counter = 0
    while True:
        material = seed if counter == 0 else f"{seed}#{counter}"
        digest = hashlib.sha1(material.encode("utf-8")).digest()
        candidate = int.from_bytes(digest[:4], byteorder="big") & 0x1FFFFFFF
        candidate |= 0x20000000
        if candidate not in used_ids:
            used_ids.add(candidate)
            return candidate
        counter += 1


def infer_type_and_flags(element: ElementInfo) -> tuple[str, Dict[str, bool]]:
    creator = element.creator

    flags = {
        "supports_text": False,
        "supports_value": False,
        "supports_visible": True,
        "supports_color": True,
        "emits_button_event": False,
        "emits_input_event": False,
    }

    if creator in BUTTON_CREATORS:
        flags["supports_text"] = True
        flags["emits_button_event"] = True
        return "SCREEN32_ELEMENT_TYPE_BUTTON", flags

    if creator in INPUT_CREATORS:
        flags["supports_text"] = True
        flags["emits_input_event"] = True
        return "SCREEN32_ELEMENT_TYPE_INPUT", flags

    if creator in VALUE_CREATORS:
        flags["supports_value"] = True
        flags["emits_input_event"] = True
        return "SCREEN32_ELEMENT_TYPE_VALUE", flags

    if creator in TEXT_CREATORS:
        flags["supports_text"] = True
        return "SCREEN32_ELEMENT_TYPE_TEXT", flags

    # Для generic-контейнеров по умолчанию считаем text-capable через вложенный поиск label.
    flags["supports_text"] = True
    return "SCREEN32_ELEMENT_TYPE_TEXT", flags


def render_page_ids_header(pages: List[PageInfo]) -> str:
    lines = [
        "#pragma once",
        "",
        "#include <stdint.h>",
        "",
        "#ifdef __cplusplus",
        "extern \"C\" {",
        "#endif",
        "",
        "typedef enum Screen32PageId {",
    ]
    for page in pages:
        short_name = page.enum_name[len("SCREEN_ID_") :]
        lines.append(f"    SCREEN32_PAGE_ID_{short_name} = {page.page_id},")
    lines.extend(
        [
            "} Screen32PageId;",
            "",
            f"#define SCREEN32_PAGE_DESCRIPTOR_COUNT {len(pages)}",
            "",
            "#ifdef __cplusplus",
            "}",
            "#endif",
            "",
        ]
    )
    return "\n".join(lines)


def render_element_ids_header(element_order: List[str], element_ids: Dict[str, int]) -> str:
    lines = [
        "#pragma once",
        "",
        "#include <stdint.h>",
        "",
        "#ifdef __cplusplus",
        "extern \"C\" {",
        "#endif",
        "",
        "typedef enum Screen32ElementId {",
    ]
    for name in element_order:
        enum_name = make_element_enum_name(name)
        lines.append(f"    {enum_name} = {element_ids[name]}u,")
    lines.extend(
        [
            "} Screen32ElementId;",
            "",
            f"#define SCREEN32_ELEMENT_DESCRIPTOR_COUNT {len(element_order)}",
            "",
            "#ifdef __cplusplus",
            "}",
            "#endif",
            "",
        ]
    )
    return "\n".join(lines)


def render_page_descriptors_header(pages: List[PageInfo]) -> str:
    lines = [
        "#pragma once",
        "",
        "#include <stddef.h>",
        "#include <stdint.h>",
        "",
        "#include \"common_app/generated/page_ids.generated.h\"",
        "",
        "#ifdef __cplusplus",
        "extern \"C\" {",
        "#endif",
        "",
        "typedef struct Screen32PageDescriptor {",
        "    uint32_t page_id;",
        "    const char* page_name;",
        "    const char* object_name;",
        "} Screen32PageDescriptor;",
        "",
        "static const Screen32PageDescriptor g_screen32_page_descriptors[SCREEN32_PAGE_DESCRIPTOR_COUNT] = {",
    ]
    for page in pages:
        short_name = page.enum_name[len("SCREEN_ID_") :]
        lines.append(
            f"    {{{page.page_id}u, \"{short_name}\", \"{page.object_name}\"}},"
        )
    lines.extend(
        [
            "};",
            "",
            "static inline size_t screen32_page_descriptor_count(void) {",
            "    return (size_t)SCREEN32_PAGE_DESCRIPTOR_COUNT;",
            "}",
            "",
            "static inline const Screen32PageDescriptor* screen32_find_page_descriptor(uint32_t page_id) {",
            "    for (size_t i = 0; i < screen32_page_descriptor_count(); ++i) {",
            "        if (g_screen32_page_descriptors[i].page_id == page_id) {",
            "            return &g_screen32_page_descriptors[i];",
            "        }",
            "    }",
            "    return 0;",
            "}",
            "",
            "#ifdef __cplusplus",
            "}",
            "#endif",
            "",
        ]
    )
    return "\n".join(lines)


def render_element_descriptors_header(
    element_order: List[str],
    element_map: Dict[str, ElementInfo],
    element_ids: Dict[str, int],
) -> str:
    lines = [
        "#pragma once",
        "",
        "#include <stdbool.h>",
        "#include <stddef.h>",
        "#include <stdint.h>",
        "",
        "#include \"common_app/generated/element_ids.generated.h\"",
        "#include \"common_app/generated/page_ids.generated.h\"",
        "",
        "#ifdef __cplusplus",
        "extern \"C\" {",
        "#endif",
        "",
        "typedef enum Screen32ElementType {",
        "    SCREEN32_ELEMENT_TYPE_BUTTON = 1,",
        "    SCREEN32_ELEMENT_TYPE_TEXT = 2,",
        "    SCREEN32_ELEMENT_TYPE_VALUE = 3,",
        "    SCREEN32_ELEMENT_TYPE_INPUT = 4,",
        "    SCREEN32_ELEMENT_TYPE_VISIBLE = 5",
        "} Screen32ElementType;",
        "",
        "typedef struct Screen32ElementDescriptor {",
        "    uint32_t element_id;",
        "    const char* element_name;",
        "    const char* object_name;",
        "    uint32_t page_id;",
        "    const char* page_name;",
        "    Screen32ElementType element_type;",
        "    bool supports_text;",
        "    bool supports_value;",
        "    bool supports_visible;",
        "    bool supports_color;",
        "    bool emits_button_event;",
        "    bool emits_input_event;",
        "} Screen32ElementDescriptor;",
        "",
        "static const Screen32ElementDescriptor g_screen32_element_descriptors[SCREEN32_ELEMENT_DESCRIPTOR_COUNT] = {",
    ]

    for name in element_order:
        info = element_map[name]
        elem_type, flags = infer_type_and_flags(info)
        page_short = info.page_enum[len("SCREEN_ID_") :]
        lines.append(
            "    "
            + "{"
            + f"{element_ids[name]}u, "
            + f"\"{make_element_enum_name(name)}\", "
            + f"\"{name}\", "
            + f"{info.page_id}u, "
            + f"\"{page_short}\", "
            + f"{elem_type}, "
            + f"{str(flags['supports_text']).lower()}, "
            + f"{str(flags['supports_value']).lower()}, "
            + f"{str(flags['supports_visible']).lower()}, "
            + f"{str(flags['supports_color']).lower()}, "
            + f"{str(flags['emits_button_event']).lower()}, "
            + f"{str(flags['emits_input_event']).lower()}"
            + "},"
        )

    lines.extend(
        [
            "};",
            "",
            "static inline size_t screen32_element_descriptor_count(void) {",
            "    return (size_t)SCREEN32_ELEMENT_DESCRIPTOR_COUNT;",
            "}",
            "",
            "static inline const Screen32ElementDescriptor* screen32_find_element_descriptor(uint32_t element_id) {",
            "    for (size_t i = 0; i < screen32_element_descriptor_count(); ++i) {",
            "        if (g_screen32_element_descriptors[i].element_id == element_id) {",
            "            return &g_screen32_element_descriptors[i];",
            "        }",
            "    }",
            "    return 0;",
            "}",
            "",
            "#ifdef __cplusplus",
            "}",
            "#endif",
            "",
        ]
    )
    return "\n".join(lines)


def render_ui_object_map_header() -> str:
    return "\n".join(
        [
            "#pragma once",
            "",
            "#include <stddef.h>",
            "#include <stdint.h>",
            "",
            "#include <lvgl.h>",
            "",
            "#include \"common_app/generated/element_descriptors.generated.h\"",
            "#include \"common_app/generated/page_descriptors.generated.h\"",
            "#include \"lvgl_eez/UiObjectMap.h\"",
            "",
            "namespace demo {",
            "",
            "struct Screen32BoundElement {",
            "    uint32_t elementId = 0;",
            "    uint32_t pageId = 0;",
            "    lv_obj_t* obj = nullptr;",
            "    const Screen32ElementDescriptor* descriptor = nullptr;",
            "};",
            "",
            "bool screen32_bind_generated_ui_map(screenlib::adapter::UiObjectMap& objectMap,",
            "                                  Screen32BoundElement* outElements,",
            "                                  size_t outCapacity,",
            "                                  size_t* outCount);",
            "",
            "const Screen32BoundElement* screen32_find_bound_element(const Screen32BoundElement* elements,",
            "                                                     size_t count,",
            "                                                     uint32_t elementId);",
            "",
            "uint32_t screen32_current_page_id();",
            "bool screen32_load_page_by_target(void* pageTarget);",
            "",
            "}  // namespace demo",
            "",
        ]
    )


def render_ui_object_map_cpp(
    pages: List[PageInfo],
    element_order: List[str],
    element_map: Dict[str, ElementInfo],
    element_ids: Dict[str, int],
) -> str:
    lines: List[str] = [
        "#include \"common_app/generated/ui_object_map.generated.h\"",
        "",
        "#include \"ui/screens.h\"",
        "",
        "namespace demo {",
        "namespace {",
        "",
        "bool bind_page(screenlib::adapter::UiObjectMap& objectMap, uint32_t pageId, lv_obj_t* pageObj) {",
        "    return pageObj != nullptr && objectMap.bindPage(pageId, pageObj);",
        "}",
        "",
        "bool bind_element(screenlib::adapter::UiObjectMap& objectMap,",
        "                  uint32_t elementId,",
        "                  uint32_t pageId,",
        "                  lv_obj_t* obj,",
        "                  Screen32BoundElement* outElements,",
        "                  size_t outCapacity,",
        "                  size_t& outCount) {",
        "    if (obj == nullptr) {",
        "        return false;",
        "    }",
        "    if (!objectMap.bindElement(elementId, obj)) {",
        "        return false;",
        "    }",
        "",
        "    if (outElements != nullptr && outCount < outCapacity) {",
        "        outElements[outCount].elementId = elementId;",
        "        outElements[outCount].pageId = pageId;",
        "        outElements[outCount].obj = obj;",
        "        outElements[outCount].descriptor = screen32_find_element_descriptor(elementId);",
        "        outCount++;",
        "    }",
        "",
        "    return true;",
        "}",
        "",
        "}  // namespace",
        "",
        "bool screen32_bind_generated_ui_map(screenlib::adapter::UiObjectMap& objectMap,",
        "                                  Screen32BoundElement* outElements,",
        "                                  size_t outCapacity,",
        "                                  size_t* outCount) {",
        "    objectMap.clear();",
        "    size_t boundCount = 0;",
        "    bool allOk = true;",
        "",
        "    // Карта страниц",
    ]

    for page in pages:
        lines.append(
            f"    allOk = bind_page(objectMap, {page.page_id}u, objects.{page.object_name}) && allOk;"
        )

    lines.extend(["", "    // Карта элементов", ""])

    for object_name in element_order:
        info = element_map[object_name]
        lines.append(
            "    allOk = bind_element(objectMap, "
            + f"{element_ids[object_name]}u, "
            + f"{info.page_id}u, "
            + f"objects.{object_name}, "
            + "outElements, outCapacity, boundCount) && allOk;"
        )

    lines.extend(
        [
            "",
            "    if (outCount != nullptr) {",
            "        *outCount = boundCount;",
            "    }",
            "",
            "    return allOk;",
            "}",
            "",
            "const Screen32BoundElement* screen32_find_bound_element(const Screen32BoundElement* elements,",
            "                                                     size_t count,",
            "                                                     uint32_t elementId) {",
            "    if (elements == nullptr) {",
            "        return nullptr;",
            "    }",
            "    for (size_t i = 0; i < count; ++i) {",
            "        if (elements[i].elementId == elementId) {",
            "            return &elements[i];",
            "        }",
            "    }",
            "    return nullptr;",
            "}",
            "",
            "}  // namespace demo",
            "",
        ]
    )

    return "\n".join(lines)


def render_eez_page_meta_cpp(pages: List[PageInfo]) -> str:
    lines: List[str] = [
        "#include \"common_app/generated/ui_object_map.generated.h\"",
        "",
        "#include \"ui/screens.h\"",
        "#include \"ui/ui.h\"",
        "",
        "namespace demo {",
        "",
        "uint32_t screen32_current_page_id() {",
        "    lv_obj_t* active = lv_scr_act();",
    ]

    for page in pages:
        lines.append(f"    if (active == objects.{page.object_name}) return {page.page_id}u;")

    lines.extend(
        [
            f"    return {pages[0].page_id}u;",
            "}",
            "",
            "bool screen32_load_page_by_target(void* pageTarget) {",
            "    lv_obj_t* target = static_cast<lv_obj_t*>(pageTarget);",
            "    if (target == nullptr) {",
            "        return false;",
            "    }",
        ]
    )

    for page in pages:
        lines.append(
            f"    if (target == objects.{page.object_name}) {{ loadScreen({page.enum_name}); return true; }}"
        )

    lines.extend(
        [
            "    return false;",
            "}",
            "",
            "}  // namespace demo",
            "",
        ]
    )

    return "\n".join(lines)


def write_file(path: Path, content: str) -> None:
    header = (
        "// АВТОСГЕНЕРИРОВАННЫЙ ФАЙЛ.\n"
        "// Источник: src/ui/screens.h + src/ui/screens.c\n"
        "// НЕ РЕДАКТИРОВАТЬ ВРУЧНУЮ. Запуск: python tools/ui_meta_gen/generate_ui_meta.py\n\n"
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(header + content, encoding="utf-8", newline="\n")


def generate() -> None:
    screens_h = read_text(SCREENS_H)
    screens_c = read_text(SCREENS_C)

    pages, objects = parse_pages_and_objects(screens_h)
    assignments = parse_object_assignments(screens_c, pages)

    page_object_names = {page.object_name for page in pages}
    element_order = [name for name in objects if name not in page_object_names]

    missing = [name for name in element_order if name not in assignments]
    if missing:
        raise RuntimeError(
            "Could not resolve page mapping for objects from screens.c: " + ", ".join(missing)
        )

    used_ids = {page.page_id for page in pages}
    element_ids = {name: stable_element_id(name, used_ids) for name in element_order}

    write_file(OUT_DIR / "page_ids.generated.h", render_page_ids_header(pages))
    write_file(OUT_DIR / "element_ids.generated.h", render_element_ids_header(element_order, element_ids))
    write_file(OUT_DIR / "page_descriptors.generated.h", render_page_descriptors_header(pages))
    write_file(
        OUT_DIR / "element_descriptors.generated.h",
        render_element_descriptors_header(element_order, assignments, element_ids),
    )
    write_file(OUT_DIR / "ui_object_map.generated.h", render_ui_object_map_header())
    write_file(
        OUT_DIR / "ui_object_map.generated.cpp",
        render_ui_object_map_cpp(pages, element_order, assignments, element_ids),
    )
    write_file(OUT_DIR / "eez_page_meta.generated.cpp", render_eez_page_meta_cpp(pages))

    print(f"Generated UI meta files in: {OUT_DIR}")
    print(f"Pages: {len(pages)}, Elements: {len(element_order)}")


if __name__ == "__main__":
    generate()
