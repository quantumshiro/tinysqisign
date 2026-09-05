# Benchmark record and methodology

This file records the chronological host diagnostics and RP2350 measurements that underlie the paper. Early entries retain their experiment identifiers and contemporaneous evidence limits. Unless explicitly marked otherwise, values are single-machine smoke measurements and are not directly comparable across different source, compiler, or target configurations.

## Recorded host environment

| Item | Value |
|---|---|
| Date/time zone | 2026-08-16, Asia/Tokyo |
| Machine | Apple Mac17,2, arm64, 24 GiB |
| OS | macOS 26.3.1, Darwin 25.3.0 build 25D2128 |
| Release compiler | Apple clang 17.0.0 (`clang-1700.6.3.2`) |
| Diagnostic compiler | GCC 15.2.0 / Apple clang 17.0.0 |
| CMake / Ninja | 4.2.1 / 1.13.2 |
| GMP | 6.3.0 for official baseline only |
| Optimization | `-O3 -DNDEBUG`, upstream LTO enabled |
| Parameter set | SQIsign Level I |

Exact repository commits are in `VERSIONS.md`.

## Correctness gates

| Implementation | Test | Result | Approximate elapsed time |
|---|---|---|---:|
| Official `dd133d7…` | `sqisign_test_kat_lvl1` official KAT | PASS | 13.27 s |
| Official `dd133d7…` | Level-I scheme KeyGen→Sign→Verify | PASS | 0.46 s |
| Compact `5b94b09…` | Deterministic Level-I KeyGen→Sign→Verify, seed 1/message 33 bytes | PASS | 2.39 s |
| Packed `find_uv` `254eda3…` | Level-I scheme K/S/V and 10-iteration signature test | PASS | not used as a benchmark |
| Baseline vs packed | 12 deterministic full transcript comparisons (PK, SK, signed message) | byte-identical | 12/12 PASS |
| C1 typed workspace `e61c1fa…` | clean build, Level-I id2iso/closure/ABI/Arm compile and 12 P1 transcript comparisons | PASS | `scripts/reproduce_c1.sh` |
| D1 compact-index workspace `cf9f6b6…` | all-level id2iso/hypercube, Level-I K/S/V, R32 and ASan gates, Arm ABI/compile, 12 C1 transcript comparisons | PASS | `scripts/reproduce_d1.sh` |
| D2 quotient-recompute workspace `d680188…` | all-level id2iso/hypercube, Level-I K/S/V, ASan+UBSan, RADIX32 runtime, Arm ABI/compile, direct-symbol closure and 12 D1 transcript comparisons | PASS | `scripts/reproduce_d2.sh` |
| D3 two-row workspace `b54922b…` | all-level id2iso/hypercube, Level-I K/S/V, ASan+UBSan, RADIX32 runtime, Arm ABI/compile, 12 transcripts and 12 complete D2/D3 candidate-stream comparisons | PASS | `scripts/reproduce_d3.sh` |
| D4 stack-flattened workspace `a6b0628…` | D3 gates plus static-frame/`-Wvla`/`-Walloca` failure gates, exact 353,008-byte ABI and 12 complete D3/D4 candidate-stream comparisons | PASS | `scripts/reproduce_d4.sh` |
| D5 early-ML2 phase overlay `2771afa…` | all-level R64, L1 RADIX32/sanitizer/strict/profile tests, Arm ABI/compile, canary/zeroization fixtures, and 12 frozen K/S/V transcripts | PASS | aggregate root reproducer pending |
| D6 Clapotis/fixed-degree workspace `15a69ee…` | D5 gates plus same-input legacy/explicit Clapotis comparison, failure nonpublication, explicit-closure symbols and three independent reviews | PASS | API-level checkpoint; aggregate root reproducer pending |
| D7 MLLL workspace `f6f7bf5…` | all-level/two-radix quaternion, L1 sanitizer/strict, twelve transcripts, alias/failure matrix, Arm route and three reviews | PASS | API-level checkpoint; production Sign still selects legacy calls |
| D8 equivalent-ideal workspace `3ea2b47…` | deterministic legacy/explicit RNG/result comparison, both radices, sanitizer/strict, twelve transcripts, Arm route and three reviews | PASS | API-level checkpoint; production KeyGen/Sign still select legacy calls |
| D9 random-ideal workspace `cb04091…` | prime and composite/cofactor exact-representation/post-RNG differential, invalid/NULL publication, both radices, sanitizer/strict, twelve transcripts, Arm route and three reviews | PASS | API-level checkpoint; production KeyGen/Sign still select legacy sampler |
| D10 encoded KeyGen workspace `7b549db…` | 12-seed exact encoded key/post-RNG differential, both radices, ASan+UBSan, effective strict aliasing, 19-TU selected KeyGen/signer Arm compile, two exact stack profiles and three reviews | PASS | operation-owned explicit KeyGen route; linked allocator/VLA and target-fit gates remain |
| D11a specialized KeyGen closure `78db285…` | exact object/archive/member/symbol/argv manifests, non-LTO/ThinLTO, effective strict, fatal ASan+UBSan, 45-object Arm closure, ABI and three reviews | PASS | allocator/GMP/legacy-free selected closure; eight dynamic frames and final Pico link remain |
| D11b HNF workspace `9934481…` | normal/low-memory HNF equivalence, invalid/nonpublication/canary/clear fixtures, normal and low-memory ASan+UBSan, exact 45-object Arm closure and three reviews | PASS | HNF VLA removed; seven dynamic frames and final Pico link remain |
| D11c theta workspace `1b9888a…` | 12-seed normal/workspace KeyGen differential, bounds/reject/clear fixtures, both sanitizers, strict alias, exact Arm closure/path and three reviews | PASS | theta-chain VLAs removed from selected closure; six dynamic frames and final Pico link remain |
| D11d-1 batched inversion `a8d30fd…` | max-bound/canary/clear fixture, 12-seed differential, both sanitizers, strict alias, exact 45-object Arm closure, two stack profiles and three reviews | PASS | batched-inversion VLA removed; five dynamic frames and final Pico link remain |
| D11d-2 two-power dlog `434e093…` | all-level direct Tate/Weil differential, bounds/canary/clear fixture, 12-seed KeyGen differential, both sanitizers, strict alias and exact 45-object Arm closure | PASS | both dlog VLAs removed; three MP dynamic frames and final Pico link remain |
| D11d-3 fixed-precision MP `f63efb4…` | direct MP differential/reject/nonpublication/canary/clear fixtures, all-level/RADIX32 KeyGen gates, 12-seed encoded differential, both sanitizers, strict alias and exact 45-object Arm closure | PASS | final three selected VLAs removed; selected Arm dynamic-frame count zero; deterministic physical gate closed by the following artifact |
| D12a decoded-key Sign owner `8a0534d…` | 12-seed exact signature/post-RNG differential, R64/R32, Level-I/III/V, fatal ASan+UBSan, effective strict alias, frozen transcripts, Arm and low-memory TU symbol gates | PASS | caller-owned API checkpoint; encoded/linked low-memory Sign closure and target execution remain |
| D13 proposed implementation `71099e0…` | archived official-request 100-vector Level-I differential conformance test through encoded caller-owned K/S/Open/Verify, repeated in two fresh processes and compared with the same-commit ordinary D13 API | 100/100 PASS in each run; all three responses byte-identical | correctness/equivalence gate, not an official-response KAT or a benchmark |
| RP2350 deterministic KeyGen `64bd997…` | clean link/map/ELF policy, host transcript, workspace guards/clear, PSP/MSP watermarks and physical Pico 2 execution | PASS | 493,728 B exclusively reserved, 38,752 B unreserved; one run 2,696.500982 s |
| Baseline and packed | AddressSanitizer-only Level-I K/S/V | PASS | one run each |
| Fixed `d0cb037…` | Level-I KeyGen→Sign→Verify with raised host stack | PASS | 176.78 s |

The fixed implementation crashes under the default 8 MiB process stack during KeyGen; the correctness run used a deliberately raised limit. This is part of the memory result, not a target configuration.

The D13 full-vector gate reads the archived official Level-I request with
SHA-256 `81ff60e3ef698751e5572f0bb7f831f069605229c220ee1cf27a92572d6ebc7e`.
For every vector, both low-memory runs check K/S, exact Open/message recovery,
one deterministic signature-bit corruption rejection, arena guards and
clearing.  The ordinary D13 response and both low-memory responses have
SHA-256 `1d86d15c5d2bfbef99f17634496086a3ab80f0e848d34d3e9409d75f3019dd68`.
This is a same-commit storage-path equivalence oracle.  It intentionally does
not compare against the legacy GMP response, whose deterministic RNG
transcript differs.  The frozen manifest is
`results/host/d13-lowmem-kat-2026-09-03/manifest.json`.

## Host performance smoke results

The upstream compact benchmark was run 20 times. Its output provides per-operation minimum, median and maximum, but not mean or standard deviation.

| Operation | Minimum | Median | Maximum | Mean | Standard deviation |
|---|---:|---:|---:|---:|---:|
| Compact KeyGen | 101.681 ms | 214.188 ms | 710.337 ms | NR | NR |
| Compact Sign | 330.111 ms | 570.063 ms | 831.380 ms | NR | NR |
| Compact Verify | 2.941 ms | 2.970 ms | 2.995 ms | NR | NR |

A separate one-run compact smoke produced K 410.601 ms, S 410.802 ms and V 2.928 ms. The variation in randomized K/S confirms that one-run values are unsuitable for claims.

The fixed implementation’s raised-stack benchmark produced K 38.0745 s, S 88.0841 s and V 2.976 ms in one run. Total elapsed time was about 126 s. These values only demonstrate the performance cost of the direct global-width arithmetic on this host.

Host maximum resident-set samples were 3,571,712 bytes (official), 8,503,296 bytes (compact), and 25,968,640 bytes (fixed). RSS includes executable mappings, runtime/library pages and allocator effects; it is recorded for diagnostics and is **not** used as an SRAM measurement.

Hardware-cycle access through macOS `kpc` was unavailable without privileged support, so phase-1 host cycle columns remain unreported rather than estimated.

## Fixed `ibz` versus mini-GMP reviewer comparison

Official v2 commit `dd133d7…` was rebuilt from the same source with system GMP
and bundled mini-GMP. Both native builds and mini-GMP with the experiment's
bounded secure pool pass all 100 Level-I KAT vectors. Thirty paired rows over
15 KAT inputs, with alternating execution order, give these ratios of medians:

| B/A | KeyGen | Sign | Verify |
|---|---:|---:|---:|
| native mini-GMP / native system GMP | 1.3621 | 1.4497 | 1.0019 |
| first-fit secure pool / tracked mini-GMP | 4.9441 | 4.2316 | 1.0019 |

The pool row measures this allocator and clear policy, not mini-GMP alone.
The Verify negative control performs no integer allocation. These are host
wall-time smokes, not target cycles or equivalence tests.

Over KAT counts 0–99, mini-GMP Sign peaks at 67,168 requested limb bytes and
4,830 live blocks. The Release first-fit implementation requires a
compile-time 317,696-byte backing array; 317,680 bytes fails by exhaustion.
Tracking-mode “physical” bytes include only the experiment header, not libc
allocator metadata or RSS; pool high water includes its own headers and
fragmentation.
This is an LP64 allocator/corpus result and not an all-input bound, intrinsic
mini-GMP minimum or full RP2350 SRAM peak. The same full pool KAT performs
85,520,200 allocate hooks, 4,962,531 realloc hooks, 2,146,396 internal
relocation-block acquisitions and 5,124,448,176 logical clear bytes. A compact
size-class/region allocator remains a valid alternative research branch.

The five same-vector primitive rows prevent an overbroad speed claim:

| Primitive | mini-GMP / fixed median |
|---|---:|
| 768x768-bit multiply | 1.7089 |
| 1536/768-bit division | 1.1775 |
| 1536-bit GCD | 0.4010 |
| inverse modulo `2^521-1` | 0.5785 |
| 1536-bit floor square root | 1.1860 |

Arm GNU 15.2.1 complete-source object payload is 7,962 versus 18,566 text
bytes under `-Os/soft`, and 17,539 versus 45,856 under the Pico Release-like
profile, for fixed versus mini-GMP core+helpers+official wrapper. This is
pre-GC object payload, not linked firmware flash. Full methods, raw rows and
claim boundaries are in `experiments/mini-gmp/README.md`.

## First transformation: packed bounded vectors

The isolated experiment is compact commit `254eda3…` based on `5b94b09…`. It retains the same algorithms, full-width norms/quotients and five allocator calls; only resident candidate coordinates and their sort-record copies use the proven `int8_t[4]` representation.

### Exact allocation delta

| Live `find_uv` block | Baseline | Packed experiment | Delta |
|---|---:|---:|---:|
| Candidate vectors | 3,773,952 B | 17,472 B | −3,756,480 B |
| Norms | 943,488 B | 943,488 B | 0 |
| Quotients | 943,488 B | 943,488 B | 0 |
| Row counts | 28 B | 28 B | 0 |
| Sort records | 678,912 B | 139,776 B | −539,136 B |
| **Five-block total** | **6,339,868 B** | **2,044,252 B** | **−4,295,616 B (−67.7556%)** |

The block sizes were checked both from frozen counts/types and from actual `malloc_history -allEvents` allocation events on the `find_uv` caller chain. This is an intermediate host allocation measurement, not total firmware SRAM. At 2,044,252 bytes it still exceeds all RP2350 SRAM by 1,511,772 bytes.

### Deterministic paired wall time

Twenty one-operation processes per implementation were alternated in order. `/usr/bin/time -p` has 10 ms resolution here, so small differences must be treated as noise. Each operation resets the same deterministic test RNG state.

| Operation/build | Minimum | Median | Mean | Standard deviation | Maximum | Mean ratio packed/base |
|---|---:|---:|---:|---:|---:|---:|
| KeyGen baseline | 0.180 s | 0.180 s | 0.181 s | 0.002 s | 0.190 s | — |
| KeyGen packed | 0.180 s | 0.180 s | 0.180 s | 0.000 s at timer resolution | 0.180 s | 0.997 |
| Sign baseline | 0.440 s | 0.450 s | 0.449 s | 0.006 s | 0.460 s | — |
| Sign packed | 0.440 s | 0.445 s | 0.446 s | 0.007 s | 0.460 s | 0.993 |

No slowdown is resolved by this host experiment. The final Mach-O `__text` section changed from 428,412 to 428,148 bytes (−264 bytes); mutable sections were unchanged. These host/LTO code-size numbers are useful only for the isolated delta and do not predict Cortex-M firmware size.

## C1 typed-workspace ownership checkpoint

C1 compares clean, identically configured Level-I/RADIX64 host builds at P1/correctness base `ee982a2c00cc6b867c038d62de0c51db3e0ec03d` and candidate `e61c1fa8fb4898fb606dac807727f97655254739`. The exact typed workspace is 2,044,256 bytes: 2,044,252 component bytes plus four bytes of ABI padding. This is **+4 bytes**, not a reduction. Current protocol K/S uses the one-allocation-per-`find_uv`-invocation compatibility wrapper; the explicit caller-owned path is covered separately by id2iso and closure gates.

The wall-time comparison alternates implementation order for 20 samples, runs five identical operations per fresh process, divides by five, and uses two warmups per binary. Every operation resets the same deterministic test seed, so this measures one repeated implementation path rather than retry/runtime distributions.

| Operation/build | Minimum | Median | Mean | Population std. dev. | Maximum |
|---|---:|---:|---:|---:|---:|
| KeyGen P1 base | 93.677 ms | 93.980 ms | 94.044 ms | 0.250 ms | 94.599 ms |
| KeyGen C1 wrapper | 95.903 ms | 96.252 ms | 96.306 ms | 0.260 ms | 97.020 ms |
| Sign P1 base | 225.901 ms | 226.442 ms | 226.591 ms | 0.468 ms | 227.462 ms |
| Sign C1 wrapper | 225.821 ms | 226.864 ms | 227.040 ms | 0.615 ms | 228.338 ms |

The paired median C1/base ratio is 1.02434 for KeyGen (+2.285 ms paired median) and 1.00211 for Sign (+0.478 ms paired median). This small deterministic host delta includes workspace entry/exit clearing and the compatibility adapter; it is not a Pico performance result.

With the same links, Mach-O `__text` changes from 429,464 to 429,792 bytes (+328), `__data` from 56 to 64 bytes (+8), and `__bss`/`__common` are unchanged. The matching Cortex-M33/RADIX32 `-Os`/soft-float `.su` row is `15,696 dynamic` for `find_uv_with_workspace`, but that value is only the fixed component: four VLAs add at most 77,168 bytes, giving a 92,864-byte maximum own frame before callees in this diagnostic configuration. An unlinked C1 source probe under the current Pico Release CPU/ABI/`-O3` flags gives 93,584 bytes. The wrapper is 64 and 72 bytes respectively. Neither probe is a complete linked call-chain/exception stack bound. Host timing and code-size raw metadata, including binary SHA-256 values and flags, are in `results/host/c1-{keypair,sign}-runtime.json` and `results/host/c1-code-size.json`.

## D1 compact-index sorting checkpoint

D1 compares the C1 typed reservation with Compact commit `cf9f6b6857996dc98f75117fec94ab8b9f0654f4`. It replaces a 139,776-byte copied sort-record member with a 1,248-byte `uint16_t[624]` permutation. Exact host and Arm ABI probes report 1,905,724 component bytes, four bytes of tail padding and `sizeof(find_uv_workspace_t) = 1,905,728`. Therefore C1→D1 saves **138,528 bytes (6.7764507%)**. The workspace still exceeds all RP2350 SRAM by 1,373,248 bytes.

Twelve deterministic seeds produce byte-identical 1,210-byte PK/SK/signed-message transcripts. Independent audit additionally exhaustively checked 29,524 small heap-order fixtures and all 46,234 permutations through length eight. The explicit dead-stripped project-code closure has no direct allocator/GMP/`qsort` symbol; the full host harness still contains `malloc/free` through the compatibility wrapper.

Thirty alternating paired samples, with five warmups per binary, reset the same deterministic seed before every operation. This is one repeated path rather than a retry-time distribution:

| Operation/build | Minimum | Median | Mean | Population std. dev. | Maximum | Candidate/base median ratio |
|---|---:|---:|---:|---:|---:|---:|
| KeyGen C1 | 99.058 ms | 99.606 ms | 99.583 ms | 0.229 ms | 100.104 ms | — |
| KeyGen D1 | 99.068 ms | 99.549 ms | 99.512 ms | 0.208 ms | 99.923 ms | 0.99943 |
| Sign C1 | 228.393 ms | 229.377 ms | 230.358 ms | 1.703 ms | 233.009 ms | — |
| Sign D1 | 227.821 ms | 229.461 ms | 230.399 ms | 1.791 ms | 233.157 ms | 1.00037 |
| Verify C1 | 2.073 ms | 2.087 ms | 2.090 ms | 0.011 ms | 2.124 ms | — |
| Verify D1 | 2.077 ms | 2.088 ms | 2.089 ms | 0.009 ms | 2.114 ms | 1.00036 |

No D1 slowdown is resolved above measurement noise. This is not a Cortex-M33 performance result. The host Mach-O comparison changes `__text` by +768 bytes and the first `__const` by +32 bytes, with mutable sections unchanged; it includes intervening test hardening and an exported regression fixture and therefore is not an isolated target code-size claim. Raw values and binary hashes are in `results/host/d1-{keypair,sign,verify}-runtime.json` and `results/host/d1-code-size.json`.

Stack accounting is independent of the workspace delta. In the frozen `-Os`/soft diagnostic, `.su` reports a 15,696-byte fixed component and source/assembly account for a 77,168-byte maximum VLA, yielding a **92,864-byte own frame before callees**. Under current Pico Release-like `-O3`/softfp/CMSE flags, an unlinked D1 probe reports 16,408 + 77,168 = **93,576 bytes**. Thus even workspace plus this own frame is 1,999,304 bytes. The latter still excludes caller/callee depth, exception state and link effects, so the final firmware must be rebuilt and remeasured with its exact flags.

An Arm disassembly audit of one observed Sign-to-ML2 path finds 273,200 bytes
of co-live ancestor frames through `quat_ml2` under the frozen `-Os`/soft
diagnostic. The projected aligned D3 workspace plus that partial path would be
549,040 bytes, already 16,560 bytes over SRAM before `quat_ml2` descendants or
platform state. `scripts/measure_d1_signer_stack_path.sh` asserts every frame
in this sum. It is a diagnostic obstruction, not a complete linked stack bound.

## D2 on-demand quotient checkpoint

D2 is the independently audited Compact commit `d6801884d9c052450a7982e3ac69b29dab0f8893`, following the separate fail-closed all-row norm-validation commit `00f42908ce0147019cd2a1bce6444a2241f45506`. It removes `ibz_t quotients[4368]` and computes the same exact quotient only after a candidate pair passes modular inversion. Exact host and Arm ABI probes report 962,236 component bytes, four bytes of tail padding and `sizeof(find_uv_workspace_t) = 962,240`. D1→D2 therefore saves **943,488 workspace bytes (49.5080095%)**.

The local quotient adds 216 bytes to the frozen `-Os` fixed frame: 15,912 fixed + 77,168 VLA = **93,080 bytes own frame before callees**. Consequently the co-live D1→D2 saving is **943,272 bytes**. The current Pico Release-like unlinked probe is 16,624 + 77,168 = **93,792 bytes**. Workspace plus that probe is 1,056,032 bytes, still 523,552 bytes over all SRAM before other frames or platform state.

Thirty alternating samples compare clean D1/D2 Level-I RADIX64 links. Every operation resets the same deterministic seed, so these are paired implementation deltas on one path, not retry distributions or Cortex-M33 performance:

| Operation/build | Minimum | Median | Mean | Population std. dev. | Maximum | D2/D1 median ratio |
|---|---:|---:|---:|---:|---:|---:|
| KeyGen D1 | 97.667 ms | 98.108 ms | 98.125 ms | 0.258 ms | 98.838 ms | — |
| KeyGen D2 | 97.912 ms | 98.202 ms | 98.301 ms | 0.385 ms | 99.776 ms | 1.00096 |
| Sign D1 | 228.941 ms | 229.705 ms | 229.833 ms | 0.579 ms | 231.504 ms | — |
| Sign D2 | 229.556 ms | 230.122 ms | 230.199 ms | 0.442 ms | 231.388 ms | 1.00182 |
| Verify D1 | 3.344 ms | 3.427 ms | 3.437 ms | 0.069 ms | 3.607 ms | — |
| Verify D2 | 3.312 ms | 3.401 ms | 3.418 ms | 0.075 ms | 3.601 ms | 0.99256 |

