#include "masked_word_arithmetic_prototype.h"

static sca_masked_word_u64_t
word_add(sca_masked_word_u64_t left, sca_masked_word_u64_t right)
{
    sca_masked_word_u64_t out;
    uint64_t low = (uint64_t) left.limb[0] + right.limb[0];

    out.limb[0] = (uint32_t) low;
    out.limb[1] = left.limb[1] + right.limb[1] + (uint32_t) (low >> 32);
    return out;
}

static sca_masked_word_u64_t
word_neg(sca_masked_word_u64_t value)
{
    sca_masked_word_u64_t out;
    uint64_t low = (uint64_t) (~value.limb[0]) + 1u;

    out.limb[0] = (uint32_t) low;
    out.limb[1] = ~value.limb[1] + (uint32_t) (low >> 32);
    return out;
}

static sca_masked_word_u64_t
word_mul_low(sca_masked_word_u64_t left, sca_masked_word_u64_t right)
{
    sca_masked_word_u64_t out;
    uint64_t p00 = (uint64_t) left.limb[0] * right.limb[0];
    uint64_t p01 = (uint64_t) left.limb[0] * right.limb[1];
    uint64_t p10 = (uint64_t) left.limb[1] * right.limb[0];

    out.limb[0] = (uint32_t) p00;
    out.limb[1] = (uint32_t) (p00 >> 32) +
                  (uint32_t) p01 + (uint32_t) p10;
    return out;
}

sca_masked_word_u64_t
sca_masked_word_from_u64(uint64_t value)
{
    sca_masked_word_u64_t out = {
        .limb = {(uint32_t) value, (uint32_t) (value >> 32)},
    };
    return out;
}

uint64_t
sca_masked_word_to_u64(sca_masked_word_u64_t value)
{
    return (uint64_t) value.limb[0] | ((uint64_t) value.limb[1] << 32);
}

void
sca_masked_word_share(sca_masked_word_pair_t *out,
                      sca_masked_word_u64_t value,
                      sca_masked_word_u64_t fresh_mask)
{
    out->share[0] = fresh_mask;
    out->share[1] = word_add(value, word_neg(fresh_mask));
}

sca_masked_word_u64_t
sca_masked_word_recombine(const sca_masked_word_pair_t *value)
{
    return word_add(value->share[0], value->share[1]);
}

void
sca_masked_word_refresh(sca_masked_word_pair_t *out,
                        const sca_masked_word_pair_t *value,
                        sca_masked_word_u64_t fresh_mask)
{
    sca_masked_word_u64_t share0 = value->share[0];
    sca_masked_word_u64_t share1 = value->share[1];

    out->share[0] = word_add(share0, fresh_mask);
    out->share[1] = word_add(share1, word_neg(fresh_mask));
}

void
sca_masked_word_add(sca_masked_word_pair_t *out,
                    const sca_masked_word_pair_t *left,
                    const sca_masked_word_pair_t *right)
{
    sca_masked_word_u64_t left0 = left->share[0];
    sca_masked_word_u64_t left1 = left->share[1];
    sca_masked_word_u64_t right0 = right->share[0];
    sca_masked_word_u64_t right1 = right->share[1];

    out->share[0] = word_add(left0, right0);
    out->share[1] = word_add(left1, right1);
}

void
sca_masked_word_mul(sca_masked_word_pair_t *out,
                    const sca_masked_word_pair_t *left,
                    const sca_masked_word_pair_t *right,
                    sca_masked_word_u64_t fresh_mask)
{
    sca_masked_word_u64_t left0 = left->share[0];
    sca_masked_word_u64_t left1 = left->share[1];
    sca_masked_word_u64_t right0 = right->share[0];
    sca_masked_word_u64_t right1 = right->share[1];
    sca_masked_word_u64_t product00 = word_mul_low(left0, right0);
    sca_masked_word_u64_t product11 = word_mul_low(left1, right1);
    sca_masked_word_u64_t cross01 = word_mul_low(left0, right1);
    sca_masked_word_u64_t cross10 = word_mul_low(left1, right0);
    sca_masked_word_u64_t cross_masked = word_neg(fresh_mask);

    cross_masked = word_add(cross_masked, cross01);
    cross_masked = word_add(cross_masked, cross10);
    out->share[0] = word_add(product00, fresh_mask);
    out->share[1] = word_add(product11, cross_masked);
}
