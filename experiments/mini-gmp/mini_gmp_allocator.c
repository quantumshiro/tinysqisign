#include "mini_gmp_allocator.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(MINI_GMP)
#include <mini-gmp.h>
#else
#include <gmp.h>
#endif

#ifndef MINI_GMP_POOL_MAX_BYTES
#define MINI_GMP_POOL_MAX_BYTES 400000u
#endif

#define TRACKING_MAGIC UINT64_C(0x5351494d474d5054)
#define POOL_MAGIC UINT64_C(0x535149504f4f4c42)

typedef union tracking_header tracking_header_t;
union tracking_header {
    max_align_t alignment;
    struct {
        size_t requested;
        uint64_t magic;
    } fields;
};

typedef union pool_block pool_block_t;
union pool_block {
    max_align_t alignment;
    struct {
        size_t capacity;
        size_t requested;
        pool_block_t *previous;
        pool_block_t *next;
        uint64_t magic;
        int is_free;
    } fields;
};

static union {
    max_align_t alignment;
    unsigned char bytes[MINI_GMP_POOL_MAX_BYTES];
} pool_storage;

static mini_gmp_allocator_mode_t selected_mode = MINI_GMP_ALLOCATOR_NATIVE;
static mini_gmp_allocator_stats_t statistics;
static pool_block_t *pool_first;
static jmp_buf *oom_target;

static size_t
alignment_bytes(void)
{
    return _Alignof(max_align_t);
}

static size_t
align_up(size_t value)
{
    const size_t alignment = alignment_bytes();
    if (value > SIZE_MAX - (alignment - 1u)) {
        return 0;
    }
    return (value + alignment - 1u) / alignment * alignment;
}

static size_t
align_down(size_t value)
{
    const size_t alignment = alignment_bytes();
    return value / alignment * alignment;
}

static void
secure_clear(void *pointer, size_t bytes)
{
    volatile unsigned char *cursor = pointer;
    for (size_t i = 0; i < bytes; i++) {
        cursor[i] = 0;
    }
    statistics.secure_clear_bytes += bytes;
}

static void
allocation_failure(void)
{
    statistics.failed_calls++;
    if (oom_target != NULL) {
        longjmp(*oom_target, 1);
    }
    fputs("mini-GMP comparison allocator exhausted\n", stderr);
    abort();
}

static void
record_requested_allocation(size_t requested, size_t physical,
                            int count_allocator_request)
{
    if (count_allocator_request) {
        statistics.allocation_calls++;
        statistics.total_requested_bytes += requested;
        if (requested > statistics.largest_request_bytes) {
            statistics.largest_request_bytes = requested;
        }
    }
    statistics.current_requested_bytes += requested;
    statistics.current_allocations++;
    if (statistics.current_allocations > statistics.peak_allocations) {
        statistics.peak_allocations = statistics.current_allocations;
    }
    if (statistics.current_requested_bytes > statistics.peak_requested_bytes) {
        statistics.peak_requested_bytes = statistics.current_requested_bytes;
    }
    if (statistics.current_requested_bytes >
        statistics.conservative_relocation_peak_bytes) {
        statistics.conservative_relocation_peak_bytes =
            statistics.current_requested_bytes;
    }
    statistics.current_physical_bytes += physical;
    if (statistics.current_physical_bytes > statistics.peak_physical_bytes) {
        statistics.peak_physical_bytes = statistics.current_physical_bytes;
    }
}

static void
record_requested_free(size_t requested, size_t physical)
{
    assert(statistics.current_requested_bytes >= requested);
    assert(statistics.current_physical_bytes >= physical);
    assert(statistics.current_allocations > 0);
    statistics.free_calls++;
    statistics.current_allocations--;
    statistics.current_requested_bytes -= requested;
    statistics.current_physical_bytes -= physical;
}

static void *
tracking_allocate(size_t requested)
{
    tracking_header_t *header;
    if (requested == 0 || requested > SIZE_MAX - sizeof(*header)) {
        allocation_failure();
    }
    header = malloc(sizeof(*header) + requested);
    if (header == NULL) {
        allocation_failure();
    }
    header->fields.requested = requested;
    header->fields.magic = TRACKING_MAGIC;
    record_requested_allocation(requested, sizeof(*header) + requested, 1);
    return header + 1;
}

