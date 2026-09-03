#!/usr/bin/env bash

set -euo pipefail

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)

generate_one()
{
    source_root=$1
    if [[ -d ${source_root}/src/pqm4 ]]; then
        printf 'Refusing to overwrite generated directory: %s/src/pqm4\n' \
            "${source_root}" >&2
        return 2
    fi

    cmake -S "${source_root}" -B "${source_root}/build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DENABLE_TESTS=ON \
        -DGF_RADIX=32 \
        -DSQISIGN_BUILD_TYPE=ref
    cmake --build "${source_root}/build" --target \
        PQCgenKAT_sign_pqm4_p324_3 \
        PQCgenKAT_sign_pqm4_p500_27 \
        PQCgenKAT_sign_pqm4_p664_17
    (
        cd "${source_root}"
        ./scripts/gen_pqm4_sources.sh m4f
    )
}

generate_one "${project_root}/work/official-v3"
generate_one "${project_root}/work/v3-lowmem-d1"
printf 'SQIsign v3 p324_3/p500_27/p664_17 m4f source generation: PASS\n'

