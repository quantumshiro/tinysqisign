cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED ENV{PICO_SDK_PATH})
    message(FATAL_ERROR "PICO_SDK_PATH must name the frozen Pico SDK checkout")
endif()

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(sqisign_v3_rp2350 C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

get_filename_component(SQISIGN_RESEARCH_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(SQISIGN_V3_SOURCE_ROOT
    "${SQISIGN_RESEARCH_ROOT}/work/official-v3"
    CACHE PATH "SQIsign v3 research worktree")
set(SQISIGN_V3_GENERATED_ROOT
    "${SQISIGN_V3_SOURCE_ROOT}/src/pqm4/sqisign_p324_3/m4f")

if(NOT DEFINED SQISIGN_V3_SOURCE_COMMIT)
    set(SQISIGN_V3_SOURCE_COMMIT "unknown")
endif()
if(NOT DEFINED SQISIGN_V3_SOURCE_DIRTY)
    set(SQISIGN_V3_SOURCE_DIRTY 1)
endif()
if(NOT DEFINED SQISIGN_FIRMWARE_GIT_COMMIT)
    set(SQISIGN_FIRMWARE_GIT_COMMIT "unknown")
endif()
if(NOT DEFINED SQISIGN_FIRMWARE_DIRTY)
    set(SQISIGN_FIRMWARE_DIRTY 1)
endif()

if(NOT EXISTS "${SQISIGN_V3_GENERATED_ROOT}/pqm4_api.c")
    message(FATAL_ERROR
        "Generate the official v3 p324_3/m4f package before configuring")
endif()

file(GLOB SQISIGN_V3_GENERATED_SOURCES CONFIGURE_DEPENDS
    "${SQISIGN_V3_GENERATED_ROOT}/*.c")
list(SORT SQISIGN_V3_GENERATED_SOURCES)

add_library(sqisign_v3_p324_3_m4f STATIC
    ${SQISIGN_V3_GENERATED_SOURCES}
    ${SQISIGN_V3_SOURCE_ROOT}/src/common/generic/fips202.c
    ${SQISIGN_V3_SOURCE_ROOT}/src/common/ref/aes_c.c
    ${SQISIGN_V3_SOURCE_ROOT}/src/common/ref/randombytes_ctrdrbg.c
)

target_include_directories(sqisign_v3_p324_3_m4f PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${SQISIGN_V3_GENERATED_ROOT}
    ${SQISIGN_V3_SOURCE_ROOT}/src/common/generic/include
    ${SQISIGN_V3_SOURCE_ROOT}/src/common/ref/include
)

target_compile_definitions(sqisign_v3_p324_3_m4f PUBLIC
    DISABLE_NAMESPACING
    ENABLE_SIGN
    NDEBUG
    RADIX_32
    RANDOMBYTES_C
    SQISIGN_BUILD_TYPE_REF
    SQISIGN_GF_IMPL_SAT32
    SQISIGN_SINGLE_THREADED
    SQISIGN_VARIANT=p324_3
    TARGET_ARM
)

target_compile_options(sqisign_v3_p324_3_m4f PRIVATE
    -O3
    -fno-strict-aliasing
    -fstack-usage
    -ffunction-sections
    -fdata-sections
    -Wvla
    -Walloca
    -Wall
    -Wextra
    -Werror
    -Wno-error=attributes
    -Wno-error=missing-braces
    -Wno-error=unused-variable
    -Wno-error=vla
    -Wno-unused-function
    -Wno-unused-parameter
    -Wno-maybe-uninitialized
)

set_target_properties(sqisign_v3_p324_3_m4f PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION FALSE
    POSITION_INDEPENDENT_CODE FALSE
)

set(SQISIGN_V3_IMAGE_KIND "baseline" CACHE STRING
    "v3 firmware identity: baseline or d1")
if(SQISIGN_V3_IMAGE_KIND STREQUAL "baseline")
    set(SQISIGN_V3_TARGET sqisign_rp2350_v3_baseline)
    set(SQISIGN_V3_BANNER "SQISIGN_RP2350_V3_BASELINE v1")
    set(SQISIGN_V3_ALIVE "SQISIGN_RP2350_V3_BASELINE alive")
    set(SQISIGN_V3_DESCRIPTION
        "Official v3 m4f source, deterministic KAT, guarded PSP")
elseif(SQISIGN_V3_IMAGE_KIND STREQUAL "d1")
    set(SQISIGN_V3_TARGET sqisign_rp2350_v3_d1)
    set(SQISIGN_V3_BANNER "SQISIGN_RP2350_V3_D1 v1")
    set(SQISIGN_V3_ALIVE "SQISIGN_RP2350_V3_D1 alive")
    set(SQISIGN_V3_DESCRIPTION
        "v3 D1 lifetime-scheduled LLL, deterministic KAT, guarded PSP")
else()
    message(FATAL_ERROR "Unsupported SQISIGN_V3_IMAGE_KIND")
endif()

add_executable(${SQISIGN_V3_TARGET}
    ${SQISIGN_RESEARCH_ROOT}/src/platform/rp2350/call_on_psp.S
    v3_ksv_kat.c
)

target_link_libraries(${SQISIGN_V3_TARGET} PRIVATE
    hardware_clocks
    hardware_structs
    pico_stdlib
    sqisign_v3_p324_3_m4f
)

target_compile_definitions(${SQISIGN_V3_TARGET} PRIVATE
    PICO_HEAP_SIZE=0
    SQISIGN_V3_BANNER="${SQISIGN_V3_BANNER}"
    SQISIGN_V3_ALIVE="${SQISIGN_V3_ALIVE}"
    SQISIGN_V3_DESCRIPTION="${SQISIGN_V3_DESCRIPTION}"
    SQISIGN_V3_IMAGE_KIND="${SQISIGN_V3_IMAGE_KIND}"
    SQISIGN_V3_SOURCE_COMMIT="${SQISIGN_V3_SOURCE_COMMIT}"
    SQISIGN_V3_SOURCE_DIRTY=${SQISIGN_V3_SOURCE_DIRTY}
    SQISIGN_FIRMWARE_GIT_COMMIT="${SQISIGN_FIRMWARE_GIT_COMMIT}"
    SQISIGN_FIRMWARE_DIRTY=${SQISIGN_FIRMWARE_DIRTY}
    SQISIGN_PICO_BOARD="${PICO_BOARD}"
    SQISIGN_PICO_PLATFORM="${PICO_PLATFORM}"
)

target_compile_options(${SQISIGN_V3_TARGET} PRIVATE
    -O3
    -fstack-usage
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    -Werror
)

target_link_options(${SQISIGN_V3_TARGET} PRIVATE
    -Wl,--gc-sections
)

pico_enable_stdio_usb(${SQISIGN_V3_TARGET} 1)
pico_enable_stdio_uart(${SQISIGN_V3_TARGET} 0)
pico_add_extra_outputs(${SQISIGN_V3_TARGET})