static void
tracking_free(void *pointer, size_t supplied_size)
{
    tracking_header_t *header;
    if (pointer == NULL) {
        return;
    }
    header = (tracking_header_t *)pointer - 1;
    assert(header->fields.magic == TRACKING_MAGIC);
    if (header->fields.requested != supplied_size) {
        statistics.size_mismatches++;
    }
    record_requested_free(header->fields.requested,
                          sizeof(*header) + header->fields.requested);
    header->fields.magic = 0;
    free(header);
}

static void *
tracking_reallocate(void *pointer, size_t supplied_old_size, size_t new_size)
{
    tracking_header_t *old_header;
    tracking_header_t *new_header;
    size_t old_size;
    size_t relocation_peak;

    if (pointer == NULL) {
        return tracking_allocate(new_size);
    }
    if (new_size == 0) {
        tracking_free(pointer, supplied_old_size);
        return NULL;
    }
    old_header = (tracking_header_t *)pointer - 1;
    assert(old_header->fields.magic == TRACKING_MAGIC);
    old_size = old_header->fields.requested;
    if (old_size != supplied_old_size) {
        statistics.size_mismatches++;
    }
    if (statistics.current_requested_bytes > SIZE_MAX - new_size) {
        allocation_failure();
    }
    relocation_peak = statistics.current_requested_bytes + new_size;
    if (relocation_peak > statistics.conservative_relocation_peak_bytes) {
        statistics.conservative_relocation_peak_bytes = relocation_peak;
    }
    if (new_size > SIZE_MAX - sizeof(*new_header)) {
        allocation_failure();
    }
    new_header = realloc(old_header, sizeof(*new_header) + new_size);
    if (new_header == NULL) {
        allocation_failure();
    }
    statistics.reallocation_calls++;
    statistics.total_requested_bytes += new_size;
    if (new_size > statistics.largest_request_bytes) {
        statistics.largest_request_bytes = new_size;
    }
    assert(statistics.current_requested_bytes >= old_size);
    statistics.current_requested_bytes -= old_size;
    statistics.current_requested_bytes += new_size;
    if (statistics.current_requested_bytes > statistics.peak_requested_bytes) {
        statistics.peak_requested_bytes = statistics.current_requested_bytes;
    }
    assert(statistics.current_physical_bytes >= sizeof(*old_header) + old_size);
    statistics.current_physical_bytes -= sizeof(*old_header) + old_size;
    statistics.current_physical_bytes += sizeof(*new_header) + new_size;
    if (statistics.current_physical_bytes > statistics.peak_physical_bytes) {
        statistics.peak_physical_bytes = statistics.current_physical_bytes;
    }
    new_header->fields.requested = new_size;
    new_header->fields.magic = TRACKING_MAGIC;
    return new_header + 1;
}

static void
pool_set_high_water(const pool_block_t *block)
{
    const unsigned char *end = (const unsigned char *)(block + 1) +
                               block->fields.capacity;
    size_t offset = (size_t)(end - pool_storage.bytes);
    if (offset > statistics.pool_high_water_bytes) {
        statistics.pool_high_water_bytes = offset;
    }
}

static void
pool_split(pool_block_t *block, size_t capacity)
{
    size_t remainder;
    pool_block_t *next;
    if (block->fields.capacity < capacity) {
        return;
    }
    remainder = block->fields.capacity - capacity;
    if (remainder < sizeof(pool_block_t) + alignment_bytes()) {
        return;
    }
    next = (pool_block_t *)((unsigned char *)(block + 1) + capacity);
    next->fields.capacity = remainder - sizeof(pool_block_t);
    next->fields.requested = 0;
    next->fields.previous = block;
    next->fields.next = block->fields.next;
    next->fields.magic = POOL_MAGIC;
    next->fields.is_free = 1;
    if (next->fields.next != NULL) {
        next->fields.next->fields.previous = next;
    }
    block->fields.next = next;
    block->fields.capacity = capacity;
}

static void
pool_merge_next(pool_block_t *block)
{
    pool_block_t *next = block->fields.next;
    if (next == NULL || !next->fields.is_free) {
        return;
    }
    assert(next->fields.magic == POOL_MAGIC);
    block->fields.capacity += sizeof(pool_block_t) + next->fields.capacity;
    block->fields.next = next->fields.next;
    if (block->fields.next != NULL) {
        block->fields.next->fields.previous = block;
    }
}

