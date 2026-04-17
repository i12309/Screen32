#!/usr/bin/env python3
"""Compatibility wrapper that runs ScreenUI meta generator."""

from __future__ import annotations

from pathlib import Path
import runpy


def resolve_screenui_root(project_root: Path) -> Path:
    candidates = [project_root / "lib" / "ScreenUI"]
    for candidate in candidates:
        if (candidate / "tools" / "ui_meta_gen" / "generate_ui_meta.py").is_file():
            return candidate
    raise FileNotFoundError("ScreenUI dependency not found (expected lib/ScreenUI submodule)")


def main() -> None:
    project_root = Path(__file__).resolve().parents[1]
    screenui_root = resolve_screenui_root(project_root)
    tool_script = screenui_root / "tools" / "ui_meta_gen" / "generate_ui_meta.py"
    runpy.run_path(str(tool_script), run_name="__main__")


if __name__ == "__main__":
    main()
