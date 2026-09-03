#ifndef SQISIGN_EXPERIMENT_MASKED_WORD_REFRESH_M33_H
#define SQISIGN_EXPERIMENT_MASKED_WORD_REFRESH_M33_H

#include <stdint.h>

/*
 * Research-only Cortex-M33 arithmetic-share refresh.
 *
 * Input and output are two arithmetic shares modulo 2^32.  fresh_mask must
 * be a freshly sampled uniform 32-bit value.  zero_scratch must point to one
 * writable 32-bit word disjoint from input and output; the primitive writes
 * zero there to precharge the modeled memory path.
 *
 * The frozen assembly uses dummy zero loads/stores and clears share-holding
 * registers, but this is only an abstract transition-model countermeasure.
 * It is not approved for production and requires physical Cortex-M33 TVLA and
 * attack validation.
 */
void sca_masked_word_refresh32_m33(uint32_t output_shares[2],
                                   const uint32_t input_shares[2],
                                   uint32_t fresh_mask,
                                   uint32_t *zero_scratch);

#endif
