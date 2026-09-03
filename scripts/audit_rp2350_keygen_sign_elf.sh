#!/bin/sh
# Audit the final RP2350 deterministic KeyGen + Sign[/Verify] ELF. The selected
# low-memory closure must physically exclude heap/GMP/system-RNG dependencies
# and every compatibility entry that owns a legacy automatic workspace.
set -eu

test "$#" -eq 3
nm_tool=$1
objdump_tool=$2
elf=$3

test -x "$nm_tool"
test -x "$objdump_tool"
test -f "$elf"

audit_mode=${SQISIGN_KEYGEN_SIGN_AUDIT_MODE:-D12b}
case "$audit_mode" in
    D12b)
        expected_crypto_su=51
        crypto_subdir=sign_lvl1
        expected_owner_size=00056370
        legacy_decoder=signature_from_bytes
        required_verify=
        audit_label='KeyGen+Sign'
        ;;
    D12c)
        expected_crypto_su=52
        crypto_subdir=ksv_lvl1
        expected_owner_size=00056370
        legacy_decoder=
        required_verify='sqisign_verify_with_workspace
protocols_verify_with_workspace
signature_from_bytes'
        audit_label='KeyGen+Sign+Verify'
        ;;
    D13)
        expected_crypto_su=52
        crypto_subdir=ksv_lvl1
        expected_owner_size=0002a0b0
        legacy_decoder=
        required_verify='sqisign_verify_with_workspace
protocols_verify_with_workspace
signature_from_bytes'
        audit_label='D13 KeyGen+Sign+Verify'
        ;;
    *)
        printf 'unsupported RP2350 K/S/V audit mode: %s\n' "$audit_mode" >&2
        exit 2
        ;;
esac

build_root=$(CDPATH= cd -- "$(dirname -- "$elf")/../../.." && pwd)
crypto_su_root="$build_root/src/sqisign/$crypto_subdir"
crypto_su_count=$(find "$crypto_su_root" -name '*.su' -type f | wc -l | tr -d ' ')
test "$crypto_su_count" -eq "$expected_crypto_su"
dynamic_rows=$(find "$build_root" -name '*.su' -type f -print0 | \
    xargs -0 awk '$NF != "static" { print FILENAME ":" $0 }')
if test -n "$dynamic_rows"; then
    printf 'dynamic stack-usage record(s) in final firmware build:\n%s\n' \
        "$dynamic_rows" >&2
    exit 1
fi

symbol_table=$($nm_tool -n -S "$elf")
symbol_names=$(printf '%s\n' "$symbol_table" | awk 'NF { print $NF }')

