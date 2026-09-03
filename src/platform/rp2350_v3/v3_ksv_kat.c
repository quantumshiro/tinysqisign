// SPDX-License-Identifier: Apache-2.0
// SQIsign v3.0 p324_3 deterministic KAT and stack measurement on RP2350.

#include <api.h>
#include <randombytes.h>

#include "hardware/clocks.h"
#include "hardware/structs/scb.h"
#include "pico/binary_info.h"
#include "pico/stdio.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"
#include "pico/version.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef SQISIGN_V3_BANNER
#define SQISIGN_V3_BANNER "SQISIGN_RP2350_V3_UNSPECIFIED v1"
#endif
#ifndef SQISIGN_V3_ALIVE
#define SQISIGN_V3_ALIVE "SQISIGN_RP2350_V3_UNSPECIFIED alive"
#endif
#ifndef SQISIGN_V3_DESCRIPTION
#define SQISIGN_V3_DESCRIPTION "SQIsign v3 RP2350 deterministic KAT"
#endif
#ifndef SQISIGN_V3_IMAGE_KIND
#define SQISIGN_V3_IMAGE_KIND "unspecified"
#endif
#ifndef SQISIGN_V3_GENERATED_TREE_SHA256
#define SQISIGN_V3_GENERATED_TREE_SHA256 "unknown"
#endif

#if !PICO_RP2350 || !defined(__ARM_ARCH_8M_MAIN__)
#error "This firmware must be built for the RP2350 Arm target"
#endif

enum {
    PROCESS_STACK_BYTES = 128 * 1024,
    PROCESS_STACK_WORDS = PROCESS_STACK_BYTES / sizeof(uint32_t),
    MSP_PATTERN_GUARD_BYTES = 512,
    KAT_SEED_BYTES = 48,
    KAT_MESSAGE_BYTES = 33,
};

#define STACK_PATTERN_BASE UINT32_C(0xd00df00d)
#define STACK_PATTERN_STEP UINT32_C(0x9e3779b9)

typedef int (*psp_thunk_t)(void *);

extern int sqisign_call_on_psp(psp_thunk_t thunk,
                               void *context,
                               void *stack_top,
                               void *stack_limit);
extern uint32_t __StackLimit[];
extern uint32_t __StackTop[];
extern unsigned char __bss_end__[];

static _Alignas(8) uint32_t process_stack[PROCESS_STACK_WORDS];
static unsigned char kat_seed[KAT_SEED_BYTES];
static unsigned char kat_message[KAT_MESSAGE_BYTES];
static unsigned char public_key[CRYPTO_PUBLICKEYBYTES];
static unsigned char secret_key[CRYPTO_SECRETKEYBYTES];
static unsigned char signed_message[CRYPTO_BYTES + KAT_MESSAGE_BYTES];
static unsigned char opened_message[KAT_MESSAGE_BYTES];
static unsigned char expected_public_key[CRYPTO_PUBLICKEYBYTES];
static unsigned char expected_secret_key[CRYPTO_SECRETKEYBYTES];
static unsigned char expected_signed_message[CRYPTO_BYTES + KAT_MESSAGE_BYTES];
static size_t signed_message_length;
static size_t opened_message_length;
static size_t msp_pattern_words;

static const char kat_seed_hex[] =
    "061550234D158C5EC95595FE04EF7A25767F2E24CC2BC479"
    "D09D86DC9ABCFDE7056A8C266F9EF97ED08541DBD2E1FFA1";
static const char kat_message_hex[] =
    "D81C4D8D734FCBFBEADE3D3F8A039FAA2A2C9957E835AD55"
    "B22E75BF57BB556AC8";
static const char kat_public_key_hex[] =
    "AA943594EB7DCB5EEEBE30D1D266941B110C034804A958A1"
    "51F308E8A3DFE05B4FEDF8D12FA25DA215CA2EA5E7E34681"
    "0A30D23E62C01536DAD9901901D1C72A81E912436CB95C0E"
    "D921300A9F1017309B2905";
