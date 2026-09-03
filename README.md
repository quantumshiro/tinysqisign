# TinySQIsign

TinySQIsign is the public artifact for Hiro Nakanishi's paper on a low-memory
SQIsign v2 implementation and a local transfer of lifetime scheduling to
SQIsign v3. It contains the Japanese manuscript, reconstruction patches,
analysis programs, machine-readable certificates, clean firmware artifacts,
and raw RP2350 captures needed for the paper-facing claims.

Contact: Hiro Nakanishi, independent researcher,
`quantumsity@protonmail.com`.

## Main results

- SQIsign v2.0.1 Level I/RADIX32: the operation arena falls from 353,008 to
  172,080 bytes (−180,928 bytes, −51.2532%). The final RP2350 image reserves
  321,408 of 532,480 on-chip SRAM bytes and leaves 211,072 bytes unreserved,
  without GMP, heap allocation, or external PSRAM.
- v2 correctness: 12 frozen reference/proposed transcripts agree. Two fresh
  processes each pass all 100 official-request vectors against the same-commit
  ordinary API, including Open, modified-signature rejection, guards, and
  clearing. This is a differential conformance test using official requests;
  it is not equality to the historical official response file.
- v2 target: the exact archived UF2 completes one deterministic KeyGen, Sign,
  and Verify path in two boots with identical transcripts and PSP extents.
  This is not a multiple-input or worst-case stack result.
- SQIsign v3.0 `p324_3/m4f`: two lifetime overlays reduce measured Sign PSP
  depth from 101,060 to 97,132 bytes. Five clean-firmware pairs and a separate
  10-vector × 2-placement campaign reproduce the 3,928-byte reduction; all 40
  valid K/S/V and 40 modified-signature rejection trials pass.
- A separate fixed-frame v3 prototype removes 19 compiler dynamic-frame
  records. Its linked K/S/V PSP bounds, including one conservative 212-byte
  Secure exception entry, are 108,300/127,932/40,468 bytes. A clean target
  vector observes 62,096/101,060/40,252 bytes. Handler callbacks and IRQ/MSP
  nesting remain outside this certificate, so it is not a whole-program bound.
- A preregistered v3 RP2350 fixed-key screen completes 200/200 verified
  signatures. Both official and overlay images reproduce key-associated
  wall-clock timing across two key orders (rank Spearman 1.0; key-median spans
  50.84--51.38%). Public and secret key components vary together, so this does
  not isolate secret-only causation.

## Claim boundary

This artifact does **not** claim constant-time execution, analog power/EM
resistance, key-recovery resistance, production-ready randomness, a
whole-program worst-case stack bound, or minimum memory. Software and timing
screens find residual variable behavior. Physical trace count is zero because
the present bench has no current/EM probe or scope/SCA acquisition instrument.
Other PQC schemes and other microcontrollers are outside this revision and are
not used as evidence of generality.

The complete state machine is in
[`future-work-status.json`](results/revision-2026-09-04/future-work-status.json):
all locally executable bounded campaigns are complete, while
`all_research_goals_achieved`, `whole_program_worst_case_stack_bound_established`,
and `side_channel_resistance_established` remain false.

## Repository map

- [`manuscript/`](manuscript/): LaTeX, monochrome TikZ figures, submission
  metadata/checklist, generated evidence rows, and the built PDF.
- [`patches/`](patches/): v2 and v3 reconstruction patches and bundles.
- [`experiments/`](experiments/): KAT/differential, lifetime-layout,
  mini-GMP, and side-channel experiment contracts and small harnesses.
- [`scripts/`](scripts/): source reconstruction, analyzers, evidence
  generators, target build/capture helpers, and ELF/stack audits.
- [`results/`](results/): frozen JSON/CSV, serial captures, firmware artifacts,
  certificates, and literature-search log.
- [`REPRODUCIBILITY.md`](REPRODUCIBILITY.md): exact reproduction routes and
  the distinction between checking frozen evidence and rerunning hardware.

## Fast verification

```sh
shasum -a 256 -c SHA256SUMS
python3 scripts/generate_manuscript_evidence.py
python3 scripts/generate_future_work_status.py \
  --require-v2-repeat --require-local-complete
make -C manuscript eprint-check
```

Reconstruct the external SQIsign sources with:

```sh
./scripts/prepare_sources.sh
```

See [`REPRODUCIBILITY.md`](REPRODUCIBILITY.md) before rebuilding firmware:
the frozen captures bind exact historical firmware commits and toolchains, and
new measurements must be stored separately rather than overwriting them.

## Licenses

SQIsign-derived source conditions and attribution notices are under
[`LICENSES/`](LICENSES/). The DPE portion is subject to GNU LGPL v3. The paper
license is intentionally left for the author to choose in the ePrint form.
