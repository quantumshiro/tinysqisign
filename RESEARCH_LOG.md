# Research log

This log retains rejected ideas as well as accepted evidence. “Accepted” means supported for the stated narrow conclusion; it does not imply that the final firmware exists.

## Cycle 0 — freeze the question

**Observation.** SQIsign entered NIST Additional Signatures Round 3 in May 2026, but no Round-3 SQIsign tweak package was public on 2026-08-15. v2.0.1 remained the newest specification found.

**Hypothesis.** The v2 source lineage can serve as a stable experimental baseline if Round-3 compatibility is explicitly provisional.

**Test.** Compared the v2.0 and v2.0.1 specifications, official tags/current main, NIST pages, and relevant post-v2 papers.

**Result.** Accepted provisionally. `VERSIONS.md` freezes commits and records the non-cosmetic v2.0.1 correction. Any Round-3 package reopens this cycle.

## Cycle 1 — does existing work already supersede the question?

**Observation.** Published Cortex-M work gives current-v2 verification and older one-dimensional verification; full current-v2 K/S/V exists on hosts and Armv8-A.

**Hypothesis.** A public full, heap-free, GMP-free current-v2 Cortex-M signer already exists.

**Test.** Version/function/platform/dependency review through 2026-08-15, including pqm4, optimized 1D, m4-modarith, fixed/compact arithmetic, Qlapoti(+), SQIsign on ARM, constant-time work, and work published after them.

**Result.** Rejected based on public evidence found. Continue without a “first” claim; re-audit before publication.

## Cycle 2 — direct fixed precision

**Observation.** The fixed implementation replaces GMP with 110 64-bit limbs at Level I.

**Hypothesis.** Removing GMP/dynamic allocation is sufficient for an RP2350 port.

**Expected effect.** Predictable memory and no heap, possibly within 520 KiB.

**Test.** Built and executed full K/S/V; inspected `find_uv` VLAs; bracketed host stack.

**Measurement.** 45,032,860-byte source-semantic `find_uv` VLA sum; optimized KeyGen fails at 23 MiB and passes at 24 MiB host stack; full correctness passes with raised stack.

**Result.** Rejected. Fixed precision is necessary but global maximum precision plus unscheduled table lifetime is grossly insufficient.

## Cycle 3 — direct compact precision

**Observation.** The frozen compact artifact uses 27 64-bit limbs (216 bytes) for Level-I integers.

**Hypothesis.** Compact quaternion bounds alone make a direct full port fit.

**Expected effect.** About 4× smaller integer objects than the direct fixed baseline.

**Test.** Built/executed full K/S/V; audited symbols/source; measured `sizeof`; captured allocation high-water caller stacks.

**Measurement.** `find_uv` allocates 6,339,868 bytes in five simultaneous blocks during K and S. Verify has no crypto heap. K/S/V correctness passes.

**Result.** Rejected as a direct port. Compact precision becomes variant B and the transformation base.

## Cycle 4 — stack-to-workspace only

**Observation.** The final firmware forbids dynamic allocation.

**Hypothesis.** Replacing five `malloc` calls with one static/caller buffer solves the embedded problem.

**Expected effect.** Heap becomes zero; total live storage unchanged at 6,339,868 bytes.

**Result.** Rejected without implementation because exact accounting proves it cannot fit. It remains a useful first refactor for ownership/differential testing, but must not be reported as a memory reduction.

## Cycle 5 — candidate-coordinate precision specialization

**Observation.** A resident candidate coordinate uses a full 216-byte integer.

**Hypothesis.** Enumeration-table coordinates can use signed bytes safely.

**Derivation.** Level-I `FINDUV_box_size=2`, and the four loops store only values in `[-2,2]`. Three signed mathematical bits suffice; `int8_t` is conservative.

**Expected effect.** `small_vecs` falls from 3,773,952 to 17,472 bytes, saving **3,756,480 bytes**. The sort record copies the same vector and falls from 1,088 to 224 bytes, saving another **539,136 bytes** over 624 records. Widening four selected coordinates uses an existing transient vector.

**Correctness implication.** None if conversion is exact; selected solution and matrix evaluation must remain byte-identical under deterministic RNG.

**Constant-time implication.** Fixed-loop conversion is regular; surrounding selected-index search remains variable-time.

**Test.** Implemented at `254eda3…`; ran Level-I/III/V hypercube regressions, full Level-I K/S/V, ten signature iterations, AddressSanitizer, and twelve seeded baseline-vs-packed transcript comparisons.

**Measurement.** The five live blocks fall from 6,339,868 to **2,044,252 bytes**, a **4,295,616-byte / 67.7556%** reduction. Actual allocation events match the source/type calculation. All twelve transcripts are byte-identical. Twenty deterministic host samples resolve no slowdown (mean K ratio 0.997; S ratio 0.993 at 10 ms timer resolution); linked host `__text` changes by −264 bytes.

**Result.** Accepted as an isolated bound-aware representation transformation. It is not heap-free and remains 3.839× RP2350 SRAM.

## Cycle 6 — remove full sort records

**Observation.** Before Cycle 5, a 678,912-byte array copied 624 full vectors and norms solely to sort each row. Packing the vector copy reduced this intermediate to 139,776 bytes, but it remains unnecessary duplication.

**Hypothesis.** Sorting a `uint16_t[624]` permutation and applying/using it indirectly is equivalent.

**Expected effect.** Relative to the implemented packed experiment, sort scratch falls from 139,776 to 1,248 bytes, saving another **138,528 bytes**. Relative to the original compact baseline, the cumulative sort-scratch saving is 677,664 bytes.

**Correctness risk.** Equal-norm ordering and “first solution” behavior could change; deterministic signatures may differ even if valid. Comparator/order semantics require an explicit regression.

**Constant-time implication.** Ordinary comparison sorting remains data-dependent; a sorting network would be a separate CT profile.

**Result.** Plausible, pending minimal implementation and differential test.

## Cycle 7 — recompute quotients

**Observation.** All 4,368 values `n / norm[i]` remain live although list matching uses only the active row pair.

**Hypothesis.** Compute the quotient/remainder at its active scan use instead of storing it.

**Expected effect.** Saves **943,488 bytes** in the all-row compact layout; adds repeated fixed-width division.

**Correctness risk.** Remainder/error paths must be preserved. The quotient is deterministic and has no RNG effect.

**Constant-time implication.** Repeated variable-time division may amplify timing/power leakage.

**Result.** Plausible, pending cycle/trace measurement.

## Cycle 8 — two-row lifetime schedule

**Observation.** `find_uv_from_lists` compares one `(j1,j2)` row pair at a time, but all seven rows remain live.

**Hypothesis.** Hold one outer and one inner row; regenerate discarded deterministic rows for later pairs.

**Expected effect.** With packed vectors/full-width norms, quotient recomputation and one index-sort scratch, `find_uv` has **275,836 component bytes and a likely 275,840-byte aligned reservation**. Preserving all-row validation requires seven validation generations plus at most 28 search generations: 35 versus seven in D2, up to 5× the enumeration component.

**Correctness risk.** Pair order, sorting and selected-first-solution behavior must remain identical. Enumeration must be confirmed RNG-free for the entire row construction.

**Constant-time implication.** More secret-derived arithmetic changes power/timing distribution; workspace offsets remain public/fixed.

**Result.** Feasibility hypothesis accepted for implementation, not yet an achieved measurement.

## Cycle 9 — narrow norms from empirical observations

**Hypothesis.** Use the largest norm seen in randomized tests to choose a smaller limb count.

**Result.** Rejected categorically. Empirical maxima cannot replace a proven worst-case bound. Instrumentation may guide proof work but cannot configure production precision.

## Cycle 10 — adaptive retry scheduling as prior art

**Observation.** A 2026 paper uses “adaptive scheduling” for commitment candidate ranking.

**Hypothesis.** It supersedes bound-aware lifetime scheduling.

**Test.** Read the paper and checked version disclosure, source, memory results and live ablation.

**Result.** Rejected. It schedules retry candidates, not object storage; exact code/version and memory are not reported; live fixed-degree features have AUC 0.50 and Rank-ML is slower than baseline. It remains a negative time/side-channel comparator.

## Cycle 11 — sanitizer attribution

**Observation.** A combined Address/UndefinedBehaviorSanitizer build stopped in `dpe_div` while testing the packed experiment.

**Hypothesis.** Narrowing candidate vectors introduced undefined arithmetic.

**Test.** Ran AddressSanitizer alone on baseline and packed K/S/V, then replayed the exact failing RNG seed under the same combined sanitizer on frozen baseline `5b94b09…`.

**Measurement.** AddressSanitizer passes both builds. Both combined builds stop at `dpe.h:459` on `-2147483648 - 638` in the ML2 KeyGen path; the exponent sentinel `INT_MIN` is involved before `find_uv` executes.

**Result.** Hypothesis rejected. This is a reproducible upstream-baseline UB, not caused by packed vectors. It remains an unresolved correctness/security issue and prevents a blanket “UBSan clean” claim.

## Cycle 12 — physical RP2350 transport

**Observation.** The board became visible in BOOTSEL as Model/Board-ID `RP2350`.

**Hypothesis.** The frozen Cortex-M33 toolchain can build, flash and recover deterministic reports without SWD or external RAM.

**Test.** Pinned Pico SDK 2.3.0, built an ARM Secure Pico 2 UF2, flashed by mass storage, captured USB CDC, and returned to BOOTSEL via the 1200-baud reset path.

**Measurement.** Physical report: CPUID `0x411fd210`, core 0, 32-bit ABI, 150 MHz system clock, 48 MHz USB clock. Probe ELF is 37,352 B text and 3,020 B BSS; source frames are 16 B and 8 B.

**Result.** Accepted for transport/toolchain feasibility only. No SQIsign operation has yet run on target.

## Cycle 13 — independent correctness, measurement and RP2350 review

**Observation.** A locally passing port can still hide stale binaries, miscount RAM, link a hidden allocator, use an undersized SDK stack, or inherit an older decoder.

**Hypothesis.** Three independent read-only reviews can falsify the Verify implementation and measurement claims before they become a recorded milestone.

**Test.** Separate agents audited (1) cryptographic source closure/KAT provenance/malformed-input behavior, (2) ELF/section/stack arithmetic, and (3) Cortex-M33 PSP/AAPCS/linker/physical-release mechanics. They rebuilt host RADIX32 verification in normal, ASan+UBSan and LTO modes; ran all 100 archived NIST-v2 vectors plus the Compact fixture; inspected source, symbols, disassembly and linker sections; and cross-compiled the wider source boundary.

**Findings and corrections.** The reviews found no Critical/High defect in the final Verify-only closure, but did find and cause correction of several material issues: the SDK default stack is only 2 KiB; GNU `size` misclassifies executable RAM `.data`; a stdio path initially retained allocator symbols; an early target vector was mislabeled; the frozen pqm4 verifier predates current decoder fixes; the first ELF/capture evidence was stale relative to source; and the release script needed broader symbol/section checks. All were fixed or the associated claim was withdrawn before the clean artifact.

**Residual findings.** The audit exposed a full-build RADIX32 `digit_t`
collision, a Level-I Clapotis/id2iso integration-test failure, and several
baseline signer UB sites. The first is resolved in Cycle 16; the latter two
remain correctness gates. The PSP watermark is an overwritten extent, not a
strict maximum. See `AUDIT_LOG.md`.

**Result.** Verify-only source-level approval, conditional on a clean rebuild, hash freeze and complete physical recapture. No approval of KeyGen/Sign.

## Cycle 14 — physical heap-free Level-I Verify

**Observation.** Host and ELF evidence did not yet show that real current-v2-style verification executes on the Cortex-M33.

**Hypothesis.** The audited Compact Level-I/RADIX32 Verify closure fits comfortably in internal SRAM and can run without allocator, GMP, RNG or external RAM.

**Test.** Clean-built project commit `40d06653130039dd57304e4d5339c6b575e14c01`, reran the post-link policy gate, froze ELF/UF2/BIN hashes, flashed the connected Pico 2, and captured the full `dirty=0` banner through `status=PASS`. The target ran an archived official NIST-v2 vector, a Compact regression fixture, and malformed signature cases.

**Measurement.** Both valid fixtures passed in one physical run at 817,184 and 817,663 microseconds. Each changed 30,912 bytes of the 64-KiB PSP reservation. The conservative exclusive SRAM reservation is 82,100 bytes: 73,908 bytes through main BSS end plus an 8,192-byte MSP region. Heap, PSRAM and XIP-RAM are zero; the raw flash image span is 81,052 bytes.

**Result.** Accepted as the first physical cryptographic milestone: full Level-I **Verify**, not full SQIsign K/S/V. Timing and watermark values are correctness-smoke observations, not paper benchmark distributions or exact peaks.

## Cycle 15 — compile the full RADIX32 boundary

**Observation.** Verify omits quaternion and IdealToIsogeny, so its successful link cannot establish that KeyGen/Sign sources are 32-bit-buildable.

**Hypothesis.** The remaining frozen Compact sources compile unchanged with `RADIX_32` for Cortex-M33.

**Test.** Cross-compiled `dim2id2iso.c` with the same Arm GNU 15.2.1 and Level-I/RADIX32 configuration.

**Measurement.** Compilation fails because field `tutil.h` defines `digit_t` as a `uint32_t` macro while quaternion `intbig.h` declares a distinct 64-bit `digit_t` typedef. Both become visible in the same translation unit.

**Result.** Hypothesis rejected. The next minimal implementation change is a namespace separation that preserves the 64-bit quaternion limb ABI and 32-bit field limb ABI. It must be reviewed and tested independently before workspace work is trusted on ARM.

## Cycle 16 — separate quaternion and field digit types

**Observation.** The collision is a C namespace problem, not evidence that the
quaternion precision should become 32-bit-limbed or that field arithmetic should
remain 64-bit.

**Hypothesis.** Naming the fixed 64-bit quaternion limb `ibz_digit_t`, retaining
the conditional field `digit_t`, and using explicit checked u32 split/join at
the boundary preserves all serialized and arithmetic behavior while making the
full source boundary Cortex-M33/RADIX32-buildable.

**Implementation.** Commit `9a92e70341a4f52a81e42bcc77d25bf757cfe546`
applies that separation. It removes the checked-result-discarding array macro,
makes every call site explicit, and treats an impossible production export
capacity as fatal. The corresponding patch SHA-256 is
`0e67f87282cf21c85000fdaac72a8f73d1fc4619f8a04520ad46453f5b5812e5`.

**Test.** RADIX32/64 Release builds complete for Levels I/III/V. The targeted
RADIX32 suite passes eight tests; Level-I quaternion, signature, NIST API and
K/S/V tests pass in RADIX32, RADIX64 and RADIX32 ASan builds. Twelve seeds
`0,1,2,3,5,8,13,21,34,55,89,144` produce byte-identical 1,210-byte PK/SK/SM
transcripts across radices. Seven signer-boundary translation units compile for
Cortex-M33 with Arm GCC 15.2.1 and `-Werror`. A broad RADIX32 run excluding the
separately tracked Level-I id2iso failure passes all completed tests; the sole
remaining result is a Level-V id2iso timeout at 1,500 seconds, not a mismatch.

**Audit.** An independent agent reviewed the final diff, conversion algebra,
minimum fixed integer, capacity failures, namespace symbols, Arm ABI,
serialization, sanitizer behavior and both-radix builds. It approved the
functional correction with no diff-specific Critical/High/Medium correctness
finding. Residual variable-time export, silent-truncating internal import API,
big-endian execution coverage, baseline UB and the id2iso test remain explicit.

**Result.** Accepted as the full-source RADIX32 portability boundary. It is not
a heap-free Sign result and does not change any memory projection.

## Cycle 17 — test whether Clapotis status zero is a same-input transient

**Observation.** Source-neutral LLDB tracing shows the first Level-I Clapotis
integration call returns status zero, while inner `find_uv` succeeds and the
first fixed-degree construction and nested `quat_represent_integer` return zero.
The selected fixed-degree input has `u=1`, `w2=0`, and `w6=0`.

**Hypothesis.** Repeating the same ideal a bounded number of times may turn the
ordinary zero status into success.

**Test.** A temporary test-only helper repeated the same operation sixteen
times in normal, RADIX32 and sanitizer builds. All sixteen attempts returned
zero. The experiment was completely reverted.

**Result.** Hypothesis rejected. Production callers distinguish zero ordinary
retry from negative fatal failure and regenerate outer inputs. The next test
hypothesis is therefore a bounded retry that regenerates the input ideal, not a
same-input loop and not an unproved `u=1` identity shortcut. The exact API/test
contract must be established before modifying code.

## Cycle 18 — align the id2iso test with the production retry contract

**Observation.** The API documentation and all KeyGen/Sign callers treat zero
as an ordinary miss, negative as fatal and positive as success. The inherited
integration test instead required immediate success for every fixed and random
input. A diagnostic sweep found fixed-ideal statuses `[0,1,1,1,1,1]`; the zero
case was index 0 with `u=1`. Eleven subsequently regenerated outer ideals all
succeeded.

**Hypothesis.** A test-only bounded retry at the production outer-ideal scope
can restore meaningful coverage without changing the cryptographic algorithm or
hiding a failure at another connecting ideal.

**Implementation.** Commit `a7e1e7f689a68adeeaa1a3659b14ba69f789ffcf`
allows fixed-ideal status zero only when `index==0 && u==1`; zero at every other
fixed ideal and all negative statuses fail. Every successful fixed result is
checked for point order and exact precomputed basis equality even under
`-DNDEBUG`. The random portion regenerates primes, quaternion representation and
ideal on every attempt, invokes the production wrapper, rejects fatal status,
and requires eleven successful basis-order-checked evaluations within 88 public
test attempts.

**Test.** Known-seed Release runs pass RADIX64 Levels I/III/V and RADIX32 Level
I. RADIX32 ASan Level I passes. An independent rerun also passed eight Level-I
Release seeds. Both-radix all-level test targets compile. Production sources and
archives are unchanged by construction.

**Audit.** The first proposed generic `N-1` success count was rejected during
review because it could hide a systematic miss at any index. After narrowing
the exception to index 0 with actual `u=1`, the independent reviewer approved
the final change with no blocking finding. The known index-0 curve/basis cannot
be inspected on the ordinary-miss run itself; this intentional limitation is
recorded rather than presented as full precomputation coverage.

**Result.** Accepted as a test-contract correction, not an SQIsign algorithm
change. The prior Level-I id2iso blocker is closed; known signer UB is next.

## Cycle 19 — remove DPE zero-sentinel UB without masking the ML2 defect

**Observation.** UBSan first reported exponent overflow in `dpe_mul` and
`dpe_div` when DPE's canonical zero exponent `INT_MIN` entered ordinary
addition or subtraction. An initial zero-numerator shortcut removed the first
report, but independent review found a real nonzero-over-zero division in ML2:
at `d=16`, `outer_iter=20`, an eager post-insertion refresh reached row 7,
pivot 5 before the dependent pivot had been reduced and moved to the skipped
prefix.

**Rejected hypotheses.** Treating every `0/0` as a harmless zero was rejected
because it hid the later nonzero-over-zero counterexample. Aborting only at the
DPE layer was also rejected because `dpe_div` has no failure channel and the
partially updated ML2 state must not be published.

**Implementation.** Compact commit
`5fdd698e52ba082ae0076b1a5356a5b9f5645a23` makes zero multiplication and
zero-numerator/nonzero-denominator division produce canonical DPE zero before
sentinel exponent arithmetic. ML2 no longer eagerly factorizes all rows after
an insertion; the main loop refreshes them in order through
`LazySizeReduce`, as required by the algorithm. Before division, ML2 accepts
only a finite positive Gram--Schmidt pivot. A bad pivot aborts the attempt;
`quat_ml2` publishes no partial output and the deterministic retry driver starts
from fresh state. Patch SHA-256:
`6b6b586f256807b0065922ce2380c31615da26e0d00308785486288d97413ed9`.

**Test.** Focused zero/alias and ML2 exact-span/nonpublication tests pass in
RADIX64, RADIX32, ASan+UBSan, ASan-only and RADIX32-ASan builds. Deterministic
ML2 stress at `d=4,8,16`, 1,000 samples per dimension and radix, reports zero
base/retry failures, rank errors and fast-path mismatches. Twelve Level-I
KeyGen/Sign/Verify transcripts remain byte-identical to the frozen baseline.
The selected RADIX32 suite passes 8/8 and the Cortex-M33 signer boundary passes
7/7 translation-unit compilation. Recovery-mode Level-I UBSan with seed 1
reports only the separately tracked `normeq.c:268-269` and
`theta_isogenies.c:890` sites; no DPE/ML2 UB remains on that path.

