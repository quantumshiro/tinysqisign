# TinySQIsign

TinySQIsignは、SQIsignの同時生存メモリを削減する研究の最小公開成果物です。
論文原稿、実装を再構成するパッチ、再現用プログラム、および論文中の主要値に対応する証拠を収録しています。巨大なビルドディレクトリ、第三者論文、外部リポジトリの複製、および途中段階のログは収録していません。

著者は独立研究者のHiro Nakanishiです。連絡先は`quantumsity@protonmail.com`です。

## 主な結果

- SQIsign v2 Level I/RADIX32では、操作アリーナを353,008 bytesから172,080 bytesへ削減しました。削減量は180,928 bytes、削減率は51.2532%です。
- RP2350ファームウェアが排他的に予約するSRAMは321,408 bytesです。オンチップSRAM 532,480 bytesのうち211,072 bytesを未予約として残しました。
- 既知解テスト（Known Answer Test、KAT）では、公式NIST-v2の100入力を使った低メモリ経路を独立に2回実行しました。KeyGen、Sign、Open、改変署名の拒否、アリーナ境界、および操作後の消去は、各回100件すべてで成功しました。
- SQIsign v3 `p324_3/m4f`への局所的な生存期間の重畳では、SignのProcess Stack Pointer（PSP）上書き深さを101,060 bytesから97,132 bytesへ削減しました。同一基板上の5組の比較におけるSign時間の増加率中央値は0.4182%です。

## ディレクトリ

- `manuscript/`: 日本語論文のLaTeXソース、白黒TikZ図、および生成済みPDF
- `patches/`: v2、v3、および論文で扱う実験的サイドチャネル対策の再構成用差分
- `experiments/`: KAT、メモリ配置、およびmini-GMP比較に必要な小規模プログラム
- `src/`: RP2350でv2とv3を測るための最小ファームウェアハーネス
- `scripts/`: 外部ソースの準備、KAT、RP2350ビルド、およびELF監査
- `results/`: 論文の主要な表と主張に対応する凍結済みJSON、CSV、シリアル出力、およびKAT証拠

## 最短の再現手順

最初に、収録ファイルの完全性を検査します。

```sh
shasum -a 256 -c SHA256SUMS
```

外部ソースを取得し、凍結した実装を再構成します。

```sh
./scripts/prepare_sources.sh
```

v2の100入力KATを実行します。結果ディレクトリは既存であってはいけません。

```sh
./scripts/run_d13_lowmem_kat.sh ./results/local/d13-kat
```

論文PDFを再生成します。

```sh
make -C manuscript
```

必要な依存関係、RP2350のビルド方法、および証拠との対応は[REPRODUCIBILITY.md](REPRODUCIBILITY.md)に記載しています。

## 主張の範囲

本成果物は、低メモリ化によってサイドチャネル耐性を達成したとは主張しません。v2の署名処理では時間分布、制御フロー、および実効アドレスに入力依存性を観測しています。RP2350のv2測定は、決定的乱数を使った単一基板・単一入力・一回のKeyGen/Sign/Verifyです。製品用乱数、最悪スタック量、複数基板の性能分布、および電力・電磁波による鍵回復耐性は未評価です。

## ライセンス

SQIsign由来コードの条件と帰属表示は`LICENSES/`に収録しています。DPE部分にはGNU Lesser General Public License version 3が適用されます。論文原稿の再利用条件は現時点で別途指定していません。
