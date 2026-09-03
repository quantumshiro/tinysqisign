# v2 D13の100入力KAT

既知解テスト（Known Answer Test、KAT）用ハーネスは、公式NIST-v2 Level-I requestの100入力を読み取ります。通常のNIST APIではなく、呼出側が所有する作業領域を受け取るKeyGen、Sign、Open、およびVerifyだけを実行します。

各入力で次を検査します。

- KeyGenとSignの成功
- Openによる元メッセージの完全復元
- 署名1 bit改変の拒否
- 作業領域の前後ガード
- 操作後の所有領域の消去

`scripts/prepare_sources.sh`を実行した後、リポジトリ直下から次を実行します。

```sh
./scripts/run_d13_lowmem_kat.sh ./results/local/d13-kat
```

凍結済みの2回の実行は、それぞれ100入力すべてで成功しました。通常D13経路と2回の低メモリ経路の応答もbyte単位で一致しました。詳細は`results/host/d13-lowmem-kat-2026-09-03/manifest.json`にあります。

このKATは、全入力についての形式的同値性、製品用乱数、RP2350上の100入力実行、性能分布、またはサイドチャネル耐性を示しません。

