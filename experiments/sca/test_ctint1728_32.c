#include "ctint1728_32.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define TEST_CANARY_A UINT64_C(0x4f6b1d92c3a587e0)
#define TEST_CANARY_B UINT64_C(0xa971e36c50df248b)

typedef struct {
    uint64_t before;
    ctint1728_32_t value;
    uint64_t after;
} guarded_value_t;

typedef struct {
    uint64_t before;
    ctint1728_32_shift_workspace_t value;
    uint64_t after;
} guarded_workspace_t;

typedef struct {
    uint64_t before;
    ctint1728_32_arith_workspace_t value;
    uint64_t after;
} guarded_arith_workspace_t;

typedef struct {
    uint64_t before;
    ctint1728_32_mul_workspace_t value;
    uint64_t after;
} guarded_mul_workspace_t;

typedef struct {
    uint64_t before;
    ctint1728_32_div_workspace_t value;
    uint64_t after;
} guarded_div_workspace_t;

typedef struct {
    uint64_t before;
    ctint1728_32_mod_workspace_t value;
    uint64_t after;
} guarded_mod_workspace_t;

typedef struct {
    uint64_t before;
    ctint1728_32_pow_workspace_t value;
    uint64_t after;
} guarded_pow_workspace_t;

typedef struct {
    uint64_t before;
    ctint1728_32_sqrt_workspace_t value;
    uint64_t after;
} guarded_sqrt_workspace_t;

typedef struct {
    uint64_t before;
    ctint1728_32_halfgcd_workspace_t value;
    uint64_t after;
} guarded_halfgcd_workspace_t;


static uint64_t random_state = UINT64_C(0x243f6a8885a308d3);


static void ref_negate(ctint1728_32_t *value);


static void
fail(const char *label, size_t case_id, uint32_t detail)
{
    fprintf(stderr,
            "ctint1728_32 FAIL: %s case=%zu detail=%" PRIu32 "\n",
            label,
            case_id,
            detail);
    exit(1);
}


static uint64_t
next_u64(void)
{
    uint64_t x = random_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    random_state = x;
    return x * UINT64_C(0x2545f4914f6cdd1d);
}


static void
random_value(ctint1728_32_t *value)
{
    for (size_t i = 0; i < CTINT1728_WORDS; i += 2) {
        const uint64_t word = next_u64();
        value->words[i] = (uint32_t)word;
        value->words[i + 1] = (uint32_t)(word >> 32);
    }
}


static void
set_u64(ctint1728_32_t *value, uint64_t word)
{
    memset(value, 0, sizeof(*value));
    value->words[0] = (uint32_t)word;
    value->words[1] = (uint32_t)(word >> 32);
}


static void
set_i64(ctint1728_32_t *value, int64_t word)
{
    const uint64_t magnitude =
        word < 0 ? UINT64_C(0) - (uint64_t)word : (uint64_t)word;
    set_u64(value, magnitude);
    if (word < 0) {
        ref_negate(value);
    }
}


static int
same_value(const ctint1728_32_t *a, const ctint1728_32_t *b)
{
    return memcmp(a, b, sizeof(*a)) == 0;
}


static int
all_zero(const void *object, size_t bytes)
{
    const uint8_t *cursor = (const uint8_t *)object;
    uint8_t aggregate = 0;
    for (size_t i = 0; i < bytes; ++i) {
        aggregate |= cursor[i];
    }
    return aggregate == 0;
}


static void
check_guarded_value(const guarded_value_t *guarded,
                    const char *label,
                    size_t case_id,
                    uint32_t detail)
{
    if (guarded->before != TEST_CANARY_A ||
        guarded->after != TEST_CANARY_B) {
        fail(label, case_id, detail);
    }
}


static void
check_guarded_workspace(const guarded_workspace_t *guarded,
                        const char *label,
                        size_t case_id,
                        uint32_t detail)
{
    if (guarded->before != TEST_CANARY_A ||
        guarded->after != TEST_CANARY_B ||
        !all_zero(&guarded->value, sizeof(guarded->value))) {
        fail(label, case_id, detail);
    }
}


static void
check_guarded_arith_workspace(const guarded_arith_workspace_t *guarded,
                              const char *label,
                              size_t case_id,
                              uint32_t detail)
{
    if (guarded->before != TEST_CANARY_A ||
        guarded->after != TEST_CANARY_B ||
        !all_zero(&guarded->value, sizeof(guarded->value))) {
        fail(label, case_id, detail);
    }
}


static void
check_guarded_mul_workspace(const guarded_mul_workspace_t *guarded,
                            const char *label,
                            size_t case_id,
                            uint32_t detail)
{
    if (guarded->before != TEST_CANARY_A ||
        guarded->after != TEST_CANARY_B ||
        !all_zero(&guarded->value, sizeof(guarded->value))) {
        fail(label, case_id, detail);
    }
}


static void
check_guarded_div_workspace(const guarded_div_workspace_t *guarded,
                            const char *label,
                            size_t case_id,
                            uint32_t detail)
{
    if (guarded->before != TEST_CANARY_A ||
        guarded->after != TEST_CANARY_B ||
        !all_zero(&guarded->value, sizeof(guarded->value))) {
        fail(label, case_id, detail);
    }
}


static void
check_guarded_mod_workspace(const guarded_mod_workspace_t *guarded,
                            const char *label,
                            size_t case_id,
                            uint32_t detail)
{
    if (guarded->before != TEST_CANARY_A ||
        guarded->after != TEST_CANARY_B ||
        !all_zero(&guarded->value, sizeof(guarded->value))) {
        fail(label, case_id, detail);
    }
}


static void
check_guarded_pow_workspace(const guarded_pow_workspace_t *guarded,
                            const char *label,
                            size_t case_id,
                            uint32_t detail)
{
    if (guarded->before != TEST_CANARY_A ||
        guarded->after != TEST_CANARY_B ||
        !all_zero(&guarded->value, sizeof(guarded->value))) {
        fail(label, case_id, detail);
    }
}


static void
check_guarded_sqrt_workspace(const guarded_sqrt_workspace_t *guarded,
                             const char *label,
                             size_t case_id,
                             uint32_t detail)
{
    if (guarded->before != TEST_CANARY_A ||
        guarded->after != TEST_CANARY_B ||
        !all_zero(&guarded->value, sizeof(guarded->value))) {
        fail(label, case_id, detail);
    }
}


static void
check_guarded_halfgcd_workspace(const guarded_halfgcd_workspace_t *guarded,
                                const char *label,
                                size_t case_id,
                                uint32_t detail)
{
    if (guarded->before != TEST_CANARY_A ||
        guarded->after != TEST_CANARY_B ||
        !all_zero(&guarded->value, sizeof(guarded->value))) {
        fail(label, case_id, detail);
    }
}


static int32_t
ref_cmp_unsigned(const ctint1728_32_t *a, const ctint1728_32_t *b)
{
    for (size_t index = CTINT1728_WORDS; index > 0; --index) {
        const size_t i = index - 1;
        if (a->words[i] < b->words[i]) {
            return -1;
        }
        if (a->words[i] > b->words[i]) {
            return 1;
        }
    }
    return 0;
}


static int32_t
ref_sign(const ctint1728_32_t *value)
{
    uint32_t aggregate = 0;
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        aggregate |= value->words[i];
    }
    if (aggregate == 0) {
        return 0;
    }
    return (value->words[CTINT1728_WORDS - 1] >> 31) ? -1 : 1;
}


static int32_t
ref_cmp_signed(const ctint1728_32_t *a, const ctint1728_32_t *b)
{
    const int32_t sign_a = ref_sign(a);
    const int32_t sign_b = ref_sign(b);
    if (sign_a < 0 && sign_b >= 0) {
        return -1;
    }
    if (sign_a >= 0 && sign_b < 0) {
        return 1;
    }
    return ref_cmp_unsigned(a, b);
}


