# Clean-commit RP2350 v3 paired campaign

This directory supersedes the 2026-09-03 development campaign. Those captures
recorded `firmware_dirty=1`, are omitted from the current tree, and remain
recoverable from Git history. Every
capture here records the same clean firmware harness commit and passes the
embedded official-vector gate.

## Frozen inputs

- Firmware harness: `quantumshiro/tinysqisign` commit
  `467ca5b61a5f6810218ee173850862d629cf07a7`; `firmware_dirty=0` in all ten
  captures.
- SQIsign v3 source: official `nist-v3` commit
  `6d017708db403bf83977fa70770fc4f7f9e9ff21`.
- Official image: no tracked source changes (`v3_dirty=0`).
- D1 image: the same source commit plus only
  `patches/0035-experiment-v3-d1-lifetime-overlays.patch`; the patch SHA-256 is
  recorded in `summary.json`.  The generated `src/pqm4` tree is an untracked
  build input in both source checkouts and is not an additional D1 change.
- Pico SDK: tag 2.3.0, commit
  `98a542c1a62fb549ffb5d66a3e5892b06276b670`.
- Compiler: Arm GNU Toolchain 15.2.Rel1, GCC 15.2.1.
- Board and clock: one Raspberry Pi Pico 2, RP2350 Arm secure target, one core,
  150 MHz.
- Workload: official SQIsign v3.0 `p324_3` known-answer vector 0.  Each capture
  boots once and compares public key, secret key, signed message, and recovered
  message byte for byte before reporting `PASS`.

## Design and interpretation

There are five paired rounds.  Odd rounds flash official then D1; even rounds
flash D1 then official.  Thus there are ten independent boots.  This controls
first-order ordering drift, but it remains a single-board, single-vector,
descriptive repetition.  It does not estimate an input distribution or a
cross-device distribution.

`summary.json` binds every serial capture and the measured ELF, UF2, link map,
cryptographic archive, stack-usage record, and D1 patch by SHA-256.  The
analyzer rejects a capture if `firmware_dirty` is nonzero, the firmware commit
differs, the banner or build identity differs, any known-answer comparison
fails, or a pair is missing.  `measurements.csv` is the normalized long-form
table.  `manuscript/generated/v3-results-rows.tex` is generated from this
summary for the paper.

The measured Sign PSP extent is 101,060 bytes for official v3 and 97,132 bytes
for D1 in every round, a reduction of 3,928 bytes.  The paired Sign-time change
has median +0.4190% and range +0.4112% to +0.4204%.  These numbers replace the
dirty-tree campaign values.

## Reproduction

Starting from the frozen clean firmware commit, prepare the official v3 source
at the frozen source commit and a second checkout with only patch 0035 applied.
Then run the two build scripts with distinct build roots, followed by the paired
campaign and analyzer.  The analyzer invocation used here is recoverable from
its required command-line arguments and the paths stored in `summary.json`.
The relevant entry points are:

```text
scripts/build_rp2350_v3_baseline.sh
scripts/build_rp2350_v3_d1.sh
scripts/run_rp2350_v3_interleaved_campaign.sh
scripts/analyze_rp2350_v3_interleaved.py
```

The physical campaign requires a Pico 2 in BOOTSEL mode when prompted and a
serial device accepted by the capture scripts.
