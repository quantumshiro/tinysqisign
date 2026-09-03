# Related work audit

Literature and public-source freeze: **2026-09-04 (Asia/Tokyo)**. This audit was rerun after the public SQIsign v3.0 release. It treats a host port, a verification-only microcontroller build, and a full KeyGen/Sign/Verify microcontroller implementation as different results. “NR” means that the cited work does not report the value; it does not mean zero. The exact databases, queries, timestamps, and exclusion decisions are frozen in `results/literature/related-work-search-2026-09-04.md`.

## Standardization context

NIST advanced SQIsign to Round 3 of the Additional Digital Signatures process on 2026-05-14. The current [Round-3 candidate page](https://csrc.nist.gov/projects/pqc-dig-sig/round-3-additional-signatures) lists SQIsign and links its project website. The official project website now publishes [SQIsign v3.0](https://sqisign.org/spec/sqisign-20260901.pdf) and the corresponding [`nist-v3` implementation](https://github.com/SQIsign/the-sqisign/tree/nist-v3), both dated 2026-09-01. This project therefore freezes two distinct objects: v2.0.1 Level I for the central low-memory result, and v3.0 `p324_3/m4f` for a separate transfer experiment. It does not treat v3 as evidence about the v2 implementation or silently update v2 parameters.

This version qualification matters. Results for the 2023 Round-1 submission and one-dimensional variants cannot be used as direct memory or performance baselines for the current v2 higher-dimensional implementation.

## Functionality and platform matrix

K, S, and V mean a real KeyGen, Sign, and Verify path respectively. A parenthesized operation is present only on a less constrained host or is mocked in the embedded artifact.

| Work | SQIsign generation targeted | K/S/V actually implemented | Target platform | Reported measurements most relevant here |
|---|---|---:|---|---|
| [Official SQIsign v3.0](https://sqisign.org/) | NIST Round 3, v3.0 | K/S/V | x86-64, Arm64, Cortex-M4 | The specification reports Level-I-equivalent Cortex-M4 stack figures of 56/99/37 kB for K/S/V. This is the direct control for the separate v3 transfer experiment and defeats any claim that this work is the first full Cortex-M SQIsign implementation without a version qualifier. |
| [Official SQIsign v2.0/v2.0.1](https://github.com/SQISign/the-sqisign) | NIST Round 2, v2 family | Host: K/S/V. Embedded artifact: V only | x86-64 host; Cortex-M4 verification | The later m4-modarith evaluation reports Level-I reference V at 123 Mcycles, 40.3 KB code and 30.9 KB RAM. No full embedded K/S figures. |
| [pqm4 study, ePrint 2024/112](https://eprint.iacr.org/2024/112) | 2023 Round-1 submission | No embedded K/S/V integration | STM32L4R5ZI, Cortex-M4, 640 KB RAM | Preliminary x86 Level-I S stack+heap “slightly above 300 KB”; V about 12 KB. SQIsign was excluded because of GMP and dynamic allocation. |
| [Optimized One-Dimensional SQIsign Verification, ePrint 2024/1563](https://eprint.iacr.org/2024/1563) | Round-1-era one-dimensional SQIsign; both NIST parameters and new primes | Host library: K/S/V; embedded benchmark: V only, with K/S mocks | Intel; STM32L4R5ZI Cortex-M4 | New Level-I p248 smart V: 94.6 Mcycles, 61.4 KB code, 4.40 KB RAM. Uncompressed p248 V: 62.6 Mcycles, 61.8 KB code, 4.84 KB RAM. These are not v2 parameters. |
| [Paving the Way for SQIsign: Toward Efficient Deployment on 32-bit Embedded Devices](https://doi.org/10.3390/math12193147) | 2023 NIST Round-1 Level I | K/S/V | Raspberry Pi 3, Cortex-A53 in 32-bit Linux mode, 1 GB LPDDR2 | Mean K 17.1 ms, S 30.4 ms, V 0.924 ms. RAM, stack and code size NR. This is not an MCU and still uses GMP/dynamic allocation. |
| [m4-modarith, ePrint 2025/1322](https://eprint.iacr.org/2025/1322) | NIST Round-2 v2 | V only | STM32L4R5ZI Cortex-M4 | Level-I V: reference 123 Mcycles/40.3 KB code/30.9 KB RAM; generated arithmetic 61.2 Mcycles, or 54.5 Mcycles with custom fast-memory placement; 42.5 KB code/27.6 KB RAM. |
| [SQIsign with Fixed-Precision Integer Arithmetic, ePrint 2025/1649](https://eprint.iacr.org/2025/1649) | NIST Round-2 v2 | K/S/V on host | x86-64 host | Proves uniform quaternion bounds of 7026/10713/14150 bits for Levels I/III/V. Full-operation RAM/stack/code NR. Local Level-I execution is very slow and has a multi-megabyte `find_uv` stack footprint. |
| [Compact Quaternion Algorithms for SQIsign, ePrint 2026/1031](https://eprint.iacr.org/2026/1031) | NIST Round-2 v2 | K/S/V on host | Intel Core i9-13900K host | Current revision bounds 1665/2521/3319 bits. Paper Level-I compact K 127.39 ms, S 272.86 ms, V 0.92 ms; fixed baseline K 14429.32 ms, S 29093.29 ms, V 0.92 ms. RAM/stack/code NR. |
| [Qlapoti, ePrint 2025/1604](https://eprint.iacr.org/2025/1604) | NIST Round-2 v2 integration | K/S/V on host | x86-64 host | Level-I K 123 to 67.5 Mcycles; S 282 to 172 Mcycles. Massif peak heap 0.42 MiB to 38 KiB (L1), 1.9 MiB to 56 KiB (L3), 1.7 MiB to 74 KiB (L5); stack is not included. |
| [Qlapoti+ and More, ePrint 2026/1640](https://eprint.iacr.org/2026/1640) | Qlapoti-based NIST Round-2 v2 | K/S/V on host | x86-64 Broadwell | Level-I K 14.31 to 11.71 Mcycles; S 37.11 to 31.08 Mcycles; V unchanged at 2.97 Mcycles. RAM/stack/code NR. |
| [Qlapoty, ePrint 2026/1700](https://eprint.iacr.org/2026/1700) | Qlapoti-based NIST Round-2 v2 | Norm-equation solver and SQIsign S integration | Host evaluation | Re-examines the published Qlapoti analysis and pseudocode/implementation discrepancies; reports a new norm-equation algorithm, 6--9x solver speedup and 1.3--2.1x signing speedup depending on security level. No Cortex-M total-SRAM result. |
| [SQIsign on ARM, ePrint 2026/394](https://eprint.iacr.org/2026/394) | NIST Round-2 v2 | K/S/V | Armv8-A NEON: Cortex-A76/A72/A53, Apple M1/M3 | Cortex-A76 Level-I K 110.63 Mcycles, S 256.37 Mcycles, V 22.85 Mcycles. RAM/stack/code NR. This is a high-performance 64-bit NEON port, not Cortex-M. |
| [Vectorized SQIsign Using AVX-512, arXiv 2608.13948](https://arxiv.org/abs/2608.13948) | NIST Round-2 v2 | K/S/V on host | Intel Core i7-11700F, x86-64 AVX-512IFMA | Level-I C/AVX-512/AVX-512+Qlapoti respectively: K 81.54/46.21/28.11 Mcycles, S 185.29/108.05/68.81 Mcycles, V 12.54/3.94/3.94 Mcycles. GMP quaternion code is unchanged; RAM/stack/code NR. |
| [sqisign-rs 0.5.0](https://github.com/anchorageoss/sqisign-rs) | SQIsign v2.0 plus additional compact/encoding variants | K/S/V | Pure Rust `no_std` library; reported benchmark host is Apple M4 Pro | Public source reports all 300 NIST KATs across Levels I/III/V and Level-I V around 1.3--2.3 ms depending on format. Full signing uses `alloc` and `num-bigint`; no MCU K/S/V RAM or stack figure is reported. |
| [miniSQI, IEEE LSSC 2026](https://doi.org/10.1109/LSSC.2026.3720807) | One-dimensional, uncompressed SQIsign verification | V only | 28-nm custom ASIC | The authors report the first SQIsign ASIC demonstration: 0.05 mm² and 1.19–7.34 mW; a multiplier redesign gives 2× speed/energy improvement and memory organization gives 31% area savings. It is neither a software MCU result nor full v2 K/S/V. |
| [Superglue, ePrint 2025/736](https://eprint.iacr.org/2025/736) | Higher-dimensional isogeny component applicable to SQIsign v2 | Gluing-isogeny formulas only | Host proof-of-concept | Approximately 2× faster than the preceding gluing formulas. No end-to-end K/S/V, RAM, stack, code-size, or Cortex-M result is reported. |
| [A Faster Software Implementation of SQISign, ePrint 2023/753](https://eprint.iacr.org/2023/753) | Pre-NIST/Round-1-era p1973 | K/S/V on host | x86-64 host | Reports 5.47% K, 8.80% S and 25.34% V speedups for p1973. RAM/stack/code NR. |
| [Constant-time Integer Arithmetic for SQIsign, ePrint 2025/832](https://eprint.iacr.org/2025/832) | Round-2 integer requirements, up to about 12,000 bits | Integer module only | x86-64 host | Function-level cycles; no full K/S/V or MCU RAM. Uses Timecop/Valgrind. |
| [Constant-time lattice reduction in dimension 4, ePrint 2025/027](https://eprint.iacr.org/2025/027) | Round-1 SQIsign lattice-reduction use case | Dimension-4 reduction module only | x86-64 host | Generic implementation about 5× slower; optimized variants about 1.3–1.5× slower under the paper’s parameter choices. LLL is reported below 3% of signing. No full-operation RAM. |
| [Constant-time Quaternion Algorithms for SQIsign, ePrint 2025/2192](https://eprint.iacr.org/2025/2192) | NIST Round-2 v2 | Selected quaternion routines integrated into host K/S; V unaffected | x86-64 host | Level-I non-CT K 118.63 ms/S 264.06 ms versus selected CT quaternion routines K 1397.21 ms/S 3818.2 ms. The whole signer is not constant-time. |
| [Simple Power Analysis Attack on SQIsign, ePrint 2025/830](https://eprint.iacr.org/2025/830) | NIST Round-2 signing structure; practical trace experiment uses a 54-bit toy instance | Attack, not a full implementation | ChipWhisperer Nano / STM32F030 target for toy experiment | One or a few traces recover the secret-derived ephemeral exponent and enable key recovery in the demonstrated setting. No full v2 MCU K/S/V result. |
| [Adaptive Scheduling Optimization for Isogeny Mapping in SQIsign](https://doi.org/10.3390/electronics15142192) | Described as NIST-I, but exact SQIsign commit/spec is not disclosed | Instrumented S only | Apple M2 | Live fixed-degree features have AUC 0.50. Baseline S 75.8 ± 14.5 ms; Rank-ML 104.2 ± 19.9 ms. No memory result or public source was identified. |
| [Efficient Quaternion Algorithms for the Deuring Correspondence, ePrint 2026/185](https://eprint.iacr.org/2026/185) | Not a SQIsign implementation | Quaternion/Deuring algorithms only | x86-64 C++ host | Fixed-size arithmetic is demonstrated for modular-polynomial evaluation; SQIsign use is proposed as future applicability. No SQIsign K/S/V figures. |

## Dependency, allocation, constant-time, and source matrix

| Work/artifact | GMP at runtime | Dynamic allocation | Constant-time claim | Source status | Direct comparison with this project |
|---|---:|---:|---|---|---|
| Official v3 | No external GMP | The official Cortex-M implementation uses fixed-precision arithmetic; this audit does not infer a whole-program allocator-free closure from that fact alone | The specification does not claim the complete submission is constant-time | Public `nist-v3` branch | Direct within-version control for the v3 D1 lifetime overlay; not a baseline for the v2 arena reduction. |
| Official v2 | Yes | Yes, directly in applications and indirectly throughout GMP | No whole-implementation claim | Public | Conformance oracle, but cannot be linked into the final firmware. |
| pqm4 2024 audit | Yes in the excluded SQIsign host candidate | Yes | No | pqm4 public; no embedded SQIsign integration in that study | Establishes a historical ~300 KB host estimate, not a heap-free MCU signer. |
| Optimized 1D | mini-GMP in the full 32-bit host port; not in V-only build | Full K/S uses it; V-only build removes it | No whole-library claim | Public | Important prior 32-bit and Cortex-M work, but embedded K/S are explicitly unfinished and the protocol generation differs. |
| Paving the Way | Yes | Yes | Constant-time claims are limited to low-level field primitives | No standalone public artifact identified | Full 32-bit K/S/V, but on a Linux Cortex-A SBC with 1 GB external DRAM. |
| m4-modarith | No GMP in V-only artifact | No heap in measured V path | Generated finite-field arithmetic is constant-time; not a whole-verifier side-channel proof | Public | Best directly relevant Cortex-M arithmetic source, but only V. |
| Fixed-Precision | No | No production heap calls found in the linked build; very large VLAs/stack | No whole-implementation claim | Public | Variant A. It solves unbounded arithmetic but uses a global worst-case precision and does not solve peak lifetime. |
| Compact Quaternion | No | **Yes** in the frozen code: five `find_uv` allocations | No | Public | Variant B and our starting point. It improves mathematical bounds but its present storage schedule exceeds RP2350 SRAM by over an order of magnitude. |
| Qlapoti | Yes | Yes | No | Public | Shows that algorithm selection can collapse heap, but total SRAM and stack are not reported and it remains GMP-based host code. |
| Qlapoti+ | Yes | Yes | No | Public | Current speed comparator; not a low-memory or MCU result. |
| SQIsign on ARM | Yes | Yes through GMP | No whole-implementation claim | Public | Full current-generation Arm implementation, but assumes 64-bit Armv8-A and NEON. |
| AVX-512 SQIsign | Yes | Yes through the unchanged GMP quaternion layer | No implementation-level side-channel-resistance claim | No standalone source artifact identified at the freeze date | Strong current x86 speed comparator, but neither a memory nor embedded result. |
| sqisign-rs | No GMP | Yes for full K/S through Rust `alloc` and `num-bigint`; the separate dimension-2 verifier is described as zero-allocation | Verification is designed to be constant-time but not formally audited; signing is explicitly variable-time | Public | The nearest new full non-GMP implementation, but it does not demonstrate heap-free signing or measured Cortex-M K/S/V. |
| miniSQI | N/A: custom hardware | N/A: custom hardware | No complete side-channel claim identified | Publication; no public RTL/source artifact identified | Relevant IoT hardware evidence for one-dimensional V only, not software full K/S/V. |
| Superglue | N/A at formula level | N/A at formula level | No whole-implementation claim | [Public SageMath/Rust code](https://github.com/MaxDuparc/Superglue) | A promising speed input for future integration; it does not establish a low-memory operation envelope. |
| Constant-time integer | No | The described arrays are fixed precision; full integration is absent | Yes for the tested integer routines via Timecop/Valgrind | No public artifact identified from the paper | Candidate arithmetic design input, not a full low-memory signer. |
| Constant-time lattice | Uses secure low-level GMP/custom routines in its artifact | Not a heap-free full SQIsign result | Yes for the reduction module; ctgrind and RTLF tests | Public | Candidate side-channel hardening; not evidence that current full SQIsign is constant-time. |
| Constant-time quaternion | GMP remains elsewhere; lattice reduction also remains non-CT | Yes in the surrounding host integration | Only the selected quaternion algorithms | Public | Makes the low-memory/constant-time conflict measurable, but does not remove all leakage. |
| SPA attack | N/A | N/A | Demonstrates leakage instead of claiming resistance | Public attack code | Requires us to label signing variable-time and avoid any side-channel-resistance claim. |
| Adaptive scheduling | NR | NR | Explicitly notes timing and memory-access leakage risks | No public source/data found | Its “scheduling” is retry ranking, not object-lifetime scheduling; the negative live result does not supersede this project. |

## Frozen source audit

README claims were not accepted as dependency evidence. We inspected CMake link definitions, includes, source calls, and linked symbols using `rg`, `nm`, and `otool`/`objdump` where applicable.

| Frozen artifact | Result of local source/binary inspection |
|---|---|
| `work/official-v3` at `6d017708…` | Official v3.0 `p324_3/m4f` control. A clean firmware harness at `467ca5b…` compiled this source into a separate ELF/UF2/map. Five paired RP2350 rounds pass official vector 0; the source and results are isolated from v2. |
| `work/v3-lowmem-d1` at `6d017708…` plus patch 0035 | One tracked source-file change introduces two local lifetime overlays. The applied diff has SHA-256 `44e08929…`; a clean firmware harness built and measured it in the same paired campaign. |
| `external/official` at `dd133d7…` | Release Level-I binary links Homebrew `libgmp.10.dylib` and exposes numerous `__gmpz_*` references. Applications also call `calloc/free`. |
| `external/fixed-precision` at `d0cb037…` | Release Level-I production binary has no GMP or allocator references from the crypto path. A bundled mini-GMP tree exists but is not linked. `find_uv` instead materializes enormous fixed-precision VLAs on stack. |
| `external/compact-sqisign` at `5b94b09…` | Release Level-I production binary has no GMP references, but `find_uv` calls `malloc` five times and later securely frees the blocks. A bundled mini-GMP tree exists but is not linked. |
| `external/pqm4-sqisign` at `5dceca0…` | Real v2 Verify path; generated K/S API bodies are mocks. It is not evidence of full embedded SQIsign. |
| `external/sqisign-1d` / `external/sqisign-1d-pqm4` | Full 32-bit host code retains mini-GMP and dynamic allocation for K/S; pqm4 measurements execute real Verify with fixed vectors and mocked K/S. |
| `external/qlapoti` and `external/qlapoti-plus` | Full host integrations retain GMP/dynamic allocation. |

## What the search supports—and does not support

The public literature found through the freeze date contains:

- an official v3.0 implementation with full Cortex-M4 K/S/V, which is the nearest embedded comparator for the v3 transfer experiment;
- full current-v2 K/S/V implementations on ordinary host and Armv8-A systems;
- a public pure-Rust v2 K/S/V implementation that is `no_std` but still heap-allocating for signer-side big integers;
- a full x86-64 AVX-512 v2 implementation that substantially improves speed while retaining GMP;
- current-v2 Verify-only implementations on Cortex-M4;
- a full older one-dimensional 32-bit host port whose Cortex-M artifact performs only Verify;
- a custom-ASIC one-dimensional Verify implementation for IoT;
- mathematical fixed-precision and compact-quaternion results whose public full implementations target hosts;
- constant-time results for individual integer, lattice, and quaternion layers; and
- a practical SPA warning against the unprotected signer.

No public paper or source artifact found in this audit demonstrates full v2.0.1 Level-I KeyGen + Sign + Verify on a Cortex-M-class MCU with on-chip SRAM only, no GMP, no dynamic allocation, and a measured whole-firmware exclusive SRAM reservation below 520 KiB. This negative result is restricted to v2.0.1. Official v3.0 already provides full Cortex-M4 K/S/V, so no analogous absence claim is made for v3. This is a dated search result, not a “first” claim.

The strongest research comparator is not the original GMP reference alone. It is the combination of Compact Quaternion Algorithms (precision), Qlapoti (algorithmic heap reduction), the 32-bit/verification Cortex-M work (platform feasibility), and the constant-time/SPA work (security limitations). Our contribution is viable only if it adds measured total-live-SRAM reduction beyond relocating bytes between heap, stack, and globals.

## Search method and update protocol

The freeze was checked against primary sources from the NIST Additional Digital Signatures pages, the official SQIsign website/specification/repository, IACR ePrint and proceedings records, Springer proceedings, arXiv, author-linked repositories, IEEE Xplore/SSCS records, and the published Rust crate/repository. Searches combined `SQIsign` with `v3`, `Round 3`, `Cortex-M`, `STM32`, `RP2040`, `RP2350`, `microcontroller`, `bare metal`, `embedded`, `heap`, `memory`, `stack`, `KeyGen`, `Sign`, `Verify`, `AVX-512`, `Rust`, `ASIC`, and publication years 2024--2026.

Inclusion in the direct-comparison conclusion requires evidence for all of the following, not merely an embedded-compatible arithmetic kernel: the current v2 protocol/parameters, real Level-I K/S/V paths, a Cortex-M-class physical target, on-chip SRAM only, no GMP, no dynamic allocation, and a measured whole-operation memory bound below 520 KiB. A host `no_std` build, a V-only MCU/ASIC artifact, an individual-function stack figure, or a heap-only profiler result does not satisfy that conjunction.

This is a dated negative search result rather than an exhaustive proof of non-existence. It was rerun after the v3.0 package appeared. A later submission date still requires repeating the frozen queries and checking new ePrint/arXiv/CHES/IEEE records and public repositories.
