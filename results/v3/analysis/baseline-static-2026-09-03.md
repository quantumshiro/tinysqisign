# SQIsign v3.0 RP2350 baseline static audit

- Upstream source: official `nist-v3`, commit
  `6d017708db403bf83977fa70770fc4f7f9e9ff21`.
- Parameter set and generated implementation: `p324_3/m4f`.
- Target: Raspberry Pi Pico 2, `rp2350-arm-s`, Arm GNU Toolchain
  15.2.1, Pico SDK 2.3.0.
- ELF: `build-rp2350-v3-official/sqisign_rp2350_v3_baseline.elf`.
- ELF size (`arm-none-eabi-size`): text 220,296 B, data 0 B,
  bss 135,736 B. The bss total includes the 131,072-B measurement PSP.
- Linker bounds: `__bss_end__=0x20022d38`,
  `__StackLimit=0x20080000`, `__StackTop=0x20082000`.
- Heap section: 0 B.
- GCC stack-usage files: 125.
- Dynamic stack-usage records: 19 (17 unbounded in GCC output and two marked
  `dynamic,bounded`).
- Largest fixed frame: `quat_lll_dual_reduce_ideal`, 34,816 B.
- `protocols_sign` fixed frame: 20,008 B.
- Allocator-related symbols retained by newlib diagnostics: `_sbrk`,
  `_malloc_r`, `_realloc_r`, `_sbrk_r`, `_free_r`. These are a linkage fact,
  not evidence that a successful K/S/V path allocated heap memory; the linked
  heap section is zero bytes.

The original development transcripts are omitted from the current artifact
tree because their firmware harness was dirty. Current target evidence is the
clean official capture series under
`results/v3/rp2350/interleaved-clean-2026-09-04/`; this file is retained only
for the compiler/static baseline inventory.
