.PHONY: all evidence pdf eprint-check clean

TEXMFVAR ?= $(CURDIR)/.texlive-var
TEXMFCACHE ?= $(TEXMFVAR)
export TEXMFVAR TEXMFCACHE

all: pdf

evidence:
	cd .. && python3 scripts/generate_manuscript_evidence.py
	cd .. && python3 scripts/generate_future_work_status.py

pdf: evidence
	mkdir -p "$(TEXMFVAR)"
	latexmk -lualatex -interaction=nonstopmode -halt-on-error main.tex

# 提出直前用。生成済み証拠とPDFの双方を検査する。
eprint-check: pdf
	cd .. && python3 scripts/generate_future_work_status.py --require-v2-repeat --require-local-complete
	@if grep -En '著者氏名（要入力）|所属（要入力）|author@example\.org|github\.com/USERNAME/REPOSITORY|^\\placeholderfigure|^\\todo\{' main.tex; then \
		echo 'ERROR: main.texに提出前プレースホルダが残っています。' >&2; \
		exit 1; \
	fi
	@if grep -En '要入力|要選択' eprint-metadata.txt; then \
		echo 'ERROR: eprint-metadata.txtに未確定項目が残っています。' >&2; \
		exit 1; \
	fi
	@if command -v pdfinfo >/dev/null 2>&1; then \
		pdfinfo main.pdf | grep -Eq '\(A4\)' || { echo 'ERROR: main.pdfがA4ではありません。' >&2; exit 1; }; \
	fi
	@if command -v pdftotext >/dev/null 2>&1; then \
		pdftotext -f 1 -l 1 main.pdf - | grep -q '@' || { echo 'ERROR: 第一頁にメールアドレスが見つかりません。' >&2; exit 1; }; \
	fi
	@echo 'ePrint向け機械チェック: PASS'

clean:
	latexmk -C main.tex
