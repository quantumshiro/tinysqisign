// SPDX-License-Identifier: Apache-2.0

#include "powmod_fixedwork_521.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    INPUT_WORDS = 17,
    MULTIPLICATION_CASES = 128,
    RANDOM_POW_CASES = 4,
};

static const uint32_t mersenne_modulus[INPUT_WORDS] = {
    UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX,
    UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX,
    UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX,
    UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX,
    UINT32_C(0x1ff),
};

static const uint32_t fixed_base[INPUT_WORDS] = {
    UINT32_C(0x9216d5d9), UINT32_C(0xb5470917), UINT32_C(0x3f84d5b5),
    UINT32_C(0xc97c50dd), UINT32_C(0xc0ac29b7), UINT32_C(0x34e90c6c),
    UINT32_C(0xbe5466cf), UINT32_C(0x38d01377), UINT32_C(0x452821e6),
    UINT32_C(0xec4e6c89), UINT32_C(0x082efa98), UINT32_C(0x299f31d0),
    UINT32_C(0xa4093822), UINT32_C(0x03707344), UINT32_C(0x13198a2e),
    UINT32_C(0x85a308d3), UINT32_C(0x00000088),
};

static uint64_t random_state = UINT64_C(0x510e527fade682d1);

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
random_words(uint32_t words[INPUT_WORDS], int force_top_bit)
{
    for (size_t i = 0; i < INPUT_WORDS; i += 2) {
        const uint64_t value = splitmix64_next();
        words[i] = (uint32_t)value;
        if (i + 1 < INPUT_WORDS)
            words[i + 1] = (uint32_t)(value >> 32);
    }
    words[INPUT_WORDS - 1] &= UINT32_C(0x1ff);
    if (force_top_bit)
        words[INPUT_WORDS - 1] |= UINT32_C(0x100);
}

static int
test_multiplications(void)
{
    for (unsigned test = 0; test < MULTIPLICATION_CASES; ++test) {
        uint32_t modulus_words[INPUT_WORDS];
        uint32_t a_words[INPUT_WORDS];
        uint32_t b_words[INPUT_WORDS];
        random_words(modulus_words, 1);
        random_words(a_words, 0);
        random_words(b_words, 0);

        ibz_t modulus, a, b, reduced_a, reduced_b, wide, expected, observed;
        ibz_copy_u32_digits(&modulus, modulus_words, INPUT_WORDS);
        ibz_copy_u32_digits(&a, a_words, INPUT_WORDS);
        ibz_copy_u32_digits(&b, b_words, INPUT_WORDS);
        ibz_mod(&reduced_a, &a, &modulus);
        ibz_mod(&reduced_b, &b, &modulus);
        ibz_mul(&wide, &reduced_a, &reduced_b);
        ibz_mod(&expected, &wide, &modulus);

        sqisign_sca_fixedwork_profile_reset();
        if (!sqisign_sca_mul_mod_fixedwork_521(
                &observed, &reduced_a, &reduced_b, &modulus))
            return 0;
        const sqisign_sca_fixedwork_profile_t profile =
            sqisign_sca_fixedwork_profile_read();
        if (ibz_cmp(&observed, &expected) != 0 ||
            profile.modular_multiplications != 1 ||
            profile.multiplier_iterations !=
                SQISIGN_SCA_FIXEDWORK_MUL_ITERATIONS ||
            profile.modular_additions !=
                2 * SQISIGN_SCA_FIXEDWORK_MUL_ITERATIONS)
            return 0;
    }
    return 1;
}

static int
run_pow_case(const ibz_t *base,
             const ibz_t *exponent,
             const ibz_t *modulus)
{
    ibz_t expected, observed;
    ibz_pow_mod(&expected, base, exponent, modulus);
    sqisign_sca_fixedwork_profile_reset();
    if (!sqisign_sca_powmod_fixedwork_521(
            &observed, base, exponent, modulus))
        return 0;
    const sqisign_sca_fixedwork_profile_t profile =
        sqisign_sca_fixedwork_profile_read();
    return ibz_cmp(&observed, &expected) == 0 &&
           profile.exponent_iterations == SQISIGN_SCA_FIXEDWORK_BITS &&
           profile.modular_multiplications ==
               SQISIGN_SCA_FIXEDWORK_POW_MULTIPLICATIONS &&
           profile.multiplier_iterations ==
               SQISIGN_SCA_FIXEDWORK_POW_MUL_ITERATIONS &&
           profile.modular_additions ==
               SQISIGN_SCA_FIXEDWORK_POW_MODULAR_ADDITIONS;
}

