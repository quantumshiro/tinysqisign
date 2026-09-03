// SPDX-License-Identifier: Apache-2.0

#include "random_ideal_fixed_budget.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static uint64_t state = UINT64_C(0x6a09e667f3bcc909);


static void
fail(const char *label, size_t case_id)
{
    fprintf(stderr, "random-ideal fixed-budget FAIL: %s case=%zu\n",
            label, case_id);
    exit(1);
}


static uint64_t
next_u64(void)
{
    uint64_t value = state;
    value ^= value >> 12;
    value ^= value << 25;
    value ^= value >> 27;
    state = value;
    return value * UINT64_C(0x2545f4914f6cdd1d);
}


static int
all_zero(const void *object, size_t bytes)
{
    const uint8_t *cursor = object;
    uint8_t aggregate = 0;
    for (size_t index = 0; index < bytes; ++index)
        aggregate |= cursor[index];
    return aggregate == 0;
}


static void
set_u64(ctint1728_32_t *value, uint64_t word)
{
    memset(value, 0, sizeof(*value));
    value->words[0] = (uint32_t)word;
    value->words[1] = (uint32_t)(word >> 32);
}


static uint64_t
get_u64(const ctint1728_32_t *value)
{
    for (size_t index = 2; index < CTINT1728_WORDS; ++index) {
        if (value->words[index] != 0)
            return UINT64_MAX;
    }
    return (uint64_t)value->words[0] |
           ((uint64_t)value->words[1] << 32);
}


static int64_t
get_i64(const ctint1728_32_t *value)
{
    const uint32_t extension =
        (value->words[1] >> 31) != 0 ? UINT32_MAX : UINT32_C(0);
    for (size_t index = 2; index < CTINT1728_WORDS; ++index) {
        if (value->words[index] != extension)
            return INT64_MIN;
    }
    return (int64_t)((uint64_t)value->words[0] |
                     ((uint64_t)value->words[1] << 32));
}


static int
same_value(const ctint1728_32_t *a, const ctint1728_32_t *b)
{
    return memcmp(a, b, sizeof(*a)) == 0;
}


static void
set_level1_modulus(ctint1728_32_t *value)
{
    memset(value, 0, sizeof(*value));
    value->words[0] = 75;
    value->words[16] = 1;
}


static void
set_words(ctint1728_32_t *value, const uint32_t *words, size_t count)
{
    memset(value, 0, sizeof(*value));
    memcpy(value->words, words, count * sizeof(words[0]));
}


static int64_t
small_form_signed(const int64_t x[4], int64_t coefficient)
{
    return x[0] * x[0] + x[1] * x[1] +
           coefficient * (x[2] * x[2] + x[3] * x[3]);
}


static uint64_t
small_form(const uint64_t x[4], uint64_t coefficient)
{
    const int64_t signed_x[4] = {
        (int64_t)x[0], (int64_t)x[1], (int64_t)x[2], (int64_t)x[3]
    };
    return (uint64_t)small_form_signed(signed_x, (int64_t)coefficient);
}


static void
test_wide_reduction(void)
{
    ctint1728_32_t modulus;
    ctint1728_32_t output;
    sqisign_ri_coordinate_workspace_t workspace;
    uint8_t block[SQISIGN_RI_WIDE_BYTES];

    set_u64(&modulus, UINT64_C(2305843009213693951));
    for (size_t case_id = 0; case_id < 256; ++case_id) {
        for (size_t index = 0; index < sizeof(block); index += 8) {
            const uint64_t word = next_u64();
            const size_t remaining = sizeof(block) - index;
            memcpy(&block[index], &word, remaining < 8 ? remaining : 8);
        }
        memset(&output, 0xa5, sizeof(output));
        if (!sqisign_ri_reduce_wide_coordinate(
                &output, block, &modulus, &workspace))
            fail("wide-status", case_id);

        uint64_t reference = 0;
        for (size_t index = SQISIGN_RI_WIDE_BYTES; index-- > 0;) {
            const uint8_t byte = index == SQISIGN_RI_WIDE_BYTES - 1
                                     ? (uint8_t)(block[index] & 1u)
                                     : block[index];
            reference = (uint64_t)(((__uint128_t)reference * 256u + byte) %
                                   UINT64_C(2305843009213693951));
        }
        if (get_u64(&output) != reference ||
            !all_zero(&workspace, sizeof(workspace)))
            fail("wide-value", case_id);
    }
}


