// SPDX-License-Identifier: Apache-2.0
#ifndef SQISIGN_RANDOM_IDEAL_FIXED_BUDGET_H
#define SQISIGN_RANDOM_IDEAL_FIXED_BUDGET_H

#include "ctint1728_32.h"

#include <stddef.h>
#include <stdint.h>

enum {
    SQISIGN_RI_WIDE_BITS = 769,
    SQISIGN_RI_WIDE_BYTES = 97,
    SQISIGN_RI_GAMMA_CANDIDATES = 130,
    SQISIGN_RI_BETA_CANDIDATES = 1,
};

typedef struct {
    ctint1728_32_t wide;
    ctint1728_32_t candidate;
    ctint1728_32_t saved_output;
    ctint1728_32_mod_workspace_t reduction;
} sqisign_ri_coordinate_workspace_t;

typedef union {
    ctint1728_32_pow_workspace_t exponentiation;
    ctint1728_32_mul_workspace_t multiplication;
    ctint1728_32_mod_workspace_t reduction;
} sqisign_ri_sqrt_phase_workspace_t;

typedef struct {
    ctint1728_32_t input;
    ctint1728_32_t saved_output;
    ctint1728_32_t root;
    ctint1728_32_t square;
    ctint1728_32_t remainder;
    sqisign_ri_sqrt_phase_workspace_t phase;
} sqisign_ri_sqrt_workspace_t;

typedef union {
    ctint1728_32_arith_workspace_t arithmetic;
    ctint1728_32_mul_workspace_t multiplication;
    ctint1728_32_mod_workspace_t reduction;
} sqisign_ri_primitive_phase_workspace_t;

typedef struct {
    ctint1728_32_t input[4];
    ctint1728_32_t saved_output[4];
    ctint1728_32_t candidate[4];
    ctint1728_32_t modulus_squared;
    ctint1728_32_t norm;
    ctint1728_32_t square;
    ctint1728_32_t weighted;
    ctint1728_32_t remainder_mod_n;
    ctint1728_32_t remainder_mod_n2;
    ctint1728_32_t coefficient_mod_n;
    ctint1728_32_t alternate;
    sqisign_ri_primitive_phase_workspace_t phase;
} sqisign_ri_primitive_workspace_t;

_Static_assert(sizeof(sqisign_ri_coordinate_workspace_t) == 2808,
               "random-ideal coordinate workspace ABI changed");
_Static_assert(_Alignof(sqisign_ri_coordinate_workspace_t) == 4,
               "random-ideal coordinate workspace alignment changed");
_Static_assert(sizeof(sqisign_ri_sqrt_phase_workspace_t) == 3240,
               "random-ideal square-root phase workspace ABI changed");
_Static_assert(sizeof(sqisign_ri_sqrt_workspace_t) == 4320,
               "random-ideal square-root workspace ABI changed");
_Static_assert(_Alignof(sqisign_ri_sqrt_workspace_t) == 4,
               "random-ideal square-root workspace alignment changed");
_Static_assert(sizeof(sqisign_ri_primitive_phase_workspace_t) == 2160,
               "random-ideal primitive phase workspace ABI changed");
_Static_assert(sizeof(sqisign_ri_primitive_workspace_t) == 6480,
               "random-ideal primitive workspace ABI changed");
_Static_assert(_Alignof(sqisign_ri_primitive_workspace_t) == 4,
               "random-ideal primitive workspace alignment changed");

/* These are experimental fixed-width components used by the opt-in protected
 * random-ideal sampler.  They regularize software control flow and addresses;
 * they do not provide masking, constant-power behavior, physical power/EM
 * resistance, or a side-channel-resistant SQIsign signer.  The primitive
 * correction below preserves the sampled class modulo N.  The separate
 * output-distribution argument additionally relies on the sampler's complete
 * fixed schedule and on right multiplication by an independently sampled
 * invertible beta; this component API alone does not establish that result. */

/* Decode exactly 769 low bits from a fixed 97-byte little-endian block and
 * reduce them modulo ``modulus`` with ctint1728's complete restoring schedule.
 * Output may alias no workspace field.  A failed public-domain validation
 * leaves output byte-identical and every return clears the workspace.
 *
 * This component regularizes control flow and addresses only.  Its Level-I
 * statistical-distance bound assumes the public modulus N=2^512+75 and the
 * complete 394-coordinate schedule from the design contract. */
int sqisign_ri_reduce_wide_coordinate(
    ctint1728_32_t *out,
    const uint8_t random_block[SQISIGN_RI_WIDE_BYTES],
    const ctint1728_32_t *modulus,
    sqisign_ri_coordinate_workspace_t *workspace);

/* Compute a square root modulo the fixed Level-I commitment norm
 * N=2^512+75 using a^((N+1)/4), then execute a fixed square/reduction check.
 * N is prime and 3 mod 4.  Zero, residues and nonresidues execute the same
 * arithmetic schedule; return one and publish only for zero or a residue.
 * Output/input aliasing is allowed.  A non-NULL workspace is cleared on every
 * return and must be disjoint from the live input/output objects. */
int sqisign_ri_sqrt_mod_level1(
    ctint1728_32_t *out,
    const ctint1728_32_t *input,
    sqisign_ri_sqrt_workspace_t *workspace);

/* Given canonical coordinates and Q=x0^2+x1^2+a*x2^2+a*x3^2 divisible by
 * odd ``modulus``, publish a primitive lift.  If modulus^2 divides Q, the
 * first nonzero coordinate is replaced by xi-modulus using masks.  This is
 * an integral lift of the same residue modulo modulus; the corrected form is
 * recomputed and checked with the same complete schedules.
 * Invalid inputs execute the arithmetic schedule, leave all outputs
 * byte-identical, and clear the workspace.  Output/input aliasing is allowed;
 * the complete workspace must be disjoint from every live input/output. */
int sqisign_ri_correct_primitive_lift(
    ctint1728_32_t out[4],
    const ctint1728_32_t coordinates[4],
    const ctint1728_32_t *modulus,
    const ctint1728_32_t *coefficient,
    sqisign_ri_primitive_workspace_t *workspace);

#endif
