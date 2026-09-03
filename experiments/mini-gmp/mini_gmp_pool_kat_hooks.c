#include "mini_gmp_allocator.h"

#include <inttypes.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef MINI_GMP_KAT_POOL_BYTES
#define MINI_GMP_KAT_POOL_BYTES 400000u
#endif

static void __attribute__((constructor))
install_mini_gmp_pool(void)
{
    size_t pool_bytes = MINI_GMP_KAT_POOL_BYTES;
    const char *configured = getenv("SQISIGN_MINI_GMP_KAT_POOL_BYTES");
    if (configured != NULL && *configured != '\0') {
        char *end = NULL;
        uintmax_t parsed;
        errno = 0;
        parsed = strtoumax(configured, &end, 0);
        if (errno != 0 || end == configured || *end != '\0' || parsed > SIZE_MAX) {
            fputs("invalid SQISIGN_MINI_GMP_KAT_POOL_BYTES\n", stderr);
            abort();
        }
        pool_bytes = (size_t)parsed;
    }
    if (!mini_gmp_allocator_select(MINI_GMP_ALLOCATOR_POOL, pool_bytes)) {
        fputs("failed to install the mini-GMP KAT pool\n", stderr);
        abort();
    }
}

static void __attribute__((destructor))
report_mini_gmp_pool(void)
{
    const mini_gmp_allocator_stats_t *stats = mini_gmp_allocator_stats();
    if (stats->current_requested_bytes != 0 ||
        stats->current_physical_bytes != 0 ||
        stats->current_allocations != 0 ||
        stats->size_mismatches != 0 ||
        stats->failed_calls != 0) {
        fputs("mini-GMP KAT pool did not return to its empty state\n", stderr);
        abort();
    }
    fprintf(stderr,
            "MINI_GMP_POOL_KAT {\"pool_bytes\":%zu,"
            "\"pool_compile_capacity_bytes\":%zu,"
            "\"pool_high_water_bytes\":%zu,"
            "\"peak_requested_bytes\":%zu,"
            "\"peak_physical_bytes\":%zu,"
            "\"peak_allocations\":%zu,"
            "\"allocation_calls\":%" PRIu64 ","
            "\"reallocation_calls\":%" PRIu64 ","
            "\"pool_relocation_allocations\":%" PRIu64 ","
            "\"free_calls\":%" PRIu64 ","
            "\"secure_clear_bytes\":%zu}\n",
            stats->pool_capacity_bytes,
            mini_gmp_allocator_pool_max_bytes(),
            stats->pool_high_water_bytes,
            stats->peak_requested_bytes,
            stats->peak_physical_bytes,
            stats->peak_allocations,
            stats->allocation_calls,
            stats->reallocation_calls,
            stats->pool_relocation_allocations,
            stats->free_calls,
            stats->secure_clear_bytes);
}