static void
test_level1_wide_vectors(void)
{
    static const uint32_t expected_n_minus_one[17] = {
        UINT32_C(0x0000004a), 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, UINT32_C(0x00000001),
    };
    static const uint32_t expected_max[17] = {
        UINT32_C(0x0000004a), 0, 0, 0, 0, 0, 0, 0,
        UINT32_C(0xffffff6a), UINT32_C(0xffffffff),
        UINT32_C(0xffffffff), UINT32_C(0xffffffff),
        UINT32_C(0xffffffff), UINT32_C(0xffffffff),
        UINT32_C(0xffffffff), UINT32_C(0xffffffff), 0,
    };
    ctint1728_32_t modulus;
    ctint1728_32_t expected;
    ctint1728_32_t output;
    sqisign_ri_coordinate_workspace_t workspace;
    uint8_t block[SQISIGN_RI_WIDE_BYTES];

    set_level1_modulus(&modulus);
    for (size_t vector = 0; vector < 4; ++vector) {
        memset(block, 0, sizeof(block));
        memset(&expected, 0, sizeof(expected));
        if (vector == 1) {
            block[0] = 74;
            block[64] = 1;
            set_words(&expected, expected_n_minus_one, 17);
        } else if (vector == 2) {
            block[0] = 75;
            block[64] = 1;
        } else if (vector == 3) {
            memset(block, 0xff, sizeof(block));
            set_words(&expected, expected_max, 17);
        }
        memset(&output, 0xa5, sizeof(output));
        if (!sqisign_ri_reduce_wide_coordinate(
                &output, block, &modulus, &workspace) ||
            !same_value(&output, &expected) ||
            !all_zero(&workspace, sizeof(workspace)))
            fail("level1-wide-vector", vector);
    }

    /* Bits 769..775 are outside the sampler domain and must not affect the
     * decoded integer. */
    memset(block, 0, sizeof(block));
    block[SQISIGN_RI_WIDE_BYTES - 1] = 0xfe;
    memset(&output, 0xa5, sizeof(output));
    if (!sqisign_ri_reduce_wide_coordinate(
            &output, block, &modulus, &workspace) ||
        !ctint1728_32_is_zero(&output))
        fail("level1-wide-high-mask", 4);
}


