# RP2350 result index

The paper uses only the frozen, terminal-result records listed below. Earlier
development captures remain historical and are not silently substituted.

## v2 D13 proposed image

- `ksv-d13-dc3289a-manifest.json`: linked memory accounting, first target
  capture, and exact artifact hashes.
- `artifacts/ksv-d13-dc3289a/`: the flashed ELF, UF2, BIN, and map.
- `ksv-d13-dc3289a.txt`: first complete deterministic K/S/V boot.
- `ksv-d13-repeat-2026-09-04.txt`: independent second boot of the same UF2 and
  deterministic input.
- `ksv-d13-repeat-2026-09-04-summary.json` and `-measurements.csv`: fail-closed
  two-boot comparison. Both boots pass and reproduce transcript and PSP/MSP
  extents. This is not a multiple-input or worst-case result.

## Side-channel diagnostics

Files beginning `sca-` are bounded timing/structural diagnostics and component
prototypes. The aggregate inventory retains 12 open surfaces and explicitly
sets side-channel resistance to false. `sca-analog-readiness-2026-09-04.json`
records that no current/EM probe or oscilloscope/SCA acquisition device was
available and that zero physical traces were collected.

SQIsign v3 target results are kept separately under `../v3/rp2350/` so that
version-dependent quantities are not compared as if they came from one
implementation.
