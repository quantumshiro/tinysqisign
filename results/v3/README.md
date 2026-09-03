# SQIsign v3 results

This directory is reserved for SQIsign version 3.0 (`nist-v3`, 2026-09-01)
artifacts. It is deliberately separate from the existing SQIsign v2/D13
artifacts.

- `host/`: official v3 and v3-D1 host validation results.
- `rp2350/`: RP2350 build manifests and serial captures.
- `analysis/`: v2-versus-v3 deltas and lifetime-scheduling analyses.
- `version-isolation-manifest.json`: v2、公式v3、lifetime版、固定frame版の
  exact commit/treeと、現行clean campaignの対応。

The unmodified v3 source is checked out at `work/official-v3`; the clean
lifetime and fixed-frame commits are reconstructed at `work/v3-lowmem-d1` and
`work/v3-static-stack-d2`. The v2 D13 source remains at `work/compact-d13`.
Thin Git bundles preserve the exact v3 commits and review patches preserve the
human-readable diffs.

Run `scripts/test_sqisign_v3_host.sh` from any directory to rebuild and repeat
the NIST API test, self-test, and official KAT for both clean v3 trees. The script deliberately
runs each KAT from its source-tree test directory because the upstream test
resolves the official response file through a relative path.

`rp2350/interleaved-clean-2026-09-04/` contains five paired official-v3/v3-D1
measurements (ten fresh boots) from a clean firmware-harness commit.  Odd
rounds use official-v3 then v3-D1; even rounds reverse the order.  Recreate the
machine-readable summary with `scripts/analyze_rp2350_v3_interleaved.py`.  All
ten captures pass the embedded official KAT. The superseded dirty-firmware
captures are omitted from the current tree and remain recoverable from Git
history; they are not used for paper values.

`rp2350/multi-input-placement-clean-2026-09-04/` extends the bounded check to
official vectors 0--9, official/D1, and two linked-code placements.  Its 40
positive K/S/V trials and 40 modified-signature rejection trials pass, with no
PSP-depth change after the 1,024-byte code shift.  This remains one board and
ten deterministic vectors, not an input-population or worst-case result.

`rp2350/d2-static-2026-09-04/`,
`analysis/d2-linked-stack-bound-audit-2026-09-04.json`, and
`rp2350/static-closure-clean-2026-09-04/` contain the fixed-frame D2 prototype,
the linked operation-PSP certificate, and its clean target cross-check.  The
image-specific K/S/V bounds are 108,300/127,932/40,468 bytes and include a
212-byte allowance for one maximum Secure Armv8-M exception entry.  The
official vector-0 capture observes 62,096/101,060/40,252 bytes, respectively,
and terminates with `status=PASS`.  This is not a whole-program bound: four
candidate interrupt roots have closed direct-call metadata, but 18 unique
indirect callback sites, enabled-IRQ/nesting assumptions, handler frames, and
live MSP amounts remain unresolved.

`rp2350/fixed-key-timing-clean-2026-09-04/` contains the predeclared target
screen with ten official keys, one fixed-address key buffer, fixed message and
signing RNG, XIP-cache invalidation before each timed operation, two key
orders, and five repetitions per key/order.  All 200 signatures verify.  Both
official and lifetime-overlay images satisfy the predeclared repeatability
rule (between-pass key-rank Spearman 1.0; key-median spans 50.84--51.38%).
The Sign PSP values are 101,060 and 97,132 bytes in every sample.  This is a
one-board key-associated wall-clock result, not secret-only attribution,
analog leakage, key recovery, worst-case timing, or side-channel resistance.

Only within-version differences are used to attribute an effect to a source
transformation.  Cross-version values are reported descriptively because v3
changes the parameters and several core algorithms as well as the memory
ownership model.
