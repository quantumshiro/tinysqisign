/* Same-vector primitive comparison: D13 fixed ibz versus bundled mini-GMP. */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(INTBIG_MICROBENCH_FIXED)
#include <intbig.h>
typedef ibz_t benchmark_integer_t;
#define BENCHMARK_BACKEND "d13-fixed-ibz"

int
randombytes(unsigned char *output, unsigned long long output_bytes)
{
    (void)output;
    (void)output_bytes;
    return -1;
}
#elif defined(INTBIG_MICROBENCH_MINI)
#include <mini-gmp.h>
typedef mpz_t benchmark_integer_t;
#define BENCHMARK_BACKEND "mini-gmp"
#else
#error "select one integer microbenchmark backend"
#endif

enum {
    RESULT_WORDS = 27,
    RESULT_HEX_CHARS = 16 * RESULT_WORDS,
    DIVIDEND_BITS = 1536,
    DIVISOR_BITS = 768,
    MULTIPLICAND_BITS = 768,
    GCD_BITS = 1536,
    INVERSE_MODULUS_BITS = 521
};

static uint64_t
nanoseconds(const struct timespec *start, const struct timespec *end)
{
    return (uint64_t)(end->tv_sec - start->tv_sec) * UINT64_C(1000000000) +
           (uint64_t)(end->tv_nsec - start->tv_nsec);
}

static uint64_t
next_word(uint64_t *state)
{
    uint64_t value = *state;
    value ^= value >> 12;
    value ^= value << 25;
    value ^= value >> 27;
    *state = value;
    return value * UINT64_C(2685821657736338717);
}

static void
make_words(uint64_t output[RESULT_WORDS], unsigned bits, uint64_t seed)
{
    const unsigned words = (bits + 63u) / 64u;
    memset(output, 0, RESULT_WORDS * sizeof(*output));
    for (unsigned i = 0; i < words; i++) {
        output[i] = next_word(&seed);
    }
    if (bits % 64u != 0) {
        output[words - 1u] &= (UINT64_C(1) << (bits % 64u)) - 1u;
    }
    output[words - 1u] |= UINT64_C(1) << ((bits - 1u) % 64u);
    output[0] |= 1u;
}

#if defined(INTBIG_MICROBENCH_FIXED)
static void integer_init(benchmark_integer_t *value) { ibz_init(value); }
static void integer_clear(benchmark_integer_t *value) { ibz_finalize(value); }
static void
integer_import(benchmark_integer_t *value, const uint64_t words[RESULT_WORDS])
{
    ibz_copy_digits(value, words, RESULT_WORDS);
}
static void integer_mul(benchmark_integer_t *r, const benchmark_integer_t *a,
                        const benchmark_integer_t *b) { ibz_mul(r, a, b); }
static void
integer_div(benchmark_integer_t *q, benchmark_integer_t *r,
            const benchmark_integer_t *a, const benchmark_integer_t *b)
{
    ibz_div(q, r, a, b);
}
static void integer_gcd(benchmark_integer_t *r, const benchmark_integer_t *a,
                        const benchmark_integer_t *b) { ibz_gcd(r, a, b); }
static int
integer_invert(benchmark_integer_t *r, const benchmark_integer_t *a,
               const benchmark_integer_t *m) { return ibz_invmod(r, a, m); }
static void
integer_sqrt(benchmark_integer_t *r, const benchmark_integer_t *a)
{
    ibz_sqrt_floor(r, a);
}
static int
integer_export(uint64_t words[RESULT_WORDS], const benchmark_integer_t *value)
{
    return ibz_to_digits_checked(words, RESULT_WORDS, value);
}
#else
static void integer_init(benchmark_integer_t *value) { mpz_init(*value); }
static void integer_clear(benchmark_integer_t *value) { mpz_clear(*value); }
static void
integer_import(benchmark_integer_t *value, const uint64_t words[RESULT_WORDS])
{
    mpz_import(*value, RESULT_WORDS, -1, sizeof(uint64_t), 0, 0, words);
}
static void integer_mul(benchmark_integer_t *r, const benchmark_integer_t *a,
                        const benchmark_integer_t *b) { mpz_mul(*r, *a, *b); }
static void
integer_div(benchmark_integer_t *q, benchmark_integer_t *r,
            const benchmark_integer_t *a, const benchmark_integer_t *b)
{
    mpz_tdiv_qr(*q, *r, *a, *b);
}
static void integer_gcd(benchmark_integer_t *r, const benchmark_integer_t *a,
                        const benchmark_integer_t *b) { mpz_gcd(*r, *a, *b); }
static int
integer_invert(benchmark_integer_t *r, const benchmark_integer_t *a,
               const benchmark_integer_t *m) { return mpz_invert(*r, *a, *m) != 0; }
static void
integer_sqrt(benchmark_integer_t *r, const benchmark_integer_t *a)
{
    mpz_sqrt(*r, *a);
}
static int
integer_export(uint64_t words[RESULT_WORDS], const benchmark_integer_t *value)
{
    size_t written = 0;
    memset(words, 0, RESULT_WORDS * sizeof(*words));
    mpz_export(words, &written, -1, sizeof(uint64_t), 0, 0, *value);
    return written <= RESULT_WORDS;
}
#endif

static void
canonical_integer(char output[RESULT_HEX_CHARS + 1],
                  const benchmark_integer_t *value)
{
    uint64_t words[RESULT_WORDS];
    if (!integer_export(words, value)) {
        abort();
    }
    for (size_t i = 0; i < RESULT_WORDS; i++) {
        if (snprintf(output + 16u * i, 17, "%016" PRIx64,
                     words[RESULT_WORDS - 1u - i]) != 16) {
            abort();
        }
    }
}

