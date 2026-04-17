Import("env")
import os
import shutil
import subprocess
import sys

WEB_BUILD_DIR_NAME = "build_web"


def resolve_screenui_root(project_dir):
    candidates = [
        os.path.join(project_dir, "lib", "ScreenUI"),
    ]
    for candidate in candidates:
        marker = os.path.join(candidate, "tools", "ui_meta_gen", "generate_ui_meta.py")
        if os.path.isfile(marker):
            return candidate
    return None


def resolve_screenlib_root(project_dir):
    candidates = [
        os.path.join(project_dir, "lib", "screenLIB"),
    ]
    for candidate in candidates:
        marker = os.path.join(candidate, "lib", "core", "src", "bridge", "ScreenBridge.cpp")
        if os.path.isfile(marker):
            return candidate
    return None


def configure_frontend_dependency_paths(project_dir, screenui_root, screenlib_root):
    nanopb_dir = os.path.join(env.subst("$PROJECT_LIBDEPS_DIR"), env.subst("$PIOENV"), "Nanopb")
    include_paths = [
        os.path.join(project_dir, "src"),
        project_dir,
        os.path.join(screenlib_root, "lib", "core", "src"),
        os.path.join(screenlib_root, "lib", "client", "src"),
        os.path.join(screenlib_root, "lib", "adapter", "src"),
        screenui_root,
        os.path.join(screenui_root, "eez_project", "src"),
        os.path.join(screenui_root, "eez_project", "src", "ui"),
        os.path.join(screenui_root, "generated", "shared"),
        os.path.join(screenui_root, "generated", "frontend_meta"),
        os.path.join(screenui_root, "adapter"),
        os.path.join(screenui_root, "adapter", "lvgl_eez"),
        os.path.join(screenui_root, "vendor"),
        os.path.join(screenui_root, "vendor", "lvgl"),
        os.path.join(screenui_root, "vendor", "lvgl", "src"),
        nanopb_dir,
    ]
    env.AppendUnique(CPPPATH=include_paths)

    # Compile ScreenUI-owned frontend sources as part of Screen32 integration build.
    env.BuildSources(
        os.path.join("$BUILD_DIR", "screenui_frontend"),
        os.path.join(screenui_root, "eez_project", "src", "ui"),
    )
    env.BuildSources(
        os.path.join("$BUILD_DIR", "screenui_frontend_meta"),
        os.path.join(screenui_root, "generated", "frontend_meta"),
    )
    env.BuildSources(
        os.path.join("$BUILD_DIR", "screenui_adapter"),
        os.path.join(screenui_root, "adapter", "lvgl_eez"),
    )


def run_ui_meta_generator():
    project_dir = env.subst("$PROJECT_DIR")
    screenui_root = resolve_screenui_root(project_dir)
    if not screenui_root:
        print("\n[ERROR] ScreenUI dependency not found (expected lib/ScreenUI submodule)")
        return 1

    generator_script = os.path.join(screenui_root, "tools", "ui_meta_gen", "generate_ui_meta.py")
    python_cmd = env.subst("$PYTHONEXE") or sys.executable

    print("\n[GEN] Running ScreenUI meta generator...")
    result = subprocess.run(
        [python_cmd, generator_script],
        cwd=screenui_root,
        capture_output=False,
        text=True,
    )
    if result.returncode != 0:
        print("[GEN] UI meta generation failed")
        return result.returncode
    return 0


if run_ui_meta_generator() != 0:
    raise SystemExit(1)

project_dir = env.subst("$PROJECT_DIR")
screenui_root = resolve_screenui_root(project_dir)
screenlib_root = resolve_screenlib_root(project_dir)
if not screenui_root or not screenlib_root:
    print("\n[ERROR] screenLIB or ScreenUI dependency not found (expected lib/screenLIB and lib/ScreenUI submodules)")
    raise SystemExit(1)
configure_frontend_dependency_paths(project_dir, screenui_root, screenlib_root)


def build_web_action(source, target, env):
    demo_root = env.subst("$PROJECT_DIR")
    build_dir = os.path.join(demo_root, "demo_web", WEB_BUILD_DIR_NAME)
    source_dir = os.path.join(demo_root, "demo_web")
    lv_conf_path = os.path.join(source_dir, "lv_conf.h")
    emsdk_root = os.environ.get("EMSDK", os.path.normpath(os.path.join(demo_root, "..", "emsdk")))

    emcmake_cmd = shutil.which("emcmake")
    if emcmake_cmd is None:
        fallback = os.path.join(emsdk_root, "upstream", "emscripten", "emcmake.bat")
        if os.path.isfile(fallback):
            emcmake_cmd = fallback
    if emcmake_cmd is None:
        print("\n[ERROR] emcmake not found. Please activate emsdk environment first.")
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
        text=True,
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
        text=True,
    )
    if result.returncode != 0:
        print("[WEB] build failed")
        return 1

    print(f"\n[WEB] Build successful! Output in demo_web/{WEB_BUILD_DIR_NAME}/")
    print(f"[WEB] Serve with: python -m http.server 8080 --directory demo_web/{WEB_BUILD_DIR_NAME}")
    return 0


env.AddPlatformTarget(
    "build_web",
    [],
    build_web_action,
    "Build WebAssembly demo",
)
