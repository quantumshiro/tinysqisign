# SQIsign v3 RP2350 fixed-key timing screen

This directory records the predeclared, clean-build RP2350 campaign run on
2026-09-04.  It compares the official `p324_3/m4f` implementation with the
lifetime-overlay adaptation on one RP2350 board.

## Frozen design

- 10 official keys, copied into the same fixed-address RAM buffer before each
  measurement;
- one fixed 33-byte public message and one fixed `a5-sequence-v1` signing-RNG
  stream;
- XIP cache invalidation immediately before every timed Sign and Verify;
- passes A and B use independently generated deterministic key orders;
- 5 repetitions per key and pass, hence 100 Sign samples per implementation
  and 200 in total;
- every signature, stable per-key signature digest, canary, terminal marker,
  firmware/source identity, and artifact digest is checked by the analyzer.

The design and binary digests were frozen before target capture in
`experiments/sca/v3-rp2350-fixed-key-timing-design.json`.  The outer image
order was official first and adapted second; therefore the paired execution-
time delta between implementations is descriptive, not a controlled speed
comparison.

## Result

All 200/200 signatures completed and verified.  The predeclared timing screen
was positive for both implementations:

| implementation | pass A key-median span | pass B key-median span | A/B rank Spearman | Sign PSP |
|---|---:|---:|---:|---:|
| official | 50.8419% | 50.8514% | 1.0 | 101,060 B |
| lifetime overlay | 51.3752% | 51.3626% | 1.0 | 97,132 B |

Each span also exceeded ten times the median within-key MAD by a wide margin.
The adapted image retained the 3,928-byte Sign PSP reduction in all 100
samples.  The fixed outer image order and uncontrolled environmental drift
mean that its observed median speed delta (about -0.89%) must not be treated
as a general performance result.

## Claim boundary

This is repeatable **key-associated wall-clock timing** under one finite target
design.  The key pairs contain both public and secret components, so the
campaign does not isolate secret-only causation.  It is not an analog power or
EM measurement, a target control/address trace, a key-recovery attack, a
population or worst-case estimate, or evidence of side-channel resistance.
The machine-readable decision in `summary.json` therefore keeps
`physical_leakage_established`, `key_recovery_established`, and
`side_channel_resistance_established` false.

## Files

- `baseline.txt`, `d1.txt`: raw serial captures;
- `measurements.csv`: one row per accepted operation measurement;
- `per-key.csv`: per-key/pass robust timing summaries;
- `summary.json`: validation, provenance, artifact hashes, decision rule, and
  claim boundary.

Reproduce the analysis with:

```sh
python3 scripts/analyze_rp2350_v3_fixed_key.py \
  results/v3/rp2350/fixed-key-timing-clean-2026-09-04 \
  --baseline-build build-rp2350-v3-baseline-sca-key \
  --d1-build build-rp2350-v3-d1-sca-key \
  --size-tool /opt/homebrew/bin/arm-none-eabi-size \
  --expected-firmware-commit 899e572f084c90b7b3be30222e340dc42e380876 \
  --expected-d1-commit 9293313fb58de4c5ce9dd27a5a9fde0058766c79 \
  --d1-patch patches/0035-experiment-v3-d1-lifetime-overlays.patch \
  --design experiments/sca/v3-rp2350-fixed-key-timing-design.json
```
