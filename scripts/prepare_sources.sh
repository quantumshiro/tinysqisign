#!/usr/bin/env bash

set -euo pipefail

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
work_root="${project_root}/work"
v2_root="${work_root}/compact-d13"
v3_official_root="${work_root}/official-v3"
v3_d1_root="${work_root}/v3-lowmem-d1"
v3_d3_root="${work_root}/v3-lowmem-d3"
v3_d2_root="${work_root}/v3-static-stack-d2"

v2_url=${SQISIGN_V2_URL:-https://github.com/pqc-lab-ku/compact-SQIsign.git}
v2_base=5b94b09a1dbbdcc8b91749fec83a9f111ef9cce3
v2_commit=71099e0827d3f0a3b3c705d2eda592c401e0d57d
v2_tree=8761bccb5b14172e21d7228878fb3fc9379db5c4
v3_url=${SQISIGN_V3_URL:-https://github.com/SQISign/the-sqisign.git}
v3_base=6d017708db403bf83977fa70770fc4f7f9e9ff21
v3_d1_commit=9293313fb58de4c5ce9dd27a5a9fde0058766c79
v3_d1_tree=30606e0b5cb2a99d782f4eb334c0c3b87b1edd1c
v3_d3_commit=874658c64aa2e20f53b1f4d696144723d558ed5c
v3_d3_tree=71e3edec18ec40ff4bc315b596c94422f68888d7
v3_d2_commit=cb94f242ba791a4ccb980b46c917830b309a9832
v3_d2_tree=d53293e903e4cb0e5766edc0b3d4b74a3fec6a59

for path in "${v2_root}" "${v3_official_root}" "${v3_d1_root}" "${v3_d3_root}" "${v3_d2_root}"; do
    if [[ -e ${path} ]]; then
        printf 'Refusing to overwrite existing source tree: %s\n' "${path}" >&2
        exit 2
    fi
done

mkdir -p "${work_root}"

git clone "${v2_url}" "${v2_root}"
git -C "${v2_root}" checkout --detach "${v2_base}"
git -C "${v2_root}" bundle verify "${project_root}/patches/v2-d13.bundle"
git -C "${v2_root}" fetch "${project_root}/patches/v2-d13.bundle" \
    "refs/heads/research/certified-norm-sketches"
git -C "${v2_root}" checkout --detach "${v2_commit}"
observed_v2_tree=$(git -C "${v2_root}" rev-parse 'HEAD^{tree}')
if [[ ${observed_v2_tree} != "${v2_tree}" ]]; then
    printf 'Unexpected reconstructed v2 tree: %s\n' "${observed_v2_tree}" >&2
    exit 1
fi
if [[ $(git -C "${v2_root}" rev-parse HEAD) != "${v2_commit}" ]] ||
   [[ -n $(git -C "${v2_root}" status --porcelain) ]]; then
    printf 'Reconstructed v2 checkout is not the clean D13 commit\n' >&2
    exit 1
fi

git clone "${v3_url}" "${v3_official_root}"
git -C "${v3_official_root}" checkout --detach "${v3_base}"
git clone --no-checkout "${v3_official_root}" "${v3_d1_root}"
git -C "${v3_d1_root}" checkout --detach "${v3_base}"
git -C "${v3_d1_root}" bundle verify \
    "${project_root}/patches/v3-lifetime-overlays.bundle"
git -C "${v3_d1_root}" fetch \
    "${project_root}/patches/v3-lifetime-overlays.bundle" \
    refs/heads/research/v3-lowmem-d1
git -C "${v3_d1_root}" checkout --detach "${v3_d1_commit}"
observed_v3_d1_tree=$(git -C "${v3_d1_root}" rev-parse 'HEAD^{tree}')
if [[ ${observed_v3_d1_tree} != "${v3_d1_tree}" ]] ||
   [[ -n $(git -C "${v3_d1_root}" status --porcelain) ]]; then
    printf 'Unexpected reconstructed v3 lifetime-overlay tree: %s\n' \
        "${observed_v3_d1_tree}" >&2
    exit 1
fi

git clone --no-checkout "${v3_official_root}" "${v3_d3_root}"
git -C "${v3_d3_root}" checkout --detach "${v3_base}"
git -C "${v3_d3_root}" bundle verify \
    "${project_root}/patches/v3-two-function-lifetime.bundle"
git -C "${v3_d3_root}" fetch \
    "${project_root}/patches/v3-two-function-lifetime.bundle" \
    refs/heads/research/v3-two-function-lifetime
git -C "${v3_d3_root}" checkout --detach "${v3_d3_commit}"
observed_v3_d3_tree=$(git -C "${v3_d3_root}" rev-parse 'HEAD^{tree}')
if [[ ${observed_v3_d3_tree} != "${v3_d3_tree}" ]] ||
   [[ -n $(git -C "${v3_d3_root}" status --porcelain) ]]; then
    printf 'Unexpected reconstructed v3 two-function lifetime tree: %s\n' \
        "${observed_v3_d3_tree}" >&2
    exit 1
fi

git clone --no-checkout "${v3_official_root}" "${v3_d2_root}"
git -C "${v3_d2_root}" checkout --detach "${v3_base}"
git -C "${v3_d2_root}" bundle verify \
    "${project_root}/patches/v3-static-stack.bundle"
git -C "${v3_d2_root}" fetch \
    "${project_root}/patches/v3-static-stack.bundle" \
    refs/heads/research/v3-static-stack-d2
git -C "${v3_d2_root}" checkout --detach "${v3_d2_commit}"
observed_v3_d2_tree=$(git -C "${v3_d2_root}" rev-parse 'HEAD^{tree}')
if [[ ${observed_v3_d2_tree} != "${v3_d2_tree}" ]] ||
   [[ -n $(git -C "${v3_d2_root}" status --porcelain) ]]; then
    printf 'Unexpected reconstructed v3 static-stack tree: %s\n' \
        "${observed_v3_d2_tree}" >&2
    exit 1
fi

printf 'v2 tree: %s\n' "${observed_v2_tree}"
printf 'v3 base: %s\n' "${v3_base}"
printf 'v3 lifetime-overlay commit/tree: %s %s\n' \
    "${v3_d1_commit}" "${observed_v3_d1_tree}"
printf 'v3 two-function lifetime commit/tree: %s %s\n' \
    "${v3_d3_commit}" "${observed_v3_d3_tree}"
printf 'v3 static-stack commit/tree: %s %s\n' \
    "${v3_d2_commit}" "${observed_v3_d2_tree}"
printf 'Source reconstruction: PASS\n'
