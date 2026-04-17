cmake_minimum_required(VERSION 3.13)
project(demo_web LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(DEMO_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
set(SRC_DIR ${DEMO_ROOT}/src)
set(COMMON_APP_DIR ${SRC_DIR}/common_app)
set(WEB_PLATFORM_DIR ${SRC_DIR}/platform_web)

set(SCREENLIB_ROOT ${DEMO_ROOT}/lib/screenLIB)

set(SCREENUI_ROOT ${DEMO_ROOT}/lib/ScreenUI)

set(SCREENLIB_CORE_DIR ${SCREENLIB_ROOT}/lib/core/src)
set(SCREENLIB_CLIENT_DIR ${SCREENLIB_ROOT}/lib/client/src)
set(SCREENLIB_ADAPTER_API_DIR ${SCREENLIB_ROOT}/lib/adapter/src)

set(SCREENUI_EEZ_SRC_DIR ${SCREENUI_ROOT}/eez_project/src)
set(SCREENUI_UI_DIR ${SCREENUI_EEZ_SRC_DIR}/ui)
set(SCREENUI_SHARED_DIR ${SCREENUI_ROOT}/generated/shared)
set(SCREENUI_FRONTEND_META_DIR ${SCREENUI_ROOT}/generated/frontend_meta)
set(SCREENUI_ADAPTER_DIR ${SCREENUI_ROOT}/adapter)
set(LVGL_DIR ${SCREENUI_ROOT}/vendor/lvgl)

if(NOT EXISTS ${LVGL_DIR}/CMakeLists.txt)
    message(FATAL_ERROR "LVGL directory not found at ${LVGL_DIR}")
endif()

if(NOT EXISTS ${SCREENLIB_CORE_DIR}/bridge/ScreenBridge.cpp)
    message(FATAL_ERROR "screenLIB core not found at ${SCREENLIB_CORE_DIR}")
endif()

if(NOT EXISTS ${SCREENUI_FRONTEND_META_DIR}/ui_object_map.generated.cpp)
    message(FATAL_ERROR "ScreenUI generated frontend meta not found at ${SCREENUI_FRONTEND_META_DIR}")
endif()
if(NOT EXISTS ${SCREENUI_UI_DIR}/screens.c)
    message(FATAL_ERROR "ScreenUI UI sources not found at ${SCREENUI_UI_DIR}")
endif()

set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE PATH "Path to lv_conf.h" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)

add_subdirectory(${LVGL_DIR} ${CMAKE_BINARY_DIR}/lvgl_build)

if(EMSCRIPTEN AND TARGET lvgl)
    target_compile_options(lvgl PRIVATE
        -sUSE_SDL=2
    )
endif()

include(FetchContent)
FetchContent_Declare(
    nanopb
    GIT_REPOSITORY https://github.com/nanopb/nanopb.git
    GIT_TAG nanopb-0.4.9.1
)
FetchContent_MakeAvailable(nanopb)

set(APP_SOURCES
    ${COMMON_APP_DIR}/app_core.cpp
    ${COMMON_APP_DIR}/frontend_config.cpp
    ${COMMON_APP_DIR}/frontend_runtime.cpp
    ${COMMON_APP_DIR}/frontend_service_responder.cpp
    ${COMMON_APP_DIR}/frontend_ui_events.cpp
    ${COMMON_APP_DIR}/offline_demo_controller.cpp
    ${COMMON_APP_DIR}/page_state_builder.cpp
    ${COMMON_APP_DIR}/shared_app.cpp
    ${WEB_PLATFORM_DIR}/platform.cpp
    ${WEB_PLATFORM_DIR}/web_main.cpp
    ${SCREENUI_FRONTEND_META_DIR}/ui_object_map.generated.cpp
    ${SCREENUI_FRONTEND_META_DIR}/eez_page_meta.generated.cpp
    ${SCREENUI_ADAPTER_DIR}/lvgl_eez/EezLvglAdapter.cpp
    ${SCREENUI_ADAPTER_DIR}/lvgl_eez/UiObjectMap.cpp
    ${SCREENLIB_CORE_DIR}/bridge/ScreenBridge.cpp
    ${SCREENLIB_CORE_DIR}/frame/FrameCodec.cpp
    ${SCREENLIB_CORE_DIR}/proto/ProtoCodec.cpp
    ${SCREENLIB_CORE_DIR}/proto/machine.pb.c
    ${SCREENLIB_CLIENT_DIR}/runtime/CommandDispatcher.cpp
    ${SCREENLIB_CLIENT_DIR}/runtime/ScreenClient.cpp
    ${SCREENLIB_CLIENT_DIR}/link/WebSocketClientLink.cpp
    ${nanopb_SOURCE_DIR}/pb_common.c
    ${nanopb_SOURCE_DIR}/pb_decode.c
    ${nanopb_SOURCE_DIR}/pb_encode.c
)

set(UI_SOURCES
    ${SCREENUI_UI_DIR}/ui.c
    ${SCREENUI_UI_DIR}/screens.c
    ${SCREENUI_UI_DIR}/images.c
    ${SCREENUI_UI_DIR}/styles.c
    ${SCREENUI_UI_DIR}/ui_font_m_18.c
    ${SCREENUI_UI_DIR}/ui_font_m_20.c
    ${SCREENUI_UI_DIR}/ui_font_m_24.c
    ${SCREENUI_UI_DIR}/ui_font_m_70.c
)

add_executable(demo_web ${APP_SOURCES} ${UI_SOURCES})

target_include_directories(demo_web PRIVATE
    ${SRC_DIR}
    ${COMMON_APP_DIR}
    ${WEB_PLATFORM_DIR}
    ${SCREENLIB_CORE_DIR}
    ${SCREENLIB_CLIENT_DIR}
    ${SCREENLIB_ADAPTER_API_DIR}
    ${SCREENUI_ROOT}
    ${SCREENUI_ROOT}/vendor
    ${SCREENUI_EEZ_SRC_DIR}
    ${SCREENUI_SHARED_DIR}
    ${SCREENUI_FRONTEND_META_DIR}
    ${SCREENUI_ADAPTER_DIR}
    ${nanopb_SOURCE_DIR}
)

target_link_libraries(demo_web PRIVATE lvgl::lvgl)

if(EMSCRIPTEN)
    target_compile_options(demo_web PRIVATE
        -O2
        -Wno-unused-parameter
    )

    target_link_options(demo_web PRIVATE
        -O2
        -sUSE_SDL=2
        -sSINGLE_FILE=1
        -lwebsocket.js
        -sALLOW_MEMORY_GROWTH=1
        -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
        -sENVIRONMENT=web
        --preload-file
        ${CMAKE_CURRENT_SOURCE_DIR}/frontend_config.json@/frontend_config.json
        --shell-file
        ${CMAKE_CURRENT_SOURCE_DIR}/shell.html
    )

    set_target_properties(demo_web PROPERTIES SUFFIX ".html")
endif()
