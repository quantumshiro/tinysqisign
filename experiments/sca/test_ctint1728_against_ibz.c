#include "ctint1728_32.h"
#include "intbig.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


_Static_assert(IBZ_LIMBS == 27, "bridge test requires Level-I ibz_t");
_Static_assert(sizeof(ibz_t) == sizeof(ctint1728_32_t),
               "bridge representations must have the same width");

static uint64_t random_state = UINT64_C(0x13198a2e03707344);


static void negate_ctint(ctint1728_32_t *value);


static void
fail(const char *label, size_t case_id, uint32_t detail)
{
    fprintf(stderr,
            "ctint/ibz differential FAIL: %s case=%zu detail=%" PRIu32 "\n",
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
random_ctint(ctint1728_32_t *value)
{
    for (size_t i = 0; i < CTINT1728_WORDS; i += 2) {
        const uint64_t word = next_u64();
        value->words[i] = (uint32_t)word;
        value->words[i + 1] = (uint32_t)(word >> 32);
    }
}


static void
set_u64_ctint(ctint1728_32_t *value, uint64_t word)
{
    memset(value, 0, sizeof(*value));
    value->words[0] = (uint32_t)word;
    value->words[1] = (uint32_t)(word >> 32);
}


static void
set_i64_ctint(ctint1728_32_t *value, int64_t word)
{
    const uint64_t magnitude =
        word < 0 ? UINT64_C(0) - (uint64_t)word : (uint64_t)word;
    set_u64_ctint(value, magnitude);
    if (word < 0) {
        negate_ctint(value);
    }
}


static void
negate_ctint(ctint1728_32_t *value)
{
    uint32_t carry = 1;
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        const uint32_t complemented = ~value->words[i];
        value->words[i] = complemented + carry;
        carry = value->words[i] < complemented;
    }
}


static void
random_bounded_ctint(ctint1728_32_t *value)
{
    memset(value, 0, sizeof(*value));
    for (size_t i = 0; i < 20; i += 2) {
        const uint64_t word = next_u64();
        value->words[i] = (uint32_t)word;
        value->words[i + 1] = (uint32_t)(word >> 32);
    }
    if ((next_u64() & 1u) != 0) {
        negate_ctint(value);
    }
}


static void
ctint_to_ibz(ibz_t *out, const ctint1728_32_t *in)
{
    for (size_t i = 0; i < IBZ_LIMBS; ++i) {
        (*out)[i] = (uint64_t)in->words[2 * i] |
                    ((uint64_t)in->words[2 * i + 1] << 32);
    }
}


static void
ibz_to_ctint(ctint1728_32_t *out, const ibz_t *in)
{
    for (size_t i = 0; i < IBZ_LIMBS; ++i) {
        out->words[2 * i] = (uint32_t)(*in)[i];
        out->words[2 * i + 1] = (uint32_t)((*in)[i] >> 32);
    }
}


static int
same_ctint(const ctint1728_32_t *a, const ctint1728_32_t *b)
{
    return memcmp(a, b, sizeof(*a)) == 0;
}


static int32_t
ibz_sign_reference(const ibz_t *value)
{
    if (ibz_is_zero(value)) {
        return 0;
    }
    return ibz_is_negative(value) ? -1 : 1;
}


static void
test_value_pair(const ctint1728_32_t *a,
                const ctint1728_32_t *b,
                size_t case_id,
                uint64_t *digest)
{
    ibz_t ibz_a;
    ibz_t ibz_b;
    ibz_t ibz_result;
    ctint1728_32_t converted;
    ctint1728_32_t ct_result;
    ctint1728_32_t original;
    ctint1728_32_arith_workspace_t arithmetic_workspace = {0};
    ctint_to_ibz(&ibz_a, a);
    ctint_to_ibz(&ibz_b, b);
    ibz_to_ctint(&converted, &ibz_a);
    if (!same_ctint(a, &converted)) {
        fail("representation roundtrip", case_id, 0);
    }
    if (ctint1728_32_cmp_signed(a, b) != ibz_cmp(&ibz_a, &ibz_b) ||
        ctint1728_32_sign(a) != ibz_sign_reference(&ibz_a) ||
        ctint1728_32_is_zero(a) != (uint32_t)ibz_is_zero(&ibz_a) ||
        ctint1728_32_abs_bit_length(a) != (uint32_t)ibz_bitsize(&ibz_a)) {
        fail("scalar differential", case_id, 0);
    }
    ibz_abs(&ibz_result, &ibz_a);
    ibz_to_ctint(&converted, &ibz_result);
    ctint1728_32_abs(&ct_result, a);
    if (!same_ctint(&converted, &ct_result)) {
        fail("abs differential", case_id, 0);
    }

    ibz_add(&ibz_result, &ibz_a, &ibz_b);
    ibz_to_ctint(&converted, &ibz_result);
    const int sign_a = ibz_sign_reference(&ibz_a) < 0;
    const int sign_b = ibz_sign_reference(&ibz_b) < 0;
    const int sign_sum = ibz_sign_reference(&ibz_result) < 0;
    const int add_valid = !((sign_a == sign_b) && (sign_sum != sign_a));
    ct_result = *b;
    original = ct_result;
    if (ctint1728_32_add(&ct_result,
                         a,
                         b,
                         &arithmetic_workspace) != add_valid ||
        (add_valid && !same_ctint(&ct_result, &converted)) ||
        (!add_valid && !same_ctint(&ct_result, &original)) ||
        memcmp(&arithmetic_workspace,
               &(ctint1728_32_arith_workspace_t){0},
               sizeof(arithmetic_workspace)) != 0) {
        fail("add differential", case_id, 0);
    }

    ibz_sub(&ibz_result, &ibz_a, &ibz_b);
    ibz_to_ctint(&converted, &ibz_result);
    const int sign_diff = ibz_sign_reference(&ibz_result) < 0;
    const int sub_valid = !((sign_a != sign_b) && (sign_diff != sign_a));
    ct_result = *a;
    original = ct_result;
    if (ctint1728_32_sub(&ct_result,
                         a,
                         b,
                         &arithmetic_workspace) != sub_valid ||
        (sub_valid && !same_ctint(&ct_result, &converted)) ||
        (!sub_valid && !same_ctint(&ct_result, &original)) ||
        memcmp(&arithmetic_workspace,
               &(ctint1728_32_arith_workspace_t){0},
               sizeof(arithmetic_workspace)) != 0) {
        fail("sub differential", case_id, 0);
    }
    *digest ^= converted.words[case_id % CTINT1728_WORDS];
    *digest *= UINT64_C(0x100000001b3);
}


static void
test_bounded_multiplication(size_t case_id, uint64_t *digest)
{
    ctint1728_32_t a;
    ctint1728_32_t b;
    ctint1728_32_t expected;
    ctint1728_32_t actual = {{0}};
    ctint1728_32_mul_workspace_t workspace = {0};
    ibz_t ibz_a;
    ibz_t ibz_b;
    ibz_t ibz_result;
    random_bounded_ctint(&a);
    random_bounded_ctint(&b);
    ctint_to_ibz(&ibz_a, &a);
    ctint_to_ibz(&ibz_b, &b);

    ibz_mul(&ibz_result, &ibz_a, &ibz_b);
    ibz_to_ctint(&expected, &ibz_result);
    if (!ctint1728_32_mul(&actual, &a, &b, &workspace) ||
        !same_ctint(&actual, &expected) ||
        memcmp(&workspace,
               &(ctint1728_32_mul_workspace_t){0},
               sizeof(workspace)) != 0) {
        fail("bounded mul differential", case_id, 0);
    }

    ibz_mul(&ibz_result, &ibz_a, &ibz_a);
    ibz_to_ctint(&expected, &ibz_result);
    actual = b;
    if (!ctint1728_32_square(&actual, &a, &workspace) ||
        !same_ctint(&actual, &expected) ||
        memcmp(&workspace,
               &(ctint1728_32_mul_workspace_t){0},
               sizeof(workspace)) != 0) {
        fail("bounded square differential", case_id, 0);
    }

    *digest ^= actual.words[(case_id + 11u) % CTINT1728_WORDS];
    *digest *= UINT64_C(0x100000001b3);
}


static int
is_int_min(const ctint1728_32_t *value)
{
    if (value->words[CTINT1728_WORDS - 1] != UINT32_C(0x80000000)) {
        return 0;
    }
    for (size_t i = 0; i + 1u < CTINT1728_WORDS; ++i) {
        if (value->words[i] != 0) {
            return 0;
        }
    }
    return 1;
}


static int
is_minus_one(const ctint1728_32_t *value)
{
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        if (value->words[i] != UINT32_MAX) {
            return 0;
        }
    }
    return 1;
}