static void *
pool_allocate_internal(size_t requested, int count_allocator_request)
{
    size_t capacity = align_up(requested);
    pool_block_t *block;
    if (requested == 0 || capacity == 0) {
        allocation_failure();
    }
    for (block = pool_first; block != NULL; block = block->fields.next) {
        assert(block->fields.magic == POOL_MAGIC);
        if (block->fields.is_free && block->fields.capacity >= capacity) {
            pool_split(block, capacity);
            block->fields.is_free = 0;
            block->fields.requested = requested;
            record_requested_allocation(requested,
                                        sizeof(*block) + block->fields.capacity,
                                        count_allocator_request);
            pool_set_high_water(block);
            return block + 1;
        }
    }
    allocation_failure();
    return NULL;
}

static void *
pool_allocate(size_t requested)
{
    return pool_allocate_internal(requested, 1);
}

static void
pool_release_block(pool_block_t *block, size_t supplied_size, int count_free)
{
    size_t requested;
    size_t physical;
    assert(block->fields.magic == POOL_MAGIC);
    assert(!block->fields.is_free);
    requested = block->fields.requested;
    physical = sizeof(*block) + block->fields.capacity;
    if (requested != supplied_size) {
        statistics.size_mismatches++;
    }
    secure_clear(block + 1, block->fields.capacity);
    if (count_free) {
        record_requested_free(requested, physical);
    } else {
        assert(statistics.current_requested_bytes >= requested);
        assert(statistics.current_physical_bytes >= physical);
        assert(statistics.current_allocations > 0);
        statistics.current_requested_bytes -= requested;
        statistics.current_physical_bytes -= physical;
        statistics.current_allocations--;
    }
    block->fields.requested = 0;
    block->fields.is_free = 1;
    pool_merge_next(block);
    if (block->fields.previous != NULL && block->fields.previous->fields.is_free) {
        block = block->fields.previous;
        pool_merge_next(block);
    }
}

static void
pool_free(void *pointer, size_t supplied_size)
{
    if (pointer == NULL) {
        return;
    }
    pool_release_block((pool_block_t *)pointer - 1, supplied_size, 1);
}

static void *
pool_reallocate(void *pointer, size_t supplied_old_size, size_t new_size)
{
    pool_block_t *block;
    size_t old_size;
    size_t desired;
    size_t combined;
    void *replacement;
    size_t relocation_peak;

    if (pointer == NULL) {
        return pool_allocate(new_size);
    }
    if (new_size == 0) {
        pool_free(pointer, supplied_old_size);
        return NULL;
    }
    block = (pool_block_t *)pointer - 1;
    assert(block->fields.magic == POOL_MAGIC);
    assert(!block->fields.is_free);
    old_size = block->fields.requested;
    if (old_size != supplied_old_size) {
        statistics.size_mismatches++;
    }
    desired = align_up(new_size);
    if (desired == 0) {
        allocation_failure();
    }
    statistics.reallocation_calls++;
    statistics.total_requested_bytes += new_size;
    if (new_size > statistics.largest_request_bytes) {
        statistics.largest_request_bytes = new_size;
    }
    if (new_size <= block->fields.capacity) {
        const size_t old_capacity = block->fields.capacity;
        if (new_size < old_size) {
            secure_clear((unsigned char *)pointer + new_size, old_size - new_size);
        }
        pool_split(block, desired);
        assert(statistics.current_requested_bytes >= old_size);
        statistics.current_requested_bytes -= old_size;
        statistics.current_requested_bytes += new_size;
        statistics.current_physical_bytes -= old_capacity - block->fields.capacity;
        block->fields.requested = new_size;
        return pointer;
    }
    if (block->fields.next != NULL && block->fields.next->fields.is_free) {
        combined = block->fields.capacity + sizeof(pool_block_t) +
                   block->fields.next->fields.capacity;
        if (combined >= desired) {
            const size_t old_capacity = block->fields.capacity;
            pool_merge_next(block);
            pool_split(block, desired);
            assert(statistics.current_requested_bytes >= old_size);
            statistics.current_requested_bytes -= old_size;
            statistics.current_requested_bytes += new_size;
            statistics.current_physical_bytes +=
                block->fields.capacity - old_capacity;
            if (statistics.current_requested_bytes > statistics.peak_requested_bytes) {
                statistics.peak_requested_bytes = statistics.current_requested_bytes;
            }
            if (statistics.current_physical_bytes > statistics.peak_physical_bytes) {
                statistics.peak_physical_bytes = statistics.current_physical_bytes;
            }
            block->fields.requested = new_size;
            pool_set_high_water(block);
            return pointer;
        }
    }
    if (statistics.current_requested_bytes > SIZE_MAX - new_size) {
        allocation_failure();
    }
    relocation_peak = statistics.current_requested_bytes + new_size;
    if (relocation_peak > statistics.conservative_relocation_peak_bytes) {
        statistics.conservative_relocation_peak_bytes = relocation_peak;
    }
    replacement = pool_allocate_internal(new_size, 0);
    statistics.pool_relocation_allocations++;
    memcpy(replacement, pointer, old_size < new_size ? old_size : new_size);
    pool_release_block(block, supplied_old_size, 0);
    return replacement;
}

