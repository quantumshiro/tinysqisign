// SPDX-License-Identifier: Apache-2.0
/*
 * SQIsign v3 fixed-message/fixed-RNG, multi-key structural trace screen.
 *
 * Project objects are compiled with Clang SanitizerCoverage.  This harness is
 * deliberately compiled without instrumentation and enables the callbacks
 * only around crypto_sign().  Every secret key is first copied to the same
 * fixed-address buffer, so a cross-key address difference is not merely the
 * address of a different row in the generated KAT table.
 *
 * The screen records control-flow edge IDs and load/store effective addresses,
 * not loaded values, register transitions, power, or EM leakage.  Equality of
 * the recorded streams is therefore not a constant-time proof.
 */

#include <api.h>
#include <rng.h>

#include <dlfcn.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "v3_kat_subset.h"

enum {
    DEFAULT_EDGE_CAPTURE = 2000000,
    DEFAULT_MEMORY_CAPTURE = 2000000,
};

typedef struct {
    uint64_t edge_events;
    uint64_t edge_unique;
    uint64_t edge_hash1;
    uint64_t edge_hash2;
    size_t edge_capture_length;
    int edge_capture_truncated;
    uint32_t *edge_capture;
    uint64_t load_events;
    uint64_t store_events;
    uint64_t memory_hash1;
    uint64_t memory_hash2;
    size_t memory_capture_length;
    int memory_capture_truncated;
    uint64_t *memory_capture;
    uint64_t signature_hash;
} trace_result_t;

static const unsigned char signing_seed[SQISIGN_V3_KAT_SEED_BYTES] = {
    0xa5, 0xa4, 0xa7, 0xa6, 0xa1, 0xa0, 0xa3, 0xa2,
    0xad, 0xac, 0xaf, 0xae, 0xa9, 0xa8, 0xab, 0xaa,
    0xb5, 0xb4, 0xb7, 0xb6, 0xb1, 0xb0, 0xb3, 0xb2,
    0xbd, 0xbc, 0xbf, 0xbe, 0xb9, 0xb8, 0xbb, 0xba,
    0x85, 0x84, 0x87, 0x86, 0x81, 0x80, 0x83, 0x82,
    0x8d, 0x8c, 0x8f, 0x8e, 0x89, 0x88, 0x8b, 0x8a,
};

static unsigned char active_public_key[CRYPTO_PUBLICKEYBYTES];
static unsigned char active_secret_key[CRYPTO_SECRETKEYBYTES];
static unsigned char
    signed_message[CRYPTO_BYTES + SQISIGN_V3_KAT_MAX_MESSAGE_BYTES];
static unsigned char opened_message[SQISIGN_V3_KAT_MAX_MESSAGE_BYTES];

static uint32_t guard_count;
static unsigned char *edge_seen;
static uintptr_t *guard_pc;
static uint32_t *edge_capture_buffer;
static size_t edge_capture_capacity;
static uint64_t *memory_capture_buffer;
static size_t memory_capture_capacity;
static uintptr_t image_base;

static volatile int trace_enabled;
static uint64_t edge_events;
static uint64_t edge_unique;
static uint64_t edge_hash1;
static uint64_t edge_hash2;
static size_t edge_capture_length;
static int edge_capture_truncated;
static uint64_t load_events;
static uint64_t store_events;
static uint64_t memory_hash1;
static uint64_t memory_hash2;
static size_t memory_capture_length;
static int memory_capture_truncated;

static uint64_t
mix1(uint64_t state, uint64_t value)
{
    state ^= value;
    return state * UINT64_C(1099511628211);
}

static uint64_t
mix2(uint64_t state, uint64_t value)
{
    return state ^ (value + UINT64_C(0x9e3779b97f4a7c15) +
                    (state << 6) + (state >> 2));
}

void
__sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop)
{
    if (start == stop || *start != 0)
        return;
    for (uint32_t *cursor = start; cursor < stop; ++cursor)
        *cursor = ++guard_count;
}

void
__sanitizer_cov_trace_pc_guard(uint32_t *guard)
{
    const uint32_t id = *guard;
    if (!trace_enabled || id == 0)
        return;
    ++edge_events;
    edge_hash1 = mix1(edge_hash1, id);
    edge_hash2 = mix2(edge_hash2, id);
    if (id <= guard_count && edge_seen[id] == 0) {
        edge_seen[id] = 1;
        ++edge_unique;
        if (guard_pc[id] == 0)
            guard_pc[id] = (uintptr_t)__builtin_return_address(0);
    }
    if (edge_capture_length < edge_capture_capacity)
        edge_capture_buffer[edge_capture_length++] = id;
    else
        edge_capture_truncated = 1;
}

