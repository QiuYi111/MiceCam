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
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_BENCHMARKS "Build performance benchmarks" ON)
option(BUILD_PYTHON_BINDINGS "Build Python bindings (requires pybind11)" OFF)
option(BUILD_GPU_PREVIEW "Build GPU preview module (requires CUDA)" OFF)

# Dependencies
find_package(Threads REQUIRED)
find_package(OpenCV QUIET)

# Standard CMake discovery
include(FetchContent)

# JSON library (header-only)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# libnop (header-only, required by depthai-core headers)
FetchContent_Declare(
    libnop
    GIT_REPOSITORY https://github.com/luxonis/libnop.git
    GIT_TAG ab842f51dc2eb13916dc98417c2186b78320ed10
)
FetchContent_MakeAvailable(libnop)

# Luxonis DepthAI Core (Using Prebuilt)
option(WITH_OAK_CAMERA "Build with OAK camera support (requires depthai-core)" ON)
if(WITH_OAK_CAMERA)
    set(DEPTHAI_PREBUILT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/depthai-core-prebuild")
    if(EXISTS "${DEPTHAI_PREBUILT_DIR}")
        message(STATUS "Using prebuilt depthai-core from ${DEPTHAI_PREBUILT_DIR}")
        
        # Add include directories (prebuilt + external headers)
        set(DEPTHAI_INCLUDE_DIRS 
            "${DEPTHAI_PREBUILT_DIR}/include"
            "${DEPTHAI_PREBUILT_DIR}/include/depthai-shared"
            "${DEPTHAI_PREBUILT_DIR}/include/depthai-shared/3rdparty"
            "${DEPTHAI_PREBUILT_DIR}/include/depthai-bootloader-shared"
            "${DEPTHAI_PREBUILT_DIR}/lib/cmake/depthai/dependencies/include"
            "${nlohmann_json_SOURCE_DIR}/include"
            "${libnop_SOURCE_DIR}/include"
        )
        
        # Find libraries
        find_library(DEPTHAI_CORE_LIB NAMES depthai-core
            PATHS "${DEPTHAI_PREBUILT_DIR}/lib"
            NO_DEFAULT_PATH
        )
        find_library(DEPTHAI_OPENCV_LIB NAMES depthai-opencv
            PATHS "${DEPTHAI_PREBUILT_DIR}/lib"
            NO_DEFAULT_PATH
        )
        
        if(DEPTHAI_CORE_LIB)
            set(DEPTHAI_FOUND ON)
            # Create imported targets to mimic original ones if needed, 
            # or just use variables.
            add_library(depthai::core SHARED IMPORTED)
            set_target_properties(depthai::core PROPERTIES
                IMPORTED_LOCATION "${DEPTHAI_PREBUILT_DIR}/bin/depthai-core.dll"
                IMPORTED_IMPLIB "${DEPTHAI_CORE_LIB}"
                INTERFACE_INCLUDE_DIRECTORIES "${DEPTHAI_INCLUDE_DIRS}"
            )
            if(DEPTHAI_OPENCV_LIB)
                add_library(depthai::opencv SHARED IMPORTED)
                set_target_properties(depthai::opencv PROPERTIES
                    IMPORTED_LOCATION "${DEPTHAI_PREBUILT_DIR}/bin/depthai-opencv.dll"
                    IMPORTED_IMPLIB "${DEPTHAI_OPENCV_LIB}"
                )
            endif()
        else()
            message(WARNING "Prebuilt depthai-core.lib not found in ${DEPTHAI_PREBUILT_DIR}/lib")
            set(DEPTHAI_FOUND OFF)
        endif()
    else()
        message(WARNING "depthai-core-prebuild not found. OAK support disabled.")
        set(DEPTHAI_FOUND OFF)
    endif()
endif()


# FFmpeg for native capture (Plan C)
find_package(PkgConfig QUIET)
find_package(FFMPEG QUIET COMPONENTS avcodec avformat avutil avdevice swscale)
if(NOT FFMPEG_FOUND)
    # Fallback for vcpkg on this system
    set(FFMPEG_DIR "C:/vcpkg/installed/x64-windows")
    if(EXISTS "${FFMPEG_DIR}")
        set(FFMPEG_FOUND ON)
        set(FFMPEG_INCLUDE_DIRS "${FFMPEG_DIR}/include")
        set(FFMPEG_LIBRARIES 
            "${FFMPEG_DIR}/lib/avcodec.lib"
            "${FFMPEG_DIR}/lib/avformat.lib"
            "${FFMPEG_DIR}/lib/avutil.lib"
            "${FFMPEG_DIR}/lib/avdevice.lib"
            "${FFMPEG_DIR}/lib/swscale.lib"
        )
        message(STATUS "Using manual FFmpeg paths from ${FFMPEG_DIR}")
    else()
        set(FFMPEG_LIBRARIES avcodec avformat avutil avdevice swscale)
    endif()
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
    src/pipeline/frame_dispatcher.cpp
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
    set(CAMERA_SOURCES src/camera/camera_backend.cpp)
    
    if(FFMPEG_FOUND)
        list(APPEND CAMERA_SOURCES src/camera/ffmpeg_camera_backend.cpp)
    endif()
    
    if(DEPTHAI_FOUND)
        list(APPEND CAMERA_SOURCES src/camera/oak_camera_backend.cpp)
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

    # OAK Camera Targets (inside WITH_CAMERA_BACKEND to fix library path resolution)
    if(DEPTHAI_FOUND)
        add_executable(oak_camera_test oak_camera_test.cpp)
        target_link_libraries(oak_camera_test PRIVATE micecam_core micecam_camera)
        target_compile_definitions(oak_camera_test PRIVATE WITH_OAK_CAMERA)

        add_executable(oak_quad_recorder oak_quad_recorder.cpp)
        target_link_libraries(oak_quad_recorder PRIVATE micecam_core micecam_camera)
        target_compile_definitions(oak_quad_recorder PRIVATE WITH_OAK_CAMERA)
        
        add_executable(oak_diagnostic oak_diagnostic.cpp)
        target_include_directories(oak_diagnostic PRIVATE ${DEPTHAI_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})
        target_link_libraries(oak_diagnostic PRIVATE depthai::core micecam_core ${OpenCV_LIBS})
    endif()

    # FFmpeg camera test
    add_executable(ffmpeg_camera_test ffmpeg_camera_test.cpp)
    target_link_libraries(ffmpeg_camera_test PRIVATE micecam_core micecam_camera)
    target_compile_definitions(ffmpeg_camera_test PRIVATE WITH_FFMPEG)
endif()

# Tests
if(BUILD_TESTS)
    add_executable(micecam_tests
        tests/core/frame_test.cpp
        tests/core/ring_buffer_test.cpp
        tests/camera/fake_camera.cpp
        tests/pipeline/pipeline_test.cpp
        tests/pipeline/integration_test.cpp
        tests/pipeline/disk_writer_stream_test.cpp
        tests/pipeline/observer_test.cpp
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

install(DIRECTORY include/micecam DESTINATION include)

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
