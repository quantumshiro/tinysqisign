#ifndef SQISIGN_EXPERIMENT_CTINT1728_32_H
#define SQISIGN_EXPERIMENT_CTINT1728_32_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Experimental Level-I fixed-width integer substrate for Cortex-M33.
 *
 * Values are little-endian 1728-bit two's-complement bit strings represented
 * as 54 32-bit words.  This is isomorphic to the current Level-I ibz_t
 * storage, which uses 27 64-bit limbs, but it does not yet replace ibz_t.
 *
 * The routines below have fixed value-independent limb schedules.  Shift
 * counts may be secret: the implementation uses an 11-stage barrel shift and
 * never indexes memory with a shift-derived value.  A caller must still prove
 * that exposing the returned validity bit is acceptable at its API boundary.
 *
 * This contract is about control flow and memory addresses.  It does not
 * provide masking, constant power, physical side-channel resistance, or a
 * complete constant-time SQIsign signer.
 */

#define CTINT1728_WORD_BITS 32u
#define CTINT1728_WORDS 54u
#define CTINT1728_BITS (CTINT1728_WORDS * CTINT1728_WORD_BITS)
#define CTINT1728_POW_BITS 521u
#define CTINT1728_POW_MAX_MODULUS_BITS 863u
#define CTINT1728_SQRT_PAIRS (CTINT1728_BITS / 2u)
#define CTINT1728_CORNACCHIA_MAX_BITS 492u
#define CTINT1728_CORNACCHIA_HALFGCD_ROUNDS \
    ((49u * CTINT1728_CORNACCHIA_MAX_BITS + 57u) / 17u)

typedef struct {
    uint32_t words[CTINT1728_WORDS];
} ctint1728_32_t;

typedef struct {
    ctint1728_32_t current;
    ctint1728_32_t next;
    ctint1728_32_t saved_output;
} ctint1728_32_shift_workspace_t;

typedef struct {
    ctint1728_32_t result;
    ctint1728_32_t saved_output;
} ctint1728_32_arith_workspace_t;

typedef struct {
    ctint1728_32_t a_magnitude;
    ctint1728_32_t b_magnitude;
    ctint1728_32_t saved_output;
    uint32_t wide_product[2u * CTINT1728_WORDS];
} ctint1728_32_mul_workspace_t;

typedef struct {
    ctint1728_32_t dividend_magnitude;
    ctint1728_32_t divisor_magnitude;
    ctint1728_32_t quotient;
    ctint1728_32_t remainder;
    ctint1728_32_t subtract_candidate;
    ctint1728_32_t saved_quotient;
    ctint1728_32_t saved_remainder;
} ctint1728_32_div_workspace_t;

typedef union {
    ctint1728_32_div_workspace_t division;
    ctint1728_32_arith_workspace_t arithmetic;
} ctint1728_32_mod_phase_workspace_t;

typedef struct {
    ctint1728_32_mod_phase_workspace_t phase;
    ctint1728_32_t quotient_or_adjusted;
    ctint1728_32_t remainder;
    ctint1728_32_t saved_output;
} ctint1728_32_mod_workspace_t;

typedef union {
    ctint1728_32_mod_workspace_t reduction;
    ctint1728_32_mul_workspace_t multiplication;
} ctint1728_32_pow_phase_workspace_t;

typedef struct {
    ctint1728_32_pow_phase_workspace_t phase;
    ctint1728_32_t base_residue;
    ctint1728_32_t accumulator;
    ctint1728_32_t square;
    ctint1728_32_t multiplied;
    ctint1728_32_t saved_output;
} ctint1728_32_pow_workspace_t;

typedef struct {
    ctint1728_32_t remainder;
    ctint1728_32_t root;
    ctint1728_32_t subtract_candidate;
    ctint1728_32_t saved_output;
} ctint1728_32_sqrt_workspace_t;

typedef union {
    ctint1728_32_sqrt_workspace_t square_root;
    ctint1728_32_mul_workspace_t multiplication;
    ctint1728_32_shift_workspace_t shift;
    ctint1728_32_arith_workspace_t arithmetic;
} ctint1728_32_halfgcd_phase_workspace_t;

typedef struct {
    ctint1728_32_halfgcd_phase_workspace_t phase;
    ctint1728_32_t a;
    ctint1728_32_t b;
    ctint1728_32_t maximum;
    ctint1728_32_t minimum;
    ctint1728_32_t shifted;
    ctint1728_32_t next;
    ctint1728_32_t limit;
    ctint1728_32_t limit_squared;
    ctint1728_32_t threshold;
    ctint1728_32_t one;
    ctint1728_32_t saved_output;
} ctint1728_32_halfgcd_workspace_t;

_Static_assert(sizeof(ctint1728_32_t) == 216,
               "ctint1728_32_t ABI changed");