static void
test_public_failure_contract(void)
{
    ctint1728_32_t modulus;
    ctint1728_32_t coefficient;
    ctint1728_32_t output;
    ctint1728_32_t sentinel;
    ctint1728_32_t coordinates[4];
    uint8_t block[SQISIGN_RI_WIDE_BYTES] = {0};
    sqisign_ri_coordinate_workspace_t coordinate_workspace;
    sqisign_ri_sqrt_workspace_t sqrt_workspace;
    sqisign_ri_primitive_workspace_t primitive_workspace;

    set_level1_modulus(&modulus);
    set_u64(&coefficient, 1);
    set_u64(&output, UINT64_C(0xa5a55a5adeadbeef));
    ctint1728_32_copy(&sentinel, &output);
    for (size_t index = 0; index < 4; ++index)
        set_u64(&coordinates[index], index + 1u);

    memset(&coordinate_workspace, 0xa5, sizeof(coordinate_workspace));
    if (sqisign_ri_reduce_wide_coordinate(
            &output, NULL, &modulus, &coordinate_workspace) ||
        !same_value(&output, &sentinel) ||
        !all_zero(&coordinate_workspace, sizeof(coordinate_workspace)))
        fail("wide-null-block", 0);
    memset(&coordinate_workspace, 0xa5, sizeof(coordinate_workspace));
    if (sqisign_ri_reduce_wide_coordinate(
            &output, block, NULL, &coordinate_workspace) ||
        !same_value(&output, &sentinel) ||
        !all_zero(&coordinate_workspace, sizeof(coordinate_workspace)))
        fail("wide-null-modulus", 1);
    memset(&coordinate_workspace, 0xa5, sizeof(coordinate_workspace));
    if (sqisign_ri_reduce_wide_coordinate(
            NULL, block, &modulus, &coordinate_workspace) ||
        !all_zero(&coordinate_workspace, sizeof(coordinate_workspace)))
        fail("wide-null-output", 2);

    memset(&sqrt_workspace, 0xa5, sizeof(sqrt_workspace));
    if (sqisign_ri_sqrt_mod_level1(NULL, &modulus, &sqrt_workspace) ||
        !all_zero(&sqrt_workspace, sizeof(sqrt_workspace)))
        fail("sqrt-null-output", 3);
    memset(&sqrt_workspace, 0xa5, sizeof(sqrt_workspace));
    if (sqisign_ri_sqrt_mod_level1(
            &output, NULL, &sqrt_workspace) ||
        !same_value(&output, &sentinel) ||
        !all_zero(&sqrt_workspace, sizeof(sqrt_workspace)))
        fail("sqrt-null-input", 4);

    memset(&primitive_workspace, 0xa5, sizeof(primitive_workspace));
    if (sqisign_ri_correct_primitive_lift(
            NULL, coordinates, &modulus, &coefficient,
            &primitive_workspace) ||
        !all_zero(&primitive_workspace, sizeof(primitive_workspace)))
        fail("primitive-null-output", 5);
    memset(&primitive_workspace, 0xa5, sizeof(primitive_workspace));
    if (sqisign_ri_correct_primitive_lift(
            coordinates, NULL, &modulus, &coefficient,
            &primitive_workspace) ||
        !all_zero(&primitive_workspace, sizeof(primitive_workspace)))
        fail("primitive-null-input", 6);
    memset(&primitive_workspace, 0xa5, sizeof(primitive_workspace));
    if (sqisign_ri_correct_primitive_lift(
            coordinates, coordinates, &modulus, NULL,
            &primitive_workspace) ||
        !all_zero(&primitive_workspace, sizeof(primitive_workspace)))
        fail("primitive-null-coefficient", 7);
}


static void
test_level1_square_root(void)
{
    ctint1728_32_t modulus;
    ctint1728_32_t input;
    ctint1728_32_t output;
    ctint1728_32_t sentinel;
    ctint1728_32_t x;
    ctint1728_32_t alternate;
    ctint1728_32_arith_workspace_t arithmetic;
    sqisign_ri_sqrt_workspace_t workspace;

    set_level1_modulus(&modulus);

    /* Zero is a valid root and must still execute the complete pow/check
     * schedule. */
    set_u64(&input, 0);
    set_u64(&output, UINT64_C(0x1234));
    if (!sqisign_ri_sqrt_mod_level1(&output, &input, &workspace) ||
        !ctint1728_32_is_zero(&output) ||
        !all_zero(&workspace, sizeof(workspace)))
        fail("sqrt-zero", 0);

    for (size_t case_id = 0; case_id < 8; ++case_id) {
        const uint64_t word = (next_u64() & UINT64_C(0x3fffffff)) + 1;
        const uint64_t square = word * word;
        set_u64(&x, word);
        set_u64(&input, square);
        if (case_id == 0) {
            if (!sqisign_ri_sqrt_mod_level1(&input, &input, &workspace))
                fail("sqrt-alias-status", case_id);
            ctint1728_32_copy(&output, &input);
        } else {
            memset(&output, 0xa5, sizeof(output));
            if (!sqisign_ri_sqrt_mod_level1(
                    &output, &input, &workspace))
                fail("sqrt-residue-status", case_id);
        }
        if (!ctint1728_32_sub(
                &alternate, &modulus, &x, &arithmetic))
            fail("sqrt-reference-sub", case_id);
        if ((!same_value(&output, &x) &&
             !same_value(&output, &alternate)) ||
            !all_zero(&workspace, sizeof(workspace)))
            fail("sqrt-residue-value", case_id);
    }

    /* N == 3 mod 8, so 2 is a quadratic nonresidue. Multiplying any nonzero
     * square by two therefore gives a deterministic no-root fixture. */
    for (size_t case_id = 0; case_id < 8; ++case_id) {
        const uint64_t word = (next_u64() & UINT64_C(0x1fffffff)) + 1;
        set_u64(&input, 2u * word * word);
        set_u64(&output, UINT64_C(0x55aa0000) + case_id);
        ctint1728_32_copy(&sentinel, &output);
        if (sqisign_ri_sqrt_mod_level1(&output, &input, &workspace) ||
            !same_value(&output, &sentinel) ||
            !all_zero(&workspace, sizeof(workspace)))
            fail("sqrt-nonresidue", case_id);
    }
}