forbidden_runtime=$(printf '%s\n' "$symbol_names" | awk '
    /^(malloc|calloc|realloc|reallocarray|free|cfree)$/ ||
    /^_(malloc|calloc|realloc|free)_r$/ ||
    /^__(wrap|real)_(malloc|calloc|realloc|free)$/ ||
    /^(aligned_alloc|memalign|posix_memalign|valloc|pvalloc)$/ ||
    /^(asprintf|vasprintf|strdup|strndup|getline|getdelim)$/ ||
    /^_?sbrk(_r)?$/ ||
    /^(__gmp|gmp_|mpf_|mpn_|mpq_|mpz_)/ ||
    /^_Zn[wa]/ || /^_Zd[la]/ ||
    /^(randombytes_system|randombytes_select|getrandom|getentropy|arc4random)$/ {
        print
    }
')
if test -n "$forbidden_runtime"; then
    printf 'forbidden allocator/GMP/system-RNG symbol(s) in %s:\n%s\n' \
        "$elf" "$forbidden_runtime" >&2
    exit 1
fi

legacy_symbols='sqisign_keypair
sqisign_sign
protocols_keygen
protocols_sign
find_uv
fixed_degree_isogeny_and_eval
dim2id2iso_ideal_to_isogeny_clapotis
dim2id2iso_arbitrary_isogeny_evaluation
dim2id2iso_enumerate_hypercube_regression_test
id2iso_kernel_dlogs_to_ideal_even
quat_ml2
quat_ml2_retry
quat_ml2_retry_with_reducer
quat_ml2_mlll_with_reducer
quat_sampling_random_ideal_O0_given_norm
quat_lideal_create
quat_lideal_create_with_norm
quat_lideal_mul
quat_lideal_add
quat_lideal_inter
quat_lideal_right_transporter
quat_lideal_right_order
quat_lideal_conjugate_without_hnf
quat_lideal_lideal_mul_reduced
quat_lideal_prime_norm_reduced_equivalent
quat_lattice_add
quat_lattice_intersect
quat_lattice_alg_elem_mul
quat_lattice_mul
quat_lattice_mul_mlll
quat_lattice_intersect_mlll
secret_key_from_bytes
ibz_mat_4xn_hnf_mod_core
theta_chain_compute_and_eval
theta_chain_compute_and_eval_randomized
theta_chain_compute_and_eval_verify
fp2_batched_inv
fp2_dlog_2e
lift_basis
weil
reduced_tate
ec_dlog_2_weil
ec_dlog_2_tate
ec_eval_even
change_of_basis_matrix_tate
change_of_basis_matrix_tate_invert
mp_mul
mp_inv_2e
mp_invert_matrix
sqisign_secure_free
sqisign_verify
sqisign_open
protocols_verify
protocols_verify_original_release'

legacy_symbols="$legacy_symbols
$legacy_decoder"

for legacy in $legacy_symbols; do
    rejected=$(printf '%s\n' "$symbol_names" | awk -v wanted="$legacy" '
        $0 == wanted || (length($0) > length(wanted) &&
                         substr($0, length($0) - length(wanted)) == "_" wanted) {
            print
        }
    ')
    if test -n "$rejected"; then
        printf 'legacy large-stack/fallback symbol(s) in %s:\n%s\n' \
            "$elf" "$rejected" >&2
        exit 1
    fi
done

required_symbols='randombytes
randombytes_init
sqisign_keypair_with_workspace
protocols_keygen_with_workspace
sqisign_sign_with_workspace
protocols_sign_with_workspace
secret_key_from_bytes_with_workspace
ec_eval_even_with_workspace
quat_lideal_inter_with_workspace
quat_lattice_mul_mlll_with_workspace
sqisign_rp2350_keygen_sign_owner
sqisign_rp2350_keygen_sign_psp'

required_symbols="$required_symbols
$required_verify"

for required in $required_symbols; do
    observed=$(printf '%s\n' "$symbol_names" | awk -v wanted="$required" '
        $0 == wanted || (length($0) > length(wanted) &&
                         substr($0, length($0) - length(wanted)) == "_" wanted) {
            print
        }
    ')
    if test -z "$observed"; then
        printf 'required KeyGen/Sign symbol missing from %s: %s\n' \
            "$elf" "$required" >&2
        exit 1
    fi
done

section_table=$($objdump_tool -h "$elf")
heap_size=$(printf '%s\n' "$section_table" | awk '$2 == ".heap" { print $3 }')
if test -z "$heap_size" || test "$heap_size" != 00000000; then
    printf 'expected a zero-byte .heap section, got <%s>\n' "$heap_size" >&2
    exit 1
fi

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

symbol_address()
{
    printf '%s\n' "$symbol_table" | awk -v wanted="$1" \
        '$NF == wanted { print $1; exit }'
}

symbol_size()
{
    printf '%s\n' "$symbol_table" | awk -v wanted="$1" \
        '$NF == wanted { print $2; exit }'
}

owner_address=$(symbol_address sqisign_rp2350_keygen_sign_owner)
owner_size=$(symbol_size sqisign_rp2350_keygen_sign_owner)
psp_address=$(symbol_address sqisign_rp2350_keygen_sign_psp)
psp_size=$(symbol_size sqisign_rp2350_keygen_sign_psp)
bss_end=$(symbol_address __bss_end__)
scratch_x_start=$(symbol_address __scratch_x_start__)
scratch_x_end=$(symbol_address __scratch_x_end__)
scratch_y_start=$(symbol_address __scratch_y_start__)
scratch_y_end=$(symbol_address __scratch_y_end__)
stack_one_bottom=$(symbol_address __StackOneBottom)
stack_one_top=$(symbol_address __StackOneTop)
stack_limit=$(symbol_address __StackLimit)
stack_top=$(symbol_address __StackTop)

test "$owner_size" = "$expected_owner_size"
test "$psp_size" = 00020000
test $((0x$owner_address % 8)) -eq 0
test $((0x$psp_address % 8)) -eq 0
test $((0x$bss_end)) -le $((0x20080000))
test -n "$scratch_x_start" && test "$scratch_x_start" = "$scratch_x_end"
test -n "$scratch_y_start" && test "$scratch_y_start" = "$scratch_y_end"
test -n "$stack_one_bottom" && test "$stack_one_bottom" = "$stack_one_top"
test "$stack_limit" = 20080000
test "$stack_top" = 20082000
main_sram_headroom=$((0x20080000 - 0x$bss_end))
test "$main_sram_headroom" -gt 0

printf 'RP2350 %s ELF audit PASS: arena_owner=%d PSP=%d bss_end=0x%s main_headroom=%d heap=0 dynamic-stack/legacy/allocator/GMP/system-RNG absent\n' \
    "$audit_label" $((0x$owner_size)) $((0x$psp_size)) "$bss_end" "$main_sram_headroom"
