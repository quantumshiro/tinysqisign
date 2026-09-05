#!/usr/bin/env python3
"""Check structural JA/EN parity, not semantic translation correctness."""
import argparse
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PAPER = ROOT / "manuscript"
CJK = re.compile(r"[\u3000-\u9fff]")

def uncomment(text):
    return re.sub(r"(?<!\\)%[^\n]*", "", text)

def body(text):
    return text.split(r"\begin{abstract}", 1)[1].split(r"\printbibliography", 1)[0]

def normalized_input(name):
    return name.replace("figures-en/", "figures/").replace("-rows-en.tex", "-rows.tex")

def signature(text):
    text = uncomment(text)
    return {
        "structure": re.findall(r"\\(?:begin|end)\{[^}]+\}|\\(?:subsection|section)\*?|\\item\b|\\caption\b", text),
        "labels": re.findall(r"\\label\{([^}]+)\}", text),
        "references": re.findall(r"\\(?:[cC]ref|ref|cite)\{([^}]+)\}", text),
        "inputs": [normalized_input(x) for x in re.findall(r"\\input\{([^}]+)\}", text)],
        "numbers": sorted(re.findall(r"\\num\{(?:\\[A-Za-z]+|[^{}]+)\}", text)),
        "inline_math": sorted(re.sub(r"\s+", "", x) for x in re.findall(r"(?<!\\)\$(.*?)(?<!\\)\$", text.replace(r"\\", " "), re.S)),
        "urls": re.findall(r"\\url\{([^}]+)\}", text),
        "display_math": [re.sub(r"\s+", "", x) for x in re.findall(
            r"\\begin\{(?:equation|align)\}(.*?)\\end\{(?:equation|align)\}", text, re.S)],
    }

def compare(ja, en, name):
    left, right = signature(ja), signature(en)
    for key in left:
        if left[key] != right[key]:
            raise ValueError(f"{name}: {key} mismatch")
    if CJK.search(uncomment(en)):
        raise ValueError(f"{name}: untranslated Japanese")
    return left

def check():
    ja = (PAPER / "main.tex").read_text()
    en = (PAPER / "main-en.tex").read_text()
    jb, eb = body(ja), body(en)
    sig = compare(jb, eb, "manuscript")
    # Blank-line blocks preserve the paragraph/list/math/float correspondence.
    jblocks = re.split(r"\n\s*\n", jb.strip())
    eblocks = re.split(r"\n\s*\n", eb.strip())
    if len(jblocks) != len(eblocks):
        raise ValueError("paragraph/block count mismatch")
    alignment = []
    for index, (j, e) in enumerate(zip(jblocks, eblocks), 1):
        compare(j, e, f"block {index}")
        alignment.append({"block": index,
                          "ja_line": ja[:ja.index(j)].count("\n") + 1,
                          "en_line": en[:en.index(e)].count("\n") + 1})
    assets = []
    for jname, ename in zip(re.findall(r"\\input\{([^}]+)\}", jb),
                           re.findall(r"\\input\{([^}]+)\}", eb)):
        j = (PAPER / jname).read_text()
        e = (PAPER / ename).read_text()
        compare(j, e, jname)
        # Every table cell and row must remain, including non-numeric cells.
        if "rows" in jname:
            rows = lambda s: [len(x.split("&")) for x in uncomment(s).splitlines() if x.strip()]
            if rows(j) != rows(e):
                raise ValueError(f"{jname}: table row/cell mismatch")
        assets.append({"ja": jname, "en": ename})
    return {
        "scope": "Structural correspondence only; semantic translation requires reading both texts.",
        "status": "PASS",
        "sha256": {name: hashlib.sha256((PAPER / name).read_bytes()).hexdigest()
                   for name in ("main.tex", "main-en.tex")},
        "counts": {
            "sections": len(re.findall(r"\\section\{", jb)),
            "subsections": len(re.findall(r"\\subsection\{", jb)),
            "figures": jb.count(r"\begin{figure}"),
            "tables": jb.count(r"\begin{table}"),
            "algorithms": jb.count(r"\begin{algorithm}"),
            "aligned_blocks": len(alignment),
            "labels": len(sig["labels"]),
        },
        "blocks": alignment, "assets": assets,
    }

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    report = check()
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n")
    print("JA/EN structural parity: PASS", json.dumps(report["counts"]))

if __name__ == "__main__":
    main()
