# SQIsign embedded-memory literature search log

Freeze time: 2026-09-04 03:49 JST (UTC+09:00)

Purpose: rerun the negative prior-art search after the public SQIsign v3.0
release and preserve the closest counterexamples.  This is a reproducible dated
search, not a proof that unpublished or unindexed work does not exist.

## Claim tested

The narrow negative claim is restricted to SQIsign **v2.0.1 Level I** and asks
whether a public paper or source artifact jointly demonstrates:

1. real KeyGen, Sign, and Verify;
2. a Cortex-M-class physical microcontroller;
3. on-chip SRAM only;
4. no GMP;
5. no dynamic allocation in the measured cryptographic closure; and
6. a measured whole-firmware exclusive SRAM reservation below 520 KiB.

Failure to meet any one item excludes a work from the conjunction, but does not
make that work irrelevant.  No analogous absence claim is made for v3 because
the official v3.0 package already reports full Cortex-M4 K/S/V.

## Sources and query strings

The following primary-source collections were checked manually and through
their site search or a web index restricted to the named domain.

| Collection | Queries frozen on 2026-09-04 | Result used |
|---|---|---|
| [NIST Additional Digital Signatures](https://csrc.nist.gov/projects/pqc-dig-sig/round-3-additional-signatures) | `SQIsign Round 3`; `SQIsign specification implementation`; `site:csrc.nist.gov SQIsign Round 3 2026` | SQIsign is a Round-3 candidate. The page was updated 2026-07-29 and links the project website. |
| [Official SQIsign site](https://sqisign.org/) | `SQIsign version 3.0`; inspection of all specification and implementation links | v3.0 specification and implementation are dated 2026-09-01; v2.0.1 remains available separately. |
| [Official SQIsign repository, nist-v3](https://github.com/SQIsign/the-sqisign/tree/nist-v3) | branch/tag list; `Cortex-M4`, `pqm4`, `p324_3`, `stack`, `malloc`, `GMP` | Official v3 is the direct full-MCU counterexample for v3, not v2. |
| [IACR ePrint](https://eprint.iacr.org/) and IACR publication records | `SQIsign Cortex-M`; `SQIsign microcontroller`; `SQIsign embedded memory`; `SQIsign stack heap`; `SQIsign KeyGen Sign Verify`; `SQIsign constant time`; `SQIsign power analysis`; years 2024--2026 | Found pqm4, 1D Verify, m4 arithmetic, fixed precision, Qlapoti/Qlapoty, Arm, constant-time components, and SPA. No work met all six v2 criteria. |
| [Springer proceedings](https://link.springer.com/) | exact titles `SQIsign with Fixed-Precision Integer Arithmetic`; `Simple Power Analysis Attack on SQIsign` | Fixed-precision paper is PKC 2026, LNCS 16553, pp. 3--30. SPA paper is AFRICACRYPT 2025, LNCS 15651, pp. 245--269. |
| [arXiv](https://arxiv.org/) | `SQIsign AVX-512`; `SQIsign embedded`; `SQIsign microcontroller`; `SQIsign memory` | AVX-512 is full host K/S/V and keeps the GMP quaternion layer; it is not an MCU-memory result. |
| [GitHub](https://github.com/) and author-linked repositories | `SQIsign Cortex-M`; `SQIsign no_std`; `sqisign-rs`; `SQIsign RP2350`; source links from papers | `sqisign-rs` is full host K/S/V without GMP but uses Rust allocation and `num-bigint` for signing; public Cortex-M artifacts found were Verify-only or official v3. |
| IEEE Xplore / SSCS records | `SQIsign ASIC`; `miniSQI`; `SQIsign embedded` | miniSQI is a one-dimensional Verify ASIC, not software v2 K/S/V. |

Additional cross-product terms used with `SQIsign` were `STM32`, `RP2040`,
`RP2350`, `bare metal`, `on-chip SRAM`, `malloc`, `allocator`, `GMP`, `heap`,
`watermark`, `RAM`, `KeyGen`, `Sign`, `Verify`, `32-bit`, `Rust`, `ASIC`, and
`AVX-512`.

## Closest counterexamples retained

| Counterexample class | Closest work | Why it does not satisfy the narrow v2 conjunction |
|---|---|---|
| Full K/S/V on MCU | Official SQIsign v3.0 | It targets v3.0, whose algorithms, parameters, integer representation, and memory ownership differ from v2.0.1. It invalidates a generation-agnostic “first Cortex-M SQIsign” claim. |
| Full K/S/V, non-GMP | `sqisign-rs` | Host evidence; full signing uses `alloc` and `num-bigint`; no measured Cortex-M K/S/V memory envelope. |
| Full K/S/V, 32-bit | *Paving the Way for SQIsign* | Cortex-A53 Linux SBC with 1 GB LPDDR2, GMP, and dynamic allocation; not Cortex-M or on-chip-only SRAM. |
| Full K/S/V, optimized host | SQIsign on ARM; AVX-512 SQIsign; Qlapoti/Qlapoty | Host-class Armv8-A/x86 systems and/or GMP/dynamic allocation; no whole-MCU SRAM reservation. |
| Cortex-M software | pqm4 study; optimized 1D Verify; m4-modarith | SQIsign integration or measurement is Verify-only, or K/S are mocks; parameter generation may also differ. |
| Embedded hardware | miniSQI ASIC | One-dimensional Verify-only custom hardware; neither software Cortex-M nor full v2 K/S/V. |
| Fixed storage | Fixed-precision and Compact Quaternion Algorithms | Host implementations; fixed precision alone leaves a multi-megabyte lifetime schedule, while the compact public source dynamically allocates the candidate tables. |

## Decision

At the freeze time, no indexed public item met all six v2.0.1 criteria.  The
paper may therefore report this narrow negative search result if it also links
this log, retains the counterexamples above, avoids “world first,” and states
the freeze date.  It must not generalize the result to v3.0.

The search must be rerun if the manuscript is submitted after this freeze date.
