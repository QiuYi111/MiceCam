# GPU Preview Module for MiceCam
# Requires: CUDA Toolkit, NVIDIA Video Codec SDK

# Check if CUDA is available
if(NOT CMAKE_CUDA_COMPILER)
    find_program(CUDA_NVCC_EXECUTABLE NAMES nvcc PATHS "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.1/bin")
    if(CUDA_NVCC_EXECUTABLE)
        set(CMAKE_CUDA_COMPILER ${CUDA_NVCC_EXECUTABLE})
    endif()
endif()

include(CheckLanguage)
check_language(CUDA)

if(CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED)
    
    message(STATUS "CUDA found: ${CUDAToolkit_VERSION} at ${CUDAToolkit_BIN_DIR}")
    
    # Check for nvcuvid (NVIDIA Video Codec SDK)
    set(VIDEO_CODEC_SDK_DIR "D:/MiceCam/3rdParty/Video_Codec_SDK_13.0.37" CACHE PATH "Path to NVIDIA Video Codec SDK")
    
    find_library(NVCUVID_LIBRARY
        NAMES nvcuvid
        PATHS 
            "${CUDAToolkit_LIBRARY_DIR}"
            "${VIDEO_CODEC_SDK_DIR}/Lib/x64"
    )
    
    find_path(NVCUVID_INCLUDE_DIR
        NAMES nvcuvid.h
        PATHS
            "${VIDEO_CODEC_SDK_DIR}/Interface"
    )
    
    if(NVCUVID_LIBRARY AND NVCUVID_INCLUDE_DIR)
        message(STATUS "NVIDIA Video Codec SDK found:")
        message(STATUS "  Library: ${NVCUVID_LIBRARY}")
        message(STATUS "  Include: ${NVCUVID_INCLUDE_DIR}")
        set(HAVE_NVDEC TRUE)
    else()
        message(WARNING "NVIDIA Video Codec SDK (nvcuvid) not found. GPU decoding disabled.")
        set(HAVE_NVDEC FALSE)
    endif()
    
    # OpenGL for rendering
    find_package(OpenGL REQUIRED)
    
    # GLEW for OpenGL extension loading
    find_package(GLEW QUIET)
    if(NOT GLEW_FOUND)
        message(STATUS "GLEW not found, will use gl3w or similar")
    endif()
    
    # Create GPU library
    add_library(micecam_gpu
        gpu_jpeg_decoder.cpp
        gl_texture_pool.cpp
    )
    
    # Add CUDA sources if NVDEC is available
    if(HAVE_NVDEC)
        target_sources(micecam_gpu PRIVATE
            nv12_to_rgba.cu
        )
        target_compile_definitions(micecam_gpu PRIVATE HAVE_NVDEC=1)
        target_link_libraries(micecam_gpu PRIVATE ${NVCUVID_LIBRARY})
    endif()
    
    target_include_directories(micecam_gpu PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CUDAToolkit_INCLUDE_DIRS}
        ${NVCUVID_INCLUDE_DIR}
    )
    
    target_link_libraries(micecam_gpu PUBLIC
        micecam_core
        CUDA::cudart
        OpenGL::GL
    )
    
    if(GLEW_FOUND)
        target_link_libraries(micecam_gpu PRIVATE GLEW::GLEW)
    endif()
    
    set_target_properties(micecam_gpu PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        CUDA_STANDARD 17
    )
    
    # Installation
    install(TARGETS micecam_gpu
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
    )
    
else()
    message(WARNING "CUDA not found. GPU preview module will not be built.")
endif()
