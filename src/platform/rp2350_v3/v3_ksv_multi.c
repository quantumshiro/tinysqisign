// SPDX-License-Identifier: Apache-2.0
// Multi-vector SQIsign v3 KAT, negative verification, and stack measurement.

#include <api.h>
#include <randombytes.h>

#include "hardware/clocks.h"
#include "hardware/structs/scb.h"
#include "pico/binary_info.h"
#include "pico/stdio.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"
#include "pico/version.h"
#include "v3_kat_subset.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef SQISIGN_V3_BANNER
#define SQISIGN_V3_BANNER "SQISIGN_RP2350_V3_MULTI_UNSPECIFIED v1"
#endif
#ifndef SQISIGN_V3_ALIVE
#define SQISIGN_V3_ALIVE "SQISIGN_RP2350_V3_MULTI_UNSPECIFIED alive"
#endif
#ifndef SQISIGN_V3_DESCRIPTION
#define SQISIGN_V3_DESCRIPTION "SQIsign v3 RP2350 multi-vector KAT"
#endif
#ifndef SQISIGN_V3_IMAGE_KIND
#define SQISIGN_V3_IMAGE_KIND "unspecified"
#endif
#ifndef SQISIGN_V3_GENERATED_TREE_SHA256
#define SQISIGN_V3_GENERATED_TREE_SHA256 "unknown"
#endif
#ifndef SQISIGN_V3_PLACEMENT_VARIANT
#define SQISIGN_V3_PLACEMENT_VARIANT "unspecified"
#endif

#if !PICO_RP2350 || !defined(__ARM_ARCH_8M_MAIN__)
#error "This firmware must be built for the RP2350 Arm target"
#endif

enum {
    PROCESS_STACK_BYTES = 128 * 1024,
    PROCESS_STACK_WORDS = PROCESS_STACK_BYTES / sizeof(uint32_t),
    MSP_PATTERN_GUARD_BYTES = 512,
};

#define STACK_PATTERN_BASE UINT32_C(0xd00df00d)
#define STACK_PATTERN_STEP UINT32_C(0x9e3779b9)

typedef int (*psp_thunk_t)(void *);

extern int sqisign_call_on_psp(psp_thunk_t thunk,
                               void *context,
                               void *stack_top,
                               void *stack_limit);
extern void sqisign_placement_anchor(void);
extern uint32_t __StackLimit[];
extern uint32_t __StackTop[];
extern unsigned char __bss_end__[];

static _Alignas(8) uint32_t process_stack[PROCESS_STACK_WORDS];
static unsigned char kat_seed[SQISIGN_V3_KAT_SEED_BYTES];
static unsigned char public_key[CRYPTO_PUBLICKEYBYTES];
static unsigned char secret_key[CRYPTO_SECRETKEYBYTES];
static unsigned char
    signed_message[CRYPTO_BYTES + SQISIGN_V3_KAT_MAX_MESSAGE_BYTES];
static unsigned char opened_message[SQISIGN_V3_KAT_MAX_MESSAGE_BYTES];
static const sqisign_v3_kat_vector_t *current_vector;
static size_t signed_message_length;
static size_t opened_message_length;
static size_t msp_pattern_words;

_Static_assert(sizeof(void *) == 4, "RP2350 Arm ABI must use 32-bit pointers");
_Static_assert((PROCESS_STACK_BYTES & 7) == 0,
               "process stack must be 8-byte aligned");