**Audit.** Independent review first rejected the incomplete zero shortcut with
a High-severity counterexample, then verified the corrected insertion-state
invariant, abort propagation, alias safety and Arm compilation. Its final
verdict on diff SHA-256 `fef2c180c600c1510cec80c7d2d64c85e8b6b7e29de7f0cdc80a5173ac14530b`
was approval with no blocking issue. Direct fixed fixtures for the defensive
bad-pivot branch and the exact deep insertion remain desirable coverage; the
deterministic full-scheme seed and stress corpus are current regression
evidence. ML2 remains variable-time.

**Result.** Accepted as an arithmetic/correctness repair. It changes no
workspace size and is not counted as a memory reduction.

## Cycle 20 — remove the norm-equation subtraction UB without changing transcripts

**Observation.** The inherited predicate narrowed two fixed-precision integers
to signed 32-bit values and subtracted them before `% 4 == 2`. The subtraction
overflowed on a reachable Level-I KeyGen path. Its de-facto target behavior was
also one-sided: a wrapped positive remainder `+2` passed, while the congruent
negative remainder `-2` did not.

**Rejected hypothesis.** Replacing the expression by canonical mathematical
reduction modulo four removed the UB, but changed frozen deterministic
KeyGen/Sign/Verify transcript 0 from SHA-256 `5a78739a…` to `d9e986…`. That
change broadens candidate acceptance and therefore changes retry/RNG behavior;
it was rejected rather than hidden inside a C-portability repair.

**Implementation.** Compact commit
`c56c441b3540d9d34c2574d83c59e9fc7a14fa7a` computes the low-32-bit difference
with unsigned subtraction. It accepts exactly when the wrapped word is below
`0x80000000` and its low two bits are `10`, which is equivalent to the legacy
two's-complement `signed32(difference) % 4 == 2` predicate without signed
overflow or implementation-defined narrowing. The generic helper is explicitly
namespaced across Levels I/III/V. Patch SHA-256:
`bce7878cb181fda3d96686d3b33314d44e4c6b1c6acffb8774b5dcbea16ae96a`.

**Test.** Full quaternion tests pass for all levels and both radices. Level-I
RADIX64 combined ASan+UBSan and RADIX32 ASan pass; recovery-mode execution no
longer reports either `normeq.c` subtraction. The selected RADIX32 suite passes
8/8, twelve RADIX32/RADIX64 deterministic transcripts remain byte-identical,
the Cortex-M33 signer boundary compiles 7/7 with `-Werror`, and `nm` exposes
only the namespaced helper in all three quaternion archives.

**Audit.** Independent review first rejected the draft as High severity because
the new generic symbol lacked the repository namespace mapping. After that was
fixed and every dependent object rebuilt, the final diff SHA-256
`61dbe0068f24513654298f03bb913a4b8cbffb7f0836223bb10a574a004395d1`
was approved with no Critical, High or Medium finding. The reviewer independently
proved equivalence to the legacy wrapped predicate and repeated all-level,
both-radix, sanitizer, Arm and symbol checks.

**Result.** Accepted as a transcript-preserving UB repair. It deliberately does
not claim to implement canonical modulo-four semantics, changes no workspace
size, and is not counted as a memory reduction.

## Cycle 21 — remove the randomized-theta byte-assembly UB

**Observation.** `sample_random_index` promoted each random byte to signed
`int` before shifting. For a high byte at least 128, the 24-bit left shift was
not representable as `int` and UBSan reported undefined behavior on the signer
path.

**Implementation.** Compact commit
`ee982a2c00cc6b867c038d62de0c51db3e0ec03d` casts every byte to `uint32_t`
before the little-endian shifts. The 32-bit seed, unbiased rejection threshold,
mod-six reduction and RNG call order are unchanged. Patch SHA-256:
`ed4ebf6b0a1da18ad8880979a1085f2b66dd7a25b1f594244811d70017e5d1d9`.

**Test.** Recovery-mode combined ASan+UBSan KeyGen/Sign/Verify passes Levels
I/III/V with seed 1 and emits no sanitizer report. RADIX32 ASan independently
passes all three levels. The selected RADIX32 suite passes 8/8, all twelve
cross-radix transcripts retain their frozen hashes, and the expanded
Cortex-M33 signer boundary compiles 8/8 translation units with `-Werror`.

**Audit.** Independent review approved final diff SHA-256
`cd7e9d30c7b215d15fd50710c2a944fad2d030203c110257b7bcbd353a6b5c03`
with no Critical, High or Medium finding. It checked ISO C semantics,
endianness, rejection/RNG behavior and Arm code generation. The old and new
Cortex-M33 `.text` were byte-identical (SHA-256 `c8950a9d…`), so the source
repair adds no target control flow or memory access. A direct stub-RNG fixture
for the four rejected values remains desirable Low-severity coverage.

**Result.** Accepted as a transcript-preserving UB repair. The saved
all-level sanitizer paths now contain none of the four independently reproduced
UB classes. This is not a proof that the complete implementation is UB-free.

## Cycle 22 — typed full-row `find_uv` ownership baseline (C1)

**Observation.** P1 still owned five simultaneous allocator blocks even though their exact 2,044,252-byte component sum and lifetimes were known. Applying index sorting, quotient recomputation and row streaming before making ownership explicit would combine unrelated correctness and memory changes.

**Hypothesis.** A typed caller-owned `find_uv_workspace_t` can preserve every row, full-width norm/quotient, sort tie-break and first-match result while establishing a cleanup boundary. Expected memory saving: **0 bytes**; expected concrete size: P1 components plus ABI padding. The explicit project-code closure should have no direct allocator/GMP symbol, while a host compatibility wrapper may remain for differential tests.

**Implementation.** Compact commit `e61c1fa8fb4898fb606dac807727f97655254739` adds namespaced init/clear/with-workspace APIs and one compatibility `find_uv()` adapter. The core binds its former five blocks to typed members, checks capacities before indexing, finalizes every active integer and clears all bytes on normal return paths. Patch SHA-256: `dbd0017eaf3ab3236fe79b41b47b81b99e22eb266a49e63e9271e0da7c64584c`.

**Test.** Clean RADIX64 rebuild and Level-I id2iso/hypercube tests pass. Twelve P1/C1 PK/SK/signed-message transcripts remain byte-identical. Combined sanitizers, the saved scheme path, all-level namespace builds, and the Cortex-M33/RADIX32 eight-TU `-Werror` gate pass. `scripts/reproduce_c1.sh` reruns commit/patch, clean-build, differential, ABI, stack, direct-symbol closure and generated-artifact freshness checks.

**Measurement.** The compiled Level-I layout is 2,044,256 bytes, eight-byte aligned: 2,044,252 component bytes plus four bytes of internal padding. Host and Arm GCC 15.2.1 agree on all offsets. Therefore P1→C1 is **+4 bytes**, while B→P1 remains the 4,295,616-byte/67.7556% packing reduction. C1 is 3.83912× all RP2350 SRAM and exceeds it by 1,511,776 bytes. The initial `.su` record was `15,696 dynamic` for `find_uv_with_workspace` and 64 static for the wrapper; Cycle 25 corrects the crucial interpretation that 15,696 is only the fixed component.

Clean `ee982a2…` versus `e61c1fa…` deterministic host measurements used 20 alternating samples, five operations/sample and two warmups. Paired median C1/base ratios are KeyGen 1.02434 (+2.285 ms) and Sign 1.00211 (+0.478 ms). These repeat one seeded path and are not retry-distribution benchmarks. Mach-O changes are +328 `__text` and +8 `__data` bytes.

**Audit.** Independent implementation review approved pre-commit diff SHA-256 `61f19f313b3f07bccb9d9fa181fc06c7c2e66bf464be729e188b06cda47efceb` with no Critical/High/Medium code defect. Independent accounting review confirmed the host/Arm ABI, arithmetic and figure attribution. Both reviewers require the claim boundary: current end-to-end K/S still enter the one-allocation-per-`find_uv`-invocation wrapper (one observed invocation in KeyGen and two sequential invocations in Sign); the dashed caller-owned profile is projected; direct-symbol scanning excludes host-libc `qsort` transitive behavior; semantic first/last-use intervals remain uninstrumented. Dedicated member-extent canaries and direct wrapper-vs-explicit fixtures remain Low methodology hardening.

**Result.** Accepted as the ownership baseline. Rejected as a memory reduction, end-to-end heap-free result, lifetime-scheduling result, RP2350 fit, or constant-time result.

## Cycle 23 — harden the C1 ownership oracle

**Observation.** The first C1 tests covered normal explicit execution and
protocol behavior but did not directly compare every wrapper/explicit output,
force dirty caller storage, or freeze exact guard adjacency.

**Implementation.** Test-only commit
`0f438c26480946eb356e3d89aa18b57e20f9e4b4` adds exact status and all-output
comparison, 64-byte guards immediately before and after the typed object,
compile-time offset/alignment checks, dirty-workspace entry, invalid-input
handling and all-byte clear verification. Patch SHA-256:
`fc98b9fb5b352397c056e679157d03f4a412a5d61f911ca7c9f89fda1d55cc9e`.

**Audit/result.** Independent review approved final pre-commit diff SHA-256
`d9aaa68b00338cb36992bf795a67bd7818364404816124d3eb6026d5ab634d8d7`
with no blocking finding. This closes the outer guard/direct-equivalence gap
without changing production arithmetic or memory size.

## Cycle 24 — compact permutation sorting (D1)

**Observation.** C1's 139,776-byte `find_uv_sort_record_t[624]` duplicates
each packed vector and full-width norm solely to sort one row.

**Hypothesis.** Sorting a 1,248-byte `uint16_t[624]` index list by the exact
total key `(full-width norm, original enumeration index)` and applying the
destination-to-source permutation in place saves 138,528 bytes while retaining
the complete candidate order and protocol bytes.

**Implementation.** Commit
`cf9f6b6857996dc98f75117fec94ab8b9f0654f4` implements an in-project heap
sort and checked cycle application. It reuses the existing `remain` integer as
norm scratch only after its earlier last semantic read. Patch SHA-256:
`1a9e203f6c990b0f32b1b44c2fb1fc370ecb3fc4bbcc3c2356d5b7983f9233e`.

**Test.** Clean all-level RADIX64 id2iso/hypercube, Level-I K/S/V, RADIX32,
ASan+UBSan and Cortex-M33 gates pass. Twelve C1/D1 transcripts are byte-for-byte
identical. The explicit dead-stripped project-code closure imports neither an
allocator, GMP nor `qsort`; the full protocol harness still allocates through
the compatibility wrapper.

**Measurement.** Host and Arm report 1,905,724 component bytes, four tail
padding bytes and `sizeof=1,905,728`. C1→D1 saves exactly 138,528 bytes, or
6.7764507%. Thirty fixed-seed paired host samples resolve no slowdown: median
ratios are KeyGen 0.99943, Sign 1.00037 and Verify 1.00036. These are not target
or retry-distribution results.

**Audit/result.** Independent implementation review approved diff SHA-256
`45119e49a33e104350ff5b6d360e319d45a258b0874f706fa7e906bad608a9da`
with no blocking or Low defect, after exhaustive small heap/permutation models.
Independent accounting approved only the 138,528-byte D1 delta—not the
cumulative B→D1 reduction—and withheld full heap-free, target-fit,
constant-time and target-performance claims.

## Cycle 25 — correct dynamic-frame accounting

**Observation.** GCC's `15,696 dynamic` `.su` row had been described as if it
were the complete Cortex-M33 `find_uv_with_workspace` frame. Disassembly shows
it is only the fixed component.

**Measurement.** Each of the maximum seven Level-I rows adds one 216-byte
integer, two 3,456-byte matrices and one 3,896-byte ideal: 11,024 bytes per row
and 77,168 VLA bytes. The frozen `-Os`/soft diagnostic therefore has a
92,864-byte maximum own frame before callees. A separate unlinked probe under
the current Pico Release CPU/ABI/`-O3` flags reports 16,408 fixed and 93,576
own-frame bytes for D1. C1 is 93,584 bytes under those flags. The values exclude
callers, descendant calls, exception frames and final-link effects.

An observed KeyGen/Sign path reaches ML2 from within `find_uv` setup. Summing
every ancestor `-Os` frame on that source path gives a 273,200-byte diagnostic
chain before `quat_ml2` descendants and target wrapper state. Adding the likely
padded D3 workspace (275,840 bytes) already gives 549,040 bytes, 16,560 bytes
above all RP2350 SRAM before platform state. This is not a
linked upper bound, but it rejects leaving the stack untouched. `find_uv` VLAs
must be flattened first, followed by ML2/MLLL storage overlaid with a
non-overlapping candidate-row phase where live-range evidence permits.

**Result.** Accepted measurement correction. The D1 workspace delta is
unchanged. Previous 15,696-byte total-frame wording is withdrawn; exact target
CMake flags, full `.su` inventory, disassembly, linker map and multi-path stack
guards are mandatory for every physical signer profile.

## Cycle 26 — preregister D2 exact quotient recomputation

**Observation.** D1 retains `ibz_t quotients[4368]`, exactly 943,488 bytes.
Each entry is only `floor(target / norm[i])`; it is immutable, deterministic and
used as a comparison bound during list search. Its stored lifetime spans every
row pair even though the value can be derived from the still-live target and
norm.

**Hypothesis.** Removing the resident quotient array and recomputing the exact
full-width quotient at its comparison site preserves loop order and selected
protocol bytes. The enclosing Level-I workspace should fall from 1,905,728 to
962,240 bytes (962,236 components plus four tail-padding bytes), an exact
943,488-byte reservation reduction. A local 216-byte quotient may increase the
co-live function frame, so workspace and total-live deltas will be reported
separately.

**Semantic prerequisite.** D1 divides by every enumerated row norm before
search, whereas on-demand recomputation might never visit a later malformed
row after an early solution. Before removal, add and test a separate all-row
positive-norm validation step ahead of every division/search. This is
fail-closed hardening for invalid internal state, not a mathematical claim that
an empirical norm is safe. Valid-input transcripts and RNG consumption must
remain unchanged.

**Expected tradeoff.** The pure minimum-memory profile may repeat a division
for many `(i1,i2)` candidates; the Level-I no-early-exit combinatorial ceiling
is 9,541,896 candidate pairs, versus at most 4,368 resident quotient slots and
one precomputed division per populated row entry in D1, before noninvertible
pairs are excluded. Reject the pure profile as the balanced
configuration if measured Sign time exceeds 2× D1, but retain it as a valid
minimum-memory point if correctness and total-live accounting pass. A separate
one-row quotient-cache profile may add 134,784 bytes to recover speed; it must
not be conflated with pure D2.

**Acceptance gates.** Require exact workspace ABI on host and Cortex-M33,
wrapper/explicit all-output equivalence, twelve byte-identical transcripts,
all-level id2iso/hypercube tests, sanitizer and RADIX32 compile gates, direct
allocator/GMP/`qsort` closure audit, measured stack change, quotient-operation
counts and paired timing. Any change in candidate order, selected tuple,
signature bytes, valid-input status, or RNG transcript rejects the change.

**Implementation.** Commit `00f42908ce0147019cd2a1bce6444a2241f45506`
adds the preregistered all-row strict-positive validation before search. Commit
`d6801884d9c052450a7982e3ac69b29dab0f8893` removes the resident quotient
member and reuses one local `ibz_t` only after the modular inverse has been
consumed to form `v`. The old/new division operands, comparison boundary,
candidate order, first match, RNG and fatal/ordinary status remain identical.
The two diffs were independently audited separately; neither has a
Critical/High/Medium finding.

**Correctness.** Twelve frozen Level-I transcripts are byte-identical. The
RADIX64 all-level id2iso/hypercube suite and Level-I K/S/V pass. Focused
ASan+UBSan Level-I id2iso/hypercube/signature, a fresh RADIX32 Level-I
id2iso/hypercube/signature/NIST/scheme path, the Cortex-M33 eight-TU compile
gate, host/Arm ABI probes and direct allocator/GMP/`qsort` closure gate pass.
The explicit closure remains narrower than end-to-end K/S, whose compatibility
wrapper still allocates once per `find_uv` invocation (one observed invocation
in KeyGen and two sequential invocations in Sign).

**Memory result.** Exact `sizeof(find_uv_workspace_t)` is 962,240 bytes:
962,236 components plus four tail-padding bytes. D1→D2 workspace saving is
943,488 bytes (49.5080095%). Arm `-Os` fixed frame rises 15,696→15,912 bytes;
with the unchanged 77,168-byte VLA, own frame is 93,080 bytes. The co-live
saving is therefore **943,272 bytes**. A Pico Release-like unlinked probe gives
93,792 bytes. Workspace plus that own frame is still 1,056,032 bytes, so D2
cannot be directly linked into RP2350 SRAM.

**Time result.** Thirty clean, alternating, fixed-path host samples give D2/D1
median ratios KeyGen 1.00096, Sign 1.00182 and unchanged-path Verify 0.99256.
Paired mean deltas are +0.176 ms, +0.366 ms and −0.019 ms respectively. No
equivalence margin was preregistered, so the small positive K/S deltas are
reported rather than labeled “no slowdown.” Test-only instrumentation across twelve
seeds observes KeyGen one `find_uv` call with 1–29 D2 divisions (median 1.5,
mean 5) versus 661–748 reconstructed D1 divisions (median 717.5, mean 711.17),
and Sign two calls with 2–106 D2 divisions (median 8, mean 18.92) versus
1,381–1,502 reconstructed D1 divisions (median 1,438, mean 1,432.58). The
4,368 slots per invocation are a capacity ceiling, not an execution count.
This is not a retry distribution or target result; the preregistered
9,541,896-pair per-call ceiling remains. D2 also makes the division trace more
directly dependent on search progress and is explicitly non-constant-time.

**Result.** Accepted as the frozen D2 low-memory point. The measured common
path does not justify adding a quotient cache to the balanced profile yet, but
target distributions and worst-case bounds remain a gate. The transformation
does not establish heap freedom, target fit or side-channel resistance.

## Cycle 27 — D3 exact-order two-row lifetime schedule

**Observation.** After D2, seven packed-vector rows and seven full-width norm
rows remain resident for the whole triangular search. They occupy 960,960 of
the 962,240 workspace bytes even though each pair search needs only an outer
and inner row. Enumeration, sorting and validation are deterministic and
consume no RNG.

**Hypothesis.** Validate every row before search, retain only its count, then
regenerate rows through two fixed slots in the original triangular pair order.
This should preserve fail-closed priority and the complete
`(j1,j2,i1,i2,v)` stream while reducing the Level-I workspace to 275,836
components plus four bytes of ABI padding: 275,840 bytes. The preregistered
worst generation count is seven validation plus 28 search generations, versus
seven in D2.

**Implementation.** Commit `f70042d1fc97e370cf6d41e6e436677c5290ed0e`
extracts the canonical row preparation sequence. Commit
`60ce94495ea32943647a7c1b946c6750c2557d49` implements the two-phase schedule:
all-row validation precedes search; slot A owns the outer row; diagonal search
aliases A; slot B owns an off-diagonal inner row; regenerated counts must match
the validation pass; successful outputs are materialized before either slot is
overwritten.

**Audit correction.** Independent review found that the first init/finalize
loops flattened `&small_norms[0][0]` across the boundary of the first inner C
array. Although physical storage was contiguous and sanitizers did not flag
it, that pointer arithmetic is not portable ISO C. The draft was rejected.
Commit `b54922bd2de94b871bf4bd477a11de6e32bd17bf` uses explicit `[row][i]`
loops for both initialization and finalization. The reviewer then approved D3
with no remaining Critical/High/Medium finding.

**Correctness.** RADIX64 Level-I K/S/V and all-level id2iso/hypercube tests,
focused combined ASan+UBSan, a fresh RADIX32 Level-I id2iso/hypercube/signature
path and the Cortex-M33 eight-TU compile gate pass. Twelve 1,210-byte protocol
transcripts are unchanged. Two hash-pinned test-only patches emit every pair
header, complete norm lists, visited index pair and candidate `v`; all twelve
81,396–120,195-byte D2/D3 traces are byte-identical. This is stronger than
checking only the selected solution.

**Memory result.** Host and Cortex-M33 ABI probes agree on `sizeof=275,840`,
alignment 8, 275,836 components and four bytes of tail padding. D2→D3 saves
686,400 workspace bytes (71.3335550%). The frozen `-Os` outer fixed frame rises
16 bytes to 15,928; with the unchanged VLA it is 93,096 bytes. A 32-byte helper
is additionally co-live during row preparation, giving an approximate local-
path saving of 686,352 bytes. The actual observed pre-enumeration D3
Sign-to-ML2 ancestor path is 273,432 bytes. Workspace plus that partial path is
549,272 bytes, 16,792 bytes over all SRAM before descendants or platform
state. A separate Pico Release-like `-O3`/softfp source-object path is 273,496
bytes and gives 549,336 bytes, 16,856 bytes over SRAM. Neither is a linked
bound, but both show that D3 alone cannot be flashed as a safe full signer.

