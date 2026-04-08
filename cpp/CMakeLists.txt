cmake_minimum_required(VERSION 3.25)
project(square-ar-game VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# MSVC warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# ── Dependencies via vcpkg ─────────────────────────────────────────────
find_package(OpenCV REQUIRED COMPONENTS core imgproc aruco calib3d)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

# ONNX Runtime — find via CMake config or manual path
find_package(onnxruntime CONFIG QUIET)
if(NOT onnxruntime_FOUND)
    find_library(ONNXRUNTIME_LIB onnxruntime)
    find_path(ONNXRUNTIME_INCLUDE onnxruntime_cxx_api.h PATH_SUFFIXES onnxruntime)
endif()

# DirectXTK (Windows only)
if(WIN32)
    find_package(directxtk CONFIG REQUIRED)
endif()

# ── Source files ───────────────────────────────────────────────────────
set(MODEL_SOURCES
    src/models/grid_model.cpp
)

set(SERVICE_SOURCES
    src/services/round_evaluator.cpp
    src/services/floor_mapping_service.cpp
    src/services/detection_deduplicator.cpp
    src/services/player_tracker_service.cpp
    src/services/game_engine_service.cpp
    src/services/calibration_service.cpp
    src/services/camera_service.cpp
    src/services/pose_tracking_service.cpp
)

# Platform-dependent rendering services (Windows/D3D11 only)
set(RENDER_SERVICE_SOURCES
    src/services/overlay_render_service.cpp
    src/services/debug_render_service.cpp
)

set(UTIL_SOURCES
    src/utils/config.cpp
)

set(UI_SOURCES
    src/ui/d3d_window.cpp
    src/ui/operator_window.cpp
    src/ui/projector_window.cpp
    src/ui/debug_window.cpp
)

set(APP_SOURCES
    src/app/application.cpp
    src/main.cpp
)

# ── Core library (models + services + utils — no UI, no platform I/O) ─
add_library(square_ar_core STATIC
    ${MODEL_SOURCES}
    ${SERVICE_SOURCES}
    ${UTIL_SOURCES}
)
target_include_directories(square_ar_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(square_ar_core PUBLIC
    ${OpenCV_LIBS}
    nlohmann_json::nlohmann_json
)
if(onnxruntime_FOUND)
    target_link_libraries(square_ar_core PUBLIC onnxruntime::onnxruntime)
elseif(ONNXRUNTIME_LIB)
    target_link_libraries(square_ar_core PUBLIC ${ONNXRUNTIME_LIB})
    if(ONNXRUNTIME_INCLUDE)
        target_include_directories(square_ar_core PUBLIC ${ONNXRUNTIME_INCLUDE})
    endif()
endif()

# ── Executable ─────────────────────────────────────────────────────────
if(WIN32)
    add_executable(square_ar_game WIN32
        ${APP_SOURCES}
        ${UI_SOURCES}
        ${RENDER_SERVICE_SOURCES}
        src/utils/d3d_helpers.cpp
        src/utils/d3d_renderer.cpp
    )
    target_link_libraries(square_ar_game PRIVATE
        square_ar_core
        imgui::imgui
        d3d11 dxgi d3dcompiler
        mf mfplat mfreadwrite mfuuid
        Microsoft::DirectXTK
    )
    target_include_directories(square_ar_game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
endif()

# ── Tests ──────────────────────────────────────────────────────────────
enable_testing()
add_subdirectory(tests)