static void
ref_abs(ctint1728_32_t *out, const ctint1728_32_t *in)
{
    if (ref_sign(in) >= 0) {
        memcpy(out, in, sizeof(*out));
        return;
    }
    uint32_t carry = 1;
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        const uint32_t complemented = ~in->words[i];
        out->words[i] = complemented + carry;
        carry = out->words[i] < complemented;
    }
}


static uint32_t
ref_bit_length_unsigned(const ctint1728_32_t *value)
{
    for (size_t index = CTINT1728_WORDS; index > 0; --index) {
        const size_t i = index - 1;
        uint32_t word = value->words[i];
        if (word != 0) {
            uint32_t width = 0;
            while (word != 0) {
                ++width;
                word >>= 1;
            }
            return (uint32_t)(i * 32u) + width;
        }
    }
    return 0;
}


static uint32_t
ref_abs_bit_length(const ctint1728_32_t *value)
{
    ctint1728_32_t magnitude;
    ref_abs(&magnitude, value);
    return ref_bit_length_unsigned(&magnitude);
}


static int
ref_add(ctint1728_32_t *out,
        const ctint1728_32_t *a,
        const ctint1728_32_t *b)
{
    uint64_t carry = 0;
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        const uint64_t sum = (uint64_t)a->words[i] + b->words[i] + carry;
        out->words[i] = (uint32_t)sum;
        carry = sum >> 32;
    }
    const size_t top = CTINT1728_WORDS - 1;
    const uint32_t sign_a = a->words[top] >> 31;
    const uint32_t sign_b = b->words[top] >> 31;
    const uint32_t sign_out = out->words[top] >> 31;
    return !((sign_a == sign_b) && (sign_out != sign_a));
}


static int
ref_sub(ctint1728_32_t *out,
        const ctint1728_32_t *a,
        const ctint1728_32_t *b)
{
    uint64_t borrow = 0;
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        const uint64_t subtrahend = (uint64_t)b->words[i] + borrow;
        out->words[i] = (uint32_t)((uint64_t)a->words[i] - subtrahend);
        borrow = (uint64_t)a->words[i] < subtrahend;
    }
    const size_t top = CTINT1728_WORDS - 1;
    const uint32_t sign_a = a->words[top] >> 31;
    const uint32_t sign_b = b->words[top] >> 31;
    const uint32_t sign_out = out->words[top] >> 31;
    return !((sign_a != sign_b) && (sign_out != sign_a));
}


static void
ref_negate(ctint1728_32_t *value)
{
    uint32_t carry = 1;
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        const uint32_t complemented = ~value->words[i];
        value->words[i] = complemented + carry;
        carry = value->words[i] < complemented;
    }
}


static int
ref_mul(ctint1728_32_t *out,
        const ctint1728_32_t *a,
        const ctint1728_32_t *b)
{
#if !defined(__SIZEOF_INT128__)
#error "The independent multiplication reference requires 128-bit integers"
#endif
    ctint1728_32_t magnitude_a;
    ctint1728_32_t magnitude_b;
    uint64_t limbs_a[CTINT1728_WORDS / 2u];
    uint64_t limbs_b[CTINT1728_WORDS / 2u];
    uint64_t wide[CTINT1728_WORDS] = {0};
    ref_abs(&magnitude_a, a);
    ref_abs(&magnitude_b, b);
    for (size_t i = 0; i < CTINT1728_WORDS / 2u; ++i) {
        limbs_a[i] = (uint64_t)magnitude_a.words[2u * i] |
                     ((uint64_t)magnitude_a.words[2u * i + 1u] << 32);
        limbs_b[i] = (uint64_t)magnitude_b.words[2u * i] |
                     ((uint64_t)magnitude_b.words[2u * i + 1u] << 32);
    }
    for (size_t i = 0; i < CTINT1728_WORDS / 2u; ++i) {
        __uint128_t carry = 0;
        for (size_t j = 0; j < CTINT1728_WORDS / 2u; ++j) {
            const size_t index = i + j;
            const __uint128_t accumulated =
                (__uint128_t)limbs_a[i] * limbs_b[j] +
                wide[index] + carry;
            wide[index] = (uint64_t)accumulated;
            carry = accumulated >> 64;
        }
        wide[i + CTINT1728_WORDS / 2u] = (uint64_t)carry;
    }

    uint64_t high_aggregate = 0;
    for (size_t i = CTINT1728_WORDS / 2u;
         i < CTINT1728_WORDS;
         ++i) {
        high_aggregate |= wide[i];
    }
    uint64_t below_top_aggregate = 0;
    const size_t top = CTINT1728_WORDS / 2u - 1u;
    for (size_t i = 0; i < top; ++i) {
        below_top_aggregate |= wide[i];
    }
    const int negative = (ref_sign(a) < 0) != (ref_sign(b) < 0);
    int valid;
    if (!negative) {
        valid = high_aggregate == 0 && (wide[top] >> 63) == 0;
    } else {
        valid = high_aggregate == 0 &&
                (wide[top] < (UINT64_C(1) << 63) ||
                 (wide[top] == (UINT64_C(1) << 63) &&
                  below_top_aggregate == 0));
    }

    for (size_t i = 0; i < CTINT1728_WORDS / 2u; ++i) {
        out->words[2u * i] = (uint32_t)wide[i];
        out->words[2u * i + 1u] = (uint32_t)(wide[i] >> 32);
    }
    if (negative) {
        ref_negate(out);
    }
    return valid;
}


static int
ref_div_trunc(ctint1728_32_t *quotient,
              ctint1728_32_t *remainder,
              const ctint1728_32_t *a,
              const ctint1728_32_t *b)
{
    if (ref_sign(b) == 0) {
        memset(quotient, 0, sizeof(*quotient));
        memset(remainder, 0, sizeof(*remainder));
        return 0;
    }
    ctint1728_32_t magnitude_a;
    ctint1728_32_t magnitude_b;
    uint64_t dividend[CTINT1728_WORDS / 2u];
    uint64_t divisor[CTINT1728_WORDS / 2u];
    uint64_t q[CTINT1728_WORDS / 2u] = {0};
    uint64_t r[CTINT1728_WORDS / 2u] = {0};
    ref_abs(&magnitude_a, a);
    ref_abs(&magnitude_b, b);
    for (size_t i = 0; i < CTINT1728_WORDS / 2u; ++i) {
        dividend[i] = (uint64_t)magnitude_a.words[2u * i] |
                      ((uint64_t)magnitude_a.words[2u * i + 1u] << 32);
        divisor[i] = (uint64_t)magnitude_b.words[2u * i] |
                     ((uint64_t)magnitude_b.words[2u * i + 1u] << 32);
    }

    for (uint32_t remaining_bits = CTINT1728_BITS;
         remaining_bits > 0;
         --remaining_bits) {
        const uint32_t bit_index = remaining_bits - 1u;
        const size_t word_index = bit_index >> 6;
        const uint32_t word_bit = bit_index & 63u;
        uint64_t carry = (dividend[word_index] >> word_bit) & 1u;
        for (size_t i = 0; i < CTINT1728_WORDS / 2u; ++i) {
            const uint64_t next_carry = r[i] >> 63;
            r[i] = (r[i] << 1) | carry;
            carry = next_carry;
        }

        int greater_or_equal = 1;
        for (size_t index = CTINT1728_WORDS / 2u; index > 0; --index) {
            const size_t i = index - 1u;
            if (r[i] < divisor[i]) {
                greater_or_equal = 0;
                break;
            }
            if (r[i] > divisor[i]) {
                break;
            }
        }
        if (greater_or_equal) {
            uint64_t borrow = 0;
            for (size_t i = 0; i < CTINT1728_WORDS / 2u; ++i) {
                const uint64_t old_word = r[i];
                const __uint128_t subtrahend =
                    (__uint128_t)divisor[i] + borrow;
                r[i] = (uint64_t)((__uint128_t)old_word - subtrahend);
                borrow = (__uint128_t)old_word < subtrahend;
            }
            q[word_index] |= UINT64_C(1) << word_bit;
        }
    }

    for (size_t i = 0; i < CTINT1728_WORDS / 2u; ++i) {
        quotient->words[2u * i] = (uint32_t)q[i];
        quotient->words[2u * i + 1u] = (uint32_t)(q[i] >> 32);
        remainder->words[2u * i] = (uint32_t)r[i];
        remainder->words[2u * i + 1u] = (uint32_t)(r[i] >> 32);
    }
    const int quotient_negative = (ref_sign(a) < 0) != (ref_sign(b) < 0);
    const int valid = quotient_negative ||
                      (quotient->words[CTINT1728_WORDS - 1] >> 31) == 0;
    if (quotient_negative) {
        ref_negate(quotient);
    }
    if (ref_sign(a) < 0) {
        ref_negate(remainder);
    }
    return valid;
}


