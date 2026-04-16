@echo off
REM Activate local web build toolchain (emsdk + cmake) for Screen32.
REM Priority:
REM   1) SCREEN32_EMSDK_DIR / SCREEN32_CMAKE_DIR env vars
REM   2) user-local defaults under C:\Users\sign\CODE\Emsdk

if defined SCREEN32_EMSDK_DIR (
    set "_SCREEN32_EMSDK_DIR=%SCREEN32_EMSDK_DIR%"
) else (
    set "_SCREEN32_EMSDK_DIR=C:\Users\sign\CODE\Emsdk\emsdk"
)

if defined SCREEN32_CMAKE_DIR (
    set "_SCREEN32_CMAKE_DIR=%SCREEN32_CMAKE_DIR%"
) else (
    set "_SCREEN32_CMAKE_DIR=C:\Users\sign\CODE\Emsdk\cmake-4.3.1-windows-x86_64"
)

set "_EMSDK_ENV_BAT=%_SCREEN32_EMSDK_DIR%\emsdk_env.bat"

if not exist "%_EMSDK_ENV_BAT%" (
    echo [env_web] ERROR: emsdk_env.bat not found:
    echo            %_EMSDK_ENV_BAT%
    exit /b 1
)

echo [env_web] Activating emsdk from:
echo           %_SCREEN32_EMSDK_DIR%
call "%_EMSDK_ENV_BAT%"
if errorlevel 1 (
    echo [env_web] ERROR: failed to activate emsdk environment.
    exit /b 1
)

if exist "%_SCREEN32_CMAKE_DIR%\bin\cmake.exe" (
    echo [env_web] Using CMake from:
    echo           %_SCREEN32_CMAKE_DIR%\bin
    set "PATH=%_SCREEN32_CMAKE_DIR%\bin;%PATH%"
) else (
    echo [env_web] WARNING: local CMake dir not found:
    echo            %_SCREEN32_CMAKE_DIR%\bin
    echo            will rely on cmake from current PATH.
)

where emcmake >nul 2>&1
if errorlevel 1 (
    echo [env_web] ERROR: emcmake not found in PATH.
    exit /b 1
)

where emmake >nul 2>&1
if errorlevel 1 (
    echo [env_web] ERROR: emmake not found in PATH.
    exit /b 1
)

where cmake >nul 2>&1
if errorlevel 1 (
    echo [env_web] ERROR: cmake not found in PATH.
    exit /b 1
)

echo [env_web] Tool checks:
where emcmake
where emmake
where cmake
echo [env_web] Versions:
call emcmake --version >nul 2>&1
if errorlevel 1 (
    echo [env_web] emcmake wrapper detected ^(version output is not available^).
) else (
    call emcmake --version
)
cmake --version

exit /b 0
