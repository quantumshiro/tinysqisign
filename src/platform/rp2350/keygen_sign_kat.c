// SPDX-License-Identifier: Apache-2.0
// Deterministic low-memory Level-I KeyGen + Sign[/Verify] validation on RP2350.

#include <encoded_sizes.h>
#include <fips202.h>
#include <mem.h>
#include <rng.h>
#include <signature_lowmem.h>

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

#if !PICO_RP2350 || !defined(__ARM_ARCH_8M_MAIN__)
#error "This firmware must be built for the RP2350 Arm target"
#endif

#ifndef SQISIGN_OPERATION_WORKSPACE_BYTES
#define SQISIGN_OPERATION_WORKSPACE_BYTES 353008
#endif

#if defined(SQISIGN_ENABLE_VERIFY) && defined(SQISIGN_KSV_PROFILE_D13)
#define SQISIGN_KAT_BANNER "SQISIGN_RP2350_KSV_D13 v1"
#define SQISIGN_KAT_ALIVE "SQISIGN_RP2350_KSV_D13 alive"
#define SQISIGN_KAT_COMPACT_COMMIT SQISIGN_COMPACT_D13_COMMIT
#elif defined(SQISIGN_ENABLE_VERIFY)
#define SQISIGN_KAT_BANNER "SQISIGN_RP2350_KSV v1"
#define SQISIGN_KAT_ALIVE "SQISIGN_RP2350_KSV alive"
#define SQISIGN_KAT_COMPACT_COMMIT SQISIGN_COMPACT_D12C_COMMIT
#else
#define SQISIGN_KAT_BANNER "SQISIGN_RP2350_KEYGEN_SIGN v1"
#define SQISIGN_KAT_ALIVE "SQISIGN_RP2350_KEYGEN_SIGN alive"
#define SQISIGN_KAT_COMPACT_COMMIT SQISIGN_COMPACT_D12_COMMIT
#endif

