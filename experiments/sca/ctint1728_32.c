#include "ctint1728_32.h"

#if defined(__GNUC__) && !defined(__clang__)
#define CTINT_NOINLINE __attribute__((noinline, noipa))
#elif defined(__GNUC__) || defined(__clang__)
#define CTINT_NOINLINE __attribute__((noinline))
#else
#define CTINT_NOINLINE
#endif


static uint32_t
ct_nonzero_u32(uint32_t value)
{
    return (value | (uint32_t)(0u - value)) >> 31;
}


static uint32_t
ct_select_u32(uint32_t old_value, uint32_t new_value, uint32_t selector)
{
    const uint32_t mask = 0u - (selector & 1u);
    return (old_value & ~mask) | (new_value & mask);
}


static void
ct_compare_word(uint32_t a, uint32_t b, uint32_t *less, uint32_t *greater)
{
    const uint32_t undecided = 1u ^ (*less | *greater);
    const uint32_t word_less = (uint32_t)(a < b);
    const uint32_t word_greater = (uint32_t)(a > b);
    *less |= undecided & word_less;
    *greater |= undecided & word_greater;
}


CTINT_NOINLINE void
ctint1728_32_secure_clear(void *object, size_t bytes)
{
    volatile uint8_t *cursor = (volatile uint8_t *)object;
    for (size_t i = 0; i < bytes; ++i) {
        cursor[i] = 0;
    }
}


CTINT_NOINLINE void
ctint1728_32_copy(ctint1728_32_t *out, const ctint1728_32_t *in)
{
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        out->words[i] = in->words[i];
    }
}


CTINT_NOINLINE void
ctint1728_32_cmov(ctint1728_32_t *out,
                  const ctint1728_32_t *in,
                  uint32_t selector)
{
    const uint32_t mask = 0u - (selector & 1u);
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        out->words[i] = (out->words[i] & ~mask) |
                        (in->words[i] & mask);
    }
}


CTINT_NOINLINE uint32_t
ctint1728_32_is_zero(const ctint1728_32_t *value)
{
    uint32_t aggregate = 0;
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        aggregate |= value->words[i];
    }
    return 1u ^ ct_nonzero_u32(aggregate);
}


CTINT_NOINLINE int32_t
ctint1728_32_sign(const ctint1728_32_t *value)
{
    uint32_t aggregate = 0;
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        aggregate |= value->words[i];
    }
    const uint32_t nonzero = ct_nonzero_u32(aggregate);
    const uint32_t negative = value->words[CTINT1728_WORDS - 1] >> 31;
    const uint32_t positive = nonzero & (1u ^ negative);
    return (int32_t)positive - (int32_t)negative;
}


CTINT_NOINLINE int32_t
ctint1728_32_cmp_unsigned(const ctint1728_32_t *a,
                         const ctint1728_32_t *b)
{
    uint32_t less = 0;
    uint32_t greater = 0;
    for (size_t index = CTINT1728_WORDS; index > 0; --index) {
        const size_t i = index - 1;
        ct_compare_word(a->words[i], b->words[i], &less, &greater);
    }
    return (int32_t)greater - (int32_t)less;
}


CTINT_NOINLINE int32_t
ctint1728_32_cmp_signed(const ctint1728_32_t *a,
                       const ctint1728_32_t *b)
{
    uint32_t less = 0;
    uint32_t greater = 0;
    const size_t top = CTINT1728_WORDS - 1;

    /* Flipping the sign bit maps two's-complement signed order to unsigned
     * lexicographic order without a secret-dependent sign branch. */
    ct_compare_word(a->words[top] ^ UINT32_C(0x80000000),
                    b->words[top] ^ UINT32_C(0x80000000),
                    &less,
                    &greater);
    for (size_t index = top; index > 0; --index) {
        const size_t i = index - 1;
        ct_compare_word(a->words[i], b->words[i], &less, &greater);
    }
    return (int32_t)greater - (int32_t)less;
}


CTINT_NOINLINE void
ctint1728_32_abs(ctint1728_32_t *out, const ctint1728_32_t *in)
{
    const uint32_t negative = in->words[CTINT1728_WORDS - 1] >> 31;
    const uint32_t mask = 0u - negative;
    uint32_t carry = negative;

    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        const uint32_t complemented = in->words[i] ^ mask;
        const uint32_t word = complemented + carry;
        carry = (uint32_t)(word < complemented);
        out->words[i] = word;
    }
}