static const char kat_secret_key_hex[] =
    "AA943594EB7DCB5EEEBE30D1D266941B110C034804A958A1"
    "51F308E8A3DFE05B4FEDF8D12FA25DA215CA2EA5E7E34681"
    "0A30D23E62C01536DAD9901901D1C72A81E912436CB95C0E"
    "D921300A9F1017309B2905BF08A57488B138665D87231B404"
    "828C915021A1E730100000000000000000000000000000000"
    "000000BD90CE3F67AAF0ABA75798ECECF81E1DC4922F052401"
    "0000000000000000000000000000000000000074081F4EA5"
    "8BE4EEFF6BD596BD3B1EE62EBDEEF86000000000000000000"
    "00000000000000000000000F015E87401300B6D263142D7CDE"
    "CEC6573392F039F64146634BE7E465CC9A12635037FC545E2"
    "E8C391027E0C8C9A4CA3B8475DC03B96704745535FE8AE41"
    "BC8F";
static const char kat_signed_message_hex[] =
    "AF9119C1D62766FE4FC73791F2DFE76016E57EED4FC02D9E"
    "1A34244262CFF1A352BFE6D09A98EEDB2E0C04E419B97369"
    "6F5C9F12135565EE410B05CB9B9DB2655218CA918D151DA2"
    "B36D5A14510D9F6CC729D1EBCF6D05702826D5A7013998E3"
    "3A1750117E4D665FFDD21ED048147A2F5F7955D463AF8CB5"
    "3A673F66AE0DD5380F186302369A07D48FFDE881F955A871D"
    "DC372ACD4A6401884530B6835FD5A050EEA188026474533AF"
    "A387C4CC966CEF0D81EC694F3C90E2CEE6C6C0CB0EBF8268"
    "2562D6D7D80D02D81C4D8D734FCBFBEADE3D3F8A039FAA2"
    "A2C9957E835AD55B22E75BF57BB556AC8";

_Static_assert(sizeof(void *) == 4, "RP2350 Arm ABI must use 32-bit pointers");
_Static_assert(sizeof(kat_seed_hex) == 2 * KAT_SEED_BYTES + 1,
               "bad KAT seed extent");
_Static_assert(sizeof(kat_message_hex) == 2 * KAT_MESSAGE_BYTES + 1,
               "bad KAT message extent");
_Static_assert(sizeof(kat_public_key_hex) == 2 * CRYPTO_PUBLICKEYBYTES + 1,
               "bad KAT public-key extent");
_Static_assert(sizeof(kat_secret_key_hex) == 2 * CRYPTO_SECRETKEYBYTES + 1,
               "bad KAT secret-key extent");
_Static_assert(sizeof(kat_signed_message_hex) ==
                   2 * (CRYPTO_BYTES + KAT_MESSAGE_BYTES) + 1,
               "bad KAT signed-message extent");
_Static_assert((PROCESS_STACK_BYTES & 7) == 0,
               "process stack must be 8-byte aligned");

static uint32_t
read_msp(void)
{
    uint32_t value;
    __asm volatile("mrs %0, msp" : "=r"(value));
    return value;
}

static uint32_t
read_msplim(void)
{
    uint32_t value;
    __asm volatile("mrs %0, msplim" : "=r"(value));
    return value;
}

static uint32_t
read_control(void)
{
    uint32_t value;
    __asm volatile("mrs %0, control" : "=r"(value));
    return value;
}

static uint32_t
read_ipsr(void)
{
    uint32_t value;
    __asm volatile("mrs %0, ipsr" : "=r"(value));
    return value;
}

static void
write_msplim(uint32_t value)
{
    __asm volatile("msr msplim, %0" : : "r"(value) : "memory");
    __asm volatile("isb" : : : "memory");
}

static uint32_t
stack_pattern(size_t index)
{
    return STACK_PATTERN_BASE ^ ((uint32_t)index * STACK_PATTERN_STEP);
}

static void
prepare_process_stack(void)
{
    for (size_t i = 0; i < PROCESS_STACK_WORDS; i++) {
        process_stack[i] = stack_pattern(i);
    }
}

static size_t
process_stack_written_extent(void)
{
    size_t untouched = 0;
    while (untouched < PROCESS_STACK_WORDS &&
           process_stack[untouched] == stack_pattern(untouched)) {
        untouched++;
    }
    return sizeof(process_stack) - untouched * sizeof(process_stack[0]);
}

