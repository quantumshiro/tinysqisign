#!/bin/sh
# Build and run the p324_3 host gates for exact clean official and D1 trees.
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
official_source=${SQISIGN_V3_OFFICIAL_SOURCE:-"$project_root/work/official-v3"}
d1_source=${SQISIGN_V3_D1_SOURCE:-"$project_root/work/v3-lowmem-d1"}
official_build=${SQISIGN_V3_OFFICIAL_HOST_BUILD:-"$project_root/build-host-v3-ref"}
d1_build=${SQISIGN_V3_D1_HOST_BUILD:-"$project_root/build-host-v3-d1"}
output=${1:-"$project_root/results/v3/host/validation-clean-2026-09-04.txt"}
official_commit=6d017708db403bf83977fa70770fc4f7f9e9ff21
d1_commit=9293313fb58de4c5ce9dd27a5a9fde0058766c79
partial="$output.partial"

run_gates()
{
    label=$1
    source_root=$2
    build_root=$3
    expected_commit=$4

    test "$(git -C "$source_root" rev-parse HEAD)" = "$expected_commit"
    test -z "$(git -C "$source_root" status --porcelain --untracked-files=no)"

    cmake -S "$source_root" -B "$build_root" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DSQISIGN_BUILD_TYPE=ref \
        -DGF_RADIX=32 \
        -DENABLE_SIGN=ON \
        -DENABLE_TESTS=ON \
        -DENABLE_STRICT=ON >/dev/null
    cmake --build "$build_root" --target \
        example_nistapi_p324_3 \
        sqisign_test_scheme_p324_3 \
        sqisign_test_kat_p324_3 --parallel >/dev/null

    mkdir -p "$source_root/build/test"
    api_output=$(CDPATH= cd -- "$build_root/apps" && ./example_nistapi_p324_3)
    selftest_output=$(CDPATH= cd -- "$build_root/test" && ./sqisign_test_scheme_p324_3)
    kat_output=$(CDPATH= cd -- "$source_root/build/test" && "$build_root/test/sqisign_test_kat_p324_3")
    printf '%s\n' "$kat_output" | grep -q '^Known Answer Tests PASSED[.]'

    {
        printf '\n[%s]\n' "$label"
        printf 'source_commit=%s\n' "$expected_commit"
        printf 'tracked_source_clean=1\n'
        printf 'nist_api_exit=0\n'
        printf 'nist_api_output_sha256=%s\n' "$(printf '%s\n' "$api_output" | shasum -a 256 | awk '{print $1}')"
        printf 'selftest_exit=0\n'
        printf 'selftest_output_sha256=%s\n' "$(printf '%s\n' "$selftest_output" | shasum -a 256 | awk '{print $1}')"
        printf 'official_response_vectors=100\n'
        printf 'official_kat_exit=0\n'
        printf 'official_kat_result=%s\n' "$kat_output"
    } >>"$partial"
}

mkdir -p "$(dirname -- "$output")"
{
    printf 'SQIsign v3 p324_3 clean-source host validation\n'
    printf 'build_type=Release implementation=ref radix=32 strict=ON\n'
} >"$partial"

run_gates official "$official_source" "$official_build" "$official_commit"
run_gates lifetime_overlay "$d1_source" "$d1_build" "$d1_commit"
printf '\nstatus=PASS\n' >>"$partial"
mv "$partial" "$output"
cat "$output"
