// SPDX-License-Identifier: Apache-2.0
/* Host-only whole-Sign control-flow and memory-address trace harness.
 *
 * All project objects are compiled with Clang SanitizerCoverage.  This file
 * provides uninstrumented callbacks and enables them only around Sign.  It
 * compares deterministic fixed/fixed negative controls with fixed/random
 * signing-RNG pairs in one process, so ASLR and stack bases are shared.
 * Values are not observed; this is not a power model or resistance proof.
 */

#include <encoded_sizes.h>
#include <rng.h>
#include <signature_lowmem.h>

#include <dlfcn.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char KAT_PK_HEX[] =
    "01C70D9910A43882F0FFD5C68305AC3220CE54A8625294DCCBDB4537809D7903"
    "E74DE894B086EFD981D508E4C5ACD9D8B5126F0C43C73B27115A542717CE1E010B";

static const char KAT_SK_HEX[] =
    "01C70D9910A43882F0FFD5C68305AC3220CE54A8625294DCCBDB4537809D7903"
    "E74DE894B086EFD981D508E4C5ACD9D8B5126F0C43C73B27115A542717CE1E010B"
    "C16A609C6E12E6C5099C2086E56EC30A6A020000000000000000000000000000"
    "78691BE93DC95931F2FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
    "4A927F64A0B4EE0CE8FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
    "38F639337B258192F0FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
    "0CE247B5A6BE2125090000000000000000000000000000000000000000000000"
    "1100515A4D1D5C08589B74635B0B6A5952D856940AFF47D37F41A74B5CE52300"
    "59E91ECE9FA0A0AD7C6B1CF3391903DD2ADCDB09EE07BF01A9415BE3A5D5EB00"
    "3135AE21B9050612311ADF69DD388DE056970E8A92178BE63EC0FCEEA3E21D00"
    "B631015DB10FF7C460BF597C01412745B6514684A2DAD54CAA299C6BC7C49100";

static const char KAT_MSG_HEX[] =
    "D81C4D8D734FCBFBEADE3D3F8A039FAA2A2C9957E835AD55B22E75BF57BB556AC8";

