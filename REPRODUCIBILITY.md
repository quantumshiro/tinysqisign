# 再現手順と証拠境界

本資料では、保存済み成果物の完全性検査、公開ソースの再構成、新しい環境での再実験を区別する。新しい測定値で凍結済み結果を上書きしてはならない。

## 1. 凍結済み成果物の検査

TeX Live（LuaLaTeX/Biber）とPython 3があれば、外部ソースや実機なしで論文が参照する証拠を検査できる。

```sh
shasum -a 256 -c SHA256SUMS
python3 scripts/generate_manuscript_evidence.py
python3 scripts/generate_future_work_status.py \
  --require-v2-repeat --require-local-complete
make -C manuscript eprint-check
```

最後の状態は`results/revision-2026-09-04/future-work-status.json`にある。これは全研究課題の解決を意味しない。ローカルで実行可能な有限実験は完了したが、全program最悪stack上界、サイドチャネル耐性、物理電力・EM測定は未達である。

## 2. ソースの厳密な再構成

`scripts/prepare_sources.sh`は次の四つの独立treeを作る。既存treeは上書きしない。

| 系列 | 基準commit | 再構成後commit | 再構成物 |
|---|---|---|---|
| v2提案実装 | `5b94b09a1dbbdcc8b91749fec83a9f111ef9cce3` | `71099e0827d3f0a3b3c705d2eda592c401e0d57d` | `patches/v2-d13.bundle` |
| v3公式版 | `6d017708db403bf83977fa70770fc4f7f9e9ff21` | 同左 | upstream |
| v3 lifetime版 | 同上 | `9293313fb58de4c5ce9dd27a5a9fde0058766c79` | `patches/v3-lifetime-overlays.bundle` |
| v3固定frame版 | 同上 | `cb94f242ba791a4ccb980b46c917830b309a9832` | `patches/v3-static-stack.bundle` |

```sh
./scripts/prepare_sources.sh
./scripts/generate_v3_pqm4.sh
```

固定frame treeだけを再生成する場合は`./scripts/generate_v3_pqm4.sh static`を使う。このprototypeは`p324_3/RADIX32`専用であり、他のv3 parameterを対象にしない。

準備スクリプトはcommit、tree ID、追跡差分0をfail-closedで検査する。閲覧用の統合差分は`patches/v2-lowmem-d13.patch`、`patches/0035-experiment-v3-d1-lifetime-overlays.patch`、`patches/0036-experiment-v3-d2-static-stack.patch`である。

## 3. v2の正しさと実機証拠

ホスト側は、保存済み公式request 100本を同一commitの通常APIと低メモリAPIへ与える差分適合試験である。

```sh
./scripts/run_d13_lowmem_kat.sh ./results/local/d13-differential
python3 scripts/check_finduv_equivalence_certificate.py
ARM_TOOLCHAIN_ROOT=/path/to/arm-gnu-toolchain \
  python3 scripts/check_lifetime_certificate.py
```

二つの低メモリprocessと通常APIの応答はbyte単位で一致する。ただし、旧公式responseをoracleとするKATではない。試験はOpen、署名1 bit改変の拒否、前後guard、workspace消去も確認する。

RP2350の凍結ビルド環境はPico SDK 2.3.0、Arm GNU Toolchain 15.2.Rel1、Ninja 1.13.2である。再ビルドには次を使う。

```sh
PICO_SDK_PATH=/path/to/pico-sdk \
ARM_TOOLCHAIN_ROOT=/path/to/arm-gnu-toolchain \
PICOTOOL_CMAKE_DIR=/path/to/picotool/cmake \
./scripts/build_rp2350_ksv_d13.sh
```

`results/rp2350/artifacts/ksv-d13-dc3289a/`は実際に測定したELF、UF2、BIN、mapを保持する。同じUF2を用いた二回のbootは`ksv-d13-repeat-2026-09-04-summary.json`で比較できる。これは同一の決定的入力を二回通した証拠であり、複数入力や最悪stack上界の証拠ではない。

## 4. v3の実機campaign

RP2350向け各build scriptにはv2と同じ三つの環境変数を渡す。

```sh
./scripts/build_rp2350_v3_baseline.sh
./scripts/build_rp2350_v3_d1.sh
./scripts/build_rp2350_v3_multi.sh baseline a
./scripts/build_rp2350_v3_multi.sh baseline b
./scripts/build_rp2350_v3_multi.sh d1 a
./scripts/build_rp2350_v3_multi.sh d1 b
./scripts/build_rp2350_v3_d2_static.sh
./scripts/build_rp2350_v3_fixed_key.sh baseline
./scripts/build_rp2350_v3_fixed_key.sh d1
```

論文値に使う凍結campaignは以下である。

- `interleaved-clean-2026-09-04/`: clean firmware harnessによる公式版/適応版5組10 boot。適応ソースは基準commitとdigest固定patchで表現した。
- `multi-input-placement-clean-2026-09-04/`: cleanな公式commitと適応commit、10公式vector、2配置。正当40試行と改変署名拒否40試行が通る。
- `static-closure-clean-2026-09-04/`: cleanな固定frame source/firmware、公式vector 0、link済みoperation-PSP上界とのcross-check。
- `fixed-key-timing-clean-2026-09-04/`: cleanな公式/適応sourceとfirmware、10鍵、2順序、各5反復、合計200 Sign。

凍結firmware commitは順に`467ca5b61a5f6810218ee173850862d629cf07a7`、`76f88dccf6ce9ba1b4d12fc20724b47e03a5f6ab`、`e564a6413766f1f299db3db1d706478c42f1cc96`、`899e572f084c90b7b3be30222e340dc42e380876`である。これらは測定時の履歴識別子であり、現在のartifact repository HEADではない。各summaryはELF/UF2/map/captureのdigestを保持する。

固定frame版の108,300/127,932/40,468-byte K/S/V上界は、この一つのlink済みimageの同期operation PSPと最大Secure例外entry一回を対象とする。handler callback 18地点、IRQ nesting、live MSP、runtime vector網羅性は閉じていないため、whole-program上界ではない。

## 5. サイドチャネル評価

`experiments/sca/`には事前条件、入力設計、software trace、固定work試作、残存面台帳がある。v3 target固定鍵screenは200/200署名を検証し、鍵に対応するwall-clock順位を二順序で再現した。公開鍵成分と秘密鍵成分が同時に変わるため、秘密だけへの因果帰属、物理漏洩、鍵回復、耐性を示さない。

`results/rp2350/sca-analog-readiness-2026-09-04.json`は取得準備と不足機材を記録する。現環境には電流/EM probeとscope/SCA取得器がなく、物理traceは0件である。計測していないデータをsoftware結果から補間してはならない。

## 6. 論文

```sh
make -C manuscript
make -C manuscript eprint-check
```

生成済み`manuscript/main.pdf`も収録する。主要数値は`manuscript/generated/`へJSON/CSVから生成される。数値変更時は一次成果物、生成表、本文、metadataの順で更新し、最後に全hashを再生成する。
