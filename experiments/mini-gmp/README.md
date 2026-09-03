# 固定精度`ibz`とmini-GMPの比較

このディレクトリには、論文のmini-GMP比較に用いた最小の計測コードと静的pool allocatorを収録しています。対象は公式SQIsign v2 commit `dd133d7aca576c361a270c8e6434832535b42ecc`です。

公式100-vector KATではsystem GMP、native mini-GMP、静的pool版mini-GMPの三構成が合格しました。凍結LP64 host上で、静的pool版が全100経路を通る最小の16-byte整列poolは317,696 bytesであり、317,680 bytesでは枯渇しました。これは当該allocator、ABI、入力集合に対する測定閾値であり、mini-GMP固有の下界でもRP2350上界でもありません。

比較プログラムは次のように構成できます。

```sh
cmake -S experiments/mini-gmp -B build-mini-gmp -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DGMP_LIBRARY=MINI \
  -DSQISIGN_SOURCE_ROOT=/path/to/the-sqisign-v2
cmake --build build-mini-gmp --target \
  mini_gmp_compare mini_gmp_pool_kat_lvl1
```

凍結結果は`results/host/mini-gmp-*`、個別多倍長演算の比較は`results/host/intbig-fixed-vs-mini-runtime.*`、Cortex-M33向けobject textは`results/rp2350/intbig-backend-object-size.json`にあります。

固定精度化はallocation traceを除き、総SRAM予約を静的に監査しやすくしますが、現在の算術・retry・比較・制御フローを定数時間にはしません。
