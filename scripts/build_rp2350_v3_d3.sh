#!/bin/sh
# Build the p324_3 two-function lifetime-scheduled image for RP2350.
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
sdk_root=${PICO_SDK_PATH:-"$project_root/external/pico-sdk"}
v3_root=${SQISIGN_V3_SOURCE_ROOT:-"$project_root/work/v3-lowmem-d3"}
build_root=${SQISIGN_RP2350_V3_BUILD_ROOT:-"$project_root/build-rp2350-v3-d3"}
toolchain_root=${ARM_TOOLCHAIN_ROOT:?Set ARM_TOOLCHAIN_ROOT to the Arm GNU Toolchain directory}
picotool_cmake_dir=${PICOTOOL_CMAKE_DIR:?Set PICOTOOL_CMAKE_DIR to the picotool CMake package directory}
expected_v3_commit=874658c64aa2e20f53b1f4d696144723d558ed5c

v3_root=$(CDPATH= cd -- "$v3_root" && pwd -P)
v3_commit=$(git -C "$v3_root" rev-parse HEAD)
test "$v3_commit" = "$expected_v3_commit"
test -f "$v3_root/src/pqm4/sqisign_p324_3/m4f/pqm4_api.c"
test -f "$sdk_root/pico_sdk_init.cmake"
test -x "$toolchain_root/bin/arm-none-eabi-gcc"
test -f "$picotool_cmake_dir/picotoolConfig.cmake"
test -z "$(git -C "$v3_root" status --porcelain --untracked-files=no)"

firmware_commit=$(git -C "$project_root" rev-parse HEAD)
firmware_dirty=0
if test -n "$(git -C "$project_root" status --porcelain)"; then
    firmware_dirty=1
fi

PATH="$toolchain_root/bin:$PATH" \
PICO_SDK_PATH="$sdk_root" \
PICO_TOOLCHAIN_PATH="$toolchain_root" \
cmake \
    -S "$project_root/src/platform/rp2350_v3" \
    -B "$build_root" \
    -G Ninja \
    -DPICO_BOARD=pico2 \
    -DPICO_PLATFORM=rp2350-arm-s \
    -DPICO_TOOLCHAIN_PATH="$toolchain_root" \
    -Dpicotool_DIR="$picotool_cmake_dir" \
    -DSQISIGN_V3_IMAGE_KIND=d3 \
    -DSQISIGN_V3_SOURCE_ROOT="$v3_root" \
    -DSQISIGN_V3_SOURCE_COMMIT="$v3_commit" \
    -DSQISIGN_V3_SOURCE_DIRTY=0 \
    -DSQISIGN_FIRMWARE_GIT_COMMIT="$firmware_commit" \
    -DSQISIGN_FIRMWARE_DIRTY="$firmware_dirty" \
    -DCMAKE_BUILD_TYPE=Release

PATH="$toolchain_root/bin:$PATH" \
cmake --build "$build_root" --target sqisign_rp2350_v3_d3 --parallel

elf="$build_root/sqisign_rp2350_v3_d3.elf"
test -f "$elf"
"$project_root/scripts/audit_rp2350_v3_elf.sh" \
    "$toolchain_root/bin/arm-none-eabi-nm" \
    "$toolchain_root/bin/arm-none-eabi-objdump" \
    "$toolchain_root/bin/arm-none-eabi-size" \
    "$elf"

printf 'RP2350 SQIsign v3 D3 build PASS\nelf=%s\nuf2=%s\nmap=%s.map\n' \
    "$elf" "${elf%.elf}.uf2" "$elf"