enum {
    DEFAULT_EDGE_CAPTURE = 4000000,
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

static protocols_operation_workspace_t operation_workspace;
static unsigned char pk[PUBLICKEY_BYTES];
static unsigned char sk[SECRETKEY_BYTES];
static unsigned char message[33];
static unsigned char signed_message[SIGNATURE_BYTES + sizeof(message)];

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
    uint32_t *cursor;
    if (start == stop || *start != 0)
        return;
    for (cursor = start; cursor < stop; ++cursor)
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
    uint64_t event;
    unsigned meta;
    if (!trace_enabled)
        return;
    meta = (is_store ? 0x80U : 0U) | width;
    event = ((uint64_t)address & UINT64_C(0x00ffffffffffffff)) |
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

#define DEFINE_MEMORY_CALLBACKS(bits, bytes)                              \
    void __sanitizer_cov_load##bits(uintptr_t address)                    \
    {                                                                     \
        record_memory(address, bytes, 0);                                 \
    }                                                                     \
    void __sanitizer_cov_store##bits(uintptr_t address)                   \
    {                                                                     \
        record_memory(address, bytes, 1);                                 \
    }

DEFINE_MEMORY_CALLBACKS(1, 1)
DEFINE_MEMORY_CALLBACKS(2, 2)
DEFINE_MEMORY_CALLBACKS(4, 4)
DEFINE_MEMORY_CALLBACKS(8, 8)
DEFINE_MEMORY_CALLBACKS(16, 16)

static int
hex_nibble(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static int
decode_hex(unsigned char *output, size_t output_length, const char *hex)
{
    size_t index;
    if (strlen(hex) != 2 * output_length)
        return 0;
    for (index = 0; index < output_length; ++index) {
        const int high = hex_nibble(hex[2 * index]);
        const int low = hex_nibble(hex[2 * index + 1]);
        if (high < 0 || low < 0)
            return 0;
        output[index] = (unsigned char)((high << 4) | low);
    }
    return 1;
}

static int
load_kat(void)
{
    return decode_hex(pk, sizeof(pk), KAT_PK_HEX) &&
           decode_hex(sk, sizeof(sk), KAT_SK_HEX) &&
           decode_hex(message, sizeof(message), KAT_MSG_HEX);
}

static void
reset_rng(uint64_t seed)
{
    unsigned char entropy[48];
    size_t index;
    for (index = 0; index < sizeof(entropy); ++index) {
        const unsigned shift = (unsigned)(8 * (index % sizeof(seed)));
        entropy[index] =
            (unsigned char)((seed >> shift) ^ (UINT64_C(0x9d) * index));
    }
    randombytes_init(entropy, NULL, 256);
}

static uint64_t
fnv1a64(const unsigned char *input, size_t length)
{
    uint64_t digest = UINT64_C(1469598103934665603);
    size_t index;
    for (index = 0; index < length; ++index) {
        digest ^= input[index];
        digest *= UINT64_C(1099511628211);
    }
    return digest;
}

static size_t
environment_capacity(const char *name, size_t fallback)
{
    const char *text = getenv(name);
    char *end = NULL;
    unsigned long long parsed;
    if (text == NULL || *text == '\0')
        return fallback;
    parsed = strtoull(text, &end, 10);
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
        "SQISIGN_SCA_EDGE_CAPTURE", DEFAULT_EDGE_CAPTURE);
    memory_capture_capacity = environment_capacity(
        "SQISIGN_SCA_MEMORY_CAPTURE", DEFAULT_MEMORY_CAPTURE);
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
run_trace(uint64_t seed, trace_result_t *result)
{
    unsigned long long signed_length = sizeof(signed_message);
    int status;
    memset(signed_message, 0xa5, sizeof(signed_message));
    reset_rng(seed);
    reset_trace();
    trace_enabled = 1;
    status = sqisign_sign_with_workspace(signed_message,
                                         &signed_length,
                                         message,
                                         sizeof(message),
                                         sk,
                                         &operation_workspace.sign);
    trace_enabled = 0;
    if (!copy_trace_result(result))
        return 0;
    if (status != 0 || signed_length != sizeof(signed_message) ||
        memcmp(signed_message + SIGNATURE_BYTES,
               message,
               sizeof(message)) != 0 ||
        sqisign_verify_with_workspace(message,
                                      sizeof(message),
                                      signed_message,
                                      SIGNATURE_BYTES,
                                      pk,
                                      &operation_workspace.verify) != 0) {
        return 0;
    }
    result->signature_hash = fnv1a64(
        signed_message, (size_t)signed_length);
    return 1;
}

static int64_t
first_edge_difference(const trace_result_t *a, const trace_result_t *b)
{
    size_t index;
    const size_t common = a->edge_capture_length < b->edge_capture_length ?
                          a->edge_capture_length : b->edge_capture_length;
    for (index = 0; index < common; ++index) {
        if (a->edge_capture[index] != b->edge_capture[index])
            return (int64_t)index;
    }
    if (a->edge_capture_length != b->edge_capture_length)
        return (int64_t)common;
    return -1;
}

static int64_t
first_memory_difference(const trace_result_t *a, const trace_result_t *b)
{
    size_t index;
    const size_t common =
        a->memory_capture_length < b->memory_capture_length ?
        a->memory_capture_length : b->memory_capture_length;
    for (index = 0; index < common; ++index) {
        if (a->memory_capture[index] != b->memory_capture[index])
            return (int64_t)index;
    }
    if (a->memory_capture_length != b->memory_capture_length)
        return (int64_t)common;
    return -1;
}

static uintptr_t
edge_pc_at(const trace_result_t *result, int64_t index)
{
    uint32_t id;
    if (index < 0 || (uint64_t)index >= result->edge_capture_length)
        return 0;
    id = result->edge_capture[(size_t)index];
    return id <= guard_count ? guard_pc[id] : 0;
}

static uint64_t
memory_event_at(const trace_result_t *result, int64_t index)
{
    if (index < 0 || (uint64_t)index >= result->memory_capture_length)
        return 0;
    return result->memory_capture[(size_t)index];
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

static void
emit_result(const char *dataset,
            unsigned pair,
            const char *class_name,
            unsigned execution_order,
            uint64_t seed,
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
    printf("%s,%u,%s,%u,%" PRIu64 ",%u,%" PRIu64 ",%" PRIu64
           ",%016" PRIx64 ",%016" PRIx64 ",%zu,%d,%" PRIu64
           ",%" PRIu64 ",%016" PRIx64 ",%016" PRIx64 ",%zu,%d,"
           "%016" PRIx64 ",%d,%d,%" PRId64 ",%016" PRIxPTR
           ",%016" PRIxPTR ",%" PRId64 ",%016" PRIx64
           ",%016" PRIx64 ",%016" PRIxPTR ",PASS\n",
           dataset, pair, class_name, execution_order, seed, guard_count,
           result->edge_events, result->edge_unique, result->edge_hash1,
           result->edge_hash2, result->edge_capture_length,
           result->edge_capture_truncated, result->load_events,
           result->store_events, result->memory_hash1, result->memory_hash2,
           result->memory_capture_length,
           result->memory_capture_truncated, result->signature_hash,
           pair_edge_equal, pair_memory_equal, edge_difference, edge_a_pc,
           edge_b_pc, memory_difference, memory_a_event, memory_b_event,
           image_base);
}

static int
run_pair(const char *dataset, unsigned pair, uint64_t seed_b)
{
    trace_result_t a;
    trace_result_t b;
    int ok;
    int edges_same;
    int memory_same;
    int64_t edge_difference;
    int64_t memory_difference;
    uintptr_t edge_a_pc;
    uintptr_t edge_b_pc;
    uint64_t memory_a_event;
    uint64_t memory_b_event;
    const unsigned a_order = (pair % 2U == 0U) ? 0U : 1U;
    const unsigned b_order = 1U - a_order;

    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    if (a_order == 0U) {
        ok = run_trace(0, &a) && run_trace(seed_b, &b);
    } else {
        ok = run_trace(seed_b, &b) && run_trace(0, &a);
    }
    if (!ok) {
        free_trace_result(&a);
        free_trace_result(&b);
        return 0;
    }
    edges_same = edge_equal(&a, &b);
    memory_same = memory_equal(&a, &b);
    edge_difference = first_edge_difference(&a, &b);
    memory_difference = first_memory_difference(&a, &b);
    edge_a_pc = edge_pc_at(&a, edge_difference);
    edge_b_pc = edge_pc_at(&b, edge_difference);
    memory_a_event = memory_event_at(&a, memory_difference);
    memory_b_event = memory_event_at(&b, memory_difference);
    emit_result(dataset, pair, "A", a_order, 0, &a, edges_same,
                memory_same, edge_difference, edge_a_pc, edge_b_pc,
                memory_difference, memory_a_event, memory_b_event);
    emit_result(dataset, pair, "B", b_order, seed_b, &b, edges_same,
                memory_same, edge_difference, edge_a_pc, edge_b_pc,
                memory_difference, memory_a_event, memory_b_event);
    free_trace_result(&a);
    free_trace_result(&b);
    return 1;
}

int
main(int argc, char **argv)
{
    static const uint64_t primary_seeds[] = {
        1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233,
    };
    unsigned control_pairs = 2;
    unsigned primary_pairs = 4;
    unsigned index;
    trace_result_t warmup;

    if (argc == 3) {
        control_pairs = (unsigned)strtoul(argv[1], NULL, 10);
        primary_pairs = (unsigned)strtoul(argv[2], NULL, 10);
    } else if (argc != 1) {
        fprintf(stderr, "usage: %s [CONTROL_PAIRS PRIMARY_PAIRS]\n", argv[0]);
        return 2;
    }
    if (control_pairs == 0 || primary_pairs == 0 ||
        primary_pairs > sizeof(primary_seeds) / sizeof(primary_seeds[0]))
        return 2;
    if (!load_kat())
        return 3;
    initialize_trace_buffers();
    memset(&warmup, 0, sizeof(warmup));
    if (!run_trace(0, &warmup))
        return 4;
    free_trace_result(&warmup);

    printf("dataset,pair,class,execution_order,seed,guard_count,edge_events,"
           "edge_unique,edge_hash1,edge_hash2,edge_capture_events,"
           "edge_capture_truncated,load_events,store_events,memory_hash1,"
           "memory_hash2,memory_capture_events,memory_capture_truncated,"
           "signature_fnv64,pair_edge_equal,pair_memory_equal,"
           "first_edge_diff_index,first_edge_a_pc,first_edge_b_pc,"
           "first_memory_diff_index,first_memory_a_event,"
           "first_memory_b_event,image_base,status\n");
    for (index = 0; index < control_pairs; ++index) {
        if (!run_pair("control", index, 0))
            return 5;
    }
    for (index = 0; index < primary_pairs; ++index) {
        if (!run_pair("fixed-random", index, primary_seeds[index]))
            return 6;
    }
    return 0;
}
