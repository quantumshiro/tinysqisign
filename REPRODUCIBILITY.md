# 再現手順と証拠の対応

この文書は、論文の主要な結果を再構成または検査するための最小手順を示します。凍結済みの測定値と、新しい環境で得られる再測定値は区別してください。

## 1. ソースの再構成

`scripts/prepare_sources.sh`は、次の公開リポジトリを取得します。

| 系列 | 公開元 | 基準commit | 適用する差分 |
|---|---|---|---|
| v2 | `pqc-lab-ku/compact-SQIsign` | `5b94b09a1dbbdcc8b91749fec83a9f111ef9cce3` | `patches/v2-d13.bundle` |
| v3 | `SQISign/the-sqisign` | `6d017708db403bf83977fa70770fc4f7f9e9ff21` | `patches/v3-lifetime-overlays.patch` |

v2のGit bundleは、研究時の凍結commit `71099e0827d3f0a3b3c705d2eda592c401e0d57d`と、その基準commit以後の履歴を保持します。Gitツリー識別子は`8761bccb5b14172e21d7228878fb3fc9379db5c4`です。閲覧用の統合差分`patches/v2-lowmem-d13.patch`を適用しても同じGitツリーを再構成できます。v3の変更後ファイル`lll_dim4.c`のSHA-256は`9a9b6643d53ad14320b6c584443ee1a8bb939c580669d5d96e95d2ec703ec403`です。

準備スクリプトは既存の`work/compact-d13`、`work/official-v3`、または`work/v3-lowmem-d1`を上書きしません。ネットワーク接続とGitが必要です。

## 2. v2の100入力KAT

ホストKATにはCMake、Ninja、C11コンパイラ、およびPython 3が必要です。

```sh
./scripts/prepare_sources.sh
./scripts/run_d13_lowmem_kat.sh ./results/local/d13-kat
```

低メモリ専用ハーネスは、呼出側が所有するKeyGen、Sign、Open、およびVerifyの作業領域APIだけを呼びます。各入力では、正常な署名付きメッセージの復元、署名1 bit改変の拒否、前後ガード、および作業領域の消去も検査します。

凍結結果は`results/host/d13-lowmem-kat-2026-09-03/manifest.json`です。通常D13経路と2回の低メモリ経路が生成した応答のSHA-256は、いずれも`1d86d15c5d2bfbef99f17634496086a3ab80f0e848d34d3e9409d75f3019dd68`です。

旧GMP版の応答は比較基準ではありません。固定精度KeyGenは旧GMP版と異なる順序で決定的乱数を消費するためです。本KATが確認する範囲は、公式requestの100入力における同一D13アルゴリズム内の通常経路と低メモリ経路の一致です。

## 3. RP2350のv2ビルド

凍結測定ではRaspberry Pi Pico 2、Pico SDK 2.3.0、Arm GNU Toolchain 15.2.Rel1、およびNinja 1.13.2を使用しました。Pico SDKとArmツールチェーンを別途用意し、次の環境変数を指定します。

```sh
PICO_SDK_PATH=/path/to/pico-sdk \
ARM_TOOLCHAIN_ROOT=/path/to/arm-gnu-toolchain \
PICOTOOL_CMAKE_DIR=/path/to/picotool/cmake \
./scripts/build_rp2350_ksv_d13.sh
```

ビルド後の監査は、ヒープ、GMP、システム乱数、動的スタックフレーム、および旧来の大規模スタック経路がリンク済みELFに含まれないことを検査します。凍結済みの実機値は`results/rp2350/ksv-d13-dc3289a-manifest.json`にあります。比較対象の全精度表実装は`results/rp2350/ksv-cb888f8-manifest.json`です。

## 4. v3の再構成とRP2350ビルド

`scripts/prepare_sources.sh`は、無改変v3を`work/official-v3`へ、v3適応実装を`work/v3-lowmem-d1`へ分離します。RP2350向けソースは次のコマンドで生成します。

```sh
./scripts/generate_v3_pqm4.sh
```

生成後、`scripts/build_rp2350_v3_baseline.sh`と`scripts/build_rp2350_v3_d1.sh`を実行します。両ビルドスクリプトには、v2と同じ3個の環境変数を指定してください。

5組10回の実機測定値は`results/v3/rp2350/interleaved-2026-09-03/`にあります。`results/v3/version-isolation-manifest.json`は、v2、無改変v3、およびv3適応実装のソースと測定境界を分離します。

## 5. サイドチャネル証拠

`results/sca/host/`は、v2署名の固定対ランダム時間検査、制御フロー列、および実効アドレス列の最終データを収録しています。`results/sca/rp2350/`は固定work指数演算試作の要約を収録しています。これらは現実装の入力依存性を示す診断結果です。電力・電磁波測定または鍵回復実験の代替ではありません。

## 6. 論文

TeX Live、LuaLaTeX、およびBiberを用意して、次を実行します。

```sh
make -C manuscript
```

生成済みの`manuscript/main.pdf`も収録しています。数値を更新するときは、先に対応するJSONまたはCSVを更新し、その後に論文を更新してください。
