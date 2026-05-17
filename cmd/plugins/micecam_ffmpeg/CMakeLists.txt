# CMakeLists.txt for micecam_ffmpeg_plugin (Phase 2)
cmake_minimum_required(VERSION 3.20)

# Find gRPC and Protobuf (found in parent scope by root CMakeLists.txt)
# find_package already invoked in parent scope with CONFIG REQUIRED
find_program(PROTOC protoc REQUIRED)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

# Proto codegen
set(PROTO_FILE ${CMAKE_SOURCE_DIR}/api/micecam/camera_plugin.proto)
set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Ensure the generated directory exists at configure time
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

add_custom_command(
    OUTPUT
        ${PROTO_GEN_DIR}/micecam/camera_plugin.pb.cc
        ${PROTO_GEN_DIR}/micecam/camera_plugin.pb.h
        ${PROTO_GEN_DIR}/micecam/camera_plugin.grpc.pb.cc
        ${PROTO_GEN_DIR}/micecam/camera_plugin.grpc.pb.h
    COMMAND ${PROTOC}
        ARGS
            --proto_path=${CMAKE_SOURCE_DIR}/api
            --cpp_out=${PROTO_GEN_DIR}
            --grpc_out=${PROTO_GEN_DIR}
            --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
            ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating gRPC stubs for camera_plugin.proto"
)

# Plugin sources
set(PLUGIN_SOURCES
    main.cpp
    FFmpegPluginServer.cpp
    AVFoundationEnumerator.cpp
    RingFrameProducer.cpp
    PlatformUtil.cpp
    ${CMAKE_SOURCE_DIR}/internal/infrastructure/PosixSharedMemory.cpp
    ${CMAKE_SOURCE_DIR}/internal/infrastructure/SharedMemoryBackend.cpp
)

set(PLUGIN_PROTO_SOURCES
    ${PROTO_GEN_DIR}/micecam/camera_plugin.pb.cc
    ${PROTO_GEN_DIR}/micecam/camera_plugin.grpc.pb.cc
)

# Plugin executable
add_executable(micecam_ffmpeg_plugin
    ${PLUGIN_SOURCES}
    ${PLUGIN_PROTO_SOURCES}
)

target_include_directories(micecam_ffmpeg_plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROTO_GEN_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/api
    ${CMAKE_SOURCE_DIR}/internal
)

if(WIN32)
    target_sources(micecam_ffmpeg_plugin PRIVATE
        ${CMAKE_SOURCE_DIR}/internal/infrastructure/Win32SharedMemory.cpp
    )
endif()

# macOS: Add Homebrew include paths
if(APPLE)
    target_include_directories(micecam_ffmpeg_plugin PRIVATE
        /opt/homebrew/include
    )
endif()

target_link_libraries(micecam_ffmpeg_plugin PRIVATE
    gRPC::grpc++
    gRPC::grpc
    protobuf::libprotobuf
    PkgConfig::FFMPEG
    PkgConfig::spdlog
    PkgConfig::nlohmann_json
)

target_compile_options(micecam_ffmpeg_plugin PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

# Output to build/cmd/plugins/micecam_ffmpeg/
set_target_properties(micecam_ffmpeg_plugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/cmd/plugins/micecam_ffmpeg"
)

# Avoid C++17 deprecation warnings from gRPC/abseil compiled under C++20
target_compile_definitions(micecam_ffmpeg_plugin PRIVATE
    _LIBCPP_ENABLE_CXX20_REMOVED_ALLOCATOR_MEMBERS
)
