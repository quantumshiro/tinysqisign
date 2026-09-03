# Phase-1 memory model

This model covers SQIsign Level I in the frozen compact implementation at `5b94b09a1dbbdcc8b91749fec83a9f111ef9cce3`, with the fixed-precision implementation at `d0cb037ee6a9f68f55cb6f55b4e3746c79550330` as the direct baseline. It separates exact source sizes, observed host behavior, conservative projections, and unproved hypotheses.

## Accounting rule

For each mutable object `x_i`, record

\[
x_i=(b_i,s_i,e_i,c_i,q_i),
\]

where `b_i` is a proven required precision or exact object size, `[s_i,e_i]` is the live interval, `c_i` is the storage class, and `q_i` records secrecy/mutability. For a 32-bit-limb integer,

\[
w_i=4\left\lceil\frac{b_i}{32}\right\rceil.
\]

At a control-flow point `t`,

\[
M_{\mathrm{live}}(t)=M_{\mathrm{stack}}(t)+M_{\mathrm{workspace}}(t)+M_{\mathrm{static\ mutable}}+M_{\mathrm{heap}}(t).
\]

The primary result is

\[
M_{\mathrm{peak}}=\max_t M_{\mathrm{live}}(t).
\]

Read-only flash/XIP is reported separately. Moving an object from stack to a global/workspace changes its class, not its contribution to total live SRAM. Heap metadata, interrupt stacks, Pico SDK mutable state, and measurement canaries must also be included in the final firmware result.

## Evidence classes

| Label | Meaning |
|---|---|
| **Exact** | Derived from frozen types/counts and checked by a compiled `sizeof` harness |
| **Observed** | Measured on the frozen host binary; valid for that host/toolchain only |
| **Conservative projection** | Uses exact components plus an explicit upper-envelope assumption |
| **Hypothesis** | Requires a proof or implementation/measurement before use |

The generated `memory-trace.json` and `figures/memory-trace.svg` show exact requested heap payloads for B/P1 and exact typed reservations for C1 through D4. Heap allocator metadata/rounding is excluded. The normalized events are allocation-to-release envelopes, not wall-clock timestamps and not semantic first-to-last-use intervals; those semantic fields are explicitly null until object instrumentation exists. A host `RLIMIT_STACK` threshold is likewise not presented as an object-level Cortex-M trace.

## Precision baselines

| Representation | Level-I mathematical status | Stored Level-I width | `sizeof(ibz_t)` | Consequence |
|---|---|---:|---:|---|
| Fixed-Precision paper/artifact | Published uniform bound 7026 bits | 110 × 64 = 7040 bits | 880 B | Every quaternion integer pays the global maximum |
| Compact Quaternion current revision | Published Level-I magnitude bound 1665 bits | 27 × 64 = 1728 signed bits | 216 B | Mathematical starting point and width used by the frozen layout |
| Frozen compact artifact | Source selects the same 1665-bit magnitude bound | 27 × 64 = 1728 signed bits | 216 B | Used for every exact phase-1 size below |

The current ePrint revision and the frozen artifact agree on the 1665-bit Level-I magnitude bound. The artifact's source comment is not treated as an independent proof: the paper revision is the mathematical citation, while the compiled `sizeof` probe establishes only the concrete stored width.

Using 54 32-bit limbs instead of 27 64-bit limbs leaves `ibz_t` at 216 bytes. Native 32-bit limbs improve portability and may change cycles/stack alignment, but do not by themselves reduce SRAM. SRAM reduction requires smaller per-object `b_i`, fewer simultaneous objects, or a different representation.

## Exact compact `find_uv` peak

Level I has:

```text
FINDUV_box_size       = 2
FINDUV_cube_size      = 624
alternate orders      = 6
total order rows      = 7
entries               = 7 × 624 = 4368
sizeof(ibz_t)         = 216 B
sizeof(ibz_vec_4_t)   = 864 B
sizeof(vec_and_norm)  = 1088 B
```

The five allocations are made before row enumeration/list matching and are released only during the common `find_uv` cleanup.

| Rank | Object | Count × element size | Exact bytes | Share of five-block peak | Birth/last use | Class and secrecy |
|---:|---|---:|---:|---:|---|---|
| 1 | `small_vecs` | 4368 × 864 | 3,773,952 | 59.53% | 40 / 70 | heap; mutable secret-derived ephemeral |
| 2 | `small_norms` | 4368 × 216 | 943,488 | 14.88% | 41 / 69 | heap; mutable secret-derived ephemeral |
| 3 | `quotients` | 4368 × 216 | 943,488 | 14.88% | 42 / 68 | heap; mutable secret-derived ephemeral |
| 4 | `small_vecs_and_norms` | 624 × 1088 | 678,912 | 10.71% | 44 / 66 | heap; mutable per-row sort scratch |
| 5 | `indices` | 7 × 4 | 28 | <0.001% | 43 / 67 | heap; mutable row lengths |
|  | **Total simultaneously live** |  | **6,339,868** | **100%** | 44–66 |  |

Birth/last-use values are normalized control-flow points used by the JSON/SVG generator. The underlying source locations and bound status are stored per object in `memory-trace.json`.

This one routine requires

\[
\frac{6{,}339{,}868}{532{,}480}=11.906
\]

times the RP2350’s entire 520 KiB SRAM, before stack, static data, runtime state, or safety margin. Therefore the direct compact implementation cannot fit regardless of allocator engineering.

## Independent validation of the heap peak

Two checks were used:

1. A compiled Level-I size harness printed the exact type sizes and counts above.
2. macOS `malloc_history -allBySize -highWaterMark` attributed all five crypto-sized blocks in both KeyGen and Sign to `protocols_keygen`/`protocols_sign -> dim2id2iso -> find_uv`.

The process-level high-water reports were about 6.1 MB for KeyGen and 6.0 MB for Sign because the tool’s presentation and VM accounting differ from the exact C-object sum. The exact 6,339,868-byte sum is the storage fact used for feasibility. Verify showed no crypto-originated heap allocation; logger/runtime startup mappings were excluded rather than misreported as verifier memory.

## Host stack envelopes

An operation-only harness was run with descending/ascending macOS stack resource limits. Each bracket is a pass/fail boundary for this host binary, not an exact object sum and not a Cortex-M33 prediction.

| Operation | Fails | Passes | Interpretation |
|---|---:|---:|---|
| KeyGen | 240 KiB | 241 KiB | Host stack maximum lies in this one-KiB bracket, subject to OS/toolchain granularity |
| Sign | 272 KiB | 273 KiB | Largest observed operation stack envelope |
| Verify | 32 KiB | 33 KiB | Consistent in scale with published Cortex-M4 verification memory |

Compiler diagnostics at `-O3`/LTO with `-Wframe-larger-than=4096` found the following largest Level-I frames:

| Function | Compiler frame bytes |
|---|---:|
| `quat_ml2` | 79,184 |
| `protocols_sign` | 29,712 |
| `quat_lattice_mul_mlll` | 27,872 |
| `quat_lattice_mul` | 24,736 |
| `quat_lattice_intersect_mlll` | 23,440 |
| `quat_lideal_create_with_norm` | 19,088 |
| `dim2id2iso_ideal_to_isogeny_clapotis` | 17,712 |
| `ml2_retry_driver` | 17,408 |
| `theta_chain_compute_impl` | 16,112 |
| `find_uv` | 14,624 |

Frame sizes cannot be summed down an arbitrary call graph: optimizers clone/in-line functions and frames from mutually exclusive branches do not necessarily coexist. They identify flattening targets. A Cortex-M build must generate `.su` files and measure a stack canary on hardware.

## Direct fixed-precision baseline

The direct fixed implementation uses 880-byte integers and places all `find_uv` tables in variable-length stack arrays. Its source-semantic table peak is:

| Object | Exact bytes |
|---|---:|
| `small_vecs` | 15,375,360 |
| `small_norms` | 3,843,840 |
| unused `alternate_small_vecs` | 15,375,360 |
| unused `alternate_small_norms` | 3,843,840 |
| `quotients` | 3,843,840 |
| sort scratch | 2,750,592 |
| indices | 28 |
| **Source-semantic VLA sum** | **45,032,860** |

The optimized host KeyGen failed with a 23 MiB stack limit and passed with 24 MiB; the default 8 MiB stack crashed. The difference from the source-semantic sum is expected because the compiler can remove unused arrays and reuse/transform storage. Neither figure is remotely compatible with the RP2350.

## First proven specialization: enumerated vectors

`enumerate_hypercube` stores `(x,y,z,w)` with each loop variable in `[-m,m]`. Level I fixes `m=2`, so every stored coordinate is in `[-2,2]`. A signed three-bit mathematical representation is sufficient, and `int8_t` safely contains the range.

This is a direct proof from the loop bounds; it is not an empirical maximum. It reduces a candidate vector from four 216-byte integers (864 B) to four bytes while resident in the enumeration table:

\[
4368\cdot864=3{,}773{,}952\ \text{B}
\quad\longrightarrow\quad
4368\cdot4=17{,}472\ \text{B}.
\]

The packed vector must be widened into ordinary `ibz_t` temporaries before matrix evaluation. Typed conversion accessors, alignment assertions, and byte-for-byte differential tests are required; no union type-punning is needed.

The norm produced by evaluating the Gram matrix is not bounded by `[-2,2]`. Packing the coordinates proves nothing about the norm or quotient width.

## Implemented packed-vector experiment

Commit `254eda3d54937ec080cf2ba42d2cae8c981e0f5a` applies the coordinate bound without changing norm/quotient precision or allocation ownership. The selected packed vector is widened into an existing `ibz_vec_4_t` temporary immediately before matrix evaluation. Static assertions require a four-byte packed type and ensure the compile-time box bound fits `int8_t`.

Because the per-row sort record also copies a candidate vector, applying the same representation there changes its size from 1,088 to 224 bytes. The exact five-block live set is therefore:

| Object | Count × packed element size | Exact bytes |
|---|---:|---:|
| `small_vecs` | 4,368 × 4 | 17,472 |
| `small_norms` | 4,368 × 216 | 943,488 |
| `quotients` | 4,368 × 216 | 943,488 |
| packed `small_vecs_and_norms` | 624 × 224 | 139,776 |
| `indices` | 7 × 4 | 28 |
| **Total simultaneously live** |  | **2,044,252** |

Full macOS allocation history observed these same five request sizes on the KeyGen `find_uv` chain. Relative to the 6,339,868-byte baseline, the experiment saves 4,295,616 bytes, or 67.7556%. It remains

\[
\frac{2{,}044{,}252}{532{,}480}=3.839
\]

times all RP2350 SRAM, so the result validates the bound and its large leverage but does not establish a fit.

Correctness evidence is byte-for-byte, not only semantic: twelve deterministic seeds produced identical serialized public keys, secret keys and signed messages in the baseline and packed builds. Level-I/III/V hypercube regression tests, Level-I K/S/V, a ten-iteration Level-I signature test and AddressSanitizer-only K/S/V pass. Host timing resolves no slowdown, and the linked host `__text` section is 264 bytes smaller. UndefinedBehaviorSanitizer exposes a separate pre-existing DPE exponent overflow in both builds; it is tracked in `SECURITY_NOTES.md` and is not hidden as a passing sanitizer result.

## Implemented C1 typed ownership baseline

Commit `e61c1fa8fb4898fb606dac807727f97655254739` replaces P1's five allocation owners with one typed `find_uv_workspace_t` for callers of `find_uv_with_workspace()`. It intentionally retains the same components:

| Field | Offset | Bytes |
|---|---:|---:|
| packed `small_vecs` | 0 | 17,472 |
| full-width `small_norms` | 17,472 | 943,488 |
| full-width `quotients` | 960,960 | 943,488 |
| `indices` | 1,904,448 | 28 |
| alignment padding | 1,904,476 | 4 |
| packed `sort_records` | 1,904,480 | 139,776 |
| **`sizeof(find_uv_workspace_t)`** |  | **2,044,256** |

Host and Arm GCC 15.2.1 Cortex-M33/RADIX32 probes agree on every offset, eight-byte alignment and four bytes of internal padding. Thus:

\[
M_{C1}=M_{P1}+4=2{,}044{,}256\ \text{B},
\qquad
\frac{M_{C1}}{532{,}480}=3.83912.
\]

C1 exceeds all RP2350 SRAM by 1,511,776 bytes and saves **zero** bytes relative to P1. It is accepted as an ownership/cleanup checkpoint, not as lifetime scheduling. Current end-to-end KeyGen/Sign still call a compatibility wrapper that allocates this object once per `find_uv` invocation; the frozen paths contain one KeyGen invocation and two sequential Sign invocations. The dashed caller-owned K/S curve in the figure is explicitly a projected integration profile. The host project-code closure has no direct allocator/GMP symbol, but host-libc `qsort` transitive behavior and the final Arm link are not covered by that symbol gate.

The C1 Cortex-M33 `-Os`/soft-float diagnostic row is `15,696 dynamic` for `find_uv_with_workspace`, but **15,696 bytes is only the fixed component**. The function also has, per order row, one 216-byte `ibz_t`, two 3,456-byte `ibz_mat_4x4_t` objects and one 3,896-byte `quat_left_ideal_t`, totaling 11,024 VLA bytes. Level I permits seven rows, so the maximum VLA extent is 77,168 bytes and this diagnostic configuration's maximum own frame is **92,864 bytes before callees**. A separate unlinked C1 probe with the current Pico Release CPU/ABI/`-O3` flags reports 16,416 fixed plus the same VLA, or **93,584 bytes**. Neither figure includes callers, callees, exception frames or final-link effects. The compatibility wrapper is 64 or 72 bytes in those respective builds. Workspace and own frame are simultaneously live and cannot be overlaid.

## Implemented D1 compact-index sort

Commit `cf9f6b6857996dc98f75117fec94ab8b9f0654f4` removes C1's copied 624-record sort array and stores one `uint16_t[624]` permutation. It sorts original row indices by the total key `(full-width norm, original enumeration index)` and applies `permutation[destination] = source` with checked in-place cycles. The existing `remain` integer is reused as permutation norm scratch only after its earlier semantic last use.

| Field | Offset | Bytes |
|---|---:|---:|
| packed `small_vecs` | 0 | 17,472 |
| full-width `small_norms` | 17,472 | 943,488 |
| full-width `quotients` | 960,960 | 943,488 |
| `indices` | 1,904,448 | 28 |
| `uint16_t permutation` | 1,904,476 | 1,248 |
| tail padding | 1,905,724 | 4 |
| **`sizeof(find_uv_workspace_t)`** |  | **1,905,728** |

Host and Arm GCC 15.2.1 probes agree on every offset and eight-byte alignment. Thus C1→D1 is

\[
2{,}044{,}256-1{,}905{,}728=138{,}528\ \text{bytes}
\]

or **6.7764507%**. This is the D1-attributable reduction; the cumulative B→D1 reduction must not be mislabeled as index-sorting benefit. The workspace alone remains 3.57897× RP2350 SRAM and exceeds it by 1,373,248 bytes. In the frozen `-Os` diagnostic, workspace plus the simultaneously live own frame is 1,998,592 bytes. Under the current Pico Release-like unlinked source probe, D1's fixed component is 16,408 bytes and own frame is 93,576 bytes, giving **1,999,304 bytes** before callers, callees, exception frames, mutable statics, I/O or platform state. The latter already exceeds SRAM by 1,466,824 bytes but still is not a final linked-firmware stack bound.

