// SPDX-License-Identifier: Apache-2.0

#include "powmod_always_521.h"

#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__) && !defined(__clang__)
#define SQISIGN_SCA_NOINLINE __attribute__((noinline, noclone))
#elif defined(__GNUC__) || defined(__clang__)
#define SQISIGN_SCA_NOINLINE __attribute__((noinline))
#else
#define SQISIGN_SCA_NOINLINE
#endif

_Static_assert(IBZ_BITS >= 2 * SQISIGN_SCA_POWMOD_BITS + 1,
               "521-bit residues must multiply without signed overflow");

#if defined(SQISIGN_SCA_POWMOD_PROFILE)
static sqisign_sca_powmod_profile_t profile;

void
sqisign_sca_powmod_profile_reset(void)
{
    profile.iterations = 0;
    profile.modular_multiplications = 0;
}

sqisign_sca_powmod_profile_t
sqisign_sca_powmod_profile_read(void)
{
    return profile;
}
#endif

static int
fits_nonnegative_521(const ibz_t *value)
{
    const size_t top_limb = (SQISIGN_SCA_POWMOD_BITS - 1) / 64;
    const unsigned top_bits = SQISIGN_SCA_POWMOD_BITS - 64 * top_limb;
    const uint64_t top_mask = (UINT64_C(1) << top_bits) - 1;

    if (ibz_is_negative(value) || ((*value)[top_limb] & ~top_mask) != 0)
        return 0;
    for (size_t limb = top_limb + 1; limb < IBZ_LIMBS; ++limb)
        if ((*value)[limb] != 0)
            return 0;
    return 1;
}

/* This helper is externally visible and out of line so the linked Arm audit
 * can prove that the regularized loop has two identical modular-multiply call
 * sites.  Its 1042-bit mathematical product fits in Level-I's signed 1728-bit
 * ibz_t because both inputs are reduced modulo a <=521-bit modulus. */
SQISIGN_SCA_NOINLINE void
sqisign_sca_mul_mod_521(ibz_t *product,
                        const ibz_t *a,
                        const ibz_t *b,
                        const ibz_t *modulus)
{
    ibz_t wide_enough_product;
    ibz_mul(&wide_enough_product, a, b);
    ibz_mod(product, &wide_enough_product, modulus);
#if defined(SQISIGN_SCA_POWMOD_PROFILE)
    ++profile.modular_multiplications;
#endif
}

int
sqisign_sca_powmod_always_521(ibz_t *power,
                              const ibz_t *x,
                              const ibz_t *e,
                              const ibz_t *m)
{
    if (!fits_nonnegative_521(e) || !fits_nonnegative_521(m) ||
        ibz_is_zero(m))
        return 0;

    ibz_t base, result, one, square, multiplied;
    ibz_mod(&base, x, m);
    ibz_set(&one, 1);
    ibz_mod(&result, &one, m);

    for (int bit = SQISIGN_SCA_POWMOD_BITS - 1; bit >= 0; --bit) {
        sqisign_sca_mul_mod_521(&square, &result, &result, m);
        sqisign_sca_mul_mod_521(&multiplied, &square, &base, m);

        const uint64_t exponent_bit =
            ((*e)[(unsigned)bit / 64] >> ((unsigned)bit % 64)) & UINT64_C(1);
        const uint64_t select_multiplied = UINT64_C(0) - exponent_bit;
        const uint64_t select_square = ~select_multiplied;
        for (size_t limb = 0; limb < IBZ_LIMBS; ++limb) {
            result[limb] =
                (square[limb] & select_square) |
                (multiplied[limb] & select_multiplied);
        }
#if defined(SQISIGN_SCA_POWMOD_PROFILE)
        ++profile.iterations;
#endif
    }

    ibz_copy(power, &result);
    return 1;
}
