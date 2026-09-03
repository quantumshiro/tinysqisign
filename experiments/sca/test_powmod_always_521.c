// SPDX-License-Identifier: Apache-2.0

#include "powmod_always_521.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { INPUT_WORDS = 17, RANDOM_CASES = 64 };

static const uint32_t modulus_words[INPUT_WORDS] = {
    UINT32_C(0xffffffff), UINT32_C(0xffffffff), UINT32_C(0xffffffff),
    UINT32_C(0xffffffff), UINT32_C(0xffffffff), UINT32_C(0xffffffff),
    UINT32_C(0xffffffff), UINT32_C(0xffffffff), UINT32_C(0xffffffff),
    UINT32_C(0xffffffff), UINT32_C(0xffffffff), UINT32_C(0xffffffff),
    UINT32_C(0xffffffff), UINT32_C(0xffffffff), UINT32_C(0xffffffff),
    UINT32_C(0xffffffff), UINT32_C(0x000001ff),
};

static const uint32_t base_words[INPUT_WORDS] = {
    UINT32_C(0x9216d5d9), UINT32_C(0xb5470917), UINT32_C(0x3f84d5b5),
    UINT32_C(0xc97c50dd), UINT32_C(0xc0ac29b7), UINT32_C(0x34e90c6c),
    UINT32_C(0xbe5466cf), UINT32_C(0x38d01377), UINT32_C(0x452821e6),
    UINT32_C(0xec4e6c89), UINT32_C(0x082efa98), UINT32_C(0x299f31d0),
    UINT32_C(0xa4093822), UINT32_C(0x03707344), UINT32_C(0x13198a2e),
    UINT32_C(0x85a308d3), UINT32_C(0x00000088),
};

static uint64_t random_state = UINT64_C(0x6a09e667f3bcc909);

/* intbig.c contains RNG-using functions outside this focused executable's
 * path.  Supply a fail-closed stub so a non-dead-stripping host linker still
 * resolves the complete translation unit. */
int
randombytes(unsigned char *output, unsigned long long length)
{
    (void)output;
    (void)length;
    return -1;
}

static uint64_t
splitmix64_next(void)
{
    uint64_t z = (random_state += UINT64_C(0x9e3779b97f4a7c15));
    z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
    return z ^ (z >> 31);
}

static void
make_random_exponent(uint32_t words[INPUT_WORDS])
{
    for (size_t i = 0; i < INPUT_WORDS; i += 2) {
        const uint64_t value = splitmix64_next();
        words[i] = (uint32_t)value;
        if (i + 1 < INPUT_WORDS)
            words[i + 1] = (uint32_t)(value >> 32);
    }
    words[INPUT_WORDS - 1] &= UINT32_C(0x000001ff);
    words[INPUT_WORDS - 1] |= UINT32_C(0x00000100);
}

static int
run_case(const ibz_t *base, const ibz_t *exponent, const ibz_t *modulus)
{
    ibz_t legacy, regularized;
    ibz_pow_mod(&legacy, base, exponent, modulus);
    sqisign_sca_powmod_profile_reset();
    if (!sqisign_sca_powmod_always_521(
            &regularized, base, exponent, modulus))
        return 0;
    const sqisign_sca_powmod_profile_t observed =
        sqisign_sca_powmod_profile_read();
    return ibz_cmp(&legacy, &regularized) == 0 &&
           observed.iterations == SQISIGN_SCA_POWMOD_BITS &&
           observed.modular_multiplications ==
               SQISIGN_SCA_POWMOD_MULTIPLICATIONS;
}

int
main(void)
{
    ibz_t base, exponent, modulus;
    ibz_copy_u32_digits(&base, base_words, INPUT_WORDS);
    ibz_copy_u32_digits(&modulus, modulus_words, INPUT_WORDS);

    uint32_t words[INPUT_WORDS] = { 0 };
    ibz_init(&exponent);
    if (!run_case(&base, &exponent, &modulus))
        return 1;
    words[0] = 1;
    ibz_copy_u32_digits(&exponent, words, INPUT_WORDS);
    if (!run_case(&base, &exponent, &modulus))
        return 1;
    memset(words, 0, sizeof(words));
    words[INPUT_WORDS - 1] = UINT32_C(0x00000100);
    ibz_copy_u32_digits(&exponent, words, INPUT_WORDS);
    if (!run_case(&base, &exponent, &modulus))
        return 1;
    for (size_t i = 0; i + 1 < INPUT_WORDS; ++i)
        words[i] = UINT32_MAX;
    words[INPUT_WORDS - 1] = UINT32_C(0x000001ff);
    ibz_copy_u32_digits(&exponent, words, INPUT_WORDS);
    if (!run_case(&base, &exponent, &modulus))
        return 1;

    for (unsigned test = 0; test < RANDOM_CASES; ++test) {
        make_random_exponent(words);
        ibz_copy_u32_digits(&exponent, words, INPUT_WORDS);
        if (!run_case(&base, &exponent, &modulus))
            return 1;
    }

    /* Output/input aliasing is useful to preserve in the eventual primitive. */
    make_random_exponent(words);
    ibz_copy_u32_digits(&exponent, words, INPUT_WORDS);
    ibz_t expected, alias_base, alias_exponent;
    ibz_pow_mod(&expected, &base, &exponent, &modulus);
    ibz_copy(&alias_base, &base);
    if (!sqisign_sca_powmod_always_521(
            &alias_base, &alias_base, &exponent, &modulus) ||
        ibz_cmp(&alias_base, &expected) != 0)
        return 1;
    ibz_copy(&alias_exponent, &exponent);
    if (!sqisign_sca_powmod_always_521(
            &alias_exponent, &base, &alias_exponent, &modulus) ||
        ibz_cmp(&alias_exponent, &expected) != 0)
        return 1;

    /* Invalid input must fail without publishing the sentinel. */
    ibz_t invalid, sentinel, output;
    ibz_set(&invalid, -1);
    ibz_set(&sentinel, 12345);
    ibz_copy(&output, &sentinel);
    if (sqisign_sca_powmod_always_521(
            &output, &base, &invalid, &modulus) ||
        ibz_cmp(&output, &sentinel) != 0)
        return 1;
    ibz_init(&invalid);
    invalid[8] = UINT64_C(1) << 9;
    ibz_copy(&output, &sentinel);
    if (sqisign_sca_powmod_always_521(
            &output, &base, &invalid, &modulus) ||
        ibz_cmp(&output, &sentinel) != 0)
        return 1;

    printf("powmod-always-521 PASS cases=%u iterations=%u multiplications=%u\n",
           RANDOM_CASES + 4,
           SQISIGN_SCA_POWMOD_BITS,
           SQISIGN_SCA_POWMOD_MULTIPLICATIONS);
    return 0;
}