static uint64_t
checksum_integer(const benchmark_integer_t *value)
{
    uint64_t words[RESULT_WORDS];
    uint64_t checksum = UINT64_C(1469598103934665603);
    if (!integer_export(words, value)) {
        abort();
    }
    for (size_t i = 0; i < RESULT_WORDS; i++) {
        checksum ^= words[i];
        checksum *= UINT64_C(1099511628211);
    }
    return checksum;
}

static void
set_mersenne_521(benchmark_integer_t *value)
{
    uint64_t words[RESULT_WORDS] = { 0 };
    for (unsigned i = 0; i < 8; i++) words[i] = UINT64_MAX;
    words[8] = UINT64_C(0x1ff);
    integer_import(value, words);
}

static unsigned
parse_iterations(const char *text)
{
    char *end = NULL;
    unsigned long value = strtoul(text, &end, 10);
    if (end == text || *end != '\0' || value == 0 || value > 1000000) {
        return 0;
    }
    return (unsigned)value;
}

int
main(int argc, char **argv)
{
    benchmark_integer_t a, b, q, r;
    uint64_t a_words[RESULT_WORDS];
    uint64_t b_words[RESULT_WORDS];
    struct timespec start, end;
    uint64_t checksum;
    char canonical[2 * RESULT_HEX_CHARS + 2];
    unsigned iterations = 1000;

    if (argc == 2) iterations = parse_iterations(argv[1]);
    if (argc > 2 || iterations == 0) {
        fprintf(stderr, "usage: %s [iterations]\n", argv[0]);
        return 2;
    }
    integer_init(&a);
    integer_init(&b);
    integer_init(&q);
    integer_init(&r);

    make_words(a_words, MULTIPLICAND_BITS, UINT64_C(0x6d756c2d6c656674));
    make_words(b_words, MULTIPLICAND_BITS, UINT64_C(0x6d756c2d72696768));
    integer_import(&a, a_words);
    integer_import(&b, b_words);
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (unsigned i = 0; i < iterations; i++) integer_mul(&r, &a, &b);
    clock_gettime(CLOCK_MONOTONIC, &end);
    checksum = checksum_integer(&r);
    canonical_integer(canonical, &r);
    printf("%s,mul,%u,%" PRIu64 ",%" PRIu64 ",%s\n",
           BENCHMARK_BACKEND, iterations, nanoseconds(&start, &end), checksum,
           canonical);

    make_words(a_words, DIVIDEND_BITS, UINT64_C(0x6469762d6c656674));
    make_words(b_words, DIVISOR_BITS, UINT64_C(0x6469762d72696768));
    integer_import(&a, a_words);
    integer_import(&b, b_words);
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (unsigned i = 0; i < iterations; i++) integer_div(&q, &r, &a, &b);
    clock_gettime(CLOCK_MONOTONIC, &end);
    checksum = checksum_integer(&q) ^ (checksum_integer(&r) << 1);
    canonical_integer(canonical, &q);
    canonical[RESULT_HEX_CHARS] = ':';
    canonical_integer(canonical + RESULT_HEX_CHARS + 1, &r);
    printf("%s,div,%u,%" PRIu64 ",%" PRIu64 ",%s\n",
           BENCHMARK_BACKEND, iterations, nanoseconds(&start, &end), checksum,
           canonical);

    make_words(a_words, GCD_BITS, UINT64_C(0x6763642d6c656674));
    make_words(b_words, GCD_BITS, UINT64_C(0x6763642d72696768));
    integer_import(&a, a_words);
    integer_import(&b, b_words);
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (unsigned i = 0; i < iterations; i++) integer_gcd(&r, &a, &b);
    clock_gettime(CLOCK_MONOTONIC, &end);
    checksum = checksum_integer(&r);
    canonical_integer(canonical, &r);
    printf("%s,gcd,%u,%" PRIu64 ",%" PRIu64 ",%s\n",
           BENCHMARK_BACKEND, iterations, nanoseconds(&start, &end), checksum,
           canonical);

    make_words(a_words, INVERSE_MODULUS_BITS - 1u,
               UINT64_C(0x696e76657273652d));
    integer_import(&a, a_words);
    set_mersenne_521(&b);
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (unsigned i = 0; i < iterations; i++) {
        if (!integer_invert(&r, &a, &b)) abort();
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    checksum = checksum_integer(&r);
    canonical_integer(canonical, &r);
    printf("%s,invmod,%u,%" PRIu64 ",%" PRIu64 ",%s\n",
           BENCHMARK_BACKEND, iterations, nanoseconds(&start, &end), checksum,
           canonical);

    make_words(a_words, DIVIDEND_BITS, UINT64_C(0x737172742d696e70));
    integer_import(&a, a_words);
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (unsigned i = 0; i < iterations; i++) integer_sqrt(&r, &a);
    clock_gettime(CLOCK_MONOTONIC, &end);
    checksum = checksum_integer(&r);
    canonical_integer(canonical, &r);
    printf("%s,sqrt,%u,%" PRIu64 ",%" PRIu64 ",%s\n",
           BENCHMARK_BACKEND, iterations, nanoseconds(&start, &end), checksum,
           canonical);

    integer_clear(&r);
    integer_clear(&q);
    integer_clear(&b);
    integer_clear(&a);
    return 0;
}
