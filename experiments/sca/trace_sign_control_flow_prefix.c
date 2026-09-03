// SPDX-License-Identifier: Apache-2.0
/* Bounded-prefix companion for trace_sign_control_flow.c.
 *
 * The full protected signer executes billions of instrumented basic blocks.
 * This harness records the first N edge events, then zeros every coverage
 * guard so the unchanged Sign operation can finish and be verified without
 * further callback traffic. Guards are restored before the next paired run.
 * The result is explicitly a prefix screen, not a whole-execution proof.
 */

#define __sanitizer_cov_trace_pc_guard_init \
    sqisign_prefix_base_trace_pc_guard_init
#define __sanitizer_cov_trace_pc_guard \
    sqisign_prefix_base_trace_pc_guard
#define main sqisign_prefix_frozen_main
#include "trace_sign_control_flow.c"
#undef main
#undef __sanitizer_cov_trace_pc_guard
#undef __sanitizer_cov_trace_pc_guard_init

enum {
    MAX_GUARD_MODULES = 4096,
};

typedef struct {
    uint32_t *start;
    uint32_t *stop;
} guard_module_t;

typedef struct {
    const uintptr_t *begin;
    size_t count;
} pc_table_range_t;

static guard_module_t guard_modules[MAX_GUARD_MODULES];
static size_t guard_module_count;
static pc_table_range_t pc_table_ranges[MAX_GUARD_MODULES];
static size_t pc_table_range_count;
static int guard_registration_error;
static int guards_disarmed;
static uintptr_t *guard_site_pc;

void
__sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop)
{
    size_t index;
    sqisign_prefix_base_trace_pc_guard_init(start, stop);
    if (start == stop)
        return;
    for (index = 0; index < guard_module_count; ++index) {
        if (guard_modules[index].start == start)
            return;
    }
    if (guard_module_count == MAX_GUARD_MODULES) {
        guard_registration_error = 1;
        return;
    }
    guard_modules[guard_module_count].start = start;
    guard_modules[guard_module_count].stop = stop;
    ++guard_module_count;
}

void
__sanitizer_cov_pcs_init(const uintptr_t *pcs_begin,
                         const uintptr_t *pcs_end)
{
    size_t count;
    size_t index;
    if (pcs_end < pcs_begin || ((size_t)(pcs_end - pcs_begin) & 1U) != 0) {
        guard_registration_error = 1;
        return;
    }
    count = (size_t)(pcs_end - pcs_begin) / 2U;
    for (index = 0; index < pc_table_range_count; ++index) {
        if (pc_table_ranges[index].begin == pcs_begin)
            return;
    }
    if (pc_table_range_count == MAX_GUARD_MODULES) {
        guard_registration_error = 1;
        return;
    }
    pc_table_ranges[pc_table_range_count].begin = pcs_begin;
    pc_table_ranges[pc_table_range_count].count = count;
    ++pc_table_range_count;
}

static int
build_guard_site_map(void)
{
    size_t range;
    uint32_t id = 0;
    if (guard_registration_error || pc_table_range_count == 0)
        return 0;
    guard_site_pc = calloc((size_t)guard_count + 1,
                           sizeof(*guard_site_pc));
    if (guard_site_pc == NULL)
        return 0;
    for (range = 0; range < pc_table_range_count; ++range) {
        size_t index;
        for (index = 0; index < pc_table_ranges[range].count; ++index) {
            if (++id > guard_count)
                return 0;
            guard_site_pc[id] = pc_table_ranges[range].begin[2U * index];
        }
    }
    return id == guard_count;
}

static int
rearm_guards(void)
{
    size_t module;
    uint32_t id = 0;
    if (guard_registration_error || guard_module_count == 0)
        return 0;
    for (module = 0; module < guard_module_count; ++module) {
        uint32_t *cursor;
        for (cursor = guard_modules[module].start;
             cursor < guard_modules[module].stop; ++cursor)
            *cursor = ++id;
    }
    guards_disarmed = 0;
    return id == guard_count;
}

static void
disarm_guards(void)
{
    size_t module;
    for (module = 0; module < guard_module_count; ++module) {
        uint32_t *cursor;
        for (cursor = guard_modules[module].start;
             cursor < guard_modules[module].stop; ++cursor)
            *cursor = 0;
    }
}

void
__sanitizer_cov_trace_pc_guard(uint32_t *guard)
{
    sqisign_prefix_base_trace_pc_guard(guard);
    if (trace_enabled && !guards_disarmed &&
        edge_capture_length >= edge_capture_capacity) {
        edge_capture_truncated = 1;
        disarm_guards();
        guards_disarmed = 1;
    }
}

static int
run_prefix_trace(uint64_t seed, trace_result_t *result)
{
    if (!rearm_guards())
        return 0;
    return run_trace(seed, result);
}

static uintptr_t
edge_site_pc_at(const trace_result_t *result, int64_t index)
{
    uint32_t id;
    if (index < 0 || (uint64_t)index >= result->edge_capture_length)
        return 0;
    id = result->edge_capture[(size_t)index];
    return id <= guard_count ? guard_site_pc[id] : 0;
}

static int
run_prefix_pair(const char *dataset, unsigned pair, uint64_t seed_b)
{
    trace_result_t a;
    trace_result_t b;
    int ok;
    int edges_same;
    int64_t edge_difference;
    uintptr_t edge_a_pc;
    uintptr_t edge_b_pc;
    const unsigned a_order = (pair % 2U == 0U) ? 0U : 1U;
    const unsigned b_order = 1U - a_order;

    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    if (a_order == 0U)
        ok = run_prefix_trace(0, &a) && run_prefix_trace(seed_b, &b);
    else
        ok = run_prefix_trace(seed_b, &b) && run_prefix_trace(0, &a);
    if (!ok)
        goto failure;
    edges_same = edge_equal(&a, &b);
    edge_difference = first_edge_difference(&a, &b);
    edge_a_pc = edge_site_pc_at(&a, edge_difference);
    edge_b_pc = edge_site_pc_at(&b, edge_difference);
    emit_result(dataset, pair, "A", a_order, 0, &a, edges_same, 1,
                edge_difference, edge_a_pc, edge_b_pc, -1, 0, 0);
    emit_result(dataset, pair, "B", b_order, seed_b, &b, edges_same, 1,
                edge_difference, edge_a_pc, edge_b_pc, -1, 0, 0);
    free_trace_result(&a);
    free_trace_result(&b);
    return 1;

failure:
    free_trace_result(&a);
    free_trace_result(&b);
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
        !load_kat() || !rearm_guards() || !build_guard_site_map())
        return 3;
    initialize_trace_buffers();

    printf("dataset,pair,class,execution_order,seed,guard_count,edge_events,"
           "edge_unique,edge_hash1,edge_hash2,edge_capture_events,"
           "edge_capture_truncated,load_events,store_events,memory_hash1,"
           "memory_hash2,memory_capture_events,memory_capture_truncated,"
           "signature_fnv64,pair_edge_equal,pair_memory_equal,"
           "first_edge_diff_index,first_edge_a_pc,first_edge_b_pc,"
           "first_memory_diff_index,first_memory_a_event,"
           "first_memory_b_event,image_base,status\n");
    for (index = 0; index < control_pairs; ++index) {
        if (!run_prefix_pair("control", index, 0))
            return 4;
        fflush(stdout);
    }
    for (index = 0; index < primary_pairs; ++index) {
        if (!run_prefix_pair("fixed-random", index, primary_seeds[index]))
            return 5;
        fflush(stdout);
    }
    return 0;
}
