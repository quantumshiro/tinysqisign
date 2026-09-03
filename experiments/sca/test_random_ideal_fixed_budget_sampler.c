// SPDX-License-Identifier: Apache-2.0

#include "random_ideal_fixed_budget_sampler.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef union {
    ctint1728_32_arith_workspace_t arithmetic;
    ctint1728_32_mul_workspace_t multiplication;
    ctint1728_32_mod_workspace_t reduction;
} reference_phase_t;


static uint64_t rng_state = UINT64_C(0x243f6a8885a308d3);


static void
fail(const char *label)
{
    fprintf(stderr, "random-ideal fixed-budget sampler FAIL: %s\n", label);
    exit(1);
}


static uint64_t
next_u64(void)
{
    uint64_t value = rng_state;
    value ^= value >> 12;
    value ^= value << 25;
    value ^= value >> 27;
    rng_state = value;
    return value * UINT64_C(0x2545f4914f6cdd1d);
}


static int
all_zero(const void *object, size_t bytes)
{
    const uint8_t *cursor = object;
    uint8_t aggregate = 0;
    for (size_t index = 0; index < bytes; ++index)
        aggregate |= cursor[index];
    return aggregate == 0;
}


static int
same_coordinates(const ctint1728_32_t a[4], const ctint1728_32_t b[4])
{
    return memcmp(a, b, 4 * sizeof(a[0])) == 0;
}


static void
set_u64(ctint1728_32_t *value, uint64_t word)
{
    memset(value, 0, sizeof(*value));
    value->words[0] = (uint32_t)word;
    value->words[1] = (uint32_t)(word >> 32);
}


static void
set_modulus(ctint1728_32_t *value)
{
    memset(value, 0, sizeof(*value));
    value->words[0] = 75;
    value->words[16] = 1;
}


static int
reference_norm_mod(ctint1728_32_t *out,
                   const ctint1728_32_t coordinates[4],
                   const ctint1728_32_t *coefficient)
{
    ctint1728_32_t modulus;
    ctint1728_32_t term;
    reference_phase_t phase;
    set_modulus(&modulus);
    memset(out, 0, sizeof(*out));
    memset(&term, 0, sizeof(term));
    uint32_t valid = 1;
    for (size_t index = 0; index < 4; ++index) {
        valid &= (uint32_t)ctint1728_32_square(
            &term, &coordinates[index], &phase.multiplication);
        valid &= (uint32_t)ctint1728_32_mod(
            &term, &term, &modulus, &phase.reduction);
        if (index >= 2) {
            valid &= (uint32_t)ctint1728_32_mul(
                &term, &term, coefficient, &phase.multiplication);
            valid &= (uint32_t)ctint1728_32_mod(
                &term, &term, &modulus, &phase.reduction);
        }
        valid &= (uint32_t)ctint1728_32_add(
            out, out, &term, &phase.arithmetic);
        valid &= (uint32_t)ctint1728_32_mod(
            out, out, &modulus, &phase.reduction);
    }
    ctint1728_32_secure_clear(&term, sizeof(term));
    ctint1728_32_secure_clear(&phase, sizeof(phase));
    return (int)valid;
}


static uint64_t
coordinate_digest(const ctint1728_32_t coordinates[4])
{
    uint64_t digest = UINT64_C(1469598103934665603);
    const uint8_t *bytes = (const uint8_t *)coordinates;
    for (size_t index = 0; index < 4 * sizeof(coordinates[0]); ++index) {
        digest ^= bytes[index];
        digest *= UINT64_C(1099511628211);
    }
    return digest;
}


int
main(void)
{
    static uint8_t entropy[SQISIGN_RI_FIXED_RANDOM_BYTES];
    static uint8_t zero_entropy[SQISIGN_RI_FIXED_RANDOM_BYTES];
    static sqisign_ri_sampler_workspace_t workspace;
    ctint1728_32_t coefficient;
    ctint1728_32_t modulus;
    ctint1728_32_t output[4];
    ctint1728_32_t success_output[4];
    ctint1728_32_t sentinel[4];
    ctint1728_32_t norm;

    for (size_t index = 0; index < sizeof(entropy); index += 8) {
        const uint64_t word = next_u64();
        const size_t remaining = sizeof(entropy) - index;
        memcpy(&entropy[index], &word, remaining < 8 ? remaining : 8);
    }
    set_u64(&coefficient, 1);
    set_modulus(&modulus);
    memset(output, 0xa5, sizeof(output));

    if (!sqisign_ri_sample_generator_level1(
            output, &coefficient, entropy, &workspace))
        fail("success-status");
    if (!all_zero(&workspace, sizeof(workspace)))
        fail("success-clear");
    for (size_t index = 0; index < 4; ++index) {
        if (ctint1728_32_sign(&output[index]) < 0 ||
            ctint1728_32_cmp_unsigned(&output[index], &modulus) >= 0)
            fail("canonical-output");
    }
    if (!reference_norm_mod(&norm, output, &coefficient) ||
        !ctint1728_32_is_zero(&norm))
        fail("output-norm");
    memcpy(success_output, output, sizeof(success_output));

    for (size_t index = 0; index < 4; ++index)
        set_u64(&sentinel[index], UINT64_C(0xa5000000) + index);
    memcpy(output, sentinel, sizeof(output));
    if (sqisign_ri_sample_generator_level1(
            output, &coefficient, zero_entropy, &workspace) ||
        !same_coordinates(output, sentinel) ||
        !all_zero(&workspace, sizeof(workspace)))
        fail("exhaustion");

    memset(&workspace, 0xa5, sizeof(workspace));
    if (sqisign_ri_sample_generator_level1(
            NULL, &coefficient, entropy, &workspace) ||
        !all_zero(&workspace, sizeof(workspace)))
        fail("null-output");
    memset(&workspace, 0xa5, sizeof(workspace));
    if (sqisign_ri_sample_generator_level1(
            output, NULL, entropy, &workspace) ||
        !same_coordinates(output, sentinel) ||
        !all_zero(&workspace, sizeof(workspace)))
        fail("null-coefficient");
    memset(&workspace, 0xa5, sizeof(workspace));
    if (sqisign_ri_sample_generator_level1(
            output, &coefficient, NULL, &workspace) ||
        !same_coordinates(output, sentinel) ||
        !all_zero(&workspace, sizeof(workspace)))
        fail("null-entropy");
    if (sqisign_ri_sample_generator_level1(
            output, &coefficient, entropy, NULL) ||
        !same_coordinates(output, sentinel))
        fail("null-workspace");

    printf(
        "random-ideal fixed-budget sampler PASS: gamma=130 beta=1 "
        "coordinates=394 entropy=38218 workspace=12096 "
        "digest=%016" PRIx64
        " distribution-proof=separate physical=false\n",
        coordinate_digest(success_output));
    return 0;
}