static int
ref_mod(ctint1728_32_t *out,
        const ctint1728_32_t *a,
        const ctint1728_32_t *modulus)
{
    if (ref_sign(modulus) == 0) {
        memset(out, 0, sizeof(*out));
        return 0;
    }
    ctint1728_32_t quotient;
    ctint1728_32_t remainder;
    ctint1728_32_t adjusted;
    (void)ref_div_trunc(&quotient, &remainder, a, modulus);
    if (ref_sign(&remainder) != 0 &&
        ((ref_sign(a) < 0) != (ref_sign(modulus) < 0))) {
        if (!ref_add(&adjusted, &remainder, modulus)) {
            fail("reference modular adjustment", 0, 0);
        }
        remainder = adjusted;
    }
    *out = remainder;
    return 1;
}


static uint64_t
small_pow_reference(int64_t base,
                    const ctint1728_32_t *exponent,
                    uint64_t modulus)
{
#if !defined(__SIZEOF_INT128__)
#error "The independent pow reference requires 128-bit integers"
#endif
    __int128 signed_residue = (__int128)base % (__int128)modulus;
    if (signed_residue < 0) {
        signed_residue += (__int128)modulus;
    }
    const uint64_t base_residue = (uint64_t)signed_residue;
    uint64_t accumulator = UINT64_C(1) % modulus;
    for (uint32_t remaining_bits = CTINT1728_POW_BITS;
         remaining_bits > 0;
         --remaining_bits) {
        const uint32_t bit_index = remaining_bits - 1u;
        accumulator = (uint64_t)((__uint128_t)accumulator * accumulator %
                                 modulus);
        const uint64_t multiplied =
            (uint64_t)((__uint128_t)accumulator * base_residue % modulus);
        if (((exponent->words[bit_index >> 5] >> (bit_index & 31u)) & 1u) !=
            0) {
            accumulator = multiplied;
        }
    }
    return accumulator;
}


static uint64_t
small_sqrt_floor_reference(uint64_t input)
{
#if !defined(__SIZEOF_INT128__)
#error "The independent sqrt reference requires 128-bit integers"
#endif
    uint64_t lower = 0;
    uint64_t upper = UINT64_C(1) << 32;
    while (lower + 1u < upper) {
        const uint64_t middle = lower + (upper - lower) / 2u;
        if ((__uint128_t)middle * middle <= input) {
            lower = middle;
        } else {
            upper = middle;
        }
    }
    return lower;
}


static void
ref_lshift(ctint1728_32_t *out,
           const ctint1728_32_t *in,
           uint32_t shift)
{
    memset(out, 0, sizeof(*out));
    const size_t word_shift = shift / 32u;
    const uint32_t bit_shift = shift % 32u;
    for (size_t i = word_shift; i < CTINT1728_WORDS; ++i) {
        const size_t source = i - word_shift;
        uint64_t word = (uint64_t)in->words[source] << bit_shift;
        if (bit_shift != 0 && source > 0) {
            word |= (uint64_t)in->words[source - 1] >> (32u - bit_shift);
        }
        out->words[i] = (uint32_t)word;
    }
}


static void
ref_rshift(ctint1728_32_t *out,
           const ctint1728_32_t *in,
           uint32_t shift)
{
    memset(out, 0, sizeof(*out));
    const size_t word_shift = shift / 32u;
    const uint32_t bit_shift = shift % 32u;
    const size_t remaining = CTINT1728_WORDS - word_shift;
    for (size_t i = 0; i < remaining; ++i) {
        const size_t source = i + word_shift;
        uint64_t word = (uint64_t)in->words[source] >> bit_shift;
        if (bit_shift != 0 && source + 1 < CTINT1728_WORDS) {
            word |= (uint64_t)in->words[source + 1] << (32u - bit_shift);
        }
        out->words[i] = (uint32_t)word;
    }
}


static void
initialize_boundaries(ctint1728_32_t values[10])
{
    memset(values, 0, 10 * sizeof(*values));
    values[1].words[0] = 1;
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        values[2].words[i] = UINT32_MAX;
        values[3].words[i] = UINT32_MAX;
        values[5].words[i] = (i & 1u) ? UINT32_C(0xaaaaaaaa) :
                                       UINT32_C(0x55555555);
        values[6].words[i] = (i & 1u) ? UINT32_C(0x55555555) :
                                       UINT32_C(0xaaaaaaaa);
    }
    values[3].words[CTINT1728_WORDS - 1] = UINT32_C(0x7fffffff);
    values[4].words[CTINT1728_WORDS - 1] = UINT32_C(0x80000000);
    values[7].words[CTINT1728_WORDS - 1] = UINT32_C(0x40000000);
    values[8].words[CTINT1728_WORDS - 1] = UINT32_C(0xc0000000);
    values[9].words[26] = UINT32_C(0x80000000);
}


static void
test_scalar_operations(const ctint1728_32_t boundaries[10], uint64_t *digest)
{
    for (size_t a_index = 0; a_index < 10; ++a_index) {
        const ctint1728_32_t *a = &boundaries[a_index];
        if (ctint1728_32_sign(a) != ref_sign(a) ||
            ctint1728_32_is_zero(a) != (uint32_t)(ref_sign(a) == 0) ||
            ctint1728_32_bit_length_unsigned(a) !=
                ref_bit_length_unsigned(a) ||
            ctint1728_32_abs_bit_length(a) != ref_abs_bit_length(a)) {
            fail("boundary scalar", a_index, 0);
        }
        ctint1728_32_t expected_abs;
        ctint1728_32_t actual_abs = *a;
        ref_abs(&expected_abs, a);
        ctint1728_32_abs(&actual_abs, &actual_abs);
        if (!same_value(&expected_abs, &actual_abs)) {
            fail("boundary abs alias", a_index, 0);
        }
        for (size_t b_index = 0; b_index < 10; ++b_index) {
            const ctint1728_32_t *b = &boundaries[b_index];
            if (ctint1728_32_cmp_unsigned(a, b) != ref_cmp_unsigned(a, b) ||
                ctint1728_32_cmp_signed(a, b) != ref_cmp_signed(a, b)) {
                fail("boundary compare", a_index, (uint32_t)b_index);
            }
            ctint1728_32_t selected = *a;
            ctint1728_32_cmov(&selected, b, 0);
            if (!same_value(&selected, a)) {
                fail("cmov zero", a_index, (uint32_t)b_index);
            }
            ctint1728_32_cmov(&selected, b, 1);
            if (!same_value(&selected, b)) {
                fail("cmov one", a_index, (uint32_t)b_index);
            }
        }
        *digest ^= actual_abs.words[a_index % CTINT1728_WORDS];
        *digest *= UINT64_C(0x100000001b3);
    }

    for (size_t case_id = 0; case_id < 10000; ++case_id) {
        ctint1728_32_t a;
        ctint1728_32_t b;
        ctint1728_32_t expected_abs;
        ctint1728_32_t actual_abs;
        random_value(&a);
        random_value(&b);
        ref_abs(&expected_abs, &a);
        ctint1728_32_abs(&actual_abs, &a);
        if (!same_value(&expected_abs, &actual_abs) ||
            ctint1728_32_sign(&a) != ref_sign(&a) ||
            ctint1728_32_is_zero(&a) != (uint32_t)(ref_sign(&a) == 0) ||
            ctint1728_32_bit_length_unsigned(&a) !=
                ref_bit_length_unsigned(&a) ||
            ctint1728_32_abs_bit_length(&a) != ref_abs_bit_length(&a) ||
            ctint1728_32_cmp_unsigned(&a, &b) != ref_cmp_unsigned(&a, &b) ||
            ctint1728_32_cmp_signed(&a, &b) != ref_cmp_signed(&a, &b)) {
            fail("random scalar", case_id, 0);
        }
        *digest ^= actual_abs.words[case_id % CTINT1728_WORDS];
        *digest *= UINT64_C(0x100000001b3);
    }
}


