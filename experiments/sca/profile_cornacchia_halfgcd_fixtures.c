// SPDX-License-Identifier: Apache-2.0
// Generate exact fixed-sqrt roots and legacy half-GCD remainders for the
// public RP2350 acquisition fixtures.  This is fixture preparation only.

#include "cornacchia_halfgcd_fixed_492.h"
#include "sqrt_mod_fixedwork_521.h"
#include "intbig_internal.h"
#include "sca_cornacchia_fixtures_368.h"

#include <stdint.h>
#include <stdio.h>

static int
legacy_halfgcd(ibz_t *out,
               uint32_t *steps,
               const ibz_t *modulus,
               const ibz_t *root)
{
    ibz_t quotient;
    ibz_t r0;
    ibz_t r1;
    ibz_t r2;
    ibz_t square;
    ibz_copy(&r2, root);
    ibz_copy(&r1, modulus);
    ibz_copy(&r0, modulus);
    ibz_copy(&square, modulus);
    uint32_t count = 0;
    while (ibz_cmp(&square, modulus) >= 0) {
        if (ibz_is_zero(&r1) || count >= 2048)
            return 0;
        ibz_div(&quotient, &r0, &r2, &r1);
        ibz_mul(&square, &r0, &r0);
        ibz_copy(&r2, &r1);
        ibz_copy(&r1, &r0);
        ++count;
    }
    ibz_copy(out, &r0);
    *steps = count;
    return 1;
}

int
main(void)
{
    for (size_t index = 0;
         index < SQISIGN_SCA_CORNACCHIA_FIXTURE_COUNT;
         ++index) {
        ibz_t modulus;
        ibz_t residue;
        ibz_t root;
        ibz_t reference;
        ibz_t fixed;
        uint32_t steps = 0;
        sqisign_sca_sqrt_fixedwork_workspace_t sqrt_workspace;
        sqisign_sca_cornacchia_halfgcd_workspace_t halfgcd_workspace;
        char root_hex[IBZ_BITS + 2];
        char reference_hex[IBZ_BITS + 2];

        if (!ibz_set_from_str(&modulus,
                              sqisign_sca_cornacchia_fixtures_368[index].prime_hex,
                              16) ||
            !ibz_set_from_str(&residue,
                              sqisign_sca_cornacchia_fixtures_368[index].residue_hex,
                              16) ||
            sqisign_sca_sqrt_mod_p_fixedwork_521(
                &root, &residue, &modulus, &sqrt_workspace) !=
                SQISIGN_SCA_SQRT_SUCCESS ||
            !legacy_halfgcd(&reference, &steps, &modulus, &root) ||
            !sqisign_sca_cornacchia_halfgcd_492(
                &fixed, &modulus, &root, &halfgcd_workspace) ||
            ibz_cmp(&fixed, &reference) != 0 ||
            !ibz_convert_to_str(&root, root_hex, 16) ||
            !ibz_convert_to_str(&reference, reference_hex, 16)) {
            fprintf(stderr, "fixture %zu failed\n", index);
            return 1;
        }
        printf("    { \"%s\", \"%s\", %u },\n",
               root_hex, reference_hex, steps);
    }
    return 0;
}