static void
test_division_differential(ctint1728_32_t a,
                           ctint1728_32_t b,
                           size_t case_id,
                           uint64_t *digest)
{
    if (ctint1728_32_is_zero(&b)) {
        b.words[0] = 1;
    }
    if (is_int_min(&a) && is_minus_one(&b)) {
        memset(&b, 0, sizeof(b));
        b.words[0] = 1;
    }
    ibz_t ibz_a;
    ibz_t ibz_b;
    ibz_t ibz_q;
    ibz_t ibz_r;
    ibz_t ibz_mod_r;
    ctint1728_32_t expected_q;
    ctint1728_32_t expected_r;
    ctint1728_32_t expected_mod;
    ctint1728_32_t actual_q = {{0}};
    ctint1728_32_t actual_r = {{0}};
    ctint1728_32_t actual_mod = a;
    ctint1728_32_div_workspace_t workspace = {0};
    ctint1728_32_mod_workspace_t mod_workspace = {0};
    ctint_to_ibz(&ibz_a, &a);
    ctint_to_ibz(&ibz_b, &b);
    ibz_div(&ibz_q, &ibz_r, &ibz_a, &ibz_b);
    ibz_mod(&ibz_mod_r, &ibz_a, &ibz_b);
    ibz_to_ctint(&expected_q, &ibz_q);
    ibz_to_ctint(&expected_r, &ibz_r);
    ibz_to_ctint(&expected_mod, &ibz_mod_r);
    if (!ctint1728_32_div_trunc(&actual_q,
                                &actual_r,
                                &a,
                                &b,
                                &workspace) ||
        !same_ctint(&actual_q, &expected_q) ||
        !same_ctint(&actual_r, &expected_r) ||
        memcmp(&workspace,
               &(ctint1728_32_div_workspace_t){0},
               sizeof(workspace)) != 0) {
        fail("division differential", case_id, 0);
    }
    if (!ctint1728_32_mod(&actual_mod, &a, &b, &mod_workspace) ||
        !same_ctint(&actual_mod, &expected_mod) ||
        memcmp(&mod_workspace,
               &(ctint1728_32_mod_workspace_t){0},
               sizeof(mod_workspace)) != 0) {
        fail("modular reduction differential", case_id, 0);
    }
    *digest ^= actual_q.words[(case_id + 17u) % CTINT1728_WORDS] ^
               actual_r.words[(case_id + 31u) % CTINT1728_WORDS] ^
               actual_mod.words[(case_id + 43u) % CTINT1728_WORDS];
    *digest *= UINT64_C(0x100000001b3);
}


