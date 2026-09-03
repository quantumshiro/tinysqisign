#!/bin/sh
set -eu

test "$#" -ge 1
test "$#" -le 2
kind=$1
device=${2:-}
case "$kind" in
    baseline) label=BASELINE ;;
    d1) label=D1 ;;
    *) exit 2 ;;
esac

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
SQISIGN_KSV_CAPTURE_BANNER="SQISIGN_RP2350_V3_${label}_SCA_KEY v1" \
SQISIGN_KSV_CAPTURE_ALIVE="SQISIGN_RP2350_V3_${label}_SCA_KEY alive" \
    exec "$project_root/scripts/capture_rp2350_ksv.sh" "$device"