static void
test_add_sub_case(const ctint1728_32_t *a,
                  const ctint1728_32_t *b,
                  size_t case_id,
                  uint64_t *digest)
{
    guarded_value_t output = {TEST_CANARY_A, {{0}}, TEST_CANARY_B};
    guarded_arith_workspace_t workspace = {
        TEST_CANARY_A, {0}, TEST_CANARY_B
    };
    ctint1728_32_t expected;
    ctint1728_32_t expected_add_value;
    ctint1728_32_t expected_sub_value;
    ctint1728_32_t original;

    output.value = *b;
    original = output.value;
    const int expected_add = ref_add(&expected, a, b);
    expected_add_value = expected;
    const int actual_add = ctint1728_32_add(&output.value,
                                            a,
                                            b,
                                            &workspace.value);
    if (actual_add != expected_add ||
        (expected_add && !same_value(&output.value, &expected)) ||
        (!expected_add && !same_value(&output.value, &original))) {
        fail("add", case_id, 0);
    }
    check_guarded_value(&output, "add output canary", case_id, 0);
    check_guarded_arith_workspace(&workspace, "add workspace", case_id, 0);

    output.value = *a;
    original = output.value;
    const int expected_sub = ref_sub(&expected, a, b);
    expected_sub_value = expected;
    const int actual_sub = ctint1728_32_sub(&output.value,
                                            a,
                                            b,
                                            &workspace.value);
    if (actual_sub != expected_sub ||
        (expected_sub && !same_value(&output.value, &expected)) ||
        (!expected_sub && !same_value(&output.value, &original))) {
        fail("sub", case_id, 0);
    }
    check_guarded_value(&output, "sub output canary", case_id, 0);
    check_guarded_arith_workspace(&workspace, "sub workspace", case_id, 0);

    ctint1728_32_t alias = *a;
    if (ctint1728_32_add(&alias, &alias, b, &workspace.value) != expected_add ||
        (expected_add && !same_value(&alias, &expected_add_value)) ||
        (!expected_add && !same_value(&alias, a))) {
        fail("add left alias", case_id, 0);
    }
    check_guarded_arith_workspace(&workspace,
                                  "add left alias workspace",
                                  case_id,
                                  0);

    alias = *b;
    if (ctint1728_32_sub(&alias, a, &alias, &workspace.value) != expected_sub ||
        (expected_sub && !same_value(&alias, &expected_sub_value)) ||
        (!expected_sub && !same_value(&alias, b))) {
        fail("sub right alias", case_id, 0);
    }
    check_guarded_arith_workspace(&workspace,
                                  "sub right alias workspace",
                                  case_id,
                                  0);

    *digest ^= output.value.words[case_id % CTINT1728_WORDS] ^
               ((uint64_t)(uint32_t)expected_add << 32) ^
               (uint32_t)expected_sub;
    *digest *= UINT64_C(0x100000001b3);
}


static void
test_mul_case(const ctint1728_32_t *a,
              const ctint1728_32_t *b,
              size_t case_id,
              int test_alias,
              uint64_t *digest)
{
    guarded_value_t output = {TEST_CANARY_A, {{0}}, TEST_CANARY_B};
    guarded_mul_workspace_t workspace = {
        TEST_CANARY_A, {0}, TEST_CANARY_B
    };
    ctint1728_32_t expected;
    ctint1728_32_t original;

    output.value = *b;
    original = output.value;
    const int expected_mul = ref_mul(&expected, a, b);
    const int actual_mul = ctint1728_32_mul(&output.value,
                                            a,
                                            b,
                                            &workspace.value);
    if (actual_mul != expected_mul ||
        (expected_mul && !same_value(&output.value, &expected)) ||
        (!expected_mul && !same_value(&output.value, &original))) {
        fail("mul", case_id, 0);
    }
    check_guarded_value(&output, "mul output canary", case_id, 0);
    check_guarded_mul_workspace(&workspace, "mul workspace", case_id, 0);

    const int expected_square = ref_mul(&expected, a, a);
    output.value = *b;
    original = output.value;
    const int actual_square = ctint1728_32_square(&output.value,
                                                  a,
                                                  &workspace.value);
    if (actual_square != expected_square ||
        (expected_square && !same_value(&output.value, &expected)) ||
        (!expected_square && !same_value(&output.value, &original))) {
        fail("square", case_id, 0);
    }
    check_guarded_value(&output, "square output canary", case_id, 0);
    check_guarded_mul_workspace(&workspace, "square workspace", case_id, 0);

    if (test_alias) {
        ctint1728_32_t alias = *a;
        ref_mul(&expected, a, b);
        if (ctint1728_32_mul(&alias, &alias, b, &workspace.value) !=
                expected_mul ||
            (expected_mul && !same_value(&alias, &expected)) ||
            (!expected_mul && !same_value(&alias, a))) {
            fail("mul left alias", case_id, 0);
        }
        check_guarded_mul_workspace(&workspace,
                                    "mul left alias workspace",
                                    case_id,
                                    0);

        alias = *b;
        if (ctint1728_32_mul(&alias, a, &alias, &workspace.value) !=
                expected_mul ||
            (expected_mul && !same_value(&alias, &expected)) ||
            (!expected_mul && !same_value(&alias, b))) {
            fail("mul right alias", case_id, 0);
        }
        check_guarded_mul_workspace(&workspace,
                                    "mul right alias workspace",
                                    case_id,
                                    0);
    }

    *digest ^= output.value.words[(case_id + 7u) % CTINT1728_WORDS] ^
               ((uint64_t)(uint32_t)expected_mul << 32) ^
               (uint32_t)expected_square;
    *digest *= UINT64_C(0x100000001b3);
}


static void
random_bounded_value(ctint1728_32_t *value)
{
    memset(value, 0, sizeof(*value));
    for (size_t i = 0; i < 20; i += 2) {
        const uint64_t word = next_u64();
        value->words[i] = (uint32_t)word;
        value->words[i + 1] = (uint32_t)(word >> 32);
    }
    if ((next_u64() & 1u) != 0 && ref_sign(value) != 0) {
        ref_negate(value);
    }
}