static int
prepare_msp_pattern(void)
{
    const uintptr_t limit = (uintptr_t)__StackLimit;
    const uintptr_t top = (uintptr_t)__StackTop;
    const uintptr_t current = read_msp();
    if (current <= limit + MSP_PATTERN_GUARD_BYTES || current > top) {
        return 0;
    }
    const uintptr_t pattern_end =
        (current - MSP_PATTERN_GUARD_BYTES) & ~(uintptr_t)3u;
    msp_pattern_words = (pattern_end - limit) / sizeof(uint32_t);
    for (size_t i = 0; i < msp_pattern_words; i++) {
        __StackLimit[i] = stack_pattern(i);
    }
    return 1;
}

static size_t
msp_written_extent(void)
{
    size_t untouched = 0;
    while (untouched < msp_pattern_words &&
           __StackLimit[untouched] == stack_pattern(untouched)) {
        untouched++;
    }
    return (uintptr_t)__StackTop -
           ((uintptr_t)__StackLimit + untouched * sizeof(__StackLimit[0]));
}

static int
hex_nibble(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int
decode_hex(unsigned char *out, size_t out_len, const char *hex)
{
    if (strlen(hex) != 2 * out_len) return 0;
    for (size_t i = 0; i < out_len; i++) {
        const int hi = hex_nibble(hex[2 * i]);
        const int lo = hex_nibble(hex[2 * i + 1]);
        if (hi < 0 || lo < 0) return 0;
        out[i] = (unsigned char)((hi << 4) | lo);
    }
    return 1;
}

static int
keygen_thunk(void *opaque)
{
    (void)opaque;
    return crypto_sign_keypair(public_key, secret_key);
}

static int
sign_thunk(void *opaque)
{
    (void)opaque;
    return crypto_sign(signed_message,
                       &signed_message_length,
                       kat_message,
                       sizeof(kat_message),
                       secret_key);
}

static int
verify_thunk(void *opaque)
{
    (void)opaque;
    return crypto_sign_open(opened_message,
                            &opened_message_length,
                            signed_message,
                            signed_message_length,
                            public_key);
}

static int
run_on_process_stack(psp_thunk_t thunk)
{
    return sqisign_call_on_psp(thunk,
                               NULL,
                               process_stack + PROCESS_STACK_WORDS,
                               process_stack);
}

static void
print_banner(void)
{
    printf("%s\r\n", SQISIGN_V3_BANNER);
    printf("scheme=SQIsign-v3.0 variant=p324_3 implementation=m4f image=%s\r\n",
           SQISIGN_V3_IMAGE_KIND);
    printf("board=%s platform=%s sdk=%s firmware=%s firmware_dirty=%u\r\n",
           SQISIGN_PICO_BOARD,
           SQISIGN_PICO_PLATFORM,
           PICO_SDK_VERSION_STRING,
           SQISIGN_FIRMWARE_GIT_COMMIT,
           (unsigned)SQISIGN_FIRMWARE_DIRTY);
    printf("v3_source=%s v3_dirty=%u cpuid=0x%08" PRIx32
           " clock_sys_hz=%" PRIu32 "\r\n",
           SQISIGN_V3_SOURCE_COMMIT,
           (unsigned)SQISIGN_V3_SOURCE_DIRTY,
           scb_hw->cpuid,
           clock_get_hz(clk_sys));
    printf("v3_generated_tree_sha256=%s\r\n",
           SQISIGN_V3_GENERATED_TREE_SHA256);
    printf("bss_end=0x%08" PRIxPTR " stack_limit=0x%08" PRIxPTR
           " stack_top=0x%08" PRIxPTR "\r\n",
           (uintptr_t)__bss_end__,
           (uintptr_t)__StackLimit,
           (uintptr_t)__StackTop);
}

int
main(void)
{
    bi_decl(bi_program_name("SQIsign v3 p324_3 RP2350 K/S/V"));
    bi_decl(bi_program_description(SQISIGN_V3_DESCRIPTION));
    bi_decl(bi_program_version_string("1"));

    write_msplim((uint32_t)(uintptr_t)__StackLimit);
    stdio_init_all();
    const absolute_time_t usb_deadline = make_timeout_time_ms(120000);
    while (!stdio_usb_connected() && !time_reached(usb_deadline)) {
        sleep_ms(10);
    }
    // Leave enough time for the host to reopen CDC after a UF2-triggered USB
    // re-enumeration; operation timing starts only after this delay.
    sleep_ms(15000);
    print_banner();

    const int decoded =
        decode_hex(kat_seed, sizeof(kat_seed), kat_seed_hex) &&
        decode_hex(kat_message, sizeof(kat_message), kat_message_hex) &&
        decode_hex(expected_public_key,
                   sizeof(expected_public_key),
                   kat_public_key_hex) &&
        decode_hex(expected_secret_key,
                   sizeof(expected_secret_key),
                   kat_secret_key_hex) &&
        decode_hex(expected_signed_message,
                   sizeof(expected_signed_message),
                   kat_signed_message_hex);
    const int mode_ok = read_ipsr() == 0 && (read_control() & 3u) == 0;
    const int msp_pattern_ok = prepare_msp_pattern();

    randombytes_init(kat_seed, NULL, 256);
    prepare_process_stack();
    const uint64_t keygen_start = time_us_64();
    const int keygen_result = run_on_process_stack(keygen_thunk);
    const uint64_t keygen_us = time_us_64() - keygen_start;
    const size_t keygen_stack_bytes = process_stack_written_extent();
    const int keygen_match =
        keygen_result == 0 &&
        memcmp(public_key, expected_public_key, sizeof(public_key)) == 0 &&
        memcmp(secret_key, expected_secret_key, sizeof(secret_key)) == 0;

    signed_message_length = 0;
    prepare_process_stack();
    const uint64_t sign_start = time_us_64();
    const int sign_result = run_on_process_stack(sign_thunk);
    const uint64_t sign_us = time_us_64() - sign_start;
    const size_t sign_stack_bytes = process_stack_written_extent();
    const int sign_match =
        sign_result == 0 &&
        signed_message_length == sizeof(signed_message) &&
        memcmp(signed_message,
               expected_signed_message,
               sizeof(signed_message)) == 0;

    opened_message_length = 0;
    prepare_process_stack();
    const uint64_t verify_start = time_us_64();
    const int verify_result = run_on_process_stack(verify_thunk);
    const uint64_t verify_us = time_us_64() - verify_start;
    const size_t verify_stack_bytes = process_stack_written_extent();
    const int verify_match =
        verify_result == 0 &&
        opened_message_length == sizeof(kat_message) &&
        memcmp(opened_message, kat_message, sizeof(kat_message)) == 0;

    const size_t msp_written_bytes = msp_written_extent();
    const int stack_ok =
        keygen_stack_bytes < PROCESS_STACK_BYTES &&
        sign_stack_bytes < PROCESS_STACK_BYTES &&
        verify_stack_bytes < PROCESS_STACK_BYTES;
    const int pass = decoded && mode_ok && msp_pattern_ok && keygen_match &&
                     sign_match && verify_match && stack_ok;

    printf("kat_decoded=%u mode_ok=%u heap_section_bytes=0\r\n",
           (unsigned)decoded,
           (unsigned)mode_ok);
    printf("keygen_result=%d keygen_match=%u keygen_us=%" PRIu64
           " psp_reserved_bytes=%u psp_written_bytes=%u\r\n",
           keygen_result,
           (unsigned)keygen_match,
           keygen_us,
           (unsigned)PROCESS_STACK_BYTES,
           (unsigned)keygen_stack_bytes);
    printf("sign_result=%d sign_match=%u sign_us=%" PRIu64
           " psp_reserved_bytes=%u psp_written_bytes=%u smlen=%u\r\n",
           sign_result,
           (unsigned)sign_match,
           sign_us,
           (unsigned)PROCESS_STACK_BYTES,
           (unsigned)sign_stack_bytes,
           (unsigned)signed_message_length);
    printf("verify_result=%d verify_match=%u verify_us=%" PRIu64
           " psp_reserved_bytes=%u psp_written_bytes=%u mlen=%u\r\n",
           verify_result,
           (unsigned)verify_match,
           verify_us,
           (unsigned)PROCESS_STACK_BYTES,
           (unsigned)verify_stack_bytes,
           (unsigned)opened_message_length);
    printf("msp_reserved_bytes=%u msp_written_upper_bytes=%u"
           " msplim=0x%08" PRIx32 "\r\n",
           (unsigned)((uintptr_t)__StackTop - (uintptr_t)__StackLimit),
           (unsigned)msp_written_bytes,
           read_msplim());
    printf("status=%s\r\n", pass ? "PASS" : "FAIL");
    stdio_flush();

    for (;;) {
        sleep_ms(5000);
        printf("%s status=%s\r\n", SQISIGN_V3_ALIVE,
               pass ? "PASS" : "FAIL");
        stdio_flush();
    }
}