CTINT_NOINLINE uint32_t
ctint1728_32_bit_length_unsigned(const ctint1728_32_t *value)
{
    uint32_t result = 0;
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        const uint32_t word = value->words[i];
        const uint32_t nonzero = ct_nonzero_u32(word);
#if defined(__GNUC__) || defined(__clang__)
        const uint32_t width = 32u -
            (uint32_t)__builtin_clz(word | UINT32_C(1));
#else
#error "The experimental CT integer slice requires a compiler CLZ builtin"
#endif
        const uint32_t candidate = (uint32_t)(i * 32u) + width;
        result = ct_select_u32(result, candidate, nonzero);
    }
    return result;
}


CTINT_NOINLINE uint32_t
ctint1728_32_abs_bit_length(const ctint1728_32_t *value)
{
    const uint32_t negative = value->words[CTINT1728_WORDS - 1] >> 31;
    const uint32_t mask = 0u - negative;
    uint32_t carry = negative;
    uint32_t result = 0;

    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        const uint32_t complemented = value->words[i] ^ mask;
        const uint32_t word = complemented + carry;
        carry = (uint32_t)(word < complemented);
        const uint32_t nonzero = ct_nonzero_u32(word);
#if defined(__GNUC__) || defined(__clang__)
        const uint32_t width = 32u -
            (uint32_t)__builtin_clz(word | UINT32_C(1));
#else
#error "The experimental CT integer slice requires a compiler CLZ builtin"
#endif
        const uint32_t candidate = (uint32_t)(i * 32u) + width;
        result = ct_select_u32(result, candidate, nonzero);
    }
    return result;
}


CTINT_NOINLINE int
ctint1728_32_add(ctint1728_32_t *out,
                 const ctint1728_32_t *a,
                 const ctint1728_32_t *b,
                 ctint1728_32_arith_workspace_t *workspace)
{
    ctint1728_32_copy(&workspace->saved_output, out);
    uint64_t carry = 0;
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        const uint64_t sum = (uint64_t)a->words[i] + b->words[i] + carry;
        workspace->result.words[i] = (uint32_t)sum;
        carry = sum >> 32;
    }

    const size_t top = CTINT1728_WORDS - 1;
    const uint32_t sign_a = a->words[top] >> 31;
    const uint32_t sign_b = b->words[top] >> 31;
    const uint32_t sign_result = workspace->result.words[top] >> 31;
    const uint32_t overflow =
        (1u ^ (sign_a ^ sign_b)) & (sign_a ^ sign_result);
    const uint32_t valid = 1u ^ overflow;
    ctint1728_32_copy(out, &workspace->saved_output);
    ctint1728_32_cmov(out, &workspace->result, valid);
    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    return (int)valid;
}


CTINT_NOINLINE int
ctint1728_32_sub(ctint1728_32_t *out,
                 const ctint1728_32_t *a,
                 const ctint1728_32_t *b,
                 ctint1728_32_arith_workspace_t *workspace)
{
    ctint1728_32_copy(&workspace->saved_output, out);
    uint64_t borrow = 0;
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        const uint64_t subtrahend = (uint64_t)b->words[i] + borrow;
        workspace->result.words[i] =
            (uint32_t)((uint64_t)a->words[i] - subtrahend);
        borrow = (uint64_t)a->words[i] < subtrahend;
    }

    const size_t top = CTINT1728_WORDS - 1;
    const uint32_t sign_a = a->words[top] >> 31;
    const uint32_t sign_b = b->words[top] >> 31;
    const uint32_t sign_result = workspace->result.words[top] >> 31;
    const uint32_t overflow =
        (sign_a ^ sign_b) & (sign_a ^ sign_result);
    const uint32_t valid = 1u ^ overflow;
    ctint1728_32_copy(out, &workspace->saved_output);
    ctint1728_32_cmov(out, &workspace->result, valid);
    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    return (int)valid;
}