static void
test_arithmetic(const ctint1728_32_t boundaries[10], uint64_t *digest)
{
    for (size_t a_index = 0; a_index < 10; ++a_index) {
        for (size_t b_index = 0; b_index < 10; ++b_index) {
            const size_t case_id = a_index * 10u + b_index;
            test_add_sub_case(&boundaries[a_index],
                              &boundaries[b_index],
                              case_id,
                              digest);
            test_mul_case(&boundaries[a_index],
                          &boundaries[b_index],
                          case_id,
                          1,
                          digest);
        }
    }

    for (size_t case_id = 100; case_id < 10100; ++case_id) {
        ctint1728_32_t a;
        ctint1728_32_t b;
        random_value(&a);
        random_value(&b);
        test_add_sub_case(&a, &b, case_id, digest);
    }
    for (size_t case_id = 10100; case_id < 12100; ++case_id) {
        ctint1728_32_t a;
        ctint1728_32_t b;
        random_value(&a);
        random_value(&b);
        test_mul_case(&a, &b, case_id, case_id < 10350, digest);
    }
    for (size_t case_id = 12100; case_id < 14100; ++case_id) {
        ctint1728_32_t a;
        ctint1728_32_t b;
        random_bounded_value(&a);
        random_bounded_value(&b);
        test_mul_case(&a, &b, case_id, case_id < 12350, digest);
    }
}


static void
test_div_case(const ctint1728_32_t *a,
              const ctint1728_32_t *b,
              size_t case_id,
              int test_alias,
              uint64_t *digest)
{
    guarded_value_t quotient = {TEST_CANARY_A, {{0}}, TEST_CANARY_B};
    guarded_value_t remainder = {TEST_CANARY_A, {{0}}, TEST_CANARY_B};
    guarded_div_workspace_t workspace = {
        TEST_CANARY_A, {0}, TEST_CANARY_B
    };
    ctint1728_32_t expected_quotient;
    ctint1728_32_t expected_remainder;
    quotient.value = *b;
    remainder.value = *a;
    const ctint1728_32_t original_quotient = quotient.value;
    const ctint1728_32_t original_remainder = remainder.value;
    const int expected_status = ref_div_trunc(&expected_quotient,
                                              &expected_remainder,
                                              a,
                                              b);
    const int actual_status = ctint1728_32_div_trunc(&quotient.value,
                                                     &remainder.value,
                                                     a,
                                                     b,
                                                     &workspace.value);
    if (actual_status != expected_status ||
        (expected_status &&
         (!same_value(&quotient.value, &expected_quotient) ||
          !same_value(&remainder.value, &expected_remainder))) ||
        (!expected_status &&
         (!same_value(&quotient.value, &original_quotient) ||
          !same_value(&remainder.value, &original_remainder)))) {
        fail("division", case_id, 0);
    }
    check_guarded_value(&quotient, "division quotient canary", case_id, 0);
    check_guarded_value(&remainder, "division remainder canary", case_id, 0);
    check_guarded_div_workspace(&workspace, "division workspace", case_id, 0);

    if (test_alias) {
        ctint1728_32_t alias_a = *a;
        ctint1728_32_t alias_b = *b;
        if (ctint1728_32_div_trunc(&alias_a,
                                   &alias_b,
                                   &alias_a,
                                   &alias_b,
                                   &workspace.value) != expected_status ||
            (expected_status &&
             (!same_value(&alias_a, &expected_quotient) ||
              !same_value(&alias_b, &expected_remainder))) ||
            (!expected_status &&
             (!same_value(&alias_a, a) || !same_value(&alias_b, b)))) {
            fail("division input aliases", case_id, 0);
        }
        check_guarded_div_workspace(&workspace,
                                    "division alias workspace",
                                    case_id,
                                    0);
    }

    *digest ^= quotient.value.words[(case_id + 13u) % CTINT1728_WORDS] ^
               remainder.value.words[(case_id + 29u) % CTINT1728_WORDS] ^
               (uint32_t)expected_status;
    *digest *= UINT64_C(0x100000001b3);
}


static void
test_mod_case(const ctint1728_32_t *a,
              const ctint1728_32_t *modulus,
              size_t case_id,
              int test_alias,
              uint64_t *digest)
{
    guarded_value_t output = {TEST_CANARY_A, {{0}}, TEST_CANARY_B};
    guarded_mod_workspace_t workspace = {
        TEST_CANARY_A, {0}, TEST_CANARY_B
    };
    ctint1728_32_t expected;
    output.value = *modulus;
    const ctint1728_32_t original_output = output.value;
    const int expected_status = ref_mod(&expected, a, modulus);
    const int actual_status = ctint1728_32_mod(&output.value,
                                               a,
                                               modulus,
                                               &workspace.value);
    if (actual_status != expected_status ||
        (expected_status && !same_value(&output.value, &expected)) ||
        (!expected_status &&
         !same_value(&output.value, &original_output))) {
        fail("modular reduction", case_id, 0);
    }
    check_guarded_value(&output, "modular output canary", case_id, 0);
    check_guarded_mod_workspace(&workspace,
                                "modular workspace",
                                case_id,
                                0);

    if (test_alias) {
        ctint1728_32_t alias = *a;
        if (ctint1728_32_mod(&alias,
                             &alias,
                             modulus,
                             &workspace.value) != expected_status ||
            (expected_status && !same_value(&alias, &expected)) ||
            (!expected_status && !same_value(&alias, a))) {
            fail("modular dividend alias", case_id, 0);
        }
        check_guarded_mod_workspace(&workspace,
                                    "modular dividend alias workspace",
                                    case_id,
                                    0);

        alias = *modulus;
        if (ctint1728_32_mod(&alias,
                             a,
                             &alias,
                             &workspace.value) != expected_status ||
            (expected_status && !same_value(&alias, &expected)) ||
            (!expected_status && !same_value(&alias, modulus))) {
            fail("modular modulus alias", case_id, 0);
        }
        check_guarded_mod_workspace(&workspace,
                                    "modular modulus alias workspace",
                                    case_id,
                                    0);
    }

    *digest ^= output.value.words[(case_id + 37u) % CTINT1728_WORDS] ^
               (uint32_t)expected_status;
    *digest *= UINT64_C(0x100000001b3);
}


static void
test_division(const ctint1728_32_t boundaries[10], uint64_t *digest)
{
    for (size_t a_index = 0; a_index < 10; ++a_index) {
        for (size_t b_index = 0; b_index < 10; ++b_index) {
            test_div_case(&boundaries[a_index],
                          &boundaries[b_index],
                          a_index * 10u + b_index,
                          1,
                          digest);
            test_mod_case(&boundaries[a_index],
                          &boundaries[b_index],
                          a_index * 10u + b_index,
                          1,
                          digest);
        }
    }
    for (size_t case_id = 100; case_id < 228; ++case_id) {
        ctint1728_32_t a;
        ctint1728_32_t b;
        random_value(&a);
        random_value(&b);
        test_div_case(&a, &b, case_id, 0, digest);
        test_mod_case(&a, &b, case_id, 0, digest);
    }
    for (size_t case_id = 228; case_id < 356; ++case_id) {
        ctint1728_32_t a;
        ctint1728_32_t b;
        random_bounded_value(&a);
        random_bounded_value(&b);
        test_div_case(&a, &b, case_id, 0, digest);
        test_mod_case(&a, &b, case_id, 0, digest);
    }
    for (size_t case_id = 356; case_id < 420; ++case_id) {
        ctint1728_32_t a;
        ctint1728_32_t b = {{0}};
        random_value(&a);
        b.words[0] = (uint32_t)next_u64() | 1u;
        b.words[1] = (uint32_t)(next_u64() >> 32);
        if ((next_u64() & 1u) != 0) {
            ref_negate(&b);
        }
        test_div_case(&a, &b, case_id, 0, digest);
        test_mod_case(&a, &b, case_id, 0, digest);
    }
}


enum {
    POW_ALIAS_NONE = 0,
    POW_ALIAS_BASE = 1,
    POW_ALIAS_EXPONENT = 2,
    POW_ALIAS_MODULUS = 3,
};


