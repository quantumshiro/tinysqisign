// SPDX-License-Identifier: Apache-2.0

#include "powmod_fixedwork_521.h"

#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__) && !defined(__clang__)
#define SQISIGN_SCA_NOINLINE __attribute__((noinline, noclone))
#elif defined(__GNUC__) || defined(__clang__)
#define SQISIGN_SCA_NOINLINE __attribute__((noinline))
#else
#define SQISIGN_SCA_NOINLINE
#endif

typedef uint32_t sca_u521_t[SQISIGN_SCA_FIXEDWORK_WORDS];

typedef struct {
    sca_u521_t modulus;
    sca_u521_t base;
    sca_u521_t result;
    sca_u521_t square;
    sca_u521_t multiplied;
    sca_u521_t multiply_accumulator;
    sca_u521_t multiply_addend;
    sca_u521_t selected_addend;
    sca_u521_t addition_sum;
    sca_u521_t subtraction_difference;
    sca_u521_t one;
} sca_fixedwork_workspace_t;

_Static_assert(IBZ_LIMBS == 27,
               "fixed-work experiment is frozen to the Level-I ibz ABI");
_Static_assert(sizeof(ibz_t) == 216,
               "fixed-work experiment requires the Level-I ibz extent");
_Static_assert(sizeof(sca_u521_t) == 68,
               "521-bit word representation changed");

#if defined(SQISIGN_SCA_FIXEDWORK_PROFILE)
static sqisign_sca_fixedwork_profile_t profile;

void
sqisign_sca_fixedwork_profile_reset(void)
{
    profile.exponent_iterations = 0;
    profile.modular_multiplications = 0;
    profile.multiplier_iterations = 0;
    profile.modular_additions = 0;
}

sqisign_sca_fixedwork_profile_t
sqisign_sca_fixedwork_profile_read(void)
{
    return profile;
}
#endif

static uint32_t
nonzero_u32(uint32_t value)
{
    return (value | (UINT32_C(0) - value)) >> 31;
}

static uint32_t
fits_nonnegative_521(const ibz_t *value)
{
    uint64_t outside = (*value)[8] & ~UINT64_C(0x1ff);
    for (size_t limb = 9; limb < IBZ_LIMBS; ++limb)
        outside |= (*value)[limb];
    return (uint32_t)(((outside | (UINT64_C(0) - outside)) >> 63) ^ 1U);
}

static void
load_u521(sca_u521_t target, const ibz_t *source)
{
    for (size_t word = 0; word < SQISIGN_SCA_FIXEDWORK_WORDS; ++word) {
        const uint64_t limb = (*source)[word / 2];
        target[word] =
            (uint32_t)(limb >> ((unsigned)(word & 1U) * 32U));
    }
}

static void
store_u521(ibz_t *target, const sca_u521_t source)
{
    for (size_t limb = 0; limb < IBZ_LIMBS; ++limb)
        (*target)[limb] = 0;
    for (size_t limb = 0; limb < 8; ++limb) {
        (*target)[limb] = (uint64_t)source[2 * limb] |
                          ((uint64_t)source[2 * limb + 1] << 32);
    }
    (*target)[8] = source[16];
}

static uint32_t
subtract_words(sca_u521_t difference,
               const sca_u521_t a,
               const sca_u521_t b)
{
    uint32_t borrow = 0;
    for (size_t word = 0; word < SQISIGN_SCA_FIXEDWORK_WORDS; ++word) {
        const uint64_t wide =
            (uint64_t)a[word] - (uint64_t)b[word] - borrow;
        difference[word] = (uint32_t)wide;
        borrow = (uint32_t)(wide >> 63);
    }
    return borrow;
}

/* Inputs are in [0,m).  Their sum is below 2*m and therefore one subtraction
 * is sufficient.  For m < 2^521 the sum fits in the fixed 17-word extent. */
