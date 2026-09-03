// SPDX-License-Identifier: Apache-2.0 and Unknown
//
// D13 low-memory adaptation of the NIST signature KAT driver.  The request
// file and response serialization follow PQCgenKAT_sign.c, while KeyGen, Sign
// and Verify/Open are forced through the caller-owned Level-I workspace API.

#include <api.h>
#include <encoded_sizes.h>
#include <rng.h>
#include <signature_lowmem.h>

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    KAT_MAX_VECTORS = 100,
    KAT_SEED_BYTES = 48,
    KAT_MAX_MESSAGE_BYTES = 3300,
    WORKSPACE_GUARD_BYTES = 64,
    MAX_MARKER_LEN = 50,
    PATH_BUFFER_BYTES = 4096,
};

typedef struct guarded_operation_workspace {
    unsigned char before[WORKSPACE_GUARD_BYTES];
    protocols_operation_workspace_t workspace;
    unsigned char after[WORKSPACE_GUARD_BYTES];
} guarded_operation_workspace_t;

static guarded_operation_workspace_t guarded_workspace;
static unsigned char message[KAT_MAX_MESSAGE_BYTES];
static unsigned char opened_message[KAT_MAX_MESSAGE_BYTES];
static unsigned char signed_message[KAT_MAX_MESSAGE_BYTES + CRYPTO_BYTES];
static unsigned char tampered_signature[CRYPTO_BYTES];
static unsigned char public_key[CRYPTO_PUBLICKEYBYTES];
static unsigned char secret_key[CRYPTO_SECRETKEYBYTES];

_Static_assert(CRYPTO_SECRETKEYBYTES == SECRETKEY_BYTES,
               "NIST and encoded secret-key sizes differ");
_Static_assert(CRYPTO_PUBLICKEYBYTES == PUBLICKEY_BYTES,
               "NIST and encoded public-key sizes differ");
_Static_assert(CRYPTO_BYTES == SIGNATURE_BYTES,
               "NIST and encoded signature sizes differ");
_Static_assert(offsetof(guarded_operation_workspace_t, workspace) ==
                   WORKSPACE_GUARD_BYTES,
               "padding separates the leading operation guard");
_Static_assert(offsetof(guarded_operation_workspace_t, after) ==
                   WORKSPACE_GUARD_BYTES +
                       sizeof(protocols_operation_workspace_t),
               "padding separates the trailing operation guard");
_Static_assert(offsetof(protocols_operation_workspace_t, keygen) == 0 &&
                   offsetof(protocols_operation_workspace_t, sign) == 0 &&
                   offsetof(protocols_operation_workspace_t, verify) == 0,
               "operation workspaces must overlay at offset zero");

static void
usage(const char *program)
{
    fprintf(stderr,
            "Usage: %s --request FILE --response FILE [--vectors 1..%d]\n",
            program,
            KAT_MAX_VECTORS);
}

static int
parse_vector_count(const char *value, int *vectors)
{
    char *end = NULL;
    errno = 0;
    long parsed = strtol(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed < 1 ||
        parsed > KAT_MAX_VECTORS) {
        return 0;
    }
    *vectors = (int)parsed;
    return 1;
}

static int
find_marker(FILE *input, const char *marker)
{
    char line[MAX_MARKER_LEN];
    int length = (int)strlen(marker);
    if (length > MAX_MARKER_LEN - 1) {
        length = MAX_MARKER_LEN - 1;
    }

    for (int i = 0; i < length; i++) {
        int current = fgetc(input);
        if (current == EOF) {
            return 0;
        }
        line[i] = (char)current;
    }
    line[length] = '\0';

    for (;;) {
        if (strncmp(line, marker, (size_t)length) == 0) {
            return 1;
        }
        for (int i = 0; i < length - 1; i++) {
            line[i] = line[i + 1];
        }
        int current = fgetc(input);
        if (current == EOF) {
            return 0;
        }
        line[length - 1] = (char)current;
        line[length] = '\0';
    }
}

static int
hex_digit(int character)
{
    if (character >= '0' && character <= '9') {
        return character - '0';
    }
    if (character >= 'A' && character <= 'F') {
        return character - 'A' + 10;
    }
    if (character >= 'a' && character <= 'f') {
        return character - 'a' + 10;
    }
    return -1;
}