static void
test_pow_differential(const ctint1728_32_t *base,
                      const ctint1728_32_t *exponent,
                      const ctint1728_32_t *modulus,
                      size_t case_id,
                      uint64_t *digest)
{
    ibz_t ibz_base;
    ibz_t ibz_exponent;
    ibz_t ibz_modulus;
    ibz_t ibz_result;
    ctint1728_32_t expected;
    ctint1728_32_t actual = *base;
    ctint1728_32_pow_workspace_t workspace = {0};
    ctint_to_ibz(&ibz_base, base);
    ctint_to_ibz(&ibz_exponent, exponent);
    ctint_to_ibz(&ibz_modulus, modulus);
    ibz_pow_mod(&ibz_result, &ibz_base, &ibz_exponent, &ibz_modulus);
    ibz_to_ctint(&expected, &ibz_result);
    if (!ctint1728_32_pow_mod_521(&actual,
                                 base,
                                 exponent,
                                 modulus,
                                 &workspace) ||
        !same_ctint(&actual, &expected) ||
        memcmp(&workspace,
               &(ctint1728_32_pow_workspace_t){0},
               sizeof(workspace)) != 0) {
        fail("modular exponentiation differential", case_id, 0);
    }
    *digest ^= actual.words[(case_id + 49u) % CTINT1728_WORDS];
    *digest *= UINT64_C(0x100000001b3);
}


