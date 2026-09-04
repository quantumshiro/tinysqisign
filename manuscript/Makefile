.PHONY: all evidence pdf eprint-check clean

TEXMFVAR ?= $(CURDIR)/.texlive-var
TEXMFCACHE ?= $(TEXMFVAR)
export TEXMFVAR TEXMFCACHE

all: pdf

evidence:
	cd .. && python3 scripts/generate_manuscript_evidence.py
	cd .. && python3 scripts/generate_future_work_status.py

pdf: evidence main.pdf main-en.pdf

main.pdf: main.tex references.bib
	mkdir -p "$(TEXMFVAR)"
	latexmk -lualatex -interaction=nonstopmode -halt-on-error main.tex

main-en.pdf: main-en.tex references.bib
	mkdir -p "$(TEXMFVAR)"
	latexmk -lualatex -interaction=nonstopmode -halt-on-error main-en.tex

# 提出直前用。生成済み証拠とPDFの双方を検査する。
eprint-check: pdf
	cd .. && python3 scripts/generate_future_work_status.py --require-v2-repeat --require-local-complete
	@if grep -En '著者氏名（要入力）|所属（要入力）|author@example\.org|github\.com/USERNAME/REPOSITORY|^\\placeholderfigure|^\\todo\{' main.tex main-en.tex; then \
		echo 'ERROR: 原稿に提出前プレースホルダが残っています。' >&2; \
		exit 1; \
	fi
	@if grep -Ein 'exact|厳密' main.tex main-en.tex figures/*.tikz.tex figures-en/*.tikz.tex; then \
		echo 'ERROR: 避けることにした表現が原稿または図に残っています。' >&2; \
		exit 1; \
	fi
	@if grep -En '要入力|要選択' eprint-metadata.txt; then \
		echo 'ERROR: eprint-metadata.txtに未確定項目が残っています。' >&2; \
		exit 1; \
	fi
	@if command -v pdfinfo >/dev/null 2>&1; then \
		for pdf in main.pdf main-en.pdf; do \
			pdfinfo "$$pdf" | grep -Eq '\(A4\)' || { echo "ERROR: $$pdfがA4ではありません。" >&2; exit 1; }; \
		done; \
	fi
	@if command -v pdftotext >/dev/null 2>&1; then \
		for pdf in main.pdf main-en.pdf; do \
			pdftotext -f 1 -l 1 "$$pdf" - | grep -q '@' || { echo "ERROR: $$pdfの第一頁にメールアドレスが見つかりません。" >&2; exit 1; }; \
		done; \
	fi
	@echo 'ePrint向け日英機械チェック: PASS'

clean:
	latexmk -C main.tex
	latexmk -C main-en.tex