static int
read_hex(FILE *input, unsigned char *output, int length, const char *marker)
{
    if (length <= 0 || !find_marker(input, marker)) {
        return 0;
    }
    for (int i = 0; i < length; i++) {
        int high = hex_digit(fgetc(input));
        int low = hex_digit(fgetc(input));
        if (high < 0 || low < 0) {
            return 0;
        }
        output[i] = (unsigned char)((high << 4) | low);
    }
    int character = fgetc(input);
    if (character == '\r') {
        character = fgetc(input);
    }
    return character == '\n';
}

static int
print_hex(FILE *output,
          const char *prefix,
          const unsigned char *bytes,
          unsigned long long length)
{
    if (fputs(prefix, output) == EOF) {
        return 0;
    }
    for (unsigned long long i = 0; i < length; i++) {
        if (fprintf(output, "%02X", bytes[i]) < 0) {
            return 0;
        }
    }
    if (length == 0 && fputs("00", output) == EOF) {
        return 0;
    }
    return fputc('\n', output) != EOF;
}

static void
workspace_fill(void)
{
    memset(guarded_workspace.before, 0x3c, sizeof(guarded_workspace.before));
    memset(&guarded_workspace.workspace,
           0xa5,
           sizeof(guarded_workspace.workspace));
    memset(guarded_workspace.after, 0xc3, sizeof(guarded_workspace.after));
}

static int
guards_are_intact(void)
{
    unsigned char difference = 0;
    for (size_t i = 0; i < WORKSPACE_GUARD_BYTES; i++) {
        difference |= guarded_workspace.before[i] ^ 0x3c;
        difference |= guarded_workspace.after[i] ^ 0xc3;
    }
    return difference == 0;
}

static int
operation_workspace_is_clear(void)
{
    const unsigned char *bytes =
        (const unsigned char *)&guarded_workspace.workspace;
    unsigned char residual = 0;
    for (size_t i = 0; i < sizeof(guarded_workspace.workspace); i++) {
        residual |= bytes[i];
    }
    return guards_are_intact() && residual == 0;
}

static int
verify_member_is_clear_and_tail_untouched(void)
{
    const unsigned char *bytes =
        (const unsigned char *)&guarded_workspace.workspace;
    unsigned char difference = 0;
    for (size_t i = 0; i < sizeof(guarded_workspace.workspace.verify); i++) {
        difference |= bytes[i];
    }
    for (size_t i = sizeof(guarded_workspace.workspace.verify);
         i < sizeof(guarded_workspace.workspace);
         i++) {
        difference |= bytes[i] ^ 0xa5;
    }
    return guards_are_intact() && difference == 0;
}

static int
kat_failure(FILE *request,
            FILE *response,
            const char *response_path,
            int count,
            const char *reason)
{
    fprintf(stderr, "KAT_FAIL count=%d reason=%s\n", count, reason);
    if (request != NULL) {
        fclose(request);
    }
    if (response != NULL) {
        fclose(response);
    }
    (void)response_path;
    return 1;
}