static size_t
test_primitive_lift(void)
{
    size_t case_id = 0;
    size_t corrected_cases = 0;
    for (uint64_t modulus_word = 3; modulus_word <= 11; modulus_word += 2) {
        if (modulus_word == 9)
            continue;
        for (uint64_t coefficient_word = 1;
             coefficient_word < modulus_word && coefficient_word <= 3;
             ++coefficient_word) {
            for (uint64_t x0 = 0; x0 < modulus_word; ++x0)
            for (uint64_t x1 = 0; x1 < modulus_word; ++x1)
            for (uint64_t x2 = 0; x2 < modulus_word; ++x2)
            for (uint64_t x3 = 0; x3 < modulus_word; ++x3) {
                const uint64_t input_words[4] = {x0, x1, x2, x3};
                const uint64_t value = small_form(input_words, coefficient_word);
                if (value % (modulus_word * modulus_word) != 0 ||
                    (x0 | x1 | x2 | x3) == 0)
                    continue;

                ctint1728_32_t modulus;
                ctint1728_32_t coefficient;
                ctint1728_32_t input[4];
                ctint1728_32_t output[4];
                sqisign_ri_primitive_workspace_t workspace;
                set_u64(&modulus, modulus_word);
                set_u64(&coefficient, coefficient_word);
                for (size_t index = 0; index < 4; ++index) {
                    set_u64(&input[index], input_words[index]);
                    memset(&output[index], 0xa5, sizeof(output[index]));
                }
                if (!sqisign_ri_correct_primitive_lift(
                        output, input, &modulus, &coefficient, &workspace))
                    fail("primitive-status", case_id);
                int64_t output_words[4];
                for (size_t index = 0; index < 4; ++index) {
                    output_words[index] = get_i64(&output[index]);
                    if (output_words[index] == INT64_MIN ||
                        (output_words[index] - (int64_t)input_words[index]) %
                                (int64_t)modulus_word != 0)
                        fail("primitive-residue", case_id);
                }
                const int64_t corrected =
                    small_form_signed(output_words, (int64_t)coefficient_word);
                if (corrected % modulus_word != 0 ||
                    corrected % (modulus_word * modulus_word) == 0 ||
                    !all_zero(&workspace, sizeof(workspace)))
                    fail("primitive-value", case_id);
                corrected_cases++;
                case_id++;
                if (corrected_cases == 64)
                    goto correction_done;
            }
        }
    }
correction_done:
    if (corrected_cases != 64)
        fail("primitive-coverage", corrected_cases);

    /* The public API promises exact input/output aliasing. */
    {
        ctint1728_32_t modulus;
        ctint1728_32_t coefficient;
        ctint1728_32_t aliased[4];
        sqisign_ri_primitive_workspace_t workspace;
        const uint64_t words[4] = {3, 4, 0, 0};
        const int64_t expected[4] = {-2, 4, 0, 0};
        set_u64(&modulus, 5);
        set_u64(&coefficient, 1);
        for (size_t index = 0; index < 4; ++index)
            set_u64(&aliased[index], words[index]);
        if (!sqisign_ri_correct_primitive_lift(
                aliased, aliased, &modulus, &coefficient, &workspace))
            fail("primitive-alias-status", case_id);
        for (size_t index = 0; index < 4; ++index) {
            if (get_i64(&aliased[index]) != expected[index])
                fail("primitive-alias-value", case_id);
        }
        if (!all_zero(&workspace, sizeof(workspace)))
            fail("primitive-alias-clear", case_id);
        case_id++;
    }

    /* A primitive isotropic input must remain byte-identical. */
    {
        ctint1728_32_t modulus;
        ctint1728_32_t coefficient;
        ctint1728_32_t input[4];
        ctint1728_32_t output[4];
        sqisign_ri_primitive_workspace_t workspace;
        set_u64(&modulus, 5);
        set_u64(&coefficient, 1);
        const uint64_t words[4] = {1, 2, 0, 0};
        for (size_t index = 0; index < 4; ++index) {
            set_u64(&input[index], words[index]);
            set_u64(&output[index], UINT64_C(0x55aa) + index);
        }
        if (!sqisign_ri_correct_primitive_lift(
                output, input, &modulus, &coefficient, &workspace))
            fail("primitive-unchanged-status", case_id);
        for (size_t index = 0; index < 4; ++index) {
            if (!same_value(&output[index], &input[index]))
                fail("primitive-unchanged-value", case_id);
        }
        if (!all_zero(&workspace, sizeof(workspace)))
            fail("primitive-unchanged-clear", case_id);
        case_id++;
    }

    /* Zero and non-isotropic inputs fail without publishing. */
    for (size_t invalid = 0; invalid < 2; ++invalid) {
        ctint1728_32_t modulus;
        ctint1728_32_t coefficient;
        ctint1728_32_t input[4];
        ctint1728_32_t output[4];
        ctint1728_32_t sentinel[4];
        sqisign_ri_primitive_workspace_t workspace;
        set_u64(&modulus, 5);
        set_u64(&coefficient, 1);
        for (size_t index = 0; index < 4; ++index) {
            set_u64(&input[index], 0);
            set_u64(&output[index], UINT64_C(0xdead) + index);
            ctint1728_32_copy(&sentinel[index], &output[index]);
        }
        if (invalid != 0)
            set_u64(&input[0], 1);
        if (sqisign_ri_correct_primitive_lift(
                output, input, &modulus, &coefficient, &workspace))
            fail("primitive-invalid-status", case_id);
        for (size_t index = 0; index < 4; ++index) {
            if (!same_value(&output[index], &sentinel[index]))
                fail("primitive-invalid-publication", case_id);
        }
        if (!all_zero(&workspace, sizeof(workspace)))
            fail("primitive-invalid-clear", case_id);
        case_id++;
    }
    return corrected_cases;
}


int
main(void)
{
    test_wide_reduction();
    test_level1_wide_vectors();
    test_public_failure_contract();
    test_level1_square_root();
    const size_t corrected = test_primitive_lift();
    printf("random-ideal fixed-budget components PASS: wide_random=256 "
           "wide_level1=5 primitive_corrected=%zu primitive_alias=1 "
           "sqrt_residue=9 sqrt_nonresidue=8 invalid=2 null_contract=8 "
           "gamma_budget=%u beta_budget=%u\n",
           corrected,
           (unsigned)SQISIGN_RI_GAMMA_CANDIDATES,
           (unsigned)SQISIGN_RI_BETA_CANDIDATES);
    return 0;
}