Twelve deterministic C1/D1 KeyGen/Sign/Verify transcripts are byte-identical. Independent exhaustive small-array tests cover heap ordering and every permutation through length eight. The explicit dead-stripped project-code closure has no direct allocator, GMP or `qsort` symbol. Current end-to-end KeyGen/Sign still use the allocation compatibility wrapper, so D1 is neither full heap freedom nor a target-fit result. The same four-VLA structure exists in C1 and D1; flattening it is a mandatory later lifetime transformation. Every final target profile must regenerate `.su`, disassembly, linker-map and multi-path canary evidence using its exact CMake flags.

## Implemented D2 on-demand exact quotients

Commit `d6801884d9c052450a7982e3ac69b29dab0f8893`, after the independently audited all-row validation commit `00f42908ce0147019cd2a1bce6444a2241f45506`, removes D1's 4,368 resident `ibz_t` quotients. Every row norm is first checked positive, before any search can return. For an invertible `(i1,i2)` candidate, D2 then computes the same exact `floor(target/norm2[i2])` immediately before its comparison. Loop order, first-match behavior, RNG consumption and selected outputs are unchanged.

| Field | Offset | Bytes |
|---|---:|---:|
| packed `small_vecs` | 0 | 17,472 |
| full-width `small_norms` | 17,472 | 943,488 |
| `indices` | 960,960 | 28 |
| `uint16_t permutation` | 960,988 | 1,248 |
| tail padding | 962,236 | 4 |
| **`sizeof(find_uv_workspace_t)`** |  | **962,240** |

Host and Arm GCC 15.2.1 Cortex-M33/RADIX32 probes agree on all offsets, eight-byte alignment, 962,236 component bytes and four tail-padding bytes. Therefore

\[
1{,}905{,}728-962{,}240=943{,}488\ \text{workspace bytes},
\]

or **49.5080095% of D1**. The local quotient adds one 216-byte `ibz_t` to the frozen `-Os` fixed frame, from 15,696 to 15,912 bytes. With the unchanged 77,168-byte VLA extent, D2's own frame is 93,080 bytes and the simultaneous workspace-plus-own-frame total is 1,055,320 bytes. Thus the D1→D2 co-live reduction is **943,272 bytes**, not 943,488. The Pico Release-like unlinked probe is 93,792 bytes, giving 1,056,032 bytes before callers, callees, exception state, statics or platform state. The workspace alone is 1.80709× RP2350 SRAM and exceeds it by 429,760 bytes.

Twelve Level-I KeyGen/Sign/Verify transcripts are byte-identical to D1. All-level RADIX64 id2iso/hypercube tests, focused ASan+UBSan, a fresh RADIX32 Level-I path, the Cortex-M33 eight-TU compile gate and the direct-symbol closure gate pass. The explicit closure contains no direct allocator, GMP or `qsort` symbol; the end-to-end wrapper still allocates once per `find_uv` invocation, so this is not yet a heap-free signer.

Test-only instrumentation over twelve deterministic seeds observed one `find_uv` call per KeyGen and two per Sign. The 4,368 entries per invocation are D1's **capacity**, not its populated-row count. Reconstructing D1's actual valid-path precompute loop as `sum(indices[j])` gives 661–748 divisions for KeyGen (median 717.5, mean 711.17, population standard deviation 32.36) and 1,381–1,502 for Sign (median 1,438, mean 1,432.58, standard deviation 38.63). D2 instead observed 1–29 divisions for KeyGen (median 1.5, mean 5, standard deviation 7.70) and 2–106 for Sign (median 8, mean 18.92, standard deviation 28.40). A 30-pair fixed-path host measurement observes small median ratios of 1.00096 for KeyGen and 1.00182 for Sign; no equivalence margin was preregistered, so this is not a “no slowdown” claim. These samples are not a retry distribution or Cortex-M33 result. The no-early-exit combinatorial ceiling remains 9,541,896 candidate pairs per invocation before noninvertible pairs are excluded, so target maximum-time and distribution measurements remain mandatory.

## Implemented schedules and remaining precision hypotheses

These figures concern `find_uv` workspace only. They do not include stack, mutable globals, runtime, interrupt reserve, or output/key buffers.

| Stage | Stored representation/schedule | Workspace | Status |
|---|---|---:|---|
| A | Direct fixed precision, source-semantic VLAs | 45,032,860 B | Exact source sum; impossible |
| B | Frozen compact implementation | 6,339,868 B | Exact heap sum; impossible |
| C | Replace the five allocations with one caller workspace, otherwise unchanged | 6,339,868 B | Heap-free but no total-SRAM reduction; impossible |
| P1 | Implemented packed vectors and packed sort-record copies; other lifetimes unchanged | **2,044,252 B** | Exact host allocation sum; byte-identical but still impossible |
| C1 | P1 components in the implemented typed full-row workspace | **2,044,256 B** | Explicit API ownership checkpoint; +4 B padding, current K/S wrapper still allocates |
| D1 | All 7 rows; packed 4-byte vectors; full-width norms and quotients; reusable `uint16_t[624]` index-sort scratch | **1,905,728 B** | Implemented exact `sizeof`; 1,905,724 components + 4 B tail padding; still impossible |
| D2 | D1 plus compute exact quotients when an invertible list pair needs them | **962,240 B** | Implemented/audited exact `sizeof`; 962,236 components + 4 B tail padding; still above total SRAM |
| D3 | Hold at most two packed-vector/full-width-norm rows; recompute quotients; reuse one index-sort scratch | **275,840 B** | Implemented/audited exact `sizeof`; 275,836 B components + 4 B tail padding; current K/S wrapper still allocates |
| D4 | D3 candidates plus the four former `find_uv` VLA families in typed lattice-state storage | **353,008 B** | Implemented/audited exact `sizeof`; eliminates 77,168 B VLA but increases workspace by the same amount, so total co-live saving is 0 B |
| D5 | D4 lattice state plus `union { complete ML2 retry; D3 candidates; }` | **353,008 B** | Implemented/audited exact `sizeof`; reservation unchanged, largest frozen early-ML2 path decreases by 96,120 B |
| D6 | D5 arena reused for both post-`find_uv` fixed-degree ML2 reductions | **353,008 B** | Implemented/audited explicit API; ABI unchanged, fixed-degree ancestor path decreases by 95,056 B; production K/S not yet connected |
| D7 | D6 plus compact product/intersection workspace APIs | **353,008 B** | Implemented/audited explicit APIs; maximum known product/intersection paths decrease by 90,568/90,560 B; production Sign not yet connected |
| D8 | D7 plus prime-equivalent ideal workspace APIs | **353,008 B** | Implemented/audited explicit APIs; maximum known equivalent-ideal path decreases by 90,936 B (`-Os`) or 90,704 B (Pico-like); production KeyGen/Sign not yet connected |
| D9 | D8 plus prime and composite random-ideal sampling workspace API | **353,008 B** | Implemented/audited explicit API; maximum known random-ideal path decreases by 84,568 B (`-Os`) or 84,560 B (Pico-like); production KeyGen/Sign not yet connected |
| D10 | One operation-owned encoded KeyGen workspace sequentially selecting D9, D8 and D6 routes | **353,008 B** | Implemented/audited KeyGen runtime route; maximum known arena-plus-path is 434,392 B (`-Os`) or 434,464 B (Pico-like), but linked allocator symbols, VLAs and platform state remain |
| D11a | D10 arena under a Level-I/RADIX32 compile-time-specialized KeyGen closure | **353,008 B** | Allocator/GMP/stdio/legacy fallback absent from selected objects/archives; eight dynamic frames remain; HNF-inclusive known partial sum is 458,816 B |
| D11b | D11a plus fixed-capacity HNF scratch overlaid with the ML2/candidate phase | **353,008 B** | HNF VLA removed without arena growth; seven dynamic frames remain; HNF-inclusive known partial sum is 445,032 B |
| D11c | D11b plus fixed-capacity theta scratch overlaid with fixed-degree ML2 | **353,008 B** | Theta-chain VLAs removed from the selected closure without arena growth; six dynamic frames remain; HNF-inclusive known partial sum is still 445,032 B |
| D11d-1 | D11c plus fixed-capacity batched-inversion scratch overlaid with completed `find_uv`/theta storage | **353,008 B** | Batched-inversion VLA removed without arena growth; five dynamic frames remain; Pico-like HNF-inclusive known partial sum is 445,040 B |
| D11d-2 | D11d-1 plus bounded two-power-dlog power tables overlaid with batched inversion | **353,008 B** | Tate/Weil dlog VLAs removed without arena growth; three MP dynamic frames remain; Pico-like HNF-inclusive known partial sum remains 445,040 B |
| D11d-3 | D11d-2 plus nested fixed-precision MP scratch overlaid with the pairing union | **353,008 B** | Last three selected VLAs removed without arena growth; selected Arm dynamic-frame count is zero; Pico-like HNF-inclusive known partial sum remains 445,040 B |
| D12a | One decoded-key Sign workspace sequentially selecting D5–D9 and D11 workspace routes | **353,008 B** | Operation-owned API implemented; Pico-like known HNF path 114,840 B and diagnostic sum 467,848 B; encoded decoder/full closure/link/target gates remain |
| E | D3 with hypothetical 256-bit norm storage | 46,204 B | **Unsafe hypothesis** until a worst-case norm bound is proved |