The frozen deterministic host path shows small positive deltas: KeyGen median +0.0959% and Sign median +0.1817%. Their paired mean changes are +0.176 ms and +0.366 ms respectively; no equivalence margin was preregistered, so these data are not labeled “no slowdown” or “equivalent.” Verify is an unchanged-path negative control and moves −0.7435% at the median. An independent clean audit rerun produced K×0.99780, S×1.00042 and V×0.99445, so this short host smoke does not resolve a reproducible practical slowdown. It does not replace target or retry-distribution measurements. Test-only instrumentation explains why recomputation is inexpensive on these paths without changing the production commit. The fixed 4,368 slots per invocation are D1's capacity; actual D1 precomputation follows the populated row lengths. Across twelve seeds, one-call KeyGen reconstructs 661–748 D1 divisions (median 717.5, mean 711.17, population standard deviation 32.36), versus 1–29 in D2 (median 1.5, mean 5, standard deviation 7.70). Two-call Sign reconstructs 1,381–1,502 D1 divisions (median 1,438, mean 1,432.58, standard deviation 38.63), versus 2–106 in D2 (median 8, mean 18.92, standard deviation 28.40). These samples do not remove the 9,541,896-pair per-invocation theoretical ceiling; maximum-time and target distributions remain required. Raw per-round timings, counters and provenance are in `results/host/d2-*-runtime.json` and `results/host/d2-quotient-divisions.json`.

The clean host link changes Mach-O `__text` by +20 bytes; `__cstring` and measured mutable sections are unchanged. This host-section delta is not an RP2350 code-size result. Raw timing, binary hashes and code-size metadata are in `results/host/d2-{keypair,sign,verify}-runtime.json` and `results/host/d2-code-size.json`.

The observed `-Os` Sign-to-ML2 ancestor partial path increases from 273,200 to 273,416 bytes solely through the local quotient. Combining the later exact D3 reservation with that historical D2 path gives 549,256 bytes, 16,776 bytes over all SRAM before descendants/platform state. This remains a diagnostic obstruction rather than a linked bound.

## D3 two-row streaming checkpoint

D3 is the independently audited Compact commit
`b54922bd2de94b871bf4bd477a11de6e32bd17bf`. It first constructs, sorts and
validates all seven rows without searching. It then repeats row construction
with two fixed slots in the original triangular pair order; diagonal pairs
alias the outer slot and off-diagonal pairs use the second slot. Exact host and
Arm ABI probes report 275,836 component bytes, four bytes of tail padding and
`sizeof(find_uv_workspace_t) = 275,840`. D2→D3 therefore saves **686,400
workspace bytes (71.3335550%)**.

The frozen `-Os` outer frame changes from 15,912 to 15,928 fixed bytes. With
the unchanged 77,168-byte VLA extent, D3's own frame is **93,096 bytes before
callees**; a 32-byte preparation helper is additionally co-live while a row is
generated. The current Pico Release-like unlinked probe is 15,936 + 77,168 =
**93,104 bytes**, with an 80-byte helper under those flags. These are source-TU
diagnostics, not linked target stack bounds.

Test-only instrumentation emits every pair-call header, both complete norm
lists, every visited `(i1,i2)` and every candidate `v` before unchanged
arithmetic. For all twelve frozen seeds, the complete 81,396–120,195-byte D3
trace is byte-identical to D2, as are the final 1,210-byte protocol
transcripts. The fixed trace hashes are checked by
`scripts/test_d3_candidate_trace.sh`. Focused combined ASan+UBSan, Level-I
RADIX32 runtime and eight Cortex-M33 translation-unit builds pass. Independent
review found and required correction of one ISO C inner-array-boundary UB in
the initial two-dimensional workspace init/finalize loop; the frozen commit is
the corrected version.

Thirty alternating samples compare clean D2/D3 Level-I RADIX64 links. Each
operation resets the same deterministic test seed, so this is one fixed path,
not a retry distribution or Cortex-M33 result:

| Operation/build | Minimum | Median | Mean | Population std. dev. | Maximum | D3/D2 median ratio |
|---|---:|---:|---:|---:|---:|---:|
| KeyGen D2 | 97.789 ms | 98.137 ms | 98.154 ms | 0.225 ms | 98.750 ms | — |
| KeyGen D3 | 97.768 ms | 98.084 ms | 98.159 ms | 0.500 ms | 100.658 ms | 0.99946 |
| Sign D2 | 229.184 ms | 229.912 ms | 230.062 ms | 0.787 ms | 232.971 ms | — |
| Sign D3 | 228.636 ms | 229.933 ms | 230.035 ms | 0.679 ms | 232.212 ms | 1.00009 |
| Verify D2 | 3.319 ms | 3.384 ms | 3.389 ms | 0.043 ms | 3.546 ms | — |
| Verify D3 | 3.312 ms | 3.384 ms | 3.384 ms | 0.032 ms | 3.453 ms | 0.99999 |

No equivalence margin was preregistered. The paired mean changes are KeyGen
+0.005 ms, Sign −0.028 ms and Verify −0.005 ms, so these data are reported as
a short host smoke rather than a “no slowdown,” speedup or equivalence claim.
The algorithmic ceiling still grows from seven row generations in D2 to seven
validation plus at most 28 search generations in D3; target and retry-
distribution measurements remain mandatory. The clean host Mach-O `__text`
section grows by 552 bytes and measured mutable sections do not change. This is
not an RP2350 code-size result. Raw rounds, binary identities and section data
are in `results/host/d3-{keypair,sign,verify}-runtime.json` and
`results/host/d3-code-size.json`.

The actual D3 `-Os` observed Sign-to-ML2 ancestor partial path is 273,432
bytes. Workspace plus that path is **549,272 bytes**, 16,792 bytes over all
RP2350 SRAM before `quat_ml2` descendants, target wrappers, exception state or
platform mutable data. The separate Pico Release-like unlinked path is 273,496
bytes, giving **549,336 bytes** with the workspace and a 16,856-byte excess.
This directly makes VLA/ML2 flattening the next gate.
Current end-to-end KeyGen/Sign still allocate once per `find_uv` invocation
through the compatibility wrapper—one observed invocation in KeyGen and two
sequential invocations in Sign—so D3 is not yet a heap-free signer.

## D4 typed stack-flattening checkpoint

D4 is Compact commit
`a6b06287706a97999d077d055818c2b5612a8704`. It moves the seven adjusted
norms, seven Gram matrices, seven reduced matrices and seven ideals from four
VLA families into `find_uv_lattice_state_t`, ahead of the unchanged D3
candidate subworkspace. Host and Cortex-M33/RADIX32 probes agree on this ABI:

| D4 object/account | Bytes |
|---|---:|
| typed lattice state | 77,168 |
| D3 candidate subworkspace | 275,840 |
| **`sizeof(find_uv_workspace_t)`** | **353,008** |
| component bytes / tail padding | 353,004 / 4 |

The frozen `-Os` frame is now `15,928 static` with no VLA; the Pico
Release-like frame is `15,936 static`. The corresponding observed ancestor
paths are 196,264 and 196,328 bytes. Consequently:

```text
D3 -Os:  275,840 + 273,432 = 549,272 B
D4 -Os:  353,008 + 196,264 = 549,272 B
D3 Pico: 275,840 + 273,496 = 549,336 B
D4 Pico: 353,008 + 196,328 = 549,336 B
```

D4 therefore saves **0 bytes** in these co-live decompositions. It is accepted
because it eliminates runtime-sized stack ownership and enables a later typed
union/phase overlay, not because stack-to-workspace relocation reduces total
SRAM. These individual-TU paths still omit descendants, target wrappers,
exception state, mutable globals and final-link effects.

Thirty stored alternating D3/D4 host samples are retained as a measurement
artifact:

| Operation/build | Minimum | Median | Mean | Population std. dev. | Maximum | D4/D3 median ratio |
|---|---:|---:|---:|---:|---:|---:|
| KeyGen D3 | 98.577 ms | 99.080 ms | 99.292 ms | 0.868 ms | 102.390 ms | — |
| KeyGen D4 | 96.239 ms | 96.824 ms | 97.094 ms | 1.053 ms | 102.288 ms | 0.97722 |
| Sign D3 | 231.239 ms | 232.224 ms | 232.290 ms | 0.710 ms | 234.310 ms | — |
| Sign D4 | 230.898 ms | 231.858 ms | 231.994 ms | 0.712 ms | 234.327 ms | 0.99842 |
| Verify D3 | 3.330 ms | 3.395 ms | 3.408 ms | 0.049 ms | 3.527 ms | — |
| Verify D4 | 3.317 ms | 3.394 ms | 3.399 ms | 0.048 ms | 3.544 ms | 0.99964 |

The apparent stored KeyGen improvement is **not reproducible**: an independent
clean rerun and three additional 30-pair blocks with the same binaries clustered
near unity while their absolute medians drifted. Those audit reruns were not
retained as frozen raw artifacts, so no exact secondary ratio is claimed. The
effect is not resolved beyond scheduler/frequency/thermal noise and is not a
speedup or equivalence result. The
clean host Mach-O `__text` section decreases by 500 bytes and measured mutable
sections are unchanged; that is not an RP2350 code-size result. Raw samples,
binary identities and build flags are in
`results/host/d4-{keypair,sign,verify}-runtime.json` and
`results/host/d4-code-size.json`.

## D5 early-ML2 phase overlay checkpoint

D5 is Compact commit
`2771afabf54b579b6f05d7440aa6de0a48544779`.  It gives ML2 and its
deterministic retry driver caller-owned typed state and overlays that state with
the later two-row candidate phase.  The Cortex-M33/RADIX32 layout is:

| D5 object/account | Bytes |
|---|---:|
| ML2 core | 77,632 |
| retry permutation + unpublished result | 17,280 |
| **complete ML2 retry state** | **94,912** |
| candidate union member | 275,840 |
| inactive union capacity during ML2 | 180,928 |
| long-lived lattice state | 77,168 |
| **outer `find_uv_workspace_t`** | **353,008** |

The inactive 180,928 bytes are capacity within the union, not padding and not
an additional saving.  The reservation is unchanged from D4.  What changes is
the co-live stack path: all three early `find_uv` ML2 routes use the arena
instead of the former automatic core/retry state.  The largest frozen branch
is the alternate-ideal route:

```text
D4 own-frame endpoint:              203,384 B
D5 own-frame endpoint:              107,264 B
saved:                               96,120 B

D4 known-descendant adjusted:       205,032 B
D5 known-descendant adjusted:       109,816 B
D4 + workspace:                     558,040 B
D5 + workspace:                     462,824 B
diagnostic reduction:                95,216 B
nominal RP2350 margin:               69,656 B
```

The descendant adjustment includes the out-of-line insertion, Gram refresh
and one `ibz_mul`; it still omits deeper descendants, mutable statics, MSP/ISR
state, allocator metadata and final-link effects.  Moreover, current complete
KeyGen/Sign still use the per-`find_uv` allocation wrapper, and later legacy
ML2/MLLL paths can coexist with a whole-operation reservation.  No target-fit,
heap-free, speed or total-peak claim follows from this checkpoint.  The fixed
clears add substantial SRAM writes and will be timed only after the complete
low-memory path is integrated.

## D6 Clapotis/fixed-degree workspace checkpoint

D6 is Compact commit
`15a69ee3a3eecc70f6f04e6e8bff134635a27696`.  It reuses the existing
`phase.ml2` member for the two fixed-degree constructions that follow
`find_uv`, in the same `u` then `v` order.  The outer workspace and all member
offsets are unchanged from D5.  Arm `-Os` individual-TU measurements give:

| Path definition | Legacy | Explicit workspace | Delta |
|---|---:|---:|---:|
| fixed-degree ancestry through ML2 own frame | 180,992 B | 85,936 B | −95,056 B |
| plus known insertion/Gram/`ibz_mul` descendants | 183,544 B | 88,488 B | −95,056 B |
| explicit path plus 353,008-byte arena | — | 441,496 B | — |

