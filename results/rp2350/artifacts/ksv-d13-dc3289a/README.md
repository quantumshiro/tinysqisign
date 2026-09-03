# Frozen D13 RP2350 KeyGen + Sign + Verify image

These are the exact files flashed for the deterministic D13 one-boot
KeyGen-then-Sign-then-Verify capture. They were built from clean project commit
`dc3289add3213cc7671f9943dfaa3bac770b2709` (tree
`6a65878fd835a93c588f9730e53efa0aa169e8cf`) and clean Compact D13 commit
`71099e0827d3f0a3b3c705d2eda592c401e0d57d`.

The target is `pico2` / `rp2350-arm-s`, Level I, RADIX32, Pico SDK 2.3.0,
Release, built with Arm GNU Toolchain 15.2.Rel1 (GCC 15.2.1). The firmware
uses the deterministic AES-256 CTR-DRBG test adapter; it is not a production
entropy configuration.

| File | Bytes | SHA-256 |
|---|---:|---|
| `sqisign_rp2350_ksv.elf` | 2,041,276 | `3ed410dc2e5fa2d465dac3d93cf5d3f61678693638cc9cb35c7920ca9883e29f` |
| `sqisign_rp2350_ksv.uf2` | 577,536 | `6971b9c84a42e26f08d761becd29c2c0e78b4ac1927ae19feb6b2f5c1a035a9f` |
| `sqisign_rp2350_ksv.bin` | 288,384 | `148b75481516ad4bdf0454c282c528704a7d0f0ae99844708260be9650012a7c` |
| `sqisign_rp2350_ksv.elf.map` | 737,390 | `8892077f26724e54e6ace740a41b509ad906079244b6fd8226bb0f20db100eed` |

The clean build's `scripts/audit_rp2350_ksv_elf.sh` gate accepted the ELF
archived here with a zero-byte heap, 52 static-stack Compact objects, no
allocator/GMP/system-RNG or legacy
large-stack path, a 172,208-byte guarded operation owner, a 131,072-byte PSP
reservation, and 211,072 bytes left unreserved after the separate 8,192-byte
MSP reservation.

Two matching Pico 2 boots complete KeyGen, Sign and Verify with `status=PASS`.
The first reports K/S/V times of 2,698.150029, 7,611.258527 and 0.814341
seconds; the second reports 2,698.150324, 7,611.257377 and 0.814356 seconds.
Both observe PSP extents of 91,980, 120,452 and 20,768 bytes and a 2,396-byte
conservative MSP upper extent. Transcript digests, operation clearing and
Verify RNG-nonuse checks pass and reproduce across boots. These are two runs
of one deterministic input on one board, not an input or performance
distribution, worst-case stack bound, or security claim.

The ELF debug information and map preserve their original absolute build
paths, so their byte hashes apply to these archived originals. The matching
capture manifest and checker bind the physical report to all four files.
