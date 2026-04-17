#!/usr/bin/env bash

# File: build_web.sh
# Purpose: local web build via emcmake/cmake.

set -euo pipefail

DEMO_ROOT="$(cd "$(dirname "$0")" && pwd)"
EMSDK_ROOT="${EMSDK_ROOT:-$DEMO_ROOT/../emsdk}"
BUILD_DIR="$DEMO_ROOT/demo_web/build_web"

SCREENUI_ROOT="$DEMO_ROOT/lib/ScreenUI"

if [ ! -f "$SCREENUI_ROOT/tools/ui_meta_gen/generate_ui_meta.py" ]; then
    echo "[GEN] ERROR: ScreenUI submodule not found at $SCREENUI_ROOT"
    echo "[GEN] Run: git submodule update --init --recursive"
    exit 1
fi

echo "[GEN] Running ScreenUI UI-meta generator..."
python "$SCREENUI_ROOT/tools/ui_meta_gen/generate_ui_meta.py"

if [ -f "$EMSDK_ROOT/emsdk_env.sh" ]; then
    # Activate emsdk if available nearby.
    # shellcheck disable=SC1090
    source "$EMSDK_ROOT/emsdk_env.sh" >/dev/null
fi

if ! command -v emcmake >/dev/null 2>&1; then
    echo "[WEB] ERROR: emcmake not found. Activate emsdk environment first."
    echo "[WEB] Hint: source \"$EMSDK_ROOT/emsdk_env.sh\""
    exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
    CMAKE_CMD="$DEMO_ROOT/../cmake-4.3.1-windows-x86_64/bin/cmake.exe"
    if [ ! -x "$CMAKE_CMD" ]; then
        echo "[WEB] ERROR: cmake not found in PATH."
        exit 1
    fi
else
    CMAKE_CMD="$(command -v cmake)"
fi
export PATH="$(dirname "$CMAKE_CMD"):$PATH"
NINJA_DIR="$HOME/AppData/Roaming/Python/Python313/Scripts"
if [ -x "$NINJA_DIR/ninja.exe" ]; then
    export PATH="$NINJA_DIR:$PATH"
fi

echo "[WEB] Configuring CMake..."
"$CMAKE_CMD" -E make_directory "$BUILD_DIR"
emcmake cmake \
    -S "$DEMO_ROOT/demo_web" \
    -B "$BUILD_DIR" \
    -G Ninja \
    -D CMAKE_BUILD_TYPE=Release \
    -D LV_BUILD_CONF_PATH="$DEMO_ROOT/demo_web/lv_conf.h"

echo "[WEB] Building..."
"$CMAKE_CMD" --build "$BUILD_DIR" --parallel

echo "[WEB] Build successful! Output in demo_web/build_web/"
echo "[WEB] Open demo_web/build_web/demo_web.html in browser"