CTINT_NOINLINE int
ctint1728_32_mul(ctint1728_32_t *out,
                 const ctint1728_32_t *a,
                 const ctint1728_32_t *b,
                 ctint1728_32_mul_workspace_t *workspace)
{
    const size_t top = CTINT1728_WORDS - 1;
    const uint32_t sign_a = a->words[top] >> 31;
    const uint32_t sign_b = b->words[top] >> 31;
    const uint32_t negative = sign_a ^ sign_b;
    ctint1728_32_copy(&workspace->saved_output, out);
    ctint1728_32_abs(&workspace->a_magnitude, a);
    ctint1728_32_abs(&workspace->b_magnitude, b);

    for (size_t i = 0; i < 2u * CTINT1728_WORDS; ++i) {
        workspace->wide_product[i] = 0;
    }
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        uint64_t carry = 0;
        for (size_t j = 0; j < CTINT1728_WORDS; ++j) {
            const size_t product_index = i + j;
            const uint64_t product =
                (uint64_t)workspace->a_magnitude.words[i] *
                workspace->b_magnitude.words[j];
            const uint64_t accumulated =
                product + workspace->wide_product[product_index] + carry;
            workspace->wide_product[product_index] = (uint32_t)accumulated;
            carry = accumulated >> 32;
        }
        workspace->wide_product[i + CTINT1728_WORDS] = (uint32_t)carry;
    }

    uint32_t high_aggregate = 0;
    for (size_t i = CTINT1728_WORDS;
         i < 2u * CTINT1728_WORDS;
         ++i) {
        high_aggregate |= workspace->wide_product[i];
    }
    uint32_t below_top_aggregate = 0;
    for (size_t i = 0; i < top; ++i) {
        below_top_aggregate |= workspace->wide_product[i];
    }
    const uint32_t high_zero = 1u ^ ct_nonzero_u32(high_aggregate);
    const uint32_t magnitude_top = workspace->wide_product[top];
    const uint32_t top_above_minimum =
        (uint32_t)(magnitude_top > UINT32_C(0x80000000));
    const uint32_t top_is_minimum =
        1u ^ ct_nonzero_u32(magnitude_top ^ UINT32_C(0x80000000));
    const uint32_t below_top_nonzero = ct_nonzero_u32(below_top_aggregate);
    const uint32_t positive_valid =
        high_zero & (1u ^ (magnitude_top >> 31));
    const uint32_t negative_valid =
        high_zero & (1u ^ top_above_minimum) &
        (1u ^ (top_is_minimum & below_top_nonzero));
    const uint32_t valid = ct_select_u32(positive_valid,
                                         negative_valid,
                                         negative);

    const uint32_t sign_mask = 0u - negative;
    uint32_t sign_carry = negative;
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        const uint32_t magnitude = workspace->wide_product[i];
        const uint32_t complemented = magnitude ^ sign_mask;
        const uint32_t word = complemented + sign_carry;
        sign_carry = (uint32_t)(word < complemented);
        workspace->a_magnitude.words[i] = word;
    }
    ctint1728_32_copy(out, &workspace->saved_output);
    ctint1728_32_cmov(out, &workspace->a_magnitude, valid);
    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    return (int)valid;
}


CTINT_NOINLINE int
ctint1728_32_square(ctint1728_32_t *out,
                    const ctint1728_32_t *a,
                    ctint1728_32_mul_workspace_t *workspace)
{
    return ctint1728_32_mul(out, a, a, workspace);
}