static void
test_pow_case(const ctint1728_32_t *base,
              const ctint1728_32_t *exponent,
              const ctint1728_32_t *modulus,
              int expected_status,
              const ctint1728_32_t *expected,
              int alias_kind,
              size_t case_id,
              uint64_t *digest)
{
    guarded_value_t guarded_base = {TEST_CANARY_A, {{0}}, TEST_CANARY_B};
    guarded_value_t guarded_exponent = {TEST_CANARY_A, {{0}}, TEST_CANARY_B};
    guarded_value_t guarded_modulus = {TEST_CANARY_A, {{0}}, TEST_CANARY_B};
    guarded_value_t guarded_output = {TEST_CANARY_A, {{0}}, TEST_CANARY_B};
    guarded_pow_workspace_t workspace = {
        TEST_CANARY_A, {0}, TEST_CANARY_B
    };
    guarded_base.value = *base;
    guarded_exponent.value = *exponent;
    guarded_modulus.value = *modulus;
    guarded_output.value = *base;

    ctint1728_32_t *output = &guarded_output.value;
    if (alias_kind == POW_ALIAS_BASE) {
        output = &guarded_base.value;
    } else if (alias_kind == POW_ALIAS_EXPONENT) {
        output = &guarded_exponent.value;
    } else if (alias_kind == POW_ALIAS_MODULUS) {
        output = &guarded_modulus.value;
    }
    const ctint1728_32_t original_output = *output;
    const int actual_status = ctint1728_32_pow_mod_521(
        output,
        &guarded_base.value,
        &guarded_exponent.value,
        &guarded_modulus.value,
        &workspace.value);
    if (actual_status != expected_status ||
        (expected_status && !same_value(output, expected)) ||
        (!expected_status && !same_value(output, &original_output))) {
        fail("modular exponentiation", case_id, (uint32_t)alias_kind);
    }
    check_guarded_value(&guarded_base,
                        "pow base canary",
                        case_id,
                        (uint32_t)alias_kind);
    check_guarded_value(&guarded_exponent,
                        "pow exponent canary",
                        case_id,
                        (uint32_t)alias_kind);
    check_guarded_value(&guarded_modulus,
                        "pow modulus canary",
                        case_id,
                        (uint32_t)alias_kind);
    check_guarded_value(&guarded_output,
                        "pow output canary",
                        case_id,
                        (uint32_t)alias_kind);
    check_guarded_pow_workspace(&workspace,
                                "pow workspace",
                                case_id,
                                (uint32_t)alias_kind);

    *digest ^= output->words[(case_id + 47u) % CTINT1728_WORDS] ^
               ((uint64_t)(uint32_t)expected_status << 32) ^
               (uint32_t)alias_kind;
    *digest *= UINT64_C(0x100000001b3);
}


static void
test_modular_exponentiation(uint64_t *digest)
{
    ctint1728_32_t base;
    ctint1728_32_t exponent;
    ctint1728_32_t modulus;
    ctint1728_32_t expected = {{0}};

    set_i64(&base, -INT64_C(123456789));
    set_u64(&exponent, 0);
    set_u64(&modulus, UINT64_C(1000000007));
    set_u64(&expected,
            small_pow_reference(-INT64_C(123456789), &exponent,
                                UINT64_C(1000000007)));
    test_pow_case(&base, &exponent, &modulus, 1, &expected,
                  POW_ALIAS_NONE, 0, digest);

    set_i64(&base, -17);
    set_u64(&exponent, 117);
    set_u64(&modulus, 97);
    set_u64(&expected, small_pow_reference(-17, &exponent, 97));
    test_pow_case(&base, &exponent, &modulus, 1, &expected,
                  POW_ALIAS_BASE, 1, digest);

    set_i64(&base, 5);
    memset(&exponent, 0, sizeof(exponent));
    exponent.words[16] = UINT32_C(1) << 8;
    set_u64(&modulus, 19);
    set_u64(&expected, small_pow_reference(5, &exponent, 19));
    test_pow_case(&base, &exponent, &modulus, 1, &expected,
                  POW_ALIAS_EXPONENT, 2, digest);

    set_i64(&base, 5);
    set_u64(&exponent, 1);
    memset(&modulus, 0, sizeof(modulus));
    modulus.words[26] = UINT32_C(1) << 30;
    set_u64(&expected, 5);
    test_pow_case(&base, &exponent, &modulus, 1, &expected,
                  POW_ALIAS_MODULUS, 3, digest);

    set_i64(&base, 7);
    set_i64(&exponent, -1);
    set_u64(&modulus, 97);
    test_pow_case(&base, &exponent, &modulus, 0, &expected,
                  POW_ALIAS_NONE, 4, digest);

    set_u64(&exponent, 0);
    exponent.words[16] = UINT32_C(1) << 9;
    test_pow_case(&base, &exponent, &modulus, 0, &expected,
                  POW_ALIAS_NONE, 5, digest);

    set_u64(&exponent, 3);
    memset(&modulus, 0, sizeof(modulus));
    test_pow_case(&base, &exponent, &modulus, 0, &expected,
                  POW_ALIAS_NONE, 6, digest);

    set_i64(&modulus, -97);
    test_pow_case(&base, &exponent, &modulus, 0, &expected,
                  POW_ALIAS_NONE, 7, digest);

    memset(&modulus, 0, sizeof(modulus));
    modulus.words[26] = UINT32_C(1) << 31;
    test_pow_case(&base, &exponent, &modulus, 0, &expected,
                  POW_ALIAS_NONE, 8, digest);
}


static void
test_sqrt_case(const ctint1728_32_t *input,
               int expected_status,
               const ctint1728_32_t *expected,
               int alias_output,
               size_t case_id,
               uint64_t *digest)
{
    const ctint1728_32_t input_copy = *input;
    guarded_value_t guarded_input = {TEST_CANARY_A, {{0}}, TEST_CANARY_B};
    guarded_value_t guarded_output = {TEST_CANARY_A, {{0}}, TEST_CANARY_B};
    guarded_sqrt_workspace_t workspace = {
        TEST_CANARY_A, {0}, TEST_CANARY_B
    };
    guarded_input.value = *input;
    guarded_output.value = *input;
    ctint1728_32_t *output = alias_output ? &guarded_input.value
                                          : &guarded_output.value;
    const ctint1728_32_t original_output = *output;
    const int actual_status = ctint1728_32_sqrt_exact(
        output,
        &guarded_input.value,
        &workspace.value);
    if (actual_status != expected_status ||
        (expected_status && !same_value(output, expected)) ||
        (!expected_status && !same_value(output, &original_output))) {
        fail("exact integer square root", case_id, (uint32_t)alias_output);
    }
    check_guarded_value(&guarded_input,
                        "sqrt input canary",
                        case_id,
                        (uint32_t)alias_output);
    check_guarded_value(&guarded_output,
                        "sqrt output canary",
                        case_id,
                        (uint32_t)alias_output);
    check_guarded_sqrt_workspace(&workspace,
                                 "sqrt workspace",
                                 case_id,
                                 (uint32_t)alias_output);

    /* The floor-root API uses the same complete schedule but publishes for
     * every nonnegative input.  Reset aliased storage before exercising it. */
    guarded_input.value = input_copy;
    guarded_output.value = input_copy;
    output = alias_output ? &guarded_input.value : &guarded_output.value;
    const ctint1728_32_t original_floor_output = *output;
    const int expected_floor_status = ref_sign(&input_copy) >= 0;
    const int actual_floor_status = ctint1728_32_sqrt_floor(
        output,
        &guarded_input.value,
        &workspace.value);
    if (actual_floor_status != expected_floor_status ||
        (expected_floor_status && !same_value(output, expected)) ||
        (!expected_floor_status &&
         !same_value(output, &original_floor_output))) {
        fail("floor integer square root",
             case_id,
             (uint32_t)alias_output);
    }
    check_guarded_value(&guarded_input,
                        "floor sqrt input canary",
                        case_id,
                        (uint32_t)alias_output);
    check_guarded_value(&guarded_output,
                        "floor sqrt output canary",
                        case_id,
                        (uint32_t)alias_output);
    check_guarded_sqrt_workspace(&workspace,
                                 "floor sqrt workspace",
                                 case_id,
                                 (uint32_t)alias_output);
    *digest ^= output->words[(case_id + 53u) % CTINT1728_WORDS] ^
               ((uint64_t)(uint32_t)expected_status << 32) ^
               (uint32_t)alias_output;
    *digest *= UINT64_C(0x100000001b3);
}


