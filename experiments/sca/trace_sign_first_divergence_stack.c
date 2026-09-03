// SPDX-License-Identifier: Apache-2.0
/* Host-only caller-stack localization for the frozen first Sign edge deltas.
 *
 * The broader and PC-table harnesses establish the event indices and exact
 * integer source decisions.  This companion replays those indices and takes
 * one native backtrace on each side, so remediation can be scoped to the
 * higher-level caller path instead of replacing unrelated integer users.
 */

#include <execinfo.h>

#define __sanitizer_cov_trace_pc_guard \
    sqisign_stack_trace_pc_guard_base
#define main sqisign_stack_frozen_main
#include "trace_sign_control_flow.c"
#undef main
#undef __sanitizer_cov_trace_pc_guard

enum {
    STACK_FRAME_CAPACITY = 64,
};

static uint64_t stack_target_index;
static int stack_capture_armed;
static int stack_capture_done;
static uint32_t stack_capture_guard;
static uintptr_t stack_capture_callback_pc;
static void *stack_frames[STACK_FRAME_CAPACITY];
static int stack_frame_count;

void
__sanitizer_cov_trace_pc_guard(uint32_t *guard)
{
    sqisign_stack_trace_pc_guard_base(guard);
    if (!stack_capture_armed || stack_capture_done || !trace_enabled ||
        edge_events == 0 || edge_events - 1U != stack_target_index) {
        return;
    }
    stack_capture_guard = *guard;
    stack_capture_callback_pc =
        (uintptr_t)__builtin_return_address(0);
    stack_frame_count = backtrace(stack_frames, STACK_FRAME_CAPACITY);
    stack_capture_done = 1;
}

static int
run_stack_trace(unsigned pair,
                const char *class_name,
                uint64_t seed,
                uint64_t target_index)
{
    trace_result_t result;
    int frame;

    memset(&result, 0, sizeof(result));
    stack_target_index = target_index;
    stack_capture_armed = 1;
    stack_capture_done = 0;
    stack_capture_guard = 0;
    stack_capture_callback_pc = 0;
    stack_frame_count = 0;
    memset(stack_frames, 0, sizeof(stack_frames));
    if (!run_trace(seed, &result)) {
        stack_capture_armed = 0;
        free_trace_result(&result);
        return 0;
    }
    stack_capture_armed = 0;
    if (!stack_capture_done || stack_capture_guard == 0 ||
        stack_frame_count < 3) {
        free_trace_result(&result);
        return 0;
    }
    for (frame = 0; frame < stack_frame_count; ++frame) {
        printf("%u,%s,%" PRIu64 ",%" PRIu64 ",%u,%016" PRIxPTR
               ",%d,%016" PRIxPTR ",%016" PRIxPTR ",%016" PRIx64
               ",PASS\n",
               pair, class_name, seed, target_index, stack_capture_guard,
               stack_capture_callback_pc, frame,
               (uintptr_t)stack_frames[frame], image_base,
               result.signature_hash);
    }
    free_trace_result(&result);
    return 1;
}

int
main(void)
{
    static const uint64_t primary_seeds[] = {1, 2, 3, 5};
    static const uint64_t first_difference_indices[] = {
        938604, 935361, 938604, 933275,
    };
    trace_result_t warmup;
    unsigned pair;

    if (!load_kat())
        return 2;
    initialize_trace_buffers();
    memset(&warmup, 0, sizeof(warmup));
    stack_capture_armed = 0;
    if (!run_trace(0, &warmup))
        return 3;
    free_trace_result(&warmup);

    printf("pair,class,seed,target_event_index,guard,callback_pc,"
           "frame_index,frame_pc,image_base,signature_fnv64,status\n");
    for (pair = 0; pair < 4; ++pair) {
        if (!run_stack_trace(pair, "A", 0,
                             first_difference_indices[pair]) ||
            !run_stack_trace(pair, "B", primary_seeds[pair],
                             first_difference_indices[pair])) {
            return 4;
        }
    }
    return 0;
}