CTINT_NOINLINE int
ctint1728_32_div_trunc(ctint1728_32_t *quotient,
                       ctint1728_32_t *remainder,
                       const ctint1728_32_t *a,
                       const ctint1728_32_t *b,
                       ctint1728_32_div_workspace_t *workspace)
{
    const size_t top = CTINT1728_WORDS - 1;
    const uint32_t sign_a = a->words[top] >> 31;
    const uint32_t sign_b = b->words[top] >> 31;
    const uint32_t quotient_negative = sign_a ^ sign_b;
    ctint1728_32_copy(&workspace->saved_quotient, quotient);
    ctint1728_32_copy(&workspace->saved_remainder, remainder);
    ctint1728_32_abs(&workspace->dividend_magnitude, a);
    ctint1728_32_abs(&workspace->divisor_magnitude, b);

    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        workspace->quotient.words[i] = 0;
        workspace->remainder.words[i] = 0;
    }

    /* Restoring binary long division.  The loop index, and therefore every
     * input/quotient address, follows the complete public 1728-bit schedule.
     * The compare/subtract decision changes only register and stored data. */
    for (uint32_t remaining_bits = CTINT1728_BITS;
         remaining_bits > 0;
         --remaining_bits) {
        const uint32_t bit_index = remaining_bits - 1u;
        const size_t word_index = bit_index >> 5;
        const uint32_t word_bit = bit_index & 31u;
        uint32_t carry =
            (workspace->dividend_magnitude.words[word_index] >> word_bit) & 1u;
        for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
            const uint32_t word = workspace->remainder.words[i];
            const uint32_t next_carry = word >> 31;
            workspace->remainder.words[i] = (word << 1) | carry;
            carry = next_carry;
        }

        uint64_t borrow = 0;
        for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
            const uint64_t subtrahend =
                (uint64_t)workspace->divisor_magnitude.words[i] + borrow;
            workspace->subtract_candidate.words[i] =
                (uint32_t)((uint64_t)workspace->remainder.words[i] -
                           subtrahend);
            borrow = (uint64_t)workspace->remainder.words[i] < subtrahend;
        }
        const uint32_t subtract = 1u ^ (uint32_t)borrow;
        ctint1728_32_cmov(&workspace->remainder,
                          &workspace->subtract_candidate,
                          subtract);
        workspace->quotient.words[word_index] |= subtract << word_bit;
    }

    const uint32_t divisor_nonzero =
        1u ^ ctint1728_32_is_zero(&workspace->divisor_magnitude);
    const uint32_t positive_quotient_fits =
        1u ^ (workspace->quotient.words[top] >> 31);
    const uint32_t valid =
        divisor_nonzero & (quotient_negative | positive_quotient_fits);

    const uint32_t quotient_mask = 0u - quotient_negative;
    const uint32_t remainder_mask = 0u - sign_a;
    uint32_t quotient_carry = quotient_negative;
    uint32_t remainder_carry = sign_a;
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        const uint32_t quotient_complemented =
            workspace->quotient.words[i] ^ quotient_mask;
        const uint32_t remainder_complemented =
            workspace->remainder.words[i] ^ remainder_mask;
        const uint32_t quotient_word =
            quotient_complemented + quotient_carry;
        const uint32_t remainder_word =
            remainder_complemented + remainder_carry;
        quotient_carry =
            (uint32_t)(quotient_word < quotient_complemented);
        remainder_carry =
            (uint32_t)(remainder_word < remainder_complemented);
        workspace->quotient.words[i] = quotient_word;
        workspace->remainder.words[i] = remainder_word;
    }

    ctint1728_32_copy(quotient, &workspace->saved_quotient);
    ctint1728_32_copy(remainder, &workspace->saved_remainder);
    ctint1728_32_cmov(quotient, &workspace->quotient, valid);
    ctint1728_32_cmov(remainder, &workspace->remainder, valid);
    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    return (int)valid;
}


CTINT_NOINLINE int
ctint1728_32_mod(ctint1728_32_t *out,
                 const ctint1728_32_t *a,
                 const ctint1728_32_t *modulus,
                 ctint1728_32_mod_workspace_t *workspace)
{
    const size_t top = CTINT1728_WORDS - 1;
    ctint1728_32_copy(&workspace->saved_output, out);
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        workspace->quotient_or_adjusted.words[i] = 0;
        workspace->remainder.words[i] = 0;
    }

    const uint32_t division_valid = (uint32_t)ctint1728_32_div_trunc(
        &workspace->quotient_or_adjusted,
        &workspace->remainder,
        a,
        modulus,
        &workspace->phase.division);

    /* div_trunc cannot represent INT_MIN/-1's positive quotient, but its
     * modular remainder is exactly zero and is representable.  Identify that
     * one case without a secret branch so modular reduction remains complete. */
    uint32_t int_min_delta =
        a->words[top] ^ UINT32_C(0x80000000);
    uint32_t minus_one_delta =
        modulus->words[top] ^ UINT32_MAX;
    for (size_t i = 0; i < top; ++i) {
        int_min_delta |= a->words[i];
        minus_one_delta |= modulus->words[i] ^ UINT32_MAX;
    }
    const uint32_t quotient_overflow_case =
        (1u ^ ct_nonzero_u32(int_min_delta)) &
        (1u ^ ct_nonzero_u32(minus_one_delta));
    uint32_t valid = division_valid | quotient_overflow_case;

    /* Truncating division leaves the remainder with the dividend's sign.
     * For unlike signs and a nonzero remainder, adding the modulus produces
     * the floor-style remainder used by ibz_mod.  The add runs in all cases;
     * only its branch-free selection is conditional. */
    const uint32_t remainder_nonzero =
        1u ^ ctint1728_32_is_zero(&workspace->remainder);
    const uint32_t sign_mismatch =
        (a->words[top] ^ modulus->words[top]) >> 31;
    const uint32_t adjust = remainder_nonzero & sign_mismatch;
    const uint32_t adjustment_valid = (uint32_t)ctint1728_32_add(
        &workspace->quotient_or_adjusted,
        &workspace->remainder,
        modulus,
        &workspace->phase.arithmetic);
    valid &= (1u ^ adjust) | adjustment_valid;
    ctint1728_32_cmov(&workspace->remainder,
                      &workspace->quotient_or_adjusted,
                      adjust);

    ctint1728_32_copy(out, &workspace->saved_output);
    ctint1728_32_cmov(out, &workspace->remainder, valid);
    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    return (int)valid;
}