int
main(int argc, char **argv)
{
    const char *request_path = NULL;
    const char *response_path = NULL;
    int vectors = KAT_MAX_VECTORS;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--request") == 0 && i + 1 < argc) {
            request_path = argv[++i];
        } else if (strcmp(argv[i], "--response") == 0 && i + 1 < argc) {
            response_path = argv[++i];
        } else if (strcmp(argv[i], "--vectors") == 0 && i + 1 < argc) {
            if (!parse_vector_count(argv[++i], &vectors)) {
                usage(argv[0]);
                return 2;
            }
        } else if (strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            usage(argv[0]);
            return 2;
        }
    }
    if (request_path == NULL || response_path == NULL) {
        usage(argv[0]);
        return 2;
    }

    FILE *request = fopen(request_path, "r");
    if (request == NULL) {
        fprintf(stderr, "cannot open request: %s\n", request_path);
        return 1;
    }
    FILE *response = fopen(response_path, "w");
    if (response == NULL) {
        fprintf(stderr, "cannot open response: %s\n", response_path);
        fclose(request);
        return 1;
    }
    if (fprintf(response, "# %s\n\n", CRYPTO_ALGNAME) < 0) {
        return kat_failure(
            request, response, response_path, -1, "response-header-write");
    }

    for (int expected_count = 0; expected_count < vectors;
         expected_count++) {
        int count = -1;
        unsigned char seed[KAT_SEED_BYTES];
        unsigned long long message_length = 0;
        unsigned long long signed_message_length = 0;
        unsigned long long opened_length = 0;

        if (!find_marker(request, "count = ") ||
            fscanf(request, "%d", &count) != 1 || count != expected_count) {
            return kat_failure(request,
                               response,
                               response_path,
                               expected_count,
                               "count-parse-or-order");
        }
        if (!read_hex(request, seed, KAT_SEED_BYTES, "seed = ")) {
            return kat_failure(request,
                               response,
                               response_path,
                               count,
                               "seed-parse");
        }
        if (!find_marker(request, "mlen = ") ||
            fscanf(request, "%llu", &message_length) != 1 ||
            message_length == 0 || message_length > sizeof(message)) {
            return kat_failure(request,
                               response,
                               response_path,
                               count,
                               "message-length-parse");
        }
        if (!read_hex(request,
                      message,
                      (int)message_length,
                      "msg = ")) {
            return kat_failure(request,
                               response,
                               response_path,
                               count,
                               "message-parse");
        }

        randombytes_init(seed, NULL, 256);

        workspace_fill();
        if (sqisign_keypair_with_workspace(
                public_key, secret_key, &guarded_workspace.workspace.keygen) !=
                0 ||
            !operation_workspace_is_clear()) {
            return kat_failure(request,
                               response,
                               response_path,
                               count,
                               "lowmem-keygen-or-workspace");
        }

        workspace_fill();
        if (sqisign_sign_with_workspace(signed_message,
                                        &signed_message_length,
                                        message,
                                        message_length,
                                        secret_key,
                                        &guarded_workspace.workspace.sign) !=
                0 ||
            signed_message_length != message_length + CRYPTO_BYTES ||
            !operation_workspace_is_clear()) {
            return kat_failure(request,
                               response,
                               response_path,
                               count,
                               "lowmem-sign-length-or-workspace");
        }

        memset(opened_message, 0xa5, (size_t)message_length);
        workspace_fill();
        if (sqisign_open_with_workspace(opened_message,
                                        &opened_length,
                                        signed_message,
                                        signed_message_length,
                                        public_key,
                                        &guarded_workspace.workspace.verify) !=
                0 ||
            opened_length != message_length ||
            memcmp(opened_message, message, (size_t)message_length) != 0 ||
            !verify_member_is_clear_and_tail_untouched()) {
            return kat_failure(request,
                               response,
                               response_path,
                               count,
                               "lowmem-open-message-or-workspace");
        }

        memcpy(tampered_signature, signed_message, sizeof(tampered_signature));
        tampered_signature[count % CRYPTO_BYTES] ^=
            (unsigned char)(1u << (count % 8));
        workspace_fill();
        if (sqisign_verify_with_workspace(
                message,
                message_length,
                tampered_signature,
                CRYPTO_BYTES,
                public_key,
                &guarded_workspace.workspace.verify) != 1 ||
            !verify_member_is_clear_and_tail_untouched()) {
            return kat_failure(request,
                               response,
                               response_path,
                               count,
                               "tamper-rejection-or-workspace");
        }

        if (fprintf(response, "count = %d\n", count) < 0 ||
            !print_hex(response, "seed = ", seed, sizeof(seed)) ||
            fprintf(response, "mlen = %llu\n", message_length) < 0 ||
            !print_hex(response, "msg = ", message, message_length) ||
            !print_hex(response,
                       "pk = ",
                       public_key,
                       CRYPTO_PUBLICKEYBYTES) ||
            !print_hex(response,
                       "sk = ",
                       secret_key,
                       CRYPTO_SECRETKEYBYTES) ||
            fprintf(response, "smlen = %llu\n", signed_message_length) < 0 ||
            !print_hex(response,
                       "sm = ",
                       signed_message,
                       signed_message_length) ||
            (count + 1 < vectors && fputc('\n', response) == EOF)) {
            return kat_failure(request,
                               response,
                               response_path,
                               count,
                               "response-write");
        }

        printf("KAT_VECTOR count=%d keygen=PASS sign=PASS open=PASS "
               "tamper=PASS workspace=PASS\n",
               count);
        fflush(stdout);
    }

    if (ferror(request) || ferror(response) || fclose(request) != 0 ||
        fclose(response) != 0) {
        fprintf(stderr, "KAT_FAIL count=%d reason=file-finalization\n", vectors);
        return 1;
    }

    printf("D13_LOWMEM_KAT {\"vectors\":%d,\"keygen\":%d,"
           "\"sign\":%d,\"open\":%d,\"tamper_reject\":%d,"
           "\"workspace_contract\":%d,\"result\":\"PASS\"}\n",
           vectors,
           vectors,
           vectors,
           vectors,
           vectors,
           vectors);
    return 0;
}