SQISIGN_SCA_NOINLINE static void
add_mod(sca_u521_t output,
        const sca_u521_t a,
        const sca_u521_t b,
        const sca_u521_t modulus,
        sca_fixedwork_workspace_t *workspace)
{
    uint32_t carry = 0;
    for (size_t word = 0; word < SQISIGN_SCA_FIXEDWORK_WORDS; ++word) {
        const uint64_t wide = (uint64_t)a[word] + b[word] + carry;
        workspace->addition_sum[word] = (uint32_t)wide;
        carry = (uint32_t)(wide >> 32);
    }

    const uint32_t borrow =
        subtract_words(workspace->subtraction_difference,
                       workspace->addition_sum,
                       modulus);
    const uint32_t use_difference = UINT32_C(0) - (borrow ^ 1U);
    const uint32_t use_sum = ~use_difference;
    for (size_t word = 0; word < SQISIGN_SCA_FIXEDWORK_WORDS; ++word) {
        output[word] =
            (workspace->subtraction_difference[word] & use_difference) |
            (workspace->addition_sum[word] & use_sum);
    }

#if defined(SQISIGN_SCA_FIXEDWORK_PROFILE)
    profile.modular_additions++;
#else
    (void)carry;
#endif
}

static void
reduce_one(sca_u521_t output,
           const sca_u521_t modulus,
           sca_fixedwork_workspace_t *workspace)
{
    for (size_t word = 0; word < SQISIGN_SCA_FIXEDWORK_WORDS; ++word)
        workspace->one[word] = 0;
    workspace->one[0] = 1;

    const uint32_t borrow =
        subtract_words(workspace->subtraction_difference,
                       workspace->one,
                       modulus);
    const uint32_t use_difference = UINT32_C(0) - (borrow ^ 1U);
    const uint32_t use_one = ~use_difference;
    for (size_t word = 0; word < SQISIGN_SCA_FIXEDWORK_WORDS; ++word) {
        output[word] =
            (workspace->subtraction_difference[word] & use_difference) |
            (workspace->one[word] & use_one);
    }
}

SQISIGN_SCA_NOINLINE static void
multiply_mod_words(sca_u521_t output,
                   const sca_u521_t a,
                   const sca_u521_t b,
                   const sca_u521_t modulus,
                   sca_fixedwork_workspace_t *workspace)
{
    for (size_t word = 0; word < SQISIGN_SCA_FIXEDWORK_WORDS; ++word) {
        workspace->multiply_accumulator[word] = 0;
        workspace->multiply_addend[word] = a[word];
    }

    for (unsigned bit = 0; bit < SQISIGN_SCA_FIXEDWORK_BITS; ++bit) {
        const uint32_t multiplier_bit =
            (b[bit / 32U] >> (bit % 32U)) & 1U;
        const uint32_t select_addend = UINT32_C(0) - multiplier_bit;
        for (size_t word = 0; word < SQISIGN_SCA_FIXEDWORK_WORDS; ++word) {
            workspace->selected_addend[word] =
                workspace->multiply_addend[word] & select_addend;
        }

        add_mod(workspace->multiply_accumulator,
                workspace->multiply_accumulator,
                workspace->selected_addend,
                modulus,
                workspace);
        add_mod(workspace->multiply_addend,
                workspace->multiply_addend,
                workspace->multiply_addend,
                modulus,
                workspace);
#if defined(SQISIGN_SCA_FIXEDWORK_PROFILE)
        profile.multiplier_iterations++;
#endif
    }

    for (size_t word = 0; word < SQISIGN_SCA_FIXEDWORK_WORDS; ++word)
        output[word] = workspace->multiply_accumulator[word];
#if defined(SQISIGN_SCA_FIXEDWORK_PROFILE)
    profile.modular_multiplications++;
#endif
}

static uint32_t
words_nonzero(const sca_u521_t value)
{
    uint32_t combined = 0;
    for (size_t word = 0; word < SQISIGN_SCA_FIXEDWORK_WORDS; ++word)
        combined |= value[word];
    return nonzero_u32(combined);
}

static void
wipe_workspace(sca_fixedwork_workspace_t *workspace)
{
    volatile uint32_t *words = (volatile uint32_t *)workspace;
    for (size_t word = 0; word < sizeof(*workspace) / sizeof(*words); ++word)
        words[word] = 0;
}