CTINT_NOINLINE int
ctint1728_32_pow_mod_521(ctint1728_32_t *out,
                         const ctint1728_32_t *base,
                         const ctint1728_32_t *exponent,
                         const ctint1728_32_t *modulus,
                         ctint1728_32_pow_workspace_t *workspace)
{
    ctint1728_32_copy(&workspace->saved_output, out);
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        workspace->base_residue.words[i] = 0;
        workspace->accumulator.words[i] = 0;
        workspace->square.words[i] = 0;
        workspace->multiplied.words[i] = 0;
    }
    workspace->accumulator.words[0] = 1;

    const int32_t exponent_sign = ctint1728_32_sign(exponent);
    const int32_t modulus_sign = ctint1728_32_sign(modulus);
    const uint32_t exponent_bits =
        ctint1728_32_bit_length_unsigned(exponent);
    const uint32_t modulus_bits =
        ctint1728_32_bit_length_unsigned(modulus);
    uint32_t valid =
        (uint32_t)(exponent_sign >= 0) &
        (uint32_t)(exponent_bits <= CTINT1728_POW_BITS) &
        (uint32_t)(modulus_sign > 0) &
        (uint32_t)(modulus_bits <= CTINT1728_POW_MAX_MODULUS_BITS);

    valid &= (uint32_t)ctint1728_32_mod(
        &workspace->base_residue,
        base,
        modulus,
        &workspace->phase.reduction);
    valid &= (uint32_t)ctint1728_32_mod(
        &workspace->accumulator,
        &workspace->accumulator,
        modulus,
        &workspace->phase.reduction);

    /* Always square and multiply, then select with one exponent bit.  The
     * address of that bit follows the complete public 520..0 schedule. */
    for (uint32_t remaining_bits = CTINT1728_POW_BITS;
         remaining_bits > 0;
         --remaining_bits) {
        const uint32_t bit_index = remaining_bits - 1u;
        const size_t word_index = bit_index >> 5;
        const uint32_t word_bit = bit_index & 31u;
        const uint32_t exponent_bit =
            (exponent->words[word_index] >> word_bit) & 1u;

        valid &= (uint32_t)ctint1728_32_mul(
            &workspace->square,
            &workspace->accumulator,
            &workspace->accumulator,
            &workspace->phase.multiplication);
        valid &= (uint32_t)ctint1728_32_mod(
            &workspace->square,
            &workspace->square,
            modulus,
            &workspace->phase.reduction);
        valid &= (uint32_t)ctint1728_32_mul(
            &workspace->multiplied,
            &workspace->square,
            &workspace->base_residue,
            &workspace->phase.multiplication);
        valid &= (uint32_t)ctint1728_32_mod(
            &workspace->multiplied,
            &workspace->multiplied,
            modulus,
            &workspace->phase.reduction);
        ctint1728_32_copy(&workspace->accumulator, &workspace->square);
        ctint1728_32_cmov(&workspace->accumulator,
                          &workspace->multiplied,
                          exponent_bit);
    }

    ctint1728_32_copy(out, &workspace->saved_output);
    ctint1728_32_cmov(out, &workspace->accumulator, valid);
    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    return (int)valid;
}


