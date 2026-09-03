#include "mini_gmp_allocator.h"

#include <api.h>
#include <rng.h>

#include <errno.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(MINI_GMP)
#include <mini-gmp.h>
#else
#include <gmp.h>
#endif

#ifndef MINI_GMP_COMPARISON_BACKEND
#define MINI_GMP_COMPARISON_BACKEND "unknown"
#endif

enum {
    KAT_SEED_BYTES = 48,
    MAX_MESSAGE_BYTES = 4096,
    MAX_SIGNED_MESSAGE_BYTES = MAX_MESSAGE_BYTES + CRYPTO_BYTES
};

typedef enum measured_operation {
    OPERATION_KEYGEN,
    OPERATION_SIGN,
    OPERATION_VERIFY
} measured_operation_t;

typedef struct kat_vector {
    unsigned char seed[KAT_SEED_BYTES];
    unsigned char message[MAX_MESSAGE_BYTES];
    unsigned char public_key[CRYPTO_PUBLICKEYBYTES];
    unsigned char secret_key[CRYPTO_SECRETKEYBYTES];
    unsigned char signed_message[MAX_SIGNED_MESSAGE_BYTES];
    size_t message_bytes;
    size_t signed_message_bytes;
} kat_vector_t;

static unsigned char generated_public_key[CRYPTO_PUBLICKEYBYTES];
static unsigned char generated_secret_key[CRYPTO_SECRETKEYBYTES];
static unsigned char generated_signed_message[MAX_SIGNED_MESSAGE_BYTES];
static unsigned char opened_message[MAX_MESSAGE_BYTES];

static int
decode_hex(unsigned char *output, size_t output_bytes, const char *input)
{
    for (size_t i = 0; i < output_bytes; i++) {
        unsigned value;
        if (sscanf(input + 2u * i, "%2x", &value) != 1) {
            return 0;
        }
        output[i] = (unsigned char)value;
    }
    return input[2u * output_bytes] == '\0' ||
           input[2u * output_bytes] == '\n' ||
           input[2u * output_bytes] == '\r';
}

static int
line_value(const char *line, const char *label, const char **value)
{
    size_t label_bytes = strlen(label);
    if (strncmp(line, label, label_bytes) != 0) {
        return 0;
    }
    *value = line + label_bytes;
    return 1;
}

static int
load_kat(const char *path, unsigned requested_count, kat_vector_t *vector)
{
    char line[2 * MAX_SIGNED_MESSAGE_BYTES + 64];
    const char *value;
    FILE *input = fopen(path, "r");
    unsigned seen = 0;
    int active = 0;
    if (input == NULL) {
        return 0;
    }
    memset(vector, 0, sizeof(*vector));
    while (fgets(line, sizeof(line), input) != NULL) {
        if (line_value(line, "count = ", &value)) {
            unsigned long count = strtoul(value, NULL, 10);
            if (active) {
                break;
            }
            active = count == requested_count;
            if (active) seen |= 1u << 0;
        } else if (!active) {
            continue;
        } else if (line_value(line, "seed = ", &value)) {
            if (!decode_hex(vector->seed, sizeof(vector->seed), value)) break;
            seen |= 1u << 1;
        } else if (line_value(line, "mlen = ", &value)) {
            vector->message_bytes = (size_t)strtoul(value, NULL, 10);
            if (vector->message_bytes > sizeof(vector->message)) break;
            seen |= 1u << 2;
        } else if (line_value(line, "msg = ", &value)) {
            if (!(seen & (1u << 2)) ||
                !decode_hex(vector->message, vector->message_bytes, value)) break;
            seen |= 1u << 3;
        } else if (line_value(line, "pk = ", &value)) {
            if (!decode_hex(vector->public_key, sizeof(vector->public_key), value)) break;
            seen |= 1u << 4;
        } else if (line_value(line, "sk = ", &value)) {
            if (!decode_hex(vector->secret_key, sizeof(vector->secret_key), value)) break;
            seen |= 1u << 5;
        } else if (line_value(line, "smlen = ", &value)) {
            vector->signed_message_bytes = (size_t)strtoul(value, NULL, 10);
            if (vector->signed_message_bytes > sizeof(vector->signed_message)) break;
            seen |= 1u << 6;
        } else if (line_value(line, "sm = ", &value)) {
            if (!(seen & (1u << 6)) ||
                !decode_hex(vector->signed_message,
                            vector->signed_message_bytes,
                            value)) break;
            seen |= 1u << 7;
            break;
        }
    }
    fclose(input);
    return seen == 0xffu &&
           vector->signed_message_bytes == vector->message_bytes + CRYPTO_BYTES;
}

