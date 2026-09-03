#!/bin/sh
set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SQISIGN_KEYGEN_SIGN_AUDIT_MODE=${SQISIGN_RP2350_KSV_AUDIT_MODE:-D12c} \
    exec "$project_root/scripts/audit_rp2350_keygen_sign_elf.sh" "$@"
