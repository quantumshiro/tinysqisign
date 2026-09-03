#!/bin/sh
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
SQISIGN_KSV_CAPTURE_BANNER='SQISIGN_RP2350_V3_D2_STATIC v1' \
SQISIGN_KSV_CAPTURE_ALIVE='SQISIGN_RP2350_V3_D2_STATIC alive' \
    exec "$project_root/scripts/capture_rp2350_ksv.sh" "$@"
