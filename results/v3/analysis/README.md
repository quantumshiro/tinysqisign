# v3 stack-analysis evidence

This directory separates three claims that must not be merged.

- `static-stack-prototype-manifest.json` records the generated-source rewrite
  that replaces the `p324_3/m4f` variable-length local arrays with checked
  parameter-specific maxima.
- `d2-linked-stack-bound-audit-2026-09-04.json` binds compiler `.su` records,
  linked Arm disassembly, fixed assembly-frame certificates, veneer targets,
  and rank bounds for all three reachable recursive components.  It also adds
  a conservative 212-byte allowance for one maximum Secure Armv8-M exception
  frame (52 words plus one alignment word).  For this one clean ELF it
  establishes PSP bounds of 108,300, 127,932, and 40,468 bytes for KeyGen,
  Sign, and Verify, respectively.
- `d2-async-stack-closure-audit-2026-09-04.json` examines four linked USB CDC
  and alarm-pool interrupt candidates.  All directly reachable compiler frames
  are static and no direct-call target lacks stack metadata after literal
  veneers are resolved.  Eighteen unique indirect callback sites remain, and
  handler callback targets, interrupt nesting, live MSP depth, and exhaustive
  runtime RAM-vector contents are not bounded.

Consequently, the first audit is an image-specific operation-PSP certificate
including one maximum exception-entry frame.  Neither audit establishes a
whole-program worst-case stack
bound.  The RP2350 cross-check in
`../rp2350/static-closure-clean-2026-09-04/` compares one official vector-0
execution with the synchronous bounds; it is not a proof over all inputs.
