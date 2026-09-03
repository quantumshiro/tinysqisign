// SPDX-License-Identifier: Apache-2.0

#include "random_ideal_fixed_budget.h"

#if defined(__GNUC__) && !defined(__clang__)
#define RI_NOINLINE __attribute__((noinline, noipa))
#elif defined(__GNUC__) || defined(__clang__)
#define RI_NOINLINE __attribute__((noinline))
#else
#define RI_NOINLINE
#endif


static const ctint1728_32_t level1_norm = {
    .words = {
        [0] = 75,
        [16] = 1,
    },
};


static const ctint1728_32_t level1_sqrt_exponent = {
    .words = {
        [0] = 19,
        [15] = UINT32_C(0x40000000),
    },
};


static uint32_t
ct_nonzero_u32(uint32_t value)
{
    return (value | (uint32_t)(0u - value)) >> 31;
}


static uint32_t
compute_form(ctint1728_32_t *result,
             const ctint1728_32_t coordinates[4],
             const ctint1728_32_t *coefficient,
             sqisign_ri_primitive_workspace_t *workspace)
{
    uint32_t valid = (uint32_t)ctint1728_32_square(
        &workspace->square,
        &coordinates[0],
        &workspace->phase.multiplication);
    ctint1728_32_copy(result, &workspace->square);

    valid &= (uint32_t)ctint1728_32_square(
        &workspace->square,
        &coordinates[1],
        &workspace->phase.multiplication);
    valid &= (uint32_t)ctint1728_32_add(
        result,
        result,
        &workspace->square,
        &workspace->phase.arithmetic);

    for (size_t index = 2; index < 4; ++index) {
        valid &= (uint32_t)ctint1728_32_square(
            &workspace->square,
            &coordinates[index],
            &workspace->phase.multiplication);
        valid &= (uint32_t)ctint1728_32_mul(
            &workspace->weighted,
            &workspace->square,
            coefficient,
            &workspace->phase.multiplication);
        valid &= (uint32_t)ctint1728_32_add(
            result,
            result,
            &workspace->weighted,
            &workspace->phase.arithmetic);
    }
    return valid;
}


RI_NOINLINE int
sqisign_ri_reduce_wide_coordinate(
    ctint1728_32_t *out,
    const uint8_t random_block[SQISIGN_RI_WIDE_BYTES],
    const ctint1728_32_t *modulus,
    sqisign_ri_coordinate_workspace_t *workspace)
{
    if (workspace == NULL)
        return 0;

    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    if (out == NULL || random_block == NULL || modulus == NULL)
        return 0;
    ctint1728_32_copy(&workspace->saved_output, out);
    for (size_t byte = 0; byte < SQISIGN_RI_WIDE_BYTES; ++byte) {
        uint32_t value = random_block[byte];
        const uint32_t is_last = (uint32_t)(byte == SQISIGN_RI_WIDE_BYTES - 1u);
        const uint32_t mask = UINT32_C(0xff) ^ (is_last * UINT32_C(0xfe));
        value &= mask;
        workspace->wide.words[byte / 4u] |= value << (8u * (byte % 4u));
    }

    uint32_t valid = (uint32_t)(ctint1728_32_sign(modulus) > 0);
    valid &= (uint32_t)ctint1728_32_mod(
        &workspace->candidate,
        &workspace->wide,
        modulus,
        &workspace->reduction);
    ctint1728_32_copy(out, &workspace->saved_output);
    ctint1728_32_cmov(out, &workspace->candidate, valid);
    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    return (int)valid;
}


RI_NOINLINE int
sqisign_ri_sqrt_mod_level1(
    ctint1728_32_t *out,
    const ctint1728_32_t *input,
    sqisign_ri_sqrt_workspace_t *workspace)
{
    if (workspace == NULL)
        return 0;

    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    if (out == NULL || input == NULL)
        return 0;

    ctint1728_32_copy(&workspace->input, input);
    ctint1728_32_copy(&workspace->saved_output, out);
    uint32_t valid = (uint32_t)(ctint1728_32_sign(&workspace->input) >= 0);
    valid &= (uint32_t)(
        ctint1728_32_cmp_unsigned(&workspace->input, &level1_norm) < 0);
    valid &= (uint32_t)ctint1728_32_pow_mod_521(
        &workspace->root,
        &workspace->input,
        &level1_sqrt_exponent,
        &level1_norm,
        &workspace->phase.exponentiation);
    valid &= (uint32_t)ctint1728_32_square(
        &workspace->square,
        &workspace->root,
        &workspace->phase.multiplication);
    valid &= (uint32_t)ctint1728_32_mod(
        &workspace->remainder,
        &workspace->square,
        &level1_norm,
        &workspace->phase.reduction);

    uint32_t difference = 0;
    for (size_t index = 0; index < CTINT1728_WORDS; ++index) {
        difference |= workspace->remainder.words[index] ^
                      workspace->input.words[index];
    }
    valid &= 1u ^ ct_nonzero_u32(difference);
    ctint1728_32_copy(out, &workspace->saved_output);
    ctint1728_32_cmov(out, &workspace->root, valid);
    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    return (int)valid;
}


