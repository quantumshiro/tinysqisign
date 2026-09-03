#!/usr/bin/env python3
"""Descriptive reproducibility screen for the v3 ten-vector RP2350 timings."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import statistics
from pathlib import Path


OPERATIONS = ("keygen", "sign", "verify", "negative")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def pearson(left: list[float], right: list[float]) -> float:
    left_mean = statistics.mean(left)
    right_mean = statistics.mean(right)
    numerator = sum((x - left_mean) * (y - right_mean) for x, y in zip(left, right))
    left_energy = sum((x - left_mean) ** 2 for x in left)
    right_energy = sum((y - right_mean) ** 2 for y in right)
    if left_energy == 0 or right_energy == 0:
        return math.nan
    return numerator / math.sqrt(left_energy * right_energy)


def ranks(values: list[float]) -> list[float]:
    ordered = sorted(enumerate(values), key=lambda pair: pair[1])
    result = [0.0] * len(values)
    cursor = 0
    while cursor < len(ordered):
        end = cursor + 1
        while end < len(ordered) and ordered[end][1] == ordered[cursor][1]:
            end += 1
        rank = (cursor + 1 + end) / 2
        for index in range(cursor, end):
            result[ordered[index][0]] = rank
        cursor = end
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("summary", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source = json.loads(args.summary.read_text(encoding="utf-8"))
    if source.get("schema") != "sqisign-v3-rp2350-multi-input-placement-v1":
        raise ValueError("unexpected campaign schema")
    if not source.get("all_trials_passed") or source.get("placement_psp_mismatch_count") != 0:
        raise ValueError("input campaign did not pass its correctness/memory gates")

    records = source["records"]
    report: dict[str, object] = {}
    for kind in ("baseline", "d1"):
        left = {int(row["vector"]): row for row in records[f"{kind}-a"]["vectors"]}
        right = {int(row["vector"]): row for row in records[f"{kind}-b"]["vectors"]}
        kind_report: dict[str, object] = {}
        for operation in OPERATIONS:
            a = [float(left[index]["operations"][operation]["time_us"]) for index in range(10)]
            b = [float(right[index]["operations"][operation]["time_us"]) for index in range(10)]
            pooled = a + b
            relative_pair_differences = [100.0 * (y - x) / x for x, y in zip(a, b)]
            kind_report[operation] = {
                "vectors": 10,
                "placement_a_vs_b_pearson": pearson(a, b),
                "placement_a_vs_b_spearman": pearson(ranks(a), ranks(b)),
                "placement_pair_delta_percent": {
                    "min": min(relative_pair_differences),
                    "median": statistics.median(relative_pair_differences),
                    "max": max(relative_pair_differences),
                },
                "pooled_time_us": {
                    "min": min(pooled),
                    "median": statistics.median(pooled),
                    "max": max(pooled),
                    "max_over_min": max(pooled) / min(pooled),
                    "coefficient_of_variation": statistics.stdev(pooled) / statistics.mean(pooled),
                },
            }
        report[kind] = kind_report

    # The large Sign ordering is reproducible across the two linked placements.
    for kind in ("baseline", "d1"):
        sign = report[kind]["sign"]
        if sign["placement_a_vs_b_pearson"] < 0.999:
            raise ValueError(f"{kind}: Sign input timing ordering was not reproducible")
        if sign["pooled_time_us"]["max_over_min"] < 1.4:
            raise ValueError(f"{kind}: expected broad KAT-input Sign timing range was absent")

    output = {
        "schema": "sqisign-v3-rp2350-ten-vector-timing-screen-v1",
        "status": "POSITIVE_INPUT_DEPENDENCE",
        "source": {
            "path": str(args.summary),
            "sha256": sha256(args.summary),
        },
        "design": {
            "board_count": 1,
            "vectors": 10,
            "repetitions_per_implementation_and_vector": 2,
            "repetition_mechanism": "two linked placements separated by 1024 code bytes",
            "clock_hz": 150000000,
        },
        "results": report,
        "decision": {
            "repeatable_input_associated_sign_timing_observed": True,
            "fixed_key_leakage_established": False,
            "power_or_em_leakage_established": False,
            "key_recovery_established": False,
            "side_channel_resistance_established": False,
            "production_signing_approved": False,
        },
        "claim_boundary": (
            "The official KAT vectors change seed, key pair, and message together.  "
            "The screen therefore shows reproducible input-associated timing structure "
            "on one board, not fixed-key leakage attribution, power/EM leakage, or an attack."
        ),
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(output, indent=2, sort_keys=True) + "\n")
    for kind in ("baseline", "d1"):
        sign = report[kind]["sign"]
        print(
            f"{kind}_sign_pearson={sign['placement_a_vs_b_pearson']:.9f} "
            f"spearman={sign['placement_a_vs_b_spearman']:.9f} "
            f"max_over_min={sign['pooled_time_us']['max_over_min']:.6f}"
        )
    print("status=POSITIVE_INPUT_DEPENDENCE fixed_key_leakage=false resistance=false")
    print(f"output={args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
