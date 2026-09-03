# v2 lifetime certificate

`lifetime-certificate-v2.csv` is the auditable inventory for the principal
overlays in the frozen D13 Level-I/RADIX32 implementation.  Sizes and
alignments are Arm Cortex-M33 ABI values.  Source locations refer to candidate
commit `71099e0827d3f0a3b3c705d2eda592c401e0d57d`.

The certificate distinguishes three facts that are easy to conflate:

1. A `union` and its static assertions establish address reuse and extent.
2. Source control flow and pointer-use review establish the intended temporal
   contract at the frozen commit.
3. guards, zero scans, sanitizers, differential tests, and the final ELF can
   detect violations on exercised paths but do not prove all C executions.

In particular, the outer Sign loop does **not** clear all 172,080 bytes before
each retry.  Its contract relies on the active callee finalizing or clearing
its state and on the next phase initializing or overwriting the reused member.
The complete Sign owner is cleared only at the public workspace entry's
terminal exit.  This limitation is included explicitly so that a reviewer can
distinguish terminal zeroization from per-retry zeroization.

Run `scripts/check_lifetime_certificate.py` to validate the schema, target-size
assertions, key source anchors, union offsets, terminal clears, and a bounded
source scan for obvious workspace-pointer escapes.  The generated result is
`lifetime-certificate-check.json`.  This checker is an annotation-driven audit
prototype, not a sound alias analysis.
