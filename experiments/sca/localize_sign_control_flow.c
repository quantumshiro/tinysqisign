// SPDX-License-Identifier: Apache-2.0
/* Host-only localization companion for trace_sign_control_flow.c.
 *
 * This translation unit reuses the frozen Sign fixture and trace machinery,
 * but adds Clang SanitizerCoverage's PC table. The PC table associates each
 * guard ID with the exact instrumented basic-block PC; the callback return PC
 * used by the frozen screen may be shared by multiple guards.
 */

#define __sanitizer_cov_trace_pc_guard_init \
    sqisign_localization_trace_pc_guard_init_base
#define main sqisign_localization_frozen_main
#include "trace_sign_control_flow.c"
#undef main
#undef __sanitizer_cov_trace_pc_guard_init

enum {
    MAX_PC_TABLE_MODULES = 4096,
};

typedef struct {
    uint32_t *guards_begin;
    uint32_t first_guard_id;
    uint32_t guard_count;
} pc_table_module_t;

typedef struct {
    const uintptr_t *pcs_begin;
    size_t pc_count;
} pc_table_range_t;

static pc_table_module_t pc_table_modules[MAX_PC_TABLE_MODULES];
static size_t pc_table_module_count;
static pc_table_range_t pc_table_ranges[MAX_PC_TABLE_MODULES];
static size_t pc_table_range_count;
static int pc_table_registration_error;
static uintptr_t *guard_site_pc;
static uintptr_t *guard_site_flags;

void
__sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop)
{
    size_t index;

    sqisign_localization_trace_pc_guard_init_base(start, stop);
    if (start == stop)
        return;
    for (index = 0; index < pc_table_module_count; ++index) {
        if (pc_table_modules[index].guards_begin == start)
            return;
    }
    if (pc_table_module_count == MAX_PC_TABLE_MODULES || *start == 0) {
        pc_table_registration_error = 1;
        return;
    }
    pc_table_modules[pc_table_module_count].guards_begin = start;
    pc_table_modules[pc_table_module_count].first_guard_id = *start;
    pc_table_modules[pc_table_module_count].guard_count =
        (uint32_t)(stop - start);
    ++pc_table_module_count;
}

void
__sanitizer_cov_pcs_init(const uintptr_t *pcs_begin,
                         const uintptr_t *pcs_end)
{
    size_t count;
    size_t index;

    if (pcs_end < pcs_begin || ((size_t)(pcs_end - pcs_begin) & 1U) != 0) {
        pc_table_registration_error = 1;
        return;
    }
    count = (size_t)(pcs_end - pcs_begin) / 2U;
    for (index = 0; index < pc_table_range_count; ++index) {
        if (pc_table_ranges[index].pcs_begin == pcs_begin) {
            if (pc_table_ranges[index].pc_count != count)
                pc_table_registration_error = 1;
            return;
        }
    }
    if (pc_table_range_count == MAX_PC_TABLE_MODULES) {
        pc_table_registration_error = 1;
        return;
    }
    pc_table_ranges[pc_table_range_count].pcs_begin = pcs_begin;
    pc_table_ranges[pc_table_range_count].pc_count = count;
    ++pc_table_range_count;
}

static int
build_guard_site_map(void)
{
    size_t range_index;
    uint32_t next_guard_id = 1;
    uint64_t mapped = 0;

    if (pc_table_registration_error || pc_table_module_count == 0 ||
        pc_table_range_count == 0) {
        fprintf(stderr,
                "PC-table registration mismatch: guards=%zu pcs=%zu "
                "error=%d\n",
                pc_table_module_count, pc_table_range_count,
                pc_table_registration_error);
        return 0;
    }
    guard_site_pc = calloc((size_t)guard_count + 1, sizeof(*guard_site_pc));
    guard_site_flags = calloc((size_t)guard_count + 1,
                              sizeof(*guard_site_flags));
    if (guard_site_pc == NULL || guard_site_flags == NULL)
        return 0;
    for (range_index = 0; range_index < pc_table_range_count;
         ++range_index) {
        const pc_table_range_t *range = &pc_table_ranges[range_index];
        size_t pc_index;
        if (range->pcs_begin == NULL)
            return 0;
        for (pc_index = 0; pc_index < range->pc_count; ++pc_index) {
            const uint32_t id = next_guard_id++;
            if (id == 0 || id > guard_count || guard_site_pc[id] != 0) {
                fprintf(stderr,
                        "PC-table guard mismatch: range=%zu index=%zu "
                        "id=%u guards=%u existing=%" PRIxPTR "\n",
                        range_index, pc_index, id, guard_count,
                        id <= guard_count ? guard_site_pc[id] : 0);
                return 0;
            }
            guard_site_pc[id] = range->pcs_begin[2U * pc_index];
            guard_site_flags[id] = range->pcs_begin[2U * pc_index + 1U];
            ++mapped;
        }
    }
    if (mapped != guard_count) {
        fprintf(stderr,
                "PC-table entry mismatch: mapped=%" PRIu64 " guards=%u\n",
                mapped, guard_count);
        return 0;
    }
    return 1;
}

