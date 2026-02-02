cmake_minimum_required(VERSION 3.20)
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
endif()

# Ensure proper threading on all platforms
if(WIN32)
    # Windows threading is handled by the runtime
else()
    # Unix platforms need pthread
    set(THREADS_PREFER_PTHREAD_FLAG ON)
endif()

# Build options
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_BENCHMARKS "Build performance benchmarks" ON)

# Dependencies
find_package(Threads REQUIRED)
find_package(OpenCV QUIET)

# Luxonis DepthAI Core
option(WITH_OAK_CAMERA "Build with OAK camera support (requires depthai-core)" ON)
if(WITH_OAK_CAMERA)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/depthai-core/CMakeLists.txt")
        set(DEPTHAI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(DEPTHAI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(DEPTHAI_BUILD_BINDINGS OFF CACHE BOOL "" FORCE)
        set(DEPTHAI_USE_VCPKG OFF CACHE BOOL "" FORCE)
        add_subdirectory(3rdParty/depthai-core)
        set(DEPTHAI_FOUND ON)
    else()
        message(WARNING "depthai-core not found in 3rdParty directory. OAK camera support will be disabled.")
        set(DEPTHAI_FOUND OFF)
    endif()
endif()


# FFmpeg for native capture (Plan C)
find_package(PkgConfig QUIET)
find_package(FFMPEG QUIET COMPONENTS avcodec avformat avutil avdevice swscale)
if(NOT FFMPEG_FOUND)
    set(FFMPEG_LIBRARIES avcodec avformat avutil avdevice swscale)
endif()

if(NOT OpenCV_FOUND AND NOT FFMPEG_FOUND)
    option(WITH_CAMERA_BACKEND "Build camera backend" OFF)
else()
    option(WITH_CAMERA_BACKEND "Build camera backend" ON)
endif()

# DepthAI (Disabled for now to avoid vcpkg fetch issues)
set(WITH_DEPTHAI OFF)
if(WITH_DEPTHAI)
    find_package(depthai CONFIG QUIET)
endif()

# JSON library (header-only)
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

if(BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    enable_testing()
endif()

# Core library (always built)
add_library(micecam_core
    src/core/frame.cpp
    src/core/ring_buffer.cpp
    src/pipeline/disk_writer.cpp
    src/pipeline/ingestion_pipeline.cpp
    src/pipeline/hdf5_converter.cpp
)

target_include_directories(micecam_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(micecam_core PUBLIC
    Threads::Threads
    nlohmann_json::nlohmann_json
)

# Camera backend (optional)
if(WITH_CAMERA_BACKEND)
    add_library(micecam_camera
        src/camera/camera_backend.cpp
        src/camera/usb_camera_backend.cpp
        src/camera/ffmpeg_camera_backend.cpp
    )

    if(DEPTHAI_FOUND)
        target_sources(micecam_camera PRIVATE src/camera/oak_camera_backend.cpp)
        target_compile_definitions(micecam_camera PRIVATE WITH_OAK_CAMERA)
    endif()


    target_include_directories(micecam_camera PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
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
endif()

# Tests
if(BUILD_TESTS)
    add_executable(micecam_tests
        tests/core/frame_test.cpp
        tests/core/ring_buffer_test.cpp
        tests/camera/fake_camera.cpp
        tests/pipeline/pipeline_test.cpp
        tests/pipeline/integration_test.cpp
        tests/pipeline/hdf5_test.cpp
        tests/pipeline/configurable_buffer_test.cpp
        tests/benchmark/stress_test.cpp
    )

    target_include_directories(micecam_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    target_link_libraries(micecam_tests
        micecam_core
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(micecam_tests)
endif()

# Main executable
add_executable(micecam
    src/main.cpp
)

target_link_libraries(micecam PRIVATE micecam_core)
if(WITH_CAMERA_BACKEND)
    target_link_libraries(micecam PRIVATE micecam_camera)
endif()

# Demo executable
add_executable(micecam_demo
    demo_simple.cpp
)
target_link_libraries(micecam_demo PRIVATE micecam_core)

# USB Camera Test
if(WITH_CAMERA_BACKEND)
    add_executable(usb_camera_test usb_camera_test.cpp)
    target_link_libraries(usb_camera_test PRIVATE micecam_core micecam_camera)

    add_executable(buffer_bench buffer_bench.cpp)
    target_link_libraries(buffer_bench PRIVATE micecam_core)

    add_executable(disk_bench disk_bench.cpp)
    target_link_libraries(disk_bench PRIVATE micecam_core)

    add_executable(ffmpeg_camera_test ffmpeg_camera_test.cpp)
    target_link_libraries(ffmpeg_camera_test PRIVATE micecam_core micecam_camera)

    if(DEPTHAI_FOUND)
        add_executable(oak_camera_test oak_camera_test.cpp)
        target_link_libraries(oak_camera_test PRIVATE micecam_core micecam_camera)
    endif()
endif()

# Installation
install(TARGETS micecam DESTINATION bin)
