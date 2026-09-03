/*
 * Fixed-message, fixed-signing-RNG host timing screen for SQIsign v3.
 *
 * The generated v3_kat_subset.h supplies ten independently generated official
 * KAT key pairs.  randombytes_init() is reset before every signature, so key
 * identity is the only intentionally varying cryptographic input.  This is a
 * software timing screen, not evidence of physical side-channel resistance.
 */

#define _POSIX_C_SOURCE 200809L

#include <api.h>
#include <rng.h>

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "v3_kat_subset.h"

typedef struct {
    size_t sequence;
    unsigned key_index;
    uint64_t elapsed_ns;
    unsigned long long signed_message_length;
    uint64_t digest;
    int sign_rc;
    int verify_rc;
} timing_record_t;

static unsigned char active_public_key[CRYPTO_PUBLICKEYBYTES];
static unsigned char active_secret_key[CRYPTO_SECRETKEYBYTES];

static const unsigned char signing_seed[SQISIGN_V3_KAT_SEED_BYTES] = {
    0xa5, 0xa4, 0xa7, 0xa6, 0xa1, 0xa0, 0xa3, 0xa2,
    0xad, 0xac, 0xaf, 0xae, 0xa9, 0xa8, 0xab, 0xaa,
    0xb5, 0xb4, 0xb7, 0xb6, 0xb1, 0xb0, 0xb3, 0xb2,
    0xbd, 0xbc, 0xbf, 0xbe, 0xb9, 0xb8, 0xbb, 0xba,
    0x85, 0x84, 0x87, 0x86, 0x81, 0x80, 0x83, 0x82,
    0x8d, 0x8c, 0x8f, 0x8e, 0x89, 0x88, 0x8b, 0x8a,
};

static uint64_t
now_ns(void)
{
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0) {
        perror("clock_gettime");
        exit(2);
    }
    return (uint64_t)value.tv_sec * UINT64_C(1000000000) +
           (uint64_t)value.tv_nsec;
}

static uint32_t
xorshift32(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static uint64_t
fnv1a64(const unsigned char *data, size_t length)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    for (size_t i = 0; i < length; i++) {
        hash ^= data[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static unsigned long
parse_unsigned(const char *text, const char *name)
{
    char *end = NULL;
    errno = 0;
    unsigned long value = strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0') {
        fprintf(stderr, "invalid %s: %s\n", name, text);
        exit(2);
    }
    return value;
}

int
main(int argc, char **argv)
{
    if (argc != 5) {
        fprintf(stderr,
                "usage: %s LABEL PASS REPETITIONS SCHEDULE_SEED\n",
                argv[0]);
        return 2;
    }
    const char *label = argv[1];
    const char *pass = argv[2];
    const unsigned long repetitions = parse_unsigned(argv[3], "repetitions");
    uint32_t schedule_seed = (uint32_t)parse_unsigned(argv[4], "schedule seed");
    if (repetitions == 0 || repetitions > 10000 || schedule_seed == 0) {
        fprintf(stderr, "repetitions must be 1..10000 and schedule seed nonzero\n");
        return 2;
    }

    const size_t key_count = SQISIGN_V3_KAT_VECTOR_COUNT;
    const size_t sample_count = key_count * repetitions;
    unsigned *schedule = calloc(sample_count, sizeof(*schedule));
    timing_record_t *records = calloc(sample_count, sizeof(*records));
    if (schedule == NULL || records == NULL) {
        fprintf(stderr, "allocation failure\n");
        return 2;
    }
    for (size_t i = 0; i < sample_count; i++) {
        schedule[i] = (unsigned)(i % key_count);
    }
    for (size_t i = sample_count; i > 1; i--) {
        size_t j = xorshift32(&schedule_seed) % i;
        unsigned tmp = schedule[i - 1];
        schedule[i - 1] = schedule[j];
        schedule[j] = tmp;
    }

    const unsigned char *message = sqisign_v3_kat_vectors[0].message;
    const unsigned long long message_length =
        sqisign_v3_kat_vectors[0].message_length;
    unsigned char signed_message[CRYPTO_BYTES + SQISIGN_V3_KAT_MAX_MESSAGE_BYTES];
    unsigned char opened_message[SQISIGN_V3_KAT_MAX_MESSAGE_BYTES];

    /* One untimed call per key warms code/data without changing the fixed RNG. */
    for (size_t key = 0; key < key_count; key++) {
        unsigned char seed[sizeof(signing_seed)];
        unsigned long long signed_length = 0;
        memcpy(seed, signing_seed, sizeof(seed));
        memcpy(active_secret_key,
               sqisign_v3_kat_vectors[key].secret_key,
               sizeof(active_secret_key));
        randombytes_init(seed, NULL, 256);
        if (crypto_sign(signed_message,
                        &signed_length,
                        message,
                        message_length,
                        active_secret_key) != 0) {
            fprintf(stderr, "warmup Sign failed for key %zu\n", key);
            return 3;
        }
    }

    for (size_t sequence = 0; sequence < sample_count; sequence++) {
        const unsigned key = schedule[sequence];
        const sqisign_v3_kat_vector_t *vector = &sqisign_v3_kat_vectors[key];
        unsigned char seed[sizeof(signing_seed)];
        unsigned long long signed_length = 0;
        unsigned long long opened_length = 0;
        memcpy(seed, signing_seed, sizeof(seed));
        memcpy(active_secret_key, vector->secret_key, sizeof(active_secret_key));
        memcpy(active_public_key, vector->public_key, sizeof(active_public_key));
        randombytes_init(seed, NULL, 256);
        const uint64_t start = now_ns();
        const int sign_rc = crypto_sign(signed_message,
                                        &signed_length,
                                        message,
                                        message_length,
                                        active_secret_key);
        const uint64_t end = now_ns();
        int verify_rc = -999;
        if (sign_rc == 0) {
            verify_rc = crypto_sign_open(opened_message,
                                         &opened_length,
                                         signed_message,
                                         signed_length,
                                         active_public_key);
            if (verify_rc == 0 &&
                (opened_length != message_length ||
                 memcmp(opened_message, message, message_length) != 0)) {
                verify_rc = -998;
            }
        }
        records[sequence] = (timing_record_t){
            .sequence = sequence,
            .key_index = key,
            .elapsed_ns = end - start,
            .signed_message_length = signed_length,
            .digest = fnv1a64(signed_message, (size_t)signed_length),
            .sign_rc = sign_rc,
            .verify_rc = verify_rc,
        };
    }

    printf("implementation,pass,sequence,key_index,elapsed_ns,signed_message_length,signature_fnv1a64,sign_rc,verify_rc\n");
    for (size_t i = 0; i < sample_count; i++) {
        const timing_record_t *record = &records[i];
        printf("%s,%s,%zu,%u,%" PRIu64 ",%llu,%016" PRIx64 ",%d,%d\n",
               label,
               pass,
               record->sequence,
               record->key_index,
               record->elapsed_ns,
               record->signed_message_length,
               record->digest,
               record->sign_rc,
               record->verify_rc);
    }
    fprintf(stderr,
            "PASS implementation=%s pass=%s keys=%zu repetitions=%lu samples=%zu "
            "fixed_message_bytes=%llu kat_rsp_sha256=%s\n",
            label,
            pass,
            key_count,
            repetitions,
            sample_count,
            message_length,
            SQISIGN_V3_KAT_RSP_SHA256);
    free(records);
    free(schedule);
    return 0;
}
