// SPDX-License-Identifier: Apache-2.0
// Fail-stop diagnostic ABI for unreachable cryptographic error reports.

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static volatile uint32_t diagnostic_violation_count;

__attribute__((noreturn, noinline)) void
sqisign_diagnostic_violation(void)
{
    ++diagnostic_violation_count;
    __builtin_trap();
    for (;;) {
    }
}

int
fprintf(FILE *stream, const char *format, ...)
{
    (void)stream;
    (void)format;
    sqisign_diagnostic_violation();
}

void
abort(void)
{
    sqisign_diagnostic_violation();
}
