.PHONY: all pdf eprint-check clean

TEXMFVAR ?= $(CURDIR)/.texlive-var
TEXMFCACHE ?= $(TEXMFVAR)
export TEXMFVAR TEXMFCACHE

all: pdf

pdf:
	mkdir -p "$(TEXMFVAR)"
	latexmk -lualatex -interaction=nonstopmode -halt-on-error main.tex

# 提出直前用の機械チェック。
eprint-check: pdf
	@if grep -En '著者氏名（要入力）|所属（要入力）|author@example\.org|github\.com/USERNAME/REPOSITORY|^\\placeholderfigure|^\\todo\{' main.tex; then \
		echo 'ERROR: main.texに提出前プレースホルダが残っています。' >&2; \
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
