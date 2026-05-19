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
option(BUILD_HIL "Build hardware-in-the-loop tests (requires real camera)" OFF)
option(BUILD_STRESS "Build 1-hour stress test binary" OFF)
option(BUILD_PLUGINS "Build bundled camera plugin executables" ON)

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
    if(UNIX AND NOT APPLE)
        set(MiceCam_QML_NO_CACHEGEN "NO_CACHEGEN")
    endif()
endif()

# depthai-core (optional)
find_package(depthai QUIET)
if(depthai_FOUND)
    set(WITH_DEPTHAI ON)
    add_compile_definitions(WITH_DEPTHAI)
endif()

# Protobuf (optional for Phase 0 — proto compilation is a contract artifact)
find_package(Protobuf CONFIG QUIET)

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
    internal/domain/PluginManifest.cpp
    internal/domain/PluginErrorRegistry.cpp
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
    internal/infrastructure/LinkedPluginConfig.cpp
    internal/infrastructure/PluginRegistryService.cpp
    internal/infrastructure/MockCameraBackend.cpp
    internal/infrastructure/CameraManager.cpp
    internal/infrastructure/OAKCameraBackend.cpp
    internal/infrastructure/FFmpegCameraBackend.cpp
    internal/infrastructure/PluginRingReader.cpp
    internal/infrastructure/PluginStreamConsumer.cpp
    internal/infrastructure/SharedMemoryBackend.cpp
    internal/infrastructure/ResourceManager.cpp
    internal/infrastructure/StreamLivenessMonitor.cpp
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
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /Zc:designatedInitializers>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
    )

    target_compile_definitions(micecam_encoding PRIVATE
        $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
        $<$<PLATFORM_ID:Windows>:NOMINMAX>
        $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
    )

    if(WIN32)
        target_sources(micecam_encoding PRIVATE
            internal/infrastructure/Win32SharedMemory.cpp
        )
    else()
        target_sources(micecam_encoding PRIVATE
            internal/infrastructure/PosixSharedMemory.cpp
        )
    endif()

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

    add_subdirectory(cmd/micecam_ui)
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
add_micecam_test(test_preflight_detail tests/unit/test_preflight_detail.cpp)
add_micecam_test(test_preflight_calibration tests/unit/test_preflight_calibration.cpp)
add_micecam_test(test_stats_collector tests/unit/test_stats_collector.cpp)
add_micecam_test(test_watchdog tests/unit/test_watchdog.cpp)
add_micecam_test(test_feishu_webhook tests/unit/test_feishu_webhook.cpp)
add_micecam_test(test_backend_ui_contract tests/unit/test_backend_ui_contract.cpp)
    add_micecam_test(test_recording_pipeline tests/unit/test_recording_pipeline.cpp)
    if(NOT WIN32)
        add_micecam_test(test_recording_pipeline_outputs tests/integration/test_recording_pipeline_outputs.cpp)
    endif()
    add_micecam_test(test_fmp4_smoke tests/integration/test_fmp4_smoke.cpp)
add_micecam_test(test_camera_pipeline_integration tests/integration/test_camera_pipeline.cpp)
add_micecam_test(test_watchdog_alerting_integration tests/integration/test_watchdog_alerting.cpp)
add_micecam_test(test_plugin_contract tests/unit/test_plugin_contract.cpp)
add_micecam_test(test_plugin_registry tests/unit/test_plugin_registry.cpp)
add_micecam_test(test_linked_plugin_config tests/unit/test_linked_plugin_config.cpp)
if(NOT WIN32)
    add_micecam_test(test_plugin_ring_reader tests/unit/test_plugin_ring_reader.cpp)
    add_micecam_test(test_plugin_stream_consumer tests/unit/test_plugin_stream_consumer.cpp)
endif()
add_micecam_test(test_resource_manager tests/unit/test_resource_manager.cpp)
add_micecam_test(test_timestamp_engine tests/unit/test_timestamp_engine.cpp)
if(NOT WIN32)
    add_micecam_test(test_shared_memory_backend tests/unit/test_shared_memory_backend.cpp)
