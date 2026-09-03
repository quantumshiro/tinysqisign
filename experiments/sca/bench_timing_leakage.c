/*
 * Host-only leakage-screen harness for the frozen low-memory SQIsign path.
 *
 * This is not a constant-time test certificate.  It measures encoded Sign
 * with one fixed key and one fixed message while varying only the deterministic
 * signing RNG stream.  The companion control assigns the same RNG stream to
 * both statistical classes.  Per-invocation ML2 counters are emitted so that
 * timing variation can be compared with a known variable-time internal path.
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <encoded_sizes.h>
#include <rng.h>
#include <signature_lowmem.h>

#include "lll_internals.h"

enum
{
    MESSAGE_BYTES = 32
};

static const uint64_t KEYPAIR_SEED = UINT64_C(0x5343414b45594745);
static const uint64_t FIXED_SIGN_SEED = UINT64_C(0x5343415349474e31);
static const uint64_t RANDOM_SEED_BASE = UINT64_C(0x9e3779b97f4a7c15);

static const unsigned char message[MESSAGE_BYTES] = {
    0x53, 0x51, 0x49, 0x53, 0x69, 0x67, 0x6e, 0x20,
    0x44, 0x31, 0x32, 0x63, 0x20, 0x53, 0x43, 0x41,
    0x20, 0x74, 0x69, 0x6e, 0x69, 0x6e, 0x67, 0x20,
    0x73, 0x63, 0x72, 0x65, 0x65, 0x6e, 0x00, 0x01
};

static protocols_operation_workspace_t operation_workspace;
static unsigned char encoded_pk[PUBLICKEY_BYTES];
static unsigned char encoded_sk[SECRETKEY_BYTES];
static unsigned char signed_message[SIGNATURE_BYTES + MESSAGE_BYTES];
static unsigned char captured_a[SIGNATURE_BYTES + MESSAGE_BYTES];
static unsigned char captured_b[SIGNATURE_BYTES + MESSAGE_BYTES];
static unsigned char captured_warmup[SIGNATURE_BYTES + MESSAGE_BYTES];

typedef struct sample_result
{
    uint64_t elapsed_ns;
    uint64_t signature_digest;
    quat_ml2_profile_t ml2;
} sample_result_t;

static void
reset_rng(uint64_t value)
{
    unsigned char entropy[48];
    for (size_t i = 0; i < sizeof(entropy); i++) {
        unsigned int shift = (unsigned int)(8 * (i % sizeof(value)));
        entropy[i] = (unsigned char)((value >> shift) ^ (0xb7u * i));
    }
    randombytes_init(entropy, NULL, 256);
}

static uint64_t
splitmix64(uint64_t *state)
{
    uint64_t value = (*state += UINT64_C(0x9e3779b97f4a7c15));
    value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31);
}

static int
now_ns(uint64_t *result)
{
    struct timespec current;
    if (clock_gettime(CLOCK_MONOTONIC, &current) != 0)
        return 0;
    *result = (uint64_t)current.tv_sec * UINT64_C(1000000000) +
              (uint64_t)current.tv_nsec;
    return 1;
}

static uint64_t
fnv1a64(const unsigned char *input, size_t length)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    for (size_t i = 0; i < length; i++) {
        hash ^= input[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static int
verify_captured(const unsigned char *captured)
{
    return memcmp(captured + SIGNATURE_BYTES,
                  message,
                  sizeof(message)) == 0 &&
           sqisign_verify_with_workspace(message,
                                         sizeof(message),
                                         captured,
                                         SIGNATURE_BYTES,
                                         encoded_pk,
                                         &operation_workspace.verify) == 0;
}

static int
run_sign(uint64_t seed,
         sample_result_t *result,
         unsigned char *captured)
{
    unsigned long long signed_length = 0;
    uint64_t start;
    uint64_t end;

    reset_rng(seed);
    quat_ml2_profile_reset();
    if (!now_ns(&start))
        return 0;
    int status = sqisign_sign_with_workspace(signed_message,
                                             &signed_length,
                                             message,
                                             sizeof(message),
                                             encoded_sk,
                                             &operation_workspace.sign);
    if (!now_ns(&end))
        return 0;
    quat_ml2_profile_get(&result->ml2);

    if (status != 0 ||
        signed_length != SIGNATURE_BYTES + sizeof(message)) {
        return 0;
    }

    result->elapsed_ns = end - start;
    result->signature_digest =
        fnv1a64(signed_message, (size_t)signed_length);
    memcpy(captured, signed_message, (size_t)signed_length);
    return 1;
}

static uint64_t
profile_inputs(const quat_ml2_profile_t *profile)
{
    return profile->d4.inputs + profile->d8.inputs + profile->d16.inputs;
}

static uint64_t
profile_attempts(const quat_ml2_profile_t *profile)
{
    return profile->d4.underlying_attempts +
           profile->d8.underlying_attempts +
           profile->d16.underlying_attempts;
}

static uint64_t
profile_first_failures(const quat_ml2_profile_t *profile)
{
    return profile->d4.first_attempt_failures +
           profile->d8.first_attempt_failures +
           profile->d16.first_attempt_failures;
}

static void
print_sample(unsigned long sample,
             unsigned long pair,
             const char *class_name,
             const char *order,
             uint64_t seed,
             const sample_result_t *result)
{
    printf("%lu,%lu,%s,%s,%" PRIu64 ",%" PRIu64
           ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
           ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
           ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
           ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
           ",%016" PRIx64 "\n",
           sample,
           pair,
           class_name,
           order,
           seed,
           result->elapsed_ns,
           result->ml2.d4.inputs,
           result->ml2.d4.underlying_attempts,
           result->ml2.d4.first_attempt_failures,
           result->ml2.d8.inputs,
           result->ml2.d8.underlying_attempts,
           result->ml2.d8.first_attempt_failures,
           result->ml2.d16.inputs,
           result->ml2.d16.underlying_attempts,
           result->ml2.d16.first_attempt_failures,
           profile_inputs(&result->ml2),
           profile_attempts(&result->ml2),
           profile_first_failures(&result->ml2),
           result->signature_digest);
}

static int
parse_count(const char *text, unsigned long *result)
{
    char *end = NULL;
    unsigned long value = strtoul(text, &end, 10);
    if (end == text || *end != '\0' || value == 0)
        return 0;
    *result = value;
    return 1;
}

int
main(int argc, char **argv)
{
    const char *mode;
    unsigned long pairs;
    unsigned long warmups;
    int fixed_random;
    uint64_t fixed_digest = 0;
    unsigned long sample = 0;

    if (argc != 4 ||
        !parse_count(argv[2], &pairs) ||
        !parse_count(argv[3], &warmups) ||
        (pairs & 1u) != 0) {
        fprintf(stderr,
                "usage: %s {control|fixed-random} EVEN_PAIRS WARMUPS\n",
                argv[0]);
        return 2;
    }
    mode = argv[1];
    if (strcmp(mode, "control") == 0) {
        fixed_random = 0;
    } else if (strcmp(mode, "fixed-random") == 0) {
        fixed_random = 1;
    } else {
        fprintf(stderr, "unknown mode: %s\n", mode);
        return 2;
    }

    reset_rng(KEYPAIR_SEED);
    if (sqisign_keypair_with_workspace(encoded_pk,
                                       encoded_sk,
                                       &operation_workspace.keygen) != 0) {
        fprintf(stderr, "deterministic key generation failed\n");
        return 1;
    }

    for (unsigned long i = 0; i < warmups; i++) {
        sample_result_t warmup_result;
        uint64_t state = RANDOM_SEED_BASE + (uint64_t)i;
        if (!run_sign(splitmix64(&state),
                      &warmup_result,
                      captured_warmup) ||
            !verify_captured(captured_warmup)) {
            fprintf(stderr, "warmup %lu failed\n", i);
            return 1;
        }
    }

    puts("sample,pair,class,order,seed,elapsed_ns,"
         "d4_inputs,d4_attempts,d4_first_failures,"
         "d8_inputs,d8_attempts,d8_first_failures,"
         "d16_inputs,d16_attempts,d16_first_failures,"
         "ml2_inputs,ml2_attempts,ml2_first_failures,signature_fnv64");

    for (unsigned long pair = 0; pair < pairs; pair++) {
        sample_result_t class_a;
        sample_result_t class_b;
        uint64_t state = RANDOM_SEED_BASE + (uint64_t)pair;
        uint64_t seed_a = FIXED_SIGN_SEED;
        uint64_t seed_b = fixed_random ? splitmix64(&state) : FIXED_SIGN_SEED;
        const char *order = (pair & 1u) == 0 ? "A-first" : "B-first";

        if (seed_b == FIXED_SIGN_SEED && fixed_random)
            seed_b ^= UINT64_C(1);

        if ((pair & 1u) == 0) {
            if (!run_sign(seed_a, &class_a, captured_a) ||
                !run_sign(seed_b, &class_b, captured_b)) {
                fprintf(stderr, "pair %lu failed\n", pair);
                return 1;
            }
        } else {
            if (!run_sign(seed_b, &class_b, captured_b) ||
                !run_sign(seed_a, &class_a, captured_a)) {
                fprintf(stderr, "pair %lu failed\n", pair);
                return 1;
            }
        }

        /* Keep verification outside both timed regions.  In particular, do
         * not let verification of A establish a class-dependent cache state
         * for B (or vice versa) inside a pair. */
        if (!verify_captured(captured_a) || !verify_captured(captured_b)) {
            fprintf(stderr, "pair %lu verification failed\n", pair);
            return 1;
        }

        if (pair == 0)
            fixed_digest = class_a.signature_digest;
        if (class_a.signature_digest != fixed_digest ||
            (!fixed_random && class_b.signature_digest != fixed_digest)) {
            fprintf(stderr, "fixed-stream signature changed at pair %lu\n", pair);
            return 1;
        }

        if ((pair & 1u) == 0) {
            print_sample(sample++, pair, "A", order, seed_a, &class_a);
            print_sample(sample++, pair, "B", order, seed_b, &class_b);
        } else {
            print_sample(sample++, pair, "B", order, seed_b, &class_b);
            print_sample(sample++, pair, "A", order, seed_a, &class_a);
        }
    }

    return 0;
}