static void
test_sqrt_differential(const ctint1728_32_t *input,
                       size_t case_id,
                       uint64_t *digest)
{
    ibz_t ibz_input;
    ibz_t ibz_result;
    ctint1728_32_t expected = {{0}};
    ctint1728_32_t actual = *input;
    const ctint1728_32_t original = actual;
    ctint1728_32_sqrt_workspace_t workspace = {0};
    ctint_to_ibz(&ibz_input, input);
    const int expected_status = ibz_sqrt(&ibz_result, &ibz_input);
    if (ibz_sign_reference(&ibz_input) >= 0) {
        ibz_to_ctint(&expected, &ibz_result);
    }
    const int actual_status = ctint1728_32_sqrt_exact(
        &actual,
        input,
        &workspace);
    if (actual_status != expected_status ||
        (expected_status && !same_ctint(&actual, &expected)) ||
        (!expected_status && !same_ctint(&actual, &original)) ||
        memcmp(&workspace,
               &(ctint1728_32_sqrt_workspace_t){0},
               sizeof(workspace)) != 0) {
        fail("exact square-root differential", case_id, 0);
    }

    actual = *input;
    const int floor_status = ctint1728_32_sqrt_floor(
        &actual,
        input,
        &workspace);
    const int expected_floor_status = ibz_sign_reference(&ibz_input) >= 0;
    if (floor_status != expected_floor_status ||
        (expected_floor_status && !same_ctint(&actual, &expected)) ||
        (!expected_floor_status && !same_ctint(&actual, &original)) ||
        memcmp(&workspace,
               &(ctint1728_32_sqrt_workspace_t){0},
               sizeof(workspace)) != 0) {
        fail("floor square-root differential", case_id, 0);
    }
    *digest ^= actual.words[(case_id + 51u) % CTINT1728_WORDS] ^
               (uint32_t)expected_status;
    *digest *= UINT64_C(0x100000001b3);
}


static void
test_halfgcd_differential(const ctint1728_32_t *modulus,
                          const ctint1728_32_t *root,
                          size_t case_id,
                          uint64_t *digest)
{
    ibz_t ibz_modulus;
    ibz_t ibz_root;
    ibz_t previous;
    ibz_t current;
    ibz_t quotient;
    ibz_t remainder;
    ibz_t square;
    ctint1728_32_t expected;
    ctint1728_32_t actual = *root;
    ctint1728_32_halfgcd_workspace_t workspace = {0};
    ctint_to_ibz(&ibz_modulus, modulus);
    ctint_to_ibz(&ibz_root, root);
    ibz_copy(&previous, &ibz_modulus);
    ibz_copy(&current, &ibz_root);

    /* Independent variable-time Euclidean oracle.  It deliberately retains
     * the exact production stop predicate rather than mirroring the new
     * shift/subtract implementation. */
    ibz_mul(&square, &current, &current);
    while (ibz_cmp(&square, &ibz_modulus) >= 0) {
        ibz_div(&quotient, &remainder, &previous, &current);
        ibz_copy(&previous, &current);
        ibz_copy(&current, &remainder);
        ibz_mul(&square, &current, &current);
    }
    ibz_to_ctint(&expected, &current);

    if (!ctint1728_32_cornacchia_halfgcd_492(
            &actual,
            modulus,
            root,
            &workspace) ||
        !same_ctint(&actual, &expected) ||
        memcmp(&workspace,
               &(ctint1728_32_halfgcd_workspace_t){0},
               sizeof(workspace)) != 0) {
        fail("Cornacchia half-GCD differential", case_id, 0);
    }

    /* Exercise both supported output aliases against the same oracle. */
    actual = *modulus;
    if (!ctint1728_32_cornacchia_halfgcd_492(
            &actual,
            &actual,
            root,
            &workspace) ||
        !same_ctint(&actual, &expected) ||
        memcmp(&workspace,
               &(ctint1728_32_halfgcd_workspace_t){0},
               sizeof(workspace)) != 0) {
        fail("Cornacchia half-GCD modulus alias", case_id, 0);
    }
    actual = *root;
    if (!ctint1728_32_cornacchia_halfgcd_492(
            &actual,
            modulus,
            &actual,
            &workspace) ||
        !same_ctint(&actual, &expected) ||
        memcmp(&workspace,
               &(ctint1728_32_halfgcd_workspace_t){0},
               sizeof(workspace)) != 0) {
        fail("Cornacchia half-GCD root alias", case_id, 0);
    }

    *digest ^= actual.words[(case_id + 13u) % CTINT1728_WORDS];
    *digest *= UINT64_C(0x100000001b3);
}


