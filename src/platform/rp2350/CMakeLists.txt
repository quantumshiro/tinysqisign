if(NOT DEFINED SQISIGN_COMPACT_D13_COMMIT)
    set(SQISIGN_COMPACT_D13_COMMIT "71099e0827d3f0a3b3c705d2eda592c401e0d57d")
endif()
if(NOT DEFINED SQISIGN_FIRMWARE_GIT_COMMIT)
    set(SQISIGN_FIRMWARE_GIT_COMMIT "unknown")
endif()
if(NOT DEFINED SQISIGN_FIRMWARE_DIRTY)
    set(SQISIGN_FIRMWARE_DIRTY 1)
endif()

add_executable(sqisign_rp2350_ksv
    call_on_psp.S
    keygen_sign_kat.c
)

target_link_libraries(sqisign_rp2350_ksv PRIVATE
    hardware_clocks
    hardware_structs
    pico_stdlib
    sqisign_ksv_lvl1_ref32
)

target_compile_definitions(sqisign_rp2350_ksv PRIVATE
    PICO_HEAP_SIZE=0
    SQISIGN_COMPACT_D13_COMMIT="${SQISIGN_COMPACT_D13_COMMIT}"
    SQISIGN_ENABLE_VERIFY
    SQISIGN_KSV_PROFILE_D13
    SQISIGN_OPERATION_WORKSPACE_BYTES=172080
    SQISIGN_FIRMWARE_DIRTY=${SQISIGN_FIRMWARE_DIRTY}
    SQISIGN_FIRMWARE_GIT_COMMIT="${SQISIGN_FIRMWARE_GIT_COMMIT}"
    SQISIGN_PICO_BOARD="${PICO_BOARD}"
    SQISIGN_PICO_PLATFORM="${PICO_PLATFORM}"
)

target_compile_options(sqisign_rp2350_ksv PRIVATE
    -fstack-usage
    -Wall
    -Wextra
    -Werror
    -Wno-unused-function
    -Wframe-larger-than=1024
)

add_custom_command(TARGET sqisign_rp2350_ksv POST_BUILD
    COMMAND
        ${PROJECT_SOURCE_DIR}/scripts/audit_rp2350_ksv_elf.sh
        ${CMAKE_NM}
        ${CMAKE_OBJDUMP}
        $<TARGET_FILE:sqisign_rp2350_ksv>
    VERBATIM
)

pico_enable_stdio_usb(sqisign_rp2350_ksv 1)
pico_enable_stdio_uart(sqisign_rp2350_ksv 0)
pico_add_extra_outputs(sqisign_rp2350_ksv)