**Time/code result.** Thirty clean alternating fixed-path host samples give
D3/D2 median ratios KeyGen 0.99946, Sign 1.00009 and Verify 0.99999. Paired
mean deltas are +0.005, −0.028 and −0.005 ms. No equivalence margin was
preregistered; these are not speed, equivalence, target or retry-distribution
claims. Host Mach-O `__text` grows 552 bytes and measured mutable sections are
unchanged. The theoretical 35-generation ceiling and leakage amplification
remain open target questions.

**Result.** Accepted as the frozen D3 local workspace schedule. It neither
removes the end-to-end compatibility allocation nor fits the unflattened
signer. Repeated secret-derived enumeration/sorting may amplify timing or power
features, so no constant-time or SPA-resistance claim follows.

## Cycle 28 — D4 typed `find_uv` lattice-state ownership

**Observation.** D3 reduced candidate residency to 275,840 bytes, but four
Level-I VLA families remained simultaneously live on PSP: seven 216-byte
adjusted norms, fourteen 3,456-byte matrices and seven 3,896-byte ideals,
77,168 bytes total. GCC's `dynamic` `.su` record exposed only the fixed
component, and the observed workspace-plus-path sum remained above all RP2350
SRAM.

**Hypothesis.** Moving those exact objects into typed caller-owned storage will
remove variable-sized stack ownership and make a later phase union safe to
express. It must not be counted as a memory reduction: the preregistered
expected delta is workspace +77,168, observed ancestor path −77,168, total 0.

**Implementation.** Compact commit
`a6b06287706a97999d077d055818c2b5612a8704` introduces
`find_uv_lattice_state_t`, nests the existing D3 candidate workspace, binds the
old row pointers to the typed arrays, and retains symmetric initialization,
finalization and whole-object clearing. Patch SHA-256 is
`018e4c243e672b49ec32e85b59041bb0918526eaa15d5a22a4aac21d70731c6a`.

**Correctness and audit.** RADIX64 all-level id2iso/hypercube and Level-I
K/S/V, fresh RADIX32 Level-I tests, focused ASan+UBSan, twelve protocol
transcripts, twelve complete D3/D4 candidate streams, Arm eight-TU builds and
`-Wvla -Walloca -Werror` pass. Independent correctness review found no blocking
effective-type, alias, bounds, initialization or cleanup defect. Independent
measurement and RP2350-oriented reviews reproduced the exact ABI, `.su`,
assembly and call-path sums. The approval is limited to D4 stack ownership;
heap-free K/S, target fit and final linked peak remain unapproved.

**Memory result.** Host and Arm agree on `sizeof=353,008`, alignment eight,
353,004 component bytes and four bytes of tail padding. The lattice state is
77,168 bytes and the nested candidate workspace 275,840 bytes. Under `-Os`,
the frame becomes 15,928 static with zero VLA and the observed path becomes
196,264 bytes; under Pico Release-like flags they are 15,936 and 196,328.
Therefore 353,008 + 196,264 = 549,272 and 353,008 + 196,328 = 549,336, exactly
the D3 totals. D4 total co-live saving is **0 bytes**.

**Time/code result.** The stored 30-pair host run reports D4/D3 ratios KeyGen
0.97722, Sign 0.99842 and Verify 0.99964, but independent clean reruns clustered
near unity with drifting absolute medians. Those secondary raw reports were not
frozen, so no exact rerun ratio is retained. The apparent 2.3% speedup is not
resolved beyond scheduler/frequency/thermal noise and is rejected as a speed
claim. Host Mach-O `__text`
falls 500 bytes with measured mutable sections unchanged; neither observation
is a target result. D4 adds about 154,336 bytes of fixed clearing writes per
normal `find_uv` invocation relative to D3—about 154,336 bytes on the frozen
KeyGen path and 308,672 bytes across the two sequential invocations on the
frozen Sign path—and target timing/energy must measure that cost.

**Result.** Accepted as a stack-control prerequisite and rejected as a memory-
saving or performance transformation. This is the intended outcome: storage
ownership is now explicit enough to implement a standard-C typed phase
overlay without hiding PSP memory.

## Cycle 29 — D5 early-ML2/candidate phase overlay

**Observation.** D4 made the four lattice-state families caller-owned but did
not change total co-live storage.  The later 275,840-byte candidate object was
not semantically active while early ideal arithmetic entered ML2, whose core,
retry permutation and unpublished output occupied large nested automatic
frames.

**Hypothesis.** A typed union can reuse the candidate extent for complete ML2
retry state if and only if core/permutation/output remain mutually disjoint,
all early ML2 entrances use the same object, no pointer escapes, and the whole
union is finalized and securely cleared before candidate activation.  The
outer workspace should remain 353,008 bytes while the largest early ML2 stack
path falls by roughly the complete retry footprint.

**Implementation.** Compact commit
`2771afabf54b579b6f05d7440aa6de0a48544779` adds fixed-capacity core and retry
types, explicit workspace ML2/retry APIs and workspace variants for the three
early `find_uv` ideal/lattice paths.  The union is
`union { quat_ml2_retry_workspace_t ml2; find_uv_candidate_workspace_t candidates; }`;
the 77,168-byte lattice state remains outside.  The complete commit diff
SHA-256 is
`4db3ea31a31f1e0f019fc6e1be198cc53ca2b8ea773e493038ec1015fe52e830`
and patch SHA-256 is
`07b5380c158806d9e7cb2c2959535033df66ff590a5df811610e34c1523c5b15`.

**Correctness and audit.** Independent reviewers traced every core/retry
initialization, permutation, failure, delayed publication, finalize and clear;
verified all three early entrances and the candidate-not-yet-live boundary;
and found no blocking alias, effective-type, OOB, cleanup or namespace defect.
All-level R64, L1 RADIX32, ASan+UBSan, strict-aliasing and profile tests pass,
as do twelve byte-identical K/S/V transcripts and Arm GCC 15.2.1 compilation.
The remaining low test gap is that a real workspace ML2 input is not forced to
fail attempt one and recover on a later attempt; the common retry driver has a
fault-injected permutation/recovery fixture.

**Memory result.** Cortex-M33/RADIX32 measures 77,632 bytes for core and
94,912 bytes for complete retry storage.  The candidate member remains larger
at 275,840 bytes, so the outer object remains exactly 353,008 bytes.  On the
largest early alternate-ideal branch the frozen `-Os` path changes
203,384→107,264 bytes, saving 96,120.  Including the now-out-of-line 904-byte
insertion helper, Gram refresh and one integer multiply gives
205,032→109,816; with the workspace, 558,040→462,824, a scoped 95,216-byte
reduction.  This is not a linked peak or a full-operation lower/upper bound.

**Time/security result.** No speed result is claimed.  Fixed-size clearing is
independent of secret-derived object sizes but adds substantial memory writes;
the inherited retry count, arithmetic and search remain variable-time and can
change power traces.  Target timing, energy and leakage remain required.

**Result.** Accepted as the first genuine post-D3 lifetime overlay.  It does
not make KeyGen/Sign heap-free: the compatibility wrapper still allocates per
`find_uv`, and later fixed-degree, direct ideal-construction and MLLL paths can
still place legacy ML2 state beside a whole-operation arena.

## Cycle 30 — D6 Clapotis/fixed-degree arena propagation

**Observation.** D5 removed the large automatic ML2 state from the early
`find_uv` phase, but Clapotis immediately performed two fixed-degree ideal
constructions through the legacy retry API after `find_uv` returned.  If the
353,008-byte arena is owned by a whole Sign operation, either legacy reduction
could remain co-live with it.

**Hypothesis.** The candidate member is dead, finalized and securely cleared
when `find_uv_with_workspace` returns.  Reusing the same union's complete ML2
retry member sequentially for the `u` and `v` fixed-degree constructions must
therefore preserve C object lifetime and protocol order while removing the
legacy ML2 stack from that explicit route.  The arena ABI should not change.

**Implementation.** Compact commit
`15a69ee3a3eecc70f6f04e6e8bff134635a27696` adds workspace variants for
`quat_lideal_create_with_norm`, fixed-degree isogeny evaluation, Clapotis and
arbitrary-isogeny evaluation.  Shared implementations retain the legacy APIs;
nonnull explicit calls dominate the workspace branch.  Both post-`find_uv`
calls receive `&workspace->phase.ml2`.  The reviewed diff SHA-256 is
`62723cec277485a707824290425a98dc4db745a1190d7b2885860f9c1979e1f2`.

**Correctness and audit.** Three independent reviews found no Critical, High
or Medium source defect.  All-level RADIX64 tests, Level-I RADIX32,
ASan+UBSan, strict-aliasing, twelve byte-identical K/S/V transcripts, Arm
GCC 15.2.1 compilation, same-input legacy/explicit output comparison,
failure nonpublication, canaries and whole-workspace clearing pass.  The
explicit dead-stripped test closure has no allocator/GMP/qsort symbol.

**Memory result.** The workspace remains exactly 353,008 bytes.  In the frozen
Arm `-Os` individual-TU profile, current legacy fixed-degree ancestry is
180,992 bytes and the explicit workspace route is 85,936 bytes, a 95,056-byte
reduction.  Including the known insertion/Gram/integer-multiply descendants,
the complete explicit Clapotis diagnostic is 88,488 bytes; arena plus path is
441,496 bytes.  These figures are partial source-object diagnostics, not a
linked peak.

**Time/security result.** The current production Sign still takes the legacy
route, so its paired host timing is intentionally unchanged.  The explicit
route adds repeated fixed-size clearing writes; inherited retries and
arithmetic remain variable-time.  No speed, constant-time or energy claim is
made.

**Result.** Accepted as an API-level phase-lifetime checkpoint.  It is not yet
production-reachable from `protocols_sign`/`protocols_keygen`; shared legacy
branches and `find_uv` allocation remain.  The next measured obstruction is
the 16-generator MLLL product path, approximately 165,152 bytes under the
same `-Os` descendant-adjusted definition.

## Cycle 31 — D7 MLLL product/intersection workspace routes

**Observation.** After D6, compact ideal multiplication and intersection could
still invoke the complete 94,912-byte ML2 retry state as automatic storage
while the 353,008-byte operation arena was reserved.

**Hypothesis.** Both MLLL routines build their result in a temporary candidate
and publish only after full-rank reduction. Passing the existing ML2 union
member through those reductions should preserve candidate order and failure
semantics while removing the nested retry frame from an explicit route.

**Implementation and validation.** Compact commit
`f6f7bf559cfab95fa1223fa4f928792ff79a7b76` adds explicit workspace APIs for
compact lattice product, lattice intersection and ideal intersection. Its
reviewed parent diff is
`81956a08924d89b6cd30f907600f8eb7119b43ce818985d2466645c3c5f58469`.
All three independent reviews approved the source. RADIX64/RADIX32 at all
levels, L1 sanitizer and strict-alias builds, twelve byte-identical K/S/V
transcripts, Arm compilation, namespace inspection, alias/failure matrices,
canaries and complete clearing pass.

**Memory result.** Looking only at the ML2 branch gives about 95 KB reduction,
but that is not the fair operation maximum after the transformation. A normal
LLL descendant chain then dominates. Under frozen Arm `-Os`, the maximum known
product path changes 165,152→74,584 bytes and ideal intersection changes
162,784→72,224 bytes, reductions of 90,568 and 90,560 bytes. Adding the arena
once gives 427,592 and 425,232 bytes. These are unlinked source-object paths.

**Result.** Accepted as an API-level checkpoint. Production `sign.c` still
references only legacy MLLL entry points, shared objects still contain both
branches, and no full-Sign timing or fit claim follows. The next obstruction
is prime-norm equivalent-ideal construction through legacy ideal/element
multiplication.

## Cycle 32 — D8 prime-equivalent ideal workspace route

**Observation.** KeyGen and Sign shorten sampled ideals by multiplying through
the prime-norm equivalent-ideal search.  That path still placed a complete
94,912-byte ML2 retry state below a separately reserved 353,008-byte arena.

**Hypothesis.** The prime search, ideal multiplication and compact lattice
multiplication are sequential with respect to candidate-row storage.  Passing
the arena's retry member through them should preserve sampling/RNG order and
late output publication while removing the nested automatic retry state.

**Implementation and validation.** Compact commit
`3ea2b47417a5a6dc0b680bc60625c2761123314b` adds explicit workspace APIs for
ideal multiplication and prime-norm reduced-equivalent search.  Its reviewed
parent diff is
`6649faa1340ae425f543eabf64ce1dfd52cd1fcd4de1b6c62c83c207d8d2af00`.
Independent reviews found no source-correctness blocker.  Deterministic
legacy/explicit tests preserve the result and post-call RNG state; all-level
namespace builds, RADIX32, sanitizer, strict-alias, transcript and Arm route
checks pass within the recorded scope.

**Memory result.** The maximum after flattening is not the ML2 branch but a
denominator-reduction descendant.  Under frozen Arm `-Os`, the fair D7→D8
maximum changes 125,560→34,624 bytes (−90,936).  The Pico-like diagnostic is
125,608→34,904 bytes (−90,704).  Adding the arena and current operation prefixes
gives 422,280 bytes for Sign and 393,896 bytes for KeyGen in the `-Os` model,
but neither is a linked total.

**Result.** Accepted as an API-level checkpoint only.  Production KeyGen/Sign
still reference the legacy entry, shared objects retain both branches, and the
extra fixed-size clearing has no target timing measurement.  The next isolated
legacy entrance is random-ideal sampling into `quat_lideal_create_with_norm`.

## Cycle 33 — D9 random-ideal workspace route

**Observation.** Prime KeyGen/commitment sampling and composite/cofactor
auxiliary Sign sampling both ended by calling legacy
`quat_lideal_create_with_norm`, placing a complete retry state on the stack
despite the available phase arena.

**Hypothesis.** RNG consumption and random search finish before final ideal
construction. Passing the arena member only to that construction should leave
the sampled distribution unchanged, preserve transactional output, and remove
the nested retry frame from an explicit route.

**Implementation and validation.** Compact commit
`cb040911ef14dd56d2c647834d884919de029ace` adds a shared implementation and
an explicit caller-owned entry. Its final reviewed diff SHA-256 is
`d35d0c8176dacfd4e5252f2c1191ff4e8ae89fe60369263371593c401ede826f`.
Fixed CTR-DRBG tests exercise prime and composite/cofactor success, exact ideal
representation, post-call RNG state, invalid input, valid-input NULL workspace,
guards and clearing. Host, both radices, ASan+UBSan, strict-alias, twelve
protocol transcripts and Arm compilation pass. Three independent audits found
no source blocker.

**Memory result.** Under Arm `-Os`, the fair frozen D8→D9 maximum changes
122,312→37,744 bytes (−84,568). Under Pico-like `-O3`, it changes
122,568→38,008 bytes (−84,560). During hostile review an initial 38,224-byte
number was rejected because it double-counted a 456-byte helper inlined into
the inverse. A proposed 37,768-byte correction was also rejected because it
omitted the real 240-byte determinant child. The final gate follows the exact
inverse→determinant→multiply assembly chain and asserts absence of the inlined
helper edge. Arena-plus-path values are 390,752 and 391,016 bytes.

**Time/security result.** No timing point is accepted. A successful explicit
call with one to four retry attempts writes 534,912–1,000,704 logical
secure-clear bytes, and the path remains variable-time. Public KeyGen/Sign do
not yet select the explicit API.

**Result.** Accepted as an API-only checkpoint. Current production peak is
unchanged; top-level operation ownership, remaining direct-ideal/decode routes,
legacy branch elimination, linked SRAM and physical execution remain open.

## Cycle 34 — D10 operation-owned encoded KeyGen

**Observation.** D6–D9 supplied independently audited explicit workspace APIs,
but legacy KeyGen still selected them piecemeal through compatibility entries
and did not own the 353,008-byte arena across the complete operation.

**Hypothesis.** Random-ideal sampling, equivalent-ideal search and
IdealToIsogeny are sequential KeyGen phases. A single encoded-operation owner
can pass the same arena to D9, D8 and D6 without changing RNG consumption,
encoded keys, retry behavior or output publication.

**Implementation and validation.** Compact commit
`7b549db43145e112366fba4509a1085b3400f52a` adds
`protocols_keygen_with_workspace` and `sqisign_keypair_with_workspace`; the
legacy entry is unchanged. The final reviewed parent diff SHA-256 is
`434fc6cf09ffa1ec6edea4c17660d1b571b5bd41c53f895a8d49336c13537d8a`.
Twelve fixed seeds preserve exact PK/SK bytes and post-call RNG state. R64
all-level signatures, Level-I RADIX32, ASan+UBSan, an actually effective
strict-alias build, Arm compilation and three independent reviews pass. The
ordinary upstream `ENABLE_STRICT=ON` preset was rejected as evidence because
it appends `-fno-strict-aliasing`; the accepted gate disables that preset and
checks the emitted commands.

**Memory result.** The production type remains exactly 353,008 bytes, alignment
eight. The largest known path is the early alternate-ideal `find_uv` branch:
81,384 bytes under Arm `-Os` and 81,456 under Pico-like flags. Arena-plus-path
sums are therefore **434,392** and **434,464 bytes**, with nominal raw-SRAM
margins 98,088 and 98,016 bytes. Fixed-degree sums are 413,120/413,192 bytes;
secret-key encoding sums are 366,624/366,640 bytes. Hostile review rejected an
`-Os` encoding value that added a 24-byte `quat_alg_mul` frame: disassembly
shows that frame restored before the tail branch, so it is not co-live.

**Time/security result.** Two 30-row host smokes use 15 deterministic seeds in
both AB/BA orders. Median paired explicit/legacy ratios are 0.998012779 and
0.998830461. They do not predefine an equivalence margin and are neither a
speedup nor target-timing result. Top-level entry/exit clearing alone writes
706,016 logical bytes before nested phase clearing. The path remains
variable-time and no side-channel claim follows.

**Result.** Accepted as a top-level ownership/correctness checkpoint, not a
heap-free or target-fit result. The explicit non-NULL runtime route avoids
allocation, but the linked closure retains `malloc/free` through same-object
legacy branches. Reachable/same-object VLAs, final link/platform state and
physical KeyGen remain open.

## Cycle 35 — D11a physically specialized KeyGen closure

**Observation.** D10's non-NULL runtime route avoided allocation, but shared
translation units still retained the NULL compatibility arms, legacy
77–95-KiB automatic workspaces, `find_uv` allocation, GMP/stdio helpers and
test-only source. A value-sensitive execution proof therefore did not imply an
allocator-free or low-stack linked closure.

**Hypothesis.** A dedicated Level-I/RADIX32 build can select the already audited
non-NULL arms at compile time and use an exact source manifest without changing
the normal build, RNG order, HNF/ML2 algorithm choice or encoded outputs. This
should physically remove allocator/GMP/stdio/legacy code from the selected
KeyGen objects while leaving VLA flattening as a separate measured step.

**Implementation and validation.** Compact commit
`78db2858780850f06b965ff87653795b529d3299` implements
`SQISIGN_LOWMEM_ONLY`; its exact parent diff SHA-256 is
`fa9b1913e38381024c538c7984ed1cde2eaf2e9e620992c95e1cc0f754bad608`.
The final gate freezes complete compile command hashes, source/object/archive
membership, all symbols and relocations, writable sections, direct HNF/ML2
edges, the Arm ABI and every expected `.su` record. Fresh host production,
deterministic non-LTO, ThinLTO, effective-strict and fatal O1 ASan+UBSan builds
pass. The Arm GNU 15.2.1 deterministic closure contains exactly 45 objects in
10 archives/45 members and preserves the 353,008-byte, eight-byte-aligned
arena. Twelve normal-build RADIX32/RADIX64 KeyGen differentials preserve exact
PK/SK and post-call RNG state.

The reproducer was hardened against partial builds, missing `.su` files,
conflicting effective flags, ABI-changing extra flags, injected archive
members, hidden allocator/stdio imports and COMMON symbols. Negative fixtures
for each class fail as intended. Three independent reviews then found no
Critical, High or Medium implementation blocker.

**Memory result.** The deterministic Arm selected objects contain 56 writable
payload bytes: 52 bytes of test-only CTR-DRBG state and a four-byte
secure-clear function pointer. `CONNECTING_IDEALS` is a 27,272-byte read-only
symbol, although final Pico XIP placement remains unmeasured. Eight dynamic
frames remain. The largest currently reconstructed normal HNF route is 105,808
PSP bytes including its 13,824-byte VLA and known descendants; with the arena,
the diagnostic sum is 458,816 bytes and the nominal raw-SRAM remainder is
73,664 bytes.

**Result.** Accepted as an allocator/GMP/stdio/legacy-free *selected-closure*
checkpoint. It is not a final Pico ELF, does not include production RNG,
MSP/exception/platform state or all descendants, and is not VLA-free. No
target-fit, speed, constant-time or side-channel claim follows.

## Cycle 36 — D11b HNF workspace overlay

