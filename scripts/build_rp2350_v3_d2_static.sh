#!/bin/sh
# Build the p324_3 generated-source static-stack audit prototype for RP2350.
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
sdk_root=${PICO_SDK_PATH:-"$project_root/external/pico-sdk"}
v3_root=${SQISIGN_V3_SOURCE_ROOT:-"$project_root/work/v3-static-stack-d2"}
build_root=${SQISIGN_RP2350_V3_BUILD_ROOT:-"$project_root/build-rp2350-v3-d2-static"}
toolchain_root=${ARM_TOOLCHAIN_ROOT:-/Users/hiro/workspace/raspico2/arm-gnu-toolchain-extracted}
picotool_cmake_dir=${PICOTOOL_CMAKE_DIR:-"$project_root/build-rp2350-ksv/_deps/picotool"}
expected_v3_base_commit=6d017708db403bf83977fa70770fc4f7f9e9ff21

v3_root=$(CDPATH= cd -- "$v3_root" && pwd -P)
generated_root="$v3_root/src/pqm4/sqisign_p324_3/m4f"
v3_commit=$(git -C "$v3_root" rev-parse HEAD)
git -C "$v3_root" merge-base --is-ancestor \
    "$expected_v3_base_commit" "$v3_commit"
test "$(git -C "$v3_root" rev-list --count "$expected_v3_base_commit..$v3_commit")" -eq 1
test -f "$generated_root/pqm4_api.c"
test -f "$sdk_root/pico_sdk_init.cmake"
test -x "$toolchain_root/bin/arm-none-eabi-gcc"
test -f "$picotool_cmake_dir/picotoolConfig.cmake"

# The upstream pqm4 package and this experiment are generated sources, so bind
# the exact tree independently of the upstream commit's tracked-file status.
generated_digest=$(
    CDPATH= cd -- "$generated_root"
    find . -type f -print | LC_ALL=C sort | while IFS= read -r file; do
        shasum -a 256 "$file"
    done | shasum -a 256 | awk '{print $1}'
)

v3_dirty=0
if test -n "$(git -C "$v3_root" status --porcelain --untracked-files=no)"; then
    v3_dirty=1
fi
test "$v3_dirty" -eq 0

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
    -DSQISIGN_V3_IMAGE_KIND=d2-static \
    -DSQISIGN_V3_REQUIRE_STATIC_STACK=ON \
    -DSQISIGN_V3_SOURCE_ROOT="$v3_root" \
    -DSQISIGN_V3_SOURCE_COMMIT="$v3_commit" \
    -DSQISIGN_V3_SOURCE_DIRTY="$v3_dirty" \
    -DSQISIGN_V3_GENERATED_TREE_SHA256="$generated_digest" \
    -DSQISIGN_FIRMWARE_GIT_COMMIT="$firmware_commit" \
    -DSQISIGN_FIRMWARE_DIRTY="$firmware_dirty" \
    -DCMAKE_BUILD_TYPE=Release

PATH="$toolchain_root/bin:$PATH" \
cmake --build "$build_root" --target sqisign_rp2350_v3_d2_static --parallel

elf="$build_root/sqisign_rp2350_v3_d2_static.elf"
test -f "$elf"
"$project_root/scripts/audit_rp2350_v3_elf.sh" \
    "$toolchain_root/bin/arm-none-eabi-nm" \
    "$toolchain_root/bin/arm-none-eabi-objdump" \
    "$toolchain_root/bin/arm-none-eabi-size" \
    "$elf"

if find "$build_root/CMakeFiles/sqisign_v3_p324_3_m4f.dir" -name '*.su' \
    -exec grep -H -E 'dynamic(,bounded)?$' {} + | grep -q .; then
    echo 'FAIL: dynamic stack-usage record remains' >&2
    exit 1
fi

printf 'RP2350 SQIsign v3 D2 static-stack build PASS\nelf=%s\nuf2=%s\nmap=%s.map\ngenerated_tree_sha256=%s\n' \
    "$elf" "${elf%.elf}.uf2" "$elf" "$generated_digest"
