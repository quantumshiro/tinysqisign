#!/usr/bin/env bash

set -euo pipefail

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
selection=${1:-all}
case "${selection}" in
    all|official|lifetime|extended|static) ;;
    *) printf 'usage: %s [all|official|lifetime|extended|static]\n' "$0" >&2; exit 2 ;;
esac

generate_one()
{
    source_root=$1
    variant_scope=$2
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
    if [[ ${variant_scope} == all ]]; then
        cmake --build "${source_root}/build" --target \
            PQCgenKAT_sign_pqm4_p324_3 \
            PQCgenKAT_sign_pqm4_p500_27 \
            PQCgenKAT_sign_pqm4_p664_17
        (
            cd "${source_root}"
            ./scripts/gen_pqm4_sources.sh m4f
        )
    else
        cmake --build "${source_root}/build" --target \
            PQCgenKAT_sign_pqm4_p324_3
        generator="${source_root}/scripts/gen_pqm4_sources.sh"
        anchor='for LVL in p324_3 p500_27 p664_17'
        [[ $(grep -Fxc "${anchor}" "${generator}") == 1 ]]
        sed "s/^${anchor}$/for LVL in p324_3/" "${generator}" | (
            cd "${source_root}"
            bash -s -- m4f
        )
    fi
}

if [[ ${selection} == all || ${selection} == official ]]; then
    generate_one "${project_root}/work/official-v3" all
fi
if [[ ${selection} == all || ${selection} == lifetime ]]; then
    generate_one "${project_root}/work/v3-lowmem-d1" all
fi
if [[ ${selection} == all || ${selection} == extended ]]; then
    generate_one "${project_root}/work/v3-lowmem-d3" all
fi
if [[ ${selection} == all || ${selection} == static ]]; then
    generate_one "${project_root}/work/v3-static-stack-d2" p324_3
fi
printf 'SQIsign v3 %s m4f source generation: PASS\n' "${selection}"
