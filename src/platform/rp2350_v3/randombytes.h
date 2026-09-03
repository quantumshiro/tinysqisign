// SPDX-License-Identifier: Apache-2.0

#ifndef SQISIGN_RP2350_V3_RANDOMBYTES_H
#define SQISIGN_RP2350_V3_RANDOMBYTES_H

#include <stddef.h>
#include <sqisign_namespace.h>

int randombytes(unsigned char *out, unsigned long long out_len);
void randombytes_init(unsigned char *entropy_input,
                      unsigned char *personalization_string,
                      int security_strength);

#endif
