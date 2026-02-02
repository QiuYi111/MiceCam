cmake_minimum_required(VERSION 3.20)
project(MiceCam VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_BENCHMARKS "Build performance benchmarks" ON)

# Dependencies
find_package(Threads REQUIRED)
find_package(OpenCV QUIET)

if(NOT OpenCV_FOUND)
    message(WARNING "OpenCV not found - camera backend will be disabled")
    option(WITH_CAMERA_BACKEND "Build camera backend (requires OpenCV)" OFF)
else()
    option(WITH_CAMERA_BACKEND "Build camera backend (requires OpenCV)" ON)
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
    )

    target_include_directories(micecam_camera PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(micecam_camera PUBLIC
        micecam_core
        ${OpenCV_LIBS}
    )
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

# Installation
install(TARGETS micecam DESTINATION bin)
