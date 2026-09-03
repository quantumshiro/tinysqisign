// SPDX-License-Identifier: Apache-2.0
// Fixed-message, fixed-signing-RNG, multi-key SQIsign v3 timing screen.

#include <api.h>
#include <randombytes.h>

#include "hardware/clocks.h"
#include "hardware/structs/scb.h"
#include "hardware/xip_cache.h"
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
#define SQISIGN_V3_BANNER "SQISIGN_RP2350_V3_SCA_KEY_UNSPECIFIED v1"
#endif
#ifndef SQISIGN_V3_ALIVE
#define SQISIGN_V3_ALIVE "SQISIGN_RP2350_V3_SCA_KEY_UNSPECIFIED alive"
#endif
#ifndef SQISIGN_V3_DESCRIPTION
#define SQISIGN_V3_DESCRIPTION "SQIsign v3 RP2350 fixed-key timing screen"
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
    REPETITIONS_PER_KEY_PER_PASS = 5,
    PASS_COUNT = 2,
    SAMPLES_PER_PASS =
        SQISIGN_V3_KAT_VECTOR_COUNT * REPETITIONS_PER_KEY_PER_PASS,
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
static unsigned char signing_seed[SQISIGN_V3_KAT_SEED_BYTES];
static unsigned char active_public_key[CRYPTO_PUBLICKEYBYTES];
static unsigned char active_secret_key[CRYPTO_SECRETKEYBYTES];
static unsigned char
    signed_message[CRYPTO_BYTES + SQISIGN_V3_KAT_MAX_MESSAGE_BYTES];
static unsigned char opened_message[SQISIGN_V3_KAT_MAX_MESSAGE_BYTES];
static const sqisign_v3_kat_vector_t *current_vector;
static size_t signed_message_length;
static size_t opened_message_length;
static size_t msp_pattern_words;

static const unsigned char fixed_signing_seed[SQISIGN_V3_KAT_SEED_BYTES] = {
    0xa5, 0xa4, 0xa7, 0xa6, 0xa1, 0xa0, 0xa3, 0xa2,
    0xad, 0xac, 0xaf, 0xae, 0xa9, 0xa8, 0xab, 0xaa,
    0xb5, 0xb4, 0xb7, 0xb6, 0xb1, 0xb0, 0xb3, 0xb2,
    0xbd, 0xbc, 0xbf, 0xbe, 0xb9, 0xb8, 0xbb, 0xba,
    0x85, 0x84, 0x87, 0x86, 0x81, 0x80, 0x83, 0x82,
    0x8d, 0x8c, 0x8f, 0x8e, 0x89, 0x88, 0x8b, 0x8a,
};

static const uint32_t schedule_seeds[PASS_COUNT] = {
    UINT32_C(0x13579bdf),
    UINT32_C(0x2468ace1),
};

_Static_assert(sizeof(void *) == 4, "RP2350 Arm ABI must use 32-bit pointers");
_Static_assert((PROCESS_STACK_BYTES & 7) == 0,
               "process stack must be 8-byte aligned");
_Static_assert(SQISIGN_V3_KAT_VECTOR_COUNT == 10,
               "fixed-key screen expects ten KAT keys");
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
sign_thunk(void *opaque)
{
    (void)opaque;
    const sqisign_v3_kat_vector_t *message_vector =
        &sqisign_v3_kat_vectors[0];
    return crypto_sign(signed_message,
                       &signed_message_length,
                       message_vector->message,
                       message_vector->message_length,
                       active_secret_key);
}

static int
verify_thunk(void *opaque)
{
    (void)opaque;
    return crypto_sign_open(opened_message,
                            &opened_message_length,
                            signed_message,
                            signed_message_length,
                            active_public_key);
}

static int
run_on_process_stack(psp_thunk_t thunk)
{
    return sqisign_call_on_psp(thunk,
                               NULL,
                               process_stack + PROCESS_STACK_WORDS,
                               process_stack);
}

