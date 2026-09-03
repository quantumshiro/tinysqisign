# Version freeze

Freeze date: **2026-08-19 (Asia/Tokyo)**.

## Standardization and protocol target

NIST advanced SQIsign, together with eight other signature candidates, to Round 3 on 2026-05-14. NIST explicitly describes the next submissions as an opportunity for updated specifications and implementations (“tweaks”); it does not yet publish a Round-3 SQIsign package on the candidate page.

- NIST status: [Round 3 advancement announcement](https://csrc.nist.gov/news/2026/nist-advances-9-candidates-to-the-3rd-round-of-pqc)
- NIST candidate page: [Round 3 Additional Signatures](https://csrc.nist.gov/projects/pqc-dig-sig/round-3-additional-signatures)
- Latest SQIsign specification found: [v2.0.1, 2025-07-07](https://sqisign.org/spec/sqisign-20250707.pdf)
- Previous stable specification: [v2.0, 2025-02-05](https://sqisign.org/spec/sqisign-20250205.pdf)
- NIST Round-2 implementation tag: `nist-v2` at `91e9e464fe5400192d13e1f9240cbf180200a103`

No Round-3-specific SQIsign specification, parameter set, source tag, or tweak package was found by the freeze date. The project therefore targets the **v2.0.1 protocol and Level-I parameters**, with the v2 source lineage as the experimental baseline. This is a provisional research target, not a claim of future Round-3 compatibility.

The v2.0.1 document is not merely a cosmetic reissue. Its algorithm text corrects the normalized discrete-log update in the relevant verification derivation (division by the appropriate power replaces the v2.0 expression), clarifies a parameter inequality, and includes errata. The public implementation has no `nist-v2.0.1` tag. The current `main` commit below is used as the conformance oracle because it contains only post-tag maintenance changes found in our diff (stack-smash/test-parallelism/RNG-state maintenance), not a new protocol or parameter set.

## Frozen repositories

| Local name | Commit | Branch | Upstream |
|---|---|---|---|
| `official` | `dd133d7aca576c361a270c8e6434832535b42ecc` | `main` | [SQISign/the-sqisign](https://github.com/SQISign/the-sqisign) |
| `fixed-precision` | `d0cb037ee6a9f68f55cb6f55b4e3746c79550330` | `new` | [munsanwon2/SQIsign-Fixed-Precision](https://github.com/munsanwon2/SQIsign-Fixed-Precision) |
| `compact-sqisign` | `5b94b09a1dbbdcc8b91749fec83a9f111ef9cce3` | `worst-case` | [pqc-lab-ku/compact-SQIsign](https://github.com/pqc-lab-ku/compact-SQIsign) |
| `pico-sdk` | `98a542c1a62fb549ffb5d66a3e5892b06276b670` | tag `2.3.0` | [raspberrypi/pico-sdk](https://github.com/raspberrypi/pico-sdk) |
| `compact-sqisign-experiment` | `bcbe6cbc6a895a3c1ab5791baaf246e27cd9dcb3` | `main` | [munsanwon2/compact-SQIsign-Experiment](https://github.com/munsanwon2/compact-SQIsign-Experiment) |
| `pqm4-sqisign` | `5dceca0448b3dec6cfa64929bcb89c03a2dd5293` | `SQIsign` | [SQISign/the-sqisign-pqm4](https://github.com/SQISign/the-sqisign-pqm4) |
| `m4-modarith` | `a04e297e220b798ecc913834db18c8e663005004` | `main` | [Crypto-TII/m4-modarith](https://github.com/Crypto-TII/m4-modarith) |
| `m4-modarith-pqm4` | `98ad59fca0aa3b684faef8d39e3e0b2abbcd0b91` | `m4-modarith` | [Crypto-TII/m4-modarith-pqm4](https://github.com/Crypto-TII/m4-modarith-pqm4) |
| `sqisign-1d` | `bf6a7ce6c99db732b529f816487d0e9db04e068f` | `main` | [Crypto-TII/the-sqisign-1d](https://github.com/Crypto-TII/the-sqisign-1d) |
| `sqisign-1d-pqm4` | `106dc20b891bbf2ebc7021f1a85ee671528e6af3` | `ePrint` | [Crypto-TII/the-sqisign-1d-pqm4](https://github.com/Crypto-TII/the-sqisign-1d-pqm4) |
| `qlapoti` | `bd8758d390fd8bdc0ed7a5f6ed299fc036b8f8b4` | `main` | [KULeuven-COSIC/Qlapoti](https://github.com/KULeuven-COSIC/Qlapoti) |
| `qlapoti-plus` | `b5720866033c70f13a8c77f63eca4fef18cb59e0` | `main` | [MaverickOtaku/Qlapoti_Plus](https://github.com/MaverickOtaku/Qlapoti_Plus) |
| `ctlll-sqisign` | `1fd09e1934e72737706dac21aa02a36bfdf245de` | `main` | [CTlll-SQIsign/CTlll-SQIsign](https://github.com/CTlll-SQIsign/CTlll-SQIsign) |
| `ct-quaternion` | `3b9281d50468819f93879679d084ef87e2f38961` | `main` | [dj33-96/Constant-time-Quaternion-SQIsign](https://github.com/dj33-96/Constant-time-Quaternion-SQIsign) |
| `spa-attack` | `3be37656c2b16ff048c8ca51512a7c2dea6f93a9` | `main` | [anishamukh/Key-Recovery-of-SQIsign](https://github.com/anishamukh/Key-Recovery-of-SQIsign) |
| `sqisign-arm` | `4585fdee33323c423b79f4d218253a74b170c0af` | `main` | [LeeJ-art/the-sqisign-vectorization](https://github.com/LeeJ-art/the-sqisign-vectorization) |
| `faster-sqisign` | `bfe7d3a687cc3d3c9c0d797d6513f9ad867ad51b` | `main` | [LinKaizhan/FasterSQIsign](https://github.com/LinKaizhan/FasterSQIsign) |
| `deuring-quaternion-algorithms` | `096638c4c83f55caf4f781641730c1c1d568040b` | `main` | [tonioecto/modular-polynomial-computation-and-evaluation-from-supersingular-curves](https://github.com/tonioecto/modular-polynomial-computation-and-evaluation-from-supersingular-curves) |

Commit dates are retained in Git metadata. These working trees were clean when inspected. Build directories are deliberately ignored by the project repository.

## Experimental baseline selection

Three baselines are kept distinct:

1. **Protocol/conformance oracle:** official `main` at `dd133d7…`, system GMP, v2/v2.0.1-compatible Level I.
2. **Direct fixed-precision baseline (variant A):** `fixed-precision` at `d0cb037…`, 110 64-bit limbs (7040 storage bits) for every Level-I quaternion integer.
3. **Compact precision baseline (variant B and the low-memory starting point):** `compact-sqisign` at `5b94b09…`, 27 64-bit limbs (1728 signed storage bits) for Level I.

The compact repository’s current worst-case bounds and code post-date the paper’s original artifact. Measurements in this project always identify whether they refer to the paper or this frozen commit; the two must not be conflated.

## Low-memory experiment freeze

The first transformation is kept in the independent checkout `work/compact-lowmem` so the frozen baseline remains byte-for-byte untouched.

| Item | Value |
|---|---|
| Base | compact `5b94b09a1dbbdcc8b91749fec83a9f111ef9cce3` |
| Initial representation branch | `lowmem/packed-finduv-v1` |
| Current experiment branch | `fix/dpe-zero-sentinel` |
| Representation commit | `254eda3d54937ec080cf2ba42d2cae8c981e0f5a` |
| Audited experiment tip | `e0a862084e95faf4c6d42557722ec0bc6c5e07f5` |
| Representation patch | `patches/0001-lowmem-pack-bounded-find_uv-vectors.patch` |
| Representation patch SHA-256 | `e3c8c2960418e108b2463a0ca86409c22b74cf3ad6fe7ae2989325d5babc1f35` |
| Capacity-assertion patch | `patches/0002-lowmem-assert-packed-find_uv-enumeration-capacity.patch` |
| Capacity-assertion patch SHA-256 | `f7c1f89b81f196697a8e4d13cbe642d74e7830d10f24f4259acc5eadf804b938` |
| RADIX32 portability branch | `fix/radix32-digit-types` |
| RADIX32 portability commit | `9a92e70341a4f52a81e42bcc77d25bf757cfe546` |
| RADIX32 portability patch | `patches/0003-portability-separate-intbig-and-field-digit-types.patch` |
| RADIX32 portability patch SHA-256 | `0e67f87282cf21c85000fdaac72a8f73d1fc4619f8a04520ad46453f5b5812e5` |
| Clapotis test-contract commit | `a7e1e7f689a68adeeaa1a3659b14ba69f789ffcf` |
| Clapotis test-contract patch | `patches/0004-test-respect-ordinary-Clapotis-retry-contract.patch` |
| Clapotis test-contract patch SHA-256 | `1739ce38220cbff53103d27f07f07287743c6a1b09e131ae9266c67b3d6dc01d` |
| DPE/ML2 correctness commit | `5fdd698e52ba082ae0076b1a5356a5b9f5645a23` |
| DPE/ML2 correctness patch | `patches/0005-fix-harden-DPE-zero-and-ML2-pivot-scheduling.patch` |
| DPE/ML2 correctness patch SHA-256 | `6b6b586f256807b0065922ce2380c31615da26e0d00308785486288d97413ed9` |
| Norm-equation UB commit | `c56c441b3540d9d34c2574d83c59e9fc7a14fa7a` |
| Norm-equation UB patch | `patches/0006-fix-define-normeq-low-word-predicate-without-UB.patch` |
| Norm-equation UB patch SHA-256 | `bce7878cb181fda3d96686d3b33314d44e4c6b1c6acffb8774b5dcbea16ae96a` |
| Theta byte-assembly UB commit | `ee982a2c00cc6b867c038d62de0c51db3e0ec03d` |
| Theta byte-assembly UB patch | `patches/0007-fix-assemble-theta-random-seed-without-signed-shift.patch` |
| Theta byte-assembly UB patch SHA-256 | `ed4ebf6b0a1da18ad8880979a1085f2b66dd7a25b1f594244811d70017e5d1d9` |
| C1 typed-workspace base | `ee982a2c00cc6b867c038d62de0c51db3e0ec03d` |
| C1 typed-workspace commit | `e61c1fa8fb4898fb606dac807727f97655254739` |
| C1 reviewed pre-commit diff SHA-256 | `61f19f313b3f07bccb9d9fa181fc06c7c2e66bf464be729e188b06cda47efceb` |
| C1 typed-workspace patch | `patches/0008-lowmem-add-typed-full-row-find_uv-workspace.patch` |
| C1 typed-workspace patch SHA-256 | `dbd0017eaf3ab3236fe79b41b47b81b99e22eb266a49e63e9271e0da7c64584c` |
| C1 test-hardening commit | `0f438c26480946eb356e3d89aa18b57e20f9e4b4` |
| C1 test-hardening reviewed diff SHA-256 | `d9aaa68b00338cb36992bf795a67bd7818364404816124d3eb6026d5ab634d8d7` |
| C1 test-hardening patch | `patches/0009-test-harden-find_uv-workspace-equivalence.patch` |
| C1 test-hardening patch SHA-256 | `fc98b9fb5b352397c056e679157d03f4a412a5d61f911ca7c9f89fda1d55cc9e` |
| D1 independent worktree | `work/compact-d1`, branch `lowmem/d1-index-sort` |
| D1 base | `0f438c26480946eb356e3d89aa18b57e20f9e4b4` |
| D1 compact-index workspace commit | `cf9f6b6857996dc98f75117fec94ab8b9f0654f4` |
| D1 reviewed pre-commit diff SHA-256 | `45119e49a33e104350ff5b6d360e319d45a258b0874f706fa7e906bad608a9da` |
| D1 compact-index patch | `patches/0010-lowmem-sort-find_uv-rows-through-compact-indices.patch` |
| D1 compact-index patch SHA-256 | `1a9e203f6c990b0f32b1b44c2fb1fc370ecb3fc4bbcc3c2356d5b7983f9233e` |
| D2 independent worktree | `work/compact-d2`, branch `lowmem/d2-quotient-recompute` |
| D2 base | `cf9f6b6857996dc98f75117fec94ab8b9f0654f4` |
| D2 all-row validation commit | `00f42908ce0147019cd2a1bce6444a2241f45506` |
| D2 validation reviewed diff SHA-256 | `10607e5b0eb4a551347f56e41421ad734d3bafc731ce29d069b101f8474c047e` |
| D2 validation patch | `patches/0011-id2iso-validate-all-find_uv-norm-rows.patch` |
| D2 validation patch SHA-256 | `13b14b18f4d29834596738e07a41795b56aa3cb1a4935ec82517b73aa8cc2f71` |
| D2 quotient-recomputation commit | `d6801884d9c052450a7982e3ac69b29dab0f8893` |
| D2 recomputation reviewed diff SHA-256 | `5dd307947bb7e1730f9e00ec281131ff7e44afc36e6a2931f804e9c3a2f07913` |
| D2 recomputation patch | `patches/0012-lowmem-recompute-find_uv-quotients-on-demand.patch` |
| D2 recomputation patch SHA-256 | `94b410df3e654dc69093a2550ce515f20a9379b0cb4bc7e21fdf2e6527c05f16` |
| D2 test-only division profiler patch SHA-256 | `c20c0186bb32379d1ca84fcf17dfa17acc5c6403420a6fe6ffe0fa3ae560311d` |
| D2 quotient-profile JSON SHA-256 | `12d2cc6b1d354ba4dff412f164fcbaf69115fcb6f43b435d9666db8f6cfd39ef` |
| D2 KeyGen runtime JSON SHA-256 | `85fba1fde6dd0230be95beff05e8a2648fe99c0dbd9e01cb39dff9e1f360a180` |
| D2 Sign runtime JSON SHA-256 | `6a3db00133ec6041dfbe312b654a314260b73cad4360e4df78aca5db5b962712` |
| D2 Verify runtime JSON SHA-256 | `75e6cec6222224153e4b6fc54944e528d2ebc2043d2bd44c2a530f86c989f0bf` |
| D2 host code-size JSON SHA-256 | `7e1875acc042902a5ebf882415b00319279de903d28f6b33bbb1d3ac0d00c8e6` |
| D2 host timing binaries SHA-256 | D1 `899cb643408467d19d5af20de99c669e285e7902549c17b1307a1a4f2922c8b2`; D2 `a6a22f66d1ad6215013c92318430086457bb4d2b4bfb7df27bcc602226fe0307` |
| D3 independent worktree | `work/compact-d3`, branch `lowmem/d3-two-row-streaming` |
| D3 base | `d6801884d9c052450a7982e3ac69b29dab0f8893` |
| D3 prepare-row refactor commit | `f70042d1fc97e370cf6d41e6e436677c5290ed0e` |
| D3 prepare-row reviewed diff SHA-256 | `ece10644d9c3495f06f9bdfc512d380491d2866fc07254ffea3ec6d5230e1e49` |
| D3 prepare-row patch | `patches/0013-refactor-isolate-canonical-find_uv-row-preparation.patch` |
| D3 prepare-row patch SHA-256 | `25b0aff8743af880cfeabbead4d702e7c38036a7e74886817b67a7657472647b` |
| D3 two-row streaming commit | `60ce94495ea32943647a7c1b946c6750c2557d49` |
| D3 streaming reviewed diff SHA-256 | `3468663cc8aeb8327e9acbaf37ff8ca08f04e82dcdec5153d2ebb24fc79124ba` |
| D3 streaming patch | `patches/0014-lowmem-stream-find_uv-through-two-resident-rows.patch` |
| D3 streaming patch SHA-256 | `27aa24a61036626b51049af08a4e85d77e1ced9f0830c9c0446b9154edd2e42d` |
| D3 array-lifetime UB fix commit | `b54922bd2de94b871bf4bd477a11de6e32bd17bf` |
| D3 UB-fix reviewed diff SHA-256 | `edae86b8360825a1894db95ac5b8f44aa0a34c3405485641d2045ff90c7c652f` |
| D3 UB-fix patch | `patches/0015-fix-keep-D3-row-lifetime-loops-within-array-bounds.patch` |
| D3 UB-fix patch SHA-256 | `c593e08d1475f7e98a85046ac3d27e8288591e995140b6d630a4cb66ba83867d` |
| D3 candidate-trace patches SHA-256 | D2 `b0fc0c4936c4f544b3afb4d8f49ba19400987835031d1d16100eb2adce673dc8`; D3 `0f51611e4c633465e26dfcdee8a1f1bfbeb06d0056429d4586f2e7669bc31513` |
| D3 KeyGen runtime JSON SHA-256 | `179a3f09c6a888090426c70862038db9a49c679987f5cf9f3608b8d4d71cd2ab` |
| D3 Sign runtime JSON SHA-256 | `43861e58d55b2eff1e82b3ad5434ddf17201115464df5c94b5f0b09464d91099` |
| D3 Verify runtime JSON SHA-256 | `e9fbb34cbcf21fb3fc86eb0ac732bb993896bf3d10214286ed0e706e8ae2ca77` |
| D3 host code-size JSON SHA-256 | `40e69334ca9c1d502e393fc307d213e44e332c17b994b17f559baadbec206f01` |
| D3 host timing binaries SHA-256 | D2 `899fd2ec604bcb8cb5ffd0c21259c0beac3c36cc273e8acf9656ee91da3af248`; D3 `c697a8e79947177571e95e523e860a74120252921c48b79da34f81859b755a47` |
| D3 Arm ancestor-path profiles | `-Os`/soft `273,432 B`; Pico Release-like `-O3`/softfp `273,496 B`; both Arm GCC 15.2.1 individual-TU diagnostics |
| D4 independent worktree | `work/compact-d4`, branch `lowmem/d4-stack-flatten` |
| D4 base | `b54922bd2de94b871bf4bd477a11de6e32bd17bf` |
| D4 stack-flattening commit | `a6b06287706a97999d077d055818c2b5612a8704` |
| D4 reviewed parent diff SHA-256 | `6ce21f3052b0e274de3421d0e6a7cdf1d4d0114f64aba6aa9a697cb590e2994e` |
| D4 patch | `patches/0016-lowmem-flatten-find_uv-lattice-state-into-workspace.patch` |
| D4 patch SHA-256 | `018e4c243e672b49ec32e85b59041bb0918526eaa15d5a22a4aac21d70731c6a` |
| D4 KeyGen runtime JSON SHA-256 | `2f3808e5f2bd86ee9d1aaf557c712e8944619bba6d449f36190b0bcc313b6d55` |
| D4 Sign runtime JSON SHA-256 | `93324341c1e6cafd50a8a38530615e03c8d432e7c2523f250e1fd68aa8e6b0c8` |
| D4 Verify runtime JSON SHA-256 | `9e9e81e3b177d908dec6165deea6e42a2a307da821a10730ebe0ce052b6ffc5d` |
| D4 host code-size JSON SHA-256 | `2546d855877fceec054c2281f51711db4a215d5a95c2f83df127f1017f7efda4` |
| D4 host timing binaries SHA-256 | D3 `cf644fa77ddd6d9472d77d385744157d2e325e4d577d9ca3e7b58ac52f303522`; D4 `372dfd4d859a4bacfb5c1ffc34da35e1090cfec25c0f224379778687173b8929` |
| D4 Arm ancestor-path profiles | `-Os`/soft `196,264 B`; Pico Release-like `-O3`/softfp `196,328 B`; workspace sums remain `549,272 B` / `549,336 B` |
| D5 independent worktree | `work/compact-d5`, branch `lowmem/d5-ml2-workspace` |
| D5 base | `a6b06287706a97999d077d055818c2b5612a8704` |
| D5 phase-overlay commit | `2771afabf54b579b6f05d7440aa6de0a48544779` |
| D5 reviewed commit diff SHA-256 | `4db3ea31a31f1e0f019fc6e1be198cc53ca2b8ea773e493038ec1015fe52e830` |
| D5 ML2 workspace header SHA-256 | `12620e8e5646340648a22bd1f14e1d36a33e5a9bc3bbcb0251fcd3c4c762d1ca` |
| D5 patch | `patches/0017-lowmem-overlay-find_uv-ML2-with-candidate-phase.patch` |
| D5 patch SHA-256 | `07b5380c158806d9e7cb2c2959535033df66ff590a5df811610e34c1523c5b15` |
| D5 Level-I Cortex-M33 ABI | outer `353,008 B`; ML2 core `77,632 B`; complete retry `94,912 B`; candidate union member `275,840 B`; alignment 8 |
| D5 `find_uv` maximum early-ML2 path (`-Os`/soft) | own-frame endpoint `203,384→107,264 B`; known-descendant-adjusted `205,032→109,816 B`; exact workspace unchanged at `353,008 B` |
| D6 independent worktree | `work/compact-d6`, branch `lowmem/d6-clapotis-workspace` |
| D6 base | `2771afabf54b579b6f05d7440aa6de0a48544779` |
| D6 Clapotis/fixed-degree workspace commit | `15a69ee3a3eecc70f6f04e6e8bff134635a27696` |
| D6 reviewed commit diff SHA-256 | `62723cec277485a707824290425a98dc4db745a1190d7b2885860f9c1979e1f2` |
| D6 patch | `patches/0018-lowmem-reuse-ML2-arena-through-Clapotis-fixed-degree.patch` |
| D6 patch SHA-256 | `bb197da1c945140c7333aca079e318c2b8bde308d3706938861fb0dc8fa620ce` |
| D6 fixed-degree Arm `-Os` path | current legacy `180,992 B`; explicit workspace `85,936 B`; reduction `95,056 B`; known-descendant-adjusted explicit `88,488 B` |
| D6 explicit Clapotis diagnostic (`-Os`/soft) | path `88,488 B`; with unchanged `353,008 B` arena `441,496 B`; not a linked peak or production Sign path |
| D7 independent worktree | `work/compact-d7`, branch `lowmem/d7-mlll-workspace` |
| D7 base | `15a69ee3a3eecc70f6f04e6e8bff134635a27696` |
| D7 MLLL workspace commit | `f6f7bf559cfab95fa1223fa4f928792ff79a7b76` |
| D7 reviewed commit diff SHA-256 | `81956a08924d89b6cd30f907600f8eb7119b43ce818985d2466645c3c5f58469` |
| D7 patch | `patches/0019-lowmem-route-MLLL-operations-through-workspace.patch` |
| D7 patch SHA-256 | `457df621d388f47611377a40b20dec189f7279e9f7f5ab4bfbebfa8a97718a05` |
| D7 maximum known MLLL paths (`-Os`/soft) | product `165,152→74,584 B`; ideal intersection `162,784→72,224 B`; arena sums `427,592 B` / `425,232 B`; API-level projections, not production Sign |
| D8 independent worktree | `work/compact-d8`, branch `lowmem/d8-equivalent-ideal-workspace` |
| D8 base | `f6f7bf559cfab95fa1223fa4f928792ff79a7b76` |
| D8 equivalent-ideal workspace commit | `3ea2b47417a5a6dc0b680bc60625c2761123314b` |
| D8 reviewed commit diff SHA-256 | `6649faa1340ae425f543eabf64ce1dfd52cd1fcd4de1b6c62c83c207d8d2af00` |
| D8 patch | `patches/0020-lowmem-route-equivalent-ideal-search-through-workspa.patch` |
| D8 patch SHA-256 | `d0c22ff5c3ffc717a0d97e9f0777e9150d5963925e215df911e60d578e03b860` |
| D8 maximum known equivalent-ideal path | Arm `-Os`/soft `125,560→34,624 B` (−90,936 B); Pico-like `125,608→34,904 B` (−90,704 B); explicit API only, not production KeyGen/Sign |
| D9 independent worktree | `work/compact-d9`, branch `lowmem/d9-random-ideal-workspace` |
| D9 base | `3ea2b47417a5a6dc0b680bc60625c2761123314b` |
| D9 random-ideal workspace commit | `cb040911ef14dd56d2c647834d884919de029ace` |
| D9 reviewed full commit diff SHA-256 | `d35d0c8176dacfd4e5252f2c1191ff4e8ae89fe60369263371593c401ede826f` |
| D9 production-only diff SHA-256 | `4115185406595d141241ae4419e724ab1e7b73a4f9320582f6053bad24004015` |
| D9 patch | `patches/0021-lowmem-route-random-ideal-construction-through-workspace.patch` |
| D9 patch SHA-256 | `47109e4a65e1359cac7e7e8e3ab31b97dbbc7c6f4eceb36e79489e605b05cb51` |
| D9 maximum known random-ideal path | Arm `-Os`/soft `122,312→37,744 B` (−84,568 B); Pico-like `122,568→38,008 B` (−84,560 B); explicit API only, not production KeyGen/Sign |
| D9 Arm diagnostic compiler/profiles | Arm GNU 15.2.1; Cortex-M33/RADIX32 `-Os`/soft and `-O3`/softfp/CMSE individual-TU probes |
| D10 independent worktree | `work/compact-d10`, branch `lowmem/d10-keygen-workspace` |
| D10 base | `cb040911ef14dd56d2c647834d884919de029ace` |
| D10 full encoded KeyGen workspace commit | `7b549db43145e112366fba4509a1085b3400f52a` |
| D10 reviewed full commit diff SHA-256 | `434fc6cf09ffa1ec6edea4c17660d1b571b5bd41c53f895a8d49336c13537d8a` |
| D10 production-only diff SHA-256 | `3f345d084e785c571ec426741de390a335152c1bd95650ec47a07a2ce2b62180` |
| D10 patch | `patches/0022-lowmem-add-caller-owned-full-keygen-workspace-API.patch` |
| D10 patch SHA-256 | `96cedef1ec6d0d29034b113ce3e2785da688762ebc40887eb5ecc1040ef3e255` |
| D10 Level-I KeyGen workspace ABI | `353,008 B`, alignment 8; one nested `find_uv_workspace_t` at offset zero |
| D10 maximum known KeyGen path including arena | Arm `-Os`/soft `434,392 B` (nominal raw margin `98,088 B`); Pico-like `434,464 B` (margin `98,016 B`); individual-TU diagnostics, not linked fit bounds |
| D10 fixed-degree / secret-encoding arena sums | `-Os`: `413,120 B` / `366,624 B`; Pico-like: `413,192 B` / `366,640 B` |
| D10 host benchmark compiler / binary SHA-256 | Apple clang 17.0.0 (`clang-1700.6.3.2`); `51ffd50feb56208c0df2c8f978c59f32c51a944219215f5039512bef7c1aee0e` |
| D10 benchmark source SHA-256 | `25cd7a2f37803336f025ff4496c355a00fcb59acf205dbfc910946d785f0fb35` |
| D10 host timing CSV SHA-256 | first `f6ea7bb692f43e8563dbcb11d0bf7b2310423f557f72a18f22ae8bc12f6a1254`; rerun `99c9581d771c47f10ab73eec9f0ec307a59e3d7cf2427b14366e92e69de82e19` |
| D10 timing design/result scope | 30 rows = 15 deterministic seeds × AB/BA order; median paired explicit/legacy ratios `0.998012779` and `0.998830461`; descriptive host smoke only |
| D10 closure boundary | non-NULL runtime route does not request allocation, but the linked compatibility closure intentionally retains `malloc/free`; whole closure is not yet VLA-free |
| D10 Arm stack / aggregate reproduce / runtime-summary script SHA-256 | `c1b22406722eee7159404e726203df971cc1c2a9b31da1682d448c70116ab171` / `a776db55ad2d76e44fdda6f6b42ff4bcb6f5d7bf4a6b9b2512256891d9e38833` / `280a89ac824a2ebf92cf30ed7ea30396703211944bf968959c91c1fc84980198` |
| D10 production layout probe SHA-256 | `99eb3635fe1c864b0a2a073ab62bb9e04df3e27e14ae45eab7533d1a9bb79be6` |
| D10 root integration / clean aggregate gate | project `47f535385baaaff6d7fefdf4494034e64708b908`; `./scripts/reproduce_d10.sh` PASS |
| D11a independent worktree | `work/compact-d11`, branch `lowmem/d11-lowmem-closure` |
| D11a base / low-memory closure commit | `7b549db43145e112366fba4509a1085b3400f52a` / `78db2858780850f06b965ff87653795b529d3299` |
| D11a reviewed commit diff / tree SHA | `fa9b1913e38381024c538c7984ed1cde2eaf2e9e620992c95e1cc0f754bad608` / `a893dd7557b20cda9f159f426bdd6b14c57d8aae` |
| D11a patch / SHA-256 | `patches/0023-lowmem-specialize-allocator-free-KeyGen-closure.patch` / `594b8285359e80d92b8b0ecd83b1c4ce4f3a04386304907263a4d7f6274047eb` |
| D11a selected closure | host production `44 objects / 10 archives / 44 members`; deterministic host `46 / 10 / 45`; Arm `45 / 10 / 45` |
| D11a Level-I Cortex-M33 ABI | caller arena `353,008 B`, alignment 8; lattice state `77,168 B`; candidate `275,840 B`; ML2 retry `94,912 B` |
| D11a Arm dynamic-frame inventory | eight fixed components: `14,136`, `6,792`, `4,968`, `4,264`, `216`, `96`, `88`, `88 B`; VLA payloads excluded |
| D11a maximum known HNF projection | project-known PSP path `105,808 B`; arena plus path `458,816 B`; nominal raw margin `73,664 B`; not a linked peak or fit bound |
| D11a object-level mutable/XIP evidence | deterministic Arm writable payload `56 B` (`52 B` test DRBG + `4 B` secure-clear pointer); `CONNECTING_IDEALS` is `R/.rodata`, `27,272 B`; final linker placement unproven |
| D11a source reproducer / artifact-auditor SHA-256 | `293fcaa237b1bca886955f4683327346d60b3a9c2e108a725e410717cfa46c6c` / `9e5856ad578ab32dd45ddf283f3d23cdd01bace56e51a84ea1e82a6dc05f5120` |
| D11a root wrapper SHA-256 | `5f4dfa67104459b9bbd88c6c8e03315454771816ad06373511e8bcf38eb4ecdb` |
| D11a root integration / clean aggregate gate | project `523ec83baa7f8536f726fa18f4bd4e32f3b86174`; `./scripts/reproduce_d11a.sh` PASS |
| D11b independent worktree | `work/compact-d11`, branch `lowmem/d11b-hnf-workspace` |
| D11b base / HNF-workspace commit | `78db2858780850f06b965ff87653795b529d3299` / `99344812b28e4a57ba0c876a27ecfa7372363f9a` |
| D11b reviewed commit diff / tree SHA | `df6591f48b7cd0427608aa3135ad0b001e0147b868371ad5ba2f79b386f256bc` / `7bd7a7ac4b876552f7d559ee0ad937ff21c94efd` |
| D11b patch / SHA-256 | `patches/0024-Flatten-low-memory-HNF-scratch-into-workspace.patch` / `a24367c9e93dc14e7cd32a43f50294f28b275d4e8a816a605911b53274fe9af6` |
| D11b selected closure | host production `44 objects / 10 archives / 44 members`; deterministic host `46 / 10 / 45`; Arm `45 / 10 / 45` |
| D11b Level-I Cortex-M33 ABI | caller arena `353,008 B`, alignment 8; HNF scratch `13,824 B`; HNF/ML2 lattice union `94,912 B`; candidate phase `275,840 B` |
| D11b Arm dynamic-frame inventory | seven fixed components: `14,136`, `4,968`, `4,264`, `216`, `96`, `88`, `88 B`; VLA payloads excluded |
| D11b maximum known HNF projection | project-known PSP path `92,024 B`; arena plus path `445,032 B`; D11a→D11b `−13,784 B`; nominal raw difference `87,448 B`; not a linked peak or fit bound |
| D11b source reproducer / artifact-auditor SHA-256 | `a9629ccbb0ec182a0b8eac50d2a800a0c86c64cd1a6b7227fb76b8358fc49746` / `ad406d6d3fbe56b545b59e8b309492c625a17d882ba89a8706c2d0531d90d204` |
| D11b root wrapper / HNF-path checker SHA-256 | `f193352373e4a7ff732ae5fe36f9449c715316e7941c3139b6cab9d2df60bf76` / `35dcd5cc6c57fd06fa4fb8cc1285dbd9e8dfa4d3232e1509101ead6c4286338b` |
| D11b benchmark compiler / source / binary SHA-256 | Apple clang 17.0.0 (`clang-1700.6.3.2`); `25cd7a2f37803336f025ff4496c355a00fcb59acf205dbfc910946d785f0fb35`; `ebbe0d8e1a521419d219d14d3838c3d0d3327bca86e4236cbd2ff7b8fb8e3ebf` |
| D11b host timing CSV SHA-256 | first `d783384448fe37993c0078fa0c2ed8a639b032e71ccf9e9b0cfac52fce52d7d6`; rerun `9e9c4b311f3d17b4e2135bf67644bbefe0cea562f8c84863049dc767cac72fb4` |
| D11b timing result/scope | 30 rows = 15 deterministic seeds × AB/BA; explicit/legacy ratio of medians `0.998439515` / `0.998935711`; complete explicit route, descriptive host smoke only |
| D11b root integration / clean aggregate gate | project `06cb0b4a9ebb04ef680010958434557d9a37dd4a`; `./scripts/reproduce_d11b.sh` PASS |
| D11c independent worktree | `work/compact-d11`, branch `lowmem/d11c-theta-workspace` |
| D11c base / theta-workspace commit | `99344812b28e4a57ba0c876a27ecfa7372363f9a` / `1b9888a765b2674a78232595d08eadc24a5c2a94` |
| D11c reviewed commit diff / tree SHA | `b9576c12986d94552dbe5a9d7c2d8233d120c514a1ee007511612772ea4ae248` / `14bb24fa695ae0ed952a54c6758ee9fd501dfaf2` |
| D11c patch / SHA-256 | `patches/0025-lowmem-flatten-theta-chain-scratch-into-workspace.patch` / `36e8f421a5aa8b9e6a80eb4702766afb830aef468fc45c8a1d462af8d23429ac` |
| D11c selected closure | host production `44 objects / 10 archives / 44 members`; deterministic host `46 / 10 / 45`; Arm `45 / 10 / 45` |
| D11c Level-I Cortex-M33 ABI | theta workspace `13,844 B`, alignment 4, offsets `0/864/884/4772/8660/11252`; fixed-degree union `94,912 B`; caller arena unchanged at `353,008 B`, alignment 8 |
| D11c Arm dynamic-frame inventory | six fixed components: `4,968`, `4,264`, `216`, `96`, `88`, `88 B`; VLA payloads excluded |
| D11c theta/global path accounting | Pico-like fixed-u/v theta path `68,584→54,792 B` (`−13,792 B`); HNF global path remains `92,024 B`; arena plus global path remains `445,032 B` |
| D11c source reproducer / artifact-auditor SHA-256 | `057e74bcb686821b1f4e308c5d5d0311da2f7fde2dd850dbf00702870614a653` / `87ecea0a0655060ced2c74038724cbea1f053f782d149e43e9d1c46be5cda184` |
| D11c root wrapper / theta-path checker SHA-256 | `5af4c04ebfd410f7e8ac40fe2d4f6fd897030e4f2401ca9b8643ff2609442911` / `f0387f9ac6d2d8ad37abe783d4012950e61d526ac9a7e7a4b399fdf31af7cce0` |
| D11c benchmark compiler / source / binary SHA-256 | Apple clang 17.0.0 (`clang-1700.6.3.2`); `25cd7a2f37803336f025ff4496c355a00fcb59acf205dbfc910946d785f0fb35`; `36d361b212b2c18db0c9919d79e5bbe38f86d4059e07cc90b008043401947cfc` |
| D11c host timing CSV SHA-256 | first `b9cfcc916ac38e73a0b40edada14a9e7ad41bdf6db27be0e19acbd3b6e6ece81`; rerun `06131d48057f6833fd82af7727a5e421f04278e027f3e019fffb0bdae7937a4a` |
| D11c timing result/scope | 30 rows = 15 deterministic seeds × AB/BA; explicit/legacy ratio of medians `0.998642360` / `0.999015373`; cumulative explicit route, descriptive host smoke only |
| D11c root integration / clean aggregate gate | project `7571c1ae0e6c31b3d96537f48b5d8f0ed0e3926b`; `./scripts/reproduce_d11c.sh` PASS |
| D11d-1 independent worktree | `work/compact-d11`, branch `lowmem/d11d1-batched-inversion-workspace` |
| D11d-1 base / batched-workspace commit | `1b9888a765b2674a78232595d08eadc24a5c2a94` / `a8d30fd64985935ed7d9b1b92fe1ae90ba4a39e3` |
| D11d-1 reviewed commit diff / tree SHA | `3536ffc3789fc8fad0925edd2673e20a58301ecd462265a13558d498ea2878a5` / `771488135438e3438fb0e1299e2c761e67c6afa4` |
| D11d-1 patch / SHA-256 | `patches/0026-lowmem-flatten-batched-inversion-scratch-into-workspace.patch` / `3457191f87c5a098c9a15b9bfe4b01b2f47ebbe513ea55a2ed2269880ac90c3a` |
| D11d-1 selected closure | host production `44 objects / 10 archives / 44 members`; deterministic host `46 / 10 / 45`; Arm `45 / 10 / 45` |
| D11d-1 Level-I Cortex-M33 ABI | batch workspace `1,584 B`, alignment 4, offsets `0/792`; theta workspace `15,428 B`, batch offset `13,844`; fixed union `94,912 B`; arena unchanged at `353,008 B`, alignment 8 |
| D11d-1 Arm dynamic-frame inventory | five fixed components: `4,968`, `4,264`, `96`, `88`, `88 B`; VLA payloads excluded |
| D11d-1 path accounting | `-Os` theta/HNF `54,464/91,448 B`, arena+HNF `444,456 B`; Pico-like theta/HNF `54,816/92,032 B`, arena+HNF `445,040 B`; no global reduction |
| D11d-1 source reproducer / artifact-auditor SHA-256 | `e2981987aa29e195ddd037995ca7b36272a6fa2985e0360a3f5dedfa257618f6` / `a9912b36b2d60381c582ab1c5aefe732c03e91c9405665f8c5d5846afb74f8e3` |
| D11d-1 root wrapper SHA-256 | `67babda7cd4f4feca2577b36d9c9ef2e39c8256c3f82e99f14209d3c7894c624` |
| D11d-1 Pico HNF/theta / `-Os` checker SHA-256 | `29f4708d24f655defa87896f2e022475c7ebe4b8cd9ff777b89cb4b95a8ebdac`; `a180f147fc08503cfd00e9496ff21e70f31a17f1262046073873104fdc7e4049`; `62744dd17e025e083f7b84c4d8632cf6d0c17b1308f098940a0a400276ddc107` |
| D11d-1 benchmark compiler / source / binary SHA-256 | Apple clang 17.0.0 (`clang-1700.6.3.2`); `25cd7a2f37803336f025ff4496c355a00fcb59acf205dbfc910946d785f0fb35`; `fcf2931f4c936bf66a6bb09017c0bc72b01d5c3220441b703e7bc542fecd9ea4` |
| D11d-1 host timing CSV SHA-256 | first `4cf8f97a3778c8a0f6ed06015a59263bfcf99adcdaf3a725a3b965ee9f9167d1`; rerun `5b71b5b5dbdf15b52a749f8c92ac3541e471e5bb5bb678b067bd3ed9e3bfbe12` |
| D11d-1 timing result/scope | 30 rows = 15 deterministic seeds × AB/BA; explicit/legacy ratio of medians `1.001836082` / `0.998576818`; cumulative explicit route, descriptive host smoke only |
| D11d-1 root integration / clean aggregate gate | project `4e23c3306816d0c60bc727aa1a49bff6179a23d4`; `./scripts/reproduce_d11d1.sh` PASS |
| D11d-2 independent worktree | `work/compact-d11`, branch `lowmem/d11d2-dlog-workspace` |
| D11d-2 base / dlog-workspace commit | `a8d30fd64985935ed7d9b1b92fe1ae90ba4a39e3` / `434e093bc5e7e4157b77176a7d762853f50f39b0` |
| D11d-2 commit diff / tree SHA | `cf7d0e1ddefb462a352550cdfd9ec1fe5dd7e7b678b19b6dc08a69f9cdf8e5b3` / `dc0e9a2c0ab4a2a3dcb16ea7883fdfdbe2f12019` |
| D11d-2 patch / SHA-256 | `patches/0027-lowmem-flatten-2power-dlog-scratch-into-workspace.patch` / `cf7d0e1ddefb462a352550cdfd9ec1fe5dd7e7b678b19b6dc08a69f9cdf8e5b3` |
| D11d-2 selected closure | host production `44 objects / 10 archives / 44 members`; deterministic host `46 / 10 / 45`; Arm `45 / 10 / 45` |
| D11d-2 Level-I Cortex-M33 ABI | dlog workspace `1,152 B`, alignment 4, offsets `0/576`; batch/dlog union `1,584 B`, alignment 4; arena unchanged at `353,008 B`, alignment 8 |
| D11d-2 Arm dynamic-frame inventory | three fixed components: `96`, `88`, `88 B`; VLA payloads excluded |
| D11d-2 path accounting | Pico-like recursive Tate/Weil branches each `−1,136 B`; HNF remains `92,032 B`; arena+HNF remains `445,040 B`; no global reduction |
| D11d-2 source reproducer / artifact-auditor SHA-256 | `51ce7ec69c8b6250c5818bfdb0a614fd54658c1edc04cb540430be9b2f359ad6` / `ded61a179b12d6cb23cba31c3de6d67431f0f73dd97876f055fe9322a54f17ca` |
| D11d-2 root wrapper / dlog-path checker SHA-256 | `f4904e367501d6c098354ba0c7b1589b7a30de474bcbd89309356c6c8696b825` / `0368290bdd888e8f8161f16d43e08a1657c99b3f0763bacc5534348b0d6ef995` |
| D11d-2 benchmark compiler / source / binary SHA-256 | Apple clang 17.0.0 (`clang-1700.6.3.2`); `25cd7a2f37803336f025ff4496c355a00fcb59acf205dbfc910946d785f0fb35`; `9a29eeb18751bca3c3d114026093f997af92c7d17cd93acb600f391a435c39b5` |
| D11d-2 host timing CSV SHA-256 | first `2ccc737acada7579d7f9a9b5240d6233a46169b20ec9662ca761d1643487252c`; rerun `6022c836962187717899feb0968d51f42c3dd0a048e5206be196f1bd4624dd69` |
| D11d-2 timing result/scope | 30 rows = 15 deterministic seeds × AB/BA; explicit/legacy ratio of medians `0.999255688` / `0.998576164`; cumulative explicit route, descriptive host smoke only |
| D11d-2 root integration / clean aggregate gate | project `8545383a95c0f0d49de6f4c1287df67891696300`; `./scripts/reproduce_d11d2.sh` PASS |
| D11d-3 independent worktree | `work/compact-d11`, branch `lowmem/d11d3-mp-workspace` |
| D11d-3 base / MP-workspace commit | `434e093bc5e7e4157b77176a7d762853f50f39b0` / `f63efb4154ffacbd1e5a6cc6ab0229512bf8d2ce` |
| D11d-3 commit diff / tree SHA | `76ddc8df206dcecd2d7c170cf03e19d63fec8f5d535e856ad97156014fff2695` / `d25b2a13fe81da61059c19e67e6707c5e4ad8e33` |
| D11d-3 patch / SHA-256 | `patches/0028-lowmem-flatten-fixed-precision-mp-scratch.patch` / `76ddc8df206dcecd2d7c170cf03e19d63fec8f5d535e856ad97156014fff2695` |
| D11d-3 selected closure | host production `44 objects / 10 archives / 44 members`; deterministic host `46 / 10 / 45`; Arm `45 / 10 / 45` |
| D11d-3 Level-I Cortex-M33 ABI | multiply `144 B` offsets `0/72`; inverse `504 B` offsets `0/72/144/216/288/360`; matrix `936 B` offsets `0/72/144/216/288/360/432`; pairing union `1,584 B`; arena unchanged at `353,008 B`, alignment 8 |
| D11d-3 Arm dynamic-frame inventory | zero selected dynamic `.su` records; exact closure compiles with `-Wvla -Walloca -Werror` |
| D11d-3 path accounting | `-Os` MP/change/HNF `192/416/91,448 B`, arena+HNF `444,456 B`; Pico-like `224/448/92,032 B`, arena+HNF `445,040 B`; no global reduction |
| D11d-3 source reproducer / artifact-auditor SHA-256 | `79c9013a78d24bf4fe900249ef9e680d823a1aa757655354098bf6169b69bcec` / `ad19c3bd51b04e808adb129adb0a71480095d00b99603d5939a65c6b4a1e748f` |
| D11d-3 root wrapper / MP-path checker SHA-256 | `766360c1c7f58c3dd4c7955ab3b91426852f361b55a023b692dd9c3c1d1f8995` / `21935e4b65f0d19bd0935401ef00508b155311b514d8c6bc29f505e906453995` |
| D11d-3 benchmark compiler / source / binary SHA-256 | Apple clang 17.0.0 (`clang-1700.6.3.2`); `25cd7a2f37803336f025ff4496c355a00fcb59acf205dbfc910946d785f0fb35`; `9a29eeb18751bca3c3d114026093f997af92c7d17cd93acb600f391a435c39b5` (byte-identical to D11d-2) |
| D11d-3 timing scope | no new timing attribution; the exact D11d-2 CSVs remain descriptive evidence for the byte-identical normal binary |
| D11d-3 root integration / clean aggregate gate | project `e3ea99b42b9781776307bce37c5959745665edbf`; `./scripts/reproduce_d11d3.sh` PASS |
| D12a independent worktree | `work/compact-d11`, branch `lowmem/d12a-sign-workspace-api` |
| D12a base / decoded-key Sign workspace commit | `f63efb4154ffacbd1e5a6cc6ab0229512bf8d2ce` / `8a0534d0fc4f2d8f0f355774d111e26b3ca19035` |
| D12a commit diff / tree SHA | `a631c53f062c9803e11ea2709b3924d59efff808e15ae9c8d51c80bc04251316` / `7f07ffba99381cce087c6acb241591c52ad81e00` |
| D12a patch / SHA-256 | `patches/0029-lowmem-add-caller-owned-Sign-workspace-API.patch` / `a631c53f062c9803e11ea2709b3924d59efff808e15ae9c8d51c80bc04251316` |
| D12a Level-I Sign workspace ABI | `353,008 B`, alignment 8; `find_uv`, lattice/HNF, ML2 retry, fixed degree, theta and dlog members all at offset zero |
| D12a Pico-like known path | decoded-key workspace wrapper through early `find_uv` HNF descendant: `114,840 B`; arena plus path `467,848 B`; nominal raw difference `64,632 B`; individual-object diagnostic, not linked fit |
| D12a remaining closure boundary | encoded top-level Sign and secret-key decoder are not integrated; `ec_eval_even_strategy` remains dynamic; current public Sign is not a low-memory target closure |
| D12a benchmark compiler / source / binary SHA-256 | Apple clang 17.0.0 (`clang-1700.6.3.2`); `13a2508e5f17d09d8ebb6f8449ed1cbd7fb991b3576cc0e89b7119bbaa64a17c`; `a50dd41a30412e96c33818cd903435030aced27e7f86188c2db1e14aa3102f35` |
| D12a host timing CSV SHA-256 | first `4414b9f2003d5f0de3fe8dc5bb1fa358ce5da0dd61b1c39ba3c19544e361cfaa`; rerun `de880a511c464453efdc6c68841f49bf860d9be6a504cbbff113aa96fd96dcfd` |
| D12a timing result/scope | 30 rows = 15 deterministic seeds × AB/BA; explicit/legacy ratio of medians `1.000625999` / `0.997403404`; cumulative explicit API, descriptive host smoke only |
| D12a Arm changed-object payload | `sign.c` +736 B and `id2iso.c` +80 B (`+816 B` total) in the frozen individual-TU profile; final linked code size unmeasured; mutable section delta zero in those objects |
| D12a aggregate gate | project `1a8f57ee2b49a260ac051e9dd7ddcc2fe0cb84a1`; clean `ARM_TOOLCHAIN_BIN=... ./scripts/reproduce_d12a.sh` PASS |
| D12b independent worktree | `work/compact-d12`, branch `lowmem/d12b-encoded-sign-closure` |
| D12b base / encoded Sign closure commit | `8a0534d0fc4f2d8f0f355774d111e26b3ca19035` / `383a1f09cc902d2e147266caf50fcc02fc316261` |
| D12b commit diff / tree SHA | `9977acdbd7eea5b4efb41b0eb1836907363ec49c08a4cf37890428cf98f62f3c` / `7544b1e3e6fb9ca8ea51fcf8b34f1eb953fda848` |
| D12b patch / SHA-256 | `patches/0030-lowmem-add-encoded-Sign-closure-and-flatten-even-isogeny-stack.patch` / `9977acdbd7eea5b4efb41b0eb1836907363ec49c08a4cf37890428cf98f62f3c` |
| D12b Level-I operation ABI / closure | encoded Sign owner `353,008 B`, alignment 8; exact 51-object Arm closure, zero dynamic `.su` records, allocator/GMP/system-RNG/curated-legacy-free |
| D12b target result | dedicated KeyGen→Sign ELF policy and physical capture PASS; 502,204 B exclusive reservation, 30,276 B unassigned; deterministic execution only, not production RNG, a stack bound, or a timing distribution |
| D12b complete source reproducer | normal, ThinLTO, effective-strict, fatal ASan+UBSan and Arm GNU 15.2.1 closure PASS; final Arm audit `51 objects / 10 archives / 51 members` |
| D12b firmware source / tree | project `0cbd1851527b89de2bec9149257d602b3bdde1bd` / `c3965797a4c5027bdb68636bd5fa26ca810ec179`; Compact `383a1f09cc902d2e147266caf50fcc02fc316261` |
| D12b ELF / UF2 / BIN / map SHA-256 | `5b384a983413a8b2439d6b33d6d49fc7d6687c2e4515f575d67b6e6011735b95` / `6f8c0937e6760453b54748c7c5eeab1b373fd51d4323e7562b15ef5e7e39accd` / `bf1dadb10011a38489c3483b438092f6c987f1862ee09b599ad8ef1c8c56e5bd` / `f3f706cca27ba8a017ba9e0bc1f3a8957a6a594cbac49b55fe34676dc5d34a19` |
| D12b raw / normalized capture / manifest SHA-256 | `dd56cda9c482561cd8b24a15bf86ec9dd51494bb99561cf15c513c511377762f` / `9b6d1652f5ffaaf9bb178ad83a1bcd8bd86a9a707c890ff424ed4c65ca5d3778` / `f16590da654ffa864dceff05dd8de689d476d1261ff35439a8ccf63bb2e61393` |
| D12b physical K/S result | `status=PASS`; KeyGen `2,696.208062 s`, PSP `91,980/131,072 B`; Sign `7,337.883516 s`, PSP `120,452/131,072 B`; MSP upper `2,348/8,192 B`; exact host digests, owner clearing, RNG results all PASS |
| D12c independent worktree | `work/compact-d12c`, branch `lowmem/d12c-bounded-verify` |
| D12c base / bounded Verify closure commit | `383a1f09cc902d2e147266caf50fcc02fc316261` / `6b79cfb5cfe1c756d7061b92038d5069bda66f72` |
| D12c commit diff / tree SHA | `4bb2d2c3042e49fb376ac40572162793c9579c65fe9ee8c2cfbfcd0ae9997d9a` / `91eed0a30c19e53fadde402c20b6e8e74fcb2454` |
| D12c patch / SHA-256 | `patches/0031-lowmem-add-bounded-encoded-Verify-closure.patch` / `4bb2d2c3042e49fb376ac40572162793c9579c65fe9ee8c2cfbfcd0ae9997d9a` |
| D12c Level-I operation ABI / closure | Verify/Open owner `15,428 B`, alignment 4; sequential K/S/V union `353,008 B`, alignment 8; exact 52-object Arm closure and zero dynamic `.su` records |
| D12c target result | integrated static ELF policy and one-boot physical K/S/V capture PASS; 502,336 B exclusive reservation and 30,144 B unassigned; deterministic test RNG only |
| D12c complete source reproducer | normal, ThinLTO, effective-strict, fatal ASan+UBSan and Arm GNU 15.2.1 closure PASS; final Arm audit `52 objects / 10 archives / 52 members` |
| D12c firmware source / tree | project `cb888f88d036c4c9e29a104770f2b92059105867` / `8b8824a3a061437850112c959363e401cf416887`; Compact `6b79cfb5cfe1c756d7061b92038d5069bda66f72` |
| D12c ELF / UF2 / BIN / map SHA-256 | `dda0e6306e751e1d202c0cbeab51a2c0eb68cc8feea766f71d590a08e24bdd37` / `1c792fc157cdbdf7b459f59af48e5ffe6350dd404e4990ad5d3c7cd4a24e941b` / `dc7aae71f6bf17b2bc2e8eb5cc71be5f5bb78347f8370c5d6ea9ef070f04e9cc` / `acbec273299570191c5cac97120e17630bdb763cb6e345608adcaa024289c4f5` |
| D12c raw / normalized capture / manifest SHA-256 | `fffba1ababaa760772688fc33d7070f017f574093a81d86c8c823ce3bead3cd5` / `29034e896a68306a5915d799172215c71396432f2e21eeba87188d2f8fca2442` / `fd6a9e59e906ccfe5649d2f49c6498d4e7b4bd793dadda7120c8b38aa8778f63` |
| D12c physical K/S/V result | `status=PASS`; K `2,696.250983 s`, PSP `91,980/131,072 B`; S `7,337.481041 s`, PSP `120,452/131,072 B`; V `0.813858 s`, PSP `20,768/131,072 B`; MSP upper `2,396/8,192 B`; exact K/S host digests, owner clearing and Verify RNG nonuse PASS |
| D12c evidence integration commit | root `d976d405d605b9a7d853288929fb33b9d760ce11`; raw capture, manifest, checker and exact artifact archive committed; root clean before aggregate execution |
| Final D12b/D12c root aggregate gates | root `d976d405d605b9a7d853288929fb33b9d760ce11`; `CMAKE_OSX_ARCHITECTURES=arm64`, Arm GNU 15.2.1; `./scripts/reproduce_d12b.sh` and `./scripts/reproduce_d12c.sh` terminal PASS |
| D12c historical binary reproduction | capture/manifest and static ELF policy PASS; clean historical rebuild with pinned Pico SDK/picotool cache reproduces archived UF2 and BIN byte-for-byte |
| D12c SCA host profile | source `6b79cfb5cfe1c756d7061b92038d5069bda66f72`; Apple clang 17.0.0; Release/ref/Level-I/RADIX64/GMP-off/ThinLTO; ML2 profile instrumentation enabled; fixed key/message, deterministic RNG classes |
| D12c SCA measured binary SHA-256 | first and reversed-order runs both `9413ae114a7722128dafcc7af2de9ee299d05602ffe9288c0d5dd9775e982ca5`; archived under `results/host/artifacts/sca-d12c-sign*-instrumented` |
| D12c SCA first raw/summary SHA-256 | control `801205b7b4349b13b0d4d7e8e2e70d330cb79135642e07cf89c74cfa596e9b57`; fixed-random `ee02c76217d9c87e5202117b8dbfb8c24a6bee8098f7a411abf4a6b9c4b37909`; summary `a7994ba35ae1dd116f4f3a797c759e3d0d92f780ba9a5e4c13b150bded2ada4b` |
| D12c SCA reversed raw/summary SHA-256 | control `00c0fbb190fedd591b57d48d9aa4d69b2dca3b562c92156344d1d799b19bf3e9`; fixed-random `35005696609cda03d83e1db2d4e71f42bc8df7a01973d6c9490c906e68406953`; summary `31f1fa2e9f1dec8ccdc3e6d73cf3ef958d390acc4f94a0ac16d2ccdceea67f44` |
| D12c SCA result / cross-run artifact | controls t1/t2 `-0.002/0.768`, `0.249/-0.204`; primary `3.590/-5.393`, `3.582/-5.386`; fixed-random timing dependence reproduced; cross-run JSON SHA-256 `1417e4fd3b0ba16a72693cd6020315c41f2e6891cdd0ff96374bb6d216358cd3` |
| D12c SCA claim boundary | positive instrumented-host timing screen only; no key recovery, RP2350 power/EM result, constant-time certificate or production-security claim |
| D13 independent worktree / branch | `work/compact-d13`; `research/certified-norm-sketches` |
| D13 base / commit / tree | Compact D12c `6b79cfb5cfe1c756d7061b92038d5069bda66f72`; D13 `71099e0827d3f0a3b3c705d2eda592c401e0d57d`; tree `8761bccb5b14172e21d7228878fb3fc9379db5c4` |
| D13 commit diff / patch SHA-256 | `5bd76e142edf870cfe127d7c3ba83f3c73f9680f43b57e6e53346092ccf02626` / `2fc0f24e8b258044300b64af6b76450728c2eccc1474d15b2fa80289f866cc14` (`patches/0032-lowmem-add-certified-norm-sketches-with-exact-replay.patch`) |
| D13 Level-I Cortex-M33 ABI | candidate `14,024 B`; phase union `94,912 B`; operation arena `172,080 B`, alignment 8; D12c→D13 `−180,928 B` (`−51.253229%`) |
| D13 Arm frame diagnostics | Arm GNU 15.2.1; `-Os/soft` outer/helper `15,952/56 B` (D12c `15,928/32`); Pico-like `-O3/softfp` `15,936/144 B` (D12c `15,936/80`) |
| D13 exactness evidence | deliberate equal-sketch collision uses exact fallback; 12 frozen PK/SK/SM transcripts byte-identical; RADIX64/RADIX32, Level-I/III/V, ASan+UBSan and 19-TU Cortex-M33 gates PASS |
| D13 timing CSV / JSON SHA-256 | `d5cb281d1ff553392d2aa081fec42a38950d1fbbf06d5c34c99b41a46b6a7f93` / `89ebb6b63359f03c3df9ccf205d08aaa2d3868c97c4bb4bcb50ea7cb677c667a` |
| D13 host timing result / scope | two reversed 30-row runs: KeyGen median ratios `1.003695/1.006464`; Sign `0.999481/0.998980`; descriptive host smoke, not equivalence, D13-only causal attribution or Cortex-M33 timing |
| D13 host SCA binary / repeated result | final D13 source `71099e0827d3f0a3b3c705d2eda592c401e0d57d`; both normal binaries SHA-256 `0688aa374392ab8369f28e10e0acc2ad0c5deffa682db3eefbf7732577cde6a7`; controls `-0.559/-0.816`, `-0.152/-0.432`; primary `3.570/-5.410`, `3.576/-5.374`; cross-run JSON SHA-256 `f9c43833eeb95766a5daec0f88dd1982a7fd6818011c15cc084f5ba7de0b53c3` |
| D13 sketch-attribution artifact | 2,062,361 comparisons, 5,041 exact ties (`0.2444%`), weak counter/timing correlations; profile JSON SHA-256 `488236fb2592460181f56eb90641ee2cad6b23001bc2e5bd8070ca3f31a7093b`; profile binary SHA-256 `0273e2afc96baa7298d2c49bc38dc69f6fcb4e0e704b24940e32b4c6c5468567` |
| D13 integrated firmware source / tree | clean project `dc3289add3213cc7671f9943dfaa3bac770b2709` / `6a65878fd835a93c588f9730e53efa0aa169e8cf`; Compact `71099e0827d3f0a3b3c705d2eda592c401e0d57d` |
| D13 linked ELF / UF2 / BIN / map SHA-256 | `3ed410dc2e5fa2d465dac3d93cf5d3f61678693638cc9cb35c7920ca9883e29f` / `6971b9c84a42e26f08d761becd29c2c0e78b4ac1927ae19feb6b2f5c1a035a9f` / `148b75481516ad4bdf0454c282c528704a7d0f0ae99844708260be9650012a7c` / `8892077f26724e54e6ace740a41b509ad906079244b6fd8226bb0f20db100eed` |
| D13 linked SRAM | main through `.bss` `313,216 B`; guarded owner `172,208 B`; PSP `131,072 B`; other main `9,936 B`; MSP `8,192 B`; exclusive total `321,408 B`; unreserved `211,072 B`; heap `0 B` |
| D13 code-size delta vs D12c | `.text` `+880 B` (`165,212→166,092`); `.rodata`, mutable non-owner payload and stack reservations unchanged; flash load image `+880 B` (`287,504→288,384`) |
| D13 aggregate source/target gate | clean root `dc3289a…`; normal, ThinLTO, effective-strict, fatal ASan+UBSan, 52-object Arm closure and clean scratch RP2350 link terminal PASS |
| D13 raw / normalized capture / manifest SHA-256 | `be762e7d2e79f424f52523981d06dbf0beda475cfcbb534b1218d05d01b3eff0` / `1c7a71c62866e34b320207d67248c2d69be6d0ad246c108d15760c6013094e4e` / `63444a4e47d609ae8dc1506529607dc76955c520dc9a67d52e2a3cf413b1d0ca` |
| D13 physical K/S/V result | `status=PASS`; K `2,698.150029 s`, PSP `91,980/131,072 B`; S `7,611.258527 s`, PSP `120,452/131,072 B`; V `0.814341 s`, PSP `20,768/131,072 B`; MSP upper `2,396/8,192 B`; exact K/S host digests, clearing and Verify RNG nonuse PASS |
| D13 one-run D13/D12c timing ratios | K `1.000704`; S `1.037312`; V `1.000593`; descriptive same-board deterministic pair only, not an equivalence test, slowdown distribution or D13-only causal attribution |
| D13 final aggregate evidence | evidence root `0ccb96918da6492cade65e96e881a513de489225`; clean normal/ThinLTO/effective-strict/fatal ASan+UBSan, 52-object Arm closure, archived physical-evidence validation and scratch RP2350 link terminal PASS |
| D13 historical image rebuild | rebuild fix `b20c008fbcf628fae39f1b3943deb14c9761e84a`; clean historical source `dc3289add3213cc7671f9943dfaa3bac770b2709` rebuilt with Arm GNU 15.2.1 and produced UF2/BIN byte-identical to the archive |
| D13 claim boundary | exact ABI, transcript, linked-SRAM, one-boot target and host-leakage result; no target-speed distribution, constant-time, analog power/EM or production-security claim |
| Integer-backend comparison sources | official v2 `dd133d7aca576c361a270c8e6434832535b42ecc`; D13 fixed `ibz` `71099e0827d3f0a3b3c705d2eda592c401e0d57d`; Level I/RADIX32; Apple clang 17.0.0; GMP 6.3.0 |
| Integer-backend measured host binaries | official system-GMP SHA-256 `9a82d0e2498f45db0cc9221362737a0bdfcbad87a792843f25fdc87022a7bc55`; official mini-GMP `12b9f691716fb66bc8541d2ad5f372584e0bba83c7ea533c6a0af861c24ab7ad` |
| Integer-backend KAT/pool evidence | system GMP, native mini-GMP and 317,696-byte mini-GMP pool each 100/100 PASS; 317,680-byte negative control exhausts; KAT SHA-256 `03898a4b415c0fd038137b1e5716be07ea7641622161aef0777035a96fc9bba8` |
| Integer-backend host runtime / memory / summary SHA-256 | `679de025e935102fcdafbd4f7ce63a8d29cbe010bcabe31ed906ae466017bf6a` / `587630305fcf63845f212ad7f9715a0a4fc793ca69074892466737000411520b` / `4dfe1b8fb47f3a28fb245468eb88baf48b306fbd8957f12e8db3fe34489c5c73` |
| Integer-backend host result | mini/system native median ratios K/S/V `1.3621/1.4497/1.0019`; secure-pool/tracked-mini `4.9441/4.2316/1.0019`; mini Sign max requested/live/pool `67,168 B / 4,830 / 317,696 B` |
| Integer primitive CSV / summary SHA-256 | `682a940361351f414d8d5c02178538ba5c35cb3031919ad8a4e9c8737a033441` / `c5f64f04983c33e6452852d081f538893c3730aad8b31588d177305b5e013d63`; fixed binary `3edaf7d68ff4bea8ecfd18940d1e8b6b73a61d7248baec3b0825603f630e427b`, mini binary `b198f436d4f859d92f6a9513636985d4da2394df331d7ea6887674186e8531c8` |
| Integer primitive result | mini/fixed median ratios: multiply `1.7089`, division `1.1775`, GCD `0.4010`, inversion `0.5785`, square root `1.1860`; same-vector canonical result and checksum equality on every row |
| Integer Arm object-size artifact | SHA-256 `ef6f942e3f02203e5fc75baaa699a65b5d9e3c8267a060377900ccc8ea8422a0`; Arm GNU 15.2.1; mini/fixed text ratios `2.3318` (`-Os/soft`) and `2.6145` (Pico Release-like), before final GC |
| Integer allocator sanitizer / KAT logs | ASan 18/18 SHA-256 `5ed147ccec27c431a7899fdab9eb22dcea3310ac3bf2c7d37d1a31b990d50437`; both native KAT logs `5e07c11290fc047689e080b3ae70d4463fa1035a5956c47b4900d6e287c91e38`; pool KAT `ebfcc5d44fffb27e8fe93ff5cb15e86d90f0d985552399a061e655211020185b`; expected exhaustion `bd624ae6eda3656aeac7da94ed656caf2883615f873630bb90b9e700bbb8732a` |
| Integer-backend claim boundary | mini-GMP is correct and sometimes faster; stock form fails this profile's zero-heap/exact-bound/erasure/trace contract. Pool size is an LP64 100-KAT corpus bound, not a lower bound, target total-SRAM result or constant-time claim. |
| RP2350 deterministic KeyGen source / tree | project `64bd99771d34f22f8863c8360ab6d2716a045b2d` / `17b01a109c835027597190ce4a4b4dbc21ed9171`; Compact `f63efb4154ffacbd1e5a6cc6ab0229512bf8d2ce` |
| RP2350 KeyGen target / tools | Pico 2, RP2350 A2 QFN60, `rp2350-arm-s`, 150 MHz; Pico SDK 2.3.0 `98a542c…`; Arm GNU 15.2.1; picotool 2.3.0 `6f6458d…`; libusb 1.0.29 |
| RP2350 KeyGen ELF / UF2 / BIN SHA-256 | `06471f2b50ac7a0cad0a5181bed11e950766e7cbad6bbb26d3b9813543375d96` / `6767f4594e04c9c217909e501759bad030c32a8135fac5a57ff87fa86d79b3b3` / `fe148080989ef8695fa14ce30a9c68f22ecfba9cdf77d9e0a5329660fcfc1a8f` |
| RP2350 KeyGen map / normalized capture / manifest SHA-256 | `3871d2efff147f66c9ef3b981cb51f15be468e16c5a154ec7ac7e6bdebb06fab` / `6d42802c9a98d95fdf757fb67be62723471daedeaa480957c3596584857b2175` / `0910403e587a04cd1b6cd6fe9e17be78fc85308809c8bd201b547f7ecf6ea507` |
| RP2350 KeyGen linked SRAM | main through `.bss` `485,536 B`; guarded owner `353,136 B`; PSP `122,880 B`; other main `9,520 B`; MSP `8,192 B`; exclusive total `493,728 B`; unreserved `38,752 B`; heap `0 B` |
| RP2350 KeyGen physical result | `status=PASS`; `2,696.500982 s`; PSP observed `91,980 B`; MSP conservative upper `2,308 B`; full workspace clear; exact host digest `1de4b175…` |
| Memory trace v0.6 JSON / SVG SHA-256 | `a6b81de36eca565f8a59ab7458cf71729c8e3e1b3c98e8450a047fc416fdea44` / `c2fd32dbe2d0c2fe52bb93c0a0a62325a4b45374124dfe455ddde42fa0ffa811` |

The first commit only narrows resident `find_uv` candidate coordinates to four signed bytes and widens a selected vector before matrix evaluation. The second adds a compile-time capacity proof obligation for the enumeration count. Neither removes the remaining allocator calls; this is not the final embedded configuration.

The portability commit is based on `e0a8620…`. It keeps quaternion `ibz_t`
storage at 27 64-bit limbs while namespacing that limb type as `ibz_digit_t`;
field and curve arithmetic retain their conditional 32/64-bit `digit_t`. It
adds explicit checked conversion at the boundary instead of changing either
precision. The patch is functional portability work, not a memory reduction,
and does not remove the full signer's remaining allocator calls.

The following `a7e1e7f…` commit changes only the id2iso integration test. It
does not alter production objects or protocol behavior. The test now
distinguishes ordinary zero status from fatal failure and regenerates the outer
ideal under a fixed public test bound, matching the production retry scope.

The following `5fdd698…` production correction handles canonical DPE zero
without exponent-sentinel overflow and restores the paper-consistent sequential
ML2 row refresh after insertion. It is an arithmetic correctness boundary, not
a memory optimization. The patch was independently audited before freezing.

The following `c56c441…` production correction removes the reachable `normeq`
signed subtraction overflow while explicitly preserving the inherited wrapped
signed-low-32-bit acceptance predicate. A mathematically canonical mod-four
draft changed a frozen deterministic transcript and was rejected rather than
silently folded into this portability repair. The final namespaced patch was
independently audited before freezing.

The following `ee982a2…` production correction explicitly promotes each random
byte to `uint32_t` before assembling the theta sampler's 32-bit seed. It leaves
the rejection distribution and frozen transcripts unchanged and was
independently audited before freezing.

The following `e61c1fa…` checkpoint introduces the typed full-row
`find_uv_workspace_t` and explicit `find_uv_with_workspace()` boundary. Its
Level-I ABI is 2,044,256 bytes, including four bytes of padding. It preserves
all rows, full-width norms/quotients, sorting and search order, so it is an
ownership baseline rather than a memory reduction. The explicit project-code
closure has no direct allocator/GMP symbol; current protocol KeyGen/Sign still
use the compatibility `find_uv()` wrapper, which performs one allocation per
invocation (one observed invocation in KeyGen and two sequential invocations
in Sign), and
host-libc `qsort` transitive behavior is not covered by that direct-symbol
claim. `scripts/reproduce_c1.sh` verifies the exact commit and patch hash,
clean-rebuilds the archive, and reruns the differential/ABI/Arm gates.

The following test-hardening commit `0f438c2…` adds exact wrapper-versus-explicit
status/output comparison, dirty-workspace entry, strict outer guard adjacency,
invalid-input handling and full-workspace clearing checks. It changes no
production algorithm and was independently approved before D1.

The D1 commit `cf9f6b6…` replaces the 139,776-byte copied row-sort array with a
1,248-byte `uint16_t[624]` permutation. Its exact Level-I typed reservation is
1,905,728 bytes: 1,905,724 components plus four tail-padding bytes. It preserves
the total key `(full-width norm, original enumeration index)`, applies the
result with checked in-place cycles, and removes `qsort` from the explicit
project-code closure. C1→D1 saves exactly 138,528 bytes (6.7764507%). Current
end-to-end KeyGen/Sign still use the allocation compatibility wrapper, so this
freeze is not a full heap-free integration or RP2350-fit claim.

The D2 validation commit `00f4290…` checks every enumerated row norm for strict
positivity before list search begins. This preserves D1's fail-closed behavior
when later D2 search no longer necessarily visits every row. The production
recomputation commit `d680188…` then removes all 4,368 resident quotient
integers and derives the same exact division bound at each invertible candidate
pair. Its exact typed Level-I reservation is 962,240 bytes. D1→D2 saves 943,488
workspace bytes; after the local 216-byte integer increases the frozen frame,
the co-live reduction is 943,272 bytes. Both commits were reviewed separately,
and twelve deterministic transcripts remain byte-identical. The profiler patch
is applied only to temporary measurement clones and is not part of the
production commit. D2 remains a projected caller-owned integration: the full
KeyGen/Sign wrapper still allocates once per `find_uv` invocation (one observed
invocation in KeyGen and two sequential invocations in Sign), and the workspace
still exceeds all RP2350 SRAM.

The D3 prepare-row commit `f70042d…` isolates the exact inherited row
construction sequence without changing storage or protocol output. Commit
`60ce944…` then performs a validation-only pass over all seven rows before
search, retains at most two rows, and regenerates discarded rows in the
original `(j1,j2,i1,i2,v)` order. Independent review found an ISO C
inner-array-boundary UB in the first workspace init/finalize loops; commit
`b54922b…` replaces the flat traversal with explicit two-dimensional indexing.
The corrected exact Level-I ABI is 275,840 bytes: 275,836 components and four
tail-padding bytes. D2→D3 saves 686,400 workspace bytes. Twelve protocol
transcripts and twelve fully instrumented candidate streams are byte-identical
to D2, and host sanitizer, RADIX32 and Cortex-M33 compile/layout gates pass.
The full KeyGen/Sign compatibility wrapper still allocates once per `find_uv`
invocation (one observed invocation in KeyGen and two sequential invocations
in Sign), and the
unflattened D3 workspace-plus-observed-stack path remains above target SRAM;
this freeze is not a heap-free or physical signing result.

The D4 commit `a6b0628…` replaces the four `find_uv_with_workspace` VLA
families with the typed `find_uv_lattice_state_t` member. The exact Level-I
workspace is 353,008 bytes: 77,168 bytes of lattice state, the unchanged
275,840-byte D3 candidate subworkspace, and no additional internal padding.
Arm GCC 15.2.1 reports static 15,928- and 15,936-byte outer frames in the
frozen `-Os` and Pico Release-like profiles; `-Wvla -Walloca -Werror` is a fail gate.
The workspace increase and ancestor-path decrease are both exactly 77,168
bytes, so D4's measured total co-live saving is zero. Both independent audits,
both radices, focused sanitizers, twelve transcripts and twelve complete
candidate traces pass. End-to-end KeyGen/Sign still allocate through the
compatibility wrapper, and the checkpoint is neither a fit nor a performance
claim.

The D5 commit `2771afa…` introduces typed caller-owned ML2 core and retry
storage and overlays the complete 94,912-byte Cortex-M33 retry object with the
later 275,840-byte candidate phase.  The outer Level-I reservation therefore
remains exactly 353,008 bytes.  All three ML2 routes reached before candidate
enumeration use the explicit workspace and the union is securely cleared at
the phase boundary.  On the largest frozen early-ML2 branch, the `-Os` partial
ancestor path falls from 203,384 to 107,264 bytes; after the known out-of-line
ML2 descendants are included, the diagnostic workspace-plus-path sum falls
from 558,040 to 462,824 bytes.  These are individual-TU partial-path
diagnostics, not a linked peak.  End-to-end KeyGen/Sign still enter the
allocation compatibility wrapper and later legacy ML2/MLLL routes remain, so
D5 is neither a heap-free nor an RP2350-fit result.

The D6 commit `15a69ee…` carries that same arena across both fixed-degree
isogeny constructions that follow `find_uv` inside Clapotis.  The public
explicit route uses one `find_uv_workspace_t`, reactivates its `phase.ml2`
member only after the candidate member has been finalized and cleared, and
reuses it sequentially for the `u` and `v` reductions.  The Level-I ABI stays
353,008 bytes.  Under the frozen Arm `-Os` diagnostic, the like-for-like
fixed-degree ancestor path falls from 180,992 to 85,936 bytes; with the known
ML2 descendants included, the explicit Clapotis path plus arena is 441,496
bytes.  Independent correctness, measurement and RP2350 reviews approved the
explicit API transformation.  Current production `protocols_sign` and
`protocols_keygen` still call the legacy arbitrary-isogeny API, however, and
the shared implementation still contains allocator/legacy branches.  D6 is
therefore an API-level checkpoint, not a full Sign measurement, heap-free ELF
or target-fit result.

The D11a commit `78db285…` specializes the complete Level-I/RADIX32 encoded
KeyGen closure. Compile-time non-NULL workspace dispatch removes allocator,
GMP, stdio and curated legacy large-stack/fallback symbols from every selected
object and archive, rather than relying on runtime dominance or final section
garbage collection. The exact 353,008-byte arena ABI is unchanged. Fresh
non-LTO, ThinLTO, effective strict-alias, fatal ASan+UBSan and Cortex-M33 builds
pass exact source/object/archive/member/external-symbol manifests; three
independent reviews approve the checkpoint. The Arm audit deliberately retains
and inventories eight dynamic frames. The largest currently reconstructed HNF
path is 105,808 bytes including its 13,824-byte VLA and known descendants, so
the arena-plus-path diagnostic is 458,816 bytes with only 73,664 nominal bytes
before unmeasured platform state. D11a is therefore an allocator-free selected
closure checkpoint, not yet a VLA-free Pico ELF, total-SRAM fit result,
production-RNG implementation or physical KeyGen execution.

The D11b commit `9934481…` removes the normal KeyGen HNF route's 13,824-byte
VLA from the specialized closure. A fixed 16-row HNF object shares the existing
ML2/candidate phase, so the outer 353,008-byte ABI does not grow. Exact normal
and workspace HNF differentials, two sanitizer routes, complete Arm manifests
and three independent reviews pass. The largest known HNF path is now 92,024
bytes and the arena-plus-path diagnostic is 445,032 bytes, a net 13,784-byte
reduction from D11a. Seven dynamic frames remain. The two frozen host timing
artifacts compare the complete explicit and legacy routes and do not isolate
D11b or predict Cortex-M33 time. D11b is therefore the first VLA-flattening
checkpoint in the exact D11a selected closure, not a VLA-free Pico ELF,
total-SRAM fit result, production-RNG
implementation or physical KeyGen execution.

The D11c commit `1b9888a…` replaces the selected low-memory theta wrapper's
six VLAs with a 13,844-byte typed workspace. Fixed-u, fixed-v and final
randomized theta calls use the same sequential fixed-degree phase, so the new
object shares the existing 94,912-byte ML2 union and leaves the 353,008-byte
arena unchanged. Runtime bounds reject invalid lengths and point counts before
RNG or output mutation, while the normal compatibility API retains its VLA and
publication behavior. The frozen Pico-like theta branch falls from 68,584 to
54,792 bytes and the selected dynamic inventory falls from seven to six.
Because HNF remains the 92,024-byte global known path, the 445,032-byte
arena-plus-path diagnostic does not fall. The two frozen host artifacts compare
the cumulative explicit and legacy routes and do not isolate D11c or predict
Cortex-M33 time. D11c is a theta-local selected-closure checkpoint, not a
VLA-free Pico ELF, total-SRAM fit result, production-RNG implementation or
physical KeyGen execution.

The D11d-1 commit `a8d30fd…` replaces the selected batched-inversion
`fp2_t` VLAs with a 1,584-byte typed workspace. All selected production
lengths are at most 11, and top-level/theta owners overlay the object only
after the previous phase is dead. The 353,008-byte arena remains unchanged and
the dynamic inventory falls from six to five. HNF remains the known global
maximum: the Pico-like path is 92,032 bytes and the arena-plus-path diagnostic
is 445,040 bytes; the separate `-Os`/soft result is 91,448/444,456 bytes. The
two frozen host artifacts compare cumulative explicit and legacy routes and do
not isolate D11d-1 or predict Cortex-M33 time. D11d-1 is a local
selected-closure VLA removal, not a VLA-free Pico ELF, total-SRAM fit result,
production-RNG implementation or physical KeyGen execution.

The D11d-2 commit `434e093…` replaces both selected Tate/Weil dlog VLAs with
a 1,152-byte typed power-table object. It shares the existing 1,584-byte field
scratch union, so the arena remains 353,008 bytes. The selected dynamic
inventory falls from five to three, while HNF remains the 92,032-byte known
global path and the arena-plus-path diagnostic remains 445,040 bytes. The two
frozen host artifacts compare cumulative explicit and legacy routes and do not
isolate D11d-2 or predict Cortex-M33 time. D11d-2 is a local selected-closure
VLA removal, not a VLA-free Pico ELF, total-SRAM fit result, production-RNG
implementation or physical KeyGen execution.

The D11d-3 commit `f63efb4…` replaces the final selected fixed-precision MP
VLAs with nested 144/504/936-byte workspaces. The maximum object shares the
existing 1,584-byte pairing member, leaving the arena at 353,008 bytes. The
selected Arm dynamic inventory falls from three to zero and the exact closure
passes `-Wvla -Walloca -Werror`. HNF remains the global known path:
91,448/92,032 bytes under the two frozen profiles and 444,456/445,040 bytes
with the arena. The normal benchmark binary is byte-identical to D11d-2, so no
D11d-3 timing result is claimed. This is selected-object VLA flattening, not a
final Pico link, total-SRAM fit result, production-RNG implementation or
physical KeyGen execution.

## Host toolchain freeze

| Item | Value |
|---|---|
| Host | Apple `Mac17,2`, arm64, 24 GiB RAM |
| OS | macOS 26.3.1 (Darwin 25.3.0, build 25D2128) |
| C compiler used for release baselines | Apple clang 17.0.0 (`clang-1700.6.3.2`) |
| Diagnostic compiler | Homebrew GCC 15.2.0 and Apple clang 17.0.0 |
| CMake / Ninja | 4.2.1 / 1.13.2 |
| GMP | Homebrew GMP 6.3.0 |
| Arm cross compiler rejected for firmware | Homebrew `arm-none-eabi-gcc` 16.1.0; lacks the required target C libraries on this host |
| Frozen firmware cross compiler | Arm GNU Toolchain 15.2.Rel1, GCC 15.2.1 (`20251203`, build `arm-15.86`) |

Release host builds use `-O3 -DNDEBUG`; the upstream CMake also enables LTO. Apple clang required `-Wno-error=macro-redefined` for the official repository’s collision with the platform `TARGET_OS_UNIX` macro. This compatibility flag does not change arithmetic.

## RP2350 toolchain and transport freeze

The unversioned local Pico SDK 2.2.0 archive is not used. The official Pico SDK 2.3.0 tag at `98a542c…` is cloned with its recorded submodules under `external/pico-sdk`.

The connected device was observed as:

```text
UF2 Bootloader v1.0
Model: Raspberry Pi RP2350
Board-ID: RP2350
BOOTSEL volume: /Volumes/RP2350
```

The project probe is configured with `PICO_BOARD=pico2`, `PICO_PLATFORM=rp2350-arm-s`, `CMAKE_BUILD_TYPE=Release`, one Arm core, flash/XIP code, USB CDC enabled, and UART disabled. Physical execution reported Cortex-M33 CPUID `0x411fd210`, core 0, 32-bit pointers, a 150 MHz system clock and a 48 MHz USB clock. Generic BOOTSEL metadata did not expose the chip revision; the later USB picotool query identified revision A2/QFN60.

The first SDK-built picotool lacked USB/libusb support on this host. A second
frozen picotool 2.3.0 build at `6f6458d…`, linked with libusb 1.0.29, can reset,
load, verify and boot the board by USB serial `14445FEBE379FC1B`; UF2
mass-storage remains a fallback. Picotool identifies RP2350 revision A2 in a
QFN60 package. No CMSIS-DAP/SWD probe has yet been observed.

## RP2350 deterministic KeyGen artifact freeze

The first physical KeyGen artifact is a dedicated Level-I/RADIX32 closure with
the D11d-3 arena and deterministic test RNG.  It is separate from the Verify
firmware and deliberately excludes Sign and the production entropy adapter.

| Item | Value |
|---|---|
| Project source commit / tree | `64bd99771d34f22f8863c8360ab6d2716a045b2d` / `17b01a109c835027597190ce4a4b4dbc21ed9171` |
| Compact SQIsign source | `f63efb4154ffacbd1e5a6cc6ab0229512bf8d2ce` (D11d-3) |
| Build target | `pico2`, `rp2350-arm-s`, Release, Level I, `RADIX_32` |
| ELF / UF2 / BIN SHA-256 | `06471f2b…` / `6767f459…` / `fe148080…` |
| Exclusive SRAM / unreserved | 493,728 / 38,752 bytes |
| Physical result | exact host transcript; guards and clear PASS; 2,696.500982 s; `status=PASS` |

The exact normalized capture and complete machine-readable accounting are
`results/rp2350/keygen-64bd997.txt` and
`results/rp2350/keygen-64bd997-manifest.json`.  Later documentation commits do
not change or relabel this firmware source commit.

## RP2350 Verify artifact freeze

The first physical cryptographic artifact is a verification-only Level-I/RADIX32 closure. It deliberately excludes KeyGen, Sign, quaternion and IdealToIsogeny sources.

| Item | Value |
|---|---|
| Project source commit embedded in firmware | `40d06653130039dd57304e4d5339c6b575e14c01` |
| Source state reported by firmware | `dirty=0` |
| Compact SQIsign source | `5b94b09a1dbbdcc8b91749fec83a9f111ef9cce3` |
| Build target | `pico2`, `rp2350-arm-s`, Release, Level I, `RADIX_32` |
| ELF SHA-256 | `d0d8660cc5b9650ead4e89a19f14b07993f9b5cc54ae05a79a7a349674008973` |
| UF2 SHA-256 | `685a311857040e8321e0976f50211a3fd14425418340acf6bf56f07f9c30135b` |
| Raw BIN SHA-256 | `f1e11ae2b7cb950f6413b4306c47e76f100847d2c91eb1c398eff8b0b9b13d10` |
| Physical result | Both valid fixtures accepted; malformed cases rejected; `status=PASS` |

The exact capture and machine-readable accounting are `results/rp2350/verify-40d0665.txt` and `results/rp2350/verify-40d0665-manifest.json`. Later documentation commits do not change or relabel this firmware source commit.

## RP2350 powmod side-channel diagnostic freeze

The first target side-channel localization image is separate from production
K/S/V firmware. It invokes the D12c Level-I/RADIX32 `ibz_pow_mod` directly with
a GPIO trigger and controlled 521-bit exponent classes.

| Item | Value |
|---|---|
| Project base embedded in firmware | `cd13b00ee429999fbf6cc41d4c897b42440aa75b`, `dirty=1` diagnostic state |
| Compact source | `6b79cfb5cfe1c756d7061b92038d5069bda66f72` (D12c) |
| Compiler / SDK | Arm GNU 15.2.1 / Pico SDK 2.3.0 `98a542c…` |
| Target / trigger | Pico 2 `rp2350-arm-s`, 150 MHz / GPIO 2 active high |
| ELF / UF2 / BIN SHA-256 | `64e27a7c…` / `499787ca…` / `e185d171…` |
| Run 1 control / fixed-random CSV | `d5528b34…` / `57afcdbd…` |
| Run 1 capture / summary | `e3161f6f…` / `b4d04f28…` |
| Run 2 control / fixed-random CSV | `3c24926f…` / `2cfd9d0f…` |
| Run 2 capture / summary | `c6f564a8…` / `3945203f…` |
| Cross-run comparison SHA-256 | `b4ddbfb688df40d73fe9906793651aa9d3121fad590babef910f0382565a28f1` |
| Target timing result | Pearson 0.999860; 4,004.02/4,004.06 µs/set bit; R² 0.999720 |
| Same-input cross-run result | Pearson 0.999999999; median/max absolute difference 2/5 µs |

The exact firmware quartet is in
`results/rp2350/artifacts/sca-powmod-499787c/`. The raw and derived records are
`results/rp2350/sca-d12c-powmod-*`. This is a coarse target-timing and GPIO
trigger artifact, not an analog power/EM or key-recovery artifact.

## RP2350 powmod branch-regularization diagnostic freeze

The first countermeasure image reuses byte-identical D12c/D13 fixed integer
arithmetic but replaces the diagnostic exponent-bit branch by 521 fixed
iterations, two modular multiplications per iteration and mask selection.

| Item | Value |
|---|---|
| Project base embedded in firmware | `7e7e8f9fac75929f775fcb9feb5a5fd9d3be0a35`, `dirty=1` diagnostic state |
| Compact source | `71099e0827d3f0a3b3c705d2eda592c401e0d57d` (D13) |
| Fixed `intbig.c` SHA-256 | `9f7bb4222684374912cffe15001d89f74a3e8c9b71f759225fdd652f3d4b1a31` (same as D12c) |
| Candidate source SHA-256 | `5d5d136bdaf112e7f61efc51e7305f132a09aa1f4b077ffeb07f066be3ceda1e` |
| Compiler / SDK | Arm GNU 15.2.1 / Pico SDK 2.3.0 `98a542c…` |
| Target / trigger | Pico 2 `rp2350-arm-s`, 150 MHz / GPIO 2 active high |
| Candidate/helper/window `.su` | 1,128 / 232 / 32 bytes, all static |
| ELF / UF2 / BIN SHA-256 | `d40fda1a…` / `7c4484d8…` / `1c848567…` |
| Run 1 control / fixed-random CSV | `91c55b9e…` / `1b4acaf8…` |
| Run 1 capture / summary | `6c7844b7…` / `26e63556…` |
| Run 2 control / fixed-random CSV | `e0bf27f5…` / `39bac37b…` |
| Run 2 capture / summary | `b747a9df…` / `27b1476f…` |
| Cross-run / legacy comparison SHA-256 | `ebfc990b…` / `c3fd0025…` |
| Target result | minimum 219.66x absolute weight-slope reduction; 1.33872x random-input median time |
| Residual result | random/fixed SD at least 680x; same-input cross-run Pearson 0.9999987 |

The exact image set is under
`results/rp2350/artifacts/sca-powmod-always-7c4484d8/`, with raw/derived rows
under `results/rp2350/sca-d13-powmod-always-*`. The dirty flag reflects the
explicit experimental workspace; all relevant source, binary and data hashes
are frozen. This is a branch-regularization artifact, not a constant-time or
analog-SPA-resistance release.

## RP2350 powmod fixed-work diagnostic freeze

The second countermeasure image fixes the exponent, multiplier-bit and
modular-addition schedules for a narrow 521-bit diagnostic domain. It remains
separate from production Sign.

| Item | Value |
|---|---|
| Project base embedded in firmware | `7e7e8f9fac75929f775fcb9feb5a5fd9d3be0a35`, `dirty=1` diagnostic state |
| Compact source | `71099e0827d3f0a3b3c705d2eda592c401e0d57d` (D13) |
| Candidate C / header / test SHA-256 | `60602faedb2768940d477178274c9287a99996479c8166e6353afb29f68f385c` / `e80e0f10ef9cdad64b54df3b599a68a80f6783749e9244f798c3a98593c39e50` / `5f9edeed4fd5a4f790e8854bab230bafef2b8d78d47b57280f06e4704061fded` |
| Compiler / SDK | Arm GNU 15.2.1 / Pico SDK 2.3.0 `98a542c…` |
| Fixed schedule | 521 exponent rounds; 1,042 modular multiplications; 542,882 multiplier-bit rounds; 1,085,764 modular additions |
| Add / multiply / wrapper / pow / window `.su` | 88 / 56 / 896 / 1,000 / 32 bytes, all static |
| Object / `.su` SHA-256 | `9760f2aadd6f43a53168fc5b6603939b7ce57d84f44a388c39e921bc0dc60d38` / `3da29eb7f050a74f36522b65633ab84ef8549afb7a73219de0abc75ee44c2ab9` |
| ELF / UF2 / BIN SHA-256 | `0c7fed494f2cca7dfa59330e17f1d9cb05b09543373552cb3976aa7c8546cd44` / `67896440f672d3a5c837b84d5b9ea4afc19d3061495cccd868a7b48206c1b95f` / `9639ddb333fc51c1529894f263c0e52564783f7c008f6300542da4809a744392` |
| Map / disassembly SHA-256 | `53d5ee6e34306a346105b38a73f1791e4cb1ec4961b74cdaf2ba2d4c9d3d6d82` / `7453a41bc11f0287eef7a59183275471a948d08070fabd030d93f715dc53a8f9` |
| Build / reproducibility / Pareto SHA-256 | `6ba6712622128d992f77834142052701731400f4c4bb5c97f908b265e610f56d` / `7900c906ef5ad516fd637f430c5ac25a7e18d991da774e65b6f68a6b8d69085a` / `62b19d4489f83f71d58c98746f20cb62710b704a0461632d93d9075ca556e992` |
| Pareto result | ≥41,843.94x legacy weight-slope reduction; 1.02426x legacy median time; +400 B diagnostic text |
| Residual result | pooled fixed−random median `-5 µs`, paired `t=-7.7963`; control warnings remain |

The artifact directory is
`results/rp2350/artifacts/sca-powmod-fixedwork-67896440/`; raw/derived records
are `results/rp2350/sca-d13-powmod-fixedwork-*`. The reproducible class offset
keeps the coarse timing gate positive. This is not a constant-time, analog-SPA,
full-Sign or production-resistance release.

## D13 Sign-to-Cornacchia SPA mapping freeze

| Item | Frozen value |
|---|---|
| Attack reference / source base | ePrint 2025/830 / D13 `71099e0827d3f0a3b3c705d2eda592c401e0d57d` |
| Instrumentation patch / source SHA-256 | `a4c4264f2f8b835cd54ce453fe3462f245b733da728b1f4ff725187e52129d4b` / `dd03dc39f7d50a590400455f04580b3ef306c4dfcf39039b43a1f38dbe854d47` |
| Control / instrumented binary SHA-256 | `ff93091703ee208b90bfc2de0242bc4b4a9ea0835e1926dadd144890f1e3a43d` / `3f8f048624ed5a0725a4c2c05327da1ec1dc1e89028a6ff4bce5d4905560d635` |
| Profile log / CSV / JSON SHA-256 | `152ab374b41c800205560e840c24ad6cddcf50fc2ed8d6194bdb6bdcb5fc7655` / `d9ed06d0147434ae100bdbf83739a121ea6be380b1a0a8329dc17619bf68ac73` / `ab643c838b565031726f7b125d2609170784feda5bfe4111a7e80093a1be370d` |
| Executed corpus | 12 Sign invocations; 361 Cornacchia calls; 13--72/call; modulus 360--378 bits; 171 `mod 8=1`, 190 `mod 8=5`; two-adicity 2--12 |
| Output exactness | all signatures verify; control and instrumented 12-row outputs byte-identical, SHA-256 `73ae9e853fd65b67bbf525f600622ec76b069fd2be8a1e52c0d5146092bcfeb7` |
| Compiled Level-I bound | fixed-degree `<2^492`; random-aux `<2^380`; maximum 492 bits; fixed-work 521-bit margin 29 bits |
| Bound probe / JSON / binary SHA-256 | `e8a3e94ee6f89d3c9c0f75087ce32f7c63c98006f579f2691b0d86f4b43380f3` / `11baf307340c2bf93eda606137c1f6d57c78eebcb24bcf1ea3d645a52d977913` / `e6af69a0d93888d57256b70bb8249b5a83db4705e874b4d1dac9cc519bdf9c9c` |
| Reproduction | `run_cornacchia_sca_profile.sh`, `run_cornacchia_fixedwork_bound.sh`, and `check_cornacchia_sca_profile.py`; an independent fresh run was byte-exact |
| Claim boundary | attack locus and Level-I width precondition only; protected code not integrated, Tonelli--Shanks/call counts variable, no analog recovery, attack not blocked |

## D13 integrated Cornacchia fixed-work freeze

| Item | Frozen value |
|---|---|
| Literature anchor | “Simple Power Analysis Attack on SQIsign”, ePrint 2025/830, AFRICACRYPT 2025, DOI `10.1007/978-3-031-97260-7_12`; related half-GCD timing work ePrint 2023/807 |
| Control / protected source | `71099e0827d3f0a3b3c705d2eda592c401e0d57d` / `c2a80712e32891c5228da3f49f0148993a4ec560` |
| Protected tree / parent diff SHA-256 | `0043a41f170415c97eb2ae2dc3ff7777ff8942d1` / `bbfe8c4801150eeba88845b4d6abc6a12e1277aa5c90d75387ee77b694720da0` |
| Patch / SHA-256 | `patches/0034-experiment-protect-Cornacchia-pow-schedule.patch` / `103f56a4b0bce39153174d7b07f0d7a497f99f1d80cbb4e457514f9a948f754d` |
| Host profile | Apple clang 17.0.0; Release/ref/Level-I/RADIX64/GMP-off/no-IPO; protected build enables fixed-work and profile counters |
| Control / protected binary SHA-256 | `249c11779a74b9d4e2053af42b377931d985aeae43306e5cb726990e0361aa25` / `37529235d6f32060987a3357a93f061ebfda0318b4a4a1764de01a87c1f4c38a` |
| Output SHA-256 | both control/protected `e6827174fb9d4ea1cccd606632271dbc932f2bad8a5d041f1c4a4c08882ac696` |
| Control / protected raw log SHA-256 | `c7fe2c5183a504606dd44a046d2e1a7cdf51cc1893d143048128219e6926ebb4` / `66c4c730c213b3782eb600dd850745b735b80d2090ec3591cae9a3b6e6228672` |
| Work CSV / summary JSON SHA-256 | `41d3b61e832bb2745dad496d8fe7e21a3a0e32259fe5f9b6b4367b7cf218f300` / `ea5433ca1169221613cb92e2932eb1acdfc74ff42a168c976446b2213923c55b` |
| Corpus result | 12 byte-identical Sign/post-RNG transcripts; 361/361 protected square roots succeed; no no-root/fatal result |
| Fixed work | 2,180 pow calls; 1,135,780 exponent rounds; 2,273,360 modular multiplies; 1,184,420,560 multiplier rounds; 2,368,841,120 full additions |
| Descriptive host smoke | 7.94 / 75.01 seconds; protected/control `9.447103x`; one aggregate run only |
| Arm profile / result | Arm GNU 15.2.1 Cortex-M33/RADIX32/Pico-like O3; three-object gate removes exponent-bit branch and legacy fallback, with static audited frames; not a linked full-Sign image |
| Analyzer / frozen checker SHA-256 | `8b45e4c16d8c81b109403563c6ece191db5ea60dda331c30bf8d94c32a60ec15` / `fc1bb5a1aa2a331c9fde6e09e002e745f5b4a7c3792f8d00025a7ea80138a06e` |
| Integration runner / Arm runner / Arm checker SHA-256 | `8ff2534fba72b8cea0c425d8da7f7d22bd1fc2cfa4149b18a8c2ec5b976f1ec0` / `e4d9f906f793981ba0a0d43723e0494b4c21687c8200d64ca35e73f9dd5aebae` / `2f34bab3a31dcdc0a13b398de182848b2e9018001617d1ad64b0e819722eac40` |
| Contract / analog preregistration SHA-256 | `0b8a2accf0552beba4bb7e9c59d9173e3e88ed9efed8654b07c4cf279990c6be` / `02f0097dda13be91dc6564bab4fb1613be6f1372189a6b4d90b452e4a5da81b0` |
| Claim boundary | exponent-bit control mechanism removed on the opt-in Level-I Cornacchia route; Tonelli--Shanks, half-GCD, retries, operand leakage and the rest of Sign remain; no analog recovery, constant-time or attack-closure claim |

## D13 RP2350 protected-Cornacchia calibration freeze

| Item | Frozen value |
|---|---|
| Target | Pico 2 / RP2350 Arm-S, 150 MHz, Level-I/RADIX32, Arm GNU 15.2.1, Release O3, GPIO 2 |
| Firmware scope | 16 public 368-bit residue/prime fixtures; interrupt-masked GPIO window contains exactly one protected square root; not full Sign |
| UF2 / ELF SHA-256 | `b19e9858fae9763cd3058807b8334563bc7b08e35f8144a0f3875b4c96759192` / `1ce62ba989d813dfe4cefe9efac76ef4982753b67dec69b28b07448eae739745` |
| Linked size | text 47,772 B; data 0 B; bss 24,360 B |
| ELF evidence | one protected sqrt, five linked fixed-pow and five O3-folded fixed-multiply call sites, one 4,344-byte clear, no legacy fallback |
| Build / serial-smoke record SHA-256 | `77a8c87fecac5bc0a76b6ab7d571bb3a66b72b24a7b7bfc537c2bbf01361f658` / `375f6164a305ab853014a1644dd2d18471a7b4e0a9c3eb27a44fba0a6def6d40` |
| Board smoke | 5/5 root, PSP and clear checks PASS; A/B 32.024229/32.024063 s; remaining public fixtures 16.004361/64.047796/70.418552 s |
| Residual-work analysis | exact public operation reconstruction explains all five times at Pearson `0.999999999981`; first non-residue and Tonelli--Shanks call counts remain variable; JSON SHA-256 `d66d02f46494c802848fe590d14403edec1193eacad3d99acbb0507402ec6d11` |
| Residual-work analyzer SHA-256 | `0777fe81103f0e20e59d2c3f47d345daf4996242c4b5c79e0d9e865f07457cfa` |
| Build / ELF checker SHA-256 | `446cf1597ba2af6eabefb5595b4bc2ff7374a317609cfdaf166d905f5c6afbae` / `16384d78c3a3bec9044dcb182e7d49b9f221c1f88ff84b3485a0965085d73d67` |
| Flash / capture script SHA-256 | `48ef50602049872b0e9f39f9d156ddf1214cc9cd318d43625276a2c4d5cac48e` / `6fea6bbb6b4406b0ffd5d99ec4a19abb97a963891f21fd9fc2f9ed9a1f0f1a42` |
| Plan checker SHA-256 | `d9cc5a4bc6fc3599b90035ba6e9ec966a94febc0e97edef09cff92745edf18ef` |
| Claim boundary | flashed serial/GPIO calibration only; no current/EM samples, no attack recovery, no analog-SPA or whole-Sign resistance claim; the wide coarse-time range is residual leakage evidence |

## D13 RP2350 one-pow acquisition and residual-surface freeze

The primary physical-acquisition image narrows GPIO 2 to one protected
Montgomery exponentiation.  The residual inventory is an analysis artifact
bound to that image; it is not a replacement for raw power or EM traces.

| Item | Frozen value |
|---|---|
| Protected source base | `c2a80712e32891c5228da3f49f0148993a4ec560`; dirty WIP reconstructed by archived-source hashes in the build record |
| Target | Pico 2 / RP2350 Arm-S, 150 MHz, Level-I/RADIX32, Arm GNU 15.2.1, Release O3, GPIO 2 |
| ELF / UF2 / BIN SHA-256 | `19dab1b1b0165b51ff407c0e6b481831b6f4636099b502809402ac439b0116bf` / `1f8b6ddc81acd98ebc6667d89135c1080de48a29953e4dd3a9e00d2d0c95681e` / `4a67f1d6f73ea061db665c150deeaab6fa9a5bf6b44c34b2d2b4f9f14fa60081` |
| Map / linked text-data-BSS | `c6d69f521382c4aec16b109092d6ed03d88a03a564d2e0d9cbd4755bc546777e` / 46,760-0-20,968 bytes |
| Trigger scope | one IRQ-masked 521-round protected pow; context/exponent setup, validation, 1,248-byte clear and USB outside |
| Serial evidence | 16/16 smoke and 160/160 ten-cycle alternating-order checks PASS; 62,042--62,053 us |
| Build / smoke / ten-cycle / timing-summary SHA-256 | `0103c9364ee89ffac88e78c9a038919243bd9a8f4f03ea99ec227767bcabfad2` / `a1ebb40a438f82a9a4deb450e549c5f079c0fbdf2077f33a89336005a5c3278b` / `677cb417bccccc050f856682f9e3676723e98c2119bfd63d836b092e3e34a048` / `a9d4d07a4afa492ef52105445be4ab4a853eb97eed3faa725d5c8b9cff862dec` |
| Public exponent truth / checker SHA-256 | `b2f2ad5aefa13c628e4d6b32a035a598a8683db8a30de76c6f988454a7de9d52` / `13fc7f265a037ff9968aeb29f1802d91f0968e61293d843a5ebcd25d2311af1b` |
| Analog plan / trace template / attack template SHA-256 | `28f1c37387c21c77ba95ca26189b772923bdc18d13767e7df201d846df13b49b` / `38ba3116724102af252f395a9b592d27890fe9d823cbbf558f1a8490a2a82f08` / `28353d320d229e52f2f1d9f16d971b862ac781b18706a34400cde40138b1f82a`; ANALOG-1 is split into isolated current-C `m=2e+1` and integrated paper-relation recovery, ANALOG-3 requires transformed multi-call GCD scoring for future exponent randomization, ANALOG-5 isolates the exact one-word refresh and preregisters bounded cross-time second-order testing, and ANALOG-4 requires complete dynamic-modulus scope plus final-assembly transition-aware testing for future masking |
| TVLA analyzer / attack scorer / scorer self-test SHA-256 | `83f90ec444f156be5d49f72e8119663b5e6576adcd27d14db5381f8169973605` / `e5da0608c0f6491c444415ac1ba6bd95e277c77ff7ae86316c2e93dac799d800` / `e8834e800f595cc08691cdaef1b622de54c3cb0413f7e480ba60560ffe686afa`; exact synthetic predictions recover 16/16 inputs, one-bit corrupt and no-candidate controls recover 0/16, and all synthetic outputs are campaign-ineligible |
| Published SPA attack closure / checker / Arm runner SHA-256 | `009896049203ae500577d9b27cdd9e63f58768b0f567350003e77a538d560691` / `9f11df9d1badff5295e70ff0b4129bf875163fae8f6d305c496c177623ae2d32` / `c87c0e9a19596411051a2043d23f352615212359f0e704ffd258cb5e5a6207c1`; normal D13 retains the published secret-bit exponentiation prerequisite, while the experimental accepted path removes the legacy schedule but remains unmasked and physically untested |
| Full-Sign host structural raw / summary SHA-256 | first `262f443e39ab434191c4e2a784119aa416772e2bf776255229d91fe6cb71058a` / `725bcff2bdcae234da7b0ae3535bf97028053ba0834bee673560a75a6bb24378`; rerun `e88f93b5eeec8b9d79a5d967c5ebfe91261509813870116553333c2b9857707e` / `db2bb774c9f6178fe7818dac5b0a04be6df036dbb34fc364a45d1df365249b25`; 2/2 fixed/fixed controls are identical and 4/4 fixed/random pairs differ in both edge and address streams in each independent process |
| Full-Sign structural binary / checker SHA-256 | `a928eb337a07fd5e50c141733167c44114846f142eb4632a0e3a27d6188c615a` / `f24bc5084654fd6d40c928c52f58f642693f9bdf5bf2dba0e60d94d1dffc7a35`; host SanitizerCoverage diagnostic only, not formal trace equivalence, final Cortex-M33 behavior, power/EM or key recovery |
| Full-Sign edge-localization first raw / summary SHA-256 | `c90a198708277d41a15dbb4ea8b6bef25d66718e06dd14505d7e8b6d075469b3` / `43418a05fe689f10265b4ba503c89865da391192aa4371729ec69cf55141c8d6`; 1/1 fixed control stable, 4/4 first differences localized |
| Full-Sign edge-localization rerun raw / summary SHA-256 | `c5dd0c3f78dc5d064a815f6a15d8708a4c871c570fbe50c0d9ce6a6470627d34` / `17590a3e68a077f7ce61f845fb9208ac6b2806e342cc33fc36b16951b85d8000`; normalized indices and source classifications reproduce in a clean rebuild |
| Full-Sign localization binary / DWARF SHA-256 | first `86e78a1835b59ee96687c10440936a64da61a622c3cf334fd9fcee5821a05971` / `495da7c5d21979573157f851588b631db051e55cf3ea9d7a09cc3263f92270f7`; rerun `5934becade2d8a444fbec1fb7c13af042572575b62db08c66266580fc8cb9772` / `67375bf7ea043ef959168332f8ab2c1578fb62435b222c9e4a4ecf39b802e8df`; differing Mach-O UUID/build paths are not treated as a source-result difference |
| Full-Sign localization harness / analyzer / runner / checker SHA-256 | `e5bfabbf1644bc49fc57dacb3d56d1ddaae34f482857dce62a41489e9f3fc908` / `c9d6cc6f09b811e484cae42766995b179ad0b3862649bf18e8a37f143362a614` / `c0810f438d551560894c9db06a75cb58b12ff32b24ce3f92f33528dcbd73d848` / `d0f4d7348d0c8410b2f74cdc6a26cd20a88f893a7a04a1a133b3ba246ee3e168`; seeds 1/3 first differ in the used-limb scan and seeds 2/5 in the early-exit comparison; host structural evidence only |
| Full-Sign first-divergence-stack first raw / summary / binary / DWARF SHA-256 | `7aa904a205d24bf4f1c5b5e142fea19dc603f7a5d148ae762f4edb11b932c912` / `9210a1a484724aafdffa1daf180a55af5ca0c6fc4834654ba9847d1b04fe2727` / `af095a60e73fc6ff2a0b13b9453022b08e00d28566e26facf16b52238f957c94` / `513704f0d6fe171059b1dd8a9d50749d9ed618e8ef3b4679289c3b87be4173d1` |
| Full-Sign first-divergence-stack rerun raw / summary / binary / DWARF SHA-256 | `1415624b76974b5cae460a6415cbaeac736067b0b91150a4a1a0022ca55c44ca` / `f486c5b56d5bf2da1eaae5eebbb4b822a91fa5b9bf7ac5c21e6649f4f4278dce` / `df126e5dc1f0977953fee069f63dc138bb9347751bf686a783e034d85ac9ea0e` / `e1f1d303e9ec02c6f134c8f23c57a3e08b80c90731502347d49974aef75b7cd7`; two clean builds agree on all normalized project frames |
| Full-Sign first-divergence-stack harness / analyzer / runner / checker SHA-256 | `f6799072361c31eeb796ffc18a36791e7ffa3798cbc848cf18fca63a224349b0` / `7ee9ca2b68e56db58ef5ca3d0c844605413c0522b85fb292aca01749c695ebcc` / `1dbc96907afa60ab67f2643809774bc3836fc2435b3cb95f8b17fa3e957eece6` / `baa27a2d19c78e59d0ac17ce0c2b88bf02bd811eb119909fca482f27828ace47`; norm-mod division and interval rejection share the random-ideal sampler; remediation/target/physical/resistance false |
| Published SPA input contract / checker SHA-256 | `a96512637d524697c4646d43c3ebf5b7576f05be9c996edac770c19a0b7edfa3` / `26128fc89105be7c68687f64e9dbae3d4d5f35c9d73eb14aa229a9832d4ab0a7`; known paper exponents recover 24/24 inputs via `4e+1`, `8e-3`, `8e+5`, while the isolated current-C Legendre exponent recovers 16/16 moduli via `m=2e+1` |
| Author toy key-recovery evidence / checker / raw SHA-256 | `adaa8b315bb468405f334da6efa3e4400ef8425971bbbddfadaf280f41ce2c94` / `3bc20a5e33aa6f146bee9f98f658207584b4fb08ac75ec46e1839599367cd9db` / `78b8fab3a01dc6d5c1b958fc988278d345cfa20c2a82f59e0ce9b7545cb62d4e`; author commit `3be37656c2b16ff048c8ca51512a7c2dea6f93a9`, Sage 10.7 compatibility only, 54-bit toy intermediates, 12 RepresentInteger successes, 7 StrongApproximation successes and a valid signature; not current-C or physical-trace recovery |
| Residual inventory / checker SHA-256 | `5e925f75009e189fc8fbcf416da4c4410fed4ca17a3f5e2dd64876e918d6b672` / `eafc0e3b6325b655474f0e8ef278326e52e9a7586d003dab0290436737586fae` |
| Published countermeasure comparison / checker SHA-256 | `477b0a48a2d599defcf0dfc7ce6d2e2b20ae1065729b0c76a5ae6029c108fc15` / `adcded4b2b50362f066559432a1ceee0f0aace7facdd7dd3722a2de4970eebae` |
| Resistance plan / checker SHA-256 | `65eac9626c35e7b90c2e25f8bbc2a72c04a5f84e385c1ce7890fe8ad3f9774dc` / `7492cf34be5815812b66cc0d7a029dfa9d963034bc451daf489f978b441b3719`; deployment remains rejected with known timing, structural and random-ideal-control leakage |
| Random-ideal constant-work design / checker SHA-256 | `d57258edc1c87614c15e36b094737bdd79cde1c4d397617564927fb06f99c08e` / `ca705bee9ff6390489b85f022f3fd90f1ab401893329ca0de36e468d34d8460e`; Level-I budget gamma `130` / beta `1`, 769-bit wide coordinate reduction, fixed Level-I square root, outer exhaustion plus coordinate bias `<2^-128`; full sampler/output-ideal proof/target/physical/resistance false |
| Random-ideal fixed-budget component source / header / test SHA-256 | `435bdfa075488f246d002573976cfeca24252c320427d56cfeae49ccc2612f28` / `288e66560747beeff46b82f8b9c018fda87a0f841e94e7e083cb7e9a15f92cf1` / `0c9c7ad3ede69df4c20704e5fb4abca9ccee78098dc196d4721b57c32c49d5a1` |
| Random-ideal fixed-budget contract / checker / Arm checker / runner SHA-256 | `fb37f7dbd9f7fe03995312cf1bfd4d1e79341b2058a7c6295c53f460a7b2175d` / `4ac0a50ffa2d72c2d3e2f7fa405c7aa146d9d6587cb2453757aa58e2ae5d616d` / `202e813234fc1277a4d06aebe250fca89eb89c0eac2a540c3106449a90e12452` / `aef2987f443aa6740bdf2eb5697c6572d9c1d1a00e54eae0a8290b64c0f61bfa` |
| Random-ideal fixed-budget component results | host optimized + fatal ASan/UBSan PASS; coordinate/square-root/primitive workspaces 2,808/4,320/6,480 B; Arm static entry frames 32/48/88 B (`-Os`) and 32/56/88 B (Pico-like), aggregate text 1,104/1,192 B; complete sampler, output-ideal distribution equivalence, physical evidence and resistance remain false |
| Random-ideal attempt raw / log / capture / analysis SHA-256 | `1047ede4e5988ebc5978219f7bda65c56d9dd873338c96a4b5ce7960af6f4fc8` / `faa8b371f8cda5b40c44885e37dcefaee31f11b6bcf81c916b178ad1834a5fc3` / `0e42eaa4f77128f8a2ae6d27e9e5e5d0dbd182d3ba4fbf0188dc59d003a2ef23` / `1d5a75017739060f6360b777cb145ab43407e4a9682315e69d35d679f46ba176`; twelve frozen Sign seeds, legacy gamma `{1:5,2:5,4:2}`, beta `{1:12}`, descriptive only |
| Random-ideal profile binary / callback / capture / analyzer / runner / checker SHA-256 | `36580104e7580fe9291cb15a303221c00281bf088f0d629dd527ed69cc003666` / `3677ef2502205ae080b22fa26c8f38d89065b9d374bd4cf87658d27f3deade7c` / `60c653f79994e14a49efeeb7f8e56e46308ade50982d47be9ba731d900614264` / `08f7c052c6dca8b89d8f14edc022836c3dc7ef2ac1ff969048b2e07d953e50e2` / `664bc3ba3887d2b39520043784c81167a562f24c455318060f3b31256f1adcb5` / `6b5d5389eb90e3703a3b15bef25b9c9589b10fc0c0bffff792ae6dd283491dd4`; effective `-O0` read-only LLDB, host diagnostic only |
| Literature source pins | CT Quaternion `3b9281d50468819f93879679d084ef87e2f38961` (Apache-2.0 NIST2 subtree); CTLLL `1fd09e1934e72737706dac21aa02a36bfdf245de` (GPL-2.0); official SQIsign anchor `dd133d7aca576c361a270c8e6434832535b42ecc` |
| Countermeasure routing | 5 primary papers, 8 candidate families, all 12 residual surfaces mapped; 21 fixed-width operations including exact/floor square root and half-GCD are component-checked, and the half-GCD subset is integrated structurally on the experimental route; the whole-Sign host edge/address screen is positive, while every final-target/physical closure claim remains open |
| CTINT1728 source / header SHA-256 | `6934d65c4475a86365664fa89a979d200670e8ace1464244d3d08560583987e6` / `af7864d1ce0ffc3b8bf2ceea2680bec3bdfb18f5e59ac150bfc061d58446dc8b` |
| CTINT1728 component / ibz-bridge test SHA-256 | `7038b7dbf940fa89dcc98890ad9f2bdb3c3a078d7f6e0ab2c82ea2d0377240db` / `0f2774a5948ac42778fe9dbd20fb0764ba8a48b6f887c4711e715ae50f60d55d` |
| CTINT1728 contract / contract checker / Arm checker / runner SHA-256 | `7f1c02ccbc953f41e77c27ed6cca6374b30d7955b481d4a94dcd81a7dca25496` / `603a120aa70fcddbf47ee3e2a681a3dc14a43e6461cdab8f138171ecd9cf6b79` / `31a7b10221b131adadc745a62409d26ee21d155c3765fde127f77a06d589ce29` / `a040ee717f8928915b1f5e392610248b32f44efa6cc87b38339814a0e80fd31c` |
| CTINT1728 frozen results | host strict + fatal ASan/UBSan PASS; division 1,728 rounds/1,512 B, reduction 2,160 B, pow 521 rounds/3,240 B, exact and floor sqrt 864 rounds/864 B, half-GCD 1,421 rounds/3,456 B; Arm text 3,560 B (`-Os`) / 4,264 B (Pico-like); static frames ≤80/96 B |
| Half-GCD bound contract / checker SHA-256 | `7b54b3ede32e14a530d7d19b3be2f5f3e825a103bdeb1660b0cffc2fc5b571a4` / `090fc36d42bee38ca43514cc9c78857aaa16ab8472746c75fe935192fe30aedf`; exact source recurrence needs at most 644 active rounds at 492 bits, versus 1,421 scheduled rounds (margin 777) |
| Exponent-blinding rejection contract / checker SHA-256 | `f5dd9ccabec6f43a777b115e97ba185b73ca0be2e67eedff8ad103fd89fafc9d` / `2ff4e473f5d25da1e7faf888fc5535a2d2a91594b296c99b984e1de4c703cde8`; `E=e+r*(p-1)` remains unadopted because recovered `E` retains a structured factorization identity and four-call transformed GCD directly recovers `p-1` in >98% of frozen conditional trials at 128 and 896 blind bits |
| Selector-masking rejection contract / checker SHA-256 | `524b0781b82d079796266867aff327c60893eaaf74c6fcf94bd49ff930305d73` / `80cd49c0d8be19cfc7e8c83e2c0280a6c670adc1c99b5e0c7da32c97de5c4de4`; individual share marginals pass, but sequential-share HD recovers every selector bit in the frozen abstract model; transition-aware software-masking principles are pinned to ePrint 2014/413, 2022/1546 and 2024/755 |
| Masked-Montgomery scope contract / checker SHA-256 | `2683dd5f223caa43c1d523d8d871696a828b56c72a8909e0e9db054518b6c004` / `1f41a1e51bd07fb11303e0930864194284426282b79885eb9beb7ba556d2ca48`; 18 values are secret-reachable in the frozen source graph and six partial scopes are rejected; no masked candidate or resistance claim exists |
| Masked-arithmetic design / checker SHA-256 | `f82e70aed6eb93734db09b81add740e507f6eb37e5380770e47164d61fbb0265` / `e057b93463a54a921999056c8c2508ecd35106564f3a1d7e625b1bdfc124b99a`; exhaustive one-bit Boolean gadgets pass correctness and individual-wire marginals but fail adjacent-share transition checks; the deliberately naive 544-bit Boolean pow proxy needs 183.800 MiB of fresh randomness; a functional two-limb oracle and one frozen refresh subprimitive exist, but a complete transition-controlled arithmetic suite does not |
| Masked-word prototype / checker SHA-256 | `0216b9f53f874719d2351bc72d785254c3dc337c7aa2631e5611868daa68cc3d` / `384dc074e78e506dd831ab3194c329d729ac076eb079489f3f3b27e98706a675`; `Z/(2^64)`, two 32-bit limbs, 1,082,400 reduced assignments plus 200,064 C pairs PASS; five direct adjacent-share HD channels fail, abstract zero-precharge passes, assembly/physical resistance remain unapproved |
| Masked-refresh M33 contract / checker SHA-256 | `7decd795ee515e65cdc6793413e099a9d4255e9670be0687944ab1513f4dfee9` / `9fd8eeb7e41a44d09d6e137fba70d8b1f1a17ffe6bc090b07a8618dc8705a193`; exact 66-byte/23-instruction function, 10,036 QEMU cases, 12 RP2350 serial-smoke records, 299,592 value/schedule assignments and 37,448 exact-instruction/55-channel transition assignments PASS; physical `ANALOG-5`, composability, multiplication and SQIsign closure remain pending |
| Masked-refresh RP2350 build / serial-smoke SHA-256 | `227f064c3b9b4bc0d727fe853e5b35a259009ed4cc3e6772eaa1d3fe501d5d9e` / `b8ab1d448f37787ff3c7bedc458c6eb3dddea6678b6dfacd21fda05875aacd7d`; archived ELF `1ec2a8457b13382f793bf962781efdd532b576771004df3ed1b08c61e5f2aabb`, UF2 `17dee8f0c71e070f5cb371123419ddea3263b634ab71531cedf22dfbde369735`; every class uses one three-call RNG path, `r12` is precharged before GPIO-high, and the image is built/flashed/functionally smoked; analog samples 0 |
| Masked-refresh/attack analog-readiness SHA-256 | `2cc264889c929bb93023aa717154f71a19109563ce7b2b597bf9d8cb4139d9d4`; the 2026-08-23 inventory found the flashed Pico target and cross-time/attack-capable analysis pipeline but no scope/SCA device, current probe, EM probe or acquisition software, so capture remains physically unavailable and analog samples remain 0 |
| Masked-refresh ANALOG-5 dataset / scorer / decision / comparator / checker SHA-256 | `0672c26ccaed1e0d78fff9cf67de84c751e3b9d1d8dbb14c74f2424b938cc954` / `6b3340e0b326675a1c1b3c509533a1d52e5383dd6e42aea02549b48d5f68436c` / `41d18544d78b16e340b14537e6cff560170288c7a978e6113b785fe09abecdba` / `7e3c18a2d934b4697ed72da1383693829482ffbf000ea6e07e36a2cd841b8c72` / `9dae7241861c60d1ecc9052d77df3a1d1a4730f0e34df4d2e014092eb4e58871`; v2 pins exact firmware/formulas, pointwise first-order/centered-square and at most 4,096 cross-time centered-product pairs from two public-timing windows, 12 physical models, 2 permuted null controls and balanced AB/BA pairs. A cross-time-only synthetic leak is detected while each target sample stays pointwise quiet; synthetic evidence remains ineligible and six distinct physical reports are still required |
| Resistance plan / aggregate checker SHA-256 | `3d40270c09f64e9e7de231f5d69342afa9817a9425f7d012b3389ce41318fb0d` / `23feeabee196b369b1156a5fb0858620617ba346dea4215e69f73307877ad0b2` |
| Residual result | 12 open surfaces; ideal source-level selector-mask observation recovers 16/16 public exponents; the frozen host whole-Sign edge/address screen is positive in two runs, while physical observability remains unmeasured |
| Claim boundary | fixed schedule, source/ELF leakage hypotheses, positive host control/address structure, exact public-fixture input inversion and author-toy algebraic backtracking only; no current/EM trace, final Cortex-M33 structural result, current-C secret exponent/key recovery, masking, blinding, whole-Sign constant-time or production approval |

## D13 fixed-round Cornacchia half-GCD component freeze

| Item | Frozen value |
|---|---|
| Component | Level-I/RADIX32 Cornacchia half-GCD adapter over 54×32-bit fixed integers; 1,421 shift/subtract rounds |
| Integrated correctness | 12 frozen Sign seeds, 361 Cornacchia calls and 512,981 fixed rounds; PK/SK/signed-message/post-RNG transcript SHA-256 `e6827174fb9d4ea1cccd606632271dbc932f2bad8a5d041f1c4a4c08882ac696` |
| RP2350 ELF / UF2 / BIN / map | `8b2ce7d21c850df8af326f0ae199435481fd57c999e10235db19c3c0501a646c` / `ce228b151a0c79234d9a51777a2b1ed3ac47388ab28f4dda4129a97f0086692f` / `8a67ff365751739b8ed64c1b7e3683a6ae437043ca815b2503bbb3382ffec389` / `0ea8bff879832943028cd8005530487abb071c239edab236532af72421a77b06` |
| Linked size | text 46,472 B; data 0 B; bss 24,824 B |
| ELF structure | one IRQ-off GPIO2 adapter call; adapter/core workspaces 4,320/3,456 B; exactly one public `bne.n` loop edge; no legacy division, indexed memory, BLX or table branch in the core |
| Build / historical integration contract at build / artifact checker SHA-256 | `6bdc4eb441aca4f7860ac7530f86c10ce2b55bdd86e4ea93cbc4e4c4a8246d3a` / `26c8577ab982d5cd88f7c59442b2b9bf77f0df7ed8fd982768e1d5f584e5c215` / `1e25ba98f2aa23fdc9bc5b82f725b9351e309eb3c90fcfcf8d50682c3a89553b` |
| Current integration contract / checker SHA-256 | `c97597e52f16011a74672c044e43148706e4d91641c46a4ca2f36b656850fe6f` / `fd9bedd13cade6cd19334be6720d05a1a539688cb4f13004f306d3177c58b62f` |
| 30-pair capture / summary SHA-256 | `bb1aa5b67e64e6613265ca610d775c8670b4bb03f916d2dfdb02e6f836408afb` / `55f8c592baadacdd596830aadc9a53abcbcc160df9b86d99680c38e6b5c587dd` |
| Coarse timer result | same-input B−A mean −0.467 µs; 122-vs-95 legacy-step H−L mean +0.200 µs; varying−fixed mean +2.000 µs at 1-µs reporting resolution; no equivalence inference |
| Physical plan / trace template / readiness SHA-256 | `4d2eadb5b22b7c539f806978b396760477628ecb0c31d902b126dba5fa08a7f7` / `0b0cc02ecc7e39d73ba668e84d138f4713dc803a56172d54b03d8d8e0a4bf28e` / `36577bf0f1628184d3f5ebf3582e8cd37092504c44e47066dbdc2d0e0d36e15f` |
| Claim boundary | fixed component control/address schedule, coarse public-fixture timer, and an exact bound for the frozen source recurrence only; compiled-C/model refinement is not formally proved, operands remain unmasked, and there are no analog samples, constant-power, whole-Sign, attack-closure or production claims |

## D13 matched variable-Euclid positive-control freeze

| Item | Frozen value |
|---|---|
| Purpose | deliberately variable Cornacchia Euclid firmware; measurement positive control only |
| RP2350 ELF / UF2 / BIN / map SHA-256 | `3e1fd734a93826f8238df5a5ac30d3974316fa345b5cc8b5652eca70e4d2b9ae` / `5e03b239ae40ac6defa01b5341dcded66ba9b095c6ed9eee8febf300898ca565` / `c7f6652d044c7dd24208d26156a2296d85fca4b808341c0f37838e385f9b672d` / `65adc8ff285eaf48ca541f6ee8505d43dda3e2f113edb78c357eea4d5388d39f` |
| Linked size / structure | text 43,800 B; data 0 B; bss 21,584 B; one conditional Euclid back edge and one compare/divide/multiply call site |
| Build / artifact checker SHA-256 | `2e5f21bcc0d437c827bd0db80a6ebcf90d3344d76dcd27f58ed91dfe6b274e40` / `be56d6e5596755d5da3baedbc19fcbbf7cd42fb2981ced6347dbc965cd8228d9` |
| Legacy capture / summary / matched comparison SHA-256 | `75081efffd9cc1ec1d205a02d44d9c76d3fb8e419e5c44b104c31394e965909d` / `c423f26c4639092bad29becaeda6a14c5c398e0662f7f7f4499031810358fef4` / `14825c7c6dbcf6ef49136d228aa365952484f8943eb9b8cda739a3dd66695802` |
| Coarse timer result | legacy H−L `+1476.8667 us`, CI `[+1474.9005,+1478.8328]`; fixed H−L `+0.2000 us`, CI `[-0.5303,+0.9303]`; matched difference of means `1476.6667 us` |
| Claim boundary | coarse timer sensitivity and known iteration leakage only; no shunt-current/EM positive control, equivalence, physical resistance, whole-Sign or key-recovery claim |

## Combined protected-Sign residual structural freeze

| Item | Frozen value |
|---|---|
| Scope | Level-I/RADIX32 low-memory encoded Sign with fixed-budget commitment sampling, fixed 521-round Cornacchia square root and fixed 1,421-round half-GCD |
| Host compiler / profile | Apple clang 17.0.0; SanitizerCoverage PC tables + flat DWARF; bounded first 4,000,000 events per captured stream |
| Whole-Sign edge CSV / JSON SHA-256 | `a4edccc6818481be797bcb88242243bcc92fd63fea7a91f0b0a683ab93939bfd` / `22aa8a22728e65f346a9b940aa5243499ab2872b8e5df00397c5267a4a37e54e` |
| Edge rerun CSV / JSON / comparison SHA-256 | `356177234f69e52a116b4022cc1b2feeda03bb63fbbad2eed56dbe61f4d5238f` / `e27ba06855dca941338f6669c8d6662ac2517099580eee46eebf1a11605b9c9c` / `5fc2aa4234ad5f0fe0effe6fb93b36761e6cae4113e2fe5233bd3b041e142168` |
| Edge binary / DWARF SHA-256 | `14e39998df5714d9c83ada0d074cb985b0938fe7b6048cda3227b652c415bbea` / `c52eac8bd854096d8f1a0666e7dcf8a46ea13aecd6b9ef3d95f21badd055bea9` |
| Focused address CSV / JSON SHA-256 | `d4276e35fa5e9bfb14acb8f7dbfc612c11e125d3229e5fc1375097a5447864fc` / `9b335d1cb63e74f38488a2d90b30fc30daf1f942402877ecc946aed37514544e` |
| Address rerun CSV / JSON / comparison SHA-256 | `9559d2e49b2ba6fd10ac8ac5137c3214445eb9b6f45da39b1a3411404b709d22` / `b818801310ba5e102abc73939dde12a1a8e4572f7925cf51740d9556fe523c7a` / `1b44394c980182105c99b78a4e15570ce426c93af310cb47ec1969c53b121e64` |
| Address binary / DWARF SHA-256 | `f74ba88de44f5e2439765bce61e035d1dae4970e7e133b4f16f415ee85ce02ab` / `3706330d564bfeafbccb9d210341994e8ce33dec23b6c1c8602f524d301c7fb5` |
| Repeated result | fixed/fixed controls 1/1 identical; fixed/random 4/4 edge differences and 4/4 selected-address differences in both independent processes |
| First localized sources | `ideal.c:168-171` bit-length maximum scan; `intbig.c:385/401/428/839-840` sign handling and used-limb scans in `ibz_bitsize`/`ibz_mul` |
| Contract / checker SHA-256 | `2500440cd1090716a5e9c54bbf299d09cb4e85a28aff9a71857064ffa7623796` / `ea80c3e227cc60c6f484cc3bf7ba587eeeec8932c8949076524ca5d7edcfd78b` |
| Resistance plan / aggregate checker SHA-256 | `be0c89159a64dbbac7787661b00d61be4b1683866f97c4f4c5b055b362207abd` / `d2c50ebe5275f7d947cf4d59a1c37f20c357905dee080399d41d9a545182324e` |
| Claim boundary | bounded deterministic host structural evidence only; no all-input proof, current-protected-C key recovery, physical power/EM, long-term-key leakage, constant time, masking, resistance or production approval |

## 2026-09-04 paper-revision freeze

| Item | Frozen value |
|---|---|
| v2 second-boot capture / summary SHA-256 | `9391e8f81e951629817abe4cd801c8baa692cd3ccd7f3d75ecfb32d35fddc8e7` / `a0d2a79dfce7c98c877de2890632e3edd0bafc6ddda8d602f488642a82390671` |
| v2 target repeat | Archived UF2 `6971b9c84a42e26f08d761becd29c2c0e78b4ac1927ae19feb6b2f5c1a035a9f`; 2 boots of one deterministic input, transcript/PSP equality, no worst-case claim |
| v3 fixed-frame firmware/source | `e564a6413766f1f299db3db1d706478c42f1cc96` / `cb94f242ba791a4ccb980b46c917830b309a9832`, both clean |
| v3 fixed-frame ELF / UF2 / map SHA-256 | `f862703e2937814d321d970ab9ea6286f1f1fbf1d27ebb45d6502eecf54e3ad3` / `f00186922e2e8f55bd0ff69eeae53a8047fde96681379bb47696688d79b4dd30` / `9254c4f31671e181ccab975343701d2f525476dcf1b8c70a21afdf90d8efd0c5` |
| v3 fixed-frame capture / summary SHA-256 | `0f3c878f22b2f13549dbaee0e27c5c307b9564d562da84dbbbb605a37d1f85b2` / `6b8e3fdb6518379cdfdd99f2d21229af835147207a6633cd6b33ea31c69e87ea` |
| v3 fixed-key firmware / sources | firmware `899e572f084c90b7b3be30222e340dc42e380876`; official `6d017708db403bf83977fa70770fc4f7f9e9ff21`; adapted `9293313fb58de4c5ce9dd27a5a9fde0058766c79`; all clean |
| v3 fixed-key official ELF / UF2 SHA-256 | `3a30a6022ce4c9e0c31a71655583a347a2c7728f36e03a69052d6981e43a834c` / `09c10cd9812a14a692c64842f11204f3d69f8ce3f04c2ae962c466ad5d03c314` |
| v3 fixed-key adapted ELF / UF2 SHA-256 | `a8bf87a862d15574de599da491c891119a634e928544b75d316b70c1db9bc335` / `8e5a5973a96f051377b5ae1100c25ec3c91972f7a0c989bd69615db522d3f9b7` |
| v3 fixed-key raw official / adapted / summary SHA-256 | `0edbd872e4f4f0e558511a5ace0c5d4073154db5a4f522375ce288bea79ecd99` / `f3e09c54a9378637ffa432f04f34901f767ef21e697c4e00da3dbf574eef96ab` / `9c5975d6ae6b91c27eb21a0539c12d37baf7a1dde4b82974567f6c4d66ff16d5` |
| Final future-work register / hardware readiness SHA-256 | `46b0fbcba7f8138c4940696501c14de752a78718dc9acb61b7a2cf6a94f5c815` / `a2f664bf61bd0d554d75ce67393d64f3d8a98e1734a39e0c05efffefc9a53b39` |
| v3 lifetime/static thin-bundle SHA-256 | `823e3b0dc251646046ba32b5b26a30e8a6c37197ff1a8192f725080e5d13dde2` / `a7aa0c4d78507606ded4cd2174898882c004ab4ca34ee6320dfd04837d0cd710` |
| Clean v3 official+lifetime / fixed-frame host gate SHA-256 | `5825787bdb4ff6b0782e980289618428da3c8f4101a97acae1b035cad8627539` / `aa8f8fe713021a0d07007c08f6863f76ecaec230922672dec798e67192c51cb4` |
| Reconstructed generated `p324_3/m4f` tree SHA-256 | official `38ee7ae007db2de0366cdb66dfe22f83f66ee5ce3d99f7f6260dc9ab24c3328a`; lifetime `57e1ae2d0d1a16d40f44f5fbcda91b4e588eca5c2f40dda04377bb61e3077ae2`; fixed-frame `7df287fc4e690b93459fa47c9bb6856a49712996ba348fba0abd0eb397383421` |
| Final boundaries | local bounded program complete; whole-program worst-case stack=false; physical traces=0; side-channel resistance=false; other PQC/MCU out of revision scope |

## Change-control rule

No baseline, protocol version, parameter set, or compiler is to be changed silently. A change requires:

- a new row or explicit superseding entry here;
- regenerated KAT/self-test and memory results;
- a note in `RESEARCH_LOG.md` explaining why comparisons remain valid or must be restarted.
