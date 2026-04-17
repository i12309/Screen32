@echo off
setlocal

set "_SCRIPT_DIR=%~dp0"
for %%I in ("%_SCRIPT_DIR%..") do set "_PROJECT_ROOT=%%~fI"

call "%_SCRIPT_DIR%env_web.bat"
if errorlevel 1 (
    echo [build_web] ERROR: env setup failed.
    exit /b 1
)

pushd "%_PROJECT_ROOT%"
if errorlevel 1 (
    echo [build_web] ERROR: cannot enter project root:
    echo             %_PROJECT_ROOT%
    exit /b 1
)

set "_BUILD_DIR=%_PROJECT_ROOT%\demo_web\build_web"
set "_CACHE_FILE=%_BUILD_DIR%\CMakeCache.txt"
set "_ROOT_FWD=%_PROJECT_ROOT:\=/%"

if exist "%_CACHE_FILE%" (
    findstr /I /C:"CMAKE_HOME_DIRECTORY:INTERNAL=%_PROJECT_ROOT%\demo_web" /C:"CMAKE_HOME_DIRECTORY:INTERNAL=%_ROOT_FWD%/demo_web" "%_CACHE_FILE%" >nul
    if errorlevel 1 (
        echo [build_web] Detected stale CMake cache from another workspace path.
        echo [build_web] Cleaning demo_web\build_web ...
        if exist "%_BUILD_DIR%" rmdir /s /q "%_BUILD_DIR%"
    )
)

echo [build_web] Running: pio run -t build_web
pio run -t build_web
set "_BUILD_RC=%ERRORLEVEL%"

popd

if not "%_BUILD_RC%"=="0" (
    echo [build_web] FAILED with code %_BUILD_RC%.
    exit /b %_BUILD_RC%
)

echo [build_web] OK. Run local preview:
echo             scripts\serve_web.bat
echo             or ^(from project root^) python -m http.server 8080 --directory demo_web/build_web
echo             Open: http://localhost:8080/demo_web.html

exit /b 0
