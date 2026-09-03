#!/bin/sh
set -eu

device=${1:-}
capture_banner=${SQISIGN_KSV_CAPTURE_BANNER:-SQISIGN_RP2350_KSV v1}
capture_alive=${SQISIGN_KSV_CAPTURE_ALIVE:-SQISIGN_RP2350_KSV alive}
export SQISIGN_KSV_CAPTURE_BANNER="$capture_banner"
export SQISIGN_KSV_CAPTURE_ALIVE="$capture_alive"
attempts=0
while test -z "$device" && test "$attempts" -lt 60; do
    for candidate in /dev/cu.usbmodem*; do
        if test -c "$candidate"; then
            device=$candidate
            break
        fi
    done
    if test -z "$device"; then
        sleep 1
        attempts=$((attempts + 1))
    fi
done

test -n "$device"
test -c "$device"
stty -f "$device" 115200 cs8 -cstopb -parenb -ixon -ixoff -echo
perl -e '
    $| = 1;
    $SIG{ALRM} = sub { die "capture timeout\n" };
    # The limit is transport hygiene, not an operation-time bound. KeyGen and
    # Sign run sequentially and may take several hours on the reference core.
    alarm 43200;
    # Resume is only for a capture file that already contains the exact banner;
    # the frozen checker still rejects a merged file without that provenance.
    $saw_banner = $ENV{SQISIGN_CAPTURE_RESUME} ? 1 : 0;
    while (<STDIN>) {
        print;
        $saw_banner = 1
            if /^\Q$ENV{SQISIGN_KSV_CAPTURE_BANNER}\E\r?$/;
        die "initial report missed; reset and reflash before capture\n"
            if /^\Q$ENV{SQISIGN_KSV_CAPTURE_ALIVE}\E / && !$saw_banner;
        if (/^status=(?:PASS|FAIL)/) {
            die "status without initial banner\n" unless $saw_banner;
            exit 0;
        }
    }
    die "capture ended before terminal status\n";
' < "$device"