The Pico Release-like explicit path is 88,584 bytes after the same descendant
adjustment.  These paths omit deeper integer descendants, mutable statics,
MSP/exception state and link/LTO effects.  Three independent reviews approved
the C union lifetime, branch dominance, output publication, namespace and
clearing behavior; all-level RADIX64, Level-I RADIX32/sanitizer/strict, Arm
compile and twelve transcript gates pass.

Production `protocols_sign` and `protocols_keygen` still call the legacy
arbitrary-isogeny API.  Therefore the current production path changes by only
small wrapper/code-layout effects, not by the 95,056-byte projected API delta.
A short paired host Sign smoke found no resolved change, as expected, but is
not retained as a performance artifact.  Repeated fixed-size workspace clears
also require target timing before the explicit path can be assigned a speed
cost.  D6 is not a heap-free, linked-SRAM or RP2350-fit measurement.

## D7 MLLL product/intersection workspace checkpoint

D7 is Compact commit
`f6f7bf559cfab95fa1223fa4f928792ff79a7b76`. The retry member and
outer arena remain 94,912 and 353,008 bytes. Under the frozen Arm `-Os`
diagnostic, the fair maximum-known path includes the normal-LLL branch that
dominates after the ML2 stack is removed:

| Projected upper-caller route | D6 legacy | D7 explicit workspace | Delta |
|---|---:|---:|---:|
| compact lattice product | 165,152 B | 74,584 B | −90,568 B |
| compact ideal intersection | 162,784 B | 72,224 B | −90,560 B |
| product plus one arena | 518,160 B | 427,592 B | −90,568 B |
| ideal intersection plus one arena | 515,792 B | 425,232 B | −90,560 B |

The explicit APIs are not yet called by production Sign, so no end-to-end
timing sample is assigned to D7. Fixed-size entry and retry clearing adds
substantial writes per invocation depending on retry count; its target cost
must be measured only after top-level integration. The table is not a linked
peak or an RP2350 fit result.

## D8 equivalent-ideal workspace checkpoint

D8 is Compact commit
`3ea2b47417a5a6dc0b680bc60625c2761123314b`.  Its fair maximum-known
comparison includes the denominator-reduction path which becomes dominant
after ML2 flattening:

| Diagnostic profile | D7 legacy maximum | D8 explicit maximum | Delta |
|---|---:|---:|---:|
| Arm `-Os`/soft | 125,560 B | 34,624 B | −90,936 B |
| Pico-like `-O3`/softfp | 125,608 B | 34,904 B | −90,704 B |

With one 353,008-byte arena and the current operation prefixes, the `-Os`
diagnostic projects 422,280 bytes for Sign and 393,896 bytes for KeyGen.
Neither includes final linked mutable state, MSP/ISR or all descendants.
Production callers still select the legacy entry, so no end-to-end timing or
current-peak delta is assigned.  Entry/retry clearing adds at least 379,648
logical bytes of fixed writes per explicit invocation beyond inherited core
clears; target cycles remain unmeasured.

## D9 random-ideal workspace checkpoint

D9 is Compact commit
`cb040911ef14dd56d2c647834d884919de029ace`. The fair comparison uses frozen
D8 rather than the new 16-byte compatibility wrapper, and follows the largest
real post-flattening descendant:

| Diagnostic profile | D8 legacy maximum | D9 explicit maximum | Delta | Arena + explicit path |
|---|---:|---:|---:|---:|
| Arm `-Os`/soft | 122,312 B | 37,744 B | −84,568 B | 390,752 B |
| Pico-like `-O3`/softfp | 122,568 B | 38,008 B | −84,560 B | 391,016 B |

The `-Os` inverse calls a 456-byte coefficient helper and then the 1,384-byte
multiply. Pico-like `-O3` inlines that helper into the 7,064-byte inverse; its
largest real child is instead a 240-byte determinant followed by the same
multiply. The reproduction gate asserts both the absent helper edge and the
positive determinant chain, preventing `.su` double-counting.

Fixed CTR-DRBG tests cover prime and composite/cofactor success, exact ideal
representation, mathematical equality, norm self-consistency, post-call RNG,
workspace guards/full clearing, invalid input and valid-input NULL-workspace
nonpublication. No performance ratio is reported. Depending on retry attempts,
the explicit path performs 534,912–1,000,704 logical secure-clear bytes per
successful invocation, and is not yet called by public KeyGen/Sign. These are
API-level individual-TU measurements rather than a production peak or a target
memory--stack--code--time trade-off result.

## D10 full encoded KeyGen workspace checkpoint

D10 is Compact commit
`7b549db43145e112366fba4509a1085b3400f52a`. It introduces a separate
`sqisign_keypair_with_workspace` entry while leaving the legacy encoded entry
byte-identical. One 353,008-byte object owns the already audited D9, D8 and D6
phases sequentially. Twelve deterministic seeds preserve exact encoded PK/SK
and the post-call RNG stream; the public NULL-workspace case leaves both output
buffers and RNG untouched.

| Diagnostic profile | Early `find_uv` path | + arena | Nominal raw-SRAM margin | Fixed-degree + arena | Encoding + arena |
|---|---:|---:|---:|---:|---:|
| Arm `-Os`/soft | 81,384 B | **434,392 B** | 98,088 B | 413,120 B | 366,624 B |
| Pico-like `-O3`/softfp | 81,456 B | **434,464 B** | 98,016 B | 413,192 B | 366,640 B |

The 353,008-byte ABI and every frame/call edge in these sums are cross-compiled
with Arm GNU 15.2.1. The largest known branch is the early alternate-ideal
route. The public-key encoder's audited crypto subtree is 560/680 bytes; even
when nested under secret-key serialization, its operation path is only
7,696/7,816 bytes and is below the 13,616/13,632-byte secret-generator path.
These are individual-TU sums rather than linked upper bounds: output
buffers, final data/BSS, SDK/runtime, MSP/ISR, untracked descendants and the
remaining VLAs are not included. The explicit non-NULL runtime route does not
execute allocation, but the linked compatibility closure still imports
`malloc/free` from same-object legacy branches.

The host benchmark uses 15 deterministic seeds, each measured in both AB and
BA order (30 CSV rows). Statistics below describe paired explicit-minus-legacy
rows; the maximum number of independent seed units is 15.

| Run | CSV SHA-256 | Median paired ratio | Difference min | median | mean | sample SD | max |
|---|---|---:|---:|---:|---:|---:|---:|
| frozen | `f6ea7bb6…a1254` | 0.998012779 | −2.707 ms | −0.3725 ms | −0.3701 ms | 0.7722 ms | 1.608 ms |
| clean rerun | `99c9581d…2e19` | 0.998830461 | −3.671 ms | −0.2440 ms | −0.2393 ms | 1.0336 ms | 2.254 ms |

The two short runs do not resolve a slowdown; equally, no equivalence margin
or target experiment was predeclared, so they do not establish equivalence or
a speedup. Clearing the outer arena on entry and exit writes 706,016 logical
bytes before nested phase/retry clearing. Cortex-M33 cycles, wall time and
energy remain unmeasured, so D10 does not yet establish a target
memory--stack--code--time trade-off point.

## D11a allocator-free selected KeyGen closure

D11a is Compact commit
`78db2858780850f06b965ff87653795b529d3299`.  It adds a dedicated
Level-I/RADIX32 low-memory build in which compile-time non-NULL dispatch and an
exact source manifest physically exclude allocator, GMP, stdio and curated
legacy large-stack/fallback code from the selected encoded-KeyGen closure.
The normal multi-level build remains unchanged.

The frozen artifact gate checks complete compile command hashes, exact object,
archive and member manifests, all symbols and relocations, writable sections,
Arm ELF format, direct HNF/ML2 edges and every expected `.su` record.  Fresh
non-LTO, ThinLTO, effective-strict-alias, fatal ASan+UBSan and Cortex-M33 builds
pass.  Their selected closures are:

| Profile | Objects | Archives | Archive members |
|---|---:|---:|---:|
| host production | 44 | 10 | 44 |
| host deterministic test | 46 | 10 | 45 |
| Arm deterministic test | 45 | 10 | 45 |

The arena ABI remains 353,008 bytes with alignment eight.  The deterministic
Arm object set has 56 bytes of writable section payload: 52 bytes belong to
the test-only CTR-DRBG and four bytes to the secure-clear function pointer.
`CONNECTING_IDEALS` is a 27,272-byte read-only symbol in `.rodata`; final Pico
linker/XIP placement is not yet established.

The selected Arm closure still contains eight dynamic frames.  The largest
currently reconstructed normal KeyGen route is the HNF branch:

| HNF-inclusive component | Bytes |
|---|---:|
| ancestor frames through lattice multiplication | 78,672 |
| HNF fixed component | 6,792 |
| HNF VLA (`16 * sizeof(ibz_vec_4_t)`) | 13,824 |
| deepest known HNF descendants | 6,520 |
| **project-known PSP partial path** | **105,808** |
| caller arena | 353,008 |
| **diagnostic co-live sum** | **458,816** |
| nominal raw-SRAM remainder | 73,664 |

This is not a linked upper bound: final data/BSS alignment, output buffers,
production RNG, SDK/libc/libgcc descendants, MSP/interrupt state, exception
entry on PSP and the other dynamic paths remain outside it.  D11a has no new
runtime benchmark and makes no speed, target-fit or side-channel claim.

## D11b HNF workspace checkpoint

D11b is Compact commit
`99344812b28e4a57ba0c876a27ecfa7372363f9a`. It overlays a fixed
`16 * 4 * sizeof(ibz_t) = 13,824`-byte HNF scratch object with the existing
94,912-byte ML2 retry member. The 275,840-byte candidate member still dominates
the phase union, so the outer arena remains 353,008 bytes with alignment eight.
The low-memory closure physically excludes the compatibility VLA entry; the
normal build retains it and independently compares 4-, 8- and 16-generator
outputs against the workspace API.

| HNF-inclusive component | D11a | D11b |
|---|---:|---:|
| ancestor frames through lattice multiplication | 78,672 | 78,680 |
| HNF workspace wrapper | — | 32 |
| HNF core implementation | 6,792 | 6,792 |
| HNF VLA | 13,824 | 0 |
| deepest known HNF descendants | 6,520 | 6,520 |
| **project-known PSP partial path** | **105,808** | **92,024** |
| caller arena | 353,008 | 353,008 |
| **diagnostic co-live sum** | **458,816** | **445,032** |
| nominal raw-SRAM remainder | 73,664 | 87,448 |

The net reduction is 13,784 bytes rather than 13,824: the lattice frame grows
by eight bytes and the new HNF wrapper contributes 32 co-live bytes. The fixed
deterministic KeyGen fixture reaches the HNF workspace six times. Each call
clears 13,824 logical bytes before and after use, for 165,888 added logical
clear bytes on that fixture; this is not a general call-count bound or a target
bus/cycle measurement.

The pinned normal/RADIX64 host binary SHA-256 is
`ebbe0d8e1a521419d219d14d3838c3d0d3327bca86e4236cbd2ff7b8fb8e3ebf`.
Two 30-row AB/BA artifacts contain 15 independent seeds each:

| Run | CSV SHA-256 | explicit/legacy ratio of medians | Paired difference median |
|---|---|---:|---:|
| first | `d7833844…d7d6` | 0.998439515 | −0.6305 ms |
| rerun | `9e9c4b31…fb4` | 0.998935711 | −0.7025 ms |

The comparison includes the complete D5–D11b explicit route rather than the
HNF change alone. Earlier unpinned runs produced tiny deltas of the opposite
sign. These data therefore resolve neither a slowdown nor a speedup and are
not an equivalence test or Cortex-M33 result. The HNF/ML2 route is variable
time and can depend on secret-derived sizes; no side-channel claim is made.

## D11c theta workspace checkpoint

