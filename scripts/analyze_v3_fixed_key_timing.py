#!/usr/bin/env python3
"""Analyze the bounded SQIsign v3 fixed-key host timing screen."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import statistics
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def median_absolute_deviation(values: list[int]) -> float:
    center = statistics.median(values)
    return float(statistics.median(abs(value - center) for value in values))


def average_ranks(values: list[float]) -> list[float]:
    indexed = sorted(enumerate(values), key=lambda item: item[1])
    ranks = [0.0] * len(values)
    start = 0
    while start < len(indexed):
        stop = start + 1
        while stop < len(indexed) and indexed[stop][1] == indexed[start][1]:
            stop += 1
        rank = ((start + 1) + stop) / 2.0
        for position in range(start, stop):
            ranks[indexed[position][0]] = rank
        start = stop
    return ranks


def pearson(left: list[float], right: list[float]) -> float:
    if len(left) != len(right) or len(left) < 2:
        raise ValueError("correlation needs equal vectors of length >= 2")
    left_mean = statistics.mean(left)
    right_mean = statistics.mean(right)
    numerator = sum(
        (x - left_mean) * (y - right_mean) for x, y in zip(left, right)
    )
    left_norm = math.sqrt(sum((x - left_mean) ** 2 for x in left))
    right_norm = math.sqrt(sum((y - right_mean) ** 2 for y in right))
    if left_norm == 0 or right_norm == 0:
        return 0.0
    return numerator / (left_norm * right_norm)


def spearman(left: list[float], right: list[float]) -> float:
    return pearson(average_ranks(left), average_ranks(right))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--campaign-dir",
        type=Path,
        default=ROOT / "results/host/v3-fixed-key-timing-2026-09-04",
    )
    args = parser.parse_args()
    manifest_path = args.campaign_dir / "manifest.json"
    raw_path = args.campaign_dir / "raw.csv"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("schema") != "sqisign-v3-fixed-key-host-timing-campaign-v1":
        raise ValueError("unexpected campaign schema")
    if manifest["outputs"]["raw_csv_sha256"] != sha256(raw_path):
        raise ValueError("raw CSV digest mismatch")

    rows: list[dict[str, object]] = []
    with raw_path.open(newline="", encoding="utf-8") as handle:
        for raw in csv.DictReader(handle):
            rows.append(
                {
                    "implementation": raw["implementation"],
                    "pass": raw["pass"],
                    "sequence": int(raw["sequence"]),
                    "key_index": int(raw["key_index"]),
                    "elapsed_ns": int(raw["elapsed_ns"]),
                    "signed_message_length": int(raw["signed_message_length"]),
                    "signature_fnv1a64": raw["signature_fnv1a64"],
                    "sign_rc": int(raw["sign_rc"]),
                    "verify_rc": int(raw["verify_rc"]),
                }
            )

    design = manifest["design"]
    if design.get("fixed_key_buffer_address") is not True:
        raise ValueError("key-address confound was not removed")
    implementations = list(design["implementations"])
    passes = list(design["passes"])
    key_count = int(design["key_count"])
    repetitions = int(design["repetitions_per_key_per_pass"])
    expected_rows = len(implementations) * len(passes) * key_count * repetitions
    if len(rows) != expected_rows:
        raise ValueError(f"expected {expected_rows} rows, found {len(rows)}")
    if any(row["sign_rc"] != 0 or row["verify_rc"] != 0 for row in rows):
        raise ValueError("Sign or Verify failure in timing corpus")
    if any(row["signed_message_length"] != 233 for row in rows):
        raise ValueError("unexpected signed-message length")

    grouped: dict[tuple[str, str, int], list[int]] = defaultdict(list)
    digests: dict[tuple[str, int], set[str]] = defaultdict(set)
    schedule_counts: dict[tuple[str, str], dict[int, int]] = defaultdict(
        lambda: defaultdict(int)
    )
    for row in rows:
        implementation = str(row["implementation"])
        pass_label = str(row["pass"])
        key = int(row["key_index"])
        if implementation not in implementations or pass_label not in passes:
            raise ValueError("unknown implementation/pass label")
        if not 0 <= key < key_count:
            raise ValueError("key index outside campaign design")
        grouped[(implementation, pass_label, key)].append(int(row["elapsed_ns"]))
        digests[(implementation, key)].add(str(row["signature_fnv1a64"]))
        schedule_counts[(implementation, pass_label)][key] += 1

    if any(len(values) != repetitions for values in grouped.values()):
        raise ValueError("unbalanced implementation/pass/key cell")
    if len(grouped) != len(implementations) * len(passes) * key_count:
        raise ValueError("missing implementation/pass/key cell")
    if any(len(values) != 1 for values in digests.values()):
        raise ValueError("fixed RNG/message did not produce a stable signature per key")
    for key in range(key_count):
        reference = digests[(implementations[0], key)]
        if any(digests[(implementation, key)] != reference for implementation in implementations):
            raise ValueError(f"implementation signature mismatch for key {key}")

    per_key_rows: list[dict[str, object]] = []
    for implementation in implementations:
        for pass_label in passes:
            for key in range(key_count):
                values = grouped[(implementation, pass_label, key)]
                per_key_rows.append(
                    {
                        "implementation": implementation,
                        "pass": pass_label,
                        "key_index": key,
                        "samples": len(values),
                        "median_ns": int(statistics.median(values)),
                        "mean_ns": statistics.mean(values),
                        "stdev_ns": statistics.stdev(values),
                        "mad_ns": median_absolute_deviation(values),
                        "min_ns": min(values),
                        "max_ns": max(values),
                        "signature_fnv1a64": next(iter(digests[(implementation, key)])),
                    }
                )

    by_cell = {
        (str(row["implementation"]), str(row["pass"]), int(row["key_index"])): row
        for row in per_key_rows
    }
    implementation_reports: dict[str, object] = {}
    detections: list[bool] = []
    for implementation in implementations:
        pass_reports: dict[str, object] = {}
        median_vectors: dict[str, list[float]] = {}
        for pass_label in passes:
            medians = [
                float(by_cell[(implementation, pass_label, key)]["median_ns"])
                for key in range(key_count)
            ]
            mads = [
                float(by_cell[(implementation, pass_label, key)]["mad_ns"])
                for key in range(key_count)
            ]
            median_vectors[pass_label] = medians
            span = max(medians) - min(medians)
            central_median = statistics.median(medians)
            within_mad = statistics.median(mads)
            pass_reports[pass_label] = {
                "key_median_min_ns": int(min(medians)),
                "key_median_max_ns": int(max(medians)),
                "key_median_span_ns": int(span),
                "key_median_span_fraction": span / central_median,
                "median_within_key_mad_ns": within_mad,
                "span_to_within_key_mad_ratio": span / max(within_mad, 1.0),
                "fastest_key": medians.index(min(medians)),
                "slowest_key": medians.index(max(medians)),
            }
        correlation = spearman(median_vectors[passes[0]], median_vectors[passes[1]])
        detected = (
            correlation >= 0.8
            and all(
                float(pass_reports[label]["key_median_span_fraction"]) >= 0.01
                and float(pass_reports[label]["span_to_within_key_mad_ratio"]) >= 10.0
                for label in passes
            )
        )
        detections.append(detected)
        implementation_reports[implementation] = {
            "passes": pass_reports,
            "between_pass_key_median_spearman": correlation,
            "predeclared_screen_rule": (
                "Spearman >= 0.8 in the two per-key median rankings, and in both passes "
                "between-key span >= 1% of the central median and >= 10x median within-key MAD"
            ),
            "fixed_key_associated_timing_detected": detected,
        }

    d1_ratios = []
    if set(implementations) == {"official", "d1"}:
        for pass_label in passes:
            for key in range(key_count):
                official = float(by_cell[("official", pass_label, key)]["median_ns"])
                d1 = float(by_cell[("d1", pass_label, key)]["median_ns"])
                d1_ratios.append(d1 / official)

    per_key_path = args.campaign_dir / "per-key.csv"
    with per_key_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(per_key_rows[0]))
        writer.writeheader()
        writer.writerows(per_key_rows)

    summary = {
        "schema": "sqisign-v3-fixed-key-host-timing-analysis-v1",
        "status": "PASS",
        "campaign_manifest": {
            "path": str(manifest_path.relative_to(ROOT)),
            "sha256": sha256(manifest_path),
        },
        "raw_csv": {
            "path": str(raw_path.relative_to(ROOT)),
            "sha256": sha256(raw_path),
            "rows": len(rows),
        },
        "design": design,
        "validation": {
            "all_signatures_verified": True,
            "balanced_cells": True,
            "stable_signature_per_key_under_fixed_rng_and_message": True,
            "official_and_d1_signatures_identical": True,
            "key_address_confound_removed": True,
        },
        "implementations": implementation_reports,
        "d1_over_official_descriptive_ratio": {
            "median": statistics.median(d1_ratios),
            "minimum": min(d1_ratios),
            "maximum": max(d1_ratios),
            "warning": "conditions were run sequentially on a non-isolated host",
        },
        "decision": {
            "repeatable_fixed_key_associated_host_timing_observed": all(detections),
            "rp2350_or_m4f_timing_tested": False,
            "secret_dependent_control_or_address_trace_established": False,
            "physical_leakage_established": False,
            "key_recovery_established": False,
            "side_channel_resistance_established": False,
        },
        "claim_boundary": (
            "Ten official p324_3 keys copied into one fixed-address buffer, one fixed public "
            "message, one fixed signing RNG stream, "
            "two randomized schedule passes, and native Darwin/arm64 reference code.  This "
            "screen can expose repeatable key-associated host time but cannot certify an "
            "RP2350 trace, physical leakage, exploitability, or resistance."
        ),
        "outputs": {
            "per_key_csv": str(per_key_path.relative_to(ROOT)),
            "per_key_csv_sha256": sha256(per_key_path),
        },
    }
    summary_path = args.campaign_dir / "summary.json"
    summary_path.write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(
        "v3 fixed-key timing analysis: PASS "
        f"rows={len(rows)} repeatable_key_association={all(detections)} "
        "resistance=false"
    )
    for implementation in implementations:
        report = implementation_reports[implementation]
        print(
            f"{implementation}: spearman={report['between_pass_key_median_spearman']:.6f} "
            f"detected={report['fixed_key_associated_timing_detected']}"
        )
    print(f"summary={summary_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
