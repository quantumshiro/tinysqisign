# certified norm sketchの配置検査

`certified_norm_sketch_layout.c`は、v2 Level Iにおける候補、phase、`find_uv`、および操作全体の型サイズを検査する独立probeです。D13ソースを再構成した後、Arm GNU Toolchain 15.2.1を指定して実行します。

```sh
ARM_TOOLCHAIN_ROOT=/path/to/arm-gnu-toolchain \
  ./scripts/check_certified_norm_sketch_layout.sh
```

RP2350/RADIX32 ABIで要求する値は、候補14,024 bytes、phase 94,912 bytes、操作アリーナ172,080 bytesです。全精度表実装の353,008 bytesから180,928 bytes（51.2532%）減少します。

ホスト時間の凍結CSVから集計値を再生成するには、次を実行します。

```sh
python3 scripts/analyze_certified_norm_sketch_runtime.py \
  results/host/d13-certified-norm-sketch-runtime.csv
```

sketchはビット長と上位64 bitが同じ場合に必ず全精度値を再生成して比較します。したがって近似比較ではありません。一方、再生成回数や探索の終了条件は入力依存であり、この変更は定数時間性を保証しません。