static void
test_magnitude_shift(ctint1728_32_t magnitude,
                     size_t case_id,
                     uint64_t *digest)
{
    magnitude.words[CTINT1728_WORDS - 1] &= UINT32_C(0x7fffffff);
    const uint32_t bit_length =
        ctint1728_32_bit_length_unsigned(&magnitude);
    uint32_t maximum_shift = CTINT1728_BITS - bit_length;
    if (maximum_shift >= CTINT1728_BITS) {
        maximum_shift = CTINT1728_BITS - 1;
    }
    const uint32_t left_shift =
        (uint32_t)(next_u64() % ((uint64_t)maximum_shift + 1u));
    const uint32_t right_shift =
        (uint32_t)(next_u64() % CTINT1728_BITS);

    ibz_t ibz_input;
    ibz_t ibz_result;
    ctint1728_32_t expected;
    ctint1728_32_t actual = {{0}};
    ctint1728_32_shift_workspace_t workspace = {0};
    ctint_to_ibz(&ibz_input, &magnitude);

    ibz_mul_2exp(&ibz_result, &ibz_input, left_shift);
    ibz_to_ctint(&expected, &ibz_result);
    if (!ctint1728_32_lshift(&actual,
                             &magnitude,
                             left_shift,
                             &workspace) ||
        !same_ctint(&actual, &expected)) {
        fail("left shift differential", case_id, left_shift);
    }
    if (memcmp(&workspace,
               &(ctint1728_32_shift_workspace_t){0},
               sizeof(workspace)) != 0) {
        fail("left workspace clear", case_id, left_shift);
    }

    ibz_div_2exp(&ibz_result, &ibz_input, right_shift);
    ibz_to_ctint(&expected, &ibz_result);
    memset(&actual, 0, sizeof(actual));
    if (!ctint1728_32_rshift(&actual,
                             &magnitude,
                             right_shift,
                             &workspace) ||
        !same_ctint(&actual, &expected)) {
        fail("right shift differential", case_id, right_shift);
    }
    if (memcmp(&workspace,
               &(ctint1728_32_shift_workspace_t){0},
               sizeof(workspace)) != 0) {
        fail("right workspace clear", case_id, right_shift);
    }
    *digest ^= actual.words[(case_id + right_shift) % CTINT1728_WORDS];
    *digest *= UINT64_C(0x100000001b3);
}


