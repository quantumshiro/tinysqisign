#!/bin/sh
# Build and audit the deterministic D13 Level-I/RADIX32 K/S/V image.
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
sdk_root=${PICO_SDK_PATH:-"$project_root/external/pico-sdk"}
build_root=${SQISIGN_RP2350_D13_BUILD_ROOT:-"$project_root/build-rp2350-ksv-d13"}
toolchain_root=${ARM_TOOLCHAIN_ROOT:?Set ARM_TOOLCHAIN_ROOT to the Arm GNU Toolchain directory}
compact_root=${SQISIGN_D13_SOURCE_ROOT:-"$project_root/work/compact-d13"}
compact_root=$(CDPATH= cd -- "$compact_root" && pwd -P)
picotool_cmake_dir=${PICOTOOL_CMAKE_DIR:?Set PICOTOOL_CMAKE_DIR to the picotool CMake package directory}
expected_compact_commit=71099e0827d3f0a3b3c705d2eda592c401e0d57d
expected_compact_tree=8761bccb5b14172e21d7228878fb3fc9379db5c4
source_commit=$(git -C "$project_root" rev-parse HEAD)
source_dirty=0

if test -n "$(git -C "$project_root" status --porcelain)"; then
    source_dirty=1
    if test "${SQISIGN_ALLOW_DIRTY_ROOT:-0}" != 1; then
        echo 'project root must be clean for the final D13 firmware build' >&2
        exit 1
    fi
fi

test "$(git -C "$compact_root" rev-parse 'HEAD^{tree}')" = "$expected_compact_tree"
test -z "$(git -C "$compact_root" status --porcelain)"
test -f "$sdk_root/pico_sdk_init.cmake"
test -x "$toolchain_root/bin/arm-none-eabi-gcc"
test "$("$toolchain_root/bin/arm-none-eabi-gcc" -dumpfullversion)" = 15.2.1
test -f "$picotool_cmake_dir/picotoolConfig.cmake"

PATH="$toolchain_root/bin:$PATH" \
PICO_SDK_PATH="$sdk_root" \
PICO_TOOLCHAIN_PATH="$toolchain_root" \
cmake \
    -S "$project_root" \
    -B "$build_root" \
    -G Ninja \
    -DPICO_BOARD=pico2 \
    -DPICO_PLATFORM=rp2350-arm-s \
    -DPICO_TOOLCHAIN_PATH="$toolchain_root" \
    -Dpicotool_DIR="$picotool_cmake_dir" \
    -DSQISIGN_KSV_SOURCE_ROOT="$compact_root" \
    -DSQISIGN_COMPACT_D13_COMMIT="$expected_compact_commit" \
    -DSQISIGN_FIRMWARE_GIT_COMMIT="$source_commit" \
    -DSQISIGN_FIRMWARE_DIRTY="$source_dirty" \
    -DSQISIGN_KSV_PROFILE_D13=ON \
    -DSQISIGN_OPERATION_WORKSPACE_BYTES=172080 \
    -DCMAKE_BUILD_TYPE=Release

PATH="$toolchain_root/bin:$PATH" \
SQISIGN_RP2350_KSV_AUDIT_MODE=D13 \
cmake --build "$build_root" --target sqisign_rp2350_ksv --parallel

crypto_su_root="$build_root/src/sqisign/ksv_lvl1"
crypto_su_count=$(find "$crypto_su_root" -name '*.su' -type f | wc -l | tr -d ' ')
test "$crypto_su_count" -eq 52
dynamic_rows=$(find "$build_root" -name '*.su' -type f -print0 | \
    xargs -0 awk '$NF != "static" { print FILENAME ":" $0 }')
if test -n "$dynamic_rows"; then
    printf 'dynamic stack-usage record(s) in D13 firmware build:\n%s\n' \
        "$dynamic_rows" >&2
    exit 1
fi

elf="$build_root/src/platform/rp2350/sqisign_rp2350_ksv.elf"
SQISIGN_RP2350_KSV_AUDIT_MODE=D13 \
    "$project_root/scripts/audit_rp2350_ksv_elf.sh" \
    "$toolchain_root/bin/arm-none-eabi-nm" \
    "$toolchain_root/bin/arm-none-eabi-objdump" \
    "$elf"

printf 'RP2350 D13 KeyGen+Sign+Verify build PASS\nelf=%s\nuf2=%s\nmap=%s.map\n' \
    "$elf" "${elf%.elf}.uf2" "$elf"
