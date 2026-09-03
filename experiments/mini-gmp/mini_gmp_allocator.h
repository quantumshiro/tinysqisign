#ifndef SQISIGN_MINI_GMP_ALLOCATOR_H
#define SQISIGN_MINI_GMP_ALLOCATOR_H

#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>

typedef enum mini_gmp_allocator_mode {
    MINI_GMP_ALLOCATOR_NATIVE = 0,
    MINI_GMP_ALLOCATOR_TRACKING = 1,
    MINI_GMP_ALLOCATOR_POOL = 2
} mini_gmp_allocator_mode_t;

typedef struct mini_gmp_allocator_stats {
    uint64_t allocation_calls;
    uint64_t reallocation_calls;
    uint64_t pool_relocation_allocations;
    uint64_t free_calls;
    uint64_t failed_calls;
    uint64_t size_mismatches;
    size_t current_allocations;
    size_t peak_allocations;
    size_t current_requested_bytes;
    size_t peak_requested_bytes;
    size_t conservative_relocation_peak_bytes;
    size_t total_requested_bytes;
    size_t largest_request_bytes;
    size_t current_physical_bytes;
    size_t peak_physical_bytes;
    size_t pool_capacity_bytes;
    size_t pool_high_water_bytes;
    size_t secure_clear_bytes;
} mini_gmp_allocator_stats_t;

int mini_gmp_allocator_select(mini_gmp_allocator_mode_t mode, size_t pool_bytes);
int mini_gmp_allocator_reset(void);
void mini_gmp_allocator_arm_oom(jmp_buf *target);
void mini_gmp_allocator_disarm_oom(void);
const mini_gmp_allocator_stats_t *mini_gmp_allocator_stats(void);
const char *mini_gmp_allocator_mode_name(mini_gmp_allocator_mode_t mode);
size_t mini_gmp_allocator_pool_max_bytes(void);

#endif
