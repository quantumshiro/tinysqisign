# IACR ePrint向け日英論文

`main.tex` は日本語版、`main-en.tex` は国際査読向け英語版です。いずれもSQIsign v2 Level Iの静的低メモリ実装と、SQIsign v3の二つの関数・全パラメータ集合への生存期間配置の転用を扱うIACR Cryptology ePrint Archive向けプレプリントです。ePrintの公式投稿フォームは専用のLaTeX文書クラスを指定しておらず、A4またはUS LetterのPDFを受け付けます。両原稿はA4、単一カラム、LuaLaTeXで構成します。

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
latexmk -lualatex -interaction=nonstopmode -halt-on-error main-en.tex
```

生成物は `main.pdf` と `main-en.pdf` です。LaTeX補助ファイルはこのディレクトリの `.gitignore` で除外し、二つのPDFは再現確認用としてリポジトリへ収録します。

提出直前には次を実行します。

```sh
make eprint-check
```

これは著者情報、メタデータ、図版などのプレースホルダ、PDF用紙サイズ、第一頁のメールアドレス、機械生成した証拠表の整合を検査します。

## ファイル

- `main.tex`: ePrint向けA4日本語本文
- `main-en.tex`: 国際査読向けA4英語本文
- `figures/`: 日本語本文用の白黒TikZ図
- `figures-en/`: 英語本文用の白黒TikZ図
- `references.bib`: 本文から参照する文献
- `eprint-metadata.txt`: ePrint投稿フォームへ転記するプレーンテキストのメタデータ
- `EPRINT_CHECKLIST.md`: 公式要件と本稿固有の提出前確認事項
- `Makefile`: LuaLaTeX/Biberによる構築、提出前検査、補助ファイル削除

現稿の図はすべて `manuscript/figures/` と `manuscript/figures-en/` 内の白黒TikZソースから生成します。図1はv2提案実装とv3適応実装を別分岐として表示します。

## 本文数値の根拠

本文の主要値は次に対応します。

- v2提案実装の操作用共有領域と全精度値の再計算: `../experiments/memory/README.md`
- v2提案実装の公式入力100本による差分適合試験: `../results/host/d13-lowmem-kat-2026-09-03/manifest.json`
- 実機測定の構成記録: `../results/rp2350/ksv-d13-dc3289a-manifest.json`
- 実機バイナリ: `../results/rp2350/artifacts/ksv-d13-dc3289a/`
- v2同一UF2の2回起動比較: `../results/rp2350/ksv-d13-repeat-2026-09-04-summary.json`
- v2のリンク後に到達可能な操作用PSP上界: `../results/v2/analysis/linked-stack-bound-2026-09-04.json`
- v3の版分離記録: `../results/v3/version-isolation-manifest.json`
- v3の版を固定したホスト検証: `../results/v3/host/validation-clean-2026-09-04.txt`
- v3一関数適応版の対比較実機測定: `../results/v3/rp2350/interleaved-clean-2026-09-04/`
- v3二関数適応版の実機測定: `../results/v3/rp2350/d3-two-function-clean-2026-09-04/`
- v3全パラメータ集合のホスト・Arm検証: `../results/v3/host/validation-all-params-2026-09-04.json`、`../results/v3/analysis/lifetime-all-params-2026-09-04.json`
- v3一関数適応版の10ベクトル・2配置試験: `../results/v3/rp2350/multi-input-placement-clean-2026-09-04/`
- v3固定長配列版のリンク後PSP上界: `../results/v3/analysis/d2-linked-stack-bound-audit-2026-09-04.json`
- v3固定長配列版の実機測定: `../results/v3/rp2350/static-closure-clean-2026-09-04/`
- v3固定鍵RP2350実行時間検査: `../results/v3/rp2350/fixed-key-timing-clean-2026-09-04/`
- v2 `find_uv`実装対応証拠: `../results/revision-2026-09-04/finduv-equivalence-certificate.json`
- v2生存期間確認表: `../results/revision-2026-09-04/lifetime-certificate-v2.csv`
- 生存期間注釈の網羅性検査: `../results/revision-2026-09-04/lifetime-annotation-coverage.json`
- v3公開後の文献検索ログ: `../results/literature/related-work-search-2026-09-04.md`
- v3生存期間分析: `../results/v3/analysis/d1-lifetime-overlay-2026-09-03.md`
- 全測定記録: `../BENCHMARKS.md`
- 文献比較と評価範囲: `../RELATED_WORK.md`
- サイドチャネル評価: `../experiments/sca/README.md`

英語版は日本語版の全文翻訳です。8節・15小節、全6図・18表・4アルゴリズムを同じ順序・番号で対応させ、本文の段落、数式、引用、条件と限界を省略しません。言語による組版の違いがあるため、ページ番号は一致しません。

日英二版の主要数値は同じ生成済みマクロを参照します。表行の英訳も日本語の全行から生成します。`eprint-check`は両PDFの構築に加え、`../scripts/check_manuscript_translation.py`で段落ブロック・見出し・図表・アルゴリズム・数式・引用・数値の対応を検査します。この機械検査は構造の一致を確認するもので、翻訳の意味の正しさを証明するものではありません。
