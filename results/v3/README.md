# SQIsign v3の凍結結果

公式SQIsign v3 commit `6d017708db403bf83977fa70770fc4f7f9e9ff21`と、同commitへ`patches/v3-lifetime-overlays.patch`を適用したv3適応実装を分離して記録しています。

- `host/`: 両系列のNIST API試験、self-test、公式KAT
- `rp2350/`: RP2350の公式KAT captureと、順序を反転した5組の測定
- `analysis/`: 静的監査と局所lifetime overlayの説明
- `version-isolation-manifest.json`: source、build、result、digest、計測境界の対応

`scripts/prepare_sources.sh`と`scripts/generate_v3_pqm4.sh`でソースを準備し、`scripts/build_rp2350_v3_baseline.sh`および`scripts/build_rp2350_v3_d1.sh`で二つのfirmwareを別々に構築できます。

5組10回は単一基板・単一の決定的KAT入力による対比較です。入力分布、複数基板、最悪stack、またはサイドチャネル耐性を示すものではありません。
