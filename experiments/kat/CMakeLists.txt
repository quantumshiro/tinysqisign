cmake_minimum_required(VERSION 3.13)

project(sqisign_d13_lowmem_kat LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SQISIGN_D13_SOURCE_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../../work/compact-d13"
    CACHE PATH "Frozen D13 Compact-SQIsign source tree")
get_filename_component(SQISIGN_D13_SOURCE_ROOT
    "${SQISIGN_D13_SOURCE_ROOT}" ABSOLUTE)

if(NOT EXISTS "${SQISIGN_D13_SOURCE_ROOT}/src/sqisign_lowmem.c")
    message(FATAL_ERROR "D13 Compact-SQIsign source tree is missing")
endif()

# Build precisely the Level-I, RADIX32, allocator-free caller-owned closure.
# These cache assignments precede add_subdirectory so that the frozen D13
# project configures the same implementation family as the RP2350 image.
set(ENABLE_SIGN ON CACHE BOOL "" FORCE)
set(ENABLE_GMP OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTS ON CACHE BOOL "" FORCE)
set(ENABLE_KAT_TESTS OFF CACHE BOOL "" FORCE)
set(GF_RADIX 32 CACHE STRING "" FORCE)
set(SQISIGN_BUILD_TYPE ref CACHE STRING "" FORCE)
set(SQISIGN_LOWMEM_ONLY ON CACHE BOOL "" FORCE)

add_subdirectory("${SQISIGN_D13_SOURCE_ROOT}" compact-d13)

add_executable(sqisign_d13_lowmem_kat d13_lowmem_kat.c)
target_link_libraries(sqisign_d13_lowmem_kat PRIVATE sqisign_lvl1_test)
target_include_directories(sqisign_d13_lowmem_kat PRIVATE
    "${SQISIGN_D13_SOURCE_ROOT}/src/nistapi/lvl1"
)
target_compile_definitions(sqisign_d13_lowmem_kat PRIVATE
    ENABLE_SIGN
    RADIX_32
    SQISIGN_BUILD_TYPE_REF
    SQISIGN_GF_IMPL_REF
    SQISIGN_LOWMEM_ONLY
)
target_compile_options(sqisign_d13_lowmem_kat PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wno-unused-function
)
