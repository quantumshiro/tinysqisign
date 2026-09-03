# v3 p324_3 static-stack prototype

The official/D1 RP2350 builds each contain 19 GCC `dynamic` stack-usage
records.  Two are reported as `dynamic,bounded`; the other 17 arise from VLAs
in seven generated m4f translation units, including inlined helpers.

`scripts/materialize_v3_static_stack_prototype.py` converts only those generated
sources into a parameter-specific experiment.  It replaces argument-sized
arrays with p324_3/RADIX32 maxima and inserts entry assertions.  The important
bounds are `IBZ_NLIMBS = 60`, chain length at most 326 (ten saved split levels),
at most three pushed theta points, FP2 batch length at most 12, DLP recursion
depth at most ten, and encoded integer length at most eleven 32-bit digits.

Acceptance requires:

1. exact source-anchor and digest checks during materialization;
2. an Arm build with `-Werror=vla -Werror=alloca`;
3. zero `dynamic` records in every linked cryptographic translation unit;
4. the official host known-answer test;
5. an RP2350 known-answer run and PSP/MSP watermark comparison.

Passing (1)--(3) bounds the individual generated frames for this parameter and
compiler.  It does not alone establish a whole-program worst-case call-chain
bound; (4)--(5) remain finite dynamic cross-checks.