The exact D3 components are:

```text
one row = 624 × (4-byte packed vector + 216-byte norm) = 137,280 B
two rows                                                = 274,560 B
one reusable uint16_t permutation                       =   1,248 B
seven int row counts                                    =      28 B
total                                                   = 275,836 B
```

The component sum is 275,836 bytes. Host and Arm GCC 15.2.1 Cortex-M33/RADIX32 probes agree that the implemented eight-byte-aligned object has **`sizeof(find_uv_workspace_t) = 275,840` bytes**, with four bytes of tail padding. Therefore D2→D3 saves 686,400 workspace bytes, or **71.3335550% of D2**. D3's frozen outer own frame is 16 bytes larger than D2, so the outer-frame-adjusted co-live saving is 686,384 bytes. During row regeneration a separate 32-byte `finduv_prepare_row.constprop` frame is also live; including that local call-path difference gives approximately 686,352 bytes of saving for that phase. Neither value is a complete linked peak.

The implemented schedule first regenerates all seven rows for a validation-only pre-pass, then performs at most 28 row generations across the triangular `(j1,j2)` search. It therefore performs at most **35 row generations versus seven in D2, a 5× increase** in the enumeration component before early success. Every complete instrumented `(j1,j2,i1,i2,v)` stream is byte-identical to D2 over twelve frozen seeds, in addition to byte-identical protocol transcripts. A clean 30-pair fixed-path host smoke observes D3/D2 median ratios KeyGen 0.99946 and Sign 1.00009; no equivalence margin was preregistered, and this is neither a worst-case/retry-distribution nor a Cortex-M33 result. A balanced profile may cache selected rows only if later whole-firmware measurements leave room.

### D4 typed lattice-state ownership

D4 keeps the D3 candidate schedule and moves the four Level-I VLA families
into the same caller-owned object:

```text
seven adjusted norms:  7 x   216 B =  1,512 B
seven Gram matrices:   7 x 3,456 B = 24,192 B
seven reduced matrices:7 x 3,456 B = 24,192 B
seven ideals:          7 x 3,896 B = 27,272 B
lattice state total                  77,168 B
D3 candidate subworkspace           275,840 B
D4 workspace                         353,008 B
```

Host and Cortex-M33/RADIX32 probes agree on eight-byte alignment, 353,004
component bytes and four bytes of tail padding. In the frozen `-Os` profile,
the `find_uv_with_workspace` frame changes from a 15,928-byte fixed component
plus a 77,168-byte VLA (`dynamic` in `.su`) to a 15,928-byte static frame with
zero VLA. The observed ancestor path correspondingly changes from 273,432 to
196,264 bytes:

\[
(353{,}008-275{,}840) + (196{,}264-273{,}432)=0.
\]

The Pico Release-like profile has the same identity: 353,008 + 196,328 =
275,840 + 273,496 = 549,336 bytes. Thus D4 is a type/lifetime-ownership and
stack-boundedness checkpoint, **not** a memory-saving transformation. It
makes the next typed phase overlay possible without claiming that moving a
buffer from PSP to a workspace reduces SRAM. The measured signer-TU slice now
passes `-Wvla -Walloca -Werror`; the final firmware must apply the same gate to
every linked translation unit.

### D5 early-ML2/candidate phase overlay

D5 makes the lifetime separation explicit.  During early ideal reduction,
ML2 needs a 77,632-byte core plus a 13,824-byte retry permutation and a
3,456-byte unpublished result on Cortex-M33/RADIX32:

```text
ML2 core                              77,632 B
retry permutation                    13,824 B
unpublished rank-four output          3,456 B
complete retry state                 94,912 B
later candidate phase               275,840 B
inactive capacity while ML2 is live 180,928 B
```

The complete retry state and candidate state are members of one typed union;
the 77,168-byte lattice state remains outside because it is live across both
phases.  Thus `sizeof(find_uv_workspace_t)` stays 353,008 bytes.  The inactive
180,928 bytes are union capacity, not ABI padding and not a second saving.

All three ML2 paths before candidate generation—algebra-element
multiplication, conjugate/right-order/transporter, and alternate-ideal
multiplication—receive the same retry workspace.  The entire phase union is
securely cleared before candidate activation, and tests poison the workspace,
guard both extents, require all-zero return state and verify delayed output
publication.  The largest frozen `-Os` branch changes as follows:

```text
D4 ancestor own-frame endpoint       203,384 B
D5 ancestor own-frame endpoint       107,264 B
own-frame-path reduction              96,120 B

D4 with known ML2 descendants        205,032 B
D5 with known ML2 descendants        109,816 B
D4 adjusted + workspace              558,040 B
D5 adjusted + workspace              462,824 B
adjusted diagnostic reduction         95,216 B
```

The 904-byte difference between the two reductions is an inlining change, so
the descendant-adjusted comparison is the fairer diagnostic.  Even it is not
a linked total: deeper descendants, other Sign branches, mutable statics,
MSP/interrupt reserve and platform state remain excluded.  In particular, a
whole-operation 353,008-byte reservation still coexists with later legacy
ML2/MLLL calls.  D5 therefore demonstrates a valid phase/lifetime saving but
does not establish full Sign fit or allocator freedom.

### D6 post-`find_uv` fixed-degree reuse

At D5, the candidate phase is finalized and the full arena is securely cleared
when `find_uv_with_workspace` returns.  Clapotis then constructs the `u` and
`v` fixed-degree ideals sequentially.  D6 reactivates the same union as
`phase.ml2` for each construction; no candidate object or first-reduction
state remains live when the next reduction begins.  The arena therefore stays
353,008 bytes rather than adding either another 94,912-byte retry object or a
legacy automatic ML2 chain.

The frozen Arm `-Os` accounting is:

```text
current legacy fixed-degree path              180,992 B
explicit fixed-degree workspace path           85,936 B
like-for-like reduction                         95,056 B

explicit path + known ML2 descendants           88,488 B
unchanged arena                                353,008 B
diagnostic Clapotis arena-plus-path            441,496 B
```

