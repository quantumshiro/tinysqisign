// SPDX-License-Identifier: Apache-2.0

#include "random_ideal_fixed_budget_sampler.h"

#if defined(__GNUC__) && !defined(__clang__)
#define RI_SAMPLER_NOINLINE __attribute__((noinline, noipa))
#elif defined(__GNUC__) || defined(__clang__)
#define RI_SAMPLER_NOINLINE __attribute__((noinline))
#else
#define RI_SAMPLER_NOINLINE
#endif


static void
set_level1_modulus(ctint1728_32_t *value)
{
    ctint1728_32_secure_clear(value, sizeof(*value));
    value->words[0] = 75;
    value->words[16] = 1;
}


static uint32_t
mul_reduce(ctint1728_32_t *out,
           const ctint1728_32_t *a,
           const ctint1728_32_t *b,
           sqisign_ri_sampler_workspace_t *workspace)
{
    uint32_t valid = (uint32_t)ctint1728_32_mul(
        &workspace->term,
        a,
        b,
        &workspace->phase.multiplication);
    valid &= (uint32_t)ctint1728_32_mod(
        out,
        &workspace->term,
        &workspace->modulus,
        &workspace->phase.reduction);
    return valid;
}


static uint32_t
add_reduce(ctint1728_32_t *accumulator,
           const ctint1728_32_t *term,
           sqisign_ri_sampler_workspace_t *workspace)
{
    uint32_t valid = (uint32_t)ctint1728_32_add(
        accumulator,
        accumulator,
        term,
        &workspace->phase.arithmetic);
    valid &= (uint32_t)ctint1728_32_mod(
        accumulator,
        accumulator,
        &workspace->modulus,
        &workspace->phase.reduction);
    return valid;
}


static uint32_t
sub_reduce(ctint1728_32_t *accumulator,
           const ctint1728_32_t *term,
           sqisign_ri_sampler_workspace_t *workspace)
{
    uint32_t valid = (uint32_t)ctint1728_32_sub(
        accumulator,
        accumulator,
        term,
        &workspace->phase.arithmetic);
    valid &= (uint32_t)ctint1728_32_mod(
        accumulator,
        accumulator,
        &workspace->modulus,
        &workspace->phase.reduction);
    return valid;
}


static uint32_t
norm_mod(ctint1728_32_t *out,
         const ctint1728_32_t coordinates[4],
         sqisign_ri_sampler_workspace_t *workspace)
{
    uint32_t valid = 1;
    ctint1728_32_copy(out, &workspace->zero);
    for (size_t index = 0; index < 2; ++index) {
        valid &= mul_reduce(
            &workspace->term,
            &coordinates[index],
            &coordinates[index],
            workspace);
        valid &= add_reduce(out, &workspace->term, workspace);
    }
    for (size_t index = 2; index < 4; ++index) {
        valid &= mul_reduce(
            &workspace->term,
            &coordinates[index],
            &coordinates[index],
            workspace);
        valid &= mul_reduce(
            &workspace->term,
            &workspace->term,
            &workspace->coefficient,
            workspace);
        valid &= add_reduce(out, &workspace->term, workspace);
    }
    return valid;
}


static uint32_t
coordinates_nonzero(const ctint1728_32_t coordinates[4])
{
    uint32_t all_zero = 1;
    for (size_t index = 0; index < 4; ++index)
        all_zero &= ctint1728_32_is_zero(&coordinates[index]);
    return all_zero ^ 1u;
}


static uint32_t
quaternion_mul_mod(ctint1728_32_t out[4],
                   const ctint1728_32_t a[4],
                   const ctint1728_32_t b[4],
                   sqisign_ri_sampler_workspace_t *workspace)
{
    uint32_t valid = 1;
    for (size_t index = 0; index < 4; ++index)
        ctint1728_32_copy(&out[index], &workspace->zero);

    /* out[0] = a0*b0 - a1*b1 - p*(a2*b2 + a3*b3). */
    valid &= mul_reduce(&workspace->term, &a[2], &b[2], workspace);
    valid &= sub_reduce(&out[0], &workspace->term, workspace);
    valid &= mul_reduce(&workspace->term, &a[3], &b[3], workspace);
    valid &= sub_reduce(&out[0], &workspace->term, workspace);
    valid &= mul_reduce(
        &out[0], &out[0], &workspace->coefficient, workspace);
    valid &= mul_reduce(&workspace->term, &a[0], &b[0], workspace);
    valid &= add_reduce(&out[0], &workspace->term, workspace);
    valid &= mul_reduce(&workspace->term, &a[1], &b[1], workspace);
    valid &= sub_reduce(&out[0], &workspace->term, workspace);

    /* out[1] = a0*b1 + a1*b0 + p*(a2*b3 - a3*b2). */
    valid &= mul_reduce(&workspace->term, &a[2], &b[3], workspace);
    valid &= add_reduce(&out[1], &workspace->term, workspace);
    valid &= mul_reduce(&workspace->term, &a[3], &b[2], workspace);
    valid &= sub_reduce(&out[1], &workspace->term, workspace);
    valid &= mul_reduce(
        &out[1], &out[1], &workspace->coefficient, workspace);
    valid &= mul_reduce(&workspace->term, &a[0], &b[1], workspace);
    valid &= add_reduce(&out[1], &workspace->term, workspace);
    valid &= mul_reduce(&workspace->term, &a[1], &b[0], workspace);
    valid &= add_reduce(&out[1], &workspace->term, workspace);

    /* out[2] = a0*b2 + a2*b0 - a1*b3 + a3*b1. */
    valid &= mul_reduce(&workspace->term, &a[0], &b[2], workspace);
    valid &= add_reduce(&out[2], &workspace->term, workspace);
    valid &= mul_reduce(&workspace->term, &a[2], &b[0], workspace);
    valid &= add_reduce(&out[2], &workspace->term, workspace);
    valid &= mul_reduce(&workspace->term, &a[1], &b[3], workspace);
    valid &= sub_reduce(&out[2], &workspace->term, workspace);
    valid &= mul_reduce(&workspace->term, &a[3], &b[1], workspace);
    valid &= add_reduce(&out[2], &workspace->term, workspace);

    /* out[3] = a0*b3 + a3*b0 - a2*b1 + a1*b2. */
    valid &= mul_reduce(&workspace->term, &a[0], &b[3], workspace);
    valid &= add_reduce(&out[3], &workspace->term, workspace);
    valid &= mul_reduce(&workspace->term, &a[3], &b[0], workspace);
    valid &= add_reduce(&out[3], &workspace->term, workspace);
    valid &= mul_reduce(&workspace->term, &a[2], &b[1], workspace);
    valid &= sub_reduce(&out[3], &workspace->term, workspace);
    valid &= mul_reduce(&workspace->term, &a[1], &b[2], workspace);
    valid &= add_reduce(&out[3], &workspace->term, workspace);
    return valid;
}