D11c is Compact commit
`1b9888a765b2674a78232595d08eadc24a5c2a94`. It replaces the selected
low-memory theta-chain wrapper's six VLA objects with a 13,844-byte typed
workspace. That object is smaller than the existing 94,912-byte fixed-degree
ML2 member and shares the same union address, so the outer arena remains
353,008 bytes with alignment eight.

| Pico-like theta component | D11b | D11c |
|---|---:|---:|
| theta wrapper fixed part | 32 | 32 |
| legacy fixed core plus bounded VLA / explicit core plus helper | 27,984 | 14,192 |
| deepest known theta descendants | 2,040 | 2,040 |
| **fixed-u/v theta partial path with ancestors** | **68,584** | **54,792** |
| caller arena | 353,008 | 353,008 |
| **arena plus theta branch** | **421,592** | **407,800** |

The theta-local reduction is 13,792 bytes, and the selected-object dynamic
inventory falls from seven to six. This is not a global peak reduction: the
HNF path remains 92,024 bytes and the global arena-plus-known-path diagnostic
remains 445,032 bytes. The normal compatibility build intentionally retains
its bounded VLA wrapper. Three successful theta calls per deterministic
Clapotis invocation add 83,064 logical clear bytes; that count is not a cycle
or retry-distribution measurement.

The pinned normal/RADIX64 host binary SHA-256 is
`36d361b212b2c18db0c9919d79e5bbe38f86d4059e07cc90b008043401947cfc`.
Each 30-row artifact contains 15 deterministic seeds in both AB/BA orders:

| Run | CSV SHA-256 | explicit/legacy ratio of medians | Paired difference median |
|---|---|---:|---:|
| first | `b9cfcc91…ece81` | 0.998642360 | −0.1615 ms |
| rerun | `06131d48…a4a` | 0.999015373 | +0.0855 ms |

The paired median changes sign. These are cumulative legacy-vs-explicit host
smokes, not a D11c-only timing attribution, equivalence test, speedup claim or
Cortex-M33 result.

## D11d-1 batched-inversion workspace checkpoint

D11d-1 is Compact commit
`a8d30fd64985935ed7d9b1b92fe1ae90ba4a39e3`. It replaces the selected
`fp2_batched_inv` pair of length-11 VLAs with a 1,584-byte typed object. The
object is overlaid with completed `find_uv` storage at the operation level and
at offset 13,844 inside theta scratch. The 94,912-byte fixed-degree member still
dominates, so the 353,008-byte arena does not grow.

| Diagnostic component | `-Os`/soft | Pico-like |
|---|---:|---:|
| fixed-u/v theta path | 54,464 B | 54,816 B |
| arena plus theta path | 407,472 B | 407,824 B |
| known HNF path | 91,448 B | 92,032 B |
| arena plus known HNF path | 444,456 B | 445,040 B |
| nominal raw difference from all SRAM | 88,024 B | 87,440 B |

The selected dynamic-frame inventory falls from six to five. The removed
1,584-byte VLA is real, but HNF remains the known global maximum; Pico-like
Clapotis code generation adds eight bytes, so D11d-1 is not reported as an
operation-peak reduction. The fixed successful KeyGen trace adds 72,864
logical clear bytes relative to D11c. That trace count is not a general retry
bound or a Cortex-M33 cycle/bus measurement.

The pinned normal/RADIX64 host binary SHA-256 is
`fcf2931f4c936bf66a6bb09017c0bc72b01d5c3220441b703e7bc542fecd9ea4`.
Each 30-row artifact contains 15 deterministic seeds in both AB/BA orders:

| Run | CSV SHA-256 | explicit/legacy ratio of medians | Paired difference median |
|---|---|---:|---:|
| first | `4cf8f97a…167d1` | 1.001836082 | −0.1165 ms |
| rerun | `5b71b5b5…fbe12` | 0.998576818 | +0.0530 ms |

Both the ratio and paired-difference direction change. These are cumulative
legacy-vs-explicit host smokes, not a D11d-1-only timing attribution,
equivalence test, speedup claim or target result.

## D11d-2 two-power-discrete-log workspace checkpoint

D11d-2 is Compact commit
`434e093bc5e7e4157b77176a7d762853f50f39b0`. It replaces both selected
Tate/Weil `fp2_t[8]` power-table VLAs with a 1,152-byte typed object overlaid
with the existing 1,584-byte batched-inversion member. The enclosing
353,008-byte arena is unchanged.

| Diagnostic component | Pico-like |
|---|---:|
| D11d-1→D11d-2 Tate recursive inner branch | 5,800→4,664 B |
| D11d-1→D11d-2 Weil recursive inner branch | 6,504→5,368 B |
| D11d-1→D11d-2 KeyGen ancestor + Tate recursive branch | 12,312→11,176 B |
| known HNF path | 92,032 B |
| arena plus known HNF path | 445,040 B |
| nominal raw difference from all SRAM | 87,440 B |

The selected dynamic-frame inventory falls from five to three. The removed
payload is real, but HNF remains the known global maximum, so D11d-2 is not an
operation-peak reduction. Four explicit dlog calls in the fixed successful
KeyGen add 9,216 logical clear bytes relative to D11d-1; this is not a general
retry bound or Cortex-M33 bus/cycle measurement.

The pinned normal/RADIX64 host binary SHA-256 is
`9a29eeb18751bca3c3d114026093f997af92c7d17cd93acb600f391a435c39b5`.
Each 30-row artifact contains 15 deterministic seeds in both AB/BA orders:

| Run | CSV SHA-256 | explicit/legacy ratio of medians | Paired difference median |
|---|---|---:|---:|
| first | `2ccc737a…7252c` | 0.999255688 | −0.0195 ms |
| rerun | `6022c836…d4dd69` | 0.998576164 | −0.2495 ms |

Both are tiny cumulative explicit/legacy host deltas. They do not isolate
D11d-2, prove equivalence or speedup, or predict Cortex-M33 timing.

## D11d-3 fixed-precision MP workspace checkpoint

D11d-3 is Compact commit
`f63efb4154ffacbd1e5a6cc6ab0229512bf8d2ce`. It replaces the final three
selected `mp_mul`, `mp_inv_2e` and matrix-inversion VLAs with nested
144/504/936-byte typed workspaces. The 936-byte maximum is overlaid with the
existing 1,584-byte pairing workspace, so the enclosing 353,008-byte arena is
unchanged.

| Diagnostic component | `-Os`/soft | Pico-like |
|---|---:|---:|
| deepest MP workspace subpath | 192 B | 224 B |
| change-of-basis inversion subpath | 416 B | 448 B |
| known HNF path | 91,448 B | 92,032 B |
| arena plus known HNF path | 444,456 B | 445,040 B |
| nominal raw difference from all SRAM | 88,024 B | 87,440 B |

The selected Arm dynamic-frame inventory falls from three to zero and the
complete selected closure compiles with `-Wvla -Walloca -Werror`. The MP
matrix-inversion route is not reached by the current KeyGen transcript, so
this checkpoint does not lower the known global path or provide a new timing
attribution. The normal/RADIX64 benchmark binary is byte-identical to D11d-2
(`9a29eeb1…`); the two D11d-2 CSVs therefore remain evidence for that exact
binary only. They do not isolate D11d-3, establish equivalence or predict
Cortex-M33 time.

## D12a decoded-key Sign workspace checkpoint

D12a is Compact commit
`8a0534d0fc4f2d8f0f355774d111e26b3ca19035`. It routes the decoded-key
`protocols_sign` operation through one caller-owned 353,008-byte union and the
existing explicit random-ideal, equivalent-ideal, arbitrary-isogeny, MLLL,
fixed-degree, theta and dlog APIs. The challenge-ideal conversion now also has
an explicit retry-workspace entry, closing a previously hidden ML2 fallback.

The frozen Pico-like individual-object path is:

| Diagnostic component | Bytes |
|---|---:|
| Sign workspace wrapper + protocol frame | 28,736 |
| arbitrary-isogeny + Clapotis + `find_uv` | 37,456 |
| ideal/lattice HNF ancestors | 42,056 |
| known HNF descendants through unsigned division | 6,592 |
| **known Sign HNF path** | **114,840** |
| caller arena | 353,008 |
| **arena plus known path** | **467,848** |
| nominal raw difference from 520 KiB | 64,632 |

The sum excludes final-link `.data/.bss`, MSP/ISR and exception state,
SDK/libc, other operation state and untracked descendants. It is not a target
stack bound or fit result. The normal Pico-like `sign.c` and `id2iso.c`
objects grow by 736 and 80 bytes respectively (`+816 B` total); final linked
code size is not measured and neither changed object adds mutable section
payload in this comparison.

The normal/RADIX64 benchmark binary SHA-256 is
`a50dd41a30412e96c33818cd903435030aced27e7f86188c2db1e14aa3102f35`.
Each raw artifact contains 15 deterministic seeds in both AB/BA orders, with
key decoding and verification outside the timed Sign interval:

| Run | CSV SHA-256 | explicit/legacy ratio of medians | Paired difference median |
|---|---|---:|---:|
| first | `4414b9f2…61cfaa` | 1.000625999 | −0.0790 ms |
| rerun | `de880a51…6dcfd` | 0.997403404 | −0.0515 ms |

The ratios move across one and the paired means also change sign. These are
descriptive cumulative explicit-API host smokes, not an equivalence test,
D12a-only clearing cost, Cortex-M33 result or speedup claim. The encoded
public Sign entry and secret-key decoder are still legacy-only, and
`ec_eval_even_strategy` retains a VLA, so no current linked Sign closure or
physical target measurement exists.

## Physical RP2350 deterministic KeyGen

The exact D11d-3 Level-I/RADIX32 selected closure was linked at project commit
`64bd99771d34f22f8863c8360ab6d2716a045b2d` and executed on a Pico 2 / RP2350
A2 at 150 MHz.  USB reporting is outside the measured operation interval.

| Measurement | Result |
|---|---:|
| KeyGen status | PASS |
| operation time | 2,696.500982 s |
| workspace payload / guarded owner | 353,008 / 353,136 B |
| exclusive SRAM reservation | 493,728 B |
| unreserved SRAM | 38,752 B |
| observed PSP extent / reservation | 91,980 / 122,880 B |
| observed conservative MSP upper extent / reservation | 2,308 / 8,192 B |
| heap | 0 B |
| host transcript match | exact |

This is a single deterministic correctness and fit observation.  It is not a
30-pair benchmark, does not estimate retry/runtime distributions, and does not
establish production-RNG performance, worst-case stack, constant time or Sign
fit.  The normalized capture and manifest are frozen under `results/rp2350/`.

## Memory measurements

### Exact source/type measurement

`tools/compact_ops.c` reports frozen Level-I `sizeof` values. The five `find_uv` allocations sum to 6,339,868 bytes. `scripts/generate_memory_trace.py` independently asserts that sum, the P1 sum, the C1 +4-byte relation, and the exact C1→D1, D1→D2, D2→D3 and D3→D4 workspace/frame deltas before emitting the current v0.6 JSON/SVG. D5's independently compiled host/Arm probes additionally freeze the unchanged 353,008-byte outer extent and the 94,912-byte Cortex-M33 ML2 retry member; D6–D11d-3 do not change that outer ABI. D10 adds a production-type Arm `_Static_assert` probe for the enclosing KeyGen owner, D11a repeats the ABI assertions inside its exact selected Arm closure, D11b adds the 13,824-byte HNF/ML2 union-member assertions, D11c adds the 13,844-byte theta/fixed-degree union assertions, D11d-1 adds the 1,584-byte batched-inversion assertions, D11d-2 adds the 1,152-byte dlog/batch union assertions, and D11d-3 adds the 144/504/936-byte nested MP assertions. The cumulative trace has not yet been advanced beyond D4. The C1 through D4 trace-check scripts mechanically compare compiled `sizeof`/alignment/offset probes with the generated trace.