_Static_assert(sizeof(ctint1728_32_shift_workspace_t) == 648,
               "ctint1728 shift workspace ABI changed");
_Static_assert(sizeof(ctint1728_32_arith_workspace_t) == 432,
               "ctint1728 arithmetic workspace ABI changed");
_Static_assert(sizeof(ctint1728_32_mul_workspace_t) == 1080,
               "ctint1728 multiplication workspace ABI changed");
_Static_assert(sizeof(ctint1728_32_div_workspace_t) == 1512,
               "ctint1728 division workspace ABI changed");
_Static_assert(sizeof(ctint1728_32_mod_phase_workspace_t) == 1512,
               "ctint1728 modular-reduction phase ABI changed");
_Static_assert(sizeof(ctint1728_32_mod_workspace_t) == 2160,
               "ctint1728 modular-reduction workspace ABI changed");
_Static_assert(sizeof(ctint1728_32_pow_phase_workspace_t) == 2160,
               "ctint1728 modular-exponentiation phase ABI changed");
_Static_assert(sizeof(ctint1728_32_pow_workspace_t) == 3240,
               "ctint1728 modular-exponentiation workspace ABI changed");
_Static_assert(sizeof(ctint1728_32_sqrt_workspace_t) == 864,
               "ctint1728 square-root workspace ABI changed");
_Static_assert(CTINT1728_CORNACCHIA_HALFGCD_ROUNDS == 1421,
               "ctint1728 Cornacchia half-GCD round count changed");
_Static_assert(sizeof(ctint1728_32_halfgcd_phase_workspace_t) == 1080,
               "ctint1728 half-GCD phase workspace ABI changed");
_Static_assert(sizeof(ctint1728_32_halfgcd_workspace_t) == 3456,
               "ctint1728 half-GCD workspace ABI changed");

void ctint1728_32_copy(ctint1728_32_t *out, const ctint1728_32_t *in);
void ctint1728_32_cmov(ctint1728_32_t *out,
                       const ctint1728_32_t *in,
                       uint32_t selector);
uint32_t ctint1728_32_is_zero(const ctint1728_32_t *value);
int32_t ctint1728_32_sign(const ctint1728_32_t *value);
int32_t ctint1728_32_cmp_unsigned(const ctint1728_32_t *a,
                                  const ctint1728_32_t *b);
int32_t ctint1728_32_cmp_signed(const ctint1728_32_t *a,
                                const ctint1728_32_t *b);
void ctint1728_32_abs(ctint1728_32_t *out,
                      const ctint1728_32_t *in);
uint32_t ctint1728_32_bit_length_unsigned(const ctint1728_32_t *value);
uint32_t ctint1728_32_abs_bit_length(const ctint1728_32_t *value);

/* Signed two's-complement arithmetic.  Return one and publish the exact
 * result when it is representable in [-2^1727, 2^1727-1].  On overflow,
 * return zero and leave an initialized out object byte-identical.  Input and
 * output aliasing is supported.  Workspace must not overlap any live input or
 * output object and is cleared on every return.  A caller must not expose the
 * returned validity bit where the operand bound is secret. */
int ctint1728_32_add(ctint1728_32_t *out,
                     const ctint1728_32_t *a,
                     const ctint1728_32_t *b,
                     ctint1728_32_arith_workspace_t *workspace);
int ctint1728_32_sub(ctint1728_32_t *out,
                     const ctint1728_32_t *a,
                     const ctint1728_32_t *b,
                     ctint1728_32_arith_workspace_t *workspace);
int ctint1728_32_mul(ctint1728_32_t *out,
                     const ctint1728_32_t *a,
                     const ctint1728_32_t *b,
                     ctint1728_32_mul_workspace_t *workspace);
int ctint1728_32_square(ctint1728_32_t *out,
                        const ctint1728_32_t *a,
                        ctint1728_32_mul_workspace_t *workspace);

/* Truncating signed division: a = quotient*b + remainder, with remainder
 * having the dividend's sign and |remainder| < |b|.  The implementation runs
 * all 1728 restoring-division rounds.  Return zero for b==0 or the sole
 * unrepresentable quotient INT_MIN/-1, leaving both initialized outputs
 * byte-identical.  Quotient and remainder must be distinct; either may alias
 * an input.  Workspace must be disjoint from all live objects, is cleared on
 * every return, and its validity bit must not be exposed for secret bounds. */
int ctint1728_32_div_trunc(ctint1728_32_t *quotient,
                           ctint1728_32_t *remainder,
                           const ctint1728_32_t *a,
                           const ctint1728_32_t *b,
                           ctint1728_32_div_workspace_t *workspace);

