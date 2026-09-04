# v3 D3 two-function RP2350 cross-check

This directory records the clean target cross-check for the two-function D3
lifetime schedule. The firmware commit is
`5df7acfb2e32019305094546939bdc50bbf71e00`; the materialized SQIsign v3
source commit is `874658c64aa2e20f53b1f4d696144723d558ed5c`. Both report
`dirty=0`, and the generated `p324_3/m4f` tree is bound by SHA-256.

The embedded official response vector 0 completed KeyGen, Sign, and Verify and
terminated with `status=PASS`. Relative to the five-capture official median,
the Sign PSP watermark changed from 101,060 to 96,396 bytes, a reduction of
4,664 bytes (4.6151%). The second scheduled function accounts for 736 bytes
of the reduction beyond the earlier one-function D1 result. This directory
contains one D3 boot, so its timing is logged but is not used to estimate a
performance distribution.

The target result is paired with host API, self-test, and 100-response checks
for all three official parameter sets in
`results/v3/host/validation-all-params-2026-09-04.json`. Across the official
and D3 trees these checks cover 300 distinct parameter-set/request pairs, each
run on both implementations, for 600 implementation-vector runs. The
corresponding Arm frame audit covers both modified functions in all three
parameter sets and records a reduction in all six comparisons.

`summary.json` verifies the capture identity, source and firmware cleanliness,
the generated-tree digest, the official target response, archived ELF/UF2/map
and library digests, and the public patch and bundle digests. The result does
not establish a whole-program interrupt/MSP stack bound, a timing distribution,
or side-channel resistance. RP2350 whole-image measurements cover only
`p324_3`; the other two parameter sets have host functional and Arm object-frame
evidence only.
