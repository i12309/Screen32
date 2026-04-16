#!/usr/bin/env python3
"""Compatibility wrapper for UI meta generator tool.

Canonical entrypoint:
    python tools/ui_meta_gen/generate_ui_meta.py
"""

from __future__ import annotations

import runpy
from pathlib import Path


def main() -> None:
    repo_root = Path(__file__).resolve().parents[1]
    tool_script = repo_root / "tools" / "ui_meta_gen" / "generate_ui_meta.py"
    if not tool_script.is_file():
        raise FileNotFoundError(f"UI meta tool entrypoint not found: {tool_script}")
    runpy.run_path(str(tool_script), run_name="__main__")


if __name__ == "__main__":
    main()
