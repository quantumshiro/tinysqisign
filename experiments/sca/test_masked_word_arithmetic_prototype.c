#include "masked_word_arithmetic_prototype.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static uint64_t prng_state = UINT64_C(0x6a09e667f3bcc909);

static uint64_t
next_u64(void)
{
    uint64_t z = (prng_state += UINT64_C(0x9e3779b97f4a7c15));
    z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
    return z ^ (z >> 31);
}

static void
fail(const char *operation, uint64_t left, uint64_t right,
     uint64_t expected, uint64_t observed)
{
    fprintf(stderr,
            "masked-word prototype FAIL: %s left=%016" PRIx64
            " right=%016" PRIx64 " expected=%016" PRIx64
            " observed=%016" PRIx64 "\n",
            operation, left, right, expected, observed);
    exit(1);
}

static uint64_t
recombine(const sca_masked_word_pair_t *value)
{
    return sca_masked_word_to_u64(sca_masked_word_recombine(value));
}

static void
check_case(uint64_t left_value, uint64_t right_value)
{
    sca_masked_word_pair_t left;
    sca_masked_word_pair_t right;
    sca_masked_word_pair_t out;
    sca_masked_word_pair_t alias;
    uint64_t left_mask = next_u64();
    uint64_t right_mask = next_u64();
    uint64_t gadget_mask = next_u64();
    uint64_t refresh_mask = next_u64();
    uint64_t observed;

    sca_masked_word_share(&left, sca_masked_word_from_u64(left_value),
                          sca_masked_word_from_u64(left_mask));
    sca_masked_word_share(&right, sca_masked_word_from_u64(right_value),
                          sca_masked_word_from_u64(right_mask));
    if (recombine(&left) != left_value || recombine(&right) != right_value) {
        fail("share", left_value, right_value, left_value, recombine(&left));
    }

    sca_masked_word_add(&out, &left, &right);
    observed = recombine(&out);
    if (observed != left_value + right_value) {
        fail("add", left_value, right_value, left_value + right_value,
             observed);
    }

    alias = left;
    sca_masked_word_add(&alias, &alias, &right);
    if (recombine(&alias) != left_value + right_value) {
        fail("add-left-alias", left_value, right_value,
             left_value + right_value, recombine(&alias));
    }

    alias = right;
    sca_masked_word_add(&alias, &left, &alias);
    if (recombine(&alias) != left_value + right_value) {
        fail("add-right-alias", left_value, right_value,
             left_value + right_value, recombine(&alias));
    }

    sca_masked_word_mul(&out, &left, &right,
                        sca_masked_word_from_u64(gadget_mask));
    observed = recombine(&out);
    if (observed != left_value * right_value) {
        fail("multiply", left_value, right_value, left_value * right_value,
             observed);
    }

    alias = left;
    sca_masked_word_mul(&alias, &alias, &right,
                        sca_masked_word_from_u64(gadget_mask));
    if (recombine(&alias) != left_value * right_value) {
        fail("multiply-left-alias", left_value, right_value,
             left_value * right_value, recombine(&alias));
    }

    alias = right;
    sca_masked_word_mul(&alias, &left, &alias,
                        sca_masked_word_from_u64(gadget_mask));
    if (recombine(&alias) != left_value * right_value) {
        fail("multiply-right-alias", left_value, right_value,
             left_value * right_value, recombine(&alias));
    }

    sca_masked_word_refresh(&out, &left,
                            sca_masked_word_from_u64(refresh_mask));
    if (recombine(&out) != left_value) {
        fail("refresh", left_value, right_value, left_value, recombine(&out));
    }
    if (refresh_mask != 0 &&
        out.share[0].limb[0] == left.share[0].limb[0] &&
        out.share[0].limb[1] == left.share[0].limb[1]) {
        fail("refresh-no-change", left_value, right_value,
             refresh_mask, 0);
    }

    alias = left;
    sca_masked_word_refresh(&alias, &alias,
                            sca_masked_word_from_u64(refresh_mask));
    if (recombine(&alias) != left_value) {
        fail("refresh-alias", left_value, right_value, left_value,
             recombine(&alias));
    }
}

int
main(void)
{
    static const uint64_t edges[] = {
        UINT64_C(0), UINT64_C(1), UINT64_C(2), UINT64_C(0xffffffff),
        UINT64_C(0x100000000), UINT64_C(0x7fffffffffffffff),
        UINT64_C(0x8000000000000000), UINT64_MAX,
    };
    uint64_t checksum = 0;

    for (size_t i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        for (size_t j = 0; j < sizeof(edges) / sizeof(edges[0]); j++) {
            check_case(edges[i], edges[j]);
            checksum ^= edges[i] * UINT64_C(0x9e3779b97f4a7c15) + edges[j];
        }
    }
    for (unsigned i = 0; i < 200000; i++) {
        uint64_t left = next_u64();
        uint64_t right = next_u64();
        check_case(left, right);
        checksum ^= left + (right << (i & 31));
    }

    printf("masked-word arithmetic prototype PASS: 64 edge pairs + 200000 "
           "deterministic random pairs; checksum=%016" PRIx64 "\n",
           checksum);
    return 0;
}