int
main(void)
{
    uint64_t digest = UINT64_C(0xcbf29ce484222325);
    ctint1728_32_t a = {{0}};
    ctint1728_32_t b = {{0}};
    test_value_pair(&a, &b, 0, &digest);
    a.words[0] = 1;
    for (size_t i = 0; i < CTINT1728_WORDS; ++i) {
        b.words[i] = UINT32_MAX;
    }
    test_value_pair(&a, &b, 1, &digest);
    memset(&a, 0, sizeof(a));
    a.words[CTINT1728_WORDS - 1] = UINT32_C(0x80000000);
    test_value_pair(&a, &b, 2, &digest);

    for (size_t case_id = 3; case_id < 10003; ++case_id) {
        random_ctint(&a);
        random_ctint(&b);
        test_value_pair(&a, &b, case_id, &digest);
        test_magnitude_shift(a, case_id, &digest);
        test_bounded_multiplication(case_id, &digest);
    }

    for (size_t case_id = 0; case_id < 512; ++case_id) {
        if (case_id < 256) {
            random_ctint(&a);
            random_ctint(&b);
        } else if (case_id < 384) {
            random_bounded_ctint(&a);
            random_bounded_ctint(&b);
        } else {
            random_ctint(&a);
            memset(&b, 0, sizeof(b));
            b.words[0] = (uint32_t)next_u64() | 1u;
            b.words[1] = (uint32_t)(next_u64() >> 32);
            if ((next_u64() & 1u) != 0) {
                negate_ctint(&b);
            }
        }
        test_division_differential(a, b, case_id, &digest);
    }

    ctint1728_32_t exponent;
    ctint1728_32_t modulus;
    set_i64_ctint(&a, -17);
    set_u64_ctint(&exponent, 117);
    set_u64_ctint(&modulus, 97);
    test_pow_differential(&a, &exponent, &modulus, 0, &digest);

    set_i64_ctint(&a, 5);
    memset(&exponent, 0, sizeof(exponent));
    exponent.words[16] = UINT32_C(1) << 8;
    set_u64_ctint(&modulus, 19);
    test_pow_differential(&a, &exponent, &modulus, 1, &digest);

    set_i64_ctint(&a, 5);
    set_u64_ctint(&exponent, 1);
    memset(&modulus, 0, sizeof(modulus));
    modulus.words[26] = UINT32_C(1) << 30;
    test_pow_differential(&a, &exponent, &modulus, 2, &digest);

    set_u64_ctint(&a, 0);
    test_sqrt_differential(&a, 0, &digest);
    set_u64_ctint(&a, 1);
    test_sqrt_differential(&a, 1, &digest);
    set_u64_ctint(&a, 2);
    test_sqrt_differential(&a, 2, &digest);
    set_i64_ctint(&a, -1);
    test_sqrt_differential(&a, 3, &digest);

    ibz_t ibz_root;
    ibz_t ibz_square;
    ibz_t ibz_one;
    ibz_set(&ibz_one, 1);
    for (size_t case_id = 0; case_id < 16; ++case_id) {
        ctint1728_32_t root = {{0}};
        if (case_id == 0) {
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
        ctint_to_ibz(&ibz_root, &root);
        ibz_mul(&ibz_square, &ibz_root, &ibz_root);
        ibz_to_ctint(&a, &ibz_square);
        test_sqrt_differential(&a, 4u + 2u * case_id, &digest);
        ibz_add(&ibz_square, &ibz_square, &ibz_one);
        ibz_to_ctint(&a, &ibz_square);
        test_sqrt_differential(&a, 5u + 2u * case_id, &digest);
    }

    /* Cornacchia half-GCD uses a variable-time ibz Euclidean loop only as
     * the independent oracle.  The first cases exercise small exact
     * boundaries (including a square modulus); the remaining cases use a
     * full 492-bit positive modulus and a 480-bit root, so root < modulus is
     * guaranteed without sharing reduction logic with the implementation
     * under test. */
    set_u64_ctint(&modulus, 61);
    set_u64_ctint(&a, 11);
    test_halfgcd_differential(&modulus, &a, 0, &digest);
    set_u64_ctint(&modulus, 7349);
    set_u64_ctint(&a, 2061);
    test_halfgcd_differential(&modulus, &a, 1, &digest);
    set_u64_ctint(&modulus, 3);
    set_u64_ctint(&a, 0);
    test_halfgcd_differential(&modulus, &a, 2, &digest);
    set_u64_ctint(&modulus, 49);
    set_u64_ctint(&a, 7);
    test_halfgcd_differential(&modulus, &a, 3, &digest);
    for (size_t case_id = 4; case_id < 16; ++case_id) {
        memset(&modulus, 0, sizeof(modulus));
        memset(&a, 0, sizeof(a));
        for (size_t i = 0; i < 16; i += 2) {
            const uint64_t modulus_word = next_u64();
            modulus.words[i] = (uint32_t)modulus_word;
            modulus.words[i + 1] = (uint32_t)(modulus_word >> 32);
            if (i < 14) {
                const uint64_t root_word = next_u64();
                a.words[i] = (uint32_t)root_word;
                a.words[i + 1] = (uint32_t)(root_word >> 32);
            }
        }
        modulus.words[15] &= UINT32_C(0x00000fff);
        modulus.words[15] |= UINT32_C(0x00000800);
        modulus.words[0] |= 1u;
        test_halfgcd_differential(&modulus, &a, case_id, &digest);
    }
    ibz_t fibonacci_previous;
    ibz_t fibonacci_current;
    ibz_t fibonacci_next;
    ibz_set(&fibonacci_previous, 1);
    ibz_set(&fibonacci_current, 1);
    while (ibz_bitsize(&fibonacci_current) <
           (int)CTINT1728_CORNACCHIA_MAX_BITS) {
        ibz_add(&fibonacci_next,
                &fibonacci_previous,
                &fibonacci_current);
        ibz_copy(&fibonacci_previous, &fibonacci_current);
        ibz_copy(&fibonacci_current, &fibonacci_next);
    }
    ibz_to_ctint(&modulus, &fibonacci_current);
    ibz_to_ctint(&a, &fibonacci_previous);
    test_halfgcd_differential(&modulus, &a, 16, &digest);

    printf("ctint1728/ibz PASS: random_pairs=10000 bounded_products=10000 "
           "divisions=512 reductions=512 pow_cases=3 sqrt_cases=36 "
           "halfgcd_cases=17 random_shifts=10000 "
           "digest=%016" PRIx64 "\n",
           digest);
    return 0;
}
