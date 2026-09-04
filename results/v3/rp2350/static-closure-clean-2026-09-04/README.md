# v3 D2 static-stack closure cross-check

This directory binds one clean RP2350 `p324_3/m4f` vector-0 K/S/V capture to
the linked D2 operation-PSP certificate. The firmware commit is
`e564a6413766f1f299db3db1d706478c42f1cc96`; the materialized v3 source commit
is `cb94f242ba791a4ccb980b46c917830b309a9832`. Both trees report `dirty=0`.

The capture terminated with `status=PASS`, matched the official vector, used
no heap section, and observed KeyGen/Sign/Verify PSP extents of 62,096,
101,060, and 40,252 bytes. The corresponding image-specific static bounds are
108,300, 127,932, and 40,468 bytes. Each bound includes a conservative
212-byte allowance for one maximum Secure Armv8-M exception entry and fits the
131,072-byte PSP reservation.

`summary.json` checks the capture identity, final ELF/map/archive/UF2 hashes,
UF2 Secure-Arm metadata, compiler stack records, linked call-graph audit, and
the inequalities `observed PSP <= static PSP bound <= reservation`. The
companion asynchronous audit closes direct-call stack metadata from four
candidate interrupt roots but leaves 18 unique indirect callback sites,
enabled-IRQ/nesting assumptions, and live MSP amounts unresolved. Therefore
this is not a whole-program worst-case stack bound.

The files under `artifacts/d2/` are the analyzed binaries. The original
build cache is intentionally represented by its hash in `summary.json` rather
than copied as a portable build recipe; source reconstruction and analysis
commands are documented at repository level.
