cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(MiceCam VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(BUILD_SPIKE "Build spike feasibility tests (headless)" OFF)

# macOS: add Homebrew paths for finding libraries
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH /opt/homebrew/opt/qt)
endif()

# PkgConfig — required on macOS/Homebrew, also supports vcpkg
find_package(PkgConfig REQUIRED)

# FFmpeg
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil libavdevice libswscale)

# spdlog (header-only via Homebrew; vcpkg provides cmake config)
pkg_check_modules(spdlog REQUIRED IMPORTED_TARGET spdlog)

# nlohmann_json (header-only)
pkg_check_modules(nlohmann_json REQUIRED IMPORTED_TARGET nlohmann_json)

# Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick QuickControls2)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# depthai-core (optional)
find_package(depthai QUIET)
if(depthai_FOUND)
    set(WITH_DEPTHAI ON)
    add_compile_definitions(WITH_DEPTHAI)
endif()

# GTest
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH /opt/homebrew/opt/googletest)
endif()
find_package(GTest CONFIG REQUIRED)
enable_testing()

# MiceCam v2 executable
add_executable(micecam
    cmd/micecam_v2/main.cpp
    cmd/micecam_v2/qml/qml.qrc
    internal/domain/TimestampEngine.cpp
    internal/domain/PluginRegistry.cpp
    internal/domain/SessionMetadata.cpp
    internal/domain/StreamStats.cpp
)

target_include_directories(micecam PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/internal
)

target_link_libraries(micecam PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    PkgConfig::FFMPEG
    PkgConfig::spdlog
    PkgConfig::nlohmann_json
    GTest::gtest
)

target_compile_options(micecam PRIVATE -Wall -Wextra)

# Output path: build/cmd/micecam/micecam
set_target_properties(micecam PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/cmd/micecam"
)

# Spike subdirectory (preserved from v1)
if(BUILD_SPIKE)
    add_subdirectory(cmd/spike)
endif()