The same arena is not yet owned by or passed from production
`protocols_sign`/`protocols_keygen`; those callers still use the allocation
compatibility API.  The shared implementation also retains legacy branches.
Accordingly D6 establishes a valid lifetime schedule for an explicit API, not
a current whole-operation peak, allocator-free ELF or linked fit.

### D7 compact product/intersection routes

D7 threads the same 94,912-byte retry member through compact MLLL product and
intersection. The result candidates remain local and are copied only after
full-rank reduction, so the arena does not overlap any input or output object.
The ABI remains 353,008 bytes.

After the large ML2 branch is removed, the maximum descendant changes: normal
LLL reduction becomes larger than the explicit ML2 route. The fair frozen
Arm `-Os` accounting is therefore:

```text
product:           165,152 -> 74,584 B   (save 90,568 B)
ideal intersection 162,784 -> 72,224 B   (save 90,560 B)

product + arena                         427,592 B
ideal intersection + arena              425,232 B
```

These are projected upper-caller paths. Production `sign.c` still references
legacy symbols, the shared objects retain both branches, and linked mutable
state/MSP/interrupt effects are absent. D7 consequently changes no measured
current Sign peak.

### D8 prime-norm equivalent-ideal route

D8 passes the same 94,912-byte retry member through the equivalent-ideal
search and its ideal/lattice multiplication.  The transformation preserves the
enumeration and RNG order and publishes the result only after the complete
candidate succeeds.  It does not change the 353,008-byte arena ABI.

The flattened ML2 branch is no longer the largest audited descendant.  The
fair comparison against frozen D7 therefore includes the denominator-reduction
chain:

```text
Arm -Os/soft:       125,560 -> 34,624 B   (save 90,936 B)
Pico-like O3:       125,608 -> 34,904 B   (save 90,704 B)

projected Sign + arena (-Os)             422,280 B
projected KeyGen + arena (-Os)           393,896 B
```

These remain individual-TU diagnostic paths.  Public KeyGen/Sign select the
legacy entry, shared objects retain both branches, and final data/BSS, MSP,
interrupt state and linked callees are absent.  D8 therefore changes no
measured production peak and supplies no RP2350-fit result.

### D9 random-ideal construction route

D9 keeps the random search, primality tests and RNG order unchanged, then
passes the existing 94,912-byte retry member only to the final
`quat_lideal_create_with_norm` step. The compatibility entry remains on the
legacy branch; the explicit entry clears the complete workspace on entry and
the retry path clears it on return. Fixed CTR-DRBG differentials cover prime
and composite/cofactor sampling, exact ideal representation, the next 64 RNG
bytes, invalid/NULL nonpublication, guards and full zeroization.

The maximum after ML2 flattening is a non-ML2 containment/inversion branch:

```text
Arm -Os/soft:       122,312 -> 37,744 B   (save 84,568 B)
Pico-like O3:       122,568 -> 38,008 B   (save 84,560 B)

arena + path (-Os)                            390,752 B
arena + path (Pico-like)                      391,016 B
projected Sign prefix + arena (-Os/Pico)      425,400 / 425,688 B
projected KeyGen prefix + arena (-Os/Pico)    397,016 / 397,280 B
```

For `-Os`, the inverse calls a 456-byte coefficient helper which calls the
1,384-byte multiply. Under Pico-like `-O3`, that helper is inlined into the
7,064-byte inverse frame; the deepest real child is instead a 240-byte 2×2
determinant which calls the multiply. `scripts/measure_d9_random_ideal_stack_path.sh`
freezes these positive and negative assembly edges so the standalone helper
record cannot be double-counted. These remain individual-TU diagnostics.
Public KeyGen/Sign still select the legacy sampler, final linked state is
absent, and no current production peak or fit result follows.

### D10 operation-owned encoded KeyGen route

D10 introduces `protocols_keygen_workspace_t` as a single-field owner of the
existing `find_uv_workspace_t`. KeyGen activates the same 353,008-byte arena
sequentially for D9 random-ideal construction, D8 equivalent-ideal search and
D6 arbitrary-isogeny conversion; no second retry or candidate arena is added.
The encoded wrapper remains live while it serializes the completed key, and
clears the whole arena on entry and every return. Host and Cortex-M33 probes
freeze size 353,008, alignment eight and offset zero.

The largest currently audited branch is the early alternate-ideal path inside
`find_uv`, not fixed-degree reduction or key serialization:

```text
                               Arm -Os/soft   Pico-like -O3
operation frames/path                81,384 B          81,456 B
caller-owned arena                  353,008 B         353,008 B
diagnostic sum                      434,392 B         434,464 B
nominal raw-SRAM margin              98,088 B          98,016 B

fixed-degree path plus arena        413,120 B         413,192 B
secret encoding path plus arena     366,624 B         366,640 B
```

The encoding arithmetic deliberately excludes a 24-byte `quat_alg_mul`
wrapper under `-Os`: assembly restores its frame before tail-branching to
`quat_alg_coord_mul`, so it is not co-live with the latter's 1,128-byte frame
and the 1,384-byte integer multiply. `scripts/measure_d10_keygen_stack_path.sh`
freezes the profile-specific wrappers, inlining, frames and call edges.
It also follows `public_key_to_bytes` through the finite-field inversion
subtree: direct public encoding is 6,360/6,480 bytes including the top wrapper,
and the nested secret-to-public branch is 7,696/7,816 bytes, both below the
secret-generator path above. C-library descendants remain excluded.

These values are **maximum-known individual-TU path diagnostics**, not total
SRAM bounds. They omit encoded PK/SK buffers, final linked `.data/.bss`,
untracked descendants, C library/RNG frames, MSP/ISR state and SDK runtime.
Moreover, the explicit execution path chooses only workspace APIs, but shared
objects still contain legacy branches and the linked host closure imports
`malloc/free`. Reachable `biextension.c`/`fp2.c` variable-length arrays and the
same-object integer decoder VLA also prevent a whole-closure VLA-free claim.
The top-level entry and exit alone clear 706,016 logical bytes, before nested
workspace/retry clearing; this is a write-length accounting fact, not target
cycles. D10 therefore closes operation ownership and correctness. D11a below
closes the selected-closure allocator specialization while leaving VLA
flattening and the linked Pico measurement open.

### D11a physically specialized KeyGen closure

D11a keeps the D10 arena layout but makes the Level-I/RADIX32 workspace route
a compile-time build profile. The selected closure contains neither allocator,
GMP, stdio nor curated legacy large-stack/fallback symbols. This is verified on
every selected object and archive, not inferred from a non-NULL runtime branch
or final garbage collection. Exact manifests cover the actually built compile
commands, archive members, unresolved symbols, relocations and all Arm `.su`
files. The deterministic Arm profile has 56 bytes of writable section payload:
52 bytes are test-only CTR-DRBG state and four bytes are the secure-clear
function pointer. This is object payload, not final linker allocation.

The specialization exposes eight remaining dynamic frames rather than hiding
their VLA payloads behind `.su` fixed components. The current largest known
route is the normal HNF branch:

```text
ancestors through lattice multiplication               78,672 B
HNF fixed component                                      6,792 B
16 bounded ibz_vec_4_t rows: 16 × 864                   13,824 B
known deepest HNF descendants                            6,520 B
                                                        ---------
project-known PSP partial path                         105,808 B
caller-owned arena                                     353,008 B
                                                        ---------
diagnostic partial sum                                 458,816 B
nominal raw margin to 532,480 B                         73,664 B
```

All arithmetic is exact for the frozen individual-object Arm profile, but the
sum is not a linked upper bound. It excludes remaining VLA paths, output
buffers, final section alignment, libgcc/newlib/libm descendants, production
RNG, MSP/ISR and exception-entry PSP state.

### D11b HNF workspace overlay

