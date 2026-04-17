#!/usr/bin/env bash

# Файл build_web.sh
# Назначение: локальная сборка web-версии через emcmake/cmake.

set -euo pipefail

DEMO_ROOT="$(cd "$(dirname "$0")" && pwd)"
EMSDK_ROOT="${EMSDK_ROOT:-$DEMO_ROOT/../emsdk}"
BUILD_DIR="$DEMO_ROOT/demo_web/build_web"

if [ -f "$EMSDK_ROOT/emsdk_env.sh" ]; then
    # Подключаем окружение emsdk, если скрипт доступен рядом с проектом.
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

echo "[WEB] Configuring CMake (LVGL is linked as full CMake library)..."
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