CTINT_NOINLINE int
ctint1728_32_sqrt_exact(ctint1728_32_t *out,
                        const ctint1728_32_t *in,
                        ctint1728_32_sqrt_workspace_t *workspace)
{
    ctint1728_32_copy(&workspace->saved_output, out);
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        workspace->remainder.words[i] = 0;
        workspace->root.words[i] = 0;
        workspace->subtract_candidate.words[i] = 0;
    }
    uint32_t valid = (uint32_t)(ctint1728_32_sign(in) >= 0);

    /* Restoring radix-4 square root.  The input address follows the complete
     * public pair schedule 863..0; every pair executes the same word scan,
     * subtract, cmov and root-bit publication. */
    for (uint32_t remaining_pairs = CTINT1728_SQRT_PAIRS;
         remaining_pairs > 0;
         --remaining_pairs) {
        const uint32_t pair_index = remaining_pairs - 1u;
        const uint32_t bit_index = 2u * pair_index;
        const size_t word_index = bit_index >> 5;
        const uint32_t word_bit = bit_index & 31u;
        uint32_t remainder_carry =
            (in->words[word_index] >> word_bit) & 3u;
        uint32_t root_carry = 0;
        uint32_t trial_carry = 1;
        uint32_t borrow = 0;

        for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
            const uint32_t old_remainder = workspace->remainder.words[i];
            const uint32_t old_root = workspace->root.words[i];
            const uint32_t shifted_remainder =
                (old_remainder << 2) | remainder_carry;
            const uint32_t shifted_root = (old_root << 1) | root_carry;
            const uint32_t trial = (shifted_root << 1) | trial_carry;
            const uint64_t subtrahend = (uint64_t)trial + borrow;
            workspace->remainder.words[i] = shifted_remainder;
            workspace->root.words[i] = shifted_root;
            workspace->subtract_candidate.words[i] =
                (uint32_t)((uint64_t)shifted_remainder - subtrahend);
            remainder_carry = old_remainder >> 30;
            root_carry = old_root >> 31;
            trial_carry = shifted_root >> 31;
            borrow = (uint64_t)shifted_remainder < subtrahend;
        }

        const uint32_t subtract = 1u ^ borrow;
        ctint1728_32_cmov(&workspace->remainder,
                          &workspace->subtract_candidate,
                          subtract);
        workspace->root.words[0] |= subtract;
    }

    valid &= ctint1728_32_is_zero(&workspace->remainder);
    ctint1728_32_copy(out, &workspace->saved_output);
    ctint1728_32_cmov(out, &workspace->root, valid);
    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    return (int)valid;
}


CTINT_NOINLINE int
ctint1728_32_sqrt_floor(ctint1728_32_t *out,
                        const ctint1728_32_t *in,
                        ctint1728_32_sqrt_workspace_t *workspace)
{
    ctint1728_32_copy(&workspace->saved_output, out);
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        workspace->remainder.words[i] = 0;
        workspace->root.words[i] = 0;
        workspace->subtract_candidate.words[i] = 0;
    }
    const uint32_t valid = (uint32_t)(ctint1728_32_sign(in) >= 0);

    /* This deliberately mirrors sqrt_exact's complete public pair schedule.
     * The only difference is publication: every nonnegative input publishes
     * the computed floor root, irrespective of the final remainder. */
    for (uint32_t remaining_pairs = CTINT1728_SQRT_PAIRS;
         remaining_pairs > 0;
         --remaining_pairs) {
        const uint32_t pair_index = remaining_pairs - 1u;
        const uint32_t bit_index = 2u * pair_index;
        const size_t word_index = bit_index >> 5;
        const uint32_t word_bit = bit_index & 31u;
        uint32_t remainder_carry =
            (in->words[word_index] >> word_bit) & 3u;
        uint32_t root_carry = 0;
        uint32_t trial_carry = 1;
        uint32_t borrow = 0;

        for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
            const uint32_t old_remainder = workspace->remainder.words[i];
            const uint32_t old_root = workspace->root.words[i];
            const uint32_t shifted_remainder =
                (old_remainder << 2) | remainder_carry;
            const uint32_t shifted_root = (old_root << 1) | root_carry;
            const uint32_t trial = (shifted_root << 1) | trial_carry;
            const uint64_t subtrahend = (uint64_t)trial + borrow;
            workspace->remainder.words[i] = shifted_remainder;
            workspace->root.words[i] = shifted_root;
            workspace->subtract_candidate.words[i] =
                (uint32_t)((uint64_t)shifted_remainder - subtrahend);
            remainder_carry = old_remainder >> 30;
            root_carry = old_root >> 31;
            trial_carry = shifted_root >> 31;
            borrow = (uint64_t)shifted_remainder < subtrahend;
        }

        const uint32_t subtract = 1u ^ borrow;
        ctint1728_32_cmov(&workspace->remainder,
                          &workspace->subtract_candidate,
                          subtract);
        workspace->root.words[0] |= subtract;
    }

    ctint1728_32_copy(out, &workspace->saved_output);
    ctint1728_32_cmov(out, &workspace->root, valid);
    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    return (int)valid;
}