static int
test_powers(void)
{
    ibz_t modulus, base, exponent;
    ibz_copy_u32_digits(&modulus, mersenne_modulus, INPUT_WORDS);
    ibz_copy_u32_digits(&base, fixed_base, INPUT_WORDS);

    uint32_t exponent_words[INPUT_WORDS] = { 0 };
    ibz_init(&exponent);
    if (!run_pow_case(&base, &exponent, &modulus))
        return 0;
    exponent_words[0] = 1;
    ibz_copy_u32_digits(&exponent, exponent_words, INPUT_WORDS);
    if (!run_pow_case(&base, &exponent, &modulus))
        return 0;
    memset(exponent_words, 0, sizeof(exponent_words));
    exponent_words[16] = UINT32_C(0x100);
    ibz_copy_u32_digits(&exponent, exponent_words, INPUT_WORDS);
    if (!run_pow_case(&base, &exponent, &modulus))
        return 0;
    for (size_t word = 0; word + 1 < INPUT_WORDS; ++word)
        exponent_words[word] = UINT32_MAX;
    exponent_words[16] = UINT32_C(0x1ff);
    ibz_copy_u32_digits(&exponent, exponent_words, INPUT_WORDS);
    if (!run_pow_case(&base, &exponent, &modulus))
        return 0;

    for (unsigned test = 0; test < RANDOM_POW_CASES; ++test) {
        random_words(exponent_words, 0);
        ibz_copy_u32_digits(&exponent, exponent_words, INPUT_WORDS);
        if (!run_pow_case(&base, &exponent, &modulus))
            return 0;
    }
    return 1;
}

static int
test_contract(void)
{
    ibz_t modulus, base, exponent, expected, alias, sentinel, invalid, output;
    ibz_copy_u32_digits(&modulus, mersenne_modulus, INPUT_WORDS);
    ibz_copy_u32_digits(&base, fixed_base, INPUT_WORDS);
    ibz_set_u64(&exponent, UINT64_C(0x123456789abcdef));
    ibz_pow_mod(&expected, &base, &exponent, &modulus);

    ibz_copy(&alias, &base);
    if (!sqisign_sca_powmod_fixedwork_521(
            &alias, &alias, &exponent, &modulus) ||
        ibz_cmp(&alias, &expected) != 0)
        return 0;

    ibz_set(&sentinel, 12345);
    ibz_set(&invalid, -1);
    ibz_copy(&output, &sentinel);
    if (sqisign_sca_powmod_fixedwork_521(
            &output, &base, &invalid, &modulus) ||
        ibz_cmp(&output, &sentinel) != 0)
        return 0;
    ibz_init(&invalid);
    ibz_copy(&output, &sentinel);
    if (sqisign_sca_powmod_fixedwork_521(
            &output, &base, &exponent, &invalid) ||
        ibz_cmp(&output, &sentinel) != 0)
        return 0;
    ibz_copy(&invalid, &modulus);
    ibz_copy(&output, &sentinel);
    if (sqisign_sca_powmod_fixedwork_521(
            &output, &invalid, &exponent, &modulus) ||
        ibz_cmp(&output, &sentinel) != 0)
        return 0;
    invalid[8] = UINT64_C(1) << 9;
    ibz_copy(&output, &sentinel);
    if (sqisign_sca_powmod_fixedwork_521(
            &output, &base, &invalid, &modulus) ||
        ibz_cmp(&output, &sentinel) != 0)
        return 0;
    return 1;
}

int
main(void)
{
    if (!test_multiplications() || !test_powers() || !test_contract())
        return 1;
    printf("powmod-fixedwork-521 PASS mul_cases=%u pow_cases=%u "
           "pow_mul_iterations=%u modular_additions=%u\n",
           MULTIPLICATION_CASES,
           RANDOM_POW_CASES + 4,
           SQISIGN_SCA_FIXEDWORK_POW_MUL_ITERATIONS,
           SQISIGN_SCA_FIXEDWORK_POW_MODULAR_ADDITIONS);
    return 0;
}
