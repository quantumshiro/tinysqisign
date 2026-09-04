#!/bin/sh
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
SQISIGN_KSV_CAPTURE_BANNER='SQISIGN_RP2350_V3_D3 v1' \
SQISIGN_KSV_CAPTURE_ALIVE='SQISIGN_RP2350_V3_D3 alive' \
    exec "$project_root/scripts/capture_rp2350_ksv.sh" "$@"
