cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(MiceCam VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Windows/MSVC specific configurations
if(MSVC)
    # Enable parallel compilation
    add_compile_options(/MP)
    # Disable C4996 warning (deprecated functions like std::fopen)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    # UTF-8 source encoding
    add_compile_options(/utf-8)
    # Disable min/max macros from Windows.h
    add_compile_definitions(NOMINMAX)
    # Bypass OpenCV dependency in DepthAI headers
    add_compile_definitions(DEPTHAI_TARGET_CORE)
endif()

# Ensure proper threading on all platforms
if(WIN32)
    # Windows threading is handled by the runtime
else()
    # Unix platforms need pthread
    set(THREADS_PREFER_PTHREAD_FLAG ON)
endif()

# Build options
option(BUILD_TESTS "Legacy compatibility toggle for MiceCam tests" OFF)
option(MICECAM_BUILD_TESTS "Build MiceCam test suite" OFF)
if(BUILD_TESTS AND NOT MICECAM_BUILD_TESTS)
    set(MICECAM_BUILD_TESTS ON)
endif()
option(BUILD_BENCHMARKS "Build performance benchmarks" ON)
option(BUILD_PYTHON_BINDINGS "Build Python bindings (requires pybind11)" OFF)
option(BUILD_GPU_PREVIEW "Build GPU preview module (requires CUDA)" OFF)

# Dependencies
find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)

# Standard CMake discovery
include(FetchContent)

# nlohmann_json - provided by vcpkg
find_package(nlohmann_json CONFIG REQUIRED)

# libnop (header-only, required by depthai-core headers) - handled by depthai-core internally

