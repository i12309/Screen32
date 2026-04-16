# Файл extra_script.py
# Назначение: добавляет в PlatformIO пользовательскую цель `build_web`,
# которая запускает CMake-сборку web-версии.
# Использование: pio run -t build_web

Import("env")
import os
import subprocess
import shutil

def build_web_action(source, target, env):
    # Точка входа кастомной цели PlatformIO.
    # Готовит команды emcmake/cmake и запускает сборку web-артефактов.
    demo_root = env.subst("$PROJECT_DIR")
    build_dir = os.path.join(demo_root, "demo_web", "build")
    source_dir = os.path.join(demo_root, "demo_web")
    lv_conf_path = os.path.join(source_dir, "lv_conf.h")
    emsdk_root = os.environ.get("EMSDK", os.path.normpath(os.path.join(demo_root, "..", "emsdk")))

    emcmake_cmd = shutil.which("emcmake")
    if emcmake_cmd is None:
        fallback = os.path.join(emsdk_root, "upstream", "emscripten", "emcmake.bat")
        if os.path.isfile(fallback):
            emcmake_cmd = fallback
    if emcmake_cmd is None:
        print("\n[ERROR] emcmake not found. Please activate emsdk environment first:")
        print("  Call emsdk_env.bat  (Windows)")
        print("  or source emsdk_env.sh  (Linux/macOS)")
        return 1
    cmake_cmd = shutil.which("cmake")
    if cmake_cmd is None:
        cmake_fallback = os.path.join(emsdk_root, "..", "cmake-4.3.1-windows-x86_64", "bin", "cmake.exe")
        cmake_fallback = os.path.normpath(cmake_fallback)
        if os.path.isfile(cmake_fallback):
            cmake_cmd = cmake_fallback
    if cmake_cmd is None:
        print("\n[ERROR] cmake not found in PATH.")
        return 1

    os.makedirs(build_dir, exist_ok=True)
    child_env = os.environ.copy()
    cmake_dir = os.path.dirname(cmake_cmd)
    ninja_dir = os.path.join(os.path.expanduser("~"), "AppData", "Roaming", "Python", "Python313", "Scripts")
    path_parts = [cmake_dir]
    if os.path.isfile(os.path.join(ninja_dir, "ninja.exe")):
        path_parts.append(ninja_dir)
    path_parts.append(child_env.get("PATH", ""))
    child_env["PATH"] = os.pathsep.join(path_parts)

    print("\n[WEB] Running emcmake cmake configure...")
    result = subprocess.run(
        [
            emcmake_cmd,
            "cmake",
            "-S", source_dir,
            "-B", build_dir,
            "-G", "Ninja",
            "-D", "CMAKE_BUILD_TYPE=Release",
            "-D", f"LV_BUILD_CONF_PATH={lv_conf_path}",
        ],
        cwd=demo_root,
        env=child_env,
        capture_output=False,
        text=True
    )
    if result.returncode != 0:
        print("[WEB] cmake configure failed")
        return 1

    print("\n[WEB] Building...")
    result = subprocess.run(
        [cmake_cmd, "--build", build_dir, "--parallel"],
        cwd=demo_root,
        env=child_env,
        capture_output=False,
        text=True
    )
    if result.returncode != 0:
        print("[WEB] build failed")
        return 1

    print("\n[WEB] Build successful! Output in demo_web/build/")
    print("[WEB] Serve with: python -m http.server 8080 --directory demo_web/build")
    return 0

# Регистрируем пользовательскую цель в SCons/PlatformIO.
env.AddPlatformTarget(
    "build_web",
    [],
    build_web_action,
    "Build WebAssembly demo"
)
