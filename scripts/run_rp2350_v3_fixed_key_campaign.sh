#!/bin/sh
# Flash and capture official/D1 screens, each with two fixed shuffled key orders.
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
device=${1:-}
output_dir=${2:-"$project_root/results/v3/rp2350/fixed-key-timing-clean-2026-09-04"}
mkdir -p "$output_dir"

run_one()
{
    kind=$1
    capture_path="$output_dir/${kind}.txt"
    partial_path="$capture_path.partial"
    test ! -e "$capture_path"
    test ! -e "$partial_path"
    build_root=${SQISIGN_RP2350_V3_BUILD_ROOT_PREFIX:-"$project_root/build-rp2350-v3"}
    build_root="${build_root}-${kind}-sca-key"
    printf 'campaign_start image=%s output=%s\n' "$kind" "$capture_path" >&2
    SQISIGN_RP2350_V3_BUILD_ROOT="$build_root" \
        "$project_root/scripts/flash_rp2350_v3_fixed_key.sh" "$kind" "$device"
    "$project_root/scripts/capture_rp2350_v3_fixed_key.sh" "$kind" "$device" |
        tee "$partial_path"
    grep -q '^status=PASS' "$partial_path"
    mv "$partial_path" "$capture_path"
    printf 'campaign_pass image=%s\n' "$kind" >&2
}

run_one baseline
run_one d1
printf 'campaign_status=PASS captures=2 output_dir=%s\n' "$output_dir"
