# Python bindings for MiceCam using Pybind11
# This module is only built when BUILD_PYTHON_BINDINGS=ON

# If the caller pins Python3_EXECUTABLE (for example to a local uv-managed
# CPython), pre-seed the include/lib hints so FindPython3 can resolve the
# matching development artifacts instead of falling back to vcpkg's Python.
if(DEFINED Python3_EXECUTABLE AND NOT DEFINED Python3_LIBRARY)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c
                "import sysconfig; print(sysconfig.get_path('include')); print(sysconfig.get_config_var('LIBDIR') or ''); print(sysconfig.get_config_var('LDLIBRARY') or '')"
        OUTPUT_VARIABLE MICECAM_PYTHON3_PATH_INFO
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE MICECAM_PYTHON3_INFO_RESULT
    )

    if(MICECAM_PYTHON3_INFO_RESULT EQUAL 0)
        string(REPLACE "\r\n" "\n" MICECAM_PYTHON3_PATH_INFO "${MICECAM_PYTHON3_PATH_INFO}")
        string(REPLACE "\n" ";" MICECAM_PYTHON3_PATH_INFO "${MICECAM_PYTHON3_PATH_INFO}")
        list(LENGTH MICECAM_PYTHON3_PATH_INFO MICECAM_PYTHON3_PATH_COUNT)

        if(MICECAM_PYTHON3_PATH_COUNT GREATER 0)
            list(GET MICECAM_PYTHON3_PATH_INFO 0 MICECAM_PYTHON3_INCLUDE_DIR)
            if(EXISTS "${MICECAM_PYTHON3_INCLUDE_DIR}")
                set(Python3_INCLUDE_DIR "${MICECAM_PYTHON3_INCLUDE_DIR}" CACHE PATH "Pinned Python include dir" FORCE)
            endif()
        endif()

        if(MICECAM_PYTHON3_PATH_COUNT GREATER 2)
            list(GET MICECAM_PYTHON3_PATH_INFO 1 MICECAM_PYTHON3_LIBDIR)
            list(GET MICECAM_PYTHON3_PATH_INFO 2 MICECAM_PYTHON3_LIBNAME)
            if(MICECAM_PYTHON3_LIBDIR AND MICECAM_PYTHON3_LIBNAME)
                set(MICECAM_PYTHON3_LIBRARY "${MICECAM_PYTHON3_LIBDIR}/${MICECAM_PYTHON3_LIBNAME}")
                if(EXISTS "${MICECAM_PYTHON3_LIBRARY}")
                    set(Python3_LIBRARY "${MICECAM_PYTHON3_LIBRARY}" CACHE FILEPATH "Pinned Python library" FORCE)
                    set(Python3_LIBRARY_RELEASE "${MICECAM_PYTHON3_LIBRARY}" CACHE FILEPATH "Pinned Python release library" FORCE)
                endif()
            endif()
        endif()
    endif()
endif()

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
