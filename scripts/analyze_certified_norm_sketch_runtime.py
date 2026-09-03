#!/usr/bin/env python3
"""Validate and summarize the D12c/D13 paired host timing smoke."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import statistics
from pathlib import Path


EXPECTED_COLUMNS = [
    "operation",
    "run",
    "round",
    "seed",
    "order",
    "execution_sequence",
    "d12c_explicit_ns",
    "d13_explicit_ns",
]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(65536), b""):
            digest.update(block)
    return digest.hexdigest()


def summarize(rows: list[dict[str, str]], operation: str, run: int) -> dict:
    selected = [
        row
        for row in rows
        if row["operation"] == operation and int(row["run"]) == run
    ]
    if len(selected) != 30:
        raise ValueError(f"{operation} run {run}: expected 30 rows")
    selected.sort(key=lambda row: int(row["round"]))

    expected_sequence = "d12c-then-d13" if run == 1 else "d13-then-d12c"
    for round_index, row in enumerate(selected):
        if int(row["round"]) != round_index:
            raise ValueError(f"{operation} run {run}: non-contiguous rounds")
        if row["execution_sequence"] != expected_sequence:
            raise ValueError(f"{operation} run {run}: wrong execution sequence")
        expected_order = "legacy-first" if round_index % 2 == 0 else "explicit-first"
        if row["order"] != expected_order:
            raise ValueError(f"{operation} run {run}: wrong AB/BA order")
        if int(selected[round_index - (round_index % 2)]["seed"]) != int(row["seed"]):
            raise ValueError(f"{operation} run {run}: seed pair mismatch")

    base = [int(row["d12c_explicit_ns"]) for row in selected]
    candidate = [int(row["d13_explicit_ns"]) for row in selected]
    differences = [new - old for old, new in zip(base, candidate, strict=True)]
    base_median = statistics.median(base)
    candidate_median = statistics.median(candidate)

    return {
        "operation": operation,
        "run": run,
        "execution_sequence": expected_sequence,
        "rows": len(selected),
        "unique_seeds": len({int(row["seed"]) for row in selected}),
        "d12c_median_ns": base_median,
        "d13_median_ns": candidate_median,
        "ratio_of_medians": candidate_median / base_median,
        "paired_difference_mean_ns": statistics.mean(differences),
        "paired_difference_median_ns": statistics.median(differences),
        "paired_difference_sample_sd_ns": statistics.stdev(differences),
        "d13_faster_rows": sum(value < 0 for value in differences),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("csv", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    with args.csv.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames != EXPECTED_COLUMNS:
            raise ValueError("unexpected CSV schema")
        rows = list(reader)

    if len(rows) != 120:
        raise ValueError("expected exactly 120 normalized rows")
    report = {
        "schema": "sqisign-certified-norm-sketch-runtime-v1",
        "input": str(args.csv),
        "input_sha256": sha256(args.csv),
        "scope": (
            "Two reversed-sequence 30-row host smoke runs per operation; "
            "not an equivalence test, target timing result, or D13-only cost attribution."
        ),
        "results": [
            summarize(rows, operation, run)
            for operation in ("keygen", "sign")
            for run in (1, 2)
        ],
    }
    encoded = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.output is None:
        print(encoded, end="")
    else:
        args.output.write_text(encoded, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