**Observation.** D11a's largest reconstructed path reached the normal HNF arm,
whose fixed component was 6,792 bytes plus a bounded 13,824-byte
`ibz_vec_4_t[16]` VLA. HNF and ML2 are mutually exclusive within one lattice
multiplication, while the larger candidate phase begins only after all early
reductions and a complete phase clear.

**Hypothesis.** A fixed 16-row HNF member can share offset zero with the
94,912-byte ML2 retry member without growing the 353,008-byte arena. The
preregistered local projection was a full 13,824-byte path reduction, from
458,816 to 444,992 bytes.

**Implementation and validation.** Compact commit
`99344812b28e4a57ba0c876a27ecfa7372363f9a` adds the typed HNF member and
workspace entry, preserves the normal-build compatibility VLA, and physically
excludes the latter from `SQISIGN_LOWMEM_ONLY`. Generator counts 4, 8 and 16
match the legacy matrix exactly; invalid bounds/null/modulus cases preserve
the output, guards remain intact and active scratch is cleared. Normal and
low-memory ASan+UBSan routes, both radices, all levels, twelve exact encoded
KeyGen/RNG differentials and the exact 45-object Arm closure pass. Three
independent reviews approved final diff SHA-256
`df6591f48b7cd0427608aa3135ad0b001e0147b868371ad5ba2f79b386f256bc`.

**Memory result.** The arena remains 353,008 bytes and the HNF dynamic record
is gone. The compiled lattice frame grows by eight bytes and a new 32-byte HNF
wrapper is co-live, so the known path is 92,024 rather than the preregistered
91,984 bytes. The arena-plus-path projection is therefore **445,032 bytes**:
a measured **13,784-byte** D11a reduction and 87,448 nominal raw bytes before
omitted state. Seven dynamic frames remain.

**Time/security result.** The fixed deterministic closure enters HNF six
times, adding 165,888 logical clear bytes on that fixture. Two pinned 30-row
normal/RADIX64 host smokes report complete explicit/legacy ratios of medians
0.998439515 and 0.998935711. Earlier unpinned runs changed the sign of the tiny
delta, so no slowdown, speedup, equivalence or target result is resolved. The
HNF/ML2 choice is variable time and can use secret-derived bounds.

**Result.** Accepted as the first VLA removal from the exact D11a selected
closure. It is not
a final Pico link or total peak: theta, dlog, batched inversion and MP dynamic
sites, dominant fixed frames, output/static/runtime state, production RNG and
MSP/PSP exception state remain.

## Cycle 37 — D11c theta-chain workspace overlay

**Observation.** The selected low-memory theta wrapper retained six VLAs. At
Level I their individually aligned Arm payload was 13,848 bytes. Fixed-u,
fixed-v and final randomized theta calls occur sequentially after the same
fixed-degree ML2 member, so their scratch lifetimes do not overlap.

**Hypothesis.** A bounded theta workspace can share the existing 94,912-byte
fixed-degree union without growing the 353,008-byte arena. The preregistered
local reduction was 13,848 bytes, but HNF was already the global known-path
maximum, so the expected global reduction was zero.

**Implementation and validation.** Compact commit
`1b9888a765b2674a78232595d08eadc24a5c2a94` adds a 13,844-byte typed
workspace, runtime bounds (`extra ? 1..T-2 : 4..T`, at most three points), and
explicit normal/verify/randomized entries. The normal compatibility wrapper
retains its VLA and output-publication behavior. A twelve-seed RADIX32 encoded
KeyGen/RNG differential, both-radix normal tests, all-level id2iso/signature tests, strict
aliasing, fatal low-memory and normal ASan+UBSan, exact object/edge/ABI gates
and three independent reviews pass. The accepted parent diff SHA-256 is
`b9576c12986d94552dbe5a9d7c2d8233d120c514a1ee007511612772ea4ae248`.

**Memory result.** The theta object is 13,842 field bytes plus two bytes of
internal padding, aligned to four. The fixed-degree union remains 94,912 bytes
and the arena remains 353,008 bytes. In the frozen Pico-like profile, the
fixed-u/v theta branch changes from 68,584 to 54,792 bytes, a 13,792-byte local
reduction after new helper/wrapper frames. The selected dynamic inventory
falls from seven to six. HNF remains the 92,024-byte global path, so arena plus
known path remains **445,032 bytes** and the global measured saving is zero.

**Time/security result.** Three successful theta calls add 83,064 logical
clear bytes per deterministic Clapotis invocation. Two pinned 30-row host
smokes report cumulative explicit/legacy ratios 0.998642 and 0.999015; paired
medians change sign. They do not isolate D11c, define equivalence, or predict
Cortex-M33 time. Theta scheduling and randomized normalization remain variable
time; no side-channel claim follows.

**Result.** Accepted as a theta-local selected-closure VLA removal. It is not a
global peak reduction, VLA-free closure, final Pico link, physical KeyGen or
fit result.

## Cycle 38 — D11d-1 batched-inversion workspace overlay

**Observation.** The selected KeyGen closure retained two `fp2_t[len]` VLAs
inside batched inversion. All production calls use lengths 2, 4, 5, 8 or 11,
so the maximum Level-I payload is `2 * 11 * 72 = 1,584` bytes. The calls occur
after `find_uv` has completed or inside theta scratch whose existing
94,912-byte fixed-degree union is larger.

**Hypothesis.** A runtime-bounded batched-inversion workspace can share dead
`find_uv`/theta storage without growing the 353,008-byte arena. The local VLA
payload should disappear, but HNF should remain the global known maximum; no
operation-level reduction was preregistered.

**Implementation and validation.** Compact commit
`a8d30fd64985935ed7d9b1b92fe1ae90ba4a39e3` adds a 1,584-byte typed object,
explicit field APIs and low-memory routing through lift, theta action,
Weil/Tate dlog and final basis change. Bounds, null/failure publication,
canaries and complete active-object clearing are tested. Twelve exact encoded
KeyGen/RNG transcripts, normal and low-memory ASan+UBSan, effective strict
aliasing, normal all-level tests, the exact 45-object Arm closure and three
independent reviews pass. The accepted parent diff SHA-256 is
`3536ffc3789fc8fad0925edd2673e20a58301ecd462265a13558d498ea2878a5`.

**Memory result.** The batch object is four-byte aligned with its second array
at offset 792. Theta grows from 13,844 to 15,428 bytes, still below the fixed
union maximum; the arena remains 353,008 bytes. The selected dynamic inventory
falls from six to five. The Pico-like theta path is 54,816 bytes and the HNF
path is 92,032 bytes, yielding **445,040 bytes** with the arena and 87,440
nominal raw bytes. The eight-byte D11c increase is a Clapotis code-generation
change. The separate `-Os`/soft profile is 54,464 bytes for theta and 91,448
for HNF, yielding 444,456 bytes with the arena. Thus the local VLA is removed
but the global known diagnostic is not reduced.

**Time/security result.** One fixed successful KeyGen requests 72,864
additional logical clear bytes relative to D11c. Two pinned 30-row cumulative
host smokes report explicit/legacy ratios 1.001836 and 0.998577, with paired
medians of opposite sign. They neither isolate D11d-1 nor establish
equivalence, speedup or Cortex-M33 cost. Input-dependent routes remain variable
time and no side-channel claim follows.

**Result.** Accepted as a batched-inversion-local VLA removal. It is not a
global peak reduction, VLA-free closure, final Pico link, physical KeyGen or
fit result.

## Cycle 39 — D11d-2 two-power discrete-log workspace overlay

**Observation.** The selected KeyGen closure retained one pair of
`fp2_t[ceil(log2(e))+1]` VLAs in each Tate/Weil implementation. Level I has
`e <= 248`, so the maximum log is eight and the two power tables occupy
`2 * 8 * 72 = 1,152` bytes. The final KeyGen basis change invokes Tate after
the earlier batched-inversion storage is dead.

**Hypothesis.** A runtime-bounded dlog object can share the existing 1,584-byte
field union without growing the 353,008-byte arena. The two dynamic records
should disappear and the recursive branch should shrink locally, while HNF
should remain the global known maximum.

**Implementation and validation.** Compact commit
`434e093bc5e7e4157b77176a7d762853f50f39b0` adds the 1,152-byte typed object,
explicit Tate/Weil APIs and low-memory routing through the final basis change.
Bounds, null/failure publication, canaries and active-object clearing are
tested. R64 Level I/III/V direct legacy/explicit Tate/Weil tests, the 12-seed
encoded KeyGen/RNG differential, normal and low-memory fatal ASan+UBSan,
effective strict aliasing and the exact 45-object Arm closure pass. The parent
diff SHA-256 is
`cf7d0e1ddefb462a352550cdfd9ec1fe5dd7e7b678b19b6dc08a69f9cdf8e5b3`.

**Memory result.** The dlog object is four-byte aligned with the second table
at offset 576; the batch/dlog union remains 1,584 bytes and the outer arena
remains 353,008 bytes. The selected dynamic inventory falls from five to
three. In the Pico-like profile, the recursive Tate and Weil branches each
fall by 1,136 bytes. HNF remains 92,032 bytes, so arena plus known global path
remains **445,040 bytes** with 87,440 nominal raw bytes before omitted state.

**Time/security result.** The fixed successful KeyGen performs four explicit
dlogs, adding 9,216 logical clear bytes relative to D11d-1. Two pinned 30-row
cumulative host smokes report explicit/legacy ratios 0.999256 and 0.998576.
They do not isolate D11d-2, prove equivalence or speedup, or establish target
timing. Dlog control flow remains variable time and may be secret-derived.

**Result.** Accepted as a dlog-local VLA removal. It is not a global peak
reduction, VLA-free closure, final Pico link, physical KeyGen or fit result.

## Cycle 40 — D11d-3 fixed-precision MP workspace overlay

**Observation.** The selected closure's final three dynamic records were
fixed-precision `mp_mul`, `mp_inv_2e` and 2-by-2 matrix inversion. Under the
Level-I/RADIX32 configuration the maximum precision is 18 32-bit words. Their
nested scratch requirements are 144, 504 and 936 bytes, respectively, and the
current encoded KeyGen does not execute the matrix-inversion route.

**Hypothesis.** A runtime-bounded MP hierarchy can share the existing
1,584-byte pairing union without growing the 353,008-byte arena. This should
remove the final selected dynamic `.su` rows, but it should not reduce the
HNF-dominated global known path. No timing or operation-peak improvement was
preregistered.

**Implementation and validation.** Compact commit
`f63efb4154ffacbd1e5a6cc6ab0229512bf8d2ce` adds nested typed MP
workspaces, explicit APIs, runtime bounds and staged matrix output. Direct MP
success/failure, alias/nonpublication, canary and complete-clear fixtures,
normal R64 Level I/III/V tests, R32 12-seed encoded KeyGen/post-RNG
differentials, normal and low-memory fatal ASan+UBSan, effective strict
aliasing and the exact 45-object Arm closure pass. The final selected Arm
closure also compiles with `-Wvla -Walloca -Werror`. The parent diff SHA-256
is `76ddc8df206dcecd2d7c170cf03e19d63fec8f5d535e856ad97156014fff2695`.

**Memory result.** The multiply/inversion/matrix workspaces are 144/504/936
bytes; the largest is below the 1,584-byte pairing member, so the arena remains
353,008 bytes. The selected dynamic inventory falls from three to zero. The
deepest audited MP/change-of-basis subpaths are 192/416 bytes under
`-Os`/soft and 224/448 bytes under the Pico-like profile. HNF remains
91,448/92,032 bytes, yielding unchanged arena-plus-known-path diagnostics of
444,456/445,040 bytes.

**Time/security result.** The normal benchmark binary is byte-identical to
D11d-2 because current KeyGen does not reach MP matrix inversion. The prior
two host smokes therefore remain descriptive evidence for that exact binary,
not D11d-3 timing evidence. The explicit arithmetic remains variable time and
no side-channel claim follows.

**Result.** Accepted as completion of selected-object VLA flattening. It is
not a final Pico ELF, total-SRAM fit result, production-RNG implementation or
physical KeyGen execution.

The clean root aggregate reproduction passed at project commit
`e3ea99b42b9781776307bce37c5959745665edbf`.

## Cycle 41 — linked and physical deterministic RP2350 KeyGen

**Observation.** The D11d-3 selected closure had zero dynamic stack records
and a 445,040-byte workspace-plus-known-path diagnostic, but that diagnostic
omitted linked SDK state, outputs, final static placement, MSP/PSP reservations
and physical execution.  It therefore could not establish target fit.

**Hypothesis.** A dedicated one-core Pico 2 image with the 353,008-byte owner,
a 120-KiB PSP, the full 8-KiB scratch-bank MSP and the reviewed deterministic
CTR-DRBG should fit in 520 KiB and reproduce the frozen host transcript.  The
image must be rejected if any allocator/GMP/system-RNG/legacy-large-stack
symbol, nonzero heap, undefined symbol or out-of-range allocated section
survives final link.

**Implementation and validation.** Project commit
`64bd99771d34f22f8863c8360ab6d2716a045b2d` builds an exact 45-TU
Level-I/RADIX32 library and dedicated firmware.  The ELF/map audit passes, the
workspace has two 64-byte guards, the PSP and MSP are independently patterned,
and USB output occurs outside the timed operation.  The image was flashed with
USB-enabled picotool 2.3.0 to an RP2350 A2/QFN60 Pico 2.

**Memory result.** Main-bank allocation through `.bss` end is 485,536 bytes:
353,136 bytes for the guarded owner, 122,880 bytes for PSP and 9,520 bytes for
remaining linked state.  A separate 8,192-byte MSP reservation gives
**493,728 bytes** exclusive SRAM and **38,752 bytes** unreserved.  The one run
observed 91,980 PSP bytes and a conservative upper 2,308 MSP bytes.  Those
watermarks support the reservation but are not strict maxima.

**Correctness/time result.** Encoded KeyGen returns success in
2,696.500982 seconds, preserves both guards, clears the complete workspace and
matches host digest
`1de4b175a5c3c376ec5f86593745fcdb8f697dbd40a78c63e70d3412dd177f61`.
This is a single deterministic correctness run, not a runtime distribution or
speed comparison.

**Result.** Accepted as the first linked and physically executed low-memory
KeyGen artifact.  It closes deterministic KeyGen fit for this exact firmware,
not production RNG, worst-case stack, constant time, Sign or combined K/S/V.

## Cycle 42 — D12a decoded-key Sign workspace owner

**Observation.** Physical deterministic KeyGen proved that the 353,008-byte
arena can coexist with a bounded target stack, but the Sign protocol still
selected legacy operation APIs. In addition to its two sequential
IdealToIsogeny calls, Sign reaches random/equivalent-ideal, MLLL
product/intersection and a challenge-ideal helper whose ML2 fallback was not
covered by the earlier top-level route inventory.

**Hypothesis.** One decoded-key Sign owner can reuse the existing arena across
all these sequential phases without changing candidate order, RNG consumption,
signature bytes or the arena ABI. This checkpoint must be rejected if any
reachable audited phase falls back to the legacy ML2 stack route or if the
workspace grows beyond 353,008 bytes.

**Implementation and validation.** Compact commit
`8a0534d0fc4f2d8f0f355774d111e26b3ca19035` adds
`protocols_sign_workspace_t`, a decoded-key
`protocols_sign_with_workspace` entry and a workspace-aware challenge-ideal
helper. The same union is passed through D6–D9/D11 routes and cleared on every
return. The parent is `f63efb4…`, tree `7f07ffb…`, and exact diff SHA-256 is
`a631c53f062c9803e11ea2709b3924d59efff808e15ae9c8d51c80bc04251316`.
R64 and R32 twelve-seed signature/post-RNG differentials pass, as do normal
Level I/III/V KeyGen→Sign→Verify, fatal ASan+UBSan, effective strict
aliasing, frozen transcripts, Arm GNU 15.2.1 compilation and low-memory symbol
checks.

**Memory result.** The Level-I arena remains 353,008 bytes aligned to eight.
The Pico-like maximum-known individual-object path through early `find_uv` HNF
and its audited descendants is 114,840 bytes, giving a diagnostic sum of
467,848 bytes and a nominal raw difference of 64,632 bytes. The changed Arm
objects add 816 bytes of text-like payload and no measured mutable payload.
These are individual-object diagnostics; encoded decoding, final sections,
MSP/PSP/exception state and runtime/library descendants are absent.

**Time/security result.** Two frozen 30-row host runs use 15 seeds in AB/BA
order. Ratios of medians are 1.000626 and 0.997403, with no reproducible
slowdown direction. They measure the cumulative explicit route, not D12a-only
clearing or Cortex-M33 performance. Sign remains variable time and no
constant-time, equivalence, power-resistance or target-speed claim follows.

**Result.** Accepted as decoded-key Sign ownership. It is not an encoded
public API, selected low-memory Sign closure, linked fit or physical Sign
result.

## Cycle 43 — D12b encoded Sign closure

**Observation.** D12a owned the decoded protocol but left the public encoded
entry, secret-key decoder and even-isogeny strategy outside the selected
low-memory closure.

**Implementation and validation.** Compact commit
`383a1f09cc902d2e147266caf50fcc02fc316261` propagates one 353,008-byte
owner through encoded Sign, gives the decoder explicit scratch and moves the
remaining bounded even-isogeny storage into the same sequential arena. The
Level-I/RADIX32 Arm closure contains 51 selected objects, no dynamic `.su`
record and no allocator, GMP, system-RNG or curated legacy-large-stack symbol.
Exact signatures, signed-message bytes and post-RNG state remain equal to the
legacy oracle under the frozen host corpus.

**Result.** Accepted at source, selected-closure, linked-image and physical
deterministic scope. Clean firmware `0cbd185…` reports `status=PASS`: KeyGen
takes 2,696.208062 seconds and Sign takes 7,337.883516 seconds, both transcript
digests match the host oracle, both owners clear, PSP observations are
91,980/120,452 of 131,072 bytes and the conservative MSP observation is
2,348/8,192 bytes. The result is one correctness execution, not a production
RNG, worst-case stack proof, target timing distribution or side-channel result.

## Cycle 44 — D12c bounded Verify/Open and operation union

**Observation.** Verify did not require the 353,008-byte Sign/KeyGen arena, but
a combined image still needed bounded ownership, failure nonpublication and a
single sequential top-level lifetime.

**Implementation and validation.** Compact commit
`6b79cfb5cfe1c756d7061b92038d5069bda66f72` adds 15,428-byte detached
Verify/Open scratch and overlays it at offset zero with KeyGen and Sign. The
top-level operation union remains 353,008 bytes aligned to eight. The selected
Arm closure contains 52 objects and zero dynamic stack records. Host fixtures
cover valid Verify/Open, altered signatures, Open failure zeroing, unchanged
RNG and complete member/owner cleanup.

**Result.** Accepted at source, selected-closure, static linked-firmware and
physical deterministic-execution scope. The integrated RP2350 ELF has a
zero-byte heap, excludes allocator/GMP, system-RNG and legacy-large-stack
routes, and leaves 30,144 bytes unassigned. One boot completes encoded KeyGen,
encoded Sign and detached Verify with exact K/S host digests, Verify RNG
nonconsumption, full cleanup and `status=PASS`. K/S/V take 2,696.250983,
7,337.481041 and 0.813858 seconds; their PSP observations are 91,980, 120,452
and 20,768 of 131,072 bytes, with a conservative MSP upper observation of
2,396/8,192 bytes.

## Final release result

The separate physical KeyGen, D12b KeyGen→Sign and Verify artifacts are
preserved alongside the final D12c combined capture. The D12c artifact quartet,
raw and normalized capture hashes, PSP/MSP observations, cleanup flags and
host-oracle digests are frozen and mechanically checked. At clean root
`d976d405…`, both D12b and D12c aggregate wrappers complete their normal,
ThinLTO, effective-strict, fatal-sanitizer and Arm GNU 15.2.1 pipelines and
report terminal PASS. The deterministic result therefore closes the
on-chip-SRAM feasibility question. Production entropy, multi-run performance,
constant-time behavior, power resistance and minimum-memory optimality remain
separate research questions rather than inferred claims.

## Cycle 45 — host fixed-vs-random Sign timing-leakage screen

**Observation.** The final D12c memory and physical artifacts deliberately
withheld side-channel claims. Source review already identified many
secret/input-dependent loops, but it did not establish whether complete Sign
timing carried a stable measurable signal, and ordinary performance benchmarks
did not include a negative control.

**Hypothesis.** With one fixed key/message, repeating one deterministic
signing-RNG stream should form a low-variance class, while unique deterministic
streams exercise different variable-time paths. A paired AB/BA design and a
fixed-vs-fixed control should distinguish that effect from execution order and
short-term host noise. Reversing whole-dataset order in a second run should
test drift sensitivity.

