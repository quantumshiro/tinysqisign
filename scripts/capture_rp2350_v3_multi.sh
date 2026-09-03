#!/bin/sh
set -eu

test "$#" -ge 2
test "$#" -le 3
kind=$1
placement=$2
device=${3:-}
case "$kind" in baseline) label=BASELINE ;; d1) label=D1 ;; *) exit 2 ;; esac
case "$placement" in a) place=A ;; b) place=B ;; *) exit 2 ;; esac

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
SQISIGN_KSV_CAPTURE_BANNER="SQISIGN_RP2350_V3_${label}_MULTI_${place} v1" \
SQISIGN_KSV_CAPTURE_ALIVE="SQISIGN_RP2350_V3_${label}_MULTI_${place} alive" \
    exec "$project_root/scripts/capture_rp2350_ksv.sh" "$device"
