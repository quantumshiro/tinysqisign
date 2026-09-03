# SQIsign v3.0 D1: LLL lifetime overlay

## Transformation

`quat_lll_dual_reduce_ideal` has two explicit phase boundaries:

1. `fp_ldl_t` is live while the LLL/Minkowski reduction is evaluated.
2. The final `Aout` matrix is constructed only after the reduction state is
   dead.

D1 represents those mutually exclusive values as members of one union.  It
also reuses the now-dead `ct_mk_result_t::Ainv_total` subobject as `Bout`, while
retaining the still-live `G` and `T` subobjects.  Compile-time extent checking
guards the second overlay.  The algorithm, public API, branch decisions, and
random-byte consumption are unchanged.

The generated `p324_3/m4f/lll_dim4.c` compiled into the RP2350 image is
byte-identical to the modified tracked source.  Both files have SHA-256
`9a9b6643d53ad14320b6c584443ee1a8bb939c580669d5d96e95d2ec703ec403`.

## Validation

- Host NIST API test: PASS.
- Host self-test: PASS.
- Host official KAT: PASS.
- RP2350 KAT #0: public key, secret key, signed message, and opened message all
  match byte for byte in all ten clean-harness captures of the five-pair
  interleaved campaign; every capture reports `firmware_dirty=0` and status
  PASS.  The D1 source is reconstructed from the pinned official commit and
  the hashed D1 patch recorded by the campaign summary.

## Baseline-to-D1 observations

| Metric | Official v3 baseline | v3-D1 | Difference |
|---|---:|---:|---:|
| `quat_lll_dual_reduce_ideal` GCC frame | 34,816 B | 30,888 B | -3,928 B |
| KeyGen PSP watermark | 56,972 B | 56,972 B | 0 B |
| Sign PSP watermark | 101,060 B | 97,132 B | -3,928 B (-3.887%) |
| Verify PSP watermark | 37,444 B | 37,444 B | 0 B |
| Linked crypto archive text | 204,157 B | 203,945 B | -212 B (-0.104%) |
| KeyGen time, median of 5 | 2,263,654 us | 2,269,166 us | paired median +0.2416% |
| Sign time, median of 5 | 7,152,690 us | 7,182,418 us | paired median +0.4190% |
| Verify time, median of 5 | 822,142 us | 820,780 us | paired median -0.1713% |

The campaign alternated the order within each pair: odd rounds ran official
v3 then v3-D1, and even rounds ran v3-D1 then official v3.  The paired Sign
delta ranged from +0.4112% to +0.4204%; its mean was +0.4176% and its median
was +0.4190%.  The PSP observations were identical in all five repetitions.
This supports a narrow runtime statement for this binary, board, clock, and
deterministic KAT vector.  It is not a workload-distribution or independent-
board performance estimate.  The nonzero KeyGen and Verify timing deltas also
show that code placement and whole-image effects cannot be excluded even
though D1 changes only one source file.

Machine-readable inputs and summaries are in
`results/v3/rp2350/interleaved-clean-2026-09-04/measurements.csv` and
`results/v3/rp2350/interleaved-clean-2026-09-04/summary.json`. The earlier
dirty-firmware campaign is omitted from the current tree, remains recoverable
from Git history, and is not used for the values above.