endif()
add_micecam_test(test_stream_liveness_monitor tests/unit/test_stream_liveness_monitor.cpp)

if(BUILD_PLUGINS)
    # FFmpeg plugin gRPC integration test
    # Must re-generate proto stubs since gRPC plugin test needs the full gRPC service stubs
    set(FFMPEG_PLUGIN_TEST_SRC tests/unit/test_ffmpeg_plugin_server.cpp)
    set(FFMPEG_PLUGIN_PROTO_GEN_DIR "${CMAKE_BINARY_DIR}/test_proto_gen")
    file(MAKE_DIRECTORY "${FFMPEG_PLUGIN_PROTO_GEN_DIR}")

    find_program(FFMPEG_TEST_PROTOC protoc REQUIRED)
    find_program(FFMPEG_TEST_GRPC_CPP_PLUGIN
        NAMES grpc_cpp_plugin grpc_cpp_plugin.exe
        HINTS
            /ucrt64/bin
            /usr/bin
            /usr/local/bin
            /opt/homebrew/bin
            ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/grpc
            ${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/tools/grpc
        REQUIRED
    )

    add_custom_command(
        OUTPUT
            ${FFMPEG_PLUGIN_PROTO_GEN_DIR}/micecam/camera_plugin.pb.cc
            ${FFMPEG_PLUGIN_PROTO_GEN_DIR}/micecam/camera_plugin.pb.h
            ${FFMPEG_PLUGIN_PROTO_GEN_DIR}/micecam/camera_plugin.grpc.pb.cc
            ${FFMPEG_PLUGIN_PROTO_GEN_DIR}/micecam/camera_plugin.grpc.pb.h
        COMMAND ${FFMPEG_TEST_PROTOC}
            ARGS
                --proto_path=${CMAKE_SOURCE_DIR}/api
                --cpp_out=${FFMPEG_PLUGIN_PROTO_GEN_DIR}
                --grpc_out=${FFMPEG_PLUGIN_PROTO_GEN_DIR}
                --plugin=protoc-gen-grpc=${FFMPEG_TEST_GRPC_CPP_PLUGIN}
                ${CMAKE_SOURCE_DIR}/api/micecam/camera_plugin.proto
        DEPENDS ${CMAKE_SOURCE_DIR}/api/micecam/camera_plugin.proto
        COMMENT "Generating gRPC stubs for plugin test"
    )

    add_executable(test_ffmpeg_plugin_server
        ${FFMPEG_PLUGIN_TEST_SRC}
        ${FFMPEG_PLUGIN_PROTO_GEN_DIR}/micecam/camera_plugin.pb.cc
        ${FFMPEG_PLUGIN_PROTO_GEN_DIR}/micecam/camera_plugin.grpc.pb.cc
        cmd/plugins/micecam_ffmpeg/FFmpegPluginServer.cpp
        cmd/plugins/micecam_ffmpeg/AVFoundationEnumerator.cpp
        cmd/plugins/micecam_ffmpeg/RingFrameProducer.cpp
        cmd/plugins/micecam_ffmpeg/PlatformUtil.cpp
        internal/infrastructure/SharedMemoryBackend.cpp
    )

    if(WIN32)
        target_sources(test_ffmpeg_plugin_server PRIVATE
            internal/infrastructure/Win32SharedMemory.cpp
        )
    else()
        target_sources(test_ffmpeg_plugin_server PRIVATE
            internal/infrastructure/PosixSharedMemory.cpp
        )
    endif()

    target_include_directories(test_ffmpeg_plugin_server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/api
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
        ${CMAKE_CURRENT_SOURCE_DIR}/cmd/plugins/micecam_ffmpeg
        ${FFMPEG_PLUGIN_PROTO_GEN_DIR}
    )

    if(APPLE)
        target_include_directories(test_ffmpeg_plugin_server PRIVATE /opt/homebrew/include)
    endif()

    target_link_libraries(test_ffmpeg_plugin_server PRIVATE
        gRPC::grpc++
        gRPC::grpc
        protobuf::libprotobuf
        GTest::gtest
        GTest::gtest_main
        PkgConfig::FFMPEG
        PkgConfig::spdlog
        PkgConfig::nlohmann_json
    )

    target_compile_options(test_ffmpeg_plugin_server PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /Zc:designatedInitializers>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
    )

    target_compile_definitions(test_ffmpeg_plugin_server PRIVATE
        _LIBCPP_ENABLE_CXX20_REMOVED_ALLOCATOR_MEMBERS
        $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
        $<$<PLATFORM_ID:Windows>:NOMINMAX>
    )

    set_target_properties(test_ffmpeg_plugin_server PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )

    add_test(NAME test_ffmpeg_plugin_server COMMAND test_ffmpeg_plugin_server
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

    add_executable(test_negative_plugin_rpcs
        tests/unit/test_negative_plugin_rpcs.cpp
        ${FFMPEG_PLUGIN_PROTO_GEN_DIR}/micecam/camera_plugin.pb.cc
        ${FFMPEG_PLUGIN_PROTO_GEN_DIR}/micecam/camera_plugin.grpc.pb.cc
        cmd/plugins/micecam_ffmpeg/FFmpegPluginServer.cpp
        cmd/plugins/micecam_ffmpeg/AVFoundationEnumerator.cpp
        cmd/plugins/micecam_ffmpeg/RingFrameProducer.cpp
        cmd/plugins/micecam_ffmpeg/PlatformUtil.cpp
        internal/infrastructure/SharedMemoryBackend.cpp
    )
    if(WIN32)
        target_sources(test_negative_plugin_rpcs PRIVATE
            internal/infrastructure/Win32SharedMemory.cpp
        )
    else()
        target_sources(test_negative_plugin_rpcs PRIVATE
            internal/infrastructure/PosixSharedMemory.cpp
        )
    endif()
    target_include_directories(test_negative_plugin_rpcs PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/api
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
        ${CMAKE_CURRENT_SOURCE_DIR}/cmd/plugins/micecam_ffmpeg
        ${FFMPEG_PLUGIN_PROTO_GEN_DIR}
    )
    if(APPLE)
        target_include_directories(test_negative_plugin_rpcs PRIVATE /opt/homebrew/include)
    endif()
    target_link_libraries(test_negative_plugin_rpcs PRIVATE
        gRPC::grpc++
        gRPC::grpc
        protobuf::libprotobuf
        GTest::gtest
        GTest::gtest_main
        PkgConfig::FFMPEG
        PkgConfig::spdlog
        PkgConfig::nlohmann_json
    )
    target_compile_options(test_negative_plugin_rpcs PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /Zc:designatedInitializers>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
    )
    target_compile_definitions(test_negative_plugin_rpcs PRIVATE
        _LIBCPP_ENABLE_CXX20_REMOVED_ALLOCATOR_MEMBERS
        $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
        $<$<PLATFORM_ID:Windows>:NOMINMAX>
    )
    set_target_properties(test_negative_plugin_rpcs PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )
    add_test(NAME test_negative_plugin_rpcs COMMAND test_negative_plugin_rpcs
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

    add_executable(test_proto_calibrate
        tests/unit/test_proto_calibrate.cpp
        ${FFMPEG_PLUGIN_PROTO_GEN_DIR}/micecam/camera_plugin.pb.cc
    )
    target_include_directories(test_proto_calibrate PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/api
        ${FFMPEG_PLUGIN_PROTO_GEN_DIR}
    )
    target_link_libraries(test_proto_calibrate PRIVATE
        protobuf::libprotobuf
        GTest::gtest
        GTest::gtest_main
    )
    set_target_properties(test_proto_calibrate PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )
    add_test(NAME test_proto_calibrate COMMAND test_proto_calibrate
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

    # Integration e2e tests (fork+exec real plugin binary)
    set(INTEG_E2E_PROTO_GEN_DIR "${CMAKE_BINARY_DIR}/integ_e2e_proto_gen")
    file(MAKE_DIRECTORY "${INTEG_E2E_PROTO_GEN_DIR}")

    add_custom_command(
        OUTPUT
            ${INTEG_E2E_PROTO_GEN_DIR}/micecam/camera_plugin.pb.cc
            ${INTEG_E2E_PROTO_GEN_DIR}/micecam/camera_plugin.pb.h
            ${INTEG_E2E_PROTO_GEN_DIR}/micecam/camera_plugin.grpc.pb.cc
            ${INTEG_E2E_PROTO_GEN_DIR}/micecam/camera_plugin.grpc.pb.h
        COMMAND ${FFMPEG_TEST_PROTOC}
            ARGS
                --proto_path=${CMAKE_SOURCE_DIR}/api
                --cpp_out=${INTEG_E2E_PROTO_GEN_DIR}
                --grpc_out=${INTEG_E2E_PROTO_GEN_DIR}
                --plugin=protoc-gen-grpc=${FFMPEG_TEST_GRPC_CPP_PLUGIN}
                ${CMAKE_SOURCE_DIR}/api/micecam/camera_plugin.proto
        DEPENDS ${CMAKE_SOURCE_DIR}/api/micecam/camera_plugin.proto
        COMMENT "Generating gRPC stubs for integration e2e tests"
    )

    function(add_e2e_integ_test name source)
        add_executable(${name}
            ${source}
            ${INTEG_E2E_PROTO_GEN_DIR}/micecam/camera_plugin.pb.cc
            ${INTEG_E2E_PROTO_GEN_DIR}/micecam/camera_plugin.grpc.pb.cc
        )
        target_include_directories(${name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/api
            ${INTEG_E2E_PROTO_GEN_DIR}
        )
        target_link_libraries(${name} PRIVATE
            gRPC::grpc++
            gRPC::grpc
            protobuf::libprotobuf
            GTest::gtest
            GTest::gtest_main
            PkgConfig::spdlog
        )
        target_compile_options(${name} PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/W4>
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
        )
        target_compile_definitions(${name} PRIVATE
            _LIBCPP_ENABLE_CXX20_REMOVED_ALLOCATOR_MEMBERS
        )
        set_target_properties(${name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
        )
        add_test(NAME ${name} COMMAND ${name}
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
        set_tests_properties(${name} PROPERTIES
            LABELS "no-hardware"
            TIMEOUT 60
        )
        add_dependencies(${name} micecam_ffmpeg_plugin)
    endfunction()

    # Fork+exec e2e tests use POSIX API (fork/exec/shm_unlink) — not available on Windows
    if(NOT WIN32)
        add_e2e_integ_test(test_plugin_e2e_no_hw tests/integration/test_plugin_e2e_no_hw.cpp)
        add_e2e_integ_test(test_calibrate_e2e tests/integration/test_calibrate_e2e.cpp)
        add_e2e_integ_test(test_dual_path_keyframe tests/integration/test_dual_path_keyframe.cpp)
    endif()
endif()

if(BUILD_UI)
    add_micecam_test(test_app_models tests/unit/test_app_models.cpp)
    add_micecam_test(test_app_controller tests/unit/test_app_controller.cpp)
    add_micecam_test(test_app_alert_notifications tests/unit/test_app_alert_notifications.cpp)
    target_link_libraries(test_app_models PRIVATE Qt6::Core)
    target_link_libraries(test_app_controller PRIVATE Qt6::Core Qt6::Qml)
    target_link_libraries(test_app_alert_notifications PRIVATE Qt6::Core)
    target_sources(test_app_models PRIVATE
        cmd/micecam_ui/AppCameraModel.cpp
        cmd/micecam_ui/AppAlertModel.cpp
        cmd/micecam_ui/CameraSourceModel.cpp
    )
    target_sources(test_app_controller PRIVATE
        cmd/micecam_ui/AppCameraModel.cpp
        cmd/micecam_ui/AppAlertModel.cpp
        cmd/micecam_ui/AppSettings.cpp
        cmd/micecam_ui/AppController.cpp
        cmd/micecam_ui/CameraSourceModel.cpp
    )
    target_sources(test_app_alert_notifications PRIVATE
        cmd/micecam_ui/AppAlertModel.cpp
    )
endif()

# Python tests (no hardware required, uses stdlib unittest)
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(
        NAME test_validate_session_artifacts
        COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tests/unit/test_validate_session_artifacts.py
            -v
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    set_tests_properties(test_validate_session_artifacts PROPERTIES
        TIMEOUT 60
        LABELS "python;no-hardware"
    )
endif()

# Hardware-in-the-Loop tests (requires real camera hardware)
if(BUILD_HIL)
    add_micecam_test(test_real_camera_hil tests/hil/test_real_camera.cpp)
    add_micecam_test(test_e2e_stress tests/hil/test_e2e_stress.cpp)
    add_micecam_test(test_hil_e2e tests/hil/test_hil_e2e.cpp)
    add_micecam_test(test_hil_crash_recovery tests/hil/test_hil_crash_recovery.cpp)
endif()

# Spike subdirectory (headless feasibility tests)
if(BUILD_SPIKE)
    add_subdirectory(cmd/spike)
endif()

# 1-hour stress test binary
if(BUILD_STRESS)
    add_executable(stress_1h_120fps cmd/stress_1h/main.cpp)
    target_include_directories(stress_1h_120fps PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/api
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
    )
    target_link_libraries(stress_1h_120fps PRIVATE
        micecam_encoding
        PkgConfig::FFMPEG
        PkgConfig::spdlog
        PkgConfig::nlohmann_json
    )
    target_compile_options(stress_1h_120fps PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
    )
endif()

# Proto codegen (optional — Phase 0 contract artifact)
if(Protobuf_FOUND)
    find_program(PROTOC_EXE protoc)
    if(PROTOC_EXE)
        set(PROTO_SRC_DIR ${CMAKE_BINARY_DIR}/proto_src)
        file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
        add_custom_command(
            OUTPUT
                ${PROTO_SRC_DIR}/micecam/camera_plugin.pb.cc
                ${PROTO_SRC_DIR}/micecam/camera_plugin.pb.h
            COMMAND ${PROTOC_EXE}
                ARGS --proto_path=${CMAKE_SOURCE_DIR}/api
                     --cpp_out=${PROTO_SRC_DIR}
                     ${CMAKE_SOURCE_DIR}/api/micecam/camera_plugin.proto
            DEPENDS ${CMAKE_SOURCE_DIR}/api/micecam/camera_plugin.proto
            COMMENT "Generating protobuf stubs for camera_plugin.proto"
        )
        file(MAKE_DIRECTORY ${PROTO_SRC_DIR}/micecam)
        add_library(micecam_plugin_proto STATIC
            ${PROTO_SRC_DIR}/micecam/camera_plugin.pb.cc
            ${PROTO_SRC_DIR}/micecam/camera_plugin.pb.h
        )
        target_include_directories(micecam_plugin_proto PUBLIC
            ${PROTO_SRC_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/api
            ${CMAKE_CURRENT_SOURCE_DIR}/internal
        )
        target_link_libraries(micecam_plugin_proto PUBLIC protobuf::libprotobuf)
        add_compile_definitions(MICECAM_HAS_PROTOBUF)
    endif()
endif()

# Plugin executables (out-of-process camera backends via gRPC)
if(BUILD_PLUGINS)
    find_package(gRPC CONFIG QUIET)
    if(NOT gRPC_FOUND)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(gRPC REQUIRED IMPORTED_TARGET grpc grpc++)
        if(NOT TARGET gRPC::grpc++)
            add_library(gRPC::grpc++ ALIAS PkgConfig::gRPC)
        endif()
        if(NOT TARGET gRPC::grpc)
            add_library(gRPC::grpc ALIAS PkgConfig::gRPC)
        endif()
    endif()
    find_package(protobuf CONFIG QUIET)
    if(NOT protobuf_FOUND)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(protobuf REQUIRED IMPORTED_TARGET protobuf)
        if(NOT TARGET protobuf::libprotobuf)
            add_library(protobuf::libprotobuf ALIAS PkgConfig::protobuf)
        endif()
    endif()
    add_subdirectory(cmd/plugins/micecam_ffmpeg)
    add_subdirectory(cmd/plugins/micecam_oak)

    # OAK plugin gRPC integration test (no hardware required)
    set(OAK_PLUGIN_TEST_SRC tests/unit/test_oak_plugin_server.cpp)
    set(OAK_PLUGIN_PROTO_GEN_DIR "${CMAKE_BINARY_DIR}/oak_test_proto_gen")
    file(MAKE_DIRECTORY "${OAK_PLUGIN_PROTO_GEN_DIR}")

    find_program(OAK_TEST_PROTOC protoc REQUIRED)
    find_program(OAK_TEST_GRPC_CPP_PLUGIN
        NAMES grpc_cpp_plugin grpc_cpp_plugin.exe
        HINTS
            /ucrt64/bin
            /usr/bin
            /usr/local/bin
            /opt/homebrew/bin
            ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/grpc
            ${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/tools/grpc
        REQUIRED
    )

    add_custom_command(
        OUTPUT
            ${OAK_PLUGIN_PROTO_GEN_DIR}/micecam/camera_plugin.pb.cc
            ${OAK_PLUGIN_PROTO_GEN_DIR}/micecam/camera_plugin.pb.h
            ${OAK_PLUGIN_PROTO_GEN_DIR}/micecam/camera_plugin.grpc.pb.cc
            ${OAK_PLUGIN_PROTO_GEN_DIR}/micecam/camera_plugin.grpc.pb.h
        COMMAND ${OAK_TEST_PROTOC}
            ARGS
                --proto_path=${CMAKE_SOURCE_DIR}/api
                --cpp_out=${OAK_PLUGIN_PROTO_GEN_DIR}
                --grpc_out=${OAK_PLUGIN_PROTO_GEN_DIR}
                --plugin=protoc-gen-grpc=${OAK_TEST_GRPC_CPP_PLUGIN}
                ${CMAKE_SOURCE_DIR}/api/micecam/camera_plugin.proto
        DEPENDS ${CMAKE_SOURCE_DIR}/api/micecam/camera_plugin.proto
        COMMENT "Generating gRPC stubs for OAK plugin test"
    )

    add_executable(test_oak_plugin_server
        ${OAK_PLUGIN_TEST_SRC}
        ${OAK_PLUGIN_PROTO_GEN_DIR}/micecam/camera_plugin.pb.cc
        ${OAK_PLUGIN_PROTO_GEN_DIR}/micecam/camera_plugin.grpc.pb.cc
        cmd/plugins/micecam_oak/OAKPluginServer.cpp
    )

    target_include_directories(test_oak_plugin_server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/api
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
        ${CMAKE_CURRENT_SOURCE_DIR}/cmd/plugins/micecam_oak
        ${OAK_PLUGIN_PROTO_GEN_DIR}
    )

    if(APPLE)
        target_include_directories(test_oak_plugin_server PRIVATE /opt/homebrew/include)
    endif()

    target_link_libraries(test_oak_plugin_server PRIVATE
        micecam_encoding
        gRPC::grpc++
        gRPC::grpc
        protobuf::libprotobuf
        GTest::gtest
        GTest::gtest_main
        PkgConfig::spdlog
        PkgConfig::nlohmann_json
    )

    target_compile_options(test_oak_plugin_server PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /Zc:designatedInitializers>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
    )

    target_compile_definitions(test_oak_plugin_server PRIVATE
        _LIBCPP_ENABLE_CXX20_REMOVED_ALLOCATOR_MEMBERS
        $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
        $<$<PLATFORM_ID:Windows>:NOMINMAX>
    )

    set_target_properties(test_oak_plugin_server PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )

    add_test(NAME test_oak_plugin_server COMMAND test_oak_plugin_server
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()
