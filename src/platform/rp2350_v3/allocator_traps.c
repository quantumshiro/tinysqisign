// SPDX-License-Identifier: Apache-2.0
// Fail-stop allocator ABI for an image whose heap size is fixed to zero.

#include <stddef.h>
#include <stdint.h>

static volatile uint32_t allocator_violation_count;

__attribute__((noreturn, noinline)) void
sqisign_allocator_violation(void)
{
    ++allocator_violation_count;
    __builtin_trap();
    for (;;) {
    }
}

void *malloc(size_t size)
{
    (void)size;
    sqisign_allocator_violation();
}

void *calloc(size_t count, size_t size)
{
    (void)count;
    (void)size;
    sqisign_allocator_violation();
}

void *realloc(void *pointer, size_t size)
{
    (void)pointer;
    (void)size;
    sqisign_allocator_violation();
}

void free(void *pointer)
{
    (void)pointer;
    sqisign_allocator_violation();
}

void *_malloc_r(void *reent, size_t size)
{
    (void)reent;
    return malloc(size);
}

void *_calloc_r(void *reent, size_t count, size_t size)
{
    (void)reent;
    return calloc(count, size);
}

void *_realloc_r(void *reent, void *pointer, size_t size)
{
    (void)reent;
    return realloc(pointer, size);
}

void _free_r(void *reent, void *pointer)
{
    (void)reent;
    free(pointer);
}

void *_sbrk(ptrdiff_t increment)
{
    (void)increment;
    sqisign_allocator_violation();
}

void *_sbrk_r(void *reent, ptrdiff_t increment)
{
    (void)reent;
    return _sbrk(increment);
}