static uint32_t
edge_guard_at(const trace_result_t *result, int64_t index)
{
    if (index < 0 || (uint64_t)index >= result->edge_capture_length)
        return 0;
    return result->edge_capture[(size_t)index];
}

static uintptr_t
site_pc(uint32_t guard)
{
    if (guard == 0 || guard > guard_count)
        return 0;
    return guard_site_pc[guard];
}

static uintptr_t
site_flags(uint32_t guard)
{
    if (guard == 0 || guard > guard_count)
        return 0;
    return guard_site_flags[guard];
}

static int
run_localization_pair(const char *dataset, unsigned pair, uint64_t seed_b)
{
    trace_result_t a;
    trace_result_t b;
    int64_t difference;
    uint32_t previous_guard;
    uint32_t a_guard;
    uint32_t b_guard;
    int edges_same;
    int signatures_same;
    const unsigned a_order = (pair % 2U == 0U) ? 0U : 1U;
    const unsigned b_order = 1U - a_order;

    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    if (a_order == 0U) {
        if (!run_trace(0, &a) || !run_trace(seed_b, &b))
            goto failure;
    } else {
        if (!run_trace(seed_b, &b) || !run_trace(0, &a))
            goto failure;
    }
    edges_same = edge_equal(&a, &b);
    signatures_same = a.signature_hash == b.signature_hash;
    difference = first_edge_difference(&a, &b);
    previous_guard = edge_guard_at(&a, difference - 1);
    a_guard = edge_guard_at(&a, difference);
    b_guard = edge_guard_at(&b, difference);

    if ((strcmp(dataset, "control") == 0 &&
         (!edges_same || !signatures_same || difference != -1)) ||
        (strcmp(dataset, "fixed-random") == 0 &&
         (edges_same || signatures_same || difference < 0 ||
          a_guard == 0 || b_guard == 0)))
        goto failure;

    printf("%s,%u,%u,%u,0,%" PRIu64 ",%d,%d,%" PRId64
           ",%u,%u,%u,%016" PRIxPTR ",%016" PRIxPTR
           ",%016" PRIxPTR ",%016" PRIxPTR ",%016" PRIxPTR
           ",%016" PRIxPTR ",%016" PRIxPTR ",%016" PRIxPTR
           ",%016" PRIxPTR ",%016" PRIx64 ",%016" PRIx64
           ",%" PRIu64 ",%" PRIu64 ",%016" PRIxPTR ",PASS\n",
           dataset, pair, a_order, b_order, seed_b, edges_same,
           signatures_same, difference, previous_guard, a_guard, b_guard,
           site_pc(previous_guard), site_pc(a_guard), site_pc(b_guard),
           site_flags(previous_guard), site_flags(a_guard),
           site_flags(b_guard), edge_pc_at(&a, difference - 1),
           edge_pc_at(&a, difference), edge_pc_at(&b, difference),
           a.signature_hash, b.signature_hash, a.edge_events, b.edge_events,
           image_base);
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
    static const uint64_t primary_seeds[] = {1, 2, 3, 5};
    unsigned control_pairs = 1;
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
    if (control_pairs == 0 || primary_pairs == 0 || primary_pairs > 4 ||
        !load_kat() || !build_guard_site_map())
        return 3;
    initialize_trace_buffers();
    memset(&warmup, 0, sizeof(warmup));
    if (!run_trace(0, &warmup))
        return 4;
    free_trace_result(&warmup);

    printf("dataset,pair,a_order,b_order,seed_a,seed_b,pair_edge_equal,"
           "signature_equal,first_edge_diff_index,previous_guard,a_guard,"
           "b_guard,previous_site_pc,a_site_pc,b_site_pc,"
           "previous_site_flags,a_site_flags,b_site_flags,"
           "previous_callback_pc,a_callback_pc,b_callback_pc,"
           "signature_a_fnv64,signature_b_fnv64,edge_events_a,"
           "edge_events_b,image_base,status\n");
    for (index = 0; index < control_pairs; ++index) {
        if (!run_localization_pair("control", index, 0))
            return 5;
    }
    for (index = 0; index < primary_pairs; ++index) {
        if (!run_localization_pair("fixed-random", index,
                                   primary_seeds[index]))
            return 6;
    }
    return 0;
}