static uint64_t
elapsed_nanoseconds(const struct timespec *start, const struct timespec *end)
{
    return (uint64_t)(end->tv_sec - start->tv_sec) * UINT64_C(1000000000) +
           (uint64_t)(end->tv_nsec - start->tv_nsec);
}

static int
parse_size(const char *text, size_t *value)
{
    char *end = NULL;
    uintmax_t parsed;
    errno = 0;
    parsed = strtoumax(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || parsed > SIZE_MAX) {
        return 0;
    }
    *value = (size_t)parsed;
    return 1;
}

static int
parse_operation(const char *text, measured_operation_t *operation)
{
    if (strcmp(text, "keygen") == 0) {
        *operation = OPERATION_KEYGEN;
    } else if (strcmp(text, "sign") == 0) {
        *operation = OPERATION_SIGN;
    } else if (strcmp(text, "verify") == 0) {
        *operation = OPERATION_VERIFY;
    } else {
        return 0;
    }
    return 1;
}

static const char *
operation_name(measured_operation_t operation)
{
    switch (operation) {
    case OPERATION_KEYGEN: return "keygen";
    case OPERATION_SIGN: return "sign";
    case OPERATION_VERIFY: return "verify";
    }
    return "invalid";
}

static int
parse_allocator(const char *text, mini_gmp_allocator_mode_t *mode)
{
    if (strcmp(text, "native") == 0) {
        *mode = MINI_GMP_ALLOCATOR_NATIVE;
    } else if (strcmp(text, "tracking") == 0) {
        *mode = MINI_GMP_ALLOCATOR_TRACKING;
    } else if (strcmp(text, "pool") == 0) {
        *mode = MINI_GMP_ALLOCATOR_POOL;
    } else {
        return 0;
    }
    return 1;
}

static int
prepare_sign(const kat_vector_t *vector)
{
    randombytes_init((unsigned char *)vector->seed, NULL, 256);
    if (crypto_sign_keypair(generated_public_key, generated_secret_key) != 0) {
        return 0;
    }
    return memcmp(generated_public_key,
                  vector->public_key,
                  sizeof(generated_public_key)) == 0 &&
           memcmp(generated_secret_key,
                  vector->secret_key,
                  sizeof(generated_secret_key)) == 0;
}

static int
run_operation(measured_operation_t operation, const kat_vector_t *vector)
{
    unsigned long long output_bytes;
    switch (operation) {
    case OPERATION_KEYGEN:
        randombytes_init((unsigned char *)vector->seed, NULL, 256);
        if (crypto_sign_keypair(generated_public_key, generated_secret_key) != 0) {
            return 0;
        }
        return memcmp(generated_public_key,
                      vector->public_key,
                      sizeof(generated_public_key)) == 0 &&
               memcmp(generated_secret_key,
                      vector->secret_key,
                      sizeof(generated_secret_key)) == 0;
    case OPERATION_SIGN:
        output_bytes = sizeof(generated_signed_message);
        if (crypto_sign(generated_signed_message,
                        &output_bytes,
                        vector->message,
                        vector->message_bytes,
                        generated_secret_key) != 0) {
            return 0;
        }
        return output_bytes == vector->signed_message_bytes &&
               memcmp(generated_signed_message,
                      vector->signed_message,
                      output_bytes) == 0;
    case OPERATION_VERIFY:
        output_bytes = sizeof(opened_message);
        if (crypto_sign_open(opened_message,
                             &output_bytes,
                             vector->signed_message,
                             vector->signed_message_bytes,
                             vector->public_key) != 0) {
            return 0;
        }
        return output_bytes == vector->message_bytes &&
               memcmp(opened_message, vector->message, output_bytes) == 0;
    }
    return 0;
}