enum {
    OPERATION_STACK_BYTES = 128 * 1024,
    OPERATION_STACK_WORDS = OPERATION_STACK_BYTES / sizeof(uint32_t),
    WORKSPACE_GUARD_BYTES = 64,
    POST_RNG_BYTES = 64,
    TRANSCRIPT_DIGEST_BYTES = 32,
    SIGN_MESSAGE_BYTES = 32,
    MSP_PATTERN_GUARD_BYTES = 512,
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

#if defined(SQISIGN_ENABLE_VERIFY)
typedef protocols_operation_workspace_t operation_workspace_t;
#else
typedef union operation_workspace
{
    protocols_keygen_workspace_t keygen;
    protocols_sign_workspace_t sign;
} operation_workspace_t;
#endif

struct guarded_operation_workspace {
    unsigned char before[WORKSPACE_GUARD_BYTES];
    operation_workspace_t workspace;
    unsigned char after[WORKSPACE_GUARD_BYTES];
};

struct guarded_operation_workspace sqisign_rp2350_keygen_sign_owner;
_Alignas(8) uint32_t
    sqisign_rp2350_keygen_sign_psp[OPERATION_STACK_WORDS];

static unsigned char public_key[PUBLICKEY_BYTES];
static unsigned char secret_key[SECRETKEY_BYTES];
static const unsigned char sign_message[SIGN_MESSAGE_BYTES] = {
    0x44, 0x31, 0x32, 0x62, 0x20, 0x6c, 0x6f, 0x77,
    0x6d, 0x65, 0x6d, 0x20, 0x65, 0x6e, 0x63, 0x6f,
    0x64, 0x65, 0x64, 0x20, 0x53, 0x69, 0x67, 0x6e,
    0x20, 0x63, 0x6c, 0x6f, 0x73, 0x75, 0x72, 0x65,
};
static unsigned char signed_message[SIGNATURE_BYTES + SIGN_MESSAGE_BYTES];
static unsigned long long signed_message_length;
static unsigned char keygen_post_rng[POST_RNG_BYTES];
static unsigned char sign_post_rng[POST_RNG_BYTES];
#if defined(SQISIGN_ENABLE_VERIFY)
static unsigned char verify_expected_rng[POST_RNG_BYTES];
static unsigned char verify_actual_rng[POST_RNG_BYTES];
#endif
static unsigned char keygen_digest[TRANSCRIPT_DIGEST_BYTES];
static unsigned char sign_digest[TRANSCRIPT_DIGEST_BYTES];
static size_t msp_pattern_words;

_Static_assert(sizeof(void *) == 4, "RP2350 Arm ABI must use 32-bit pointers");
_Static_assert(sizeof(protocols_keygen_workspace_t) ==
                   SQISIGN_OPERATION_WORKSPACE_BYTES,
               "KeyGen arena extent changed");
_Static_assert(sizeof(protocols_sign_workspace_t) ==
                   SQISIGN_OPERATION_WORKSPACE_BYTES,
               "Sign arena extent changed");
#if defined(SQISIGN_ENABLE_VERIFY)
_Static_assert(sizeof(protocols_verify_workspace_t) == 15428,
               "Verify scratch extent changed");
_Static_assert(sizeof(protocols_operation_workspace_t) ==
                   SQISIGN_OPERATION_WORKSPACE_BYTES,
               "operation arena extent changed");
#endif
_Static_assert(sizeof(operation_workspace_t) ==
                   SQISIGN_OPERATION_WORKSPACE_BYTES,
               "sequential operation arena must not grow");
_Static_assert(_Alignof(operation_workspace_t) == 8,
               "operation arena alignment changed");
_Static_assert(offsetof(struct guarded_operation_workspace, workspace) ==
                   WORKSPACE_GUARD_BYTES,
               "padding separates the leading workspace guard");
_Static_assert(offsetof(struct guarded_operation_workspace, after) ==
                   WORKSPACE_GUARD_BYTES + sizeof(operation_workspace_t),
               "padding separates the trailing workspace guard");
_Static_assert((OPERATION_STACK_BYTES & 7) == 0,
               "process stack must be 8-byte aligned");
_Static_assert(sizeof(signed_message) == SIGNATURE_BYTES + SIGN_MESSAGE_BYTES,
               "signed-message extent changed");

static const unsigned char expected_keygen_digest[TRANSCRIPT_DIGEST_BYTES] = {
    0x1d, 0xe4, 0xb1, 0x75, 0xa5, 0xc3, 0xc3, 0x76,
    0xec, 0x5f, 0x86, 0x59, 0x37, 0x45, 0xfc, 0xdb,
    0x8f, 0x69, 0x7d, 0xbd, 0x40, 0xa7, 0x8c, 0x63,
    0xe7, 0x0d, 0x34, 0x12, 0xdd, 0x17, 0x7f, 0x61,
};

static const unsigned char expected_sign_digest[TRANSCRIPT_DIGEST_BYTES] = {
    0x67, 0x17, 0x80, 0x8a, 0xcd, 0x4a, 0x27, 0xc1,
    0x60, 0x80, 0x1d, 0x83, 0xd6, 0xbc, 0x66, 0xcc,
    0xfa, 0xec, 0x63, 0x5d, 0x04, 0xd7, 0x99, 0x6a,
    0x89, 0x91, 0x01, 0x82, 0x65, 0x7f, 0x25, 0x37,
};

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
prepare_operation_stack(void)
{
    for (size_t i = 0; i < OPERATION_STACK_WORDS; i++) {
        sqisign_rp2350_keygen_sign_psp[i] = stack_pattern(i);
    }
}

static size_t
operation_psp_written_extent(void)
{
    size_t untouched = 0;
    while (untouched < OPERATION_STACK_WORDS &&
           sqisign_rp2350_keygen_sign_psp[untouched] ==
               stack_pattern(untouched)) {
        untouched++;
    }
    return sizeof(sqisign_rp2350_keygen_sign_psp) -
           untouched * sizeof(sqisign_rp2350_keygen_sign_psp[0]);
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
           ((uintptr_t)__StackLimit +
            untouched * sizeof(__StackLimit[0]));
}

static void
reset_rng(void)
{
    unsigned char entropy[48] = {
        0x8d, 0x56, 0x1a, 0x43, 0xf2, 0x90, 0x7b, 0xcd,
        0x35, 0xe1, 0x0c, 0x9a, 0x64, 0xb7, 0x28, 0x5f,
        0x11, 0xa8, 0x72, 0xde, 0x49, 0x03, 0xbc, 0x67,
        0x95, 0x2d, 0xe6, 0x38, 0x70, 0xca, 0x14, 0x8b,
        0x22, 0xfd, 0x59, 0xa1, 0x46, 0x7e, 0xd3, 0x0b,
        0x6c, 0x31, 0x87, 0xea, 0x5d, 0x04, 0xb9, 0x73,
    };
    randombytes_init(entropy, NULL, 256);
}

static void
workspace_fill(void)
{
    memset(sqisign_rp2350_keygen_sign_owner.before,
           0x3c,
           sizeof(sqisign_rp2350_keygen_sign_owner.before));
    memset(&sqisign_rp2350_keygen_sign_owner.workspace,
           0xa5,
           sizeof(sqisign_rp2350_keygen_sign_owner.workspace));
    memset(sqisign_rp2350_keygen_sign_owner.after,
           0xc3,
           sizeof(sqisign_rp2350_keygen_sign_owner.after));
}

static int
workspace_is_guarded_and_clear(void)
{
    const unsigned char *const bytes =
        (const unsigned char *)&sqisign_rp2350_keygen_sign_owner.workspace;
    unsigned char residual = 0;
    for (size_t i = 0; i < WORKSPACE_GUARD_BYTES; i++) {
        residual |= sqisign_rp2350_keygen_sign_owner.before[i] ^ 0x3c;
        residual |= sqisign_rp2350_keygen_sign_owner.after[i] ^ 0xc3;
    }
    for (size_t i = 0;
         i < sizeof(sqisign_rp2350_keygen_sign_owner.workspace);
         i++) {
        residual |= bytes[i];
    }
    return residual == 0;
}

#if defined(SQISIGN_ENABLE_VERIFY)
static int
verify_member_is_clear_and_tail_untouched(void)
{
    const unsigned char *const bytes =
        (const unsigned char *)&sqisign_rp2350_keygen_sign_owner.workspace;
    unsigned char residual = 0;
    for (size_t i = 0; i < WORKSPACE_GUARD_BYTES; i++) {
        residual |= sqisign_rp2350_keygen_sign_owner.before[i] ^ 0x3c;
        residual |= sqisign_rp2350_keygen_sign_owner.after[i] ^ 0xc3;
    }
    for (size_t i = 0; i < sizeof(protocols_verify_workspace_t); i++)
        residual |= bytes[i];
    for (size_t i = sizeof(protocols_verify_workspace_t);
         i < sizeof(sqisign_rp2350_keygen_sign_owner.workspace);
         i++) {
        residual |= bytes[i] ^ 0xa5;
    }
    return residual == 0;
}
#endif

static int
keygen_thunk(void *opaque)
{
    (void)opaque;
    return sqisign_keypair_with_workspace(
        public_key,
        secret_key,
        &sqisign_rp2350_keygen_sign_owner.workspace.keygen);
}

static int
sign_thunk(void *opaque)
{
    (void)opaque;
    return sqisign_sign_with_workspace(
        signed_message,
        &signed_message_length,
        sign_message,
        sizeof(sign_message),
        secret_key,
        &sqisign_rp2350_keygen_sign_owner.workspace.sign);
}

#if defined(SQISIGN_ENABLE_VERIFY)
static int
verify_thunk(void *opaque)
{
    (void)opaque;
    return sqisign_verify_with_workspace(
        sign_message,
        sizeof(sign_message),
        signed_message,
        SIGNATURE_BYTES,
        public_key,
        &sqisign_rp2350_keygen_sign_owner.workspace.verify);
}
#endif

static int
run_on_operation_stack(psp_thunk_t thunk)
{
    return sqisign_call_on_psp(
        thunk,
        NULL,
        sqisign_rp2350_keygen_sign_psp + OPERATION_STACK_WORDS,
        sqisign_rp2350_keygen_sign_psp);
}

static void
compute_keygen_digest(void)
{
    shake256incctx context;
    shake256_inc_init(&context);
    shake256_inc_absorb(&context, public_key, sizeof(public_key));
    shake256_inc_absorb(&context, secret_key, sizeof(secret_key));
    shake256_inc_absorb(&context, keygen_post_rng, sizeof(keygen_post_rng));
    shake256_inc_finalize(&context);
    shake256_inc_squeeze(keygen_digest, sizeof(keygen_digest), &context);
    shake256_inc_ctx_release(&context);
}

static void
compute_sign_digest(void)
{
    shake256incctx context;
    shake256_inc_init(&context);
    shake256_inc_absorb(&context, signed_message, sizeof(signed_message));
    shake256_inc_absorb(&context, sign_post_rng, sizeof(sign_post_rng));
    shake256_inc_finalize(&context);
    shake256_inc_squeeze(sign_digest, sizeof(sign_digest), &context);
    shake256_inc_ctx_release(&context);
}

static void
print_digest(const char *label, const unsigned char *digest)
{
    printf("%s=", label);
    for (size_t i = 0; i < TRANSCRIPT_DIGEST_BYTES; i++) {
        printf("%02x", digest[i]);
    }
    printf("\r\n");
}

static void
print_banner(void)
{
    printf("%s\r\n", SQISIGN_KAT_BANNER);
    printf("board=%s platform=%s sdk=%s source=%s compact=%s dirty=%u\r\n",
           SQISIGN_PICO_BOARD,
           SQISIGN_PICO_PLATFORM,
           PICO_SDK_VERSION_STRING,
           SQISIGN_FIRMWARE_GIT_COMMIT,
           SQISIGN_KAT_COMPACT_COMMIT,
           (unsigned)SQISIGN_FIRMWARE_DIRTY);
    printf("cpuid=0x%08" PRIx32 " clock_sys_hz=%" PRIu32 "\r\n",
           scb_hw->cpuid,
           clock_get_hz(clk_sys));
    printf("bss_end=0x%08" PRIxPTR " stack_limit=0x%08" PRIxPTR
           " stack_top=0x%08" PRIxPTR "\r\n",
           (uintptr_t)__bss_end__,
           (uintptr_t)__StackLimit,
           (uintptr_t)__StackTop);
}

int
main(void)
{
#if defined(SQISIGN_ENABLE_VERIFY) && defined(SQISIGN_KSV_PROFILE_D13)
    bi_decl(bi_program_name("SQIsign D13 Level-I RP2350 K/S/V"));
    bi_decl(bi_program_description(
        "Deterministic certified-norm-sketch K/S/V, shared arena/PSP"));
#elif defined(SQISIGN_ENABLE_VERIFY)
    bi_decl(bi_program_name("SQIsign D12c Level-I RP2350 K/S/V"));
    bi_decl(bi_program_description(
        "Deterministic low-memory KeyGen, Sign and Verify, shared arena/PSP"));
#else
    bi_decl(bi_program_name("SQIsign D12b Level-I RP2350 KeyGen + Sign"));
    bi_decl(bi_program_description(
        "Deterministic low-memory KeyGen and Sign, shared arena and PSP"));
#endif
    bi_decl(bi_program_version_string("1"));

    write_msplim((uint32_t)(uintptr_t)__StackLimit);
    stdio_init_all();
    const absolute_time_t usb_deadline = make_timeout_time_ms(120000);
    while (!stdio_usb_connected() && !time_reached(usb_deadline)) {
        sleep_ms(10);
    }
    /* A reset-to-CDC transition can report DTR before the host-side capture
     * process has reopened the newly enumerated device.  This delay is
     * outside both timed cryptographic intervals and makes the initial
     * provenance banner reliably capturable after flashing. */
    sleep_ms(5000);
    print_banner();

    const int mode_ok = read_ipsr() == 0 && (read_control() & 3u) == 0;
    const int msp_pattern_ok = prepare_msp_pattern();

    workspace_fill();
    prepare_operation_stack();
    reset_rng();
    const uint64_t keygen_start = time_us_64();
    const int keygen_result = run_on_operation_stack(keygen_thunk);
    const uint64_t keygen_us = time_us_64() - keygen_start;
    const int keygen_rng_result =
        randombytes(keygen_post_rng, sizeof(keygen_post_rng));
    compute_keygen_digest();
    const int keygen_workspace_ok = workspace_is_guarded_and_clear();
    const size_t keygen_psp_written = operation_psp_written_extent();
    const int keygen_digest_ok =
        memcmp(keygen_digest,
               expected_keygen_digest,
               sizeof(expected_keygen_digest)) == 0;

    workspace_fill();
    prepare_operation_stack();
    signed_message_length = 0;
    reset_rng();
    const uint64_t sign_start = time_us_64();
    const int sign_result = run_on_operation_stack(sign_thunk);
    const uint64_t sign_us = time_us_64() - sign_start;
    const int sign_rng_result = randombytes(sign_post_rng, sizeof(sign_post_rng));
    compute_sign_digest();
    const int sign_workspace_ok = workspace_is_guarded_and_clear();
    const size_t sign_psp_written = operation_psp_written_extent();
    const int sign_digest_ok =
        memcmp(sign_digest,
               expected_sign_digest,
               sizeof(expected_sign_digest)) == 0;

#if defined(SQISIGN_ENABLE_VERIFY)
    reset_rng();
    const int verify_expected_rng_result =
        randombytes(verify_expected_rng, sizeof(verify_expected_rng));
    workspace_fill();
    prepare_operation_stack();
    reset_rng();
    const uint64_t verify_start = time_us_64();
    const int verify_result = run_on_operation_stack(verify_thunk);
    const uint64_t verify_us = time_us_64() - verify_start;
    const int verify_actual_rng_result =
        randombytes(verify_actual_rng, sizeof(verify_actual_rng));
    const int verify_member_clear =
        verify_member_is_clear_and_tail_untouched();
    sqisign_secure_clear(&sqisign_rp2350_keygen_sign_owner.workspace,
                         sizeof(sqisign_rp2350_keygen_sign_owner.workspace));
    const int verify_arena_clear = workspace_is_guarded_and_clear();
    const int verify_workspace_ok =
        verify_member_clear && verify_arena_clear;
    const size_t verify_psp_written = operation_psp_written_extent();
    const int verify_rng_ok =
        memcmp(verify_expected_rng,
               verify_actual_rng,
               sizeof(verify_expected_rng)) == 0;
#endif

    const size_t msp_written = msp_written_extent();
    int pass =
        mode_ok && msp_pattern_ok && keygen_result == 0 && sign_result == 0 &&
        keygen_rng_result == 0 && sign_rng_result == 0 &&
        keygen_workspace_ok && sign_workspace_ok && keygen_digest_ok &&
        sign_digest_ok && signed_message_length == sizeof(signed_message) &&
        keygen_psp_written < OPERATION_STACK_BYTES &&
        sign_psp_written < OPERATION_STACK_BYTES;
#if defined(SQISIGN_ENABLE_VERIFY)
    pass = pass && verify_result == 0 && verify_expected_rng_result == 0 &&
           verify_actual_rng_result == 0 && verify_workspace_ok &&
           verify_rng_ok && verify_psp_written < OPERATION_STACK_BYTES;
#endif

    printf("mode_ok=%u arena_bytes=%u keygen_clear=%u sign_clear=%u\r\n",
           (unsigned)mode_ok,
           (unsigned)sizeof(operation_workspace_t),
           (unsigned)keygen_workspace_ok,
           (unsigned)sign_workspace_ok);
    printf("keygen_result=%d keygen_us=%" PRIu64
           " psp_reserved_bytes=%u psp_written_bytes=%u\r\n",
           keygen_result,
           keygen_us,
           (unsigned)OPERATION_STACK_BYTES,
           (unsigned)keygen_psp_written);
    printf("sign_result=%d sign_us=%" PRIu64
           " psp_reserved_bytes=%u psp_written_bytes=%u smlen=%u\r\n",
           sign_result,
           sign_us,
           (unsigned)OPERATION_STACK_BYTES,
           (unsigned)sign_psp_written,
           (unsigned)signed_message_length);
#if defined(SQISIGN_ENABLE_VERIFY)
    printf("verify_result=%d verify_us=%" PRIu64
           " psp_reserved_bytes=%u psp_written_bytes=%u"
           " verify_member_clear=%u arena_clear=%u rng_unchanged=%u\r\n",
           verify_result,
           verify_us,
           (unsigned)OPERATION_STACK_BYTES,
           (unsigned)verify_psp_written,
           (unsigned)verify_member_clear,
           (unsigned)verify_arena_clear,
           (unsigned)verify_rng_ok);
#endif
    printf("msp_reserved_bytes=%u msp_written_upper_bytes=%u"
           " msplim=0x%08" PRIx32 "\r\n",
           (unsigned)((uintptr_t)__StackTop - (uintptr_t)__StackLimit),
           (unsigned)msp_written,
           read_msplim());
    print_digest("keygen_transcript_shake256_256", keygen_digest);
    print_digest("sign_transcript_shake256_256", sign_digest);
    printf("keygen_match=%u sign_match=%u rng_results=%d,%d"
           " heap_section_bytes=0\r\n",
           (unsigned)keygen_digest_ok,
           (unsigned)sign_digest_ok,
           keygen_rng_result,
           sign_rng_result);
    printf("status=%s\r\n", pass ? "PASS" : "FAIL");
    stdio_flush();

    for (;;) {
        sleep_ms(5000);
        printf("%s status=%s\r\n",
               SQISIGN_KAT_ALIVE,
               pass ? "PASS" : "FAIL");
        stdio_flush();
    }
}
