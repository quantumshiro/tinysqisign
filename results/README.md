# 凍結済み証拠

- `host/d13-lowmem-kat-2026-09-03/`: 公式request 100入力を用いた通常D13経路と低メモリD13経路のKAT
- `host/d13-certified-norm-sketch-runtime.*`: certified norm sketch導入前後のホスト時間
- `host/mini-gmp-*`: mini-GMPのメモリ・時間・KAT比較
- `rp2350/ksv-*.json`および`rp2350/ksv-*.txt`: v2の全精度表実装とD13提案実装の実機証拠
- `v3/`: 公式v3とv3適応実装の版内比較
- `sca/`: 論文で報告するサイドチャネル診断の最終データ

コンパイラや基板を変えた再測定値は、これらの凍結値を上書きせず、`results/local/`などの別ディレクトリへ保存してください。