_Static_assert(CRYPTO_PUBLICKEYBYTES == 83, "unexpected public-key size");
_Static_assert(CRYPTO_SECRETKEYBYTES == 270, "unexpected secret-key size");
_Static_assert(CRYPTO_BYTES == 200, "unexpected signature size");

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
                       current_vector->message,
                       current_vector->message_length,
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
    printf("scheme=SQIsign-v3.0 variant=p324_3 implementation=m4f image=%s"
           " placement=%s\r\n",
           SQISIGN_V3_IMAGE_KIND,
           SQISIGN_V3_PLACEMENT_VARIANT);
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
    printf("kat_rsp_sha256=%s kat_first=0 kat_count=%u max_mlen=%u\r\n",
           SQISIGN_V3_KAT_RSP_SHA256,
           (unsigned)SQISIGN_V3_KAT_VECTOR_COUNT,
           (unsigned)SQISIGN_V3_KAT_MAX_MESSAGE_BYTES);
    printf("placement_anchor=0x%08" PRIxPTR
           " crypto_sign_keypair=0x%08" PRIxPTR
           " crypto_sign=0x%08" PRIxPTR
           " crypto_sign_open=0x%08" PRIxPTR "\r\n",
           (uintptr_t)&sqisign_placement_anchor,
           (uintptr_t)&crypto_sign_keypair,
           (uintptr_t)&crypto_sign,
           (uintptr_t)&crypto_sign_open);
    printf("bss_end=0x%08" PRIxPTR " stack_limit=0x%08" PRIxPTR
           " stack_top=0x%08" PRIxPTR " heap_section_bytes=0\r\n",
           (uintptr_t)__bss_end__,
           (uintptr_t)__StackLimit,
           (uintptr_t)__StackTop);
}

static size_t
larger(size_t left, size_t right)
{
    return left > right ? left : right;
}

