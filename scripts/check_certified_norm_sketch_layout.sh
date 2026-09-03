#!/bin/sh
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
source_root=${SQISIGN_D13_SOURCE_ROOT:-"$project_root/work/compact-d13"}
arm_toolchain_root=${ARM_TOOLCHAIN_ROOT:?Set ARM_TOOLCHAIN_ROOT to the Arm GNU Toolchain directory}
arm_cc="$arm_toolchain_root/bin/arm-none-eabi-gcc"
arm_nm="$arm_toolchain_root/bin/arm-none-eabi-nm"
probe_source="$project_root/experiments/memory/certified_norm_sketch_layout.c"
probe_root=$(mktemp -d "${TMPDIR:-/tmp}/sqisign-certified-sketch-layout.XXXXXX")
trap 'rm -rf "$probe_root"' EXIT HUP INT TERM

test -f "$probe_source"
test -x "$arm_cc"
test -x "$arm_nm"

common_defines='-DENABLE_SIGN -DSQISIGN_BUILD_TYPE_REF -DSQISIGN_GF_IMPL_REF -DSQISIGN_VARIANT=lvl1'
common_includes="
  -I$source_root/include
  -I$source_root/src/common/generic/include
  -I$source_root/src/precomp/ref/lvl1/include
  -I$source_root/src/quaternion/ref/generic/include
  -I$source_root/src/quaternion/ref/generic/internal_quaternion_headers
  -I$source_root/src/mp/ref/generic/include
  -I$source_root/src/gf/ref/include
  -I$source_root/src/gf/ref/lvl1/include
  -I$source_root/src/ec/ref/include
  -I$source_root/src/hd/ref/include
  -I$source_root/src/id2iso/ref/include
  -I$source_root/src/verification/ref/include
  -I$source_root/src/signature/ref/include
"

# shellcheck disable=SC2086
cc -std=c11 -DHAVE_UINT128 -DRADIX_64 -DSQISIGN_TARGET_OS_UNIX \
    $common_defines $common_includes \
    "$probe_source" -o "$probe_root/layout-host"
host_output=$($probe_root/layout-host)
printf '%s\n' "$host_output"
printf '%s\n' "$host_output" | grep -qx 'candidates=14024 align=8'
printf '%s\n' "$host_output" | grep -qx 'phase=94920'
printf '%s\n' "$host_output" | grep -qx 'find_uv=172088 align=8'
printf '%s\n' "$host_output" | grep -qx 'operation=172088 align=8'

# shellcheck disable=SC2086
"$arm_cc" -mcpu=cortex-m33 -mthumb -mfloat-abi=soft -Os \
    -std=gnu11 -ffreestanding -DRADIX_32 -DSQISIGN_TARGET_OS_OTHER \
    -DTARGET_ARM $common_defines $common_includes \
    -c "$probe_source" -o "$probe_root/layout-arm.o"

symbol_size()
{
    symbol=$1
    size_hex=$($arm_nm -S "$probe_root/layout-arm.o" |
        awk -v symbol="$symbol" '$4 == symbol { print $2 }')
    test -n "$size_hex"
    printf '%d\n' "$((0x$size_hex))"
}

candidate_bytes=$(symbol_size certified_norm_sketch_probe_candidate)
phase_bytes=$(symbol_size certified_norm_sketch_probe_phase)
find_uv_bytes=$(symbol_size certified_norm_sketch_probe_find_uv)
operation_bytes=$(symbol_size certified_norm_sketch_probe_operation)

test "$candidate_bytes" = 14024
test "$phase_bytes" = 94912
test "$find_uv_bytes" = 172080
test "$operation_bytes" = 172080

printf 'arm_candidate_bytes=%s\n' "$candidate_bytes"
printf 'arm_phase_bytes=%s\n' "$phase_bytes"
printf 'arm_find_uv_bytes=%s\n' "$find_uv_bytes"
printf 'arm_operation_bytes=%s\n' "$operation_bytes"
printf 'd12c_to_d13_saved_bytes=%s\n' 180928
printf 'd12c_to_d13_saved_percent=%s\n' 51.253229389
printf '%s\n' 'certified norm-sketch layout PASS'