D11b replaces the HNF VLA with a 13,824-byte fixed-capacity workspace. It
shares offset zero with the 94,912-byte ML2 retry workspace, which in turn
shares the early phase with the larger 275,840-byte candidate reservation.
HNF and ML2 are mutually exclusive within one lattice multiplication; the
candidate phase begins only after all early lattice reductions and a complete
phase clear. The arena ABI therefore remains 353,008 bytes, aligned to eight.

```text
ancestors through lattice multiplication               78,680 B
HNF workspace wrapper                                      32 B
HNF fixed implementation                                6,792 B
known deepest HNF descendants                            6,520 B
                                                        --------
project-known PSP partial path                          92,024 B
caller-owned arena                                     353,008 B
                                                        --------
diagnostic partial sum                                 445,032 B
nominal raw margin to 532,480 B                         87,448 B
```

Relative to D11a, removing 13,824 VLA bytes is offset by an eight-byte lattice
frame increase and a 32-byte wrapper, giving a net 13,784-byte reduction. The
dynamic-frame manifest falls from eight entries to seven. This still excludes
the remaining theta, dlog, batched-inversion and MP VLA payloads, all final
link/runtime state, output buffers and both exception stacks. D11b is the first
VLA-flattening checkpoint in the exact D11a selected closure, not a total peak
or fit result.

### D11c theta workspace overlay

D11c replaces the selected low-memory theta wrapper's six arrays with a
13,844-byte fixed workspace. Its Level-I Arm layout is 13,842 bytes of fields
plus two bytes of internal padding, aligned to four. The fixed-degree union is
still governed by the 94,912-byte ML2 member, so the 353,008-byte outer arena
does not grow.

```text
D11b fixed-u/v theta path with ancestors                68,584 B
D11c fixed-u/v theta path with ancestors                54,792 B
                                                        --------
theta-local reduction                                   13,792 B
caller-owned arena                                     353,008 B
arena plus D11c theta path                             407,800 B
```

This local reduction does not update the operation maximum. The HNF path is
still 92,024 bytes, so the global diagnostic partial sum remains 445,032
bytes. The selected-object dynamic-frame inventory falls from seven to six.
The remaining records are Tate/Weil dlog, one batched inversion and three MP
routines; their VLA payloads, final linked sections, production RNG, outputs,
MSP/ISR and exception-entry state remain outside this accounting. D11c is
therefore a local stack-control checkpoint, not a total peak or fit result.

### D11d-1 batched-inversion workspace overlay

D11d-1 replaces two `fp2_t[11]` arrays with a 1,584-byte, four-byte-aligned
workspace whose second array begins at offset 792. The top-level owner overlays
it at offset zero with the completed `find_uv` phase; theta places the same
object at offset 13,844 after its other bounded fields. The resulting theta
workspace is 15,428 bytes, still far smaller than the existing 94,912-byte
fixed-degree ML2 member. The outer arena therefore remains 353,008 bytes.

```text
                                             -Os/soft     Pico-like
D11d-1 fixed-u/v theta path                   54,464 B       54,816 B
arena plus theta path                        407,472 B      407,824 B
known HNF path                                91,448 B       92,032 B
arena plus known HNF path                    444,456 B      445,040 B
nominal raw difference from 532,480 B         88,024 B       87,440 B
```

The VLA payload removed at a maximum batched-inversion call is 1,584 bytes,
and the selected-object dynamic inventory falls from six to five. It does not
reduce the known operation maximum: in the Pico-like build Clapotis grows by
eight bytes, while the `-Os` HNF maximum is unchanged. One fixed successful
KeyGen trace executes 40 entry/exit clears of the 1,584-byte object and six
clears of the 1,584-byte theta extension, totaling 72,864 additional logical
clear bytes relative to D11c. This count is transcript-specific and is not a
retry bound, bus-traffic measurement or target-cycle result. The remaining
dynamic records are Tate/Weil dlog and three MP routines. Their payloads,
final linked sections, production RNG, outputs, MSP/ISR and exception-entry
state remain outside this accounting.

### D11d-2 two-power-discrete-log workspace overlay

D11d-2 bounds Level-I `ceil(log2(e)) + 1` at eight and replaces each pair of
`fp2_t[8]` power-table VLAs with a 1,152-byte, four-byte-aligned object whose
second table begins at offset 576. It is unioned with the existing 1,584-byte
batched-inversion object, so the field scratch union and enclosing 353,008-byte
arena do not grow.

In the frozen Pico-like build, the recursive Tate path including public
KeyGen ancestors falls from 12,312 to 11,176 bytes. At the inner-operation
boundary, Tate falls from 5,800 to 4,664 bytes and Weil from 6,504 to 5,368
bytes: **1,136 bytes** in both cases after including the new 24-byte helper.
These are recursive-dlog branches rather than complete operation maxima. HNF
still dominates at 92,032 bytes, and arena plus global known path remains
445,040 bytes with 87,440 nominal raw bytes before omitted target state.

The selected dynamic inventory falls from five to three, leaving only
`mp_mul`, `mp_inv_2e` and `mp_invert_matrix`. One fixed successful KeyGen calls
the Tate dlog helper four times; entry/exit clearing therefore adds 9,216
logical bytes relative to D11d-1. This is not a bus/cycle measurement or a
general operation bound. Final link sections, production RNG, outputs,
MSP/ISR, exception entry and untracked library descendants remain excluded.

### D11d-3 fixed-precision MP workspace overlay

D11d-3 bounds the bundled parameter-set precision at 576 bits and gives the
three nested MP operations explicit storage.  On Cortex-M33/RADIX32 the
multiplication object is 144 bytes, the inversion object is 504 bytes and the
complete matrix-inversion object is 936 bytes.  The latter contains six
72-byte result/determinant rows plus a union of the nested multiply/inversion
state.  It fits below the existing 1,584-byte batch/dlog member at the same
offset, so the enclosing KeyGen arena remains 353,008 bytes and eight-byte
aligned.

The low-memory build physically omits the legacy `mp_mul`, `mp_inv_2e` and
`mp_invert_matrix` VLA bodies.  Its exact 45-object Arm closure compiles with
`-Wvla -Walloca -Werror` and every selected `.su` row is static.  The normal
build retains those legacy functions for compatibility.  The deepest audited
new MP subpath is 224 bytes in the Pico-like profile (88-byte matrix wrapper +
64-byte inversion core + 72-byte multiply core) and 192 bytes under
`-Os`/soft-float.  Including the explicit inverted-basis wrapper and shared
change-basis core gives 448 and 416 bytes respectively.

This is a closure/stack-shape result, not an operation-peak reduction.  Current
KeyGen calls the non-inverting basis route, so it does not execute the MP
matrix workspace.  HNF remains the known global branch at 92,032 bytes
Pico-like or 91,448 bytes `-Os`; arena plus path remains 445,040 or 444,456
bytes.  Final linked sections, output buffers, production RNG, PSP/MSP/ISR,
exception entry and runtime-library descendants remain outside these sums.

### D12a decoded-key Sign owner

D12a adds `protocols_sign_workspace_t`, an eight-byte-aligned union whose
largest member is the existing 353,008-byte `find_uv_workspace_t`. The
commitment, challenge ideal, response intersection/product, response ideal,
auxiliary ideal/isogeny, theta and basis-change phases are sequential and all
reuse offset zero. The hidden challenge-ideal conversion is explicitly routed
through the same 94,912-byte ML2 retry member. The outer arena therefore does
not grow.

The frozen Pico-like individual-object HNF path contains 32 bytes in the
explicit wrapper, 28,704 in the Sign protocol frame, 37,456 across arbitrary
isogeny/Clapotis/`find_uv`, 42,056 across ideal/lattice/HNF entry frames and
6,592 in the deepest audited HNF division descendants. Thus:

