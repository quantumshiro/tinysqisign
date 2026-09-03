#ifndef SQISIGN_EXPERIMENT_MASKED_WORD_ARITHMETIC_PROTOTYPE_H
#define SQISIGN_EXPERIMENT_MASKED_WORD_ARITHMETIC_PROTOTYPE_H

#include <stdint.h>

/*
 * Functional research oracle only.
 *
 * Values live in Z/(2^64) and are stored as two little-endian 32-bit limbs.
 * A masked value x has two arithmetic shares x = share[0] + share[1]
 * (mod 2^64).  The C implementation is intentionally not claimed to be
 * transition-safe: a compiler may place complementary shares consecutively
 * in a register, ALU port, load path, or store path.  Production integration
 * is forbidden until reviewed share-touching Cortex-M33 assembly and physical
 * leakage tests exist.
 */

typedef struct {
    uint32_t limb[2];
} sca_masked_word_u64_t;

typedef struct {
    sca_masked_word_u64_t share[2];
} sca_masked_word_pair_t;

sca_masked_word_u64_t sca_masked_word_from_u64(uint64_t value);
uint64_t sca_masked_word_to_u64(sca_masked_word_u64_t value);

void sca_masked_word_share(sca_masked_word_pair_t *out,
                           sca_masked_word_u64_t value,
                           sca_masked_word_u64_t fresh_mask);

sca_masked_word_u64_t
sca_masked_word_recombine(const sca_masked_word_pair_t *value);

void sca_masked_word_refresh(sca_masked_word_pair_t *out,
                             const sca_masked_word_pair_t *value,
                             sca_masked_word_u64_t fresh_mask);

void sca_masked_word_add(sca_masked_word_pair_t *out,
                         const sca_masked_word_pair_t *left,
                         const sca_masked_word_pair_t *right);

/*
 * Two-share ring multiplication with one fresh uniform 64-bit ring element:
 *
 *   c0 = a0*b0 + r
 *   c1 = a1*b1 + (-r + a0*b1 + a1*b0)       (mod 2^64)
 *
 * Locals make all documented output/input aliases functionally correct.  This
 * equation and the C code are not a composable probing proof or a physical
 * transition-security claim.
 */
void sca_masked_word_mul(sca_masked_word_pair_t *out,
                         const sca_masked_word_pair_t *left,
                         const sca_masked_word_pair_t *right,
                         sca_masked_word_u64_t fresh_mask);

#endif