### Allocation-stack measurement

macOS `malloc_history -allBySize -highWaterMark` and full-history `-allEvents` were used around individual deterministic operations. Both KeyGen and Sign showed the five blocks on the `find_uv` caller chain. Verify showed no crypto-originated allocation. Process VM totals are not substituted for the exact object sum.

### Stack-limit bracketing

The operation-only harness was launched with controlled process stack limits:

| Implementation/operation | Largest failing limit | Smallest passing limit |
|---|---:|---:|
| Compact KeyGen | 240 KiB | 241 KiB |
| Compact Sign | 272 KiB | 273 KiB |
| Compact Verify | 32 KiB | 33 KiB |
| Fixed KeyGen | 23 MiB | 24 MiB |

These are host execution thresholds, not precise frame sums. Signals, guard pages, ABI, inlining, compiler and OS behavior make them non-portable.

### Compiler frames

The compact Level-I source at `5b94b09…` was rebuilt with Homebrew GCC 15.2.0, `-O3`/LTO stack diagnostics and `-Wframe-larger-than=4096`. The ten largest baseline frames are recorded in `memory-trace.json` and `MEMORY_MODEL.md`; `quat_ml2` is largest at 79,184 bytes. C1/D1 are measured separately with Arm GCC 15.2.1. The scripts explicitly combine the fixed `.su` component with the source/ABI-checked VLAs; they also run an unlinked Pico Release-like flag probe. A complete linked signer call-chain peak remains unmeasured.

## RP2350 transport smoke

This is a platform result, not a SQIsign benchmark.

| Item | Observed value |
|---|---|
| BOOTSEL identity | UF2 Bootloader v1.0; Model/Board-ID `RP2350` |
| Build target | `pico2`, `rp2350-arm-s`, ARM Secure image, Release |
| Pico SDK | 2.3.0, `98a542c…` |
| Cross compiler | Arm GNU Toolchain 15.2.Rel1, GCC 15.2.1 |
| Physical CPU report | Cortex-M33 CPUID `0x411fd210`, core 0, 32-bit pointers |
| Clocks | system 150,000,000 Hz; USB 48,000,000 Hz |
| Linker addresses | BSS end `0x20002464`; stack limit `0x20080000`; stack top `0x20082000` |
| ELF size | text 37,352 B; data 0 B; BSS 3,020 B |
| Probe frames | report 16 B; main 8 B (`-fstack-usage`) |
| UF2 SHA-256 | `350cd2b09d027e46a4edc72d487f1d608e6033883ae2baa9e14dbf9ef93d657e` |
| Transport result | UF2 flash PASS; USB CDC report PASS; 1200-baud return to BOOTSEL PASS |

The stack symbols describe the SDK linker layout, not measured peak stack. No board revision or RP2350 silicon stepping beyond the CPUID was exposed by the probe.

## Physical RP2350 Level-I Verify milestone

This is a physical single-run correctness smoke, not yet a performance distribution. The firmware source is the clean project commit `40d06653130039dd57304e4d5339c6b575e14c01`; Compact SQIsign is frozen at `5b94b09…`, built with `RADIX_32`, `-O3`, Cortex-M33/Thumb, Release, no LTO. Pico SDK USB interrupts were enabled. The 64-KiB PSP pattern initialization occurs **outside** the timed region; the reported time still includes the thin PSP call wrapper and normal interrupt/XIP effects. No cycle counter was used.

| Target test | Result | Wall time | PSP overwritten extent |
|---|---:|---:|---:|
| Archived official NIST-v2 vector 0 | PASS | 817,184 µs | 30,912 B |
| Compact verification regression fixture 0 | PASS | 817,663 µs | 30,912 B |
| Bit-flipped signature | rejected | not separately timed | — |
| Backtracking/response-length/noncanonical cases | rejected | not separately timed | — |
| Short/long signature lengths | rejected | not separately timed | — |

The raw firmware also reports `randombytes_calls=0` and `status=PASS`. Host-side validation of the same manual RADIX32 closure passes all 100 archived NIST-v2 vectors plus one Compact fixture in normal, ASan+UBSan and LTO configurations. This does not turn the target’s two valid cases into exhaustive target testing.

### SRAM and image accounting

| Item | Bytes | Note |
|---|---:|---|
| RAM vector table | 272 | mutable SRAM |
| RAM `.data` | 5,144 | GNU `size` counts it as `text` because the section is executable |
| `.bss` excluding PSP | 2,956 | mutable SRAM |
| Dedicated PSP reservation | 65,536 | included in total `.bss` 68,492 B |
| Main-RAM subtotal | **73,908** | agrees with reported BSS end `0x200120b4` |
| Separate MSP reservation | 8,192 | full scratch X/Y range, core 1 unused |
| **Conservative exclusive SRAM reservation** | **82,100** | primary current-memory figure |
| PSP overwritten extent | 30,912 | contained within PSP reservation; not an exact maximum |
| Heap / PSRAM / XIP-RAM | **0 / 0 / 0** | final ELF policy gate PASS |
| XIP content bytes | 75,884 | 75,888-byte aligned XIP span |
| Raw flash image span | 81,052 | includes `.data` initializers and flash-end block |
| UF2 transfer container | 162,816 | not flash usage |

GNU `size` prints `text=83,096`, `data=0`, `bss=68,764`; these columns are not the SRAM/flash decomposition above. The linker puts 5,144 bytes of executable RAM `.data` into the `text` column and includes the 2,048-byte linker stack dummy there, while its `bss` includes the 272-byte RAM vector table. The section-address accounting is used instead.

Compiler stack diagnostics for the target closure report `_theta_chain_compute_impl.isra` at 12,544 bytes (dynamic), `ec_dlog_2_weil` at 4,960 bytes, `ec_dlog_2_tate` at 4,256 bytes, `xDBLMUL` at 3,912 bytes, and `protocols_verify_core` at 3,872 bytes. These frames cannot be naively summed. The 30,912-byte PSP extent can miss unwritten frame regions, pattern collisions and untested paths; MSP use is not watermarked. Therefore 82,100 reserved bytes, not 30,912 bytes, is the primary result.

Artifact hashes:

```text
ELF  d0d8660cc5b9650ead4e89a19f14b07993f9b5cc54ae05a79a7a349674008973
UF2  685a311857040e8321e0976f50211a3fd14425418340acf6bf56f07f9c30135b
BIN  f1e11ae2b7cb950f6413b4306c47e76f100847d2c91eb1c398eff8b0b9b13d10
```

The complete capture and manifest are in `results/rp2350/`. The approximately 0.817-second values come from a correctness smoke run; that artifact does not contain a larger input corpus, alternate ordering, separated wrapper/interrupt effects, or a timing distribution.

## RP2350 measurement fields

Standalone target Verify and KeyGen correctness smokes, a sequential K→S run,
and a deterministic one-boot K→S→V run are archived. The KeyGen/Sign records
do not provide input distributions, and the Verify smoke is not a timing
distribution. Target manifests distinguish the following fields where they
apply:

```text
board model/revision and RP2350 stepping where available
Pico SDK tag and commit
arm-none-eabi-gcc version
all C/CMake/linker flags
CPU and peripheral clock configuration
flash/XIP configuration and wait/cache settings
SQIsign protocol/source commit and Level-I parameter identity
RNG mode (deterministic test or production adapter)
firmware .text/.rodata/.data/.bss sizes
explicit workspace size
maximum stack by canary, including interrupt policy
total peak live SRAM and reserved headroom
wall time and cycles for KeyGen, Sign and Verify
minimum, median, mean, standard deviation and maximum
number of repetitions and retry counts
```

Mutable `.data/.bss`, stack, workspaces, SDK/runtime state and measurement buffers are all included in total peak SRAM. Read-only flash is separate and its XIP time cost is measured.

The recorded optimization sequence applies deterministic correctness checks before performance attribution. Energy is not reported because the artifact does not include a documented shunt or calibrated acquisition apparatus.

## Historical transformation matrix

