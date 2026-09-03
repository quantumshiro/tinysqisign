#!/bin/sh
# Build a ten-vector official-v3 or D1 image at placement A or B.
set -eu

test "$#" -eq 2
kind=$1
placement=$2
case "$kind" in
    baseline|d1) ;;
    *) echo 'kind must be baseline or d1' >&2; exit 2 ;;
esac
case "$placement" in
    a|b) ;;
    *) echo 'placement must be a or b' >&2; exit 2 ;;
esac

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
sdk_root=${PICO_SDK_PATH:-"$project_root/external/pico-sdk"}
if test "$kind" = baseline; then
    default_v3_root="$project_root/work/official-v3"
else
    default_v3_root="$project_root/work/v3-lowmem-d1"
fi
v3_root=${SQISIGN_V3_SOURCE_ROOT:-"$default_v3_root"}
build_root=${SQISIGN_RP2350_V3_BUILD_ROOT:-"$project_root/build-rp2350-v3-${kind}-multi-${placement}"}
toolchain_root=${ARM_TOOLCHAIN_ROOT:?Set ARM_TOOLCHAIN_ROOT to the Arm GNU Toolchain directory}
picotool_cmake_dir=${PICOTOOL_CMAKE_DIR:?Set PICOTOOL_CMAKE_DIR to the picotool CMake package directory}
base_commit=6d017708db403bf83977fa70770fc4f7f9e9ff21

v3_root=$(CDPATH= cd -- "$v3_root" && pwd -P)
generated_root="$v3_root/src/pqm4/sqisign_p324_3/m4f"
v3_commit=$(git -C "$v3_root" rev-parse HEAD)
if test "$kind" = baseline; then
    test "$v3_commit" = "$base_commit"
else
    git -C "$v3_root" merge-base --is-ancestor "$base_commit" "$v3_commit"
    test "$(git -C "$v3_root" rev-list --count "$base_commit..$v3_commit")" -eq 1
fi
test -z "$(git -C "$v3_root" status --porcelain --untracked-files=no)"
test -f "$generated_root/pqm4_api.c"
test -f "$v3_root/KAT/PQCsignKAT_270_SQIsign_p324_3.rsp"
test -f "$sdk_root/pico_sdk_init.cmake"
test -x "$toolchain_root/bin/arm-none-eabi-gcc"
test -f "$picotool_cmake_dir/picotoolConfig.cmake"

generated_digest=$(
    CDPATH= cd -- "$generated_root"
    find . -type f -print | LC_ALL=C sort | while IFS= read -r file; do
        shasum -a 256 "$file"
    done | shasum -a 256 | awk '{print $1}'
)

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
    -DSQISIGN_V3_IMAGE_KIND="${kind}-multi" \
    -DSQISIGN_V3_PLACEMENT_VARIANT="$placement" \
    -DSQISIGN_V3_SOURCE_ROOT="$v3_root" \
    -DSQISIGN_V3_SOURCE_COMMIT="$v3_commit" \
    -DSQISIGN_V3_SOURCE_DIRTY=0 \
    -DSQISIGN_V3_GENERATED_TREE_SHA256="$generated_digest" \
    -DSQISIGN_FIRMWARE_GIT_COMMIT="$firmware_commit" \
    -DSQISIGN_FIRMWARE_DIRTY="$firmware_dirty" \
    -DCMAKE_BUILD_TYPE=Release

target="sqisign_rp2350_v3_${kind}_multi_${placement}"
PATH="$toolchain_root/bin:$PATH" \
cmake --build "$build_root" --target "$target" --parallel

elf="$build_root/$target.elf"
test -f "$elf"
"$project_root/scripts/audit_rp2350_v3_elf.sh" \
    "$toolchain_root/bin/arm-none-eabi-nm" \
    "$toolchain_root/bin/arm-none-eabi-objdump" \
    "$toolchain_root/bin/arm-none-eabi-size" \
    "$elf"

printf 'RP2350 SQIsign v3 multi-vector build PASS\nkind=%s\nplacement=%s\nelf=%s\nuf2=%s\ngenerated_tree_sha256=%s\n' \
    "$kind" "$placement" "$elf" "${elf%.elf}.uf2" "$generated_digest"