static uint32_t
xorshift32(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static void
make_schedule(unsigned *schedule, uint32_t seed)
{
    for (size_t i = 0; i < SAMPLES_PER_PASS; i++) {
        schedule[i] = (unsigned)(i % SQISIGN_V3_KAT_VECTOR_COUNT);
    }
    for (size_t i = SAMPLES_PER_PASS; i > 1; i--) {
        const size_t j = xorshift32(&seed) % i;
        const unsigned temporary = schedule[i - 1];
        schedule[i - 1] = schedule[j];
        schedule[j] = temporary;
    }
}

static uint64_t
fnv1a64(const unsigned char *data, size_t length)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    for (size_t i = 0; i < length; i++) {
        hash ^= data[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static size_t
larger(size_t left, size_t right)
{
    return left > right ? left : right;
}

static void
reset_signing_rng(void)
{
    memcpy(signing_seed, fixed_signing_seed, sizeof(signing_seed));
    randombytes_init(signing_seed, NULL, 256);
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
    printf("kat_rsp_sha256=%s kat_first=0 kat_count=%u fixed_message_vector=0"
           " fixed_message_bytes=%u signing_seed=a5-sequence-v1"
           " fixed_key_buffer_address=1"
           " xip_cache_invalidate_before_timing=1"
           " repetitions=%u passes=%u samples=%u\r\n",
           SQISIGN_V3_KAT_RSP_SHA256,
           (unsigned)SQISIGN_V3_KAT_VECTOR_COUNT,
           (unsigned)sqisign_v3_kat_vectors[0].message_length,
           (unsigned)REPETITIONS_PER_KEY_PER_PASS,
           (unsigned)PASS_COUNT,
           (unsigned)(PASS_COUNT * SAMPLES_PER_PASS));
    printf("bss_end=0x%08" PRIxPTR " stack_limit=0x%08" PRIxPTR
           " stack_top=0x%08" PRIxPTR " heap_section_bytes=0\r\n",
           (uintptr_t)__bss_end__,
           (uintptr_t)__StackLimit,
           (uintptr_t)__StackTop);
}

int
main(void)
{
    bi_decl(bi_program_name("SQIsign v3 p324_3 fixed-key timing screen"));
    bi_decl(bi_program_description(SQISIGN_V3_DESCRIPTION));
    bi_decl(bi_program_version_string("1"));

    write_msplim((uint32_t)(uintptr_t)__StackLimit);
    stdio_init_all();
    const absolute_time_t usb_deadline = make_timeout_time_ms(120000);
    while (!stdio_usb_connected() && !time_reached(usb_deadline)) {
        sleep_ms(10);
    }
    sleep_ms(15000);
    print_banner();

    const int mode_ok = read_ipsr() == 0 && (read_control() & 3u) == 0;
    const int msp_pattern_ok = prepare_msp_pattern();
    int all_pass = mode_ok && msp_pattern_ok;
    unsigned passed_samples = 0;
    size_t max_sign_stack = 0;
    size_t max_verify_stack = 0;
    uint64_t key_digests[SQISIGN_V3_KAT_VECTOR_COUNT] = { 0 };

    current_vector = &sqisign_v3_kat_vectors[0];
    memcpy(active_secret_key,
           current_vector->secret_key,
           sizeof(active_secret_key));
    memcpy(active_public_key,
           current_vector->public_key,
           sizeof(active_public_key));
    reset_signing_rng();
    signed_message_length = 0;
    prepare_process_stack();
    const int warmup_sign = run_on_process_stack(sign_thunk);
    opened_message_length = 0;
    prepare_process_stack();
    const int warmup_verify = run_on_process_stack(verify_thunk);
    const int warmup_ok =
        warmup_sign == 0 && warmup_verify == 0 &&
        opened_message_length == sqisign_v3_kat_vectors[0].message_length &&
        memcmp(opened_message,
               sqisign_v3_kat_vectors[0].message,
               opened_message_length) == 0;
    all_pass &= warmup_ok;
    printf("warmup_sign=%d warmup_verify=%d warmup_status=%s\r\n",
           warmup_sign,
           warmup_verify,
           warmup_ok ? "PASS" : "FAIL");
    stdio_flush();

    for (size_t pass = 0; pass < PASS_COUNT; pass++) {
        unsigned schedule[SAMPLES_PER_PASS];
        unsigned seen[SQISIGN_V3_KAT_VECTOR_COUNT] = { 0 };
        make_schedule(schedule, schedule_seeds[pass]);
        for (size_t sequence = 0; sequence < SAMPLES_PER_PASS; sequence++) {
            const unsigned key = schedule[sequence];
            const unsigned repetition = seen[key]++;
            current_vector = &sqisign_v3_kat_vectors[key];
            memcpy(active_secret_key,
                   current_vector->secret_key,
                   sizeof(active_secret_key));
            memcpy(active_public_key,
                   current_vector->public_key,
                   sizeof(active_public_key));
            reset_signing_rng();

            signed_message_length = 0;
            prepare_process_stack();
            xip_cache_invalidate_all();
            const uint64_t sign_start = time_us_64();
            const int sign_result = run_on_process_stack(sign_thunk);
            const uint64_t sign_us = time_us_64() - sign_start;
            const size_t sign_stack = process_stack_written_extent();
            const uint64_t digest =
                fnv1a64(signed_message, signed_message_length);

            opened_message_length = 0;
            prepare_process_stack();
            xip_cache_invalidate_all();
            const uint64_t verify_start = time_us_64();
            const int verify_result = run_on_process_stack(verify_thunk);
            const uint64_t verify_us = time_us_64() - verify_start;
            const size_t verify_stack = process_stack_written_extent();

            const sqisign_v3_kat_vector_t *message_vector =
                &sqisign_v3_kat_vectors[0];
            const int signature_shape_ok =
                signed_message_length ==
                CRYPTO_BYTES + message_vector->message_length;
            const int message_ok =
                opened_message_length == message_vector->message_length &&
                memcmp(opened_message,
                       message_vector->message,
                       opened_message_length) == 0;
            int digest_stable = 1;
            if (key_digests[key] == 0) {
                key_digests[key] = digest;
            } else {
                digest_stable = key_digests[key] == digest;
            }
            const int stack_ok = sign_stack < PROCESS_STACK_BYTES &&
                                 verify_stack < PROCESS_STACK_BYTES;
            const int sample_pass = sign_result == 0 && verify_result == 0 &&
                                    signature_shape_ok && message_ok &&
                                    digest_stable && stack_ok;
            if (sample_pass) {
                passed_samples++;
            } else {
                all_pass = 0;
            }
            max_sign_stack = larger(max_sign_stack, sign_stack);
            max_verify_stack = larger(max_verify_stack, verify_stack);

            printf("pass=%c sequence=%u key=%u repetition=%u sign_result=%d"
                   " sign_us=%" PRIu64 " sign_psp=%u verify_result=%d"
                   " verify_us=%" PRIu64 " verify_psp=%u digest=%016" PRIx64
                   " digest_stable=%u sample_status=%s\r\n",
                   pass == 0 ? 'A' : 'B',
                   (unsigned)sequence,
                   key,
                   repetition,
                   sign_result,
                   sign_us,
                   (unsigned)sign_stack,
                   verify_result,
                   verify_us,
                   (unsigned)verify_stack,
                   digest,
                   (unsigned)digest_stable,
                   sample_pass ? "PASS" : "FAIL");
            stdio_flush();
        }
    }

    const size_t msp_written_bytes = msp_written_extent();
    printf("summary samples=%u passed=%u mode_ok=%u msp_pattern_ok=%u"
           " sign_psp_max=%u verify_psp_max=%u psp_reserved_bytes=%u\r\n",
           (unsigned)(PASS_COUNT * SAMPLES_PER_PASS),
           passed_samples,
           (unsigned)mode_ok,
           (unsigned)msp_pattern_ok,
           (unsigned)max_sign_stack,
           (unsigned)max_verify_stack,
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