**Implementation and validation.** A host-only D12c Level-I/RADIX64 diagnostic
binary times only `sqisign_sign_with_workspace` using `CLOCK_MONOTONIC` and
records per-invocation ML2 counters. Verification follows both timed calls in
each pair. Each of two runs contains 64 A and 64 B traces in the control plus
64 A and 64 B traces in the primary dataset (512 timed signatures total), with
three warmups per dataset. Every result verifies, fixed-stream signatures are
stable, random-stream signatures/path counters reproduce by seed, both dataset
orders are exercised, and the archived reports regenerate byte-for-byte from
the raw CSVs.

**Result.** Both negative controls remain below `|t|=4.5`. Primary
first-order t is 3.590/3.582, while centered-square second-order t is
**-5.393/-5.386**, independently crossing the screening threshold. Random
class standard deviation is 161.59x/121.37x the fixed class. Across the two
runs, the same 64 random streams have Pearson 0.999958 and Spearman 0.999634
timing correlation with a 1.877-ms median absolute difference. ML2 attempt
count itself has near-zero timing correlation, so this experiment detects but
does not localize the variable-time behavior.

**Decision.** Accept as a positive host timing-leakage screen and as the basis
for physical trace acquisition. Do not infer key recovery, Cortex-M33
power/EM leakage, a constant-time verdict or production security. The next
gate is an uninstrumented RP2350 fixed-vs-fixed/fixed-vs-random power or EM
experiment with external triggering, stable negative controls and two
independent acquisitions.

## Cycle 46 — RP2350 GPIO-triggered powmod timing localization

**Observation.** Complete physical Sign takes about 7,337 seconds, making a
naive thousands-trace campaign impractical. The SPA literature identifies the
Cornacchia modular-square-root exponentiation surface, and the frozen D12c
`ibz_pow_mod` source visibly branches on every exponent bit.

**Experiment.** A dedicated Pico 2 image links the exact D12c
Level-I/RADIX32 primitive under the release compiler profile. It fixes a
521-bit prime and base, accepts identical-control, fixed, deterministic-random
and low/high-weight exponent commands, disables interrupts and holds GPIO 2
high around exactly one exponentiation. A Python oracle self-test precedes USB
command acceptance. The linked image has a zero-byte heap and no allocator,
GMP or project-RNG symbol; an objdump gate verifies the complete trigger
ordering.

Two 32-pair acquisitions interleave AB/BA control and FR/RF primary commands;
the second reverses dataset and pair order. All fixed outputs match the oracle
and every output is in range. Controls remain quiet (`t1/t2=-0.518/-0.144` and
`-0.603/-1.405`). Fixed/random group means are close (`t1=0.661` twice)
because fixed weight 261 is near the random mean, but the 32 random exponents
span weights 235–282 and show Pearson **0.999860** between weight and elapsed
time in both runs. Regressions give **4,004.02/4,004.06 µs/set bit** with R²
**0.999720**. Same-input times correlate across runs at **0.999999999**, with
2-µs median and 5-µs maximum absolute difference. The independent
two-bit/521-bit endpoints give about 4,011 µs per bit and corroborate the
regression.

**Decision.** Accept as a strong RP2350 timing-localization result for the
current variable-time exponentiation. It does not establish analog leakage,
single-trace bit recovery, secret-key recovery or dominance within full Sign.
The next gate is external shunt/EM acquisition on GPIO 2, initially with
segmented or reduced-width calibration because the full window is 2–4 seconds,
followed by coarse full-Sign phase capture and an independent repeated run.

## Cycle 47 — certified norm sketches with exact replay

**Observation.** After D12c, the 353,008-byte operation arena was dominated by
two rows of 624 arbitrary-precision `ibz_t` candidate norms. Those rows cost
269,568 bytes, even though every norm is a deterministic function of an
already-retained four-byte vector, a Gram matrix and an adjusted divisor. A
fixed-width norm representation would save memory but would first require a
global mathematical bound that this project has not proved.

**Hypothesis.** Retain an order-preserving sketch consisting of the exact bit
length and leading 64 bits. Unequal sketches certify the exact integer order;
on every sketch tie, regenerate both complete norms and compare them exactly.
Regenerate exact search norms at their original use point. This can eliminate
the resident table without truncation, collision probability or a new bound on
the integer size.

**Prototype and current evidence.** The isolated `work/compact-d13` prototype
reduces the Cortex-M33/RADIX32 candidate workspace from 275,840 to 14,024
bytes. The ML2 retry member then dominates the phase union at 94,912 bytes,
so the complete operation arena falls from 353,008 to **172,080 bytes**: a
180,928-byte or **51.253229%** reduction. A deliberate `2^100` versus
`2^100+1` sketch collision exercises the exact fallback. RADIX64 and RADIX32
id2iso/KeyGen tests, combined ASan+UBSan tests, the Arm 19-TU compile gate and
all 12 frozen transcripts pass. The first measured corpus uses exact fallback
for about 0.28% of sort comparisons; search replay remains input dependent.
Two reversed-sequence 30-row host runs show D13/D12c ratios of medians
1.003695/1.006464 for KeyGen and 0.999481/0.998980 for Sign. They isolate a
small sub-percent KeyGen cost and no Sign slowdown, but are neither an
equivalence test nor Cortex-M33 evidence.

**Decision.** Continue as a new memory-representation experiment, not yet as a
frozen replacement for D12c. The host paired smoke and local Arm frame audit
are complete; acceptance still requires full linked path accounting, RP2350
integration/target timing and a renewed fixed-vs-random timing/power screen.
Fixed memory addresses do not make this constant-time: exact-fallback
frequency, search progress and first success can change execution traces.

## Cycle 48 — D13 leakage re-screen and sketch attribution

**Observation.** D13 changes sorting and search memory traffic, so D12c's
positive timing screen could not simply be inherited.  Conversely, a repeated
whole-Sign alert alone would not establish whether the new sketch replay was
responsible for the signal.

**Method.** Repeat the 64-pair fixed-vs-fixed control and fixed-vs-random Sign
screen twice with reversed dataset order using the final D13 binary.  Then
build a separate profile binary that records sort-comparison, exact-tie and
search-replay counts.  Discard its instrumented timings and join only the
counts to the normal timing rows by deterministic seed.

**Result.** Both controls stay below `|t|=4.5`; the two primary runs report
first/second-order statistics `3.570/-5.410` and `3.576/-5.374`.  Same-seed
times reproduce with Pearson `0.999929` and Spearman `0.999771`, and all path
counters and signature digests agree.  The attribution profile covers 128
Sign invocations, 2,062,361 sort comparisons and 5,041 exact ties (`0.2444%`).
Comparison, tie and search-evaluation counts correlate only weakly with normal
elapsed time (absolute Pearson at most about `0.217`).

**Decision.** Record D13 as exact and substantially smaller, but explicitly
variable-time.  The renewed screen is a positive host timing finding; it does
not identify the sketch as the dominant cause, establish analog power/EM
leakage or support key recovery.  Existing signer paths, including the
physically confirmed RP2350 exponent-bit branch, remain security work.  Keep
the side-channel evidence independent from the SRAM acceptance result.

## Cycle 49 — D13 linked RP2350 integration

**Question.** Does the 180,928-byte ABI reduction survive the complete Pico
SDK link, or is it consumed by alignment, section placement or a larger stack
reservation?

**Method.** Integrate the frozen Compact D13 tree into the deterministic
Level-I/RADIX32 one-boot KeyGen/Sign/Verify firmware. Rebuild from clean root
`dc3289a…` with Arm GNU 15.2.1 and Pico SDK 2.3.0, require all 52 Compact
stack-usage records to be static, and rerun the ELF policy against allocator,
GMP, system-RNG, legacy-large-stack, heap and address-space violations. Repeat
the build inside the complete D13 aggregate source gate.

**Linked result.** The guarded owner is 172,208 bytes and `.bss` ends at
`0x2004c780`, so the linked main-bank reservation is 313,216 bytes. Adding the
unchanged 8,192-byte MSP reservation gives 321,408 bytes of exclusive SRAM and
leaves 211,072 of 532,480 bytes unassigned. Relative to D12c, `.bss`, the
exclusive reservation and the remaining-SRAM complement all change by exactly
180,928 bytes. `.text` and the flash load image grow by 880 bytes; the heap is
zero. The clean aggregate gate independently rebuilds and re-audits the same
layout.

**Physical result.** The exact linked image completes one deterministic
KeyGen→Sign→Verify boot with `status=PASS`. K/S/V take `2,698.150029`,
`7,611.258527` and `0.814341` seconds. The K/S digests equal the host oracle,
Verify consumes no RNG, all required clear checks pass, and observed PSP/MSP
extents remain inside their reservations. The single-run D13/D12c ratios are
`1.000704`, `1.037312` and `1.000593`; the Sign difference is reported as an
observation, not a general slowdown estimate, because there is no repeated
target distribution and the host repeats did not show the same direction.

**Decision and boundary.** Freeze D13 as the smaller exact physical
checkpoint. It closes deterministic source, link and one-boot execution
evidence, not worst-case stack, production entropy, timing equivalence or
side-channel resistance. The existing positive leakage findings are not
weakened by the additional SRAM headroom.

## Cycle 50 — test mini-GMP instead of assuming it cannot fit

**Question.** Is the fixed 27-limb `ibz_t` choice actually necessary, or was
mini-GMP rejected without a fair comparison?

**Method.** Build official v2 `dd133d7…` twice with only
`GMP_LIBRARY=SYSTEM/MINI` changed. Route both allocators through requested-byte
tracking, and additionally route mini-GMP through a clear-on-release static
first-fit allocator. Run all 100 Level-I KAT vectors, bracket the pool in
16-byte steps, record live payload/block/call statistics, and run a separate
five-primitive same-vector comparison against D13 fixed arithmetic. Compile
the complete integer backend sources under two Arm GNU 15.2.1 profiles.

**Result.** All three KAT routes pass. Native mini-GMP KeyGen/Sign medians are
1.3621/1.4497 times system GMP on the frozen host. mini-GMP Sign peaks at only
67,168 requested bytes but 4,830 live blocks; this generic allocator passes at
317,696 bytes and fails at 317,680. Pool KeyGen/Sign are 4.9441/4.2316 times
tracked mini-GMP because first-fit traversal and erasure dominate. Primitive
results are mixed: fixed wins multiply/division/square-root while mini-GMP
wins GCD/inversion. The complete mini-GMP object payload is 2.33–2.61 times
the fixed backend before final section GC.

**Decision.** Reject *stock* mini-GMP for the frozen RP2350 profile because it
uses a general heap, lacks a compile-time whole-operation bound and guaranteed
secret erasure, and adds allocation count/size/address traces. Do not reject
mini-GMP as mathematically wrong or uniformly slow. The pool experiment makes
a compact 32-bit size-class/region allocator a legitimate future branch, but
317,696 bytes is only a host corpus bound and neither backend is constant-time.

## Cycle 51 — require SCA gates and regularize the exponent branch

**Question.** Can the strongest measured RP2350 powmod channel be reduced
without surrendering the D13 memory result, and does doing so make the
primitive constant time?

**Method.** Freeze a threat/acceptance matrix that keeps deployment rejected
while known leakage remains. Implement a diagnostic-only 521-bit
square-and-multiply-always candidate: 521 fixed MSB-first iterations, two
modular multiplications each, and full-limb mask selection. Compare it against
the byte-identical D12c/D13 `intbig.c` primitive on 68 host boundary/random
cases and under ASan+UBSan. Build the candidate with Arm GNU 15.2.1, audit the
linked loop and IRQ/GPIO window, then repeat the old 32-exponent target schedule
twice with reversed order.

**Result.** All host results match and every profiled candidate call performs
521 iterations and 1,042 modular multiplications. Arm frames are static
1,128/232/32 bytes for candidate/helper/window. The old random exponent calls
perform 755--802 modular multiplications (median 779.5). Across the two target
runs, weight/time Pearson falls from 0.999860 to -0.13110/-0.13136, R² from
0.999720 to 0.01719/0.01726 and slope from +4,004.02/+4,004.06 to
-18.207/-18.229 µs per bit. The absolute slope reduction is at least 219.66x;
the low/high diagnostic improves at least 333.82x. Median random-exponent time
increases by 1.33872x.

**Residual finding.** Candidate random-input SD is still about 1.432 ms versus
1.5--2.1 µs fixed-input SD, at least a 680x ratio. Same-input timings reproduce
across reversed runs with Pearson 0.9999987 and 2/5-µs median/maximum absolute
difference. Thus the explicit extra-multiplication Hamming-weight channel is
substantially reduced, but deterministic value-dependent timing remains.

**Decision.** Accept as the first experimental mitigation checkpoint, not as
a production change or a constant-time result. Preserve the D13 arena; next
regularize modular multiplication/reduction/division, then acquire analog
power/EM traces and attempt the published exponent-recovery workflow. Only
after integration into full Sign and repetition of correctness, timing,
analog and attack-driven gates may the fail-closed deployment status change.

## Cycle 52 — fixed-work multiplication and fail-closed residual test

**Question.** Does fixing every multiplication/addition round remove the
remaining target-timing evidence at acceptable component cost?

**Method.** Implement a diagnostic-only 17-word arithmetic domain with 521
fixed exponent rounds. Each of 1,042 modular multiplications executes all 521
multiplier-bit rounds and two complete modular additions per round. Differential
test the implementation against the unchanged oracle, run fatal ASan/UBSan,
audit the Arm object/ELF schedule, and capture two 32-pair RP2350 campaigns
with both dataset and within-pair order reversed. Compare legacy,
square-and-multiply-always and fixed-work candidates with identical exponent
and result digests.

**Result.** Oracle and sanitizer tests pass; every call records 542,882
multiplier-bit rounds and 1,085,764 modular additions. Arm frames are static
88/56/896/1,000/32 bytes for add, multiply core, public multiply wrapper,
powmod and GPIO window. The absolute legacy weight slope falls by at least
41,843.94x. Median time is 1.02426x legacy and 0.76510x the first candidate;
linked diagnostic text is +400/+512 bytes respectively.

**Residual finding.** Fixed-minus-random paired means are `-5.1875` and
`-4.96875 µs`; pooled median is `-5 µs`, paired `t=-7.7963`, with 52/64 pairs
negative. The sign survives reversed scheduling. Control execution-position
and second-order warnings remain, while per-random-input cross-run correlation
is only 0.24884. The defensible conclusion is a reproducible class offset in a
noisy coarse-timer experiment, not a localized cause or recovered exponent.

**Decision.** Accept the fixed-work implementation as a stronger experimental
Pareto checkpoint but fail the leakage gate. Do not claim constant time,
analog-SPA resistance or protection of SQIsign Sign. Next integrate it into the
published Cornacchia attack surface, acquire power/EM traces, attempt exponent
recovery, and audit/mitigate all remaining variable-time Sign paths.

## Cycle 53 — map the published SPA attack into the executed signer

**Question.** Does the D13 Sign implementation actually execute the
Cornacchia/modular-square-root exponent relations used by ePrint 2025/830, and
is the 521-bit countermeasure domain large enough for every Level-I production
instance rather than only the measured corpus?

**Method.** Add one optional host-only record after
`ibz_cornacchia_prime` calls its modular square root.  Build a clean D13
control and a one-file instrumented variant, run the same fixture key/message
under twelve deterministic signing streams, verify every signature, and
require byte-identical outputs.  Freeze every modulus class, two-adicity,
exponent length/weight and call count.  Separately compile the authoritative
Level-I constants and derive symbolic bounds for both production
`quat_represent_integer` call sites.

**Result.** The 12 Sign invocations execute 361 Cornacchia square roots, 13--72
per invocation.  Moduli are 360--378 bits; 171 are `1 mod 8` and 190 are
`5 mod 8`.  For the latter, the current Tonelli--Shanks exponents satisfy
`q=(m-1)/4` and `(q+1)/2=(m+3)/8`, directly matching the published linear
relation.  Two-adicity ranges from 2 to 12.  Control and instrumented signature
digests are byte-identical and all signatures verify.

The measured maximum is not promoted to a bound.  Fixed-degree
IdealToIsogeny gives a strict `2^492` upper bound for the adjusted Cornacchia
target; the Sign random-aux route gives `2^380`.  Hence every Level-I target,
Tonelli--Shanks exponent and reduced base fits the 521-bit domain with 29 bits
of margin.  The derivation is source/hash/compiled-parameter gated.

**Decision.** Complete the attack-locus and width precondition gate.  Do not
claim the attack blocked: no protected arithmetic is integrated, calls and
Tonelli--Shanks loops remain variable, and no analog exponent recovery has
been attempted.  The next checkpoint is a guarded no-fallback Cornacchia sqrt
API with exact signature/RNG/status differential tests, followed by target
traces and the published recovery experiment.

## Cycle 54 — integrate fixed-work arithmetic at the published attack locus

**Question.** Can the Level-I Sign Cornacchia route remove the published
exponent-bit control branch without changing signatures, RNG consumption or
failure behavior, and what cost/residual leakage remains?

**Method.** Add a build-time Level-I-only protected square-root adapter with a
521-bit runtime guard and no legacy fallback. Route every exponentiation and
Tonelli--Shanks modular multiplication through a caller-owned fixed-work
workspace. Differential-test ten prime families, aliases, no-root and fatal
domain cases; verify guards, nonpublication and full clear. Run twelve frozen
Sign streams against clean D13, compare complete signed messages and 64 bytes
of post-Sign RNG, record exact work counters, run fatal ASan+UBSan, and inspect
the three Arm objects for loop bounds, call edges, branches and static frames.

**Result.** Protected commit `c2a80712…` passes every direct and sanitizer
test. The twelve Sign calls execute 361 successful protected square roots and
produce byte-identical Sign/RNG output (SHA-256 `e6827174…`). Arm GNU 15.2.1
finds a single protected Cornacchia adapter edge, no legacy sqrt/pow edge, 521
exponent and multiplier rounds, two multiplications per exponent round and two
full additions per multiplier round. The corpus performs 2,180 pow calls,
2,273,360 modular multiplications and 2,368,841,120 full modular additions.
One clean aggregate host run is 7.94 versus 75.01 seconds (`9.447x`).

**Residual finding.** The fixed schedule closes only the attacked exponent-bit
operation-count mechanism. Tonelli--Shanks iteration counts, non-residue
search, factorization, Cornacchia Euclid/half-GCD, retries, value-dependent
power/EM and the remainder of Sign remain. No analog trace or exponent
recovery was attempted, and the host ratio is not a target benchmark.

**Decision.** Accept as a narrow opt-in countermeasure checkpoint, not as a
side-channel-resistant signer. Keep deployment `known_leakage`. Preregister
two independent analog campaigns with fixed-vs-fixed controls, attack-driven
single/few-trace recovery and fixed-vs-random first-/second-order testing.

## Cycle 55 — freeze and execute the RP2350 Cornacchia calibration image

**Question.** Can the integrated protected square root be isolated in a
publicly triggered RP2350 measurement window, and does the board smoke reveal
any residual control-flow variation before power/EM acquisition?

**Method.** Link the protected Level-I/RADIX32 sources into a dedicated
RP2350 Arm-S Release image with 16 public 368-bit prime/residue fixtures. Mask
interrupts and raise GPIO 2 only around one protected square root. Audit the
final ELF for one protected call, the five linked fixed-pow and five O3-folded
fixed-multiply sites, an exact 4,344-byte clear and no legacy fallback. Flash
the byte-frozen UF2 and issue A, B, L, H and R serial commands, checking the
root, PSP status and full workspace clear after each trigger window.

**Result.** The build and linked-ELF gates pass; the UF2 SHA-256 is
`b19e9858…`. All five board executions pass correctness and clear checks. The
same-input A/B pair takes 32.024229/32.024063 seconds, while the other public
fixtures take 16.004361, 64.047796 and 70.418552 seconds. Thus the protected
exponent schedule is structurally present, but residual Tonelli--Shanks and
non-residue paths remain strongly variable at coarse timer resolution.
Reconstructing the exact algorithm gives 5,210--22,924 fixed-base modular
multiplications and Pearson `0.999999999981` against all five target times,
pinning the variation to non-residue-search and Tonelli--Shanks call counts.

**Decision.** Freeze the calibration artifacts and treat the timing spread as
a negative security finding. Do not mark ANALOG-0 complete: no shunt-current
or EM samples were acquired. Use the GPIO image for segmented power/EM capture
and attack-driven exponent recovery next; retain `known_leakage` and all
constant-time/side-channel claims as false.

## Cycle 56 — isolate the published exponentiation for analog acquisition

**Question.** Can the exact exponentiation targeted by ePrint 2025/830 be
measured without the roughly 768-ms Tonelli--Shanks envelope, while preserving
firmware provenance and attack-scoring integrity?