\[
114{,}840 + 353{,}008 = 467{,}848\ \text{bytes}.
\]

The 64,632-byte difference from all RP2350 SRAM is not reserved headroom. The
sum omits final-link mutable state, the encoded wrapper and decoder, MSP/ISR
and exception frames, SDK/libc and untracked descendants. It also does not
inherit the zero-dynamic-frame property of the D11d-3 KeyGen closure:
`secret_key_from_bytes` remains a legacy decoder and `ec_eval_even_strategy`
still uses a VLA. D12a is therefore an operation-ownership checkpoint, not a
linked allocator-free Sign closure or a fit result.

### D12b encoded Sign and D12c Verify ownership

D12b extends the D12a union through secret-key decoding and the encoded public
Sign entry. The last selected dynamic object, `ec_eval_even_strategy`, becomes
a bounded 1,316-byte member at offset zero. It is smaller than every dominant
arena member, so the Level-I/RADIX32 Sign owner remains **353,008 bytes**,
eight-byte aligned. The D12b selected Arm closure contains 51 objects and zero
dynamic `.su` records; compatibility builds retain the old VLA path.

D12c defines a **15,428-byte**, four-byte-aligned Verify owner. Its 1,316-byte
even-isogeny member and 15,428-byte theta member are sequential and share
offset zero. KeyGen, Sign and Verify are then members of one top-level union:

| Operation member | Bytes | Alignment | Top-level offset |
|---|---:|---:|---:|
| KeyGen | 353,008 | 8 | 0 |
| Sign | 353,008 | 8 | 0 |
| Verify | 15,428 | 4 | 0 |
| **operation union** | **353,008** | **8** | — |

Verify clears its complete 15,428-byte contractual member. The outer operation
owner must separately clear the unused tail of the 353,008-byte union before
releasing ownership; the RP2350 combined harness checks both facts. This is a
sequential overlay, never an overlap between an active workspace and PSP.

The exact D12b/D12c selected object sets exclude allocator, GMP and legacy
large-stack entries and compile with `-Wvla -Walloca -Werror`. These closure
facts do not substitute for the final linked SRAM sum or the physical PSP/MSP
watermarks, which remain release evidence rather than source-layout evidence.

The clean D12c release artifact supplies that separate evidence boundary:

| Region/observation | Bytes |
|---|---:|
| guarded sequential operation owner | 353,136 |
| dedicated PSP reservation | 131,072 |
| other linked main-bank state | 9,936 |
| main-bank linked total | 494,144 |
| separate MSP reservation | 8,192 |
| **exclusive on-chip reservation** | **502,336** |
| **unreserved RP2350 SRAM** | **30,144** |
| observed KeyGen / Sign / Verify PSP extents | 91,980 / 120,452 / 20,768 |
| conservative observed MSP upper extent | 2,396 |

One physical boot completes K/S/V and retains the operation-owner and stack
guards. These section totals are exact for the archived ELF; pattern extents
are observations for the frozen inputs, not worst-case stack bounds. The heap
is zero and the deterministic test RNG is not a production entropy source.

### Physical RP2350 deterministic KeyGen image

Project commit `64bd997…` closes those omitted terms for one concrete
deterministic Level-I/RADIX32 firmware.  The final linked and exclusively
reserved SRAM is:

| Region | Bytes | Evidence boundary |
|---|---:|---|
| guarded KeyGen owner | 353,136 | 353,008-byte workspace plus two 64-byte guards |
| dedicated PSP | 122,880 | linker-reserved, pattern-observed extent 91,980 B |
| other main-bank state | 9,520 | vectors, `.data`, remaining `.bss` and alignment |
| main SRAM through `.bss` end | **485,536** | exact linker address `0x200768a0 - 0x20000000` |
| dedicated scratch-bank MSP | 8,192 | conservative observed upper extent 2,308 B |
| **exclusive on-chip reservation** | **493,728** | main bank plus scratch banks |
| **unreserved RP2350 SRAM** | **38,752** | 532,480 − 493,728 |

The linked heap is zero bytes.  Scratch X/Y contain only the MSP reservation;
there is no core-1 stack payload.  The ELF audit finds no allocator, GMP,
system-RNG, undefined or legacy-large-stack symbol.  A physical KeyGen run
preserved both guards, fully cleared the workspace and matched the host
transcript.  These figures are exact for this linked image, but the pattern
watermarks are observations rather than strict worst-case stack bounds, and
the deterministic RNG is not a production entropy implementation.

## Coarse whole-operation fit envelope

Adding a host stack *upper bracket* to D3 is intentionally conservative and does not assert temporal co-liveness:

| Operation | D3 workspace | Host pass bracket | Conservative sum | Compared with 532,480 B |
|---|---:|---:|---:|---:|
| KeyGen | 275,840 | 246,784 | 522,624 | 9,856 B nominal headroom before all statics/runtime—insufficient as a firmware design |
| Sign | 275,840 | 279,552 | 555,392 | 22,912 B over total SRAM before statics/runtime |
| Verify | 0 `find_uv` workspace | 33,792 | 33,792 | Large nominal headroom; target measurement still required |

This is why “pack the vectors” alone is not the final result. A full fit needs stack flattening/overlay at least in the `quat_ml2`/signature/MLLL chain, precise co-live measurement, and likely additional specialization or streaming. The 555,392-byte Sign row is a deliberately coarse host-derived envelope, not a Cortex-M total or a lower bound; it motivates the next experiment but cannot establish fit on its own.

A newer Cortex-M33 diagnostic gives the same qualitative answer without treating host `RLIMIT_STACK` as target memory. An observed source path reaches `quat_ml2` during `find_uv` setup. In D3, summing every ancestor own-frame record from the public Sign entry through that call gives 273,432 bytes before `quat_ml2` descendants, target wrappers and exception state. The exact D3 reservation plus that unflattened chain is

\[
275{,}840+273{,}432=549{,}272\ \text{bytes},
\]

16,792 bytes above all SRAM before platform mutable state. A second unlinked
Pico Release-like `-O3`/softfp path is 273,496 bytes, giving 549,336 bytes and a
16,856-byte excess. D4 removes the VLA and changes these decompositions to
353,008 + 196,264 = 549,272 and 353,008 + 196,328 = 549,336; the totals do not
move. These sums are not linked upper bounds—individual `.su`
rows and a host-observed path do not prove a target maximum—but they decisively
reject an unflattened D3 port. The dominant opportunity is phase overlay: the candidate-row arena is not yet semantically live during early ideal reduction/ML2, so an explicit owner may reuse those bytes only after a static/dynamic last-use certificate and typed-lifetime tests.

If a future proof established a 256-bit bound for these particular row norms, D3 plus the current host Sign stack envelope would be about 325,756 B. That number is included only to quantify the value of a bound; it must not be selected in production or cited as achieved memory.

## D13 frozen checkpoint: certified norm sketches

The D13 checkpoint takes a different route from the hypothetical 256-bit
bound. It stores no full candidate-norm row. Each retained four-byte vector
has a 10-byte sorting sketch (exact bit length plus leading 64 bits), while two
full `ibz_t` values and shared quadratic-form scratch certify sketch ties and
materialize search operands on demand. Every ambiguous comparison is replayed
exactly, so correctness does not depend on a collision assumption or a maximum
norm width.

For Level I on Cortex-M33/RADIX32, the candidate member is 14,024 bytes and no
longer dominates the existing 94,912-byte ML2 phase union. With the co-live
77,168-byte lattice state, the operation arena is therefore 172,080 bytes:

\[
353{,}008-172{,}080=180{,}928\ \text{bytes}
\quad (51.253229\%).
\]