static void
record_memory(uintptr_t address, unsigned width, unsigned is_store)
{
    if (!trace_enabled)
        return;
    const unsigned meta = (is_store ? 0x80U : 0U) | width;
    const uint64_t event =
        ((uint64_t)address & UINT64_C(0x00ffffffffffffff)) |
        ((uint64_t)meta << 56);
    if (is_store)
        ++store_events;
    else
        ++load_events;
    memory_hash1 = mix1(memory_hash1, event);
    memory_hash2 = mix2(memory_hash2, event);
    if (memory_capture_length < memory_capture_capacity)
        memory_capture_buffer[memory_capture_length++] = event;
    else
        memory_capture_truncated = 1;
}

#define DEFINE_MEMORY_CALLBACKS(bits, bytes)                            \
    void __sanitizer_cov_load##bits(uintptr_t address)                  \
    {                                                                   \
        record_memory(address, bytes, 0);                               \
    }                                                                   \
    void __sanitizer_cov_store##bits(uintptr_t address)                 \
    {                                                                   \
        record_memory(address, bytes, 1);                               \
    }

DEFINE_MEMORY_CALLBACKS(1, 1)
DEFINE_MEMORY_CALLBACKS(2, 2)
DEFINE_MEMORY_CALLBACKS(4, 4)
DEFINE_MEMORY_CALLBACKS(8, 8)
DEFINE_MEMORY_CALLBACKS(16, 16)