**Method.** Build a dedicated RP2350 image from the current protected
Montgomery source.  For each of 16 public 368-bit prime/residue fixtures,
derive `(p-1)/2` and initialize the Montgomery context before the trigger.
Mask interrupts and place GPIO 2 around exactly one 521-round protected pow
call; validate and securely clear the complete 1,248-byte acquisition object
after the trigger.  Audit the final ELF for trigger ordering, exact call count,
legacy fallback absence and static frames.  Derive every public exponent
independently from the archived header and require its weight and 17-word FNV
digest to match both serial corpora.  Make the exponent-recovery scorer reject
classifier-supplied truth that differs from this manifest.

**Result.** The exact UF2 SHA-256 is `1f8b6ddc…`; linked text/data/BSS are
46,760/0/20,968 bytes.  ELF policy and the dedicated disassembly gate pass.
All 16 fixtures pass once, and a ten-cycle alternating-order run passes
160/160 result, PSP and clear checks.  One pow takes 62,042--62,053 us.  The
cycle-centered time/weight Pearson coefficient is `0.086615`; minimum- versus
maximum-weight means differ by `-0.4 us` with Welch `t=-0.578691`.  The frozen
ground-truth checker independently reproduces all 16 367-bit exponents and
serial digests.

**Decision.** Use this approximately 12.4x shorter window as the primary
ANALOG-0/1/2 target, and retain the full square-root image as an integration
follow-up.  The target timer has one-microsecond resolution and no current/EM
samples were acquired.  Therefore do not infer equivalence, constant power,
published-attack resistance or whole-Sign safety.  The next required evidence
is two independent analog campaigns with fixed-vs-fixed controls, fixed-
fixture/varying-fixture TVLA screens and preregistered one-/ten-/hundred-trace
exponent recovery.

## Cycle 57 — reject “fixed schedule equals power resistance” structurally

**Question.** After removing the published exponent-bit operation-count
branch, which secret-dependent timing and switching surfaces remain, and can
the project prevent them from being silently relabeled as mitigated?

**Method.** Freeze the exact one-pow source, current Cornacchia/RepresentInteger
integration and three primary side-channel papers in a machine-readable
inventory.  Inspect both source and the Arm GNU 15.2.1 linked pow body.  Model
only the logical Hamming weight of the 32-bit selector word, clearly separated
from any physical leakage model.  Bind each open surface to required evidence
and a claim that remains withheld, then execute that audit from the aggregate
SCA gate.

**Result.** Twelve surfaces remain open.  In source, `select_words` computes
an all-zero/all-one word directly from each exponent bit.  In the frozen
Cortex-M33 ELF, GCC keeps the bit live and selects with arithmetic on `bit`
and `bit-1`; prefix-dependent Montgomery states remain.  Under a perfect
source-level selector-Hamming-weight observation, all 521 scheduled bits are
recovered exactly for all 16 public fixtures.  This is a structural leakage
hypothesis, not a power/EM trace.  The same inventory records masked
Tonelli--Shanks state, result/status branches, Cornacchia Euclid/half-GCD,
RepresentInteger searches, retries and whole-Sign composition.

**Decision.** Keep `side_channel_resistant=false` and
`published_key_recovery_attack_blocked=false`.  Prioritize the frozen one-pow
physical SPA/EM experiment, then compare the published constant-time integer
implementation and prototype the published constant-time half-GCD or
randomized lattice alternative.  Do not introduce ad hoc exponent blinding
without a fixed-width, freshness, failure and correctness argument covering
all production exponent families.

## Cycle 58 — turn the side-channel literature into an implementation order

**Question.** Which published SQIsign countermeasure should be implemented
first, and can component results be prevented from silently becoming a
whole-Sign or power-resistance claim?

**Method.** Review the primary SPA/timing papers and three recent constant-time
component papers.  Pin the available quaternion and dimension-4 lattice source
repositories by commit and license, inspect their lower arithmetic and target
assumptions, and map eight candidate families onto the twelve frozen residual
surfaces.  Require every mapping to retain `closes_surface=false`, then encode
the dependency order and acceptance gates in a checker consumed by the
aggregate SCA policy.

**Result.** No reviewed artifact is a complete constant-time signer.  The
quaternion reference C-halfgcd still calls GMP-backed compare, bit-length,
shift and multiply operations; importing only its fixed outer loop would not
close the lower leakage.  The dimension-4 lattice code assumes 64-bit little-
endian arithmetic, GMP support and GPL-2.0 reuse conditions.  The integer
arithmetic paper covers the necessary substrate, but this review did not
locate a public source repository.  Every open surface is now mapped to at
least one candidate without being marked closed.

**Decision.** Select `WP-CTINT-FOUNDATION` first: fixed-width compare/cmov,
sign, bit-length, shifts, arithmetic, division, reduction, exponentiation and
square root for Level-I/RADIX32.  Only after it passes differential, bounds,
secret-taint branch/address, Cortex-M33 disassembly and RAM/stack/flash/cycle
gates should C-halfgcd or randomized Cornacchia be compared.  Quaternion and
lattice work follow; whole-Sign timing and physical power/EM/key-recovery
experiments remain final independent gates.  Keep production, constant-time,
masking, analog-resistance and published-attack-closure decisions false.

## Cycle 59 — implement and freeze the first fixed-width integer slice

**Question.** Can the lowest-level compare/select/shift substrate be made
auditable on Cortex-M33 before modifying production Cornacchia, without
mistaking component structure for physical resistance?

**Method.** Implement a 1,728-bit signed representation as 54 little-endian
32-bit words, isomorphic to the current Level-I 27-by-64-bit `ibz_t`.  Use
fixed limb scans for copy/cmov, zero/sign, signed and unsigned comparison,
absolute value and bit length.  Implement shifts as eleven fixed conditional
stages, reject invalid counts and lossy left shifts without publishing output,
and clear a caller-owned 648-byte workspace on every return.  Compare against
independent branchy references and the existing `ibz_t`, then inspect exact
Arm GNU 15.2.1 output under `-Os/soft` and Pico-like `-O3/softfp` profiles.

**Result.** Strict host and fatal ASan/UBSan runs pass 10,000 scalar cases,
17,280 valid boundary shifts and 2,000 random shifts (digest
`a73f651a7fd6450b`).  A separate 10,000-pair/10,000-shift `ibz_t`
differential passes (digest `6d8f9d204b9c36f8`).  The Arm component is 882
or 1,412 text bytes, uses a 648-byte caller workspace, has only static frames
up to 40 bytes, and the frozen disassembly has fixed scalar loop back-edges
and no shift-derived memory address.

**Decision.** Mark only the first slice implemented and unintegrated.  Do not
claim formal constant-time behavior, constant power, masking, whole-Sign
resistance, published-attack closure, or production readiness.  The next
dependency is fixed-width add/subtract, multiply/square, and a reduced-width
division kernel; only after those pass the same gates may production
Cornacchia be changed.

## Cycle 60 — extend CTINT1728 through fixed schoolbook multiplication

**Question.** Can signed add/subtract/multiply/square preserve fail-closed
overflow and fixed Cortex-M33 control/address schedules without placing a
1,728-bit temporary on the stack?

**Method.** Add caller-owned 432-byte add/subtract and 1,080-byte multiply
workspaces.  Compute every carry, borrow and all 54-by-54 schoolbook products
on a fixed schedule.  Form a complete 3,456-bit unsigned product, check the
signed 1,728-bit range without early exit, publish with cmov only when valid,
and clear the complete workspace on every return.  Use a separate 27-by-64-bit
`__uint128_t` reference, boundary and alias cases, full-width overflow cases,
bounded successful products, current-`ibz_t` differential tests, sanitizers,
and exact Arm disassembly.

**Result.** Ten thousand random add/subtract cases, 2,000 full-width products,
2,000 bounded products, and 10,000 bounded products through the independent
`ibz_t` bridge pass.  Component and bridge digests are
`b5b4cc81c6741892` and `22af5b578f51a967`.  Arm text is 1,500 bytes under
`-Os/soft` and 2,088 bytes under Pico-like `-O3/softfp`; workspaces are
432/648/1,080 bytes and the largest own frame is 64 bytes.  The exact objects
contain only six fixed backward branches in multiplication and no
operand-derived address or indirect/table branch.

**Decision.** Accept this as an unintegrated structural component, not as a
whole-Sign or physical result.  Fixed-schedule signed truncating division is now the
next blocking integer primitive; reduction, exponentiation, square root,
integration, target cycles, analog traces and attack recovery remain open.

## Cycle 61 — add fixed-schedule signed truncating division

**Question.** Can the Level-I fixed-width substrate reproduce current `ibz_div`
semantics without secret-dependent loop counts, secret-derived addresses, or
failure publication?

**Method.** Add a caller-owned 1,512-byte workspace containing seven 216-byte
values.  Convert both operands to magnitudes, execute all 1,728 restoring
binary-division rounds, select each subtract result with `cmov`, and restore
the quotient and remainder signs on a fixed 54-word schedule.  The dividend
and quotient word/bit address is derived only from the public round index.
Division by zero and `INT_MIN / -1` still execute the full schedule, return
failure, preserve both initialized outputs byte-for-byte, and clear the whole
workspace.  Quotient and remainder must be distinct; either may alias an
input.

**Result.** Strict and fatal ASan/UBSan component tests pass 100 boundary pairs
and 320 random divisions in addition to the earlier arithmetic corpus (digest
`2ebda027e0eaff3b`).  An independent 27-by-64-bit restoring reference and the
current Level-I `ibz_div` agree on 512 further full-width, bounded and small-
divisor cases (bridge digest `585067bb760c2a9f`).  Arm GNU 15.2.1 emits 1,898
bytes of component text with an 80-byte maximum frame under `-Os/soft`, and
2,548 bytes with a 96-byte maximum frame under Pico-like `-O3/softfp`.  The
exact lowering has five conditional branches in division, all backward edges
of fixed loops; there is no indirect/table branch, and the 1,512-byte clear is
present.

**Decision.** Accept division only as the third unintegrated structural slice.
It matches truncating `ibz_div`, not floor/Euclidean remainder semantics.
Modular reduction, exponentiation and square root are the remaining integer
primitives before production Cornacchia integration.  Fixed control flow and
addresses do not imply constant power, so whole-Sign, analog-resistance,
published-attack-closure and production claims remain false.

## Cycle 62 — compose fixed signed modular reduction

**Question.** Can current `ibz_mod` floor-remainder semantics be built from the
fixed division slice without adding secret-dependent control flow or growing a
production stack frame?

**Method.** Add a 2,160-byte caller workspace.  Overlay the 1,512-byte division
workspace with the later 432-byte add workspace, while keeping quotient,
remainder and saved output disjoint.  Always execute truncating division and
the add path; select `r + modulus` only when the nonzero truncating remainder
and operand signs differ.  Detect the `INT_MIN / -1` quotient-overflow case on
a fixed scan so `INT_MIN mod -1` succeeds with zero.  A zero modulus executes
the complete work and preserves the initialized output.

**Result.** The independent component test passes 100 boundary and 320 random
reductions in addition to the division corpus (digest
`986e2674f40cea4b`).  The bridge agrees with current `ibz_mod` on 512 signed
full-width/bounded/small-divisor inputs (digest `97d3689f94961142`).  Arm GNU
15.2.1 emits 2,116 bytes of component text under `-Os/soft` and 2,840 bytes
under Pico-like `-O3/softfp`; the largest frames remain 80 and 96 bytes.  The
reduction wrapper has only two fixed backward loop edges, exact calls into the
audited division/add/cmov primitives, no indirect/table branch, and an exact
2,160-byte clear.

**Decision.** Accept signed reduction as the fourth unintegrated structural
slice.  Modular exponentiation and integer square root remain the final two
integer primitives before Cornacchia integration.  The reduction still
processes secret-dependent data values and is not masking, constant-power, or
physical leakage evidence.

## Cycle 63 — add fixed-round modular exponentiation

**Question.** Can the Level-I Cornacchia exponent bound be implemented on the
fixed-width substrate without exponent-bit-dependent operation counts or
memory addresses, while keeping invalid inputs fail-closed?

**Method.** Add a 3,240-byte caller workspace that overlays the 2,160-byte
reduction workspace with the 1,080-byte multiplier and retains four 216-byte
residues plus the saved output.  Reduce the base and one, then execute exactly
521 most-significant-bit-first rounds.  Every round squares and reduces,
multiplies by the base and reduces, and selects the second result with `cmov`.
Accept only a nonnegative exponent of at most 521 bits and a positive modulus
of at most 863 bits; the modulus bound proves a product of two reduced
residues is below 2^1726 and hence representable.  Invalid inputs still run the
complete schedule and preserve the initialized output.

**Result.** Independent branchy 128-bit reference cases pass four valid and
five invalid inputs, including the exact exponent and modulus bounds and
output aliasing with base, exponent and modulus (component digest
`007f3c306f7aac62`).  Three additional cases, including bit 520 and a
2^862 modulus, agree byte-for-byte with current Level-I `ibz_pow_mod` (bridge
digest `a13359b3bfeb0316`).  Strict O3 and fatal ASan/UBSan pass.  Arm GNU
15.2.1 emits 2,468 bytes of component text under `-Os/soft` and 3,192 bytes
under Pico-like `-O3/softfp`; all frames remain static with 80/96-byte maxima.
The pow frame itself is 64 bytes in both profiles.  Exact disassembly has only
the fixed initialization and 521-round back-edges, two multiply and four
reduction call sites, no indirect/table branch, an exponent-word address from
the public round counter, and a 3,240-byte final clear.

**Decision.** Accept this only as the fifth unintegrated structural slice.
The exponent bit still selects secret-dependent data and can leak switching
activity; this is not masking, constant power, a physical SPA result, or proof
that ePrint 2025/830 is blocked.  Integer square root is the last missing
integer primitive before a production Cornacchia integration experiment.

## Cycle 64 — complete the integer foundation with exact square root

**Question.** Can the exact-square predicate used at the end of Cornacchia be
implemented without Newton termination, secret-derived addresses, stack VLAs,
or publishing a partial floor root on failure?

**Method.** Add an 864-byte caller workspace containing restoring remainder,
root, subtract candidate and saved output.  Process all 864 input bit-pairs
from public indices 1726..0.  Every radix-4 round shifts remainder/root,
computes `2*root+1`, performs a full 54-word subtract, selects with `cmov`, and
publishes one root bit.  Accept only nonnegative inputs with zero final
remainder.  Negative and nonsquare inputs execute the same schedule, preserve
the initialized output, and clear the complete workspace.

**Result.** Ten small/sign/boundary cases and 32 random exact squares plus
their 32 adjacent nonsquares pass an independent binary-search/metamorphic
reference, including output/input aliasing and root `2^863-1` (component
digest `379099567d0dfe54`).  Thirty-six additional cases agree with current
Level-I `ibz_sqrt` (bridge digest `e231a2fda31f1500`).  Strict O3 and fatal
ASan/UBSan pass.  Arm GNU 15.2.1 emits 2,758 bytes of component text under
`-Os/soft` and 3,480 bytes under Pico-like `-O3/softfp`; all frames remain
static with 80/96-byte maxima and the sqrt frame is 64 bytes.  Exact lowering
has only three fixed backward edges, no indirect/table branch, and an input
word address visibly derived from the public pair counter.

**Decision.** The 19-operation Level-I integer dependency set is now complete
as an unintegrated structural experiment.  This does not complete Cornacchia,
whole-Sign constant time, masking, or physical resistance.  The next gate is
to integrate these fixed-width operations at the exact production Cornacchia
locus while preserving KAT bytes, RNG consumption, failure semantics, bounds,
workspace lifetime, and the no-legacy-fallback property.

For each commit:

```text
derive expected byte delta
run official/compact deterministic correctness
run sanitizer and malformed-input tests
measure exact workspace, stack, cycles/wall time and code size
record constant-time impact
accept or revert before adding the next transformation
```

## Cycle 65 — integrate the fixed-round Cornacchia half-GCD

**Question.** Can the secret-dependent Euclidean loop identified by the
Cornacchia timing literature be replaced on the exact Level-I Sign route
without changing signatures, RNG consumption, failure behavior, or the outer
workspace size?

**Method.** Extend CTINT1728 with a fixed 1,421-round shift/subtract half-GCD
for moduli up to 492 bits, using a 3,456-byte core workspace and a 4,320-byte
`ibz_t` adapter.  Freeze completed state with conditional moves rather than
early termination.  Integrate it only behind the protected Level-I Cornacchia
configuration and abort on an out-of-contract result instead of falling back
to the legacy division loop.  Reuse the existing square-root phase union, so
the outer workspace grows by zero bytes.

**Result.** Twelve deterministic Sign seeds execute 361 protected Cornacchia
calls and 512,981 half-GCD rounds.  PK/SK, signed messages, verification and
the next RNG bytes are byte-identical to the control corpus.  The adapter and
core clear 8,640 and 6,912 logical bytes per call.  Arm GNU 15.2.1 emits static
48/64-byte frames in the linked RP2350 profile.  The core disassembly has one
public `bne.n` loop edge, no register-indexed memory, indirect/table branch,
or legacy division.  The conservative 1,421-round formula follows ePrint
2023/807, but has not been independently formalized for this exact C program.

**Decision.** Accept this as an experimental component/control-address
checkpoint.  It removes the legacy Euclid iteration channel on this route but
does not mask secret-derived operands, update masks, remainders, or outputs.
The complete signer remains variable-time, and neither the 2023 timing attack
premise nor the 2025 power attack is considered closed without physical and
whole-Sign evidence.

## Cycle 66 — freeze an RP2350 half-GCD leakage target

**Question.** Does the fixed half-GCD retain a coarse dependence on public
input classes, and is the component ready for a defensible physical leakage
campaign?

**Method.** Build a Level-I/RADIX32 RP2350 image whose interrupt-masked GPIO 2
window contains exactly one half-GCD adapter call.  Precompute and validate 16
public 368-bit roots and reference remainders outside the trigger.  Capture 30
alternating pairs for (A,B) identical inputs, (L,H) fixtures whose legacy
Euclid counts are 95 and 122, and (F,R) repeated versus varying public inputs.
Freeze the ELF/UF2/BIN/map, source hashes, all serial rows, and a separate
physical plan requiring a matched legacy positive control, two 10,000-trace
sessions, first-/second-order Welch screens, and a preregistered per-round
mask/state classifier.

**Result.** All 180 measured calls use exactly 1,421 rounds and pass output,
canary and full-workspace-clear checks.  The linked diagnostic image is
46,472 bytes text, 0 data and 24,824 BSS.  At the firmware's 1-us reporting
resolution, B−A is −0.467 us on average (approximate 95% CI
[−1.372,+0.438]), H−L is +0.200 us ([−0.530,+0.930]), and R−F is +2.000 us
([+0.949,+3.051]).  The last effect is small but positive in this one run; it
precludes an equivalence claim and cannot be assigned to a particular
leakage mechanism.  The attached machine has the RP2350 board but no shunt,
current/EM probe, oscilloscope/SCA device, instrument CLI, NumPy/SciPy, or
ChipWhisperer stack, so no analog samples were captured.

**Decision.** The target and preregistration are ready, but physical execution
is blocked on acquisition hardware.  Preserve `analog=false`,
`side_channel_resistance=false`, `published_attack_blocked=false`, and
`production_signing_approved=false`.  Fixed control and addresses are a useful
countermeasure and alignment aid, not a power-resistance result.

## Cycle 67 — execute the matched variable-Euclid positive control

**Question.** Is the fixed half-GCD H/L non-detection merely a coarse timer
that cannot resolve any relevant difference, or does the same design detect
the known secret-dependent Euclid iteration channel when it is deliberately
restored?

**Method.** Build and final-ELF-audit a separate positive-control firmware
using the same RP2350 Arm-S core, 150-MHz clock, GPIO 2 window, public 368-bit
fixtures, precomputed roots/references, 16-KiB PSP and serial exclusion as the
fixed target.  The measured helper deliberately runs the legacy Euclidean
division loop.  Freeze its source, ELF/UF2/BIN/map, step counts and 30 balanced
pairs, then compare its fixture-4/fixture-7 H/L result against the frozen
fixed-1421-round corpus.

**Result.** The legacy ELF retains one conditional loop back edge and one
compare/divide/multiply call site.  All outputs, 95/122 step counts, guards and
1,080-byte workspace clears pass.  The legacy H−L mean is `+1476.8667 us`
with approximate 95% CI `[+1474.9005,+1478.8328]`; elapsed time versus legacy
step count has Pearson `0.9999875` on the H/L corpus and `0.9937828` across the
random fixture corpus.  The identical-input B−A CI crosses zero.  By contrast,
the fixed firmware's H−L mean is `+0.2000 us` with CI
`[-0.5303,+0.9303]`.  The matched difference of mean differences is
`1476.6667 us`.

