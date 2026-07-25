cmake_minimum_required(VERSION 3.18.1)

project(pikmin_gps_copy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMMON_FLAGS "-Werror=format -fdata-sections -ffunction-sections -O2 -fvisibility=hidden")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -fno-exceptions -fno-rtti -fvisibility-inlines-hidden")
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -ffixed-x18 -Wl,--hash-style=both -Wl,--gc-sections -Wl,--strip-all")

include_directories(xdl/include)
aux_source_directory(xdl XDL_SOURCES)

add_library(pikmin_gps_copy SHARED
    main.cpp
    hack.cpp
    And64InlineHook.cpp
    ${XDL_SOURCES})

target_link_libraries(pikmin_gps_copy log)
