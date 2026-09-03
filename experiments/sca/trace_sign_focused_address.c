// SPDX-License-Identifier: Apache-2.0
/* Selected-function Sign control-flow/address trace harness.
 *
 * The build instruments only functions in
 * combined-focused-address-allowlist.txt.  Besides each effective address,
 * this companion records the callback return PC so the first address-sequence
 * difference can be mapped to a source line.  This is a host structural
 * screen, not a value/power model or a side-channel-resistance proof.
 */

#define __sanitizer_cov_load1 sqisign_focused_base_load1
#define __sanitizer_cov_load2 sqisign_focused_base_load2
#define __sanitizer_cov_load4 sqisign_focused_base_load4
#define __sanitizer_cov_load8 sqisign_focused_base_load8
#define __sanitizer_cov_load16 sqisign_focused_base_load16
#define __sanitizer_cov_store1 sqisign_focused_base_store1
#define __sanitizer_cov_store2 sqisign_focused_base_store2
#define __sanitizer_cov_store4 sqisign_focused_base_store4
#define __sanitizer_cov_store8 sqisign_focused_base_store8
#define __sanitizer_cov_store16 sqisign_focused_base_store16
#define main sqisign_focused_frozen_main
#include "trace_sign_control_flow.c"
#undef main
#undef __sanitizer_cov_store16
#undef __sanitizer_cov_store8
#undef __sanitizer_cov_store4
#undef __sanitizer_cov_store2
#undef __sanitizer_cov_store1
#undef __sanitizer_cov_load16
#undef __sanitizer_cov_load8
#undef __sanitizer_cov_load4
#undef __sanitizer_cov_load2
#undef __sanitizer_cov_load1

typedef struct {
    trace_result_t trace;
    uintptr_t *memory_site_pc;
} focused_trace_result_t;

static uintptr_t *focused_memory_site_buffer;

static void
record_focused_memory(uintptr_t address,
                      unsigned width,
                      unsigned is_store,
                      uintptr_t site_pc)
{
    const size_t index = memory_capture_length;
    if (!trace_enabled)
        return;
    record_memory(address, width, is_store);
    if (index < memory_capture_capacity &&
        memory_capture_length == index + 1)
        focused_memory_site_buffer[index] = site_pc;
    if (memory_capture_length >= memory_capture_capacity) {
        memory_capture_truncated = 1;
        trace_enabled = 0;
    }
}

#define DEFINE_FOCUSED_MEMORY_CALLBACKS(bits, bytes)                    \
    __attribute__((noinline))                                           \
    void __sanitizer_cov_load##bits(uintptr_t address)                  \
    {                                                                   \
        record_focused_memory(address, bytes, 0,                        \
                              (uintptr_t)__builtin_return_address(0));  \
    }                                                                   \
    __attribute__((noinline))                                           \
    void __sanitizer_cov_store##bits(uintptr_t address)                 \
    {                                                                   \
        record_focused_memory(address, bytes, 1,                        \
                              (uintptr_t)__builtin_return_address(0));  \
    }

DEFINE_FOCUSED_MEMORY_CALLBACKS(1, 1)
DEFINE_FOCUSED_MEMORY_CALLBACKS(2, 2)
DEFINE_FOCUSED_MEMORY_CALLBACKS(4, 4)
DEFINE_FOCUSED_MEMORY_CALLBACKS(8, 8)
DEFINE_FOCUSED_MEMORY_CALLBACKS(16, 16)

static int
run_focused_trace(uint64_t seed, focused_trace_result_t *result)
{
    memset(result, 0, sizeof(*result));
    if (!run_trace(seed, &result->trace))
        return 0;
    result->memory_site_pc = malloc(
        result->trace.memory_capture_length *
        sizeof(*result->memory_site_pc));
    if (result->trace.memory_capture_length != 0 &&
        result->memory_site_pc == NULL)
        return 0;
    memcpy(result->memory_site_pc, focused_memory_site_buffer,
           result->trace.memory_capture_length *
           sizeof(*result->memory_site_pc));
    return 1;
}

static void
free_focused_trace_result(focused_trace_result_t *result)
{
    free(result->memory_site_pc);
    free_trace_result(&result->trace);
    memset(result, 0, sizeof(*result));
}

static uintptr_t
memory_site_at(const focused_trace_result_t *result, int64_t index)
{
    if (index < 0 ||
        (uint64_t)index >= result->trace.memory_capture_length)
        return 0;
    return result->memory_site_pc[(size_t)index];
}