**Decision.** The coarse timer positive control is detected, so the fixed
H/L non-detection is meaningful evidence that the iteration-count timing
mechanism was removed at 1-us granularity.  It is not an equivalence test and
does not validate shunt-current or EM sensitivity.  The same legacy image must
be acquired first as the analog positive control; until then
`analog=false`, `side_channel_resistance=false`,
`published_attack_blocked=false`, and `production_signing_approved=false`
remain mandatory.  After capture, the board was restored to UF2
`ce228b151a0c79234d9a51777a2b1ed3ac47388ab28f4dda4129a97f0086692f`;
the serial banner independently reports `halfgcd_schedule=fixed rounds=1421`.

## Cycle 68 — independently bound the frozen half-GCD recurrence

**Question.** Does the Bernstein--Yang-derived 1,421-round schedule actually
cover the exact shift/subtract recurrence implemented here, without assuming
that a divstep theorem automatically transfers to Algorithm 8?

**Method.** Pin the byte-identical component and integrated C sources, map the
sort/bit-length/shift/subtract/freeze statements to an integer recurrence, and
prove a one-round contraction for `S=a^2+b^2`. When `alpha=0`, the required
inequality factors as `24(r-1/4)(7/6-r)>=0`. When `alpha>=1`, setting
`x=2^alpha*r` gives `1/4<x<1`, `r^2<=x^2/4`, and a concave quadratic positive
at both endpoints. Thus every active round has `S_next <= (10/17)S`.
Machine-check the rational identities and the exact large-integer stopping
inequality, exhaust all 523,776 `(M,u)` pairs with `M<2^10`, and compare 10,000
deterministic 492-bit model inputs to ordinary Euclid.

**Result.** Since initially `S<2M^2`, while an active state has
`S>=M`, the exact inequality `2^493*10^644 < 17^644` proves that 644 active
rounds suffice for every positive `M<2^492` and `0<=u<M`. The previous round
does not satisfy this particular proof inequality. The scheduled 1,421 rounds
therefore leave 777 freeze rounds. Reduced-domain comparison is exact; the
10,000-case full-width model maximum is 334 active rounds at frozen case 3437.

**Decision.** Close the independent source-recurrence bound gate and retain
the stronger 1,421-round schedule. Do not call this a formal C/assembly
refinement, constant-power result, whole-Sign constant-time result, or closure
of the published attack. The next empirical gate remains a matched analog
positive control followed by protected power/EM mask/state classification.

## Cycle 69 — reject naive Cornacchia exponent blinding

**Question.** Can the SPA paper's exponent-randomization direction be realized
simply as `E=e+r*(p-1)`, and can the entropy of `r` be used as the claimed
security level?

**Method.** Start from the attacked affine exponent relation
`e=(p+k1)/k2`.  Derive the exact expression left after recovering `E`, check
modular correctness over the Legendre and common square-root exponent forms,
exhaust all bases in a reduced domain, replay all 16 frozen RP2350 acquisition
fixtures, and compute fixed-schedule width, fresh-randomness cost and a clearly
labelled leading-term ECM proxy for candidate blind widths.  Freeze the result
as a rejection-oriented design contract rather than implementing it first.

**Result.** The checker covers 349,568 reduced modular cases and 16 full-width
public fixtures.  Correctness holds, but so does
`k2*E-(k1+1)=(1+k2*r)*(p-1)`: exponent recovery leaves a structured
factorization problem.  A 128-bit blind has only about a 40.7-bit
leading-ECM-term proxy for a blind-sized factor.  The table's first row above
128 proxy bits uses an 896-bit blind, expands 521 fixed rounds to 1,408
(`2.7025x`), and consumes about 20.35 KiB of fresh randomness per average Sign
before formation cost.  This heuristic is not a concrete security estimate;
known structure, multiple traces and leakage during construction remain open.
The frozen square-root source supplies four affine exponent observations under
one modulus.  Conditional on exact recovery of all four independently blinded
exponents, their transformed GCD equals `p-1` in 4,017/4,096 deterministic
128-bit-blind trials and 4,032/4,096 896-bit-blind trials.  Blind width therefore
does not repair this multi-observation construction; physical exponent
recovery probability remains unmeasured.

**Decision.** Reject standalone exponent blinding pending SQIsign-specific
cryptanalysis and a reviewed leakage model.  Keep masking unavailable,
physical resistance false, the published attack open and production signing
unapproved.  Run the preregistered analog positive control and protected
mask/state attacks before selecting a masking or randomization construction.
Add `ANALOG-3` as a mandatory future-candidate gate: group four recovered
power candidates under one modulus and reject on reproducible `p-1`/`p`
recovery, rather than judging randomization from a single pow trace alone.

## Cycle 70 — reject marginal-only selector masking

**Question.** Is it sufficient to Boolean-share the exponent selector so that
each share has a secret-independent Hamming-weight distribution?

**Method.** Model a `w`-bit selector `M=0-b` as
`share0=S`, `share1=S XOR M` for uniform `S`.  Exhaust every secret/share
assignment for `w=1..12`, compare individual value and Hamming-weight
distributions, and evaluate the consecutive `share0 -> share1` Hamming
distance.  Check the 32-bit result symbolically and bind the model to the
frozen selector source.

**Result.** All 16,380 reduced assignments confirm that either share alone has
the same distribution for both secret bits.  Nevertheless,
`HD(share0,share1)=HW(M)`: it is 0 for `b=0` and `w` for `b=1`, giving exact
bit recovery in every assignment.  At the production selector width this is
0 versus 32.

**Decision.** Reject the naive sequential two-share schedule.  This does not
reject reviewed masking gadgets generally and does not prove physical
observability.  A future candidate must include arithmetic-state masking,
final-assembly share tracking and transition-aware `ANALOG-4` attacks; static
per-share leakage checks alone are not an acceptance criterion.

## Cycle 71 — expand masking scope to the secret Montgomery context

**Question.** If the exponent selector is masked correctly, is the protected
Cornacchia exponentiation then an adequate first-order masking candidate?

**Method.** Freeze the archived 521-bit Montgomery implementation and state
layout by SHA-256, then trace dependencies from the secret-derived temporary
modulus, base/radicand and exponent through context initialization and every
multiplication/reduction.  Encode those dependencies as a graph and calculate
the uncovered sensitive nodes for schedule-only, selector-only, exponent-only,
accumulator-only, context-only and base-only countermeasure scopes.

**Result.** All 18 inventoried values are reachable from a secret root.  In
addition to exponent and selector state, the source directly handles modulus
words, the low-word Montgomery inverse, `R mod M`, `R^2 mod M`, schoolbook
products, reduction factors, carries, borrows and reduced accumulator state.
Every one of the six partial scopes leaves at least one named value path
unprotected.  The new checker passes against the frozen source and fails
closed with every masking/resistance decision false.

**Decision.** Do not implement selector masking as an isolated patch.  The
next construction must cover the complete dynamic-modulus Montgomery state
and surrounding Tonelli--Shanks composition, with composable gadgets,
transition-aware final assembly and fresh-mask accounting.  Only after that
structural gate may `ANALOG-4` evaluate first-/higher-order leakage and
horizontal exponent/modulus/state recovery.  No masked candidate or physical
resistance claim exists yet.

## Cycle 72 — select a reduced arithmetic-masking prototype

**Question.** Should the complete 521-bit Montgomery scope first be realized
as a Boolean circuit, or can arithmetic sharing retain word-level efficiency
without ignoring carry/comparison and transition leakage?

**Method.** Model two-share Boolean secure-AND and full-adder gadgets
exhaustively, recording both single-wire value distributions and adjacent-share
Hamming-distance transitions.  Derive a deliberately simple cost proxy for a
17-by-32-bit Boolean schoolbook product and the 1,042 products in one fixed
power call.  Compare that reference with word-level arithmetic sharing using
Boolean conversion only at non-linear carry, borrow, comparison and selection
boundaries.  Bind the design to the complete 18-node Montgomery scope and to
the transition-aware `ANALOG-4` campaign.

**Result.** All 32 secure-AND and 256 full-adder assignments satisfy
correctness and secret-independent individual-wire marginals.  Nevertheless,
the Hamming distance between consecutive complementary shares exactly exposes
the corresponding input, output, sum or carry bit.  The deliberately naive
Boolean proxy requires 1,479,680 secure ANDs and 184,960 fresh random bytes per
544-bit product, or 192,728,320 bytes (183.800 MiB) per fixed power call.  That
number is not a lower bound and deliberately excludes reduction, refresh and
assembly hardening.

**Decision.** Keep the full Boolean construction only as a correctness/value
model oracle.  Advance a two- or three-limb word-arithmetic sharing prototype
with reviewed A2B/B2A boundaries, fresh randomness for every non-linear
composition, and hand-controlled share-touching Cortex-M33 primitives.  Pure C
and individual-share marginals cannot approve it.  No masked Montgomery
implementation, probing proof, physical result or published-attack closure is
claimed at this checkpoint.

## Cycle 73 — implement the two-limb functional arithmetic oracle

**Question.** Can the selected word-arithmetic direction be made concrete
without accidentally treating a correct C program as a transition-secure
implementation?

**Method.** Implement arithmetic sharing over `Z/(2^64)` as two 32-bit limbs.
For multiplication, choose a fresh uniform ring element `r` and compute
`c0=a0*b0+r` and
`c1=a1*b1+(-r+a0*b1+a1*b0)` modulo `2^64`.  Check share,
recombine, share-wise add, refresh, multiply and all documented aliases over
64 edge pairs and 200,000 deterministic random pairs in Release and fatal
ASan/UBSan builds.  Cross-compile the functional C with Arm GNU 15.2.1.  In a
separate exhaustive model, enumerate every secret, input mask and multiplication
mask for rings of one through four bits, and classify individual values,
direct adjacent-share HD, reviewed internal-lane HD and ideal zero-precharge
HD independently.

**Result.** The C oracle passes all 200,064 pairs with checksum
`ffa2febdd10c5801`; the Cortex-M33 multiplication frame is 48 bytes static and
has no unresolved symbols.  The reduced model covers 1,082,400 assignments.
All operation identities and individual-wire marginals pass.  Five direct
channels—both input share pairs and add, refresh and multiply output share
pairs—have secret-dependent HD at every width.  The reviewed masked internal
lanes and transitions to/from an ideal zero precharge have
secret-independent HD distributions in this model.

**Decision.** Accept the code only as a functional/value-model oracle.  The
zero-precharge result is necessary scheduling evidence, not a Cortex-M33
microarchitectural proof.  Next implement one share-touching primitive in
hand-controlled assembly, freeze its instruction/register/memory transition
schedule, and require direct physical TVLA/HD attacks before adding A2B/B2A or
scaling beyond two limbs.  `published_attack_blocked=false` and production
approval remain mandatory.

## Cycle 74 — freeze and execute one transition-controlled M33 refresh

**Question.** Can the first share-touching primitive be removed from compiler
register allocation, executed as exact Cortex-M33 code, and still be kept
strictly below a physical-resistance claim?

**Method.** Implement a two-share refresh over `Z/(2^32)` in hand-written
Thumb assembly: `y0=x0+r`, `y1=x1-r`, with a fresh uniform `r`.  Route the
modeled load, store, result-register and ALU paths through an explicit zero
scratch value; erase every share-holding register before return.  Pin the
Arm GNU 15.2.1 function bytes, symbol size, instruction count, stack frame,
relocations and branch/call absence.  Execute that exact function on the QEMU
`mps2-an505` Cortex-M33 model over all 36 edge pairs and 10,000 deterministic
random cases, checking both output formulas, recombination, scratch erasure and
checksum `0x6f653503`.  Separately exhaust all secrets, input shares and masks
in rings of one through six bits, comparing value, operation-event and HD
distributions.

**Result.** The function is exactly 66 bytes, 23 instructions and a 16-byte
saved-register frame, with no calls, conditional branches, relocations or
undefined symbols.  QEMU passes all 10,036 cases.  The reduced model covers
299,592 assignments: individual values and add/sub event tuples are
secret-independent; both direct input- and output-share transitions leak at
every width; every frozen transition routed to or from zero passes this
first-order HW/HD abstraction.  To avoid selecting only convenient hand-named
transitions, a second model expands the exact 23-instruction sequence into all
55 adjacent ALU A/B/result, r2/r4/r5/r12-write and load/store-data channels.
It covers 37,448 reduced assignments through five bits with no first-order
class difference; direct ALU, load and store share-to-share mutations all fail.

The same 66-byte function was then embedded without recompilation into a
Pico 2/RP2350 acquisition image.  The image was built with Arm GNU 15.2.1,
passed the ELF window policy, was flashed, and returned 12 valid alternating
`A/B/R` serial records with correct share formulas, recombination, scratch
erasure and context clearing.  A follow-up boundary audit found two physical
experiment confounds before any traces were taken: A/B and R had different
RNG-call histories, and caller-saved `r12` had no specified entry value.  The
fixture now executes exactly three RNG calls for every class and precharges
`r12` before GPIO-high; the ELF checker verifies both properties.  Frozen
evidence is build manifest
`227f064c3b9b4bc0d727fe853e5b35a259009ed4cc3e6772eaa1d3fe501d5d9e`,
ELF `1ec2a8457b13382f793bf962781efdd532b576771004df3ed1b08c61e5f2aabb`,
UF2 `17dee8f0c71e070f5cb371123419ddea3263b634ab71531cedf22dfbde369735`
and serial-smoke
`b8ab1d448f37787ff3c7bedc458c6eb3dddea6678b6dfacd21fda05875aacd7d`.
The GPIO window contains exactly one refresh; RNG, preparation, checking,
clearing and USB output are outside it.  Equalizing acquisition history
prevents those labels from measuring different preparation paths, but does
not establish physical resistance.

The `ANALOG-5` analysis path is also executable before instrument arrival.
The frozen manifest enforces exact firmware hashes, balanced AB/BA pairs and
validates every per-trace share, mask, output and clear field.  Its scorer
reports pointwise first-order and centered-square TVLA, twelve preregistered
secret-attack/diagnostic HW/HD/carry/borrow models and two deterministically
permuted null controls.  A follow-up audit found that pointwise squaring alone
cannot detect the standard two-share case where two individually quiet samples
leak only through their product.  The scorer therefore also evaluates at most
4,096 cross-time centered products between two disjoint windows selected from
public trigger/instruction timing and frozen before inspecting class labels or
statistics.  An end-to-end synthetic regression injects a class leak,
direct-input-share HD and a cross-time-only second-order pair at known samples
and recovers all three, while confirming that the pair's individual samples do
not cross either pointwise threshold and forcing
`analog_capture_run=false`, `campaign_evidence_eligible=false` and
`side_channel_resistance_established=false`.  This validates the analysis
pipeline without manufacturing physical evidence.  A separate comparator
requires six distinct reports (three datasets times two acquisitions), aligns
pointwise samples and both cross-time coordinates within three samples,
rejects reproducible attack/TVLA
signals and treats control failure as inconclusive.  Even six negative reports
remain bounded evidence for this exact primitive only.

The comparison audit also corrected the polarity of the fixed/fixed dataset:
an aligned attack-model alert there is evidence that the acquisition/control
is confounded, not evidence against the refresh.  The comparator now permits
rejection signals only in the two primary purposes and makes any fixed/fixed
model, pointwise or cross-time alert inconclusive.

A fail-closed 2026-08-23 readiness inventory found the flashed RP2350 target
and the complete ANALOG-5 scoring/decision pipeline, but no scope/SCA device,
current probe, near-field EM probe or acquisition software.  It therefore
records `analog_capture_run=false` and zero physical samples; the next action
is external instrumentation rather than another synthetic inference.

**Decision.** Retain the primitive only as an instruction-set-functional,
abstract-transition checkpoint.  QEMU does not model RP2350 register-file or
bus coupling, pipeline remnants, interrupts, power or EM.  Preregister
`ANALOG-5`: a byte-identical IRQ-policy-explicit RP2350 fixture, independent
masks, fixed/random plus fixed/fixed controls, first-order TVLA, second-order
characterization and direct HD/classification attacks in two independent
acquisitions.  Do not start masked multiplication, Montgomery integration or
claim closure of ePrint 2025/830 until this local primitive survives that
physical gate.  The fixture is now built/flashed/functionally smoked, but no
power or EM trace has been acquired; `published_attack_blocked=false` remains
mandatory.

## Cycle 75 — bind the published SPA attack to normal and protected objects

**Question.** Does the current SQIsign lineage still contain the concrete
implementation prerequisite used by the SPA attack of ePrint 2025/830, and
what exactly has the experimental Cornacchia route removed?

**Method.** Freeze the normal D13 and protected experimental source hashes,
then rebuild their RepresentInteger, Cornacchia, square-root and exponentiation
objects under the same Arm GNU 15.2.1 Cortex-M33/RADIX32 `-O3` profile.  Check
the relocation and disassembly chains separately.  In the normal object,
require the legacy square root, two variable Euclid divisions, three direct
legacy pow sites and the compiler-expanded secret-bit-conditioned multiply
shape.  In the protected object, require the fixed-work square root, fixed
half-GCD, five fixed pow calls, no accepted-path legacy sqrt and only the final
validation division.  Bind this structural result to the frozen twelve-seed
Sign profile and signed-message transcript.

**Result.** The normal chain is structurally reachable exactly as required by
the published attack: `quat_represent_integer -> ibz_cornacchia_prime ->
ibz_sqrt_mod_p -> ibz_pow_mod`.  Arm GCC specializes the source's one
bit-conditioned result multiply and one loop-progress square into six total
relocation sites while retaining exponent-bit shift/sign branches.  The
experimental accepted path removes that legacy
operation-count schedule and the variable Euclid loop, but retains unmasked
selector and prefix-dependent arithmetic values.  The frozen corpus contains
361 Cornacchia square roots and 2,180 fixed exponentiations across 12 Sign
executions, with 75--409 exponentiations per Sign and 521 rounds per
exponentiation.  Both the source/runtime checker and the fresh-object Arm gate
pass.

**Decision.** Treat the normal implementation as containing the published
attack prerequisite.  Treat the experimental path only as schedule
regularization, not as a countermeasure that closes the paper.  Physical SPA,
exact exponent recovery and signing-key recovery remain untested;
`published_attack_blocked=false`, whole-Sign resistance and production signing
approval remain mandatory until two independent physical campaigns and the
preregistered attacks are completed.

## Cycle 76 — replay the published SPA algebra and bind physical attack gates

**Question.** If a power classifier recovers an exponent, does the frozen
evaluation pipeline actually reconstruct the Cornacchia input and continue
toward the key as described in ePrint 2025/830, or is that connection only a
literature citation?

**Method.** Encode the three affine exponent-to-input relations from the paper
as an exact fixture contract, add corrupt/no-candidate negative controls, and
separate the physical campaign into the isolated current-C Legendre target and
the integrated paper exponent branches.  Pin the authors' key-recovery code at
commit `3be37656c2b16ff048c8ca51512a7c2dea6f93a9`, apply only a Sage 10.7 API
compatibility patch, archive its raw output, and require its algebraic
backtracking and valid-signature markers.  Reject synthetic reports as physical
campaign evidence.

**Result.** Known paper-branch exponents reconstruct 24/24 frozen inputs via
`4e+1`, `8e-3`, or `8e+5`.  The isolated current-C Legendre exponents do not
match those paper formulas, but directly reconstruct 16/16 fixture moduli as
`m=2e+1`.  The pinned 54-bit author demo records 12 successful
RepresentInteger steps, 7 successful StrongApproximation steps and one valid
signature.  Exact synthetic exponent predictions score 16/16, while one-bit
corrupt and no-candidate controls score 0/16 and remain evidence-ineligible.

**Decision.** The algebraic attack chain is reproducible, so a future physical
exponent leak must be treated as key-recovery relevant rather than as a benign
component leak.  This run starts from public fixtures or author-provided toy
intermediates; it does not recover an exponent from RP2350 power/EM or a key
from the current C signer.  Keep `published_attack_blocked=false` and
`side_channel_resistant=false` until two independent `ANALOG-1A/1B`
campaigns, the integrated key-recovery scoring and the remaining twelve
surfaces pass.

## Cycle 77 — trace complete Sign control flow and memory addresses

**Question.** After regularizing the isolated Cornacchia exponentiation and
half-GCD schedules, does the complete compiled Sign operation still expose a
reproducible input-dependent branch or address trace?

**Method.** Rebuild every frozen D13 project object with Apple Clang 17
SanitizerCoverage edge, load and store callbacks. Keep the tracing harness
uninstrumented, enable callbacks only around encoded Sign, and verify outside
the measured interval. Hold the Level-I key and message fixed. Alternate two
seed-0/seed-0 negative-control pairs and four seed-0/distinct-signing-RNG pairs,
then repeat the complete experiment in a second process using the identical
binary. Stream two independent 64-bit edge and address hashes plus exact event
counts; retain bounded prefixes only for early divergence diagnostics. Rebuild
both JSON reports from the raw rows and require byte-identical summaries.

