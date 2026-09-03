// SPDX-License-Identifier: Apache-2.0
// No-allocator memory contract for the isolated RP2350 firmware.

#include <mem.h>

#include <stddef.h>

void
sqisign_secure_clear(void *mem, size_t size)
{
    volatile unsigned char *cursor = (volatile unsigned char *)mem;
    while (size-- != 0) {
        *cursor++ = 0;
    }
}

void
sqisign_secure_free(void *mem, size_t size)
{
    if (mem != NULL) {
        sqisign_secure_clear(mem, size);
        // Dynamic ownership is outside this firmware's contract.  An
        // unexpected call is a fail-stop error, never an allocator fallback.
        __builtin_trap();
    }
}
