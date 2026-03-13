# Python bindings for MiceCam using Pybind11
# This module is only built when BUILD_PYTHON_BINDINGS=ON

# Find Python first
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
message(STATUS "Python3 found: ${Python3_EXECUTABLE}")
message(STATUS "Python3 include: ${Python3_INCLUDE_DIRS}")

# Find pybind11 - prefer pip-installed version
find_package(pybind11 CONFIG QUIET)
if(NOT pybind11_FOUND)
    # Try to get pybind11 cmake dir from pip
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import pybind11; print(pybind11.get_cmake_dir())"
        OUTPUT_VARIABLE PYBIND11_CMAKE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE PYBIND11_RESULT
    )

    if(PYBIND11_RESULT EQUAL 0 AND EXISTS "${PYBIND11_CMAKE_DIR}")
        message(STATUS "Found pip-installed pybind11: ${PYBIND11_CMAKE_DIR}")
        list(APPEND CMAKE_PREFIX_PATH "${PYBIND11_CMAKE_DIR}")
        find_package(pybind11 CONFIG REQUIRED)
    else()
        # Fallback: FetchContent
        message(STATUS "Fetching pybind11 from GitHub...")
        include(FetchContent)
        FetchContent_Declare(
            pybind11
            GIT_REPOSITORY https://github.com/pybind/pybind11.git
            GIT_TAG        v2.11.1
        )
        FetchContent_MakeAvailable(pybind11)
    endif()
endif()

message(STATUS "pybind11 version: ${pybind11_VERSION}")

# Create the Python module
pybind11_add_module(_micecam
    module.cpp
)

# Link to core library and camera library (which includes OAK/USB support)
target_link_libraries(_micecam PRIVATE
    micecam_core
    micecam_camera
)

if(TARGET micecam_oak)
    target_link_libraries(_micecam PRIVATE micecam_oak)
endif()

# Pass OAK camera define if it was built with OAK support
if(DEPTHAI_FOUND)
    target_compile_definitions(_micecam PRIVATE WITH_OAK_CAMERA=1)
    message(STATUS "Python bindings: OAK camera support enabled")
endif()

# Pass USB/FFmpeg camera define
if(WITH_CAMERA_BACKEND)
    target_compile_definitions(_micecam PRIVATE WITH_CAMERA_BACKEND=1)
    if(FFMPEG_FOUND)
        target_compile_definitions(_micecam PRIVATE WITH_FFMPEG=1)
    endif()
    message(STATUS "Python bindings: Camera backend enabled")
endif()

target_include_directories(_micecam PRIVATE
    ${CMAKE_SOURCE_DIR}/api
    ${CMAKE_SOURCE_DIR}/internal
)

# Set the output name without lib prefix
set_target_properties(_micecam PROPERTIES
    PREFIX ""
    OUTPUT_NAME "_micecam"
)

# Install the Python module
install(TARGETS _micecam
    LIBRARY DESTINATION python/micecam
    RUNTIME DESTINATION python/micecam
)

install(FILES __init__.py
    DESTINATION python/micecam
)