| Variant | Definition | Correct | Peak SRAM | K/S/V time | Status |
|---|---|---:|---:|---:|---|
| A | Direct fixed-precision baseline | Host PASS | Cannot fit; 45.0 MB source VLA sum | Host smoke only | Measured/rejected |
| B | Compact precision baseline | Host PASS | Cannot fit; 6,339,868-byte `find_uv` heap | Host smoke above | Measured/rejected as direct port |
| B1 | B + packed bounded vectors/sort copies | Host byte-identical | 2,044,252-byte `find_uv` heap | No host slowdown resolved | Implemented/measured; still cannot fit |
| C | B + explicit heap-free workspaces only | — | 6,339,868 B before scheduling | — | Designed; useful ownership baseline but cannot fit |
| C1 | B1 + typed explicit workspace API | Host byte-identical; id2iso/closure gates PASS | 2,044,256 B (+4 B padding); projected caller integration only | Deterministic host wrapper: paired median K ×1.0243, S ×1.0021 | Implemented/audited ownership checkpoint; current full K/S allocates per `find_uv` invocation |
| D1 | C1 + compact index/permutation sort | Host byte-identical; independent audit PASS | 1,905,728 B exact typed reservation | Deterministic host path: K ×0.99943, S ×1.00037 | Implemented/audited; current full K/S allocates per `find_uv` invocation |
| D2 | D1 + all-row validation and on-demand exact quotient recomputation | Host byte-identical; independent audit PASS | 962,240 B exact typed reservation; net co-live −943,272 B | Deterministic host path: K ×1.00096, S ×1.00182 | Implemented/audited; current full K/S allocates per `find_uv` invocation |
| D3 | D2 + all-row validation and exact-order two-row streaming | Host transcript/candidate-stream byte-identical; independent audit PASS | 275,840 B exact typed reservation; D2→D3 −686,400 B | Deterministic host path: K ×0.99946, S ×1.00009 | Implemented/audited; current full K/S allocates per `find_uv` invocation; unflattened target path does not fit |
| D4 | D3 + typed ownership of the 77,168-byte lattice-state VLA | Host transcript/candidate-stream byte-identical; independent audit PASS | 353,008 B workspace; path −77,168 B; **net co-live 0 B** | Stored host ratios are noisy/nonreproducible; no speed claim | Implemented/audited stack-control prerequisite; current full K/S allocates per `find_uv` invocation and target fit is unproven |
| D5 | D4 + typed early-ML2 retry state overlaid with the later candidate phase | Host transcripts byte-identical; both radices/focused sanitizers/Arm and independent audits PASS | workspace unchanged at 353,008 B; largest early path −96,120 B (known-descendant adjusted −95,216 B) | Not yet measured on target; fixed clearing cost pending | Implemented/audited early-phase reduction; later legacy paths and allocation wrapper remain, so full fit/heap-free are unproven |
| D6 | D5 + reuse the same ML2 member for both post-`find_uv` fixed-degree reductions | Explicit legacy/output differential, both radices/focused sanitizers/Arm and three independent audits PASS | workspace unchanged; explicit fixed-degree path −95,056 B; explicit Clapotis arena+known path 441,496 B | Production path unchanged; target clear cost pending | Implemented/audited API route; public K/S still selects legacy calls, so no current peak/fit/heap-free claim |
| D7 | D6 + workspace APIs for compact lattice product and intersection | All levels/radices, L1 sanitizer/strict, twelve transcripts, Arm and three audits PASS | workspace unchanged; maximum known product/intersection paths −90,568/−90,560 B | Production path unchanged; target clear cost pending | Implemented/audited API route; public Sign still selects legacy calls |
| D8 | D7 + workspace APIs for prime-norm equivalent-ideal construction | Deterministic legacy/explicit result and post-RNG state, sanitizer/strict, transcripts, Arm and three audits PASS | workspace unchanged; maximum known path −90,936 B (`-Os`) / −90,704 B (Pico-like) | Production path unchanged; target clear cost pending | Implemented/audited API route; public KeyGen/Sign still select legacy calls |
| D9 | D8 + workspace API for prime and composite/cofactor random-ideal construction | Exact representation/post-RNG differential, failure/NULL, sanitizer/strict, transcripts, Arm and three audits PASS | workspace unchanged; maximum known path −84,568 B (`-Os`) / −84,560 B (Pico-like) | Production path unchanged; 534,912–1,000,704 logical clear bytes per successful explicit call | Implemented/audited API route; public KeyGen/Sign still select legacy sampler |
| D10 | D9/D8/D6 routes under one encoded-operation KeyGen owner | Exact encoded PK/SK/post-RNG differential; both radices, sanitizer, effective strict, Arm and three audits PASS | 353,008 B arena; maximum-known arena+path 434,392/434,464 B | Host AB/BA median paired ratio 0.99801 and rerun 0.99883; no target/equivalence claim | Implemented/audited runtime route; linked allocator/VLA and total-fit gates pending |
| D11a | D10 + exact low-memory-only selected KeyGen closure | Exact manifests/symbols/relocations/argv, non-LTO/ThinLTO, strict, fatal sanitizer, Arm ABI and three audits PASS | arena 353,008 B; HNF-inclusive known partial sum 458,816 B; nominal raw remainder 73,664 B | No new timing point | Implemented/audited allocator/GMP/legacy-free selected closure; eight dynamic frames, final Pico link and physical KeyGen pending |
| D11b | D11a + fixed-capacity HNF scratch overlaid with ML2/candidate phase | Normal/workspace HNF differential, invalid/clear/canary, two sanitizer routes, exact Arm manifests and three audits PASS | arena unchanged; known partial sum 445,032 B; D11a→D11b −13,784 B; nominal raw remainder 87,448 B | Pinned host ratios 0.99844/0.99894; whole explicit route only, no target or equivalence claim | Implemented/audited first VLA removal from the exact D11a selected closure; seven dynamic frames, final Pico link and physical KeyGen pending |
| D11c | D11b + fixed-capacity theta scratch overlaid with fixed-degree ML2 | 12-seed differential, reject/clear bounds, normal/lowmem sanitizer, strict, exact Arm manifests/path and three audits PASS | arena and global known sum unchanged at 353,008/445,032 B; theta branch −13,792 B | Pinned host ratios 0.99864/0.99902; cumulative explicit route only, no D11c or target attribution | Implemented/audited theta-local VLA removal; six selected-object dynamic frames, final Pico link and physical KeyGen pending |
| D11d-1 | D11c + bounded batched-inversion scratch overlaid with completed `find_uv`/theta storage | Max-bound/canary/clear, 12-seed differential, normal/lowmem sanitizer, strict, exact two-profile Arm paths and three audits PASS | arena unchanged; Pico-like global known sum 445,040 B; dynamic frames six→five; no global reduction | Pinned host ratios 1.00184/0.99858 with opposing paired signs; cumulative route only | Implemented/audited local VLA removal; five selected-object dynamic frames, final Pico link and physical KeyGen pending |
| D11d-2 | D11d-1 + bounded two-power-dlog tables overlaid with batched inversion | Direct all-level Tate/Weil differential, bounds/canary/clear, 12-seed KeyGen differential, normal/lowmem sanitizer, strict and exact Arm closure PASS | arena/global known sum unchanged at 353,008/445,040 B; recursive Tate/Weil branches −1,136 B; dynamic frames five→three | Pinned host ratios 0.99926/0.99858; cumulative route only | Implemented local VLA removal; three MP dynamic frames, final Pico link and physical KeyGen pending |
| D11d-3 | D11d-2 + bounded fixed-precision MP hierarchy overlaid with the pairing workspace | Direct MP differential and reject/nonpublication/canary/clear fixtures, 12-seed KeyGen differential, normal/lowmem sanitizer, strict and exact Arm closure PASS | arena/global known sum unchanged at 353,008/445,040 B; selected dynamic frames three→zero | Byte-identical to D11d-2 benchmark; no D11d-3 timing attribution | Implemented selected-closure VLA removal; later physical KeyGen artifact below closes its deterministic target gate |
| D12a | D11d-3 APIs under one decoded-key Sign owner | 12-seed exact signature/post-RNG differential, both radices, all-level normal regression, sanitizer/strict, frozen transcript and Arm TU gates PASS | arena 353,008 B; Pico-like known path 114,840 B; diagnostic sum 467,848 B | Host ratios 1.00063/0.99740; cumulative explicit route only | Implemented decoded-key API owner; superseded by encoded D12b closure |
| D12b | D12a plus workspace-aware secret-key decode and bounded even-isogeny strategy | Exact signed-message/post-RNG differential, both radices, all-level normal, sanitizer/strict, exact 51-object Arm closure and physical K→S PASS | 502,204 B exclusive reservation; 30,276 B unassigned; K/S PSP 91,980/120,452 of 131,072 B | one physical run: K 2,696.208062 s, S 7,337.883516 s | Deterministic correctness/feasibility point only; no production RNG, target distribution, equivalence, worst-case stack or side-channel claim |
| D12c | D12b plus bounded detached Verify/Open under one sequential K/S/V union | Valid/altered Verify, Open success/failure zeroing, RNG-nonuse and cleanup fixtures; exact 52-object Arm closure, static ELF policy and one-boot K/S/V PASS | Verify member 15,428 B; operation union unchanged at 353,008 B; 502,336 B exclusive reservation leaves 30,144 B unassigned; K/S/V PSP 91,980/120,452/20,768 of 131,072 B | one physical run: K 2,696.250983 s, S 7,337.481041 s, V 0.813858 s | Deterministic correctness/feasibility point only; no production RNG, target distribution, equivalence, worst-case stack or side-channel claim |
| RP2350 KG | D11d-3 selected closure plus Pico SDK, deterministic CTR-DRBG, guarded owner and separate PSP/MSP | Physical transcript/guard/clear and ELF policy PASS | **493,728 B** exclusive reservation; **38,752 B** unreserved | one run **2,696.500982 s** | Physical deterministic KeyGen complete; production RNG, Sign, worst-case stack and benchmark distribution pending |

The physical KeyGen and Verify measurements are listed outside variants
because the table rows describe the transformation lineage rather than
particular target firmware profiles.

## RP2350 powmod exponent-branch mitigation

The countermeasure diagnostic uses the same 521-bit base/modulus/exponent
schedule and byte-identical D12c/D13 `intbig.c`. It changes only the top-level
exponentiation schedule from `520 + HammingWeight(e)` modular multiplications
to a fixed 1,042 and selects through a full-limb mask.

| Profile | Weight Pearson | Slope µs/bit | R² | Random median | Random SD | Fixed SD |
|---|---:|---:|---:|---:|---:|---:|
| Legacy run 1 | `0.999860` | `+4,004.021` | `0.999720` | 3,125,112.5 µs | 41,321.9 µs | 1.62 µs |
| Regularized run 1 | `-0.131096` | `-18.207` | `0.017186` | 4,183,651.5 µs | 1,433.1 µs | 2.11 µs |
| Legacy run 2 | `0.999860` | `+4,004.056` | `0.999720` | 3,125,111.0 µs | 41,322.3 µs | 1.67 µs |
| Regularized run 2 | `-0.131362` | `-18.229` | `0.017256` | 4,183,651.0 µs | 1,431.9 µs | 1.54 µs |

The regularized/legacy ratio of random medians is
`1.3387203/1.3387208`. Its absolute weight slope is at least 219.66x smaller,
but residual same-input timing repeats across runs at Pearson 0.9999987 and
random SD remains at least 680x fixed SD. The Arm candidate/helper/window
frames are static 1,128/232/32 bytes; no arena growth is attributed. This is a
component-level memory--code--time trade-off measurement, not an analog,
constant-time, full-Sign, or production result.

## RP2350 fixed-work powmod memory--code--time trade-off

The second diagnostic fixes both the exponent schedule and the 521-bit
multiplication/addition schedule. The results below are coarse GPIO-window
timings from a standalone primitive, not full-Sign cycles or analog traces.

| Profile | Random median, run 1 / run 2 | Weight slope, run 1 / run 2 | Fixed−random paired mean, run 1 / run 2 | Linked text |
|---|---:|---:|---:|---:|
| Legacy | `3,125,112.5 / 3,125,111.0 µs` | `+4,004.021 / +4,004.056 µs/bit` | `+4,825.84 / +4,825.22 µs` | 41,528 B |
| Square-and-multiply-always | `4,183,651.5 / 4,183,651.0 µs` | `-18.207 / -18.229 µs/bit` | `-312.31 / -311.97 µs` | 41,416 B |
| Fixed-work multiply/add | `3,200,926.5 / 3,200,926.0 µs` | `+0.095689 / +0.059788 µs/bit` | `-5.1875 / -4.96875 µs` | 41,928 B |

The fixed-work/legacy median ratios are `1.0242596/1.0242599`; relative to the
first countermeasure they are `0.7651035/0.7651035`. Minimum absolute
weight-slope reductions are 41,843.94x versus legacy and 190.28x versus the
first countermeasure. The fixed-work source/object schedule is 521 exponent
rounds, 1,042 modular multiplications, 542,882 multiplier-bit rounds and
1,085,764 modular additions. Arm frames for add/multiply/wrapper/pow/window are
88/56/896/1,000/32 bytes, all static.

The favorable time/code-size trade-off is not a resistance result. Across both
fixed-work runs, the pooled fixed-minus-random median remains `-5 µs` with paired
`t=-7.7963`, and negative-control ordering/variance warnings remain. The
coarse timing screen therefore fails; the candidate is neither integrated
into Sign nor evaluated by analog SPA/EM or exponent recovery.

## Integrated Cornacchia fixed-work checkpoint

The standalone arithmetic is now connected to the opt-in Level-I
`Sign -> RepresentInteger -> Cornacchia -> protected sqrt` route at commit
`c2a80712…`. This is a functional/structural checkpoint, not a performance
claim or an analog result.

| Evidence | Frozen result |
|---|---:|
| Sign invocations / protected square roots | 12 / 361 |
| Signature and post-Sign RNG comparison | byte-identical, output SHA-256 `e6827174…` |
| Pow / exponent rounds | 2,180 / 1,135,780 |
| Modular multiply / multiplier rounds | 2,273,360 / 1,184,420,560 |
| Full 17-word modular additions | 2,368,841,120 |
| Aggregate host control / protected | 7.94 / 75.01 s |
| Descriptive elapsed ratio | `9.447103x` |

The host result is one aggregate deterministic run, not paired samples and not
target timing. It demonstrates that the conservative add-based fixed-work
construction is expensive when applied to every Cornacchia exponentiation;
it does not attribute all cost to the published leak. Cortex-M33 object
inspection proves the fixed loop/control structure only. A later component
checkpoint also replaces the Euclidean loop with a fixed 1,421-round
half-GCD, but Tonelli--Shanks state, half-GCD operand/mask switching, retries,
and higher-level power/EM surfaces remain, so neither speed equivalence nor
side-channel resistance is reported.

The RP2350 public-fixture calibration image also passed its linked-ELF and
five-command serial smoke gates:

| Command class | Target elapsed time |
|---|---:|
| identical control A / B | 32.024229 / 32.024063 s |
| low-weight public fixture | 16.004361 s |
| high-weight public fixture | 64.047796 s |
| first randomized public fixture | 70.418552 s |

