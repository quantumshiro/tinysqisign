// SPDX-License-Identifier: Apache-2.0
#ifndef SQISIGN_SCA_POWMOD_FIXEDWORK_521_H
#define SQISIGN_SCA_POWMOD_FIXEDWORK_521_H

#include <intbig.h>
#include <stdint.h>

enum {
    SQISIGN_SCA_FIXEDWORK_BITS = 521,
    SQISIGN_SCA_FIXEDWORK_WORDS = 17,
    SQISIGN_SCA_FIXEDWORK_POW_MULTIPLICATIONS =
        2 * SQISIGN_SCA_FIXEDWORK_BITS,
    SQISIGN_SCA_FIXEDWORK_MUL_ITERATIONS =
        SQISIGN_SCA_FIXEDWORK_BITS,
    SQISIGN_SCA_FIXEDWORK_POW_MUL_ITERATIONS =
        SQISIGN_SCA_FIXEDWORK_POW_MULTIPLICATIONS *
        SQISIGN_SCA_FIXEDWORK_MUL_ITERATIONS,
    SQISIGN_SCA_FIXEDWORK_POW_MODULAR_ADDITIONS =
        2 * SQISIGN_SCA_FIXEDWORK_POW_MUL_ITERATIONS,
};

/* Experimental fixed-work arithmetic for the 521-bit SCA diagnostic.
 *
 * The accepted domain is deliberately narrow: all inputs must be
 * nonnegative and fit in 521 bits, modulus must be nonzero, and the base (or
 * both multiplication operands) must already be strictly below modulus.
 * Invalid input returns zero without publishing output.
 *
 * Every accepted modular multiplication executes 521 multiplier-bit rounds.
 * Every round executes two complete 17-word modular additions.  The pow
 * operation executes 521 exponent rounds and two such multiplications per
 * round.  There are no source-level secret-bit branches, used-limb scans,
 * division calls, or table lookups in the accepted arithmetic path.
 *
 * This is still an experimental constant-work C construction, not a complete
 * constant-time or power-analysis-resistance claim.  Compiler output must be
 * audited per target, operand values remain visible to power/EM, and the rest
 * of SQIsign Sign remains variable-work. */
int sqisign_sca_mul_mod_fixedwork_521(ibz_t *product,
                                      const ibz_t *a,
                                      const ibz_t *b,
                                      const ibz_t *modulus);

int sqisign_sca_powmod_fixedwork_521(ibz_t *power,
                                     const ibz_t *x,
                                     const ibz_t *e,
                                     const ibz_t *m);

#if defined(SQISIGN_SCA_FIXEDWORK_PROFILE)
typedef struct {
    uint64_t exponent_iterations;
    uint64_t modular_multiplications;
    uint64_t multiplier_iterations;
    uint64_t modular_additions;
} sqisign_sca_fixedwork_profile_t;

void sqisign_sca_fixedwork_profile_reset(void);
sqisign_sca_fixedwork_profile_t sqisign_sca_fixedwork_profile_read(void);
#endif

#endif