CTINT_NOINLINE int
ctint1728_32_cornacchia_halfgcd_492(
    ctint1728_32_t *out,
    const ctint1728_32_t *modulus,
    const ctint1728_32_t *root,
    ctint1728_32_halfgcd_workspace_t *workspace)
{
    /* Save before publishing and sanitize invalid operands to one fixed safe
     * pair.  Invalid calls still execute the complete sqrt and half-GCD
     * schedules, but cannot trigger an internal shift or subtraction bound. */
    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    ctint1728_32_copy(&workspace->saved_output, out);
    workspace->one.words[0] = 1u;
    workspace->a.words[0] = 3u;
    workspace->b.words[0] = 1u;

    const int32_t modulus_sign = ctint1728_32_sign(modulus);
    const int32_t root_sign = ctint1728_32_sign(root);
    const uint32_t modulus_bits =
        ctint1728_32_bit_length_unsigned(modulus);
    const int32_t root_vs_modulus =
        ctint1728_32_cmp_unsigned(root, modulus);
    uint32_t valid =
        (uint32_t)(modulus_sign > 0) &
        (uint32_t)(modulus_bits <= CTINT1728_CORNACCHIA_MAX_BITS) &
        (uint32_t)(root_sign >= 0) &
        (uint32_t)(root_vs_modulus < 0);
    ctint1728_32_cmov(&workspace->a, modulus, valid);
    ctint1728_32_cmov(&workspace->b, root, valid);

    /* threshold=ceil(sqrt(modulus)).  This also handles square diagnostic
     * inputs exactly like the legacy test r*r >= modulus, although production
     * Cornacchia reaches this routine only with a prime modulus. */
    valid &= (uint32_t)ctint1728_32_sqrt_floor(
        &workspace->limit,
        &workspace->a,
        &workspace->phase.square_root);
    valid &= (uint32_t)ctint1728_32_square(
        &workspace->limit_squared,
        &workspace->limit,
        &workspace->phase.multiplication);
    const uint32_t limit_is_exact = (uint32_t)(
        ctint1728_32_cmp_unsigned(&workspace->limit_squared,
                                  &workspace->a) == 0);
    valid &= (uint32_t)ctint1728_32_add(
        &workspace->threshold,
        &workspace->limit,
        &workspace->one,
        &workspace->phase.arithmetic);
    ctint1728_32_cmov(&workspace->threshold,
                      &workspace->limit,
                      limit_is_exact);

    /* Shift/subtract Euclid following Algorithm 8 of ePrint 2023/807.  The
     * full-input Bernstein--Yang bound is conservative for half-GCD.  Once
     * min(a,b)^2 < modulus, active becomes zero and both states freeze. */
    for (uint32_t remaining = CTINT1728_CORNACCHIA_HALFGCD_ROUNDS;
         remaining > 0;
         --remaining) {
        const int32_t comparison =
            ctint1728_32_cmp_unsigned(&workspace->a, &workspace->b);
        const uint32_t b_is_greater = (uint32_t)(comparison < 0);
        ctint1728_32_copy(&workspace->maximum, &workspace->a);
        ctint1728_32_copy(&workspace->minimum, &workspace->b);
        ctint1728_32_cmov(&workspace->maximum,
                          &workspace->b,
                          b_is_greater);
        ctint1728_32_cmov(&workspace->minimum,
                          &workspace->a,
                          b_is_greater);

        const uint32_t maximum_bits =
            ctint1728_32_bit_length_unsigned(&workspace->maximum);
        const uint32_t minimum_bits =
            ctint1728_32_bit_length_unsigned(&workspace->minimum);
        const uint32_t difference = maximum_bits - minimum_bits;
        const uint32_t difference_nonzero = ct_nonzero_u32(difference);
        const uint32_t alpha =
            (difference - 1u) & (0u - difference_nonzero);
        valid &= (uint32_t)ctint1728_32_lshift(
            &workspace->shifted,
            &workspace->minimum,
            alpha,
            &workspace->phase.shift);
        valid &= (uint32_t)ctint1728_32_sub(
            &workspace->next,
            &workspace->maximum,
            &workspace->shifted,
            &workspace->phase.arithmetic);

        const uint32_t active = (uint32_t)(
            ctint1728_32_cmp_unsigned(&workspace->minimum,
                                      &workspace->threshold) >= 0);
        ctint1728_32_cmov(&workspace->a, &workspace->next, active);
        ctint1728_32_cmov(&workspace->b, &workspace->minimum, active);
    }

    const int32_t final_comparison =
        ctint1728_32_cmp_unsigned(&workspace->a, &workspace->b);
    const uint32_t final_b_is_greater =
        (uint32_t)(final_comparison < 0);
    ctint1728_32_copy(&workspace->minimum, &workspace->b);
    ctint1728_32_cmov(&workspace->minimum,
                      &workspace->a,
                      final_b_is_greater);
    const uint32_t finished = (uint32_t)(
        ctint1728_32_cmp_unsigned(&workspace->minimum,
                                  &workspace->threshold) < 0);
    valid &= finished;

    ctint1728_32_copy(out, &workspace->saved_output);
    ctint1728_32_cmov(out, &workspace->minimum, valid);
    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    return (int)valid;
}