All results and workspace clears passed. These five samples are a calibration
smoke, not a timing distribution, and no power/EM samples were captured. The
wide coarse-time range is a residual-variable-flow finding, not evidence of
protection.

The dedicated fixed-half-GCD RP2350 image isolates exactly one protected call
under GPIO 2. Its ELF has one public loop back edge, no legacy division,
register-indexed memory, indirect branch, or table branch in the core. Each
of three datasets contains 30 alternating pairs:

| Dataset | Paired difference | Approximate 95% CI for mean | Interpretation |
|---|---:|---:|---|
| identical A/B | B−A −0.467 us | [−1.372, +0.438] us | negative control |
| fixed schedule, legacy-step class 95/122 | H−L +0.200 us | [−0.530, +0.930] us | no coarse separation resolved |
| fixed/varying public input | R−F +2.000 us | [+0.949, +3.051] us | small input-class effect in this run |

All 180 measured calls use 1,421 rounds and pass output, guard and workspace
clear checks. The firmware timer reports integer microseconds; there are no
current or EM samples. The confidence intervals are descriptive normal
approximations, no equivalence margin was preregistered, and the +2-us result
cannot be attributed to a specific leakage mechanism. Consequently these
data support only target execution readiness and reject a constant-time or
side-channel-resistance claim.

The scheduled count is now independently bounded for the frozen source
recurrence. Exact rational arithmetic proves a per-active-round squared-norm
contraction of at most `10/17`; the integer inequality
`2^493 * 10^644 < 17^644` therefore shows that 644 active rounds suffice for
all positive moduli below `2^492`. The 1,421-round schedule has a 777-round
freeze margin. This proves neither C-to-model refinement nor analog leakage
resistance.

A matched positive-control firmware deliberately restores the variable
Euclidean loop while retaining the same board/core/clock, public fixtures,
precomputed setup, GPIO window, PSP reservation and serial exclusion:

| Dataset | Paired difference | Approximate 95% CI for mean | Interpretation |
|---|---:|---:|---|
| legacy identical A/B | B−A −1.267 us | [−2.839, +0.306] us | negative control crosses zero |
| legacy variable loop, 95/122 steps | H−L +1476.867 us | [+1474.901, +1478.833] us | known iteration leak detected |
| fixed schedule, same 95/122 fixtures | H−L +0.200 us | [−0.530, +0.930] us | not resolved at 1-us resolution |

Elapsed time versus legacy step count has Pearson `0.9999875` on the paired
H/L corpus and `0.9937828` across the random fixture corpus.  This validates
the coarse timer positive control and the schedule-level mitigation at this
granularity.  It does not validate analog acquisition sensitivity, prove
timing equivalence, or establish resistance to SPA/EM/key recovery.

The follow-up acquisition image isolates one protected Montgomery
exponentiation from that square root:

| Evidence | Frozen result |
|---|---:|
| Public fixtures / alternating cycles / calls | 16 / 10 / 160 |
| Result and 1,248-byte clear checks | 160 / 160 PASS |
| Target elapsed range / median | 62,042--62,053 / 62,046 µs |
| Cycle-centered time vs exponent weight | Pearson `0.086615` |
| Minimum- vs maximum-weight means | `-0.4 µs`, Welch `t=-0.578691` |
| Linked text / data / BSS | 46,760 / 0 / 20,968 B |

The UF2 SHA-256 is `1f8b6ddc81acd98e…`; the final ELF verifies that setup,
exponent derivation, validation, clearing and serial I/O are outside an IRQ-
masked GPIO window containing exactly one 521-round protected pow call.  This
is roughly a 12.4x shorter acquisition window than the integrated 768-ms
image.  The values above use a one-microsecond target timer and do not
constitute an equivalence test, an analog sample, or evidence against
bit-/operand-dependent switching leakage.

The corresponding structural residual-leakage gate reports:

| Item | Result |
|---|---:|
| Open side-channel surfaces | 12 |
| Selector control word | 32-bit all zero / all one |
| Ideal selector-HW recovery | 16/16 public exponents exact |
| First-order masking / exponent blinding | absent / absent |
| Physical power/EM result | not measured |

The ideal recovery row is deliberately not a hardware benchmark.  It shows
that the source representation itself contains each exponent bit without
masking; actual observability must be tested against the frozen RP2350 image.
The exact inventory and recomputation gate are
`experiments/sca/residual-surface-inventory.json` and
`scripts/check_sca_residual_surfaces.py`.

The same protected components were then combined and the Sign route was
re-screened with bounded host structural traces:

| Combined protected Sign screen | Control | Fixed/random primary | Repeated process |
|---|---:|---:|---:|
| first 4,000,000 edge events | 1/1 identical | 4/4 different | exact first-event/source reproduction |
| first 4,000,000 selected load/store events | 1/1 identical | 4/4 different | exact normalized event/source/region reproduction |

The earliest full-edge differences are at 1,767,695--1,769,252 events. The
focused address differences are at 3,026,298--3,033,316 events and identify
`ibz_bitsize` sign handling or `ibz_mul` used-limb loops. This is a positive
bounded-prefix structural screen, not a cycle benchmark or physical leakage
measurement. It demonstrates no long-term-key recovery, but it rejects a
whole-Sign constant-time or side-channel-resistance claim for the current
combined source. Exact records are frozen by
`experiments/sca/combined-residual-trace-contract.json`.

## RADIX32 full-source portability gate

This is a correctness/build gate rather than a benchmark point. Compact commit
`9a92e70341a4f52a81e42bcc77d25bf757cfe546` builds the full RADIX32 and
RADIX64 source trees for Levels I/III/V. The targeted RADIX32 suite passes 8/8;
the Level-I quaternion/signature/NIST API/K/S/V set passes in RADIX32, RADIX64
and RADIX32 ASan builds. Twelve deterministic seeds produce byte-identical
1,210-byte PK/SK/SM transcripts across radices. Seven relevant translation
units pass the Cortex-M33/Arm GCC 15.2.1 `-Werror` compile gate.

The optional broad RADIX32 suite, with the separately tracked Level-I id2iso
test excluded, completed 37 of 38 tests successfully; Level-V id2iso reached the
1,500-second harness timeout without reporting a mismatch. This result is not
recorded as a full-suite pass and contains no target timing or target SRAM
measurement.

Test-only commit `a7e1e7f…` subsequently closes the Level-I id2iso test-contract
failure without changing production code. The known seed passes RADIX64 Levels
I/III/V, RADIX32 Level I and RADIX32-ASan Level I; eight additional Level-I
Release seeds pass in an independent audit. RADIX32 Level III/V execution was
not repeated for the final test-only diff because of its extreme host runtime;
only its compilation is recorded. The earlier broad-suite row remains an
historical measurement rather than being relabeled after this change.

## DPE/ML2 correctness gate

Compact commit `5fdd698e52ba082ae0076b1a5356a5b9f5645a23` removes the
reproduced DPE sentinel overflow and the ML2 eager-refresh cause. It does not
change the measured `find_uv` workspace or constitute a benchmark variant.

- focused DPE/ML2 tests: PASS in RADIX64, RADIX32, combined ASan+UBSan,
  ASan-only and RADIX32-ASan builds;
- deterministic ML2 stress: `d=4,8,16` × 1,000 samples for each radix, zero
  failures, invalid ranks or fast-path mismatches;
- frozen-baseline differential corpus: 12/12 Level-I PK/SK/SM transcripts
  byte-identical;
- selected RADIX32 suite: 8/8 PASS;
- Cortex-M33 signer-boundary compilation: 7/7 translation units PASS.

The independent audit additionally passed release quaternion tests at all
three levels and combined-sanitizer RADIX64 tests at all three levels. The only
remaining Level-I UBSan reports are the separately tracked norm-equation and
theta sites. No timing or memory delta is attributed to this correctness-only
commit.

## Norm-equation UB gate

Compact commit `c56c441b3540d9d34c2574d83c59e9fc7a14fa7a` removes the
reachable narrow signed subtraction without changing the accepted deterministic
behavior.

- full quaternion tests: Levels I/III/V × RADIX32/RADIX64, 6/6 PASS;
- selected RADIX32 suite: 8/8 PASS;
- frozen cross-radix Level-I transcripts: 12/12 byte-identical;
- Level-I combined ASan+UBSan (RADIX64) and ASan (RADIX32): PASS;
- Cortex-M33/RADIX32 signer boundary: 7/7 translation units PASS;
- L1/L3/L5 archives: namespaced helper only, no bare generic symbol.

The independent audit approved final diff SHA-256 `61dbe006…` after first
rejecting and correcting a missing namespace mapping. The canonical-mod-four
draft was experimentally rejected because it changed a frozen transcript. No
time or memory delta is attributed to this correctness-only commit. The sole
remaining reproduced Level-I UBSan site is the theta byte assembly.

## Theta byte-assembly UB gate

Compact commit `ee982a2c00cc6b867c038d62de0c51db3e0ec03d` makes all four
little-endian shifts explicitly unsigned.

- recovery-mode combined ASan+UBSan scheme: Levels I/III/V, 3/3 PASS with no
  report for seed 1;
- RADIX32 ASan scheme: Levels I/III/V, 3/3 PASS;
- selected RADIX32 suite: 8/8 PASS;
- frozen cross-radix Level-I transcripts: 12/12 byte-identical;
- Cortex-M33/RADIX32 signer boundary: 8/8 translation units PASS.

Independent audit approved final diff SHA-256 `cd7e9d30…` with no Critical,
High or Medium finding. Old/new Cortex-M33 `.text` is byte-identical, so no time
or code-size delta is assigned. A stub-RNG test of the rejection branch remains
desirable; the saved tests do not prove every program path free of UB.

## Paper evidence summary (2026-09-04)

| Evidence | Bounded result | Evidence limitation |
|---|---|---|
| v2 target repeat | Measured UF2; 2/2 boots PASS; K/S/V PSP 91,980/120,452/20,768 B and transcripts identical | One deterministic input and one board; no input distribution or worst-case stack bound |
| v3 multi-input/placement | 10 official vectors × 2 implementations × 2 placements; 40 valid K/S/V and 40 modified-signature rejection trials PASS; PSP mismatches 0/80 | Finite vectors and one board; no population/worst-case result |
| v3 fixed-frame linked bound | Compiler dynamic records 19→0; K/S/V PSP bounds including one 212-B maximum Secure exception entry are 108,300/127,932/40,468 B; commit-pinned vector-0 observations 62,096/101,060/40,252 B | Linked synchronous operation roots only; 18 handler-side indirect callbacks and IRQ/MSP nesting remain, so not a whole-program bound |
| v3 RP2350 fixed-key timing | 200/200 signatures verify; both images have A/B key-rank Spearman 1.0 and key-median spans 50.8419--51.3752%; Sign PSP delta −3,928 B in all 100 pairs | Key-associated wall-clock timing only; no secret-only attribution, physical leakage, attack, or resistance |

Machine-readable sources are
`results/rp2350/ksv-d13-repeat-2026-09-04-summary.json`,
`results/v3/rp2350/multi-input-placement-clean-2026-09-04/summary.json`,
`results/v3/rp2350/static-closure-clean-2026-09-04/summary.json`,
and `results/v3/rp2350/fixed-key-timing-clean-2026-09-04/summary.json`.

## Public reconstruction closure (2026-09-04)

The public thin bundles reconstruct clean v3 lifetime and fixed-frame commits
from the official base. The regenerated `p324_3/m4f` source-tree digests match
the target manifests byte for byte. Fresh host builds pass NIST API,
self-test, and 100 official responses for both official and lifetime heads;
the fixed-frame head independently passes all 100 official responses. These
are correctness/reconstruction gates, not new performance measurements.