static void
print_json(measured_operation_t operation,
           mini_gmp_allocator_mode_t mode,
           size_t pool_bytes,
           unsigned kat_count,
           const char *status,
           uint64_t elapsed_ns)
{
    const mini_gmp_allocator_stats_t *stats = mini_gmp_allocator_stats();
    printf("{\"schema\":\"sqisign-mini-gmp-comparison-v1\","
           "\"backend\":\"%s\",\"limb_bits\":%d,"
           "\"operation\":\"%s\",\"allocator\":\"%s\","
           "\"kat_count\":%u,\"pool_bytes\":%zu,"
           "\"pool_compile_capacity_bytes\":%zu,\"status\":\"%s\","
           "\"elapsed_ns\":%" PRIu64,
           MINI_GMP_COMPARISON_BACKEND,
           (int)(sizeof(mp_limb_t) * 8u),
           operation_name(operation),
           mini_gmp_allocator_mode_name(mode),
           kat_count,
           pool_bytes,
           mini_gmp_allocator_pool_max_bytes(),
           status,
           elapsed_ns);
    if (mode == MINI_GMP_ALLOCATOR_NATIVE) {
        printf(",\"allocator_stats\":null}");
    } else {
        printf(",\"allocator_stats\":{"
               "\"allocation_calls\":%" PRIu64 ","
               "\"reallocation_calls\":%" PRIu64 ","
               "\"pool_relocation_allocations\":%" PRIu64 ","
               "\"free_calls\":%" PRIu64 ","
               "\"failed_calls\":%" PRIu64 ","
               "\"size_mismatches\":%" PRIu64 ","
               "\"current_allocations\":%zu,"
               "\"peak_allocations\":%zu,"
               "\"current_requested_bytes\":%zu,"
               "\"peak_requested_bytes\":%zu,"
               "\"conservative_relocation_peak_bytes\":%zu,"
               "\"total_requested_bytes\":%zu,"
               "\"largest_request_bytes\":%zu,"
               "\"current_physical_bytes\":%zu,"
               "\"peak_physical_bytes\":%zu,"
               "\"pool_capacity_bytes\":%zu,"
               "\"pool_high_water_bytes\":%zu,"
               "\"secure_clear_bytes\":%zu}}",
               stats->allocation_calls,
               stats->reallocation_calls,
               stats->pool_relocation_allocations,
               stats->free_calls,
               stats->failed_calls,
               stats->size_mismatches,
               stats->current_allocations,
               stats->peak_allocations,
               stats->current_requested_bytes,
               stats->peak_requested_bytes,
               stats->conservative_relocation_peak_bytes,
               stats->total_requested_bytes,
               stats->largest_request_bytes,
               stats->current_physical_bytes,
               stats->peak_physical_bytes,
               stats->pool_capacity_bytes,
               stats->pool_high_water_bytes,
               stats->secure_clear_bytes);
    }
    putchar('\n');
}

