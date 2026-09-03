// SPDX-License-Identifier: Apache-2.0
/* Host-only caller-stack localization at a requested Sign edge-event index.
 *
 * The bounded-prefix screen supplies a deterministic first-difference index.
 * This companion captures one native stack at that index for the fixed seed
 * and one for a requested comparison seed.  Tracing is disabled immediately
 * after capture so the unchanged Sign and Verify operations can finish
 * without billions of additional callbacks.
 */

#include <execinfo.h>

#define __sanitizer_cov_trace_pc_guard_init \
    sqisign_target_trace_pc_guard_init_base
#define __sanitizer_cov_trace_pc_guard sqisign_target_trace_pc_guard_base
#define main sqisign_target_frozen_main
#include "trace_sign_control_flow.c"
#undef main
#undef __sanitizer_cov_trace_pc_guard
#undef __sanitizer_cov_trace_pc_guard_init

enum {
    TARGET_STACK_FRAME_CAPACITY = 64,
    TARGET_GUARD_MODULE_CAPACITY = 4096,
};

typedef struct {
    uint32_t *start;
    uint32_t *stop;
} target_guard_module_t;

static uint64_t target_event_index;
static int target_capture_armed;
static int target_capture_done;
static uint32_t target_capture_guard;
static uintptr_t target_capture_callback_pc;
static void *target_stack_frames[TARGET_STACK_FRAME_CAPACITY];
static int target_stack_frame_count;
static target_guard_module_t
    target_guard_modules[TARGET_GUARD_MODULE_CAPACITY];
static size_t target_guard_module_count;
static int target_guard_registration_error;

void
__sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop)
{
    size_t index;
    sqisign_target_trace_pc_guard_init_base(start, stop);
    if (start == stop)
        return;
    for (index = 0; index < target_guard_module_count; ++index) {
        if (target_guard_modules[index].start == start)
            return;
    }
    if (target_guard_module_count == TARGET_GUARD_MODULE_CAPACITY) {
        target_guard_registration_error = 1;
        return;
    }
    target_guard_modules[target_guard_module_count].start = start;
    target_guard_modules[target_guard_module_count].stop = stop;
    ++target_guard_module_count;
}

static int
target_rearm_guards(void)
{
    size_t module;
    uint32_t id = 0;
    if (target_guard_registration_error || target_guard_module_count == 0)
        return 0;
    for (module = 0; module < target_guard_module_count; ++module) {
        uint32_t *cursor;
        for (cursor = target_guard_modules[module].start;
             cursor < target_guard_modules[module].stop; ++cursor)
            *cursor = ++id;
    }
    return id == guard_count;
}

static void
target_disarm_guards(void)
{
    size_t module;
    for (module = 0; module < target_guard_module_count; ++module) {
        uint32_t *cursor;
        for (cursor = target_guard_modules[module].start;
             cursor < target_guard_modules[module].stop; ++cursor)
            *cursor = 0;
    }
}

void
__sanitizer_cov_trace_pc_guard(uint32_t *guard)
{
    sqisign_target_trace_pc_guard_base(guard);
    if (!target_capture_armed || target_capture_done || !trace_enabled ||
        edge_events == 0 || edge_events - 1U != target_event_index) {
        return;
    }
    target_capture_guard = *guard;
    target_capture_callback_pc = (uintptr_t)__builtin_return_address(0);
    target_stack_frame_count =
        backtrace(target_stack_frames, TARGET_STACK_FRAME_CAPACITY);
    target_capture_done = 1;
    trace_enabled = 0;
    target_disarm_guards();
}

static int
run_target_trace(const char *class_name, uint64_t seed)
{
    trace_result_t result;
    int frame;

    memset(&result, 0, sizeof(result));
    target_capture_armed = 1;
    target_capture_done = 0;
    target_capture_guard = 0;
    target_capture_callback_pc = 0;
    target_stack_frame_count = 0;
    memset(target_stack_frames, 0, sizeof(target_stack_frames));
    if (!target_rearm_guards() || !run_trace(seed, &result)) {
        target_capture_armed = 0;
        free_trace_result(&result);
        return 0;
    }
    target_capture_armed = 0;
    if (!target_capture_done || target_capture_guard == 0 ||
        target_stack_frame_count < 3) {
        free_trace_result(&result);
        return 0;
    }
    for (frame = 0; frame < target_stack_frame_count; ++frame) {
        printf("%s,%" PRIu64 ",%" PRIu64 ",%u,%016" PRIxPTR
               ",%d,%016" PRIxPTR ",%016" PRIxPTR ",%016" PRIx64
               ",PASS\n",
               class_name, seed, target_event_index, target_capture_guard,
               target_capture_callback_pc, frame,
               (uintptr_t)target_stack_frames[frame], image_base,
               result.signature_hash);
    }
    free_trace_result(&result);
    return 1;
}

int
main(int argc, char **argv)
{
    char *end = NULL;
    uint64_t comparison_seed;

    if (argc != 3) {
        fprintf(stderr, "usage: %s TARGET_EVENT_INDEX COMPARISON_SEED\n",
                argv[0]);
        return 2;
    }
    target_event_index = strtoull(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0')
        return 2;
    comparison_seed = strtoull(argv[2], &end, 10);
    if (end == argv[2] || *end != '\0')
        return 2;
    if (!load_kat())
        return 3;
    initialize_trace_buffers();

    printf("class,seed,target_event_index,guard,callback_pc,frame_index,"
           "frame_pc,image_base,signature_fnv64,status\n");
    if (!run_target_trace("A", 0) ||
        !run_target_trace("B", comparison_seed)) {
        return 4;
    }
    return 0;
}