RI_NOINLINE int
sqisign_ri_correct_primitive_lift(
    ctint1728_32_t out[4],
    const ctint1728_32_t coordinates[4],
    const ctint1728_32_t *modulus,
    const ctint1728_32_t *coefficient,
    sqisign_ri_primitive_workspace_t *workspace)
{
    if (workspace == NULL)
        return 0;

    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    if (out == NULL || coordinates == NULL || modulus == NULL ||
        coefficient == NULL)
        return 0;
    uint32_t valid = (uint32_t)(ctint1728_32_sign(modulus) > 0);
    valid &= modulus->words[0] & 1u;
    valid &= (uint32_t)(ctint1728_32_sign(coefficient) > 0);
    for (size_t index = 0; index < 4; ++index) {
        ctint1728_32_copy(&workspace->input[index], &coordinates[index]);
        ctint1728_32_copy(&workspace->saved_output[index], &out[index]);
        ctint1728_32_copy(&workspace->candidate[index], &coordinates[index]);
        valid &= (uint32_t)(ctint1728_32_sign(&coordinates[index]) >= 0);
        valid &= (uint32_t)(
            ctint1728_32_cmp_unsigned(&coordinates[index], modulus) < 0);
    }

    valid &= (uint32_t)ctint1728_32_mul(
        &workspace->modulus_squared,
        modulus,
        modulus,
        &workspace->phase.multiplication);
    valid &= compute_form(
        &workspace->norm,
        workspace->input,
        coefficient,
        workspace);
    valid &= (uint32_t)ctint1728_32_mod(
        &workspace->remainder_mod_n,
        &workspace->norm,
        modulus,
        &workspace->phase.reduction);
    valid &= (uint32_t)ctint1728_32_mod(
        &workspace->remainder_mod_n2,
        &workspace->norm,
        &workspace->modulus_squared,
        &workspace->phase.reduction);
    valid &= (uint32_t)ctint1728_32_mod(
        &workspace->coefficient_mod_n,
        coefficient,
        modulus,
        &workspace->phase.reduction);

    const uint32_t input_is_isotropic =
        ctint1728_32_is_zero(&workspace->remainder_mod_n);
    const uint32_t n2_divides =
        ctint1728_32_is_zero(&workspace->remainder_mod_n2);
    const uint32_t coefficient_nonzero =
        1u ^ ctint1728_32_is_zero(&workspace->coefficient_mod_n);
    uint32_t found_nonzero = 0;

    for (size_t index = 0; index < 4; ++index) {
        const uint32_t coordinate_nonzero =
            1u ^ ctint1728_32_is_zero(&workspace->input[index]);
        const uint32_t take =
            n2_divides & coordinate_nonzero & (found_nonzero ^ 1u);
        /* Choose a different integral lift of the *same* residue modulo N.
         * Replacing xi by xi-N changes Q/N by -2*ai*xi modulo N, but does
         * not change the quaternion in O_0/N O_0.  Using N-xi here would
         * make the same norm correction while negating that residue and
         * would unnecessarily change the subsequent sampling map. */
        valid &= (uint32_t)ctint1728_32_sub(
            &workspace->alternate,
            &workspace->input[index],
            modulus,
            &workspace->phase.arithmetic);
        ctint1728_32_cmov(
            &workspace->candidate[index],
            &workspace->alternate,
            take);
        found_nonzero |= coordinate_nonzero;
    }

    /* Recompute both predicates after the masked correction.  This is
     * redundant with the algebraic proof, but makes the component fail
     * closed if its integration contract or representation ever changes. */
    valid &= compute_form(
        &workspace->norm,
        workspace->candidate,
        coefficient,
        workspace);
    valid &= (uint32_t)ctint1728_32_mod(
        &workspace->remainder_mod_n,
        &workspace->norm,
        modulus,
        &workspace->phase.reduction);
    valid &= (uint32_t)ctint1728_32_mod(
        &workspace->remainder_mod_n2,
        &workspace->norm,
        &workspace->modulus_squared,
        &workspace->phase.reduction);
    const uint32_t output_is_isotropic =
        ctint1728_32_is_zero(&workspace->remainder_mod_n);
    const uint32_t output_is_primitive =
        1u ^ ctint1728_32_is_zero(&workspace->remainder_mod_n2);

    valid &= input_is_isotropic & output_is_isotropic &
             output_is_primitive & found_nonzero & coefficient_nonzero;
    for (size_t index = 0; index < 4; ++index) {
        ctint1728_32_copy(&out[index], &workspace->saved_output[index]);
        ctint1728_32_cmov(&out[index], &workspace->candidate[index], valid);
    }
    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    return (int)valid;
}
