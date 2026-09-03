# 凍結済み証拠の索引

論文値に使うterminal-resultのみを現行索引に載せる。開発途中のdirty captureは現行treeから除外し、必要ならGit履歴から回収できる。

- `host/d13-lowmem-kat-2026-09-03/`: v2公式request 100本による通常API対低メモリAPIの差分適合試験。旧公式responseとのKAT一致ではない。
- `host/d13-certified-norm-sketch-runtime.*`: certified norm sketch導入前後のhost時間。
- `host/mini-gmp-*`: mini-GMPのメモリ、時間、公式KAT比較。
- `host/v3-fixed-key-timing-2026-09-04/`: v3固定message/RNG・10鍵host timing screen。
- `host/v3-fixed-key-structural-trace-2026-09-04/`: v3固定鍵の制御/address traceと同一鍵control。
- `rp2350/`: v2の静的メモリ内訳、実際に測定したELF/UF2/map、二回のboot、およびサイドチャネル取得準備状態。
- `v3/rp2350/interleaved-clean-2026-09-04/`: clean firmwareによる5組10 boot。
- `v3/rp2350/multi-input-placement-clean-2026-09-04/`: 10 vector × 2実装 × 2配置。
- `v3/rp2350/static-closure-clean-2026-09-04/`: 固定frame版のclean実機cross-checkとexact binary。
- `v3/rp2350/fixed-key-timing-clean-2026-09-04/`: RP2350固定鍵200 Sign screen。
- `v3/analysis/`: lifetime overlay、静的frame、同期PSP上界、非同期closure境界。
- `v3/host/validation-clean-2026-09-04.txt`: cleanな公式版・lifetime版のNIST API、self-test、公式100-vector KAT。
- `v3/host/d2-static-validation-2026-09-04.txt`: 固定frame版の公式100-vector KAT。
- `revision-2026-09-04/`: 実装同値性certificate、lifetime certificate、自動配置、Pareto表、将来工程台帳。
- `literature/`: v3公開後の検索式、対象database、除外基準、最近接例。
- `sca/`: v2 component-level timing/trace試作の凍結結果。

compiler、基板、clock、配置を変えた再測定値は`results/local/`等へ保存し、上記ファイルを上書きしないこと。