# Luxonis DepthAI Core (Using FetchContent)
option(WITH_OAK_CAMERA "Build with OAK camera support (requires depthai-core)" ON)
if(WITH_OAK_CAMERA)
    # The depthai-core submodule has HunterGate issues when added via add_subdirectory
    # because HunterGate expects to run BEFORE the first project() call.
    # To bypass this without modifying the submodule, we rely on FetchContent instead
    # which manages this more gracefully, or simply define hunter vars.
    # But since the submodule is in 3rdParty, we can just compile the library
    # directly using standard cmake without hunter if we disable its tests

    set(DEPTHAI_SUBMODULE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/depthai-core")

    # Disable Hunter and examples/tests
    set(HUNTER_ENABLED OFF CACHE BOOL "Disable Hunter package manager" FORCE)
    set(DEPTHAI_BUILD_EXAMPLES OFF CACHE BOOL "Disable depthai-core examples" FORCE)
    set(DEPTHAI_BUILD_TESTS OFF CACHE BOOL "Disable depthai-core tests" FORCE)

    # Disable depthai-core's internal vcpkg bootstrap (we use our own project-level vcpkg)
    set(DEPTHAI_BOOTSTRAP_VCPKG OFF CACHE BOOL "Disable depthai-core internal vcpkg" FORCE)

    # Use external dependencies (vcpkg)
    set(DEPTHAI_JSON_EXTERNAL ON CACHE BOOL "" FORCE)
    # Let depthai-core fetch libnop internally since the vcpkg port lacks CMake configs
    set(DEPTHAI_LIBNOP_EXTERNAL OFF CACHE BOOL "" FORCE)

    # Disable unused features to reduce dependency bloat
    set(DEPTHAI_ENABLE_CURL OFF CACHE BOOL "Disable depthai-core CURL support" FORCE)
    set(DEPTHAI_ENABLE_MP4V2 OFF CACHE BOOL "Disable depthai-core MP4V2 support" FORCE)
    set(DEPTHAI_ENABLE_APRIL_TAG OFF CACHE BOOL "Disable depthai-core AprilTag support" FORCE)
    set(DEPTHAI_ENABLE_PROTOBUF OFF CACHE BOOL "Disable depthai-core Protobuf support" FORCE)
    set(DEPTHAI_XTENSOR_SUPPORT ON CACHE BOOL "Enable depthai-core xtensor support" FORCE)
    set(DEPTHAI_ENABLE_REMOTE_CONNECTION OFF CACHE BOOL "Disable depthai-core remote connection" FORCE)

    # Ensure libusb for XLink
    set(XLINK_ENABLE_LIBUSB ON CACHE BOOL "" FORCE)
    set(XLINK_LIBUSB_SYSTEM ON CACHE BOOL "Use system libusb via vcpkg" FORCE)

    # Provide cpp-httplib via FetchContent (vcpkg download has hash mismatch)
    FetchContent_Declare(
        httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG v0.18.3
        GIT_SHALLOW TRUE
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(httplib)
    # Required for depthai-core's target export
    install(TARGETS httplib EXPORT depthaiDependencies)

    set(MICECAM_THIRDPARTY_BUILD_TESTS ${BUILD_TESTS})
    set(BUILD_TESTS OFF CACHE BOOL "Disable third-party test suites" FORCE)
    add_subdirectory(${DEPTHAI_SUBMODULE_DIR} ${CMAKE_BINARY_DIR}/depthai-core EXCLUDE_FROM_ALL)
    set(BUILD_TESTS ${MICECAM_THIRDPARTY_BUILD_TESTS} CACHE BOOL "Restore MiceCam test toggle" FORCE)

    set(DEPTHAI_FOUND ON)
endif()


# FFmpeg for native capture (Managed via vcpkg now)
find_package(PkgConfig QUIET)
find_package(FFMPEG QUIET COMPONENTS avcodec avformat avutil avdevice swscale)
if(NOT FFMPEG_FOUND)
    message(STATUS "Using vcpkg mapped FFMPEG targets")
    set(FFMPEG_FOUND ON)
    # vcpkg creates standard targets we can link against usually;
    # but we can also set the raw lib names for fallback.
    set(FFMPEG_LIBRARIES avcodec avformat avutil avdevice swscale)
endif()

set(WITH_CAMERA_BACKEND_DEFAULT OFF)
if(OpenCV_FOUND OR FFMPEG_FOUND OR DEPTHAI_FOUND)
    set(WITH_CAMERA_BACKEND_DEFAULT ON)
endif()
option(WITH_CAMERA_BACKEND "Build camera backend" ${WITH_CAMERA_BACKEND_DEFAULT})

# JSON library (header-only)
include(FetchContent)
# nlohmann_json - Moved earlier
# ... handled above ...

# libnop (header-only, required by depthai-core headers) - Moved earlier
if(WITH_OAK_CAMERA)
    # ... libnop FetchContent already handled above ...
endif()

message(STATUS "DEPTHAI_FOUND: ${DEPTHAI_FOUND}")
message(STATUS "FFMPEG_FOUND: ${FFMPEG_FOUND}")
message(STATUS "OpenCV_FOUND: ${OpenCV_FOUND}")

if(MICECAM_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    enable_testing()
endif()

# ----- Qt6 configuration -----
# Hint to find Qt6 installed via Homebrew when using vcpkg toolchain
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/qt")

find_package(Qt6 COMPONENTS Core Gui Qml Quick QuickControls2 Multimedia REQUIRED)
# Require C++17 or C++20 for Qt6 usually
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
# -----------------------------

# Core library (always built)
add_library(micecam_core
    internal/domain/frame.cpp
    internal/domain/ring_buffer.cpp
    internal/infrastructure/disk_writer.cpp
    internal/infrastructure/ingestion_pipeline.cpp
    internal/infrastructure/frame_dispatcher.cpp
    internal/infrastructure/hdf5_converter.cpp
    internal/infrastructure/decoder.cpp
)

target_include_directories(micecam_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/internal
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(micecam_core PUBLIC
    Threads::Threads
    nlohmann_json::nlohmann_json
    ${OpenCV_LIBS}
)

# Camera backend (optional)
if(WITH_CAMERA_BACKEND)
    set(CAMERA_SOURCES
        internal/infrastructure/camera_backend.cpp
        internal/infrastructure/ffmpeg_device_selector.cpp
    )

    if(FFMPEG_FOUND)
        list(APPEND CAMERA_SOURCES internal/infrastructure/ffmpeg_camera_backend.cpp)
    endif()

    if(DEPTHAI_FOUND)
        list(APPEND CAMERA_SOURCES internal/infrastructure/oak_camera_backend.cpp)
    endif()

    add_library(micecam_camera ${CAMERA_SOURCES})

    if(FFMPEG_FOUND)
        target_compile_definitions(micecam_camera PUBLIC WITH_FFMPEG)
        target_link_libraries(micecam_camera PUBLIC ${FFMPEG_LIBRARIES})
    endif()

    if(DEPTHAI_FOUND)
        target_compile_definitions(micecam_camera PUBLIC WITH_OAK_CAMERA)
        target_link_libraries(micecam_camera PUBLIC depthai::core)
    endif()


    target_include_directories(micecam_camera PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/api
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
    )

    target_link_libraries(micecam_camera PUBLIC micecam_core)

    if(OpenCV_FOUND)
        target_link_libraries(micecam_camera PUBLIC ${OpenCV_LIBS})
    endif()

    if(DEPTHAI_FOUND)
        target_link_libraries(micecam_camera PUBLIC depthai::core)
    endif()



    if(FFMPEG_FOUND)
        target_link_libraries(micecam_camera PUBLIC ${FFMPEG_LIBRARIES})
        target_compile_definitions(micecam_camera PRIVATE WITH_FFMPEG)
        if(FFMPEG_INCLUDE_DIRS)
            target_include_directories(micecam_camera PRIVATE ${FFMPEG_INCLUDE_DIRS})
        endif()
    endif()

    # OAK Camera Targets
    if(DEPTHAI_FOUND)
        if(APPLE)
            set(MAC_FRAMEWORKS "-framework CoreFoundation" "-framework IOKit")
            target_link_libraries(micecam_camera PUBLIC ${MAC_FRAMEWORKS})
        endif()
    if(MICECAM_BUILD_TESTS)
        add_executable(oak_camera_test tests/oak_camera_test.cpp)
        target_link_libraries(oak_camera_test PRIVATE micecam_core micecam_camera depthai::core)
        target_compile_definitions(oak_camera_test PRIVATE WITH_OAK_CAMERA)
            if(APPLE)
                target_link_libraries(oak_camera_test PRIVATE ${MAC_FRAMEWORKS})
            endif()
        endif()

        add_executable(oak_quad_recorder cmd/examples/oak_quad_recorder.cpp)
        target_link_libraries(oak_quad_recorder PRIVATE micecam_core micecam_camera depthai::core)
        target_compile_definitions(oak_quad_recorder PRIVATE WITH_OAK_CAMERA)
        if(APPLE)
            target_link_libraries(oak_quad_recorder PRIVATE ${MAC_FRAMEWORKS})
        endif()

        add_executable(oak_diagnostic cmd/examples/oak_diagnostic.cpp)
        target_link_libraries(oak_diagnostic PRIVATE depthai::core micecam_core ${OpenCV_LIBS})
        if(APPLE)
            target_link_libraries(oak_diagnostic PRIVATE ${MAC_FRAMEWORKS})
        endif()
    endif()

    # FFmpeg camera test
    if(MICECAM_BUILD_TESTS)
        add_executable(ffmpeg_camera_test tests/ffmpeg_camera_test.cpp)
        target_link_libraries(ffmpeg_camera_test PRIVATE micecam_core micecam_camera)
        target_compile_definitions(ffmpeg_camera_test PRIVATE WITH_FFMPEG)
    endif()
endif()

# Tests
if(MICECAM_BUILD_TESTS)
    add_executable(worker_protocol_hang_helper
        tests/ui/worker_protocol_hang_helper.cpp
    )

    add_executable(micecam_tests
        tests/core/frame_test.cpp
        tests/core/ring_buffer_test.cpp
        tests/camera/fake_camera.cpp
        tests/camera/ffmpeg_device_selector_test.cpp
        tests/camera/oak_device_selector_test.cpp
        tests/pipeline/pipeline_test.cpp
        tests/pipeline/integration_test.cpp
        tests/pipeline/disk_writer_stream_test.cpp
        tests/pipeline/observer_test.cpp
        tests/pipeline/hdf5_test.cpp
        tests/pipeline/configurable_buffer_test.cpp
        tests/benchmark/stress_test.cpp
        tests/ui/native_app_preflight_test.cpp
        tests/ui/pipeline_controller_test.cpp
        tests/ui/recording_supervisor_service_test.cpp
        tests/ui/worker_process_runtime_test.cpp
        cmd/micecam_ui/ActivityEventModel.cpp
        cmd/micecam_ui/PipelineController.cpp
        cmd/micecam_ui/RecordingSupervisorService.cpp
        cmd/micecam_ui/RecordingSetup.cpp
        cmd/micecam_ui/CameraInventoryModel.cpp
        cmd/micecam_ui/VideoFrameProvider.cpp
        cmd/micecam_ui/WorkerProcessRuntime.cpp
        internal/infrastructure/ffmpeg_device_selector.cpp
    )

    target_include_directories(micecam_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/api
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    target_link_libraries(micecam_tests
        GTest::gtest
        GTest::gtest_main
        micecam_core
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        Qt6::Multimedia
    )

    add_dependencies(micecam_tests worker_protocol_hang_helper)

    include(GoogleTest)
    gtest_discover_tests(micecam_tests)
endif()

# Main executable
add_executable(micecam
    cmd/micecam/main.cpp
)

target_link_libraries(micecam PRIVATE micecam_core)
if(WITH_CAMERA_BACKEND)
    target_link_libraries(micecam PRIVATE micecam_camera)
endif()

# Qt QML UI executable
add_executable(micecam_ui
    cmd/micecam_ui/main.cpp
    cmd/micecam_ui/ActivityEventModel.cpp
    cmd/micecam_ui/NativeWorkerRuntime.cpp
    cmd/micecam_ui/PipelineController.cpp
    cmd/micecam_ui/RecordingSupervisorService.cpp
    cmd/micecam_ui/VideoFrameProvider.cpp
    cmd/micecam_ui/WorkerProcessRuntime.cpp
    cmd/micecam_ui/RecordingSetup.cpp
    cmd/micecam_ui/CameraInventoryModel.cpp
    cmd/micecam_ui/qml/qml.qrc
)

target_link_libraries(micecam_ui PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
    micecam_core
)

if(WITH_CAMERA_BACKEND)
    target_link_libraries(micecam_ui PRIVATE micecam_camera)
endif()



# Installation


# Installation
# SDK Packaging Configuration
if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    set(BUILD_SHARED_LIBS ON)
endif()

# Installation Rules
include(GNUInstallDirs)

install(TARGETS micecam_core
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY api/micecam DESTINATION include)

if(WITH_CAMERA_BACKEND)
    install(TARGETS micecam_camera
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
    )
endif()

# Install Tools (Recorder only - oak_diagnostic is not built by default)
if(WITH_OAK_CAMERA AND TARGET oak_quad_recorder)
    install(TARGETS oak_quad_recorder
        RUNTIME DESTINATION bin
    )
endif()

# Main executable
install(TARGETS micecam DESTINATION bin)

# Python bindings (optional)
if(BUILD_PYTHON_BINDINGS)
    add_subdirectory(bindings/python)
endif()

# GPU preview module (optional)
if(BUILD_GPU_PREVIEW)
    add_subdirectory(src/gpu)
endif()
