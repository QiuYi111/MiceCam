cmake_minimum_required(VERSION 3.20)

find_program(PROTOC protoc REQUIRED)
find_program(GRPC_CPP_PLUGIN
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

set(PROTO_FILE ${CMAKE_SOURCE_DIR}/api/micecam/camera_plugin.proto)
set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

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
    COMMENT "Generating gRPC stubs for camera_plugin.proto (OAK plugin)"
)

set(OAK_PLUGIN_SOURCES
    main.cpp
    OAKPluginServer.cpp
)

set(OAK_PLUGIN_PROTO_SOURCES
    ${PROTO_GEN_DIR}/micecam/camera_plugin.pb.cc
    ${PROTO_GEN_DIR}/micecam/camera_plugin.grpc.pb.cc
)

add_executable(micecam_oak_plugin
    ${OAK_PLUGIN_SOURCES}
    ${OAK_PLUGIN_PROTO_SOURCES}
)

target_include_directories(micecam_oak_plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROTO_GEN_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/api
    ${CMAKE_SOURCE_DIR}/internal
)

if(APPLE)
    target_include_directories(micecam_oak_plugin PRIVATE
        /opt/homebrew/include
    )
endif()

target_link_libraries(micecam_oak_plugin PRIVATE
    gRPC::grpc++
    gRPC::grpc
    protobuf::libprotobuf
    PkgConfig::spdlog
)

target_compile_definitions(micecam_oak_plugin PRIVATE
    _LIBCPP_ENABLE_CXX20_REMOVED_ALLOCATOR_MEMBERS
    $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
    $<$<PLATFORM_ID:Windows>:NOMINMAX>
)

target_compile_options(micecam_oak_plugin PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /Zc:designatedInitializers>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

set_target_properties(micecam_oak_plugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/cmd/plugins/micecam_oak"
)

# DepthAI SDK integration (optional)
find_package(depthai QUIET)
if(depthai_FOUND)
    target_compile_definitions(micecam_oak_plugin PRIVATE WITH_DEPTHAI)
    target_link_libraries(micecam_oak_plugin PRIVATE depthai::core)
    message(STATUS "OAK plugin: DepthAI SDK found, enabling WITH_DEPTHAI")
else()
    message(STATUS "OAK plugin: DepthAI SDK not found, building in diagnostic-only mode")
endif()
