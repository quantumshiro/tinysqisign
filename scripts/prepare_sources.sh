#!/usr/bin/env bash

set -euo pipefail

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
work_root="${project_root}/work"
v2_root="${work_root}/compact-d13"
v3_official_root="${work_root}/official-v3"
v3_d1_root="${work_root}/v3-lowmem-d1"

v2_url=${SQISIGN_V2_URL:-https://github.com/pqc-lab-ku/compact-SQIsign.git}
v2_base=5b94b09a1dbbdcc8b91749fec83a9f111ef9cce3
v2_commit=71099e0827d3f0a3b3c705d2eda592c401e0d57d
v2_tree=8761bccb5b14172e21d7228878fb3fc9379db5c4
v3_url=${SQISIGN_V3_URL:-https://github.com/SQISign/the-sqisign.git}
v3_base=6d017708db403bf83977fa70770fc4f7f9e9ff21
v3_changed_blob=6b709425c3abb8b3d171c8a13fd5a6877e1369ec

for path in "${v2_root}" "${v3_official_root}" "${v3_d1_root}"; do
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
git -C "${v3_d1_root}" apply \
    "${project_root}/patches/v3-lifetime-overlays.patch"
observed_v3_blob=$(git -C "${v3_d1_root}" hash-object \
    src/quaternion/ref/lvlx/lll/lll_dim4.c)
if [[ ${observed_v3_blob} != "${v3_changed_blob}" ]]; then
    printf 'Unexpected reconstructed v3 file: %s\n' "${observed_v3_blob}" >&2
    exit 1
fi

printf 'v2 tree: %s\n' "${observed_v2_tree}"
printf 'v3 base: %s\n' "${v3_base}"
printf 'v3 changed blob: %s\n' "${observed_v3_blob}"
printf 'Source reconstruction: PASS\n'