static CTINT_NOINLINE void
shift_left_small(ctint1728_32_t *out,
                 const ctint1728_32_t *in,
                 uint32_t bits)
{
    out->words[0] = in->words[0] << bits;
    for (size_t i = 1; i < CTINT1728_WORDS; ++i) {
        out->words[i] = (in->words[i] << bits) |
                        (in->words[i - 1] >> (32u - bits));
    }
}


static CTINT_NOINLINE void
shift_right_small(ctint1728_32_t *out,
                  const ctint1728_32_t *in,
                  uint32_t bits)
{
    const size_t top = CTINT1728_WORDS - 1;
    for (size_t i = 0; i < top; ++i) {
        out->words[i] = (in->words[i] >> bits) |
                        (in->words[i + 1] << (32u - bits));
    }
    out->words[top] = in->words[top] >> bits;
}


static CTINT_NOINLINE void
shift_left_words(ctint1728_32_t *out,
                 const ctint1728_32_t *in,
                 size_t words)
{
    for (size_t i = 0; i < words; ++i) {
        out->words[i] = 0;
    }
    for (size_t i = words; i < CTINT1728_WORDS; ++i) {
        out->words[i] = in->words[i - words];
    }
}


static CTINT_NOINLINE void
shift_right_words(ctint1728_32_t *out,
                  const ctint1728_32_t *in,
                  size_t words)
{
    const size_t remaining = CTINT1728_WORDS - words;
    for (size_t i = 0; i < remaining; ++i) {
        out->words[i] = in->words[i + words];
    }
    for (size_t i = remaining; i < CTINT1728_WORDS; ++i) {
        out->words[i] = 0;
    }
}


static CTINT_NOINLINE int
shift_bits(ctint1728_32_t *out,
           const ctint1728_32_t *in,
           uint32_t shift,
           ctint1728_32_shift_workspace_t *workspace,
           uint32_t right)
{
    ctint1728_32_copy(&workspace->saved_output, out);
    ctint1728_32_copy(&workspace->current, in);

    /* Stages 0..4 shift by 1,2,4,8,16 bits within words.  Stages 5..10
     * shift by 1,2,4,8,16,32 complete words.  Every stage is executed and
     * selected with one shift bit; no shift-derived memory address occurs. */
    for (uint32_t stage = 0; stage < 5; ++stage) {
        const uint32_t bits = UINT32_C(1) << stage;
        if (right != 0) {
            shift_right_small(&workspace->next, &workspace->current, bits);
        } else {
            shift_left_small(&workspace->next, &workspace->current, bits);
        }
        ctint1728_32_cmov(&workspace->current,
                          &workspace->next,
                          (shift >> stage) & 1u);
    }
    for (uint32_t stage = 0; stage < 6; ++stage) {
        const size_t words = (size_t)1u << stage;
        if (right != 0) {
            shift_right_words(&workspace->next,
                              &workspace->current,
                              words);
        } else {
            shift_left_words(&workspace->next,
                             &workspace->current,
                             words);
        }
        ctint1728_32_cmov(&workspace->current,
                          &workspace->next,
                          (shift >> (stage + 5u)) & 1u);
    }

    const uint32_t count_valid = (uint32_t)(shift < CTINT1728_BITS);
    const uint32_t safe_shift = ct_select_u32(0, shift, count_valid);
    const uint32_t room = CTINT1728_BITS - safe_shift;
    const uint32_t no_left_loss =
        (uint32_t)(ctint1728_32_bit_length_unsigned(in) <= room);
    const uint32_t valid = count_valid & (right | no_left_loss);
    ctint1728_32_copy(out, &workspace->saved_output);
    ctint1728_32_cmov(out, &workspace->current, valid);
    ctint1728_32_secure_clear(workspace, sizeof(*workspace));
    return (int)valid;
}


CTINT_NOINLINE int
ctint1728_32_lshift(ctint1728_32_t *out,
                    const ctint1728_32_t *in,
                    uint32_t shift,
                    ctint1728_32_shift_workspace_t *workspace)
{
    return shift_bits(out, in, shift, workspace, 0);
}


CTINT_NOINLINE int
ctint1728_32_rshift(ctint1728_32_t *out,
                    const ctint1728_32_t *in,
                    uint32_t shift,
                    ctint1728_32_shift_workspace_t *workspace)
{
    return shift_bits(out, in, shift, workspace, 1);
}