static void
test_integer_square_root(uint64_t *digest)
{
    static const uint64_t small_inputs[] = {
        UINT64_C(0),
        UINT64_C(1),
        UINT64_C(2),
        UINT64_C(4),
        UINT64_C(15),
        UINT64_C(16),
        UINT64_C(17),
        UINT64_C(0xffffffff) * UINT64_C(0xffffffff),
        UINT64_MAX,
    };
    ctint1728_32_t input;
    ctint1728_32_t expected;
    for (size_t case_id = 0;
         case_id < sizeof(small_inputs) / sizeof(small_inputs[0]);
         ++case_id) {
        const uint64_t root = small_sqrt_floor_reference(
            small_inputs[case_id]);
        const int exact = (__uint128_t)root * root == small_inputs[case_id];
        set_u64(&input, small_inputs[case_id]);
        set_u64(&expected, root);
        test_sqrt_case(&input,
                       exact,
                       &expected,
                       (int)(case_id & 1u),
                       case_id,
                       digest);
    }

    set_i64(&input, -1);
    memset(&expected, 0, sizeof(expected));
    test_sqrt_case(&input, 0, &expected, 1,
                   sizeof(small_inputs) / sizeof(small_inputs[0]), digest);

    const ctint1728_32_t one = {{1}};
    for (size_t random_id = 0; random_id < 32; ++random_id) {
        ctint1728_32_t root = {{0}};
        ctint1728_32_t square;
        ctint1728_32_t nonsquare;
        if (random_id == 0) {
            for (size_t i = 0; i < 26; ++i) {
                root.words[i] = UINT32_MAX;
            }
            root.words[26] = UINT32_C(0x7fffffff);
        } else {
            for (size_t i = 0; i < 26; i += 2) {
                const uint64_t word = next_u64();
                root.words[i] = (uint32_t)word;
                root.words[i + 1] = (uint32_t)(word >> 32);
            }
            root.words[26] =
                (uint32_t)next_u64() & UINT32_C(0x7fffffff);
        }
        root.words[0] |= 1u;
        if (!ref_mul(&square, &root, &root)) {
            fail("sqrt reference square overflow", random_id, 0);
        }
        test_sqrt_case(&square, 1, &root, (int)(random_id & 1u),
                       10u + 2u * random_id, digest);
        if (!ref_add(&nonsquare, &square, &one)) {
            fail("sqrt reference near-square overflow", random_id, 0);
        }
        test_sqrt_case(&nonsquare, 0, &root, (int)(1u ^ (random_id & 1u)),
                       11u + 2u * random_id, digest);
    }
}


static uint64_t
small_halfgcd_reference(uint64_t modulus, uint64_t root)
{
    uint64_t previous = modulus;
    uint64_t current = root;
    while ((__uint128_t)current * current >= modulus) {
        const uint64_t remainder = previous % current;
        previous = current;
        current = remainder;
    }
    return current;
}


enum halfgcd_alias_kind {
    HALFGCD_ALIAS_NONE = 0,
    HALFGCD_ALIAS_MODULUS = 1,
    HALFGCD_ALIAS_ROOT = 2,
};


static void
test_halfgcd_case(const ctint1728_32_t *modulus,
                  const ctint1728_32_t *root,
                  int expected_status,
                  const ctint1728_32_t *expected,
                  enum halfgcd_alias_kind alias_kind,
                  size_t case_id,
                  uint64_t *digest)
{
    guarded_value_t guarded_modulus = {
        TEST_CANARY_A, {{0}}, TEST_CANARY_B
    };
    guarded_value_t guarded_root = {
        TEST_CANARY_A, {{0}}, TEST_CANARY_B
    };
    guarded_value_t guarded_output = {
        TEST_CANARY_A, {{0}}, TEST_CANARY_B
    };
    guarded_halfgcd_workspace_t workspace = {
        TEST_CANARY_A, {0}, TEST_CANARY_B
    };
    guarded_modulus.value = *modulus;
    guarded_root.value = *root;
    guarded_output.value = *root;
    ctint1728_32_t *output = &guarded_output.value;
    if (alias_kind == HALFGCD_ALIAS_MODULUS) {
        output = &guarded_modulus.value;
    } else if (alias_kind == HALFGCD_ALIAS_ROOT) {
        output = &guarded_root.value;
    }
    const ctint1728_32_t original_output = *output;

    const int actual_status = ctint1728_32_cornacchia_halfgcd_492(
        output,
        &guarded_modulus.value,
        &guarded_root.value,
        &workspace.value);
    if (actual_status != expected_status ||
        (expected_status && !same_value(output, expected)) ||
        (!expected_status && !same_value(output, &original_output))) {
        fail("Cornacchia half-GCD", case_id, (uint32_t)alias_kind);
    }
    check_guarded_value(&guarded_modulus,
                        "halfgcd modulus canary",
                        case_id,
                        (uint32_t)alias_kind);
    check_guarded_value(&guarded_root,
                        "halfgcd root canary",
                        case_id,
                        (uint32_t)alias_kind);
    check_guarded_value(&guarded_output,
                        "halfgcd output canary",
                        case_id,
                        (uint32_t)alias_kind);
    check_guarded_halfgcd_workspace(&workspace,
                                    "halfgcd workspace",
                                    case_id,
                                    (uint32_t)alias_kind);
    *digest ^= output->words[(case_id + 7u) % CTINT1728_WORDS] ^
               ((uint64_t)(uint32_t)expected_status << 32) ^
               (uint32_t)alias_kind;
    *digest *= UINT64_C(0x100000001b3);
}


