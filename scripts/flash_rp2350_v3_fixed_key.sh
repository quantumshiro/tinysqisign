#!/bin/sh
set -eu

test "$#" -ge 1
test "$#" -le 3
kind=$1
case "$kind" in baseline|d1) ;; *) exit 2 ;; esac

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
build_root=${SQISIGN_RP2350_V3_BUILD_ROOT:-"$project_root/build-rp2350-v3-${kind}-sca-key"}
uf2="$build_root/sqisign_rp2350_v3_${kind}_sca_key.uf2"
bootsel_root=${SQISIGN_RP2350_BOOTSEL_ROOT:-/Volumes/RP2350}
info="$bootsel_root/INFO_UF2.TXT"
serial_device=${2:-}
serial_id=${3:-${SQISIGN_RP2350_SERIAL:-}}

test -f "$uf2"
if test ! -f "$info"; then
    if test -z "$serial_device"; then
        for candidate in /dev/cu.usbmodem*; do
            if test -c "$candidate"; then serial_device=$candidate; break; fi
        done
    fi
    test -n "$serial_device"
    test -c "$serial_device"
    stty -f "$serial_device" 1200 cs8 -cstopb -parenb -ixon -ixoff -echo || true
    attempts=0
    while test ! -f "$info" && test "$attempts" -lt 30; do
        sleep 1
        attempts=$((attempts + 1))
    done
fi

if test -f "$info"; then
    grep -q '^Model: Raspberry Pi RP2350$' "$info"
    grep -q '^Board-ID: RP2350$' "$info"
    COPYFILE_DISABLE=1 cp -X "$uf2" "$bootsel_root/"
    sync
    printf 'flashed_via=uf2-volume image=%s\n' "$uf2"
    exit 0
fi

picotool=${SQISIGN_PICOTOOL:-"$project_root/build-rp2350-keygen/picotool-usb/picotool"}
test -x "$picotool"
if test -n "$serial_id"; then
    "$picotool" load -v -x --ser "$serial_id" "$uf2" -t uf2
else
    "$picotool" load -v -x "$uf2" -t uf2
fi
printf 'flashed_via=picotool image=%s\n' "$uf2"
