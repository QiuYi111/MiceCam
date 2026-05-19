cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil libswscale)
pkg_check_modules(SPDLOG REQUIRED IMPORTED_TARGET spdlog)

add_executable(micecam_spike
    main.cpp
    spike_encoder.cpp
    spike_oak.cpp
)

target_link_libraries(micecam_spike PRIVATE
    PkgConfig::FFMPEG
    PkgConfig::SPDLOG
)

target_compile_definitions(micecam_spike PRIVATE
    $<$<BOOL:${DEPTHAI_FOUND}>:HAS_DEPTHAI>
)

if(DEPTHAI_FOUND)
    target_link_libraries(micecam_spike PRIVATE depthai::core)
    if(APPLE)
        target_link_libraries(micecam_spike PRIVATE
            "-framework CoreFoundation"
            "-framework IOKit"
        )
    endif()
endif()
