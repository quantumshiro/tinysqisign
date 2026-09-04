# 今後の研究工程：実施状況台帳

Freeze date: **2026-09-04 (Asia/Tokyo)**

この台帳は、論文の旧「今後の研究工程」に列挙した項目を一件ずつ追跡します。有限の試験を完了したことと、一般的な性質を証明したことを区別します。

| ID | 工程 | 状態 | 結果と境界 |
|---|---|---|---|
| `FW-V2-TARGET-REPEAT` | v2実機反復・操作PSP上界 | 限定完了 | 同一UF2・同一決定的入力を2 bootで完走し、transcriptとPSP深さが一致。測定ELFとtext一致するclean再構築版で操作PSP上界を92192/120664/20980 bytesとし、全て128 KiB予約内。handler/MSPを含む全program上界ではない。 |
| `FW-V3-MULTI-PLACEMENT` | v3 D1複数入力・複数配置 | 限定完了 | 10公式ベクトルを2実装×2配置で検査。正当K/S/V 40件と改変拒否40件が通り、1024-byte配置移動後もPSP深さの不一致は0/80。 |
| `FW-V3-TWO-FUNCTION-ALL-PARAMETERS` | v3 D3二関数・全parameter | 限定完了 | 二関数の三領域を重畳。公式版/D3の全三parameterでAPI・self-test・各100 response、計600ベクトルを通過し、Arm frameは全6比較で減少。RP2350 p324_3の一経路ではSign PSPを4664 bytes削減。他二parameterのwhole-image実機fitとD3時間分布は未評価。 |
| `FW-V3-STACK-BOUND` | v3 stack上界 | 限定完了 | 固定frame試作で動的frame記録を19件から0件へ変更したが、実測PSPは減らなかった。link済みK/S/V根のsoftware callと最大例外entryを含むPSP上界は108300/127932/40468 bytesで、全て128 KiB予約内。clean closure imageの公式vector 0でも全観測PSPが上界内。割込み候補4根の直接call metadataは閉じたが、handler側間接callback 18地点とIRQ/MSP nestingが残るため全program上界ではない。 |
| `FW-AUTOMATION` | 配置・図表の自動生成 | 試作完了 | contract compilerが172080 bytesを再現し、宣言memberと直接accessのcoverage gateを追加。証拠生成器がSRAM・flash・cycle関係とPareto座標を再計算する。phase、間接alias、escape、消去の完全性は人手監査に依存する。 |
| `FW-LITERATURE` | v3公開後の文献再監査 | 完了 | 2026-09-04時点の検索式・データベース・除外基準・最近接反例を保存。否定的検索結果はv2の六条件の論理積だけに限定した。 |
| `FW-V2-SCA-HARDENING` | v2固定work・漏洩再検査 | 実験完了（負） | 固定budget samplerとCornacchia固定scheduleを統合した。全Signの制御・address差は再現し、残存面12件が開いているため耐性は不成立。 |
| `FW-V3-SCA` | v3漏洩評価 | 実験完了（負） | targetの10入力順位に加え、hostでmessage・署名乱数を固定した10鍵×30反復×2順序を実施し、公式版とv3適応版の双方で鍵別順位を再現した。RP2350でも同じ10鍵・固定message/RNGの200 Signを測定し、二順序で鍵対応timingを検出した。host構造traceを各実装2 processで実行し、同一鍵control 8組は一致。鍵対応の制御差=true、address差=true。物理漏洩は未判定。 |
| `FW-PHYSICAL-SCA` | 電力・EM実験 | 外部機材待ち | 取得firmware・解析器・合成positive controlは準備済み。電流/EM probeとscope/SCA装置が未接続で、物理traceは0件。 |
| `FW-CROSS-SCHEME-MCU` | 他PQC・複数マイコン | 対象外 | 著者方針に従い本改訂では実施しない。一般性の主張にも用いない。 |

## 総合判定

ローカルで実行できる有限の工程はすべて完了しました。link済みK/S/Vのsoftware callと最大例外entryを含むPSP上界は成立しましたが、handler call・非同期割込み/MSP nestingを含む全プログラム最悪スタック上界とサイドチャネル耐性は成立していません。物理電力・電磁波実験は取得機材がないため実行していません。

機械可読な状態と各証拠のSHA-256は `future-work-status.json` に保存します。
