#!/bin/sh
# Put an attached RP2350 in BOOTSEL mode and flash the D3 UF2 image.
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
build_root=${SQISIGN_RP2350_V3_BUILD_ROOT:-"$project_root/build-rp2350-v3-d3"}
uf2="$build_root/sqisign_rp2350_v3_d3.uf2"
serial_device=${1:-}
picotool=${SQISIGN_PICOTOOL:-"$project_root/build-rp2350-keygen/picotool-usb/picotool"}

test -f "$uf2"

if test -n "$serial_device" && test -c "$serial_device"; then
    stty -f "$serial_device" 1200 cs8 -cstopb -parenb -ixon -ixoff -echo || true
fi

attempts=0
while ! "$picotool" info >/dev/null 2>&1 && test "$attempts" -lt 30; do
    sleep 1
    attempts=$((attempts + 1))
done

test -x "$picotool"
"$picotool" load -v -x "$uf2" -t uf2
printf 'flashed_via=picotool image=%s\n' "$uf2"
