// SPDX-License-Identifier: Apache-2.0
/* Host-only driver for the optional Cornacchia SCA profile.
 *
 * It keeps the official Level-I KAT secret key/message fixed and changes only
 * the deterministic signing RNG seed.  The instrumented integer module emits
 * one SQISIGN_CORNACCHIA_PROFILE row per modular-square-root attempt.  This
 * driver emits only public seed/status/signature-digest metadata.
 */

#include <encoded_sizes.h>
#include <rng.h>
#include <signature_lowmem.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Compact-v1 Level-I verification fixture retained by the existing transcript
 * harness.  The secret-key encoding below has this public-key prefix. */
static const char KAT_PK_HEX[] =
    "01C70D9910A43882F0FFD5C68305AC3220CE54A8625294DCCBDB4537809D7903"
    "E74DE894B086EFD981D508E4C5ACD9D8B5126F0C43C73B27115A542717CE1E010B";

static const char KAT_SK_HEX[] =
    "01C70D9910A43882F0FFD5C68305AC3220CE54A8625294DCCBDB4537809D7903"
    "E74DE894B086EFD981D508E4C5ACD9D8B5126F0C43C73B27115A542717CE1E010B"
    "C16A609C6E12E6C5099C2086E56EC30A6A020000000000000000000000000000"
    "78691BE93DC95931F2FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
    "4A927F64A0B4EE0CE8FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
    "38F639337B258192F0FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
    "0CE247B5A6BE2125090000000000000000000000000000000000000000000000"
    "1100515A4D1D5C08589B74635B0B6A5952D856940AFF47D37F41A74B5CE52300"
    "59E91ECE9FA0A0AD7C6B1CF3391903DD2ADCDB09EE07BF01A9415BE3A5D5EB00"
    "3135AE21B9050612311ADF69DD388DE056970E8A92178BE63EC0FCEEA3E21D00"
    "B631015DB10FF7C460BF597C01412745B6514684A2DAD54CAA299C6BC7C49100";

static const char KAT_MSG_HEX[] =
    "D81C4D8D734FCBFBEADE3D3F8A039FAA2A2C9957E835AD55B22E75BF57BB556AC8";

static protocols_operation_workspace_t operation_workspace;
static unsigned char pk[PUBLICKEY_BYTES];
static unsigned char sk[SECRETKEY_BYTES];
static unsigned char message[33];
static unsigned char signed_message[SIGNATURE_BYTES + sizeof(message)];

static int
hex_nibble(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static int
decode_hex(unsigned char *output, size_t output_length, const char *hex)
{
    if (strlen(hex) != 2 * output_length)
        return 0;
    for (size_t index = 0; index < output_length; ++index) {
        const int high = hex_nibble(hex[2 * index]);
        const int low = hex_nibble(hex[2 * index + 1]);
        if (high < 0 || low < 0)
            return 0;
        output[index] = (unsigned char)((high << 4) | low);
    }
    return 1;
}

static int
load_kat(void)
{
    return decode_hex(pk, sizeof(pk), KAT_PK_HEX) &&
           decode_hex(sk, sizeof(sk), KAT_SK_HEX) &&
           decode_hex(message, sizeof(message), KAT_MSG_HEX);
}

static void
reset_rng(uint64_t seed)
{
    unsigned char entropy[48];
    for (size_t index = 0; index < sizeof(entropy); ++index) {
        const unsigned shift = (unsigned)(8 * (index % sizeof(seed)));
        entropy[index] =
            (unsigned char)((seed >> shift) ^ (UINT64_C(0x9d) * index));
    }
    randombytes_init(entropy, NULL, 256);
}

static uint64_t
fnv1a64(const unsigned char *input, size_t length)
{
    uint64_t digest = UINT64_C(1469598103934665603);
    for (size_t index = 0; index < length; ++index) {
        digest ^= input[index];
        digest *= UINT64_C(1099511628211);
    }
    return digest;
}

static int
run_seed(const char *seed_text)
{
    char *end = NULL;
    const uint64_t seed = strtoull(seed_text, &end, 0);
    if (end == seed_text || *end != '\0')
        return 2;

    unsigned long long signed_length = sizeof(signed_message);
    reset_rng(seed);
    fprintf(stderr, "SQISIGN_CORNACCHIA_PHASE seed=%" PRIu64 " phase=sign\n", seed);
    if (sqisign_sign_with_workspace(signed_message,
                                    &signed_length,
                                    message,
                                    sizeof(message),
                                    sk,
                                    &operation_workspace.sign) != 0 ||
        signed_length != sizeof(signed_message)) {
        return 1;
    }
    if (memcmp(signed_message + SIGNATURE_BYTES,
               message,
               sizeof(message)) != 0 ||
        sqisign_verify_with_workspace(message,
                                      sizeof(message),
                                      signed_message,
                                      SIGNATURE_BYTES,
                                      pk,
                                      &operation_workspace.verify) != 0) {
        return 1;
    }
    printf("seed=%" PRIu64 " signature_fnv64=%016" PRIx64 " status=PASS\n",
           seed,
           fnv1a64(signed_message, (size_t)signed_length));
    return 0;
}

int
main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s SEED [SEED ...]\n", argv[0]);
        return 2;
    }
    if (!load_kat())
        return 3;
    for (int index = 1; index < argc; ++index) {
        const int status = run_seed(argv[index]);
        if (status != 0)
            return status;
    }
    return 0;
}