static uint64_t
fnv1a64(const unsigned char *data, size_t length)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    for (size_t i = 0; i < length; ++i) {
        hash ^= data[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static size_t
environment_capacity(const char *name, size_t fallback)
{
    const char *text = getenv(name);
    char *end = NULL;
    if (text == NULL || *text == '\0')
        return fallback;
    const unsigned long long parsed = strtoull(text, &end, 10);
    if (end == text || *end != '\0' || parsed == 0 ||
        parsed > UINT64_C(100000000)) {
        fprintf(stderr, "invalid %s\n", name);
        exit(2);
    }
    return (size_t)parsed;
}

static void
initialize_trace_buffers(void)
{
    Dl_info info;
    edge_capture_capacity = environment_capacity(
        "SQISIGN_V3_EDGE_CAPTURE", DEFAULT_EDGE_CAPTURE);
    memory_capture_capacity = environment_capacity(
        "SQISIGN_V3_MEMORY_CAPTURE", DEFAULT_MEMORY_CAPTURE);
    edge_seen = calloc((size_t)guard_count + 1, 1);
    guard_pc = calloc((size_t)guard_count + 1, sizeof(*guard_pc));
    edge_capture_buffer = malloc(edge_capture_capacity *
                                 sizeof(*edge_capture_buffer));
    memory_capture_buffer = malloc(memory_capture_capacity *
                                   sizeof(*memory_capture_buffer));
    if (edge_seen == NULL || guard_pc == NULL || edge_capture_buffer == NULL ||
        memory_capture_buffer == NULL) {
        fprintf(stderr, "trace buffer allocation failed\n");
        exit(3);
    }
    if (dladdr((void *)&initialize_trace_buffers, &info) != 0)
        image_base = (uintptr_t)info.dli_fbase;
}

static void
reset_trace(void)
{
    memset(edge_seen, 0, (size_t)guard_count + 1);
    edge_events = 0;
    edge_unique = 0;
    edge_hash1 = UINT64_C(1469598103934665603);
    edge_hash2 = UINT64_C(0x6a09e667f3bcc909);
    edge_capture_length = 0;
    edge_capture_truncated = 0;
    load_events = 0;
    store_events = 0;
    memory_hash1 = UINT64_C(1469598103934665603);
    memory_hash2 = UINT64_C(0xbb67ae8584caa73b);
    memory_capture_length = 0;
    memory_capture_truncated = 0;
}

static int
copy_trace_result(trace_result_t *result)
{
    memset(result, 0, sizeof(*result));
    result->edge_events = edge_events;
    result->edge_unique = edge_unique;
    result->edge_hash1 = edge_hash1;
    result->edge_hash2 = edge_hash2;
    result->edge_capture_length = edge_capture_length;
    result->edge_capture_truncated = edge_capture_truncated;
    result->load_events = load_events;
    result->store_events = store_events;
    result->memory_hash1 = memory_hash1;
    result->memory_hash2 = memory_hash2;
    result->memory_capture_length = memory_capture_length;
    result->memory_capture_truncated = memory_capture_truncated;
    result->edge_capture = malloc(edge_capture_length *
                                  sizeof(*result->edge_capture));
    result->memory_capture = malloc(memory_capture_length *
                                    sizeof(*result->memory_capture));
    if ((edge_capture_length != 0 && result->edge_capture == NULL) ||
        (memory_capture_length != 0 && result->memory_capture == NULL))
        return 0;
    memcpy(result->edge_capture, edge_capture_buffer,
           edge_capture_length * sizeof(*result->edge_capture));
    memcpy(result->memory_capture, memory_capture_buffer,
           memory_capture_length * sizeof(*result->memory_capture));
    return 1;
}

static void
free_trace_result(trace_result_t *result)
{
    free(result->edge_capture);
    free(result->memory_capture);
    memset(result, 0, sizeof(*result));
}

static int
run_trace(unsigned key_index, trace_result_t *result)
{
    const sqisign_v3_kat_vector_t *vector =
        &sqisign_v3_kat_vectors[key_index];
    const sqisign_v3_kat_vector_t *message_vector =
        &sqisign_v3_kat_vectors[0];
    unsigned char seed[sizeof(signing_seed)];
    unsigned long long signed_length = 0;
    unsigned long long opened_length = 0;
    memcpy(active_public_key, vector->public_key, sizeof(active_public_key));
    memcpy(active_secret_key, vector->secret_key, sizeof(active_secret_key));
    memcpy(seed, signing_seed, sizeof(seed));
    randombytes_init(seed, NULL, 256);
    memset(signed_message, 0xa5, sizeof(signed_message));
    reset_trace();
    trace_enabled = 1;
    const int sign_status = crypto_sign(
        signed_message,
        &signed_length,
        message_vector->message,
        message_vector->message_length,
        active_secret_key);
    trace_enabled = 0;
    if (!copy_trace_result(result))
        return 0;
    const int verify_status = sign_status == 0 ? crypto_sign_open(
        opened_message,
        &opened_length,
        signed_message,
        signed_length,
        active_public_key) : -1;
    if (sign_status != 0 || verify_status != 0 ||
        signed_length != CRYPTO_BYTES + message_vector->message_length ||
        opened_length != message_vector->message_length ||
        memcmp(opened_message, message_vector->message, opened_length) != 0) {
        return 0;
    }
    result->signature_hash = fnv1a64(signed_message, (size_t)signed_length);
    return 1;
}

static int64_t
first_edge_difference(const trace_result_t *a, const trace_result_t *b)
{
    const size_t common = a->edge_capture_length < b->edge_capture_length ?
                          a->edge_capture_length : b->edge_capture_length;
    for (size_t i = 0; i < common; ++i) {
        if (a->edge_capture[i] != b->edge_capture[i])
            return (int64_t)i;
    }
    return a->edge_capture_length == b->edge_capture_length ? -1 :
           (int64_t)common;
}

static int64_t
first_memory_difference(const trace_result_t *a, const trace_result_t *b)
{
    const size_t common = a->memory_capture_length < b->memory_capture_length ?
                          a->memory_capture_length : b->memory_capture_length;
    for (size_t i = 0; i < common; ++i) {
        if (a->memory_capture[i] != b->memory_capture[i])
            return (int64_t)i;
    }
    return a->memory_capture_length == b->memory_capture_length ? -1 :
           (int64_t)common;
}

static int
edge_equal(const trace_result_t *a, const trace_result_t *b)
{
    return a->edge_events == b->edge_events &&
           a->edge_hash1 == b->edge_hash1 &&
           a->edge_hash2 == b->edge_hash2;
}

static int
memory_equal(const trace_result_t *a, const trace_result_t *b)
{
    return a->load_events == b->load_events &&
           a->store_events == b->store_events &&
           a->memory_hash1 == b->memory_hash1 &&
           a->memory_hash2 == b->memory_hash2;
}

static uintptr_t
edge_pc_at(const trace_result_t *result, int64_t index)
{
    if (index < 0 || (uint64_t)index >= result->edge_capture_length)
        return 0;
    const uint32_t id = result->edge_capture[(size_t)index];
    return id <= guard_count ? guard_pc[id] : 0;
}

static uint64_t
memory_event_at(const trace_result_t *result, int64_t index)
{
    if (index < 0 || (uint64_t)index >= result->memory_capture_length)
        return 0;
    return result->memory_capture[(size_t)index];
}

static void
emit_result(const char *implementation,
            const char *run,
            const char *dataset,
            unsigned pair,
            const char *class_name,
            unsigned execution_order,
            unsigned key,
            const trace_result_t *result,
            int pair_edge_equal,
            int pair_memory_equal,
            int64_t edge_difference,
            uintptr_t edge_a_pc,
            uintptr_t edge_b_pc,
            int64_t memory_difference,
            uint64_t memory_a_event,
            uint64_t memory_b_event)
{
    printf("%s,%s,%s,%u,%s,%u,%u,%u,%" PRIu64 ",%" PRIu64
           ",%016" PRIx64 ",%016" PRIx64 ",%zu,%d,%" PRIu64
           ",%" PRIu64 ",%016" PRIx64 ",%016" PRIx64 ",%zu,%d,"
           "%016" PRIx64 ",%d,%d,%" PRId64 ",%016" PRIxPTR
           ",%016" PRIxPTR ",%" PRId64 ",%016" PRIx64
           ",%016" PRIx64 ",%016" PRIxPTR ",PASS\n",
           implementation, run, dataset, pair, class_name, execution_order, key,
           guard_count, result->edge_events, result->edge_unique,
           result->edge_hash1, result->edge_hash2,
           result->edge_capture_length, result->edge_capture_truncated,
           result->load_events, result->store_events,
           result->memory_hash1, result->memory_hash2,
           result->memory_capture_length, result->memory_capture_truncated,
           result->signature_hash, pair_edge_equal, pair_memory_equal,
           edge_difference, edge_a_pc, edge_b_pc, memory_difference,
           memory_a_event, memory_b_event, image_base);
}

static int
run_pair(const char *implementation,
         const char *run,
         const char *dataset,
         unsigned pair,
         unsigned key_b)
{
    trace_result_t a = { 0 };
    trace_result_t b = { 0 };
    const unsigned a_order = pair % 2U == 0U ? 0U : 1U;
    const unsigned b_order = 1U - a_order;
    int ok;
    if (a_order == 0U)
        ok = run_trace(0, &a) && run_trace(key_b, &b);
    else
        ok = run_trace(key_b, &b) && run_trace(0, &a);
    if (!ok) {
        free_trace_result(&a);
        free_trace_result(&b);
        return 0;
    }
    const int edges_same = edge_equal(&a, &b);
    const int memory_same = memory_equal(&a, &b);
    const int64_t edge_difference = first_edge_difference(&a, &b);
    const int64_t memory_difference = first_memory_difference(&a, &b);
    const uintptr_t edge_a_pc = edge_pc_at(&a, edge_difference);
    const uintptr_t edge_b_pc = edge_pc_at(&b, edge_difference);
    const uint64_t memory_a_event = memory_event_at(&a, memory_difference);
    const uint64_t memory_b_event = memory_event_at(&b, memory_difference);
    emit_result(implementation, run, dataset, pair, "A", a_order, 0, &a,
                edges_same, memory_same, edge_difference, edge_a_pc,
                edge_b_pc, memory_difference, memory_a_event, memory_b_event);
    emit_result(implementation, run, dataset, pair, "B", b_order, key_b, &b,
                edges_same, memory_same, edge_difference, edge_a_pc,
                edge_b_pc, memory_difference, memory_a_event, memory_b_event);
    free_trace_result(&a);
    free_trace_result(&b);
    return 1;
}

int
main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s IMPLEMENTATION RUN\n", argv[0]);
        return 2;
    }
    if (SQISIGN_V3_KAT_VECTOR_COUNT != 10)
        return 2;
    initialize_trace_buffers();
    trace_result_t warmup = { 0 };
    if (!run_trace(0, &warmup))
        return 3;
    free_trace_result(&warmup);

    printf("implementation,run,dataset,pair,class,execution_order,key,guard_count,"
           "edge_events,edge_unique,edge_hash1,edge_hash2,edge_capture_events,"
           "edge_capture_truncated,load_events,store_events,memory_hash1,"
           "memory_hash2,memory_capture_events,memory_capture_truncated,"
           "signature_fnv64,pair_edge_equal,pair_memory_equal,"
           "first_edge_diff_index,first_edge_a_pc,first_edge_b_pc,"
           "first_memory_diff_index,first_memory_a_event,"
           "first_memory_b_event,image_base,status\n");
    for (unsigned pair = 0; pair < 2; ++pair) {
        if (!run_pair(argv[1], argv[2], "control", pair, 0))
            return 4;
    }
    for (unsigned key = 1; key < 10; ++key) {
        if (!run_pair(argv[1], argv[2], "different-key", key - 1, key))
            return 5;
    }
    return 0;
}