int
main(void)
{
    bi_decl(bi_program_name("SQIsign v3 p324_3 RP2350 multi-vector K/S/V"));
    bi_decl(bi_program_description(SQISIGN_V3_DESCRIPTION));
    bi_decl(bi_program_version_string("1"));

    write_msplim((uint32_t)(uintptr_t)__StackLimit);
    stdio_init_all();
    const absolute_time_t usb_deadline = make_timeout_time_ms(120000);
    while (!stdio_usb_connected() && !time_reached(usb_deadline)) {
        sleep_ms(10);
    }
    sleep_ms(15000);
    sqisign_placement_anchor();
    print_banner();

    const int mode_ok = read_ipsr() == 0 && (read_control() & 3u) == 0;
    const int msp_pattern_ok = prepare_msp_pattern();
    int all_pass = mode_ok && msp_pattern_ok;
    unsigned passed_vectors = 0;
    size_t max_keygen_stack = 0;
    size_t max_sign_stack = 0;
    size_t max_verify_stack = 0;
    size_t max_negative_stack = 0;

    for (size_t index = 0; index < SQISIGN_V3_KAT_VECTOR_COUNT; index++) {
        current_vector = &sqisign_v3_kat_vectors[index];
        memcpy(kat_seed, current_vector->seed, sizeof(kat_seed));
        randombytes_init(kat_seed, NULL, 256);

        prepare_process_stack();
        const uint64_t keygen_start = time_us_64();
        const int keygen_result = run_on_process_stack(keygen_thunk);
        const uint64_t keygen_us = time_us_64() - keygen_start;
        const size_t keygen_stack = process_stack_written_extent();
        const int keygen_match =
            keygen_result == 0 &&
            memcmp(public_key,
                   current_vector->public_key,
                   sizeof(public_key)) == 0 &&
            memcmp(secret_key,
                   current_vector->secret_key,
                   sizeof(secret_key)) == 0;

        signed_message_length = 0;
        prepare_process_stack();
        const uint64_t sign_start = time_us_64();
        const int sign_result = run_on_process_stack(sign_thunk);
        const uint64_t sign_us = time_us_64() - sign_start;
        const size_t sign_stack = process_stack_written_extent();
        const int sign_match =
            sign_result == 0 &&
            signed_message_length == current_vector->signed_message_length &&
            memcmp(signed_message,
                   current_vector->signed_message,
                   signed_message_length) == 0;

        opened_message_length = 0;
        prepare_process_stack();
        const uint64_t verify_start = time_us_64();
        const int verify_result = run_on_process_stack(verify_thunk);
        const uint64_t verify_us = time_us_64() - verify_start;
        const size_t verify_stack = process_stack_written_extent();
        const int verify_match =
            verify_result == 0 &&
            opened_message_length == current_vector->message_length &&
            memcmp(opened_message,
                   current_vector->message,
                   opened_message_length) == 0;

        signed_message[0] ^= 1u;
        opened_message_length = 0;
        prepare_process_stack();
        const uint64_t negative_start = time_us_64();
        const int negative_result = run_on_process_stack(verify_thunk);
        const uint64_t negative_us = time_us_64() - negative_start;
        const size_t negative_stack = process_stack_written_extent();
        signed_message[0] ^= 1u;
        const int negative_rejected = negative_result != 0;

        const int stack_ok =
            keygen_stack < PROCESS_STACK_BYTES &&
            sign_stack < PROCESS_STACK_BYTES &&
            verify_stack < PROCESS_STACK_BYTES &&
            negative_stack < PROCESS_STACK_BYTES;
        const int vector_pass = keygen_match && sign_match && verify_match &&
                                negative_rejected && stack_ok;
        if (vector_pass) {
            passed_vectors++;
        } else {
            all_pass = 0;
        }
        max_keygen_stack = larger(max_keygen_stack, keygen_stack);
        max_sign_stack = larger(max_sign_stack, sign_stack);
        max_verify_stack = larger(max_verify_stack, verify_stack);
        max_negative_stack = larger(max_negative_stack, negative_stack);

        printf("vector=%u mlen=%u keygen_result=%d keygen_match=%u"
               " keygen_us=%" PRIu64 " keygen_psp=%u\r\n",
               current_vector->count,
               (unsigned)current_vector->message_length,
               keygen_result,
               (unsigned)keygen_match,
               keygen_us,
               (unsigned)keygen_stack);
        printf("vector=%u sign_result=%d sign_match=%u sign_us=%" PRIu64
               " sign_psp=%u smlen=%u\r\n",
               current_vector->count,
               sign_result,
               (unsigned)sign_match,
               sign_us,
               (unsigned)sign_stack,
               (unsigned)signed_message_length);
        printf("vector=%u verify_result=%d verify_match=%u verify_us=%" PRIu64
               " verify_psp=%u negative_result=%d negative_rejected=%u"
               " negative_us=%" PRIu64 " negative_psp=%u\r\n",
               current_vector->count,
               verify_result,
               (unsigned)verify_match,
               verify_us,
               (unsigned)verify_stack,
               negative_result,
               (unsigned)negative_rejected,
               negative_us,
               (unsigned)negative_stack);
        printf("vector=%u vector_status=%s\r\n",
               current_vector->count,
               vector_pass ? "PASS" : "FAIL");
        stdio_flush();
    }

    const size_t msp_written_bytes = msp_written_extent();
    printf("summary vectors=%u passed=%u mode_ok=%u msp_pattern_ok=%u"
           " keygen_psp_max=%u sign_psp_max=%u verify_psp_max=%u"
           " negative_psp_max=%u psp_reserved_bytes=%u\r\n",
           (unsigned)SQISIGN_V3_KAT_VECTOR_COUNT,
           passed_vectors,
           (unsigned)mode_ok,
           (unsigned)msp_pattern_ok,
           (unsigned)max_keygen_stack,
           (unsigned)max_sign_stack,
           (unsigned)max_verify_stack,
           (unsigned)max_negative_stack,
           (unsigned)PROCESS_STACK_BYTES);
    printf("msp_reserved_bytes=%u msp_written_upper_bytes=%u"
           " msplim=0x%08" PRIx32 "\r\n",
           (unsigned)((uintptr_t)__StackTop - (uintptr_t)__StackLimit),
           (unsigned)msp_written_bytes,
           read_msplim());
    printf("status=%s\r\n", all_pass ? "PASS" : "FAIL");
    stdio_flush();

    for (;;) {
        sleep_ms(5000);
        printf("%s status=%s\r\n",
               SQISIGN_V3_ALIVE,
               all_pass ? "PASS" : "FAIL");
        stdio_flush();
    }
}
