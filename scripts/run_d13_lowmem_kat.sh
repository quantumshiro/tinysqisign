#!/usr/bin/env bash

set -euo pipefail

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
d13_root=${SQISIGN_D13_SOURCE_ROOT:-"${project_root}/work/compact-d13"}
reference_build=${SQISIGN_D13_KAT_REFERENCE_BUILD:-"${project_root}/build-host-d13-kat-reference"}
lowmem_build=${SQISIGN_D13_KAT_LOWMEM_BUILD:-"${project_root}/build-host-d13-kat-lowmem"}
result_dir=${1:-"${project_root}/results/host/d13-lowmem-kat-2026-09-03"}
vectors=${SQISIGN_D13_KAT_VECTORS:-100}

if [[ ${vectors} != 100 ]]; then
    printf 'This release gate requires exactly 100 vectors (got %s).\n' "${vectors}" >&2
    exit 2
fi
if [[ -e ${result_dir} ]]; then
    printf 'Result directory already exists: %s\n' "${result_dir}" >&2
    exit 2
fi

official_request="${d13_root}/KAT/legacy-nist-v2/PQCsignKAT_353_SQIsign_lvl1.req"
legacy_response="${d13_root}/KAT/legacy-nist-v2/PQCsignKAT_353_SQIsign_lvl1.rsp"
reference_dir="${result_dir}/reference"
lowmem_a_dir="${result_dir}/lowmem-run-1"
lowmem_b_dir="${result_dir}/lowmem-run-2"
artifact_dir="${result_dir}/artifacts"

mkdir -p "${reference_dir}" "${lowmem_a_dir}" "${lowmem_b_dir}" \
    "${artifact_dir}"

cmake -S "${d13_root}" -B "${reference_build}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_GMP=OFF \
    -DENABLE_KAT_TESTS=OFF \
    -DENABLE_TESTS=ON \
    -DGF_RADIX=32 \
    -DSQISIGN_BUILD_TYPE=ref \
    -DSQISIGN_LOWMEM_ONLY=OFF \
    2>&1 | tee "${result_dir}/reference-configure.log"

cmake --build "${reference_build}" --target PQCgenKAT_sign_lvl1 \
    2>&1 | tee "${result_dir}/reference-build.log"

reference_binary="${artifact_dir}/PQCgenKAT_sign_lvl1-d13-reference"
install -m 0755 "${reference_build}/apps/PQCgenKAT_sign_lvl1" \
    "${reference_binary}"
install -m 0644 "${reference_build}/CMakeCache.txt" \
    "${artifact_dir}/reference-CMakeCache.txt"

"${reference_binary}" \
    --vectors "${vectors}" \
    --output-dir "${reference_dir}" \
    2>&1 | tee "${result_dir}/reference-run.log"

cmake -S "${project_root}/experiments/kat" -B "${lowmem_build}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DSQISIGN_D13_SOURCE_ROOT="${d13_root}" \
    2>&1 | tee "${result_dir}/lowmem-configure.log"

cmake --build "${lowmem_build}" --target sqisign_d13_lowmem_kat \
    2>&1 | tee "${result_dir}/lowmem-build.log"

lowmem_binary="${artifact_dir}/sqisign_d13_lowmem_kat"
install -m 0755 "${lowmem_build}/sqisign_d13_lowmem_kat" \
    "${lowmem_binary}"
install -m 0644 "${lowmem_build}/CMakeCache.txt" \
    "${artifact_dir}/lowmem-CMakeCache.txt"
install -m 0644 "${lowmem_build}/compile_commands.json" \
    "${artifact_dir}/lowmem-compile-commands.json"
lowmem_response_a="${lowmem_a_dir}/PQCsignKAT_353_SQIsign_lvl1.rsp"
lowmem_response_b="${lowmem_b_dir}/PQCsignKAT_353_SQIsign_lvl1.rsp"

"${lowmem_binary}" \
    --request "${official_request}" \
    --response "${lowmem_response_a}" \
    --vectors "${vectors}" \
    2>&1 | tee "${result_dir}/lowmem-run-1.log"

"${lowmem_binary}" \
    --request "${official_request}" \
    --response "${lowmem_response_b}" \
    --vectors "${vectors}" \
    2>&1 | tee "${result_dir}/lowmem-run-2.log"

nm "${lowmem_binary}" > "${result_dir}/lowmem-symbols.txt"

python3 "${project_root}/scripts/check_d13_lowmem_kat.py" \
    --project-root "${project_root}" \
    --source "${d13_root}" \
    --official-request "${official_request}" \
    --legacy-response "${legacy_response}" \
    --reference-request "${reference_dir}/PQCsignKAT_353_SQIsign_lvl1.req" \
    --reference-response "${reference_dir}/PQCsignKAT_353_SQIsign_lvl1.rsp" \
    --lowmem-response-a "${lowmem_response_a}" \
    --lowmem-response-b "${lowmem_response_b}" \
    --lowmem-log-a "${result_dir}/lowmem-run-1.log" \
    --lowmem-log-b "${result_dir}/lowmem-run-2.log" \
    --reference-binary "${reference_binary}" \
    --lowmem-binary "${lowmem_binary}" \
    --harness-source "${project_root}/experiments/kat/d13_lowmem_kat.c" \
    --harness-cmake "${project_root}/experiments/kat/CMakeLists.txt" \
    --runner "${project_root}/scripts/run_d13_lowmem_kat.sh" \
    --checker "${project_root}/scripts/check_d13_lowmem_kat.py" \
    --reference-cache "${artifact_dir}/reference-CMakeCache.txt" \
    --lowmem-cache "${artifact_dir}/lowmem-CMakeCache.txt" \
    --lowmem-compile-commands "${artifact_dir}/lowmem-compile-commands.json" \
    --symbols "${result_dir}/lowmem-symbols.txt" \
    --vectors "${vectors}" \
    | tee "${result_dir}/manifest.json"

printf 'D13 low-memory KAT evidence: %s\n' "${result_dir}"