The clean D13 RP2350 link confirms that the ABI reduction survives final code
generation. The guarded operation owner falls from 353,136 to 172,208 bytes,
`.bss` falls from 487,888 to 306,960 bytes, and main-bank use through `.bss`
falls from 494,144 to 313,216 bytes. With the same 131,072-byte PSP and
8,192-byte MSP reservations, the exclusive on-chip reservation is therefore
321,408 bytes and 211,072 of the RP2350's 532,480 SRAM bytes remain
unassigned. Every linked quantity improves by the exact 180,928-byte owner
delta; this is no longer a projection.

The recomputed norms increase the `find_uv` outer/helper co-live
row-preparation path by 48 bytes under `-Os/soft` and 64 bytes under the
Pico-like `-O3/softfp` profile. Net local storage reduction there is therefore
180,880/180,864 bytes. The final ELF still has a zero-byte heap, 52 static
Compact stack records, and no allocator, GMP, system-RNG or curated legacy
large-stack symbol. These linker and individual-TU facts do not by themselves
prove worst-case PSP/MSP use, constant-time behavior or production entropy
suitability; the physical run remains a separate evidence boundary.

That physical boundary now has one frozen deterministic K/S/V observation.
The PSP overwritten extents are 91,980, 120,452 and 20,768 bytes within the
131,072-byte reservation, and the conservative MSP upper extent is 2,396
bytes within 8,192 bytes. They are unchanged from D12c in this fixture. K/S/V
take 2,698.150029, 7,611.258527 and 0.814341 seconds. These watermarks and
times validate this execution only; they are not worst-case stack bounds or a
performance distribution.

## Live-range opportunities

| Object/phase | Current lifetime problem | Proposed schedule | Safety obligation |
|---|---|---|---|
| Candidate coordinates | Wide global `ibz_t` representation for values in `[-2,2]` | **Implemented:** packed signed bytes, widened only at selected matrix evaluation | Direct range proof plus byte-identical conversion tests |
| Sort records | C1 copied a 139,776-byte packed vector+norm record array | **Implemented D1:** sort a 1,248-byte permutation by `(full norm, original index)` and apply cycles | Byte-identical transcripts plus exhaustive small ordering/cycle tests |
| Quotients | D1 retained every `n/norm[j]` for all rows | **Implemented D2:** validate all rows, then recompute inside the active pair/list scan | Exact-division operand/order equivalence; byte-identical outputs; measure worst-case timing and leakage |
| Order rows | D2 retained seven rows for all pair combinations | **Implemented D3:** validation pre-pass, then outer row plus one inner row; regenerate discarded rows | Complete candidate-stream equality; RNG-free deterministic enumeration; preserve fatal priority and first solution |
| Whole KeyGen operation | Legacy KeyGen allocated per `find_uv` invocation and did not own the D6–D9 phase arena | **Implemented D10:** one encoded-operation owner sequentially routes D9, D8 and D6 | Exact output/RNG differential; full clear; remove same-object allocator calls, all legacy fallback/large-stack branches and reachable VLAs before linked target claim |
| Two IdealToIsogeny invocations in Sign | Legacy public Sign allocated one workspace per `find_uv` invocation | **Implemented D12b encoded route:** one owner reuses the D6 arena across both invocations and decoder/even-isogeny phases | No secret survives cleanup; explicit zeroization policy; final ELF allocator gate |
| `find_uv` lattice state | Four VLA families totaling 77,168 B | **Implemented D4:** typed caller-owned lattice-state member | Exact ABI/offset probes, init/finalize symmetry, `-Wvla -Walloca -Werror`; total co-live delta must remain 0 until overlay |
| Early `find_uv` ML2 locals | Large nested automatic arrays, live before candidate enumeration | **Implemented D5:** overlay typed core/retry storage with the later D3 candidate subworkspace after explicit phase teardown | Audited alignment/effective type, no escaped pointer, canaries, all-return zeroization and candidate-not-yet-live ordering |
| Later Sign ML2/MLLL locals | Legacy public/encoded route owned no arena | **Implemented D12b encoded route:** D6–D9 calls, decoder and even-isogeny scratch share one operation owner | Complete reachable-call inventory, delayed publication, retries, non-alias ownership and linked PSP watermark |
| Public precomputation | May consume mutable address space if copied | `const` flash/XIP placement | Confirm linker map; no run-time relocation into SRAM |

## Constant-time and secrecy constraints

The proposed sizes and offsets are parameter-set and control-flow-phase dependent, not secret-value dependent. The allocator decision is eliminated. However, the frozen signer already contains variable-time number theory, retries, sorting, and early-success behavior; this study does not claim side-channel resistance.

Recomputation must not add a new secret-dependent choice of which buffers exist or where they are located. It may still change timing and power traces, so each accepted optimization receives a constant-time-impact entry in `RESEARCH_LOG.md` and `SECURITY_NOTES.md`. Secret-derived candidate rows are SRAM-only and are never spilled to XIP flash.

## Physical RP2350 Verify accounting

The clean verification-only firmware at project commit `40d06653130039dd57304e4d5339c6b575e14c01` has the following final ELF sections:

| Mutable reservation | Bytes | Evidence/interpretation |
|---|---:|---|
| RAM vector table | 272 | `.ram_vector_table` |
| RAM `.data` | 5,144 | Includes RAM-resident SDK code/data; GNU `size` classifies this under `text` because the section is executable |
| `.bss` excluding dedicated PSP | 2,956 | SDK state, fixtures and reporting state |
| Dedicated PSP reservation | 65,536 | Included in total `.bss` of 68,492 bytes |
| **Main-RAM subtotal** | **73,908** | Exactly `0x200120b4 - 0x20000000` |
| Heap | **0** | Zero-size `.heap`; allocator/GMP/RNG symbols forbidden at link audit |
| Separate MSP reservation | 8,192 | `0x20080000..0x20082000`; core 1 and scratch banks unused |
| **Conservative exclusive SRAM reservation** | **82,100** | Main subtotal + MSP reservation |
| Remaining main SRAM before MSP area | 450,380 | `0x20080000 - 0x200120b4` |

The valid NIST-v2 and Compact fixture paths each changed 30,912 bytes at the bottom of the pre-patterned 64-KiB PSP buffer. This is an **overwritten extent**, not a proof of exact maximum SP descent: unwritten frame bytes, pattern collisions, other valid signatures and MSP/interrupt usage can make it an underestimate. It also must not be added to the 65,536-byte reservation, because it is contained within it.

For the current implementation, 82,100 bytes is therefore the defensible primary SRAM number. A later optimized profile may reduce the PSP reservation only after broader stack measurements and guard testing. The firmware's XIP sections contain 75,884 bytes over a 75,888-byte aligned address span; the 5,144-byte `.data` initializer and 20-byte flash-end block make the raw image span 81,052 bytes. Flash is reported separately from SRAM.

## Trace artifact

Regenerate the machine-readable trace and figure with:

```bash
./scripts/generate_memory_trace.py
./scripts/check_finduv_workspace_trace.sh
./scripts/check_d1_workspace_trace.sh
./scripts/check_d2_workspace_trace.sh
./scripts/check_d3_workspace_trace.sh
./scripts/check_d4_workspace_trace.sh
```

The JSON records, per top object:

```text
name and type
requested heap payload or typed reservation bytes
mathematical/storage width where known
normalized allocation/release points
null semantic first/last-use points until instrumented
heap/storage class
secret classification
mutability
source location
bound/proof status
```

The current artifact is sufficient to explain the dominant phase-1 allocation envelope, validate the C1/D1/D2/D3/D4 ABIs, and reject both a direct port and an unoverlaid D4 signer. It is not yet the final complete object-lifetime trace requested for the paper. The next implementation phases must replace normalized allocation events with semantic instrumentation IDs across all major frames/workspaces, cover repeated/retried IdealToIsogeny calls, and combine them with firmware linker/canary measurements.