static int
pool_initialize(size_t pool_bytes)
{
    pool_bytes = align_down(pool_bytes);
    if (pool_bytes <= sizeof(pool_block_t) || pool_bytes > MINI_GMP_POOL_MAX_BYTES) {
        return 0;
    }
    memset(&statistics, 0, sizeof(statistics));
    secure_clear(pool_storage.bytes, pool_bytes);
    statistics.pool_capacity_bytes = pool_bytes;
    statistics.secure_clear_bytes = 0;
    pool_first = (pool_block_t *)pool_storage.bytes;
    pool_first->fields.capacity = pool_bytes - sizeof(pool_block_t);
    pool_first->fields.requested = 0;
    pool_first->fields.previous = NULL;
    pool_first->fields.next = NULL;
    pool_first->fields.magic = POOL_MAGIC;
    pool_first->fields.is_free = 1;
    return 1;
}

int
mini_gmp_allocator_select(mini_gmp_allocator_mode_t mode, size_t pool_bytes)
{
    if (statistics.current_requested_bytes != 0) {
        return 0;
    }
    selected_mode = mode;
    pool_first = NULL;
    oom_target = NULL;
    memset(&statistics, 0, sizeof(statistics));
    switch (mode) {
    case MINI_GMP_ALLOCATOR_NATIVE:
        return 1;
    case MINI_GMP_ALLOCATOR_TRACKING:
        mp_set_memory_functions(tracking_allocate,
                                tracking_reallocate,
                                tracking_free);
        return 1;
    case MINI_GMP_ALLOCATOR_POOL:
        if (!pool_initialize(pool_bytes)) {
            return 0;
        }
        mp_set_memory_functions(pool_allocate, pool_reallocate, pool_free);
        return 1;
    }
    return 0;
}

int
mini_gmp_allocator_reset(void)
{
    size_t pool_bytes = statistics.pool_capacity_bytes;
    if (statistics.current_requested_bytes != 0) {
        return 0;
    }
    if (selected_mode == MINI_GMP_ALLOCATOR_POOL) {
        return pool_initialize(pool_bytes);
    }
    memset(&statistics, 0, sizeof(statistics));
    return 1;
}

void
mini_gmp_allocator_arm_oom(jmp_buf *target)
{
    oom_target = target;
}

void
mini_gmp_allocator_disarm_oom(void)
{
    oom_target = NULL;
}

const mini_gmp_allocator_stats_t *
mini_gmp_allocator_stats(void)
{
    return &statistics;
}

const char *
mini_gmp_allocator_mode_name(mini_gmp_allocator_mode_t mode)
{
    switch (mode) {
    case MINI_GMP_ALLOCATOR_NATIVE:
        return "native";
    case MINI_GMP_ALLOCATOR_TRACKING:
        return "tracking";
    case MINI_GMP_ALLOCATOR_POOL:
        return "pool";
    }
    return "invalid";
}

size_t
mini_gmp_allocator_pool_max_bytes(void)
{
    return MINI_GMP_POOL_MAX_BYTES;
}
