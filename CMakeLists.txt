cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED ENV{PICO_SDK_PATH})
    message(FATAL_ERROR "PICO_SDK_PATH must name the frozen Pico SDK checkout")
endif()

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(tinysqisign_rp2350 C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(src/sqisign/ksv_lvl1)
add_subdirectory(src/platform/rp2350)

