#!/bin/sh
# Structural audit for the isolated SQIsign v3 RP2350 firmware.
set -eu

test "$#" -eq 4 || test "$#" -eq 5
nm_tool=$1
objdump_tool=$2
size_tool=$3
elf=$4
audit_profile=${5:-ksv}

case $audit_profile in
    ksv)
        required_api_symbols='crypto_sign_keypair crypto_sign crypto_sign_open'
        required_protocol_symbols='protocols_keygen protocols_sign protocols_verify'
        ;;
    sign-verify)
        required_api_symbols='crypto_sign crypto_sign_open'
        required_protocol_symbols='protocols_sign protocols_verify'
        ;;
    *)
        printf 'unknown SQIsign v3 ELF audit profile: %s\n' \
            "$audit_profile" >&2
        exit 1
        ;;
esac

case ${elf##*/} in
    *v3_baseline*) audit_label='v3 baseline' ;;
    *v3_d1*) audit_label='v3 D1' ;;
    *v3_d2_static*) audit_label='v3 D2 static-stack' ;;
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
    $required_api_symbols \
    $required_protocol_symbols \
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

# The isolated firmware replaces upstream secure_free() and the re-entrant
# newlib allocation ABI.  The only permitted allocator-shaped symbols are
# three six-byte fail-stop shims used by newlib stdio; each must branch directly
# to sqisign_allocator_violation(), whose body contains an Arm UDF instruction.
allocator_symbols=$(printf '%s\n' "$symbol_names" | awk '
    /^(malloc|calloc|realloc|free)$/ ||
    /^_(malloc|calloc|realloc|free)_r$/ ||
    /^_?sbrk(_r)?$/ { print }
')
allocator_symbol_count=0
if test -n "$allocator_symbols"; then
    allocator_symbol_count=$(printf '%s\n' "$allocator_symbols" | wc -l | tr -d ' ')
fi
allocator_symbols_sorted=$(printf '%s\n' "$allocator_symbols" | sed '/^$/d' | LC_ALL=C sort)
expected_allocator_symbols=$(printf '%s\n' _free_r _malloc_r _realloc_r | LC_ALL=C sort)
if test "$allocator_symbols_sorted" != "$expected_allocator_symbols"; then
    printf '%s allocator ABI differs from the fail-stop policy:\n%s\n' \
        "$audit_label" "$allocator_symbols_sorted" >&2
    exit 1
fi
if ! $objdump_tool -d --disassemble=sqisign_allocator_violation "$elf" | \
    grep -q '[[:space:]]udf[[:space:]]'; then
    printf '%s allocator violation handler lacks UDF\n' "$audit_label" >&2
    exit 1
fi
map_file="$elf.map"
test -f "$map_file"
for allocator_symbol in _free_r _malloc_r _realloc_r; do
    if ! $objdump_tool -d --disassemble="$allocator_symbol" "$elf" | \
        grep -q '<sqisign_allocator_violation>'; then
        printf '%s is not a direct fail-stop shim: %s\n' \
            "$audit_label" "$allocator_symbol" >&2
        exit 1
    fi
    if ! grep -A1 "[.]text[.]unlikely[.]${allocator_symbol}$" "$map_file" | \
        grep -q 'allocator_traps[.]c[.]o'; then
        printf '%s has unexpected allocator symbol provenance: %s\n' \
            "$audit_label" "$allocator_symbol" >&2
        exit 1
    fi
done

crypto_archive="$build_root/libsqisign_v3_p324_3_m4f.a"
test -f "$crypto_archive"
archive_allocator_refs=$($nm_tool -u "$crypto_archive" | awk '
    /[[:space:]](malloc|calloc|realloc|free)$/ ||
    /[[:space:]]_(malloc|calloc|realloc|free)_r$/ ||
    /[[:space:]]_?sbrk(_r)?$/ { print $NF }
')
if test -n "$archive_allocator_refs"; then
    printf '%s forbidden allocator reference(s) in crypto archive:\n%s\n' \
        "$audit_label" "$archive_allocator_refs" >&2
    exit 1
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

printf 'SQIsign %s RP2350 ELF audit PASS (profile=%s)\n' \
    "$audit_label" "$audit_profile"
printf 'stack_usage_files=%s dynamic_stack_records=%s allocator_trap_symbols=%s process_stack_bytes=%u heap_section_bytes=0\n' \
    "$su_count" "$dynamic_count" "$allocator_symbol_count" \
    $((0x$process_stack_size))
"$size_tool" "$elf"
