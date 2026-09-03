# IACR ePrint向け日本語論文ドラフト

`main.tex` は、SQIsign v2 Level Iの静的低メモリ実装と、SQIsign v3 `p324_3`へのlifetime scheduling転用を題材にした、IACR Cryptology ePrint Archive向けの日本語プレプリント原稿です。ePrintの公式投稿フォームは専用LaTeX classを指定しておらず、A4またはUS LetterのPDFを受け付けます。本原稿はA4、単一カラム、LuaLaTeXで構成しています。

ePrintは非匿名投稿です。公開用PDFの第一頁には、タイトル、全著者の実名、所属または連絡先、およびメールアドレスが必要です。現稿には、著者名、所属、メールアドレスを反映しています。

## ビルド

TeX LiveとBiberが入った環境で次を実行します。

```sh
cd manuscript
make
```

または直接、次を実行します。

```sh
latexmk -lualatex -interaction=nonstopmode -halt-on-error main.tex
```

生成物は `main.pdf` です。補助ファイルとPDFはこのディレクトリの `.gitignore` で除外しています。

提出直前には次を実行します。

```sh
make eprint-check
```

これは著者情報、メタデータ、図版などのプレースホルダ、PDF用紙サイズ、第一頁のメールアドレス、機械生成した証拠表の整合を検査します。

## ファイル

- `main.tex`: ePrint向けA4日本語本文、数式、表、TikZ図、GitHub補助資料へのリンク
- `figures/overview.tikz.tex`: 図1の研究全体図
- `references.bib`: 現時点で本文から参照する一次文献
- `eprint-metadata.txt`: ePrint投稿フォームへ転記するplain-text metadata
- `EPRINT_CHECKLIST.md`: 公式要件と本稿固有の提出前確認事項
- `Makefile`: LuaLaTeX/Biberによるビルド、提出前check、clean

図は `manuscript/figures/` またはrepository直下の `figures/` に置けます。現稿の図は全て `manuscript/figures/` 内の白黒TikZ sourceから生成します。図1はv2提案実装とv3適応実装を別分岐として表示します。

## 数値の一次成果物

本文の主要値は次に対応します。

- v2提案実装のarenaとreplay: `../experiments/memory/README.md`
- v2提案実装の公式request 100-vector差分適合試験: `../results/host/d13-lowmem-kat-2026-09-03/manifest.json`
- 実機manifest: `../results/rp2350/ksv-d13-dc3289a-manifest.json`
- 実機artifact: `../results/rp2350/artifacts/ksv-d13-dc3289a/`
- v2同一UF2の2 boot比較: `../results/rp2350/ksv-d13-repeat-2026-09-04-summary.json`
- v3版分離manifest: `../results/v3/version-isolation-manifest.json`
- v3 clean-source host gate: `../results/v3/host/validation-clean-2026-09-04.txt`
- v3 clean-commit実機campaign: `../results/v3/rp2350/interleaved-clean-2026-09-04/`
- v3 10ベクトル・2配置campaign: `../results/v3/rp2350/multi-input-placement-clean-2026-09-04/`
- v3固定frameのlink済みPSP上界: `../results/v3/analysis/d2-linked-stack-bound-audit-2026-09-04.json`
- v3固定frameの実機cross-check: `../results/v3/rp2350/static-closure-clean-2026-09-04/`
- v3固定鍵RP2350 timing screen: `../results/v3/rp2350/fixed-key-timing-clean-2026-09-04/`
- v2 `find_uv`実装対応証拠: `../results/revision-2026-09-04/finduv-equivalence-certificate.json`
- v2 lifetime certificate: `../results/revision-2026-09-04/lifetime-certificate-v2.csv`
- 追加研究工程の機械可読台帳: `../results/revision-2026-09-04/future-work-status.json`
- v3公開後の文献検索ログ: `../results/literature/related-work-search-2026-09-04.md`
- v3 lifetime分析: `../results/v3/analysis/d1-lifetime-overlay-2026-09-03.md`
- 全checkpointとhost測定: `../BENCHMARKS.md`
- 文献比較とclaim境界: `../RELATED_WORK.md`
- サイドチャネル評価: `../experiments/sca/README.md`

数値を手で更新した場合は、上記の機械可読JSON/CSVと矛盾していないことを確認してください。

## ePrint用の原稿モード

草稿用macroと非表示の謝辞placeholderは本文から除去済みです。詳細な再現性情報は本文の付録ではなく、`https://github.com/quantumshiro/tinysqisign`の補助資料へ置きます。

投稿フォームのabstractへPDFからcopy-and-pasteすると文字化けすることがあるため、`eprint-metadata.txt`のplain textを使います。著者順、表記、タイトル、abstractはPDFと投稿フォームで一致させてください。

## 投稿前に残る作業

ローカルで実行できる有限の追加実験、証拠再生成、v3公開後の文献監査は完了しています。研究上の未解決事項と投稿時の著者判断を区別します。

1. PDFとePrint投稿フォームで、著者名、所属、メールアドレスの表記を一致させる。
2. 撤回不能なライセンス選択と、第三者成果物を公開する権利を著者が最終確認する。
3. PDFを別環境で目視し、文字化け、欠落フォント、切れた表、壊れたリンクがないことを確認する。
4. 匿名または未認証環境から公開GitHub URLと再現手順を確認する。
5. 投稿直前に `make eprint-check` を再実行する。

物理電力・EM取得は、取得firmware・解析器・positive controlまでは準備済みですが、電流/EM probeとscope/SCA装置が現環境にないため0 traceです。全program最悪stack上界とサイドチャネル耐性も成立していません。他PQC方式と複数マイコンは著者方針により本改訂の対象外です。これらを提出済み工程として装わず、`future-work-status.json`に状態を固定しています。

現稿は、v2の低メモリ実現可能性、exact replayの正しさ、v3へのlifetime scheduling転用、およびRP2350での実機成立を中心にしています。v2のサイドチャネル結果は重要なnegative resultとして含めていますが、v2にもv3にも耐性達成を主張していません。

投稿フォームはUTF-8文字を扱えますが、ePrintの国際的な読者と後続のconference/journal投稿を考えると、研究内容が固まった段階で英語版へ移行するのが望ましいです。初稿では日本語のまま論理と証拠を固めます。
