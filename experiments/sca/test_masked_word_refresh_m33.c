#include "masked_word_refresh_m33.h"

#include <stdint.h>

volatile uint32_t sca_refresh_m33_test_checksum;

static uint32_t prng_state;

static uint32_t
next_u32(void)
{
    uint32_t value = prng_state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    prng_state = value;
    return value;
}

int
main(void)
{
    static const uint32_t edges[] = {
        UINT32_C(0), UINT32_C(1), UINT32_C(2),
        UINT32_C(0x7fffffff), UINT32_C(0x80000000), UINT32_MAX,
    };
    uint32_t checksum = 0;

    prng_state = UINT32_C(0x243f6a88);

    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        for (unsigned j = 0; j < sizeof(edges) / sizeof(edges[0]); j++) {
            uint32_t input[2] = {edges[j], edges[i] - edges[j]};
            uint32_t output[2] = {UINT32_C(0xa5a5a5a5),
                                  UINT32_C(0x5a5a5a5a)};
            uint32_t scratch = UINT32_C(0xffffffff);
            uint32_t mask = next_u32();
            sca_masked_word_refresh32_m33(output, input, mask, &scratch);
            if (output[0] != input[0] + mask ||
                output[1] != input[1] - mask ||
                output[0] + output[1] != edges[i] || scratch != 0) {
                return 1;
            }
            checksum ^= output[0] + (output[1] << (i & 15));
        }
    }

    for (unsigned i = 0; i < 10000; i++) {
        uint32_t value = next_u32();
        uint32_t share0 = next_u32();
        uint32_t input[2] = {share0, value - share0};
        uint32_t output[2] = {0, 0};
        uint32_t scratch = UINT32_C(0xffffffff);
        uint32_t mask = next_u32();
        sca_masked_word_refresh32_m33(output, input, mask, &scratch);
        if (output[0] != input[0] + mask ||
            output[1] != input[1] - mask ||
            output[0] + output[1] != value || scratch != 0) {
            return 2;
        }
        checksum ^= output[0] + (output[1] << (i & 15));
    }

    sca_refresh_m33_test_checksum = checksum;
    if (checksum != UINT32_C(0x6f653503)) {
        return 3;
    }
    return 0;
}
