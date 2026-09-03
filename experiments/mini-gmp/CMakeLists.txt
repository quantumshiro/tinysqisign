cmake_minimum_required(VERSION 3.20)
project(SQIsignMiniGMPComparison LANGUAGES C ASM)

if(NOT SQISIGN_SOURCE_ROOT)
    message(FATAL_ERROR "SQISIGN_SOURCE_ROOT must name the frozen official SQIsign source")
endif()

set(ENABLE_SIGN ON CACHE BOOL "" FORCE)
set(ENABLE_TESTS ON CACHE BOOL "" FORCE)
set(GF_RADIX 32 CACHE STRING "" FORCE)
set(SQISIGN_BUILD_TYPE ref CACHE STRING "" FORCE)
set(SQISIGN_MINI_GMP_POOL_MAX_BYTES 317696 CACHE STRING
    "Compile-time capacity of the comparison allocator backing array")

add_subdirectory("${SQISIGN_SOURCE_ROOT}" sqisign)

add_executable(mini_gmp_compare
    mini_gmp_compare.c
    mini_gmp_allocator.c)
target_link_libraries(mini_gmp_compare PRIVATE sqisign_lvl1_test_nistapi)
target_include_directories(mini_gmp_compare PRIVATE
    "${SQISIGN_SOURCE_ROOT}/include"
    "${SQISIGN_SOURCE_ROOT}/src/nistapi/lvl1"
    "${SQISIGN_SOURCE_ROOT}/src/mini-gmp")
target_compile_definitions(mini_gmp_compare PRIVATE
    ENABLE_SIGN
    SQISIGN_VARIANT=lvl1
    SQISIGN_BUILD_TYPE_REF
    SQISIGN_GF_IMPL_REF
    MINI_GMP_POOL_MAX_BYTES=${SQISIGN_MINI_GMP_POOL_MAX_BYTES}
    MINI_GMP_COMPARISON_BACKEND="${GMP_LIBRARY}")
if(GMP_LIBRARY STREQUAL "MINI")
    target_compile_definitions(mini_gmp_compare PRIVATE MINI_GMP)

    add_executable(mini_gmp_pool_kat_lvl1
        "${SQISIGN_SOURCE_ROOT}/test/test_kat.c"
        mini_gmp_pool_kat_hooks.c
        mini_gmp_allocator.c)
    target_link_libraries(mini_gmp_pool_kat_lvl1 PRIVATE sqisign_lvl1_test)
    target_include_directories(mini_gmp_pool_kat_lvl1 PRIVATE
        "${SQISIGN_SOURCE_ROOT}/include"
        "${SQISIGN_SOURCE_ROOT}/src/nistapi/lvl1"
        "${SQISIGN_SOURCE_ROOT}/src/mini-gmp"
        "${SQISIGN_SOURCE_ROOT}/src/quaternion/ref/generic/include"
        "${SQISIGN_SOURCE_ROOT}/src/precomp/ref/lvl1/include"
        "${SQISIGN_SOURCE_ROOT}/src/gf/ref/lvl1/include"
        "${SQISIGN_SOURCE_ROOT}/src/ec/ref/include"
        "${SQISIGN_SOURCE_ROOT}/src/common/generic/include"
        "${SQISIGN_SOURCE_ROOT}/src/id2iso/ref/include"
        "${SQISIGN_SOURCE_ROOT}/src/signature/ref/include")
    target_compile_definitions(mini_gmp_pool_kat_lvl1 PRIVATE
        ENABLE_SIGN
        MINI_GMP
        SQISIGN_VARIANT=lvl1
        SQISIGN_BUILD_TYPE_REF
        SQISIGN_GF_IMPL_REF
        MINI_GMP_POOL_MAX_BYTES=${SQISIGN_MINI_GMP_POOL_MAX_BYTES})
endif()
