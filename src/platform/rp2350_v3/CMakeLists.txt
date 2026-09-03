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
option(SQISIGN_V3_REQUIRE_STATIC_STACK
    "Reject variable-length arrays and alloca in the generated library" OFF)

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
if(NOT DEFINED SQISIGN_V3_GENERATED_TREE_SHA256)
    set(SQISIGN_V3_GENERATED_TREE_SHA256 "unknown")
endif()

if(NOT EXISTS "${SQISIGN_V3_GENERATED_ROOT}/pqm4_api.c")
    message(FATAL_ERROR
        "Generate the official v3 p324_3/m4f package before configuring")
endif()

file(GLOB SQISIGN_V3_GENERATED_SOURCES CONFIGURE_DEPENDS
    "${SQISIGN_V3_GENERATED_ROOT}/*.c")
list(SORT SQISIGN_V3_GENERATED_SOURCES)
# The upstream generic helper exports secure_free() and therefore leaves an
# otherwise unreachable free() reference in the archive.  Embedded runs have
# no allocator: replace it with a clear-and-trap contract for accidental use.
list(REMOVE_ITEM SQISIGN_V3_GENERATED_SOURCES
    "${SQISIGN_V3_GENERATED_ROOT}/mem.c")

add_library(sqisign_v3_p324_3_m4f STATIC
    ${SQISIGN_V3_GENERATED_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/mem_noheap.c
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

# Keep these after the compatibility exceptions above: for the static-stack
# experiment, a VLA or alloca is a build failure rather than a warning.
if(SQISIGN_V3_REQUIRE_STATIC_STACK)
    target_compile_options(sqisign_v3_p324_3_m4f PRIVATE
        -Werror=vla
        -Werror=alloca
        -fno-builtin-fprintf
        -fno-builtin-fwrite
    )
endif()

set_target_properties(sqisign_v3_p324_3_m4f PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION FALSE
    POSITION_INDEPENDENT_CODE FALSE
)

set(SQISIGN_V3_IMAGE_KIND "baseline" CACHE STRING
    "v3 firmware identity: baseline, d1, d2-static, multi, or fixed-key SCA")
set(SQISIGN_V3_PLACEMENT_VARIANT "a" CACHE STRING
    "linked-code placement variant for multi-vector images: a or b")
if(NOT SQISIGN_V3_PLACEMENT_VARIANT MATCHES "^[ab]$")
    message(FATAL_ERROR "SQISIGN_V3_PLACEMENT_VARIANT must be a or b")
endif()
string(TOUPPER "${SQISIGN_V3_PLACEMENT_VARIANT}"
    SQISIGN_V3_PLACEMENT_VARIANT_UPPER)
set(SQISIGN_V3_IS_MULTI OFF)
set(SQISIGN_V3_IS_FIXED_KEY_SCA OFF)
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
elseif(SQISIGN_V3_IMAGE_KIND STREQUAL "d2-static")
    set(SQISIGN_V3_TARGET sqisign_rp2350_v3_d2_static)
    set(SQISIGN_V3_BANNER "SQISIGN_RP2350_V3_D2_STATIC v1")
    set(SQISIGN_V3_ALIVE "SQISIGN_RP2350_V3_D2_STATIC alive")
    set(SQISIGN_V3_DESCRIPTION
        "v3 p324_3 fixed-bound generated frames, deterministic KAT, guarded PSP")
elseif(SQISIGN_V3_IMAGE_KIND STREQUAL "baseline-multi")
    set(SQISIGN_V3_IS_MULTI ON)
    set(SQISIGN_V3_TARGET
        sqisign_rp2350_v3_baseline_multi_${SQISIGN_V3_PLACEMENT_VARIANT})
    set(SQISIGN_V3_BANNER
        "SQISIGN_RP2350_V3_BASELINE_MULTI_${SQISIGN_V3_PLACEMENT_VARIANT_UPPER} v1")
    set(SQISIGN_V3_ALIVE
        "SQISIGN_RP2350_V3_BASELINE_MULTI_${SQISIGN_V3_PLACEMENT_VARIANT_UPPER} alive")
    set(SQISIGN_V3_DESCRIPTION
        "Official v3 m4f, ten-vector K/S/V plus negative verify, placement audit")
elseif(SQISIGN_V3_IMAGE_KIND STREQUAL "d1-multi")
    set(SQISIGN_V3_IS_MULTI ON)
    set(SQISIGN_V3_TARGET
        sqisign_rp2350_v3_d1_multi_${SQISIGN_V3_PLACEMENT_VARIANT})
    set(SQISIGN_V3_BANNER
        "SQISIGN_RP2350_V3_D1_MULTI_${SQISIGN_V3_PLACEMENT_VARIANT_UPPER} v1")
    set(SQISIGN_V3_ALIVE
        "SQISIGN_RP2350_V3_D1_MULTI_${SQISIGN_V3_PLACEMENT_VARIANT_UPPER} alive")
    set(SQISIGN_V3_DESCRIPTION
        "v3 D1 lifetime-scheduled LLL, ten-vector K/S/V plus negative verify, placement audit")
elseif(SQISIGN_V3_IMAGE_KIND STREQUAL "baseline-sca-key")
    set(SQISIGN_V3_IS_FIXED_KEY_SCA ON)
    set(SQISIGN_V3_TARGET sqisign_rp2350_v3_baseline_sca_key)
    set(SQISIGN_V3_BANNER "SQISIGN_RP2350_V3_BASELINE_SCA_KEY v1")
    set(SQISIGN_V3_ALIVE "SQISIGN_RP2350_V3_BASELINE_SCA_KEY alive")
    set(SQISIGN_V3_DESCRIPTION
        "Official v3 m4f, fixed-message fixed-RNG ten-key timing screen")
elseif(SQISIGN_V3_IMAGE_KIND STREQUAL "d1-sca-key")
    set(SQISIGN_V3_IS_FIXED_KEY_SCA ON)
    set(SQISIGN_V3_TARGET sqisign_rp2350_v3_d1_sca_key)
    set(SQISIGN_V3_BANNER "SQISIGN_RP2350_V3_D1_SCA_KEY v1")
    set(SQISIGN_V3_ALIVE "SQISIGN_RP2350_V3_D1_SCA_KEY alive")
    set(SQISIGN_V3_DESCRIPTION
        "v3 D1 m4f, fixed-message fixed-RNG ten-key timing screen")
else()
    message(FATAL_ERROR "Unsupported SQISIGN_V3_IMAGE_KIND")
endif()

if(SQISIGN_V3_IS_MULTI OR SQISIGN_V3_IS_FIXED_KEY_SCA)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(SQISIGN_V3_KAT_HEADER
        "${CMAKE_CURRENT_BINARY_DIR}/generated/v3_kat_subset.h")
    set(SQISIGN_V3_KAT_RSP
        "${SQISIGN_V3_SOURCE_ROOT}/KAT/PQCsignKAT_270_SQIsign_p324_3.rsp")
    add_custom_command(
        OUTPUT "${SQISIGN_V3_KAT_HEADER}"
        COMMAND ${Python3_EXECUTABLE}
            "${SQISIGN_RESEARCH_ROOT}/scripts/generate_v3_kat_subset.py"
            --rsp "${SQISIGN_V3_KAT_RSP}"
            --output "${SQISIGN_V3_KAT_HEADER}"
            --count 10
        DEPENDS
            "${SQISIGN_RESEARCH_ROOT}/scripts/generate_v3_kat_subset.py"
            "${SQISIGN_V3_KAT_RSP}"
        VERBATIM
    )
    if(SQISIGN_V3_IS_MULTI)
        set(SQISIGN_V3_KSV_SOURCE v3_ksv_multi.c)
        set(SQISIGN_V3_PLACEMENT_SOURCE
            placement_${SQISIGN_V3_PLACEMENT_VARIANT}.S)
    else()
        set(SQISIGN_V3_KSV_SOURCE v3_sca_fixed_key.c)
        set(SQISIGN_V3_PLACEMENT_SOURCE "")
    endif()
else()
    set(SQISIGN_V3_KAT_HEADER "")
    set(SQISIGN_V3_KSV_SOURCE v3_ksv_kat.c)
    set(SQISIGN_V3_PLACEMENT_SOURCE "")
endif()

set(SQISIGN_V3_DIAGNOSTIC_TRAP_SOURCE "")
if(SQISIGN_V3_REQUIRE_STATIC_STACK)
    set(SQISIGN_V3_DIAGNOSTIC_TRAP_SOURCE diagnostic_traps.c)
endif()

add_executable(${SQISIGN_V3_TARGET}
    ${SQISIGN_RESEARCH_ROOT}/src/platform/rp2350/call_on_psp.S
    allocator_traps.c
    ${SQISIGN_V3_DIAGNOSTIC_TRAP_SOURCE}
    ${SQISIGN_V3_PLACEMENT_SOURCE}
    ${SQISIGN_V3_KSV_SOURCE}
    ${SQISIGN_V3_KAT_HEADER}
)

set_source_files_properties(allocator_traps.c PROPERTIES
    COMPILE_OPTIONS "-fno-builtin"
)
if(SQISIGN_V3_REQUIRE_STATIC_STACK)
    set_source_files_properties(diagnostic_traps.c PROPERTIES
        COMPILE_OPTIONS "-fno-builtin")
endif()

target_link_libraries(${SQISIGN_V3_TARGET} PRIVATE
    hardware_clocks
    hardware_structs
    hardware_xip_cache
    pico_stdlib
    sqisign_v3_p324_3_m4f
)

if(SQISIGN_V3_IS_MULTI OR SQISIGN_V3_IS_FIXED_KEY_SCA)
    target_include_directories(${SQISIGN_V3_TARGET} PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/generated")
endif()

target_compile_definitions(${SQISIGN_V3_TARGET} PRIVATE
    PICO_HEAP_SIZE=0
    SQISIGN_V3_BANNER="${SQISIGN_V3_BANNER}"
    SQISIGN_V3_ALIVE="${SQISIGN_V3_ALIVE}"
    SQISIGN_V3_DESCRIPTION="${SQISIGN_V3_DESCRIPTION}"
    SQISIGN_V3_IMAGE_KIND="${SQISIGN_V3_IMAGE_KIND}"
    SQISIGN_V3_PLACEMENT_VARIANT="${SQISIGN_V3_PLACEMENT_VARIANT}"
    SQISIGN_V3_SOURCE_COMMIT="${SQISIGN_V3_SOURCE_COMMIT}"
    SQISIGN_V3_SOURCE_DIRTY=${SQISIGN_V3_SOURCE_DIRTY}
    SQISIGN_V3_GENERATED_TREE_SHA256="${SQISIGN_V3_GENERATED_TREE_SHA256}"
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