static void
emit_focused_result(const char *dataset,
                    unsigned pair,
                    const char *class_name,
                    unsigned execution_order,
                    uint64_t seed,
                    const focused_trace_result_t *focused,
                    int pair_edge_equal,
                    int pair_memory_equal,
                    int64_t edge_difference,
                    uintptr_t edge_a_pc,
                    uintptr_t edge_b_pc,
                    int64_t memory_difference,
                    uint64_t memory_a_event,
                    uint64_t memory_b_event,
                    uintptr_t memory_a_site_pc,
                    uintptr_t memory_b_site_pc)
{
    const trace_result_t *result = &focused->trace;
    printf("%s,%u,%s,%u,%" PRIu64 ",%u,%" PRIu64 ",%" PRIu64
           ",%016" PRIx64 ",%016" PRIx64 ",%zu,%d,%" PRIu64
           ",%" PRIu64 ",%016" PRIx64 ",%016" PRIx64 ",%zu,%d,"
           "%016" PRIx64 ",%d,%d,%" PRId64 ",%016" PRIxPTR
           ",%016" PRIxPTR ",%" PRId64 ",%016" PRIx64
           ",%016" PRIx64 ",%016" PRIxPTR ",%016" PRIxPTR
           ",%016" PRIxPTR ",PASS\n",
           dataset, pair, class_name, execution_order, seed, guard_count,
           result->edge_events, result->edge_unique, result->edge_hash1,
           result->edge_hash2, result->edge_capture_length,
           result->edge_capture_truncated, result->load_events,
           result->store_events, result->memory_hash1, result->memory_hash2,
           result->memory_capture_length, result->memory_capture_truncated,
           result->signature_hash, pair_edge_equal, pair_memory_equal,
           edge_difference, edge_a_pc, edge_b_pc, memory_difference,
           memory_a_event, memory_b_event, memory_a_site_pc,
           memory_b_site_pc, image_base);
}

static int
run_focused_pair(const char *dataset, unsigned pair, uint64_t seed_b)
{
    focused_trace_result_t a;
    focused_trace_result_t b;
    int ok;
    int edges_same;
    int memory_same;
    int64_t edge_difference;
    int64_t memory_difference;
    uintptr_t edge_a_pc;
    uintptr_t edge_b_pc;
    uint64_t memory_a_event;
    uint64_t memory_b_event;
    uintptr_t memory_a_site_pc;
    uintptr_t memory_b_site_pc;
    const unsigned a_order = (pair % 2U == 0U) ? 0U : 1U;
    const unsigned b_order = 1U - a_order;

    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    if (a_order == 0U)
        ok = run_focused_trace(0, &a) &&
             run_focused_trace(seed_b, &b);
    else
        ok = run_focused_trace(seed_b, &b) &&
             run_focused_trace(0, &a);
    if (!ok)
        goto failure;
    edges_same = edge_equal(&a.trace, &b.trace);
    memory_same = memory_equal(&a.trace, &b.trace);
    edge_difference = first_edge_difference(&a.trace, &b.trace);
    memory_difference = first_memory_difference(&a.trace, &b.trace);
    edge_a_pc = edge_pc_at(&a.trace, edge_difference);
    edge_b_pc = edge_pc_at(&b.trace, edge_difference);
    memory_a_event = memory_event_at(&a.trace, memory_difference);
    memory_b_event = memory_event_at(&b.trace, memory_difference);
    memory_a_site_pc = memory_site_at(&a, memory_difference);
    memory_b_site_pc = memory_site_at(&b, memory_difference);
    emit_focused_result(dataset, pair, "A", a_order, 0, &a, edges_same,
                        memory_same, edge_difference, edge_a_pc, edge_b_pc,
                        memory_difference, memory_a_event, memory_b_event,
                        memory_a_site_pc, memory_b_site_pc);
    emit_focused_result(dataset, pair, "B", b_order, seed_b, &b,
                        edges_same, memory_same, edge_difference, edge_a_pc,
                        edge_b_pc, memory_difference, memory_a_event,
                        memory_b_event, memory_a_site_pc, memory_b_site_pc);
    free_focused_trace_result(&a);
    free_focused_trace_result(&b);
    return 1;

failure:
    free_focused_trace_result(&a);
    free_focused_trace_result(&b);
    return 0;
}

int
main(int argc, char **argv)
{
    static const uint64_t primary_seeds[] = {
        1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233,
    };
    unsigned control_pairs = 1;
    unsigned primary_pairs = 4;
    unsigned index;

    if (argc == 3) {
        control_pairs = (unsigned)strtoul(argv[1], NULL, 10);
        primary_pairs = (unsigned)strtoul(argv[2], NULL, 10);
    } else if (argc != 1) {
        fprintf(stderr, "usage: %s [CONTROL_PAIRS PRIMARY_PAIRS]\n", argv[0]);
        return 2;
    }
    if (control_pairs == 0 || primary_pairs == 0 ||
        primary_pairs > sizeof(primary_seeds) / sizeof(primary_seeds[0]) ||
        !load_kat())
        return 3;
    initialize_trace_buffers();
    focused_memory_site_buffer = calloc(
        memory_capture_capacity, sizeof(*focused_memory_site_buffer));
    if (focused_memory_site_buffer == NULL || guard_count == 0)
        return 4;

    printf("dataset,pair,class,execution_order,seed,guard_count,edge_events,"
           "edge_unique,edge_hash1,edge_hash2,edge_capture_events,"
           "edge_capture_truncated,load_events,store_events,memory_hash1,"
           "memory_hash2,memory_capture_events,memory_capture_truncated,"
           "signature_fnv64,pair_edge_equal,pair_memory_equal,"
           "first_edge_diff_index,first_edge_a_pc,first_edge_b_pc,"
           "first_memory_diff_index,first_memory_a_event,"
           "first_memory_b_event,first_memory_a_site_pc,"
           "first_memory_b_site_pc,image_base,status\n");
    for (index = 0; index < control_pairs; ++index) {
        if (!run_focused_pair("control", index, 0))
            return 5;
        fflush(stdout);
    }
    for (index = 0; index < primary_pairs; ++index) {
        if (!run_focused_pair(
                "fixed-random", index, primary_seeds[index]))
            return 6;
        fflush(stdout);
    }
    return 0;
}