/* Floor-style signed modular reduction matching current ibz_mod semantics:
 * out has the modulus's sign (or is zero), |out| < |modulus|, and
 * a = q*modulus + out for some integer q.  The implementation always runs
 * the complete truncating-division and adjustment schedules.  A zero modulus
 * returns zero and leaves an initialized out object byte-identical;
 * INT_MIN mod -1 succeeds with zero.  Input/output aliasing is supported.
 * Workspace must be disjoint from all live objects, is cleared on every
 * return, and the validity bit must not be exposed for a secret modulus. */
int ctint1728_32_mod(ctint1728_32_t *out,
                     const ctint1728_32_t *a,
                     const ctint1728_32_t *modulus,
                     ctint1728_32_mod_workspace_t *workspace);

/* Fixed 521-round square-and-multiply-always modular exponentiation.  The
 * exponent must be nonnegative and fit in 521 bits; the modulus must be
 * positive and fit in 863 bits.  The latter bound proves that every product
 * of two reduced residues fits the signed 1728-bit multiplication range.
 * Every call executes 521 rounds, two multiplications and two complete
 * reductions per round, plus fixed input reductions.  Invalid inputs execute
 * the same schedule, return zero, and leave an initialized output
 * byte-identical.  Output may alias any input; workspace must be disjoint and
 * is completely cleared.  This regularizes control flow and addresses only;
 * it does not mask exponent-dependent data switching. */
int ctint1728_32_pow_mod_521(ctint1728_32_t *out,
                             const ctint1728_32_t *base,
                             const ctint1728_32_t *exponent,
                             const ctint1728_32_t *modulus,
                             ctint1728_32_pow_workspace_t *workspace);

/* Exact nonnegative integer square root.  The implementation executes all
 * 864 radix-4 restoring rounds.  Return one and publish floor(sqrt(in)) only
 * when its square equals in.  A negative or nonsquare input runs the same
 * schedule, returns zero, and leaves an initialized output byte-identical.
 * Output may alias input; workspace must be disjoint and is fully cleared.
 * Exposing the exact-square status may itself leak a secret predicate. */
int ctint1728_32_sqrt_exact(ctint1728_32_t *out,
                            const ctint1728_32_t *in,
                            ctint1728_32_sqrt_workspace_t *workspace);

/* Floor square root with the same complete 864-round radix-4 schedule.
 * Return one and publish floor(sqrt(in)) for every nonnegative input.  A
 * negative input executes the same schedule, returns zero and leaves an
 * initialized output byte-identical.  Output may alias input; workspace must
 * be disjoint and is fully cleared. */
int ctint1728_32_sqrt_floor(ctint1728_32_t *out,
                            const ctint1728_32_t *in,
                            ctint1728_32_sqrt_workspace_t *workspace);

/* Fixed-iteration half-Euclid used by Level-I Cornacchia.  For positive
 * modulus <=492 bits and 0<=root<modulus, publish the same first Euclidean
 * remainder r for which r^2 < modulus as the legacy division loop.  The
 * implementation computes ceil(sqrt(modulus)) once and then executes the
 * Bernstein--Yang-derived conservative bound
 * floor((49*492+57)/17)=1421 shift/subtract rounds.  Completed states freeze
 * by conditional move, so shorter inputs do not shorten the trace.
 *
 * Invalid inputs and an exhausted bound execute the complete schedule, return
 * zero and leave output byte-identical.  Output may alias either input;
 * workspace must be disjoint and is fully cleared.  The routine regularizes
 * control flow and addresses only: its comparisons, masks and arithmetic data
 * remain unmasked and can still leak through power or EM observations. */
int ctint1728_32_cornacchia_halfgcd_492(
    ctint1728_32_t *out,
    const ctint1728_32_t *modulus,
    const ctint1728_32_t *root,
    ctint1728_32_halfgcd_workspace_t *workspace);

/* Shift an unsigned magnitude.  Left shift additionally rejects any value
 * that would discard a set high bit.  Right shift is logical; callers
 * requiring signed truncating division must take the magnitude and restore
 * the sign explicitly.  Valid shifts are 0..1727.  On an invalid count or
 * left-shift overflow, return zero and leave out byte-identical.  Input/output
 * aliasing is supported.  Workspace must not overlap any live input or output
 * object and is cleared on every return. */
int ctint1728_32_lshift(ctint1728_32_t *out,
                        const ctint1728_32_t *in,
                        uint32_t shift,
                        ctint1728_32_shift_workspace_t *workspace);
int ctint1728_32_rshift(ctint1728_32_t *out,
                        const ctint1728_32_t *in,
                        uint32_t shift,
                        ctint1728_32_shift_workspace_t *workspace);

void ctint1728_32_secure_clear(void *object, size_t bytes);

#ifdef __cplusplus
}
#endif

#endif
