// SPDX-License-Identifier: Apache-2.0
#ifndef SQISIGN_SCA_POWMOD_ALWAYS_521_H
#define SQISIGN_SCA_POWMOD_ALWAYS_521_H

#include <intbig.h>
#include <stdint.h>

enum {
    SQISIGN_SCA_POWMOD_BITS = 521,
    SQISIGN_SCA_POWMOD_MULTIPLICATIONS = 2 * SQISIGN_SCA_POWMOD_BITS,
};

/* Experimental exponent-branch regularization for the frozen SCA diagnostic.
 *
 * The accepted domain is deliberately narrow: m and e must be nonnegative,
 * m must be nonzero, and both must fit in 521 bits.  On success this computes
 * x^e mod m and returns one.  Invalid input returns zero without publishing a
 * result.
 *
 * Every accepted call executes 521 iterations and two modular
 * multiplications per iteration.  Selection is a full-limb mask operation;
 * there is no source-level branch on the exponent bit.  This removes the
 * current square-and-multiply Hamming-weight operation-count channel, but it
 * is NOT a constant-time primitive: ibz_mul, ibz_mod and their division and
 * used-limb descendants remain variable-time and data-dependent. */
int sqisign_sca_powmod_always_521(ibz_t *power,
                                  const ibz_t *x,
                                  const ibz_t *e,
                                  const ibz_t *m);

#if defined(SQISIGN_SCA_POWMOD_PROFILE)
typedef struct {
    uint64_t iterations;
    uint64_t modular_multiplications;
} sqisign_sca_powmod_profile_t;

void sqisign_sca_powmod_profile_reset(void);
sqisign_sca_powmod_profile_t sqisign_sca_powmod_profile_read(void);
#endif

#endif
