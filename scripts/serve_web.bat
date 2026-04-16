@echo off
setlocal

set "_SCRIPT_DIR=%~dp0"
for %%I in ("%_SCRIPT_DIR%..") do set "_PROJECT_ROOT=%%~fI"
set "_WEB_DIR=%_PROJECT_ROOT%\demo_web\build"

if not exist "%_WEB_DIR%" (
    echo [serve_web] ERROR: build output not found:
    echo            %_WEB_DIR%
    echo            Run scripts\build_web.bat first.
    exit /b 1
)
if not exist "%_WEB_DIR%\demo_web.html" (
    echo [serve_web] ERROR: demo_web.html not found:
    echo            %_WEB_DIR%\demo_web.html
    echo            Run scripts\build_web.bat first.
    exit /b 1
)
if not exist "%_WEB_DIR%\demo_web.js" (
    echo [serve_web] ERROR: demo_web.js not found:
    echo            %_WEB_DIR%\demo_web.js
    echo            Rebuild: scripts\build_web.bat
    exit /b 1
)

pushd "%_PROJECT_ROOT%"
if errorlevel 1 (
    echo [serve_web] ERROR: cannot enter project root.
    exit /b 1
)

where py >nul 2>&1
if not errorlevel 1 (
    echo [serve_web] Serving on http://localhost:8080/demo_web.html
    py -3 -m http.server 8080 --directory "%_WEB_DIR%"
    set "_RC=%ERRORLEVEL%"
    popd
    exit /b %_RC%
)

where python >nul 2>&1
if not errorlevel 1 (
    echo [serve_web] Serving on http://localhost:8080/demo_web.html
    python -m http.server 8080 --directory "%_WEB_DIR%"
    set "_RC=%ERRORLEVEL%"
    popd
    exit /b %_RC%
)

popd
echo [serve_web] ERROR: Python not found (py/python).
exit /b 1
