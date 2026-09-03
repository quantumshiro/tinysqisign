# SQIsign v3 fixed-key structural trace

This frozen native Darwin/arm64 campaign compares SQIsign v3.0 official and
D1 Sign execution with one message and one signing-RNG stream fixed across ten
official `p324_3` KAT keys.  Each key is copied to the same buffer address
before tracing.

- `raw.csv`: 88 rows from two process runs per implementation.
- `manifest.json`: clean source commits, exact build/link commands, archive
  digests, compiler, and input digests.
- `summary.json`: validated pairs, decisions, and claim boundary.
- `official-{a,b}.log`, `d1-{a,b}.log`: captured standard error (empty on the
  successful frozen run).

All eight same-key controls have equal edge/address counts and stream digests.
All 36 key-0-versus-other-key comparisons have different edge and address
event counts in both process runs.  This is host structural evidence, not
secret-only attribution, Cortex-M33 evidence, power/EM evidence, or a
side-channel-resistance result.

Reproduce from the research tree with:

```sh
python3 scripts/run_v3_fixed_key_structural_trace.py
```
