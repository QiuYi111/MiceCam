cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(MiceCam VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default build type to Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Build options
option(BUILD_SPIKE "Build spike feasibility tests (headless)" OFF)
option(BUILD_UI "Build Qt/QML UI executable (requires Qt6)" ON)

# macOS: add Homebrew paths for finding libraries
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH /opt/homebrew/opt/qt)
endif()

# ---- Dependencies (cross-platform: find_package first, pkg-config fallback) ----

# FFmpeg
find_package(FFmpeg QUIET)
if(NOT FFmpeg_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil libavdevice libswscale)
else()
    # Create an alias target compatible with our PkgConfig::FFMPEG usage
    if(NOT TARGET PkgConfig::FFMPEG)
        add_library(PkgConfig::FFMPEG INTERFACE IMPORTED)
        foreach(_lib avcodec avformat avutil avdevice swscale)
            if(TARGET FFmpeg::${_lib})
                target_link_libraries(PkgConfig::FFMPEG INTERFACE FFmpeg::${_lib})
            endif()
        endforeach()
        target_include_directories(PkgConfig::FFMPEG INTERFACE ${FFMPEG_INCLUDE_DIRS})
    endif()
endif()

# spdlog
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(spdlog REQUIRED IMPORTED_TARGET spdlog)
    endif()
else()
    if(NOT TARGET PkgConfig::spdlog)
        add_library(PkgConfig::spdlog INTERFACE IMPORTED)
        target_link_libraries(PkgConfig::spdlog INTERFACE spdlog::spdlog)
    endif()
endif()

# nlohmann_json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(nlohmann_json REQUIRED IMPORTED_TARGET nlohmann_json)
    endif()
else()
    if(NOT TARGET PkgConfig::nlohmann_json)
        add_library(PkgConfig::nlohmann_json INTERFACE IMPORTED)
        target_link_libraries(PkgConfig::nlohmann_json INTERFACE nlohmann_json::nlohmann_json)
    endif()
endif()

# Qt6 (optional for headless CI builds)
if(BUILD_UI)
    find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick QuickControls2)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
endif()

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

# Encoding infrastructure library
add_library(micecam_encoding STATIC
    internal/domain/TimestampEngine.cpp
    internal/domain/PluginRegistry.cpp
    internal/domain/SessionMetadata.cpp
    internal/domain/StreamStats.cpp
    internal/infrastructure/HardwareEncoderSelector.cpp
    internal/infrastructure/FFmpegEncoder.cpp
    internal/infrastructure/StreamWriter.cpp
    internal/infrastructure/SRTWriter.cpp
    internal/infrastructure/MetadataWriter.cpp
    internal/infrastructure/ConfigLoader.cpp
    internal/infrastructure/AlertManager.cpp
    internal/infrastructure/Watchdog.cpp
    internal/infrastructure/FeishuWebhook.cpp
    internal/infrastructure/MockCameraBackend.cpp
    internal/infrastructure/CameraManager.cpp
    internal/infrastructure/OAKCameraBackend.cpp
    internal/infrastructure/FFmpegCameraBackend.cpp
    internal/pipeline/TranscodeStage.cpp
    internal/pipeline/StatsCollector.cpp
    internal/pipeline/PreflightValidator.cpp
    internal/pipeline/RecordingPipeline.cpp
)

target_include_directories(micecam_encoding PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/internal
)

target_link_libraries(micecam_encoding PUBLIC
    PkgConfig::FFMPEG
    PkgConfig::spdlog
    PkgConfig::nlohmann_json
)

target_compile_options(micecam_encoding PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

# MiceCam v2 executable (requires Qt6, skipped in CI headless mode)
if(BUILD_UI)
    add_executable(micecam
        cmd/micecam_v2/main.cpp
        cmd/micecam_v2/qml/qml.qrc
    )
    target_include_directories(micecam PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/api
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
    )
    target_link_libraries(micecam PRIVATE
        micecam_encoding
        Qt6::Core
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        PkgConfig::FFMPEG
        PkgConfig::spdlog
        PkgConfig::nlohmann_json
    )
    target_compile_options(micecam PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
    )
    set_target_properties(micecam PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/cmd/micecam"
    )
endif()

# Ensure test output directory exists
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/test_output")

# Test executables
function(add_micecam_test name sources)
    add_executable(${name} ${sources})
    target_include_directories(${name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/api
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
    )
    target_link_libraries(${name} PRIVATE micecam_encoding GTest::gtest GTest::gtest_main PkgConfig::FFMPEG PkgConfig::spdlog PkgConfig::nlohmann_json)
    target_compile_options(${name} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
    )
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )
    add_test(NAME ${name} COMMAND ${name} WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
endfunction()

add_micecam_test(test_encoder_selector tests/unit/test_encoder_selector.cpp)
add_micecam_test(test_ffmpeg_encoder tests/unit/test_ffmpeg_encoder.cpp)
add_micecam_test(test_stream_writer tests/unit/test_stream_writer.cpp)
add_micecam_test(test_srt_writer tests/unit/test_srt_writer.cpp)
add_micecam_test(test_metadata_writer tests/unit/test_metadata_writer.cpp)
add_micecam_test(test_encoding_pipeline tests/integration/test_encoding_pipeline.cpp)
add_micecam_test(test_config_loader tests/unit/test_config_loader.cpp)
add_micecam_test(test_alert_manager tests/unit/test_alert_manager.cpp)
add_micecam_test(test_mock_camera tests/unit/test_mock_camera.cpp)
add_micecam_test(test_camera_manager tests/unit/test_camera_manager.cpp)
add_micecam_test(test_oak_camera tests/unit/test_oak_camera.cpp)
add_micecam_test(test_ffmpeg_camera tests/unit/test_ffmpeg_camera.cpp)
add_micecam_test(test_preflight tests/unit/test_preflight.cpp)
add_micecam_test(test_stats_collector tests/unit/test_stats_collector.cpp)
add_micecam_test(test_watchdog tests/unit/test_watchdog.cpp)
add_micecam_test(test_feishu_webhook tests/unit/test_feishu_webhook.cpp)
add_micecam_test(test_recording_pipeline tests/unit/test_recording_pipeline.cpp)
add_micecam_test(test_camera_pipeline_integration tests/integration/test_camera_pipeline.cpp)
add_micecam_test(test_watchdog_alerting_integration tests/integration/test_watchdog_alerting.cpp)

# Spike subdirectory (headless feasibility tests)
if(BUILD_SPIKE)
    add_subdirectory(cmd/spike)
endif()