int
sqisign_sca_mul_mod_fixedwork_521(ibz_t *product,
                                  const ibz_t *a,
                                  const ibz_t *b,
                                  const ibz_t *modulus)
{
    sca_fixedwork_workspace_t workspace;
    load_u521(workspace.base, a);
    load_u521(workspace.result, b);
    load_u521(workspace.modulus, modulus);

    sca_u521_t comparison;
    const uint32_t a_below_modulus =
        subtract_words(comparison, workspace.base, workspace.modulus);
    const uint32_t b_below_modulus =
        subtract_words(comparison, workspace.result, workspace.modulus);
    const uint32_t valid = fits_nonnegative_521(a) &
                           fits_nonnegative_521(b) &
                           fits_nonnegative_521(modulus) &
                           words_nonzero(workspace.modulus) &
                           a_below_modulus & b_below_modulus;
    if (valid == 0) {
        wipe_workspace(&workspace);
        volatile uint32_t *wipe = comparison;
        for (size_t word = 0; word < SQISIGN_SCA_FIXEDWORK_WORDS; ++word)
            wipe[word] = 0;
        return 0;
    }

    multiply_mod_words(workspace.square,
                       workspace.base,
                       workspace.result,
                       workspace.modulus,
                       &workspace);
    store_u521(product, workspace.square);
    wipe_workspace(&workspace);
    volatile uint32_t *wipe = comparison;
    for (size_t word = 0; word < SQISIGN_SCA_FIXEDWORK_WORDS; ++word)
        wipe[word] = 0;
    return 1;
}

int
sqisign_sca_powmod_fixedwork_521(ibz_t *power,
                                 const ibz_t *x,
                                 const ibz_t *e,
                                 const ibz_t *m)
{
    sca_fixedwork_workspace_t workspace;
    load_u521(workspace.base, x);
    load_u521(workspace.modulus, m);
    sca_u521_t exponent;
    load_u521(exponent, e);

    sca_u521_t comparison;
    const uint32_t base_below_modulus =
        subtract_words(comparison, workspace.base, workspace.modulus);
    const uint32_t valid = fits_nonnegative_521(x) &
                           fits_nonnegative_521(e) &
                           fits_nonnegative_521(m) &
                           words_nonzero(workspace.modulus) &
                           base_below_modulus;
    if (valid == 0) {
        wipe_workspace(&workspace);
        volatile uint32_t *wipe_exponent = exponent;
        volatile uint32_t *wipe_comparison = comparison;
        for (size_t word = 0; word < SQISIGN_SCA_FIXEDWORK_WORDS; ++word) {
            wipe_exponent[word] = 0;
            wipe_comparison[word] = 0;
        }
        return 0;
    }

    reduce_one(workspace.result, workspace.modulus, &workspace);
    for (int bit = SQISIGN_SCA_FIXEDWORK_BITS - 1; bit >= 0; --bit) {
        multiply_mod_words(workspace.square,
                           workspace.result,
                           workspace.result,
                           workspace.modulus,
                           &workspace);
        multiply_mod_words(workspace.multiplied,
                           workspace.square,
                           workspace.base,
                           workspace.modulus,
                           &workspace);

        const uint32_t exponent_bit =
            (exponent[(unsigned)bit / 32U] >> ((unsigned)bit % 32U)) & 1U;
        const uint32_t select_multiplied = UINT32_C(0) - exponent_bit;
        const uint32_t select_square = ~select_multiplied;
        for (size_t word = 0; word < SQISIGN_SCA_FIXEDWORK_WORDS; ++word) {
            workspace.result[word] =
                (workspace.square[word] & select_square) |
                (workspace.multiplied[word] & select_multiplied);
        }
#if defined(SQISIGN_SCA_FIXEDWORK_PROFILE)
        profile.exponent_iterations++;
#endif
    }

    store_u521(power, workspace.result);
    wipe_workspace(&workspace);
    volatile uint32_t *wipe_exponent = exponent;
    volatile uint32_t *wipe_comparison = comparison;
    for (size_t word = 0; word < SQISIGN_SCA_FIXEDWORK_WORDS; ++word) {
        wipe_exponent[word] = 0;
        wipe_comparison[word] = 0;
    }
    return 1;
}