static void
test_cornacchia_halfgcd(uint64_t *digest)
{
    static const struct {
        uint64_t modulus;
        uint64_t root;
    } fixed_cases[] = {
        {UINT64_C(1), UINT64_C(0)},
        {UINT64_C(2), UINT64_C(1)},
        {UINT64_C(3), UINT64_C(1)},
        {UINT64_C(4), UINT64_C(2)},
        {UINT64_C(5), UINT64_C(1)},
        {UINT64_C(5), UINT64_C(2)},
        {UINT64_C(13), UINT64_C(5)},
        {UINT64_C(17), UINT64_C(4)},
        {UINT64_C(29), UINT64_C(12)},
        {UINT64_C(61), UINT64_C(11)},
        {UINT64_C(7349), UINT64_C(2061)},
        {UINT64_C(0xffffffffffffffc5), UINT64_C(0x6a09e667f3bcc909)},
    };
    ctint1728_32_t modulus;
    ctint1728_32_t root;
    ctint1728_32_t expected;
    size_t case_id = 0;

    for (; case_id < sizeof(fixed_cases) / sizeof(fixed_cases[0]);
         ++case_id) {
        set_u64(&modulus, fixed_cases[case_id].modulus);
        set_u64(&root, fixed_cases[case_id].root);
        set_u64(&expected,
                small_halfgcd_reference(fixed_cases[case_id].modulus,
                                        fixed_cases[case_id].root));
        test_halfgcd_case(&modulus,
                          &root,
                          1,
                          &expected,
                          (enum halfgcd_alias_kind)(case_id % 3u),
                          case_id,
                          digest);
    }

    for (size_t random_id = 0; random_id < 8; ++random_id, ++case_id) {
        const uint64_t modulus_word =
            (next_u64() | (UINT64_C(1) << 62) | 1u) &
            UINT64_C(0x7fffffffffffffff);
        const uint64_t root_word = next_u64() % modulus_word;
        set_u64(&modulus, modulus_word);
        set_u64(&root, root_word);
        set_u64(&expected,
                small_halfgcd_reference(modulus_word, root_word));
        test_halfgcd_case(&modulus,
                          &root,
                          1,
                          &expected,
                          (enum halfgcd_alias_kind)(random_id % 3u),
                          case_id,
                          digest);
    }

    /* Every malformed case must retain the initialized output despite the
     * full 1421-round schedule. */
    set_u64(&modulus, 0);
    set_u64(&root, 0);
    test_halfgcd_case(&modulus, &root, 0, &expected,
                      HALFGCD_ALIAS_NONE, case_id++, digest);
    set_i64(&modulus, -5);
    set_u64(&root, 2);
    test_halfgcd_case(&modulus, &root, 0, &expected,
                      HALFGCD_ALIAS_MODULUS, case_id++, digest);
    memset(&modulus, 0, sizeof(modulus));
    modulus.words[CTINT1728_CORNACCHIA_MAX_BITS >> 5] =
        UINT32_C(1) << (CTINT1728_CORNACCHIA_MAX_BITS & 31u);
    set_u64(&root, 1);
    test_halfgcd_case(&modulus, &root, 0, &expected,
                      HALFGCD_ALIAS_ROOT, case_id++, digest);
    set_u64(&modulus, 61);
    set_i64(&root, -1);
    test_halfgcd_case(&modulus, &root, 0, &expected,
                      HALFGCD_ALIAS_NONE, case_id++, digest);
    set_u64(&root, 61);
    test_halfgcd_case(&modulus, &root, 0, &expected,
                      HALFGCD_ALIAS_MODULUS, case_id++, digest);
    set_u64(&root, 62);
    test_halfgcd_case(&modulus, &root, 0, &expected,
                      HALFGCD_ALIAS_ROOT, case_id++, digest);
}


static void
test_one_shift(const ctint1728_32_t *input,
               uint32_t shift,
               size_t case_id,
               uint64_t *digest)
{
    guarded_value_t output = {TEST_CANARY_A, {{0}}, TEST_CANARY_B};
    guarded_workspace_t workspace = {TEST_CANARY_A, {0}, TEST_CANARY_B};
    ctint1728_32_t expected;

    ref_lshift(&expected, input, shift);
    const int expected_left_status =
        ref_bit_length_unsigned(input) <= CTINT1728_BITS - shift;
    const ctint1728_32_t original_output = output.value;
    const int left_status = ctint1728_32_lshift(&output.value,
                                                input,
                                                shift,
                                                &workspace.value);
    if (left_status != expected_left_status ||
        (expected_left_status && !same_value(&output.value, &expected)) ||
        (!expected_left_status &&
         !same_value(&output.value, &original_output))) {
        fail("left shift", case_id, shift);
    }
    check_guarded_value(&output, "left output canary", case_id, shift);
    check_guarded_workspace(&workspace, "left workspace", case_id, shift);

    ref_rshift(&expected, input, shift);
    memset(&output.value, 0, sizeof(output.value));
    const int right_status = ctint1728_32_rshift(&output.value,
                                                 input,
                                                 shift,
                                                 &workspace.value);
    if (right_status != 1 || !same_value(&output.value, &expected)) {
        fail("right shift", case_id, shift);
    }
    check_guarded_value(&output, "right output canary", case_id, shift);
    check_guarded_workspace(&workspace, "right workspace", case_id, shift);

    ctint1728_32_t alias = *input;
    ref_lshift(&expected, input, shift);
    const ctint1728_32_t original_alias = alias;
    const int alias_status =
        ctint1728_32_lshift(&alias, &alias, shift, &workspace.value);
    if (alias_status != expected_left_status ||
        (expected_left_status && !same_value(&alias, &expected)) ||
        (!expected_left_status && !same_value(&alias, &original_alias))) {
        fail("left alias", case_id, shift);
    }
    check_guarded_workspace(&workspace, "left alias workspace", case_id, shift);

    alias = *input;
    ref_rshift(&expected, input, shift);
    if (!ctint1728_32_rshift(&alias, &alias, shift, &workspace.value) ||
        !same_value(&alias, &expected)) {
        fail("right alias", case_id, shift);
    }
    check_guarded_workspace(&workspace, "right alias workspace", case_id, shift);

    *digest ^= alias.words[(case_id + shift) % CTINT1728_WORDS];
    *digest *= UINT64_C(0x100000001b3);
}


static void
test_shifts(const ctint1728_32_t boundaries[10], uint64_t *digest)
{
    for (size_t value_id = 0; value_id < 10; ++value_id) {
        for (uint32_t shift = 0; shift < CTINT1728_BITS; ++shift) {
            test_one_shift(&boundaries[value_id],
                           shift,
                           value_id * CTINT1728_BITS + shift,
                           digest);
        }
    }

    for (size_t case_id = 0; case_id < 2000; ++case_id) {
        ctint1728_32_t input;
        random_value(&input);
        test_one_shift(&input,
                       (uint32_t)(next_u64() % CTINT1728_BITS),
                       10u * CTINT1728_BITS + case_id,
                       digest);
    }

    const uint32_t invalid_shifts[] = {
        CTINT1728_BITS,
        CTINT1728_BITS + 1u,
        2047u,
        2048u,
        UINT32_MAX,
    };
    for (size_t case_id = 0;
         case_id < sizeof(invalid_shifts) / sizeof(invalid_shifts[0]);
         ++case_id) {
        guarded_value_t output = {TEST_CANARY_A, {{0}}, TEST_CANARY_B};
        guarded_workspace_t workspace = {TEST_CANARY_A, {0}, TEST_CANARY_B};
        ctint1728_32_t input;
        ctint1728_32_t original_output;
        random_value(&input);
        random_value(&output.value);
        original_output = output.value;
        const uint32_t shift = invalid_shifts[case_id];
        if (ctint1728_32_lshift(&output.value,
                               &input,
                               shift,
                               &workspace.value) != 0 ||
            !same_value(&output.value, &original_output)) {
            fail("invalid left nonpublication", case_id, shift);
        }
        check_guarded_value(&output, "invalid left canary", case_id, shift);
        check_guarded_workspace(&workspace,
                                "invalid left workspace",
                                case_id,
                                shift);
        if (ctint1728_32_rshift(&output.value,
                               &input,
                               shift,
                               &workspace.value) != 0 ||
            !same_value(&output.value, &original_output)) {
            fail("invalid right nonpublication", case_id, shift);
        }
        check_guarded_value(&output, "invalid right canary", case_id, shift);
        check_guarded_workspace(&workspace,
                                "invalid right workspace",
                                case_id,
                                shift);
    }
}


int
main(void)
{
    ctint1728_32_t boundaries[10];
    uint64_t digest = UINT64_C(0xcbf29ce484222325);
    initialize_boundaries(boundaries);
    test_scalar_operations(boundaries, &digest);
    test_arithmetic(boundaries, &digest);
    test_division(boundaries, &digest);
    test_modular_exponentiation(&digest);
    test_integer_square_root(&digest);
    test_cornacchia_halfgcd(&digest);
    test_shifts(boundaries, &digest);
    printf("ctint1728_32 PASS: scalar_random=10000 add_sub_random=10000 "
           "mul_full_random=2000 mul_bounded_random=2000 "
           "div_boundary=100 div_random=320 mod_boundary=100 "
           "mod_random=320 pow_valid=4 pow_invalid=5 sqrt_boundary=10 "
           "sqrt_random_exact=32 sqrt_random_nonsquare=32 "
           "halfgcd_valid=20 halfgcd_invalid=6 shift_valid=%u "
           "shift_random=2000 digest=%016" PRIx64 "\n",
           10u * CTINT1728_BITS,
           digest);
    return 0;
}
