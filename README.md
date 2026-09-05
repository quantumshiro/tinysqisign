# TinySQIsign

TinySQIsign is the public artifact for Hiro Nakanishi's paper on a low-memory
SQIsign v2 implementation and a local transfer of lifetime scheduling to
SQIsign v3. It contains Japanese and English manuscripts, reconstruction patches,
analysis programs, machine-readable certificates, commit-pinned firmware artifacts,
and raw RP2350 captures needed for the paper-facing claims.

Contact: Hiro Nakanishi, independent researcher,
`quantumsity@protonmail.com`.

## Main results

- SQIsign v2.0.1 Level I/RADIX32: the operation arena falls from 353,008 to
  172,080 bytes (−180,928 bytes, −51.2532%). The measured RP2350 image reserves
  321,408 of 532,480 on-chip SRAM bytes and leaves 211,072 bytes unreserved,
  without GMP, heap allocation, or external PSRAM.
- v2 correctness: 12 fixed reference/proposed transcripts agree. Two fresh
  processes each pass all 100 official-request vectors against the same-commit
  ordinary API, including Open, modified-signature rejection, guards, and
  clearing. This is a differential conformance test using official requests;
  it is not equality to the historical official response file.
- v2 target: the measured UF2 completes one deterministic KeyGen, Sign, and
  Verify path in two boots with identical transcripts and PSP extents. A linked
  call-graph certificate for a commit-pinned rebuild with byte-identical `.text` bounds
  the three operation PSP paths at 92,192/120,664/20,980 bytes, including one
  212-byte Secure exception entry. Handler software, IRQ nesting, and live MSP
  remain outside that certificate.
- SQIsign v3.0: the two-function schedule overlays three pairs in two functions.
  Host API, self-test, and 100-response checks pass for both implementations
  and all three official parameter sets. The corpus contains 300 distinct
  parameter-set/request pairs, each run on both implementations, for 600
  implementation-vector runs in total. All six Arm frame comparisons decrease.
  A commit-pinned `p324_3/m4f` target capture reduces Sign
  PSP from 101,060 to 96,396 bytes (−4,664 bytes, −4.6151%). Whole-image target
  measurements for the other two parameter sets are not claimed.
- A separate fixed-frame v3 analysis build removes 19 compiler dynamic-frame
  records. Its linked K/S/V PSP bounds, including one conservative 212-byte
  Secure exception entry, are 108,300/127,932/40,468 bytes. A commit-pinned target
  vector observes 62,096/101,060/40,252 bytes. Handler callbacks and IRQ/MSP
  nesting remain outside this certificate, so it is not a whole-program bound.
- A preregistered v3 RP2350 fixed-key screen completes 200/200 verified
  signatures. Both official and overlay images reproduce key-associated
  wall-clock timing across two key orders (rank Spearman 1.0; key-median spans
  50.84--51.38%). Public and secret key components vary together, so this does
  not isolate secret-only causation.

## Scope of evidence

This artifact does **not** claim constant-time execution, analog power/EM
resistance, key-recovery resistance, production-ready randomness, a
whole-program worst-case stack bound, or minimum memory. Software and timing
screens find residual variable behavior. Physical trace count is zero because
the present bench has no current/EM probe or scope/SCA acquisition instrument.
Other PQC schemes and other microcontrollers are outside this study and are
not used as evidence of generality.

## Repository map

- [`manuscript/`](manuscript/): LaTeX, monochrome TikZ figures, submission
  metadata/checklist, generated evidence rows, and the built PDF.
- [`patches/`](patches/): v2 and v3 reconstruction patches and bundles.
- [`experiments/`](experiments/): KAT/differential, lifetime-layout,
  mini-GMP, and side-channel experiment contracts and small harnesses.
- [`scripts/`](scripts/): source reconstruction, analyzers, evidence
  generators, target build/capture helpers, and ELF/stack audits.
- [`results/`](results/): machine-readable JSON/CSV, serial captures, firmware artifacts,
  certificates, and literature-search log.
- [`REPRODUCIBILITY.md`](REPRODUCIBILITY.md): source reconstruction routes and
  the distinction between checking published evidence and rerunning hardware.

## Fast verification

```sh
shasum -a 256 -c SHA256SUMS
python3 scripts/generate_manuscript_evidence.py
make -C manuscript eprint-check
```

Reconstruct the external SQIsign sources with:

```sh
./scripts/prepare_sources.sh
```

See [`REPRODUCIBILITY.md`](REPRODUCIBILITY.md) before rebuilding firmware:
the published captures bind specific firmware commits and toolchains, and
new measurements must be stored separately rather than overwriting them.

## Licenses

SQIsign-derived source conditions and attribution notices are under
[`LICENSES/`](LICENSES/). The DPE portion is subject to GNU LGPL v3. The paper
license is intentionally left for the author to choose in the ePrint form.
