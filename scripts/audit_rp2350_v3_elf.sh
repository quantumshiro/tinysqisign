#!/bin/sh
# Structural audit for the isolated SQIsign v3 RP2350 firmware.
set -eu

test "$#" -eq 4
nm_tool=$1
objdump_tool=$2
size_tool=$3
elf=$4

case ${elf##*/} in
    *v3_baseline*) audit_label='v3 baseline' ;;
    *v3_d1*) audit_label='v3 D1' ;;
    *) audit_label='v3 image' ;;
esac

test -x "$nm_tool"
test -x "$objdump_tool"
test -x "$size_tool"
test -f "$elf"

build_root=$(CDPATH= cd -- "$(dirname -- "$elf")" && pwd -P)
su_count=$(find "$build_root" -name '*.su' -type f | wc -l | tr -d ' ')
test "$su_count" -ge 50
dynamic_rows=$(find "$build_root" -name '*.su' -type f -print0 | \
    xargs -0 awk '$NF != "static" { print FILENAME ":" $0 }')
dynamic_count=0
if test -n "$dynamic_rows"; then
    dynamic_count=$(printf '%s\n' "$dynamic_rows" | wc -l | tr -d ' ')
    printf '%s dynamic stack-usage record(s), retained for analysis:\n%s\n' \
        "$audit_label" "$dynamic_rows"
fi

symbol_table=$($nm_tool -n -S "$elf")
symbol_names=$(printf '%s\n' "$symbol_table" | awk 'NF { print $NF }')

for required in \
    crypto_sign_keypair crypto_sign crypto_sign_open \
    protocols_keygen protocols_sign protocols_verify \
    dim2id2iso_ideal_to_isogeny_qlapoty \
    randombytes randombytes_init process_stack; do
    if ! printf '%s\n' "$symbol_names" | grep -q "^${required}$"; then
        printf 'required SQIsign v3 symbol missing: %s\n' "$required" >&2
        exit 1
    fi
done

for forbidden in \
    find_uv __gmpz_init mpz_init dpe_init; do
    if printf '%s\n' "$symbol_names" | grep -q "^${forbidden}$"; then
        printf 'forbidden allocator/v2-arithmetic symbol present: %s\n' \
            "$forbidden" >&2
        exit 1
    fi
done

# The current official and D1 images retain a qlapoty diagnostic path using
# fprintf()/abort().  That path links newlib allocator machinery even though
# the linker reserves a zero-byte heap.  Report this fact without interpreting
# symbol presence as successful-path allocation.
allocator_symbols=$(printf '%s\n' "$symbol_names" | awk '
    /^(malloc|calloc|realloc|free)$/ ||
    /^_(malloc|calloc|realloc|free)_r$/ ||
    /^_?sbrk(_r)?$/ { print }
')
allocator_symbol_count=0
if test -n "$allocator_symbols"; then
    allocator_symbol_count=$(printf '%s\n' "$allocator_symbols" | wc -l | tr -d ' ')
    printf '%s linked allocator symbol(s) retained for comparison:\n%s\n' \
        "$audit_label" "$allocator_symbols"
fi

process_stack_size=$(printf '%s\n' "$symbol_table" | \
    awk '$NF == "process_stack" { print $2; exit }')
test "$process_stack_size" = 00020000

section_table=$($objdump_tool -h "$elf")
heap_size=$(printf '%s\n' "$section_table" | \
    awk '$2 == ".heap" { print $3; exit }')
test "$heap_size" = 00000000

alloc_sections=$(printf '%s\n' "$section_table" | awk '
    /^[[:space:]]*[0-9]+[[:space:]]/ {
        name = $2; size = $3; vma = $4; next
    }
    /ALLOC/ && size !~ /^0+$/ {
        readonly = ($0 ~ /READONLY/) ? 1 : 0
        print name, size, vma, readonly
    }
')

set -- $alloc_sections
while test "$#" -ge 4; do
    section_name=$1
    section_size=$2
    section_vma=$3
    section_readonly=$4
    shift 4
    section_start=$((0x$section_vma))
    section_end=$((section_start + 0x$section_size))

    if test "$section_start" -ge $((0x10000000)) && \
       test "$section_end" -le $((0x11000000)); then
        if test "$section_readonly" != 1; then
            printf 'writable allocated section in XIP flash: %s\n' \
                "$section_name" >&2
            exit 1
        fi
    elif test "$section_start" -ge $((0x20000000)) && \
         test "$section_end" -le $((0x20082000)); then
        :
    else
        printf 'allocated section outside approved flash/SRAM: %s size=%s vma=%s\n' \
            "$section_name" "$section_size" "$section_vma" >&2
        exit 1
    fi
done
test "$#" -eq 0

printf 'SQIsign %s RP2350 ELF audit PASS\n' "$audit_label"
printf 'stack_usage_files=%s dynamic_stack_records=%s allocator_symbols=%s process_stack_bytes=%u heap_section_bytes=0\n' \
    "$su_count" "$dynamic_count" "$allocator_symbol_count" \
    $((0x$process_stack_size))
"$size_tool" "$elf"