**Result.** In both runs the 2/2 fixed/fixed controls have identical
signatures, 173,062,335-edge streams, 572,012,107-load streams and
271,404,872-store streams. All 4/4 fixed/random pairs have different
signatures and differ in both full edge and address streams. The first saved
edge divergence indices reproduce at 938,604, 935,361, 938,604 and 933,275;
nearest-symbol diagnostics name `ibz_div` or `ibz_cmp`, but are not exact
source attribution. The saved address prefix ends before the first address
divergence, while full online hashes and counts establish paired inequality in
this diagnostic model.

**Decision.** Replace “complete path not audited” with a positive frozen-host
structural finding. This is not physical leakage evidence and does not observe
data values, register switching, the final Cortex-M33 image or a recovered key.
Nevertheless, stable controls prevent using local countermeasures or negative
component timing as a deployment argument. Keep
`constant_time=false`, `side_channel_resistant=false` and
`production_signing_approved=false`; next localize the divergences, repeat the
audit on the final target, and execute the preregistered power/EM and attack
campaigns.

## Cycle 78 — localize the first full-Sign edge divergences

**Question.** Are the first saved full-Sign edge differences merely
nearest-symbol hints, or can they be attributed reproducibly to concrete
source-level variable-control operations?

**Method.** Rebuild the frozen D13 signer twice from a clean source tree with
Apple Clang SanitizerCoverage edge guards plus its PC table, preserve flat LTO
DWARF, and replay one fixed/fixed control and four fixed/random signing-RNG
pairs. Record the guard immediately before and on both sides of each first
difference, normalize PCs to image offsets, and require exact source
classification in a hash-pinned checker. Keep the Level-I key and message
fixed and verification outside the traced interval.

**Result.** Both clean builds agree. The fixed/fixed control remains exact.
For signing-RNG seeds 1 and 3, the first difference is edge event 938,604 in
the `intbig.c:163-164` high-zero used-limb scan, inlined into dividend-size
calculation at line 250. Seed 2 first differs at event 935,361 and seed 5 at
event 933,275 in the `intbig.c:992-996` most-significant-limb early-exit
comparison. All four reports set physical evidence, long-term-key leakage,
source remediation, target tracing and resistance to false.

**Decision.** Promote variable integer control from a suspected surface to a
reproduced first-divergence finding. Do not patch only the visible condition:
the returned used-limb count still drives division loop bounds and addresses,
and comparison results legitimately drive higher-level algorithms. The
remediation direction is the existing fixed-width/fixed-round integer
substrate, followed by another full-Sign trace to expose the next divergence.
Keep `constant_time=false`, `side_channel_resistant=false` and
`production_signing_approved=false`; no long-term key leak or physical
power/EM leakage has been demonstrated by this host localization.

## Cycle 79 — identify the first common remediation boundary

**Question.** Do the two source-level first-divergence classes require two
independent patches, or do they share a higher-level signing owner that must be
protected as one unit?

**Method.** Rebuild the frozen D13 signer twice with the same edge guards,
frame pointers and LTO DWARF used by the source-localization experiment. At
only the eight frozen class-A/class-B first-difference events, capture a native
caller stack and normalize every project frame to its function and image
offset. Require the exact event, guard, seed, stack order, source/binary/DWARF
hashes and cross-build normalized equality in a fail-closed checker.

**Result.** Both clean builds agree. The used-limb class follows
`ibz_divrem_unsigned_wide` → `ibz_div` → `ibz_div_floor` → `ibz_mod` →
`quat_alg_norm_mod`; the early-comparison class follows `ibz_cmp` →
`ibz_rand_interval`. Both paths share
`quat_sampling_random_ideal_O0_given_norm_impl`, then
`protocols_sign_with_workspace` and `sqisign_sign_with_workspace`.

**Decision.** Select the random-ideal sampler as the next remediation
boundary. Do not accept a branchless `ibz_cmp` patch: it would leave
variable-width norm modular reduction, rejection count, RNG consumption and
first-success publication. The next design must use fixed-width arithmetic and
a rigorously specified fixed-budget sampler, then repeat the whole-Sign trace
to expose the next divergence. Keep physical evidence, long-term-key leakage,
constant-time, resistance and production approval false.

## Cycle 80 — prove the outer random-ideal budget

**Question.** Can the variable gamma/beta loops be replaced by public fixed
budgets without inferring a tail bound from a small signing corpus?

**Method.** Build the frozen D13 Level-I signer with effective `-O0` debug
information and attach read-only LLDB callbacks to the first executable line
of `while (!found)` and `while (!beta_ok)`. Replay the twelve deterministic
Sign seeds and require every signature to verify. Separately count the affine
zeros of `Q=x0^2+x1^2+p*x2^2+p*x3^2` over the fixed odd prime field. Check the
formula and the primitive-lift transformation exhaustively over four reduced
prime fields and fifteen nonzero-coefficient cases. Account exactly for a
fixed 769-bit modulo-N coordinate sampler rather than treating its output as
perfectly uniform.

**Result.** There is one prime commitment sampler call per frozen signature.
Gamma takes 1--4 attempts with distribution `{1:5, 2:5, 4:2}` and mean
`1.9167`; beta takes one attempt in all twelve. These values remain descriptive.
The exact four-dimensional zero count is `N^3+N^2-N`; gamma has
`(N^3+2N^2-N-2)/2` successful triples before lift correction, hence success
probability greater than one half. If `N^2 | Q(x)`, changing the first nonzero
canonical coordinate from `xi` to `N-xi` makes `Q/N` nonzero modulo N and
removes the lift rejection. A budget of 130 gamma and one beta candidate,
using 394 fixed 769-bit reductions, gives combined exhaustion plus coordinate
distribution distance strictly below `2^-128`. The reduced checker exercises
845 actual primitive corrections.

**Decision.** Promote the random-ideal design from “outer budget unknown” to
“budget proved, implementation pending.” Reject the old 128-inner-rejection
baseline because it would require 50,432 coordinate candidates. Use the
38,218-byte wide-reduction schedule as the conservative first implementation
point, measure its cost honestly, and investigate a direct isotropic sampler
only as a later optimized design. Keep implementation, target trace, physical
leakage, constant-time, resistance and production approval false.

## Cycle 81 — implement the first fixed-budget random-ideal components

**Question.** Can the wide-coordinate and primitive-lift parts of the proved
budget be implemented without prematurely claiming that the complete sampler
or signer is side-channel resistant?

**Method.** Add standalone 54×32-bit components over the frozen CTINT1728
substrate. The coordinate API consumes exactly 97 little-endian bytes, masks
the seven public excess bits and executes the complete 1,728-round modular
reduction. The primitive API evaluates the four-dimensional norm, selects the
first nonzero coordinate by masks, applies `N-xi` only when `N^2` divides, then
recomputes both divisibility predicates before transactional publication.
Both APIs clear their caller-owned workspaces on every non-NULL-workspace
return. Tests cover 256 random reductions, five exact Level-I boundary vectors,
64 actual corrections, input/output aliasing, failure nonpublication and NULL
contracts under optimized host and fatal ASan+UBSan builds.

Arm GNU 15.2.1 individual-TU gates pass for `-Os`/soft and Pico-like
`-O3`/softfp. Coordinate/primitive workspaces are 2,808/6,480 bytes; their
entry frames are 32/88 bytes in both profiles. The helper text is 876/932
bytes. The checker permits only the fixed public 97-byte and four-coordinate
loop back edges after pointer validation; it is structural evidence, not a
formal or analog leakage proof.

**Decision.** Accept the two helpers as a component checkpoint only. Do not
promote SCA-20 to complete: the 130/1 loops, fixed-work modular square root,
first-success selection, ideal construction and Sign integration remain open.
The deterministic correction changes the rare legacy rejection map, so a
proof or tight bound for the required output-ideal distribution is now an
explicit integration blocker; finite transcript equality cannot discharge it.
Keep `constant_time=false`, `side_channel_resistant=false` and
`production_signing_approved=false`.

## Cycle 82 — add the fixed Level-I modular-square-root component

**Question.** Can the modular square root needed by the fixed-budget Level-I
random-ideal design be regularized without importing another variable-time
Tonelli--Shanks or Cornacchia path?

**Method.** Use the exact public Level-I prime `N=2^512+75`, for which
`N mod 4 = 3`, and compute `a^((N+1)/4) mod N` with the already frozen
521-round fixed-width exponentiation. Always square and reduce the candidate,
compare all 54 words, and mask-select publication only when the check succeeds.
Keep input/output aliasing, failure nonpublication and complete workspace
clearing as explicit API contracts. Test zero, eight nonzero residues, eight
deterministic nonresidues, aliasing and NULL contracts under optimized host and
fatal ASan+UBSan builds.

**Result.** Both host profiles pass. The square-root workspace is 4,320 bytes,
four-byte aligned. Arm GNU 15.2.1 emits a 48-byte static frame under `-Os` and
56 bytes in the Pico-like profile. The three-helper object now contains
1,104/1,192 bytes of text. Frozen disassembly gates find one public fixed
54-word comparison-loop edge, public pointer guards, direct calls to the
fixed exponentiation/square/reduction primitives, read-only modulus/exponent
constants and no indirect or table branch.

**Decision.** Add the square root to the component checkpoint, but keep SCA-20
partial. The 130/1 outer loops, first-success publication, ideal construction,
protected-output distribution/security proof, Sign integration and physical
power/EM validation remain open. Fixed control/address structure does not mask
operands or establish constant-power behavior, so
`side_channel_resistant=false` remains mandatory.

## Cycle 83 — re-screen the combined protected signer and localize residual addresses

**Question.** After combining the fixed-budget commitment sampler with the
fixed-schedule Cornacchia square root and half-GCD, does the protected Sign
route have identical control-flow and effective-address traces on the frozen
fixed/random corpus?

**Method.** Build the combined Level-I/RADIX32 low-memory closure with Apple
Clang 17 SanitizerCoverage PC tables and flat DWARF. Capture one fixed/fixed
control and four fixed/random signing-RNG pairs for the first 4,000,000 edge
events, then repeat the entire process. Independently instrument loads/stores
only in `ideal.c`, `ibz_bitsize` and `ibz_mul`; retain the first 4,000,000
selected memory events and repeat that experiment in a second process. Compare
source/link PCs and ASLR-normalized image regions/offsets, not raw runtime
addresses.

**Result.** Both fixed/fixed controls are identical. In both full-edge runs,
all 4/4 primary pairs differ at events 1,767,695--1,769,252. Seeds 1--3 first
separate at the ideal-coordinate bit-length maximum scan (`ideal.c:168-169`);
seed 5 first separates at integer copy versus absolute-value handling
(`intbig.c:385/428`). In both focused runs, all 4/4 primary pairs also differ
in selected control flow and effective addresses. First address differences
at events 3,026,298--3,033,316 map to `ibz_bitsize` sign handling
(`intbig.c:401`) or `ibz_mul` used-limb loops (`intbig.c:839-840`). Every
memory capture reaches the public bound.

**Decision.** The known Cornacchia exponent-schedule mechanism is removed on
the measured route, but the combined signer is not constant-time: generic
fixed-storage integer code still exposes secret-reachable sign, bit length and
used-limb control/address behavior. Freeze both repeated runs and their exact
toolchain in `combined-residual-trace-contract.json`. Do not infer physical
leakage, long-term-key recovery or resistance from software traces. Keep
`whole_sign_constant_time=false`, `physical_tvla_pass=false`,
`side_channel_resistant=false` and `production_signing_approved=false`. Next
regularize the generic integer operations and rerun both structural screens,
then execute the preregistered RP2350 power/EM and attack-recovery campaigns.

## Cycle 84 — close the D13 full-vector differential gate

**Question.** Does the final D13 caller-owned encoded KeyGen/Sign/Verify path
preserve the complete 100-vector deterministic behavior, rather than only the
existing twelve frozen transcript seeds and one physical K/S/V fixture?

**Method.** Build two Release, Level-I/RADIX32 profiles from clean Compact
commit `71099e0827d3f0a3b3c705d2eda592c401e0d57d`: the ordinary encoded NIST API
and `SQISIGN_LOWMEM_ONLY=ON`.  Feed the archived official NIST-v2 request
(SHA-256 `81ff60e3ef698751e5572f0bb7f831f069605229c220ee1cf27a92572d6ebc7e`)
directly to a dedicated driver that can call only the explicit workspace
entries.  For every vector, require successful K/S, exact Open/message
recovery, rejection after one deterministic signature-bit flip, intact arena
guards and complete clearing of the owned member.  Repeat the low-memory run
in a fresh process and compare both complete responses with the same-commit
ordinary response.

**Result.** Both low-memory runs pass 100/100 vectors and therefore 200/200
valid Open, corruption-rejection and workspace-contract checks.  The ordinary
D13 response and both low-memory responses are byte-identical, each with
SHA-256 `1d86d15c5d2bfbef99f17634496086a3ab80f0e848d34d3e9409d75f3019dd68`.
The archived request and current generated request have identical 100
`count/seed/mlen/msg` records; the latter omits only the former's final
blank-line LF.  The machine checker also verifies the clean source commit,
normal/low-memory CMake profiles, required workspace symbols and artifact
hashes.

**Decision and boundary.** Close the v2 proposed-implementation full-vector
host regression gate.  Report it as an official-request, same-commit
normal-versus-low-memory differential conformance test, not an official-response
KAT.  Do not claim equality to the legacy
GMP NIST-v2 response: Compact fixed-precision KeyGen intentionally consumes a
different deterministic RNG transcript.  This finite host corpus is not a
formal all-input proof, production-RNG test, physical 100-vector campaign,
timing distribution or side-channel result.

## Cycle 85 — repeat the frozen v2 target path

**Question.** Does the archived D13 firmware reproduce its deterministic
KeyGen/Sign/Verify transcript and stack watermarks after an independent boot,
rather than only in the original capture?

**Method.** Reflash the exact archived UF2 (SHA-256
`6971b9c84a42e26f08d761becd29c2c0e78b4ac1927ae19feb6b2f5c1a035a9f`) on
the same Pico 2 and capture a second complete boot. Fail closed on source or
firmware identity, dirty trees, CPU/clock/stack layout, terminal status,
operation errors, guards, clearing, Verify RNG consumption, PSP/MSP
reservations, or transcript differences. Compare the two captures with a
separate analyzer.

**Result.** Both boots terminate `PASS`. KeyGen and Sign SHAKE256 transcript
digests reproduce exactly. KeyGen/Sign/Verify PSP extents are
91,980/120,452/20,768 bytes in both boots, and the conservative MSP upper
extent is 2,396 bytes. First-boot times are
2,698.150029/7,611.258527/0.814341 seconds; second-boot times are
2,698.150324/7,611.257377/0.814356 seconds.

**Decision and boundary.** Close the planned same-binary repetition as a
bounded reproducibility result. It is two observations of one deterministic
input on one board, not a multiple-input campaign, performance distribution,
retry-tail study, production-RNG test, or worst-case stack proof.

## Cycle 86 — bind the v3 fixed-frame prototype to a linked PSP certificate

**Question.** Can the v3 D2 fixed-frame image obtain an auditable operation-
PSP upper bound and pass a clean target cross-check without overstating this
as a whole-program asynchronous stack proof?

**Method.** Materialize `p324_3/RADIX32` with no compiler-reported dynamic
frames, close linked direct and indirect calls from the three K/S/V thunks,
bind metadata-free assembly/newlib frames by code hashes, and assign
configuration-specific decreasing ranks to three recursive SCCs. Scan every
linked `MSR PSP` site and add 212 bytes for one conservative maximum Secure
Armv8-M exception entry. Separately enumerate candidate interrupt roots and
then run official vector 0 on the exact clean RP2350 image.

**Result.** The final KeyGen/Sign/Verify PSP bounds are
108,300/127,932/40,468 bytes, all within the 131,072-byte reservation. The
clean target capture terminates `PASS`, matches the official vector, and
observes 62,096/101,060/40,252 bytes. Direct-call stack metadata from four
candidate interrupt roots is closed, but 18 unique indirect callback sites
remain. The observed MSP upper extent is 2,188/8,192 bytes.

**Decision and boundary.** Accept an image-specific bound for linked K/S/V
software calls plus one maximum exception entry. Do not promote it to a
whole-program bound: handler software frames, enabled IRQs, priority nesting,
live MSP at entry, C/assembly semantic refinement, other inputs, and other
parameter sets remain outside the certificate.

## Cycle 87 — run the preregistered v3 target fixed-key timing screen

**Question.** After removing the direct key-buffer address and XIP-cache-state
confounds, does key-associated Sign timing reproduce on RP2350 in both the
official and lifetime-overlay images?

**Method.** Freeze the firmware commit, both source commits, reconstruction
patch, ELF/UF2/map/archive hashes, and decision rule before capture. Copy each
of ten official `p324_3` key pairs to one fixed RAM address, use one fixed
33-byte message and one fixed signing-RNG stream, invalidate XIP cache before
every timed operation, and execute two deterministic randomized key schedules
with five repetitions per key. Require every signature to verify and require
stable signature digests, canaries, identities, and terminal markers. Declare
a per-image detection only when the two per-key median rankings have Spearman
at least 0.8 and both key-median spans exceed 1% of the central median and ten
times the median within-key MAD; require both images for the overall result.

**Result.** All 200/200 signatures pass. Both images have between-pass
Spearman 1.0. Official spans are 50.8419% and 50.8514%; adapted spans are
51.3752% and 51.3626%. The same fastest and slowest keys reproduce. Sign PSP
is 101,060 bytes in every official sample and 97,132 bytes in every adapted
sample, preserving the 3,928-byte reduction in all 100 pairs.

**Decision and boundary.** Record repeatable key-associated RP2350 wall-clock
timing. Do not attribute it to secret-only state because the corresponding
public components change with the key pair. The outer image order was fixed,
so the observed inter-image speed delta is descriptive only. This is not a
target control/address trace, physical current/EM leakage, key recovery,
population/worst-case timing, or resistance evidence. Keep all three physical
leakage, key-recovery, and side-channel-resistance decisions false.

## Cycle 88 — close the bounded future-work execution register

**Question.** Are all items previously listed as future work either completed
within a finite local scope or assigned an explicit non-local status without
turning a negative result into a successful security claim?

**Method.** Regenerate one fail-closed register from the v2 second-boot result,
v3 multi-input/placement campaign, fixed-frame linked bound and target check,
layout/evidence generators, post-v3 literature log, v2 and v3 leakage
experiments, and current hardware inventory. Require the v2 repeat, clean
static cross-check, and 200-sample target fixed-key campaign before accepting
local completion.

**Result.** The strict generator terminates
`BOUNDED_LOCAL_PROGRAM_COMPLETE_WITH_EXTERNAL_HARDWARE_BLOCKER` and records
all locally executable bounded campaigns as complete. The SCA hardening and
v3 leakage experiments terminate as negative results: the planned experiments
ran, but resistance did not result. The only unexecuted acquisition is analog
power/EM because the current bench has no current/EM probe or scope/SCA
instrument; physical trace count remains zero. Other PQC schemes and multiple
MCUs remain explicitly outside the author-selected revision scope.

**Decision and boundary.** Close the paper's local bounded work program, not
the research questions in general. Preserve
`whole_program_worst_case_stack_bound_established=false`,
`side_channel_resistance_established=false`, and
`all_research_goals_achieved=false`. The machine-readable register and every
evidence digest are frozen under `results/revision-2026-09-04/`.

## Cycle 89 — close public clean-source reconstruction

**Question.** Can a fresh artifact checkout reconstruct every source head used
by the revised claims, generate the target source packages, and repeat the v3
host correctness gates without an uncommitted source patch?

**Method.** Package the v3 lifetime-overlay and fixed-frame commits as thin Git
bundles over official commit `6d017708…`. Make the preparation script verify
the exact commit, tree ID, and zero tracked differences for v2, official v3,
the lifetime head, and the fixed-frame head. Reconstruct all four trees from
local mirrors, generate the `pqm4` packages, and compare their generated-tree
digests with the frozen target summaries. Finally rebuild the host gates from
the clean official, lifetime, and fixed-frame heads.

**Result.** All four source trees reconstruct at their declared commit/tree
pairs with zero tracked differences. Generated `p324_3/m4f` digests are
`38ee7ae0…` (official), `57e1ae2d…` (lifetime), and `7df287fc…`
(fixed-frame), exactly matching the frozen target records. The first generator
test exposed that the fixed-frame prototype is intentionally `p324_3`-only;
the public wrapper was corrected to avoid attempting the incompatible
`p500_27` and `p664_17` targets. Official and lifetime heads each pass NIST
API, self-test, and all 100 official KAT responses; the fixed-frame head also
passes its 100-response KAT.

**Decision and boundary.** The public source-to-host-gate chain is closed for
the frozen commits and `p324_3` prototype scope. This does not reproduce the
physical target measurements on another board, extend the fixed-frame rewrite
to other parameter sets, close the asynchronous stack obligations, or add any
side-channel resistance claim.