RI_SAMPLER_NOINLINE int
sqisign_ri_sample_generator_level1(
    ctint1728_32_t out[4],
    const ctint1728_32_t *coefficient,
    const uint8_t entropy[SQISIGN_RI_FIXED_RANDOM_BYTES],
    sqisign_ri_sampler_workspace_t *workspace)
{
    if (workspace == NULL)
        return 0;
    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    if (out == NULL || coefficient == NULL || entropy == NULL)
        return 0;

    for (size_t index = 0; index < 4; ++index)
        ctint1728_32_copy(&workspace->saved_output[index], &out[index]);
    ctint1728_32_copy(&workspace->coefficient, coefficient);
    set_level1_modulus(&workspace->modulus);

    uint32_t valid = (uint32_t)(
        ctint1728_32_sign(&workspace->coefficient) > 0);
    uint32_t gamma_found = 0;
    size_t entropy_offset = 0;

    for (size_t candidate_index = 0;
         candidate_index < SQISIGN_RI_GAMMA_CANDIDATES;
         ++candidate_index) {
        ctint1728_32_secure_clear(
            workspace->candidate, sizeof(workspace->candidate));
        uint32_t candidate_valid = 1;
        for (size_t coordinate = 1; coordinate < 4; ++coordinate) {
            candidate_valid &= (uint32_t)sqisign_ri_reduce_wide_coordinate(
                &workspace->candidate[coordinate],
                &entropy[entropy_offset],
                &workspace->modulus,
                &workspace->phase.coordinate);
            entropy_offset += SQISIGN_RI_WIDE_BYTES;
        }
        candidate_valid &= norm_mod(
            &workspace->norm, workspace->candidate, workspace);
        candidate_valid &= (uint32_t)ctint1728_32_sub(
            &workspace->discriminant,
            &workspace->zero,
            &workspace->norm,
            &workspace->phase.arithmetic);
        candidate_valid &= (uint32_t)ctint1728_32_mod(
            &workspace->discriminant,
            &workspace->discriminant,
            &workspace->modulus,
            &workspace->phase.reduction);
        candidate_valid &= (uint32_t)sqisign_ri_sqrt_mod_level1(
            &workspace->candidate[0],
            &workspace->discriminant,
            &workspace->phase.square_root);
        candidate_valid &= coordinates_nonzero(workspace->candidate);
        candidate_valid &= (uint32_t)sqisign_ri_correct_primitive_lift(
            workspace->candidate,
            workspace->candidate,
            &workspace->modulus,
            &workspace->coefficient,
            &workspace->phase.primitive);
        const uint32_t take =
            candidate_valid & (gamma_found ^ 1u);
        for (size_t coordinate = 0; coordinate < 4; ++coordinate) {
            ctint1728_32_cmov(
                &workspace->selected_gamma[coordinate],
                &workspace->candidate[coordinate],
                take);
        }
        gamma_found |= candidate_valid;
    }

    ctint1728_32_secure_clear(workspace->beta, sizeof(workspace->beta));
    uint32_t beta_valid = 1;
    for (size_t coordinate = 0; coordinate < 4; ++coordinate) {
        beta_valid &= (uint32_t)sqisign_ri_reduce_wide_coordinate(
            &workspace->beta[coordinate],
            &entropy[entropy_offset],
            &workspace->modulus,
            &workspace->phase.coordinate);
        entropy_offset += SQISIGN_RI_WIDE_BYTES;
    }
    beta_valid &= norm_mod(&workspace->norm, workspace->beta, workspace);
    beta_valid &= ctint1728_32_is_zero(&workspace->norm) ^ 1u;

    uint32_t product_valid = quaternion_mul_mod(
        workspace->product,
        workspace->selected_gamma,
        workspace->beta,
        workspace);
    valid &= gamma_found & beta_valid & product_valid;
    valid &= (uint32_t)(
        entropy_offset == SQISIGN_RI_FIXED_RANDOM_BYTES);

    for (size_t coordinate = 0; coordinate < 4; ++coordinate) {
        ctint1728_32_copy(
            &out[coordinate], &workspace->saved_output[coordinate]);
        ctint1728_32_cmov(
            &out[coordinate], &workspace->product[coordinate], valid);
    }
    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    return (int)valid;
}
