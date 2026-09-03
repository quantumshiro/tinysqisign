#!/bin/sh
# Build the transformed upstream source and run all official p324_3 KAT cases.
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
source_root=${SQISIGN_V3_D2_SOURCE_ROOT:-"$project_root/work/v3-static-stack-d2"}
build_root=${SQISIGN_V3_D2_HOST_BUILD_ROOT:-"$project_root/build-host-v3-d2-static"}
output=${1:-"$project_root/results/v3/host/d2-static-validation-2026-09-04.txt"}
base_commit=6d017708db403bf83977fa70770fc4f7f9e9ff21

source_root=$(CDPATH= cd -- "$source_root" && pwd -P)
source_commit=$(git -C "$source_root" rev-parse HEAD)
git -C "$source_root" merge-base --is-ancestor "$base_commit" "$source_commit"
test "$(git -C "$source_root" rev-list --count "$base_commit..$source_commit")" -eq 1
test -z "$(git -C "$source_root" status --porcelain --untracked-files=no)"

cmake -S "$source_root" -B "$build_root" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DSQISIGN_BUILD_TYPE=ref \
    -DGF_RADIX=32 \
    -DENABLE_SIGN=ON \
    -DENABLE_TESTS=ON \
    -DENABLE_STRICT=ON >/dev/null
cmake --build "$build_root" --target sqisign_test_kat_p324_3 --parallel >/dev/null

mkdir -p "$source_root/build/test" "$(dirname -- "$output")"
kat_output=$(
    CDPATH= cd -- "$source_root/build/test"
    "$build_root/test/sqisign_test_kat_p324_3"
)
printf '%s\n' "$kat_output" | grep -q '^Known Answer Tests PASSED[.]'

partial="$output.partial"
{
    printf 'SQIsign v3 p324_3 D2 static-stack host validation\n'
    printf 'source_base_commit=%s\n' "$base_commit"
    printf 'source_commit=%s\n' "$source_commit"
    printf 'tracked_source_clean=1\n'
    printf 'build_type=Release implementation=ref radix=32 strict=ON\n'
    printf 'official_response_vectors=100\n'
    printf 'official_kat_exit=0\n'
    printf 'official_kat_result=%s\n' "$kat_output"
    printf 'status=PASS\n'
} >"$partial"
mv "$partial" "$output"
cat "$output"
