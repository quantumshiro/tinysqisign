// SPDX-License-Identifier: Apache-2.0
#ifndef SQISIGN_RANDOM_IDEAL_FIXED_BUDGET_SAMPLER_H
#define SQISIGN_RANDOM_IDEAL_FIXED_BUDGET_SAMPLER_H

#include "random_ideal_fixed_budget.h"

#include <stddef.h>
#include <stdint.h>

enum {
    SQISIGN_RI_COORDINATE_DRAWS =
        3 * SQISIGN_RI_GAMMA_CANDIDATES +
        4 * SQISIGN_RI_BETA_CANDIDATES,
    SQISIGN_RI_FIXED_RANDOM_BYTES =
        SQISIGN_RI_COORDINATE_DRAWS * SQISIGN_RI_WIDE_BYTES,
};

typedef union {
    sqisign_ri_coordinate_workspace_t coordinate;
    sqisign_ri_sqrt_workspace_t square_root;
    sqisign_ri_primitive_workspace_t primitive;
    ctint1728_32_arith_workspace_t arithmetic;
    ctint1728_32_mul_workspace_t multiplication;
    ctint1728_32_mod_workspace_t reduction;
} sqisign_ri_sampler_phase_workspace_t;

typedef struct {
    ctint1728_32_t selected_gamma[4];
    ctint1728_32_t candidate[4];
    ctint1728_32_t beta[4];
    ctint1728_32_t product[4];
    ctint1728_32_t saved_output[4];
    ctint1728_32_t modulus;
    ctint1728_32_t coefficient;
    ctint1728_32_t norm;
    ctint1728_32_t discriminant;
    ctint1728_32_t term;
    ctint1728_32_t zero;
    sqisign_ri_sampler_phase_workspace_t phase;
} sqisign_ri_sampler_workspace_t;

_Static_assert(SQISIGN_RI_COORDINATE_DRAWS == 394,
               "random-ideal coordinate schedule changed");
_Static_assert(SQISIGN_RI_FIXED_RANDOM_BYTES == 38218,
               "random-ideal entropy schedule changed");
_Static_assert(sizeof(sqisign_ri_sampler_phase_workspace_t) == 6480,
               "random-ideal sampler phase ABI changed");
_Static_assert(sizeof(sqisign_ri_sampler_workspace_t) == 12096,
               "random-ideal sampler workspace ABI changed");
_Static_assert(_Alignof(sqisign_ri_sampler_workspace_t) == 4,
               "random-ideal sampler workspace alignment changed");

/* Experimental exact-Level-I generator sampler.
 *
 * The caller must supply exactly 38,218 already-generated random bytes.  The
 * function always decodes 394 coordinates, processes all 130 gamma candidates
 * and the one beta candidate, and mask-selects the first valid gamma.  It then
 * computes gamma*beta modulo N in the quaternion basis (1,i,j,ij), where
 * i^2=-1 and j^2=-coefficient.  The fixed public modulus is N=2^512+75.
 *
 * Return one and publish four canonical coordinates only if the complete
 * schedule succeeds.  Failure leaves output byte-identical.  Output may alias
 * coefficient, but neither may overlap entropy or workspace.  Every return
 * with a non-NULL workspace clears that complete workspace.
 *
 * This API regularizes software control flow, addresses, entropy consumption
 * and first-success publication.  It does not itself construct an ideal,
 * mask operands, provide constant-power behavior, or establish physical
 * side-channel resistance.  The repository's separate distribution argument
 * proves uniform output ideals in the exact-uniform model and bounds the
 * wide-reduction deviation; that proof is not a physical leakage claim. */
int sqisign_ri_sample_generator_level1(
    ctint1728_32_t out[4],
    const ctint1728_32_t *coefficient,
    const uint8_t entropy[SQISIGN_RI_FIXED_RANDOM_BYTES],
    sqisign_ri_sampler_workspace_t *workspace);

#endif
