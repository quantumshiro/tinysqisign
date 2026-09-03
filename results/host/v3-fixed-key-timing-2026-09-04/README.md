# SQIsign v3 fixed-key host timing screen

This bounded screen isolates key identity more closely than the RP2350
multi-vector campaign.  It signs the same 33-byte public message with the same
48-byte external signing-RNG seed under ten official `p324_3` KAT secret keys.
Each selected key is copied into the same buffer before timing, so the table-row
address is not confounded with key identity.
For each of the official and D1 native reference builds, every key is measured
30 times in each of two independently shuffled schedules.  All 1,200 signatures
verify, and official/D1 signatures are byte-identical under the fixed inputs.

The official build has between-pass key-median Spearman 1.0 and D1 has
0.987879.  The fastest-to-slowest key-median span is 52.63--53.52% of the central
key median, well above the preregistered 1% and 10-times-within-key-MAD screen.
This is positive evidence of key-associated timing on the recorded
Darwin/arm64 reference implementation.  It is not an RP2350/m4f result, a
physical trace, a key-recovery experiment, or evidence of resistance.
The encoded secret key includes its corresponding public key, so this screen
does not attribute the difference to the non-public component alone.

Reproduce and analyze with:

```sh
python3 scripts/run_v3_fixed_key_timing.py --repetitions 30
python3 scripts/analyze_v3_fixed_key_timing.py
```

`manifest.json` records source commits, linked archive digests, compilation
commands, host metadata, and the exact randomized schedules.  `raw.csv` is the
full timing corpus; `per-key.csv` and `summary.json` are deterministic analysis
outputs.