int
main(int argc, char **argv)
{
    const char *kat_path = NULL;
    measured_operation_t operation = OPERATION_KEYGEN;
    mini_gmp_allocator_mode_t mode = MINI_GMP_ALLOCATOR_TRACKING;
    size_t pool_bytes = mini_gmp_allocator_pool_max_bytes();
    size_t kat_count_value = 0;
    kat_vector_t vector;
    struct timespec start;
    struct timespec end;
    jmp_buf oom_environment;
    volatile int operation_ok = 0;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--kat=", 6) == 0) {
            kat_path = argv[i] + 6;
        } else if (strncmp(argv[i], "--operation=", 12) == 0) {
            if (!parse_operation(argv[i] + 12, &operation)) return 2;
        } else if (strncmp(argv[i], "--allocator=", 12) == 0) {
            if (!parse_allocator(argv[i] + 12, &mode)) return 2;
        } else if (strncmp(argv[i], "--pool-bytes=", 13) == 0) {
            if (!parse_size(argv[i] + 13, &pool_bytes)) return 2;
        } else if (strncmp(argv[i], "--count=", 8) == 0) {
            if (!parse_size(argv[i] + 8, &kat_count_value) ||
                kat_count_value > 99u) return 2;
        } else {
            fprintf(stderr,
                    "usage: %s --kat=FILE --operation=keygen|sign|verify "
                    "--allocator=native|tracking|pool [--pool-bytes=N] "
                    "[--count=0..99]\n",
                    argv[0]);
            return 2;
        }
    }
    if (kat_path == NULL ||
        !load_kat(kat_path, (unsigned)kat_count_value, &vector)) {
        fputs("failed to read requested Level-I KAT vector\n", stderr);
        return 2;
    }

    if (!mini_gmp_allocator_select(mode, pool_bytes)) {
        fputs("invalid allocator selection or pool size\n", stderr);
        return 2;
    }
    if (operation == OPERATION_SIGN) {
        mini_gmp_allocator_arm_oom(&oom_environment);
        if (setjmp(oom_environment) != 0) {
            mini_gmp_allocator_disarm_oom();
            print_json(operation,
                       mode,
                       pool_bytes,
                       (unsigned)kat_count_value,
                       "setup-oom",
                       0);
            return 4;
        }
        if (!prepare_sign(&vector)) {
            mini_gmp_allocator_disarm_oom();
            fputs("setup did not reproduce the KAT keypair\n", stderr);
            return 3;
        }
        mini_gmp_allocator_disarm_oom();
    }
    if (mode != MINI_GMP_ALLOCATOR_NATIVE && !mini_gmp_allocator_reset()) {
        fputs("allocator was not empty before the measured operation\n", stderr);
        return 3;
    }

    mini_gmp_allocator_arm_oom(&oom_environment);
    if (setjmp(oom_environment) != 0) {
        mini_gmp_allocator_disarm_oom();
        print_json(operation,
                   mode,
                   pool_bytes,
                   (unsigned)kat_count_value,
                   "oom",
                   0);
        return 4;
    }
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        return 3;
    }
    operation_ok = run_operation(operation, &vector);
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        return 3;
    }
    mini_gmp_allocator_disarm_oom();
    if (!operation_ok) {
        print_json(operation,
                   mode,
                   pool_bytes,
                   (unsigned)kat_count_value,
                   "kat-mismatch",
                   elapsed_nanoseconds(&start, &end));
        return 5;
    }
    if (mode != MINI_GMP_ALLOCATOR_NATIVE &&
        (mini_gmp_allocator_stats()->current_requested_bytes != 0 ||
         mini_gmp_allocator_stats()->current_physical_bytes != 0 ||
         mini_gmp_allocator_stats()->current_allocations != 0 ||
         mini_gmp_allocator_stats()->size_mismatches != 0)) {
        print_json(operation,
                   mode,
                   pool_bytes,
                   (unsigned)kat_count_value,
                   "allocator-leak",
                   elapsed_nanoseconds(&start, &end));
        return 6;
    }
    print_json(operation,
               mode,
               pool_bytes,
               (unsigned)kat_count_value,
               "pass",
               elapsed_nanoseconds(&start, &end));
    return 0;
}
