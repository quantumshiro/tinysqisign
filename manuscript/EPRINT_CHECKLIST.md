# IACR ePrint 提出チェックリスト

2026-09-04に公式の[submission form](https://eprint.iacr.org/submit)と[conditions of use](https://eprint.iacr.org/operations.html)を再確認した。投稿直前にも再確認すること。

## PDF

- [x] PDFはISO A4またはUS Letterである（本原稿はA4）。
- [x] 第一頁に論文タイトル、全著者の実名、所属または連絡先がある。
- [x] PDF本文に著者のメールアドレスがある。
- [x] 匿名化していない。ePrintは非匿名投稿である。
- [ ] PDFを別環境で開き、文字化け、欠落フォント、切れた表、壊れたリンクがない。
- [x] 草稿メモ、著者プレースホルダ、図版プレースホルダが残っていない。

## 内容と権利

- [ ] cryptologyへのtechnical contributionであり、明瞭、可読、self-containedである。
- [ ] 新規性と関心を持つ理由を説明し、主張には証明または再現可能な証拠がある。
- [ ] 全著者が著者表記、内容、ePrint公開、選択ライセンスに同意している。
- [ ] 第三者の文章、図、コード、データを公開する権利を確認した。
- [ ] ePrint上では改訂・withdraw後も過去版がアクセス可能であることを全著者が理解した。

## 投稿フォーム

- [ ] `eprint-metadata.txt`のタイトルをUTF-8のplain textとして入力した。
- [ ] 全著者をPDFと同じ順序で入力した。少なくとも一名、できれば全員にメールを設定した。
- [ ] abstractは64文字以上で、HTML、文書用LaTeX命令、citation命令を含まない。
- [ ] categoryを選択した。本稿の第一候補は`Implementation`である。
- [ ] keywordsはcomma区切り、各40文字以内、全体120文字以内で、LaTeX encodingを含まない。
- [ ] 既発表または掲載予定ならpublication informationを入力した。
- [ ] public notesとeditor-only messageを取り違えていない。
- [ ] ライセンスを選択し、その非独占かつ撤回不能な配布許諾を確認した。

## 現稿で投稿前に解消する項目

- [x] `main.tex`の著者氏名、所属、メールアドレスを実値へ置換する。
- [x] `hypersetup`の`pdfauthor`を実著者名へ更新する。
- [x] 本文中の図を白黒TikZ sourceへ置換した。
- [x] 現時点で謝辞は設けず、非表示placeholderも除去した。
- [x] v2提案実装専用の公式request 100-vector差分適合試験を二回完走し、専用ログ・応答・binary/configuration hashを凍結して本文のclaim boundaryを更新する。
- [x] v2/v3の実機測定に関する主張を、v2は同一入力2 boot、v3はclean commitの単一ベクトル5組・10ベクトル2配置・固定鍵200署名・固定frame cross-checkという最終証拠へ合わせる。
- [x] artifact URLに関する主張を公開先へ合わせる。
- [x] `https://github.com/quantumshiro/tinysqisign`を公開し、到達可能であることを確認する。
- [x] v3 test harnessをclean commitへ凍結し、ELF、UF2、map、archive、stack-usage、capture、patchのhashを再取得する。
- [x] v2のarenaとv3のstack watermarkを同一のRAM量として比較していない。
- [x] 初版は日本語版として論理と証拠を固定する。
- [x] v2の実装同値性対応表とlifetime certificateを機械検査可能な成果物として凍結する。
- [x] v3のlink済みK/S/V software-call PSP上界へ最大例外entryを含め、実機観測が上界内であることを確認する。
- [x] 「今後の研究工程」のうちローカルで実行可能な有限工程を完了し、外部計測器待ち・著者判断の対象外・未成立の研究目標を区別して台帳化する。
- [x] RP2350固定鍵screenを事前固定設計で200署名完走し、鍵対応timingの検出と非耐性claim boundaryを本文へ反映する。
