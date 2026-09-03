#!/usr/bin/env python3
"""Validate the bounded SQIsign-v3 fixed-key RP2350 timing screen."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import re
import statistics
import subprocess
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
KV_RE = re.compile(r"([A-Za-z0-9_]+)=([^\s]+)")
IMPLEMENTATIONS = ("baseline", "d1")
PASSES = ("A", "B")
OFFICIAL_COMMIT = "6d017708db403bf83977fa70770fc4f7f9e9ff21"
KAT_RSP_SHA256 = "b632c926c72692f850a71b7f7bb338fb1aeee0d9f3362aa35a8ac6255ca3155b"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def kv_line(lines: list[str], prefix: str) -> dict[str, str]:
    matches = [line for line in lines if line.startswith(prefix)]
    if len(matches) != 1:
        raise ValueError(f"expected one {prefix!r} line, found {len(matches)}")
    return dict(KV_RE.findall(matches[0]))


def require(fields: dict[str, str], expected: dict[str, str], context: str) -> None:
    for key, expected_value in expected.items():
        observed = fields.get(key)
        if observed != expected_value:
            raise ValueError(
                f"{context}: expected {key}={expected_value}, observed {observed!r}"
            )


def distribution(values: list[int | float]) -> dict[str, int | float]:
    if not values:
        raise ValueError("cannot summarize an empty distribution")
    return {
        "n": len(values),
        "min": min(values),
        "median": statistics.median(values),
        "mean": statistics.mean(values),
        "max": max(values),
        "sample_stdev": statistics.stdev(values) if len(values) > 1 else 0.0,
    }


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
        raise ValueError("correlation requires equal vectors of length >= 2")
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


def parse_capture(
    path: Path,
    implementation: str,
    expected_firmware_commit: str,
    expected_d1_commit: str,
) -> dict[str, object]:
    lines = path.read_text(encoding="ascii").splitlines()
    label = "BASELINE" if implementation == "baseline" else "D1"
    banner = f"SQISIGN_RP2350_V3_{label}_SCA_KEY v1"
    if not lines or lines[0] != banner:
        raise ValueError(f"{path}: wrong or missing banner")

    require(
        kv_line(lines, "scheme="),
        {
            "scheme": "SQIsign-v3.0",
            "variant": "p324_3",
            "implementation": "m4f",
            "image": f"{implementation}-sca-key",
        },
        str(path),
    )
    require(
        kv_line(lines, "board="),
        {
            "board": "pico2",
            "platform": "rp2350-arm-s",
            "sdk": "2.3.0",
            "firmware": expected_firmware_commit,
            "firmware_dirty": "0",
        },
        str(path),
    )
    require(
        kv_line(lines, "v3_source="),
        {
            "v3_source": (
                OFFICIAL_COMMIT if implementation == "baseline" else expected_d1_commit
            ),
            "v3_dirty": "0",
            "clock_sys_hz": "150000000",
        },
        str(path),
    )
    kat = kv_line(lines, "kat_rsp_sha256=")
    require(
        kat,
        {
            "kat_rsp_sha256": KAT_RSP_SHA256,
            "kat_first": "0",
            "kat_count": "10",
            "fixed_message_vector": "0",
            "fixed_message_bytes": "33",
            "signing_seed": "a5-sequence-v1",
            "fixed_key_buffer_address": "1",
            "xip_cache_invalidate_before_timing": "1",
            "repetitions": "5",
            "passes": "2",
            "samples": "100",
        },
        str(path),
    )
    require(kv_line(lines, "bss_end="), {"heap_section_bytes": "0"}, str(path))
    require(
        kv_line(lines, "warmup_sign="),
        {"warmup_sign": "0", "warmup_verify": "0", "warmup_status": "PASS"},
        str(path),
    )

    samples: list[dict[str, object]] = []
    for line in lines:
        if not line.startswith("pass="):
            continue
        fields = dict(KV_RE.findall(line))
        require(
            fields,
            {
                "sign_result": "0",
                "verify_result": "0",
                "digest_stable": "1",
                "sample_status": "PASS",
            },
            f"{path}: sample",
        )
        samples.append(
            {
                "implementation": implementation,
                "pass": fields["pass"],
                "sequence": int(fields["sequence"]),
                "key": int(fields["key"]),
                "repetition": int(fields["repetition"]),
                "sign_us": int(fields["sign_us"]),
                "sign_psp": int(fields["sign_psp"]),
                "verify_us": int(fields["verify_us"]),
                "verify_psp": int(fields["verify_psp"]),
                "signature_fnv1a64": fields["digest"],
            }
        )
    if len(samples) != 100:
        raise ValueError(f"{path}: expected 100 samples, observed {len(samples)}")

    summary = kv_line(lines, "summary samples=")
    require(
        summary,
        {
            "samples": "100",
            "passed": "100",
            "mode_ok": "1",
            "msp_pattern_ok": "1",
            "psp_reserved_bytes": "131072",
        },
        str(path),
    )
    msp = kv_line(lines, "msp_reserved_bytes=")
    require(msp, {"msp_reserved_bytes": "8192"}, str(path))
    require(kv_line(lines, "status="), {"status": "PASS"}, str(path))

    return {
        "implementation": implementation,
        "capture": path.name,
        "capture_bytes": path.stat().st_size,
        "capture_sha256": sha256(path),
        "generated_tree_sha256": kv_line(lines, "v3_generated_tree_sha256=")[
            "v3_generated_tree_sha256"
        ],
        "summary": {
            "sign_psp_max": int(summary["sign_psp_max"]),
            "verify_psp_max": int(summary["verify_psp_max"]),
            "msp_written_upper_bytes": int(msp["msp_written_upper_bytes"]),
        },
        "samples": samples,
    }


def artifact_metadata(
    build_dir: Path, implementation: str, size_tool: Path
) -> dict[str, object]:
    target = f"sqisign_rp2350_v3_{implementation}_sca_key"
    paths = {
        "elf": build_dir / f"{target}.elf",
        "uf2": build_dir / f"{target}.uf2",
        "map": build_dir / f"{target}.elf.map",
        "archive": build_dir / "libsqisign_v3_p324_3_m4f.a",
    }
    for label, path in paths.items():
        if not path.is_file():
            raise ValueError(f"{implementation}: missing {label}: {path}")

    size_output = subprocess.run(
        [str(size_tool), str(paths["elf"])],
        check=True,
        text=True,
        capture_output=True,
    ).stdout.splitlines()
    values = size_output[-1].split()
    if len(values) < 6:
        raise ValueError(f"{implementation}: malformed size output")
    text_bytes, data_bytes, bss_bytes = map(int, values[:3])

    stack_records = 0
    dynamic_records = 0
    for stack_path in sorted(build_dir.rglob("*.su")):
        for line in stack_path.read_text(encoding="utf-8").splitlines():
            if not line.strip():
                continue
            stack_records += 1
            fields = line.rsplit("\t", 2)
            if len(fields) == 3 and "dynamic" in fields[2].split(","):
                dynamic_records += 1
    if dynamic_records != 19:
        raise ValueError(
            f"{implementation}: expected 19 dynamic stack records, "
            f"observed {dynamic_records}"
        )

    return {
        "size": {
            "text_bytes": text_bytes,
            "data_bytes": data_bytes,
            "bss_bytes": bss_bytes,
        },
        "stack_usage": {
            "records": stack_records,
            "dynamic_records": dynamic_records,
        },
        "files": {
            label: {
                "filename": path.name,
                "bytes": path.stat().st_size,
                "sha256": sha256(path),
            }
            for label, path in paths.items()
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture_dir", type=Path)
    parser.add_argument("--baseline-build", type=Path, required=True)
    parser.add_argument("--d1-build", type=Path, required=True)
    parser.add_argument("--size-tool", type=Path, required=True)
    parser.add_argument("--expected-firmware-commit", required=True)
    parser.add_argument("--expected-d1-commit", required=True)
    parser.add_argument(
        "--design",
        type=Path,
        default=ROOT / "experiments/sca/v3-rp2350-fixed-key-timing-design.json",
    )
    args = parser.parse_args()

    design_record = json.loads(args.design.read_text(encoding="utf-8"))
    if (
        design_record.get("schema")
        != "sqisign-v3-rp2350-fixed-key-timing-design-v1"
        or design_record.get("status") != "FROZEN_BEFORE_TARGET_CAPTURE"
    ):
        raise ValueError("unexpected predeclared design record")
    expected_design = {
        "official_keys": 10,
        "repetitions_per_key_per_pass": 5,
        "samples_per_implementation": 100,
        "total_sign_samples": 200,
    }
    for field, expected in expected_design.items():
        if design_record["design"].get(field) != expected:
            raise ValueError(f"predeclared design changed: {field}")
    if design_record.get("firmware_commit") != args.expected_firmware_commit:
        raise ValueError("predeclared design names a different firmware commit")
    if design_record["sources"].get("official") != OFFICIAL_COMMIT:
        raise ValueError("predeclared design names a different official source")
    if design_record["sources"].get("d1") != args.expected_d1_commit:
        raise ValueError("predeclared design names a different D1 source")
    rule = design_record["predeclared_decision_rule"]
    if (
        rule.get("between_pass_per_key_median_spearman_minimum") != 0.8
        or rule.get("each_pass_between_key_span_fraction_minimum") != 0.01
        or rule.get("each_pass_span_to_median_within_key_mad_minimum") != 10.0
        or rule.get("overall_detection_requires_both_implementations") is not True
    ):
        raise ValueError("predeclared timing decision rule changed")

    records = {
        implementation: parse_capture(
            args.capture_dir / f"{implementation}.txt",
            implementation,
            args.expected_firmware_commit,
            args.expected_d1_commit,
        )
        for implementation in IMPLEMENTATIONS
    }
    artifacts = {
        "baseline": artifact_metadata(
            args.baseline_build, "baseline", args.size_tool
        ),
        "d1": artifact_metadata(args.d1_build, "d1", args.size_tool),
    }

    cells: dict[tuple[str, str, int], list[dict[str, object]]] = defaultdict(list)
    digests: dict[tuple[str, int], set[str]] = defaultdict(set)
    for implementation in IMPLEMENTATIONS:
        for row in records[implementation]["samples"]:
            pass_label = str(row["pass"])
            key = int(row["key"])
            repetition = int(row["repetition"])
            sequence = int(row["sequence"])
            if pass_label not in PASSES or not 0 <= key < 10:
                raise ValueError("sample outside the declared pass/key design")
            if not 0 <= repetition < 5 or not 0 <= sequence < 50:
                raise ValueError("sample outside the declared repetition/sequence design")
            cells[(implementation, pass_label, key)].append(row)
            digests[(implementation, key)].add(str(row["signature_fnv1a64"]))

    expected_cells = len(IMPLEMENTATIONS) * len(PASSES) * 10
    if len(cells) != expected_cells or any(len(rows) != 5 for rows in cells.values()):
        raise ValueError("missing or unbalanced implementation/pass/key cell")
    for implementation in IMPLEMENTATIONS:
        for pass_label in PASSES:
            pass_rows = [
                row
                for row in records[implementation]["samples"]
                if row["pass"] == pass_label
            ]
            if sorted(int(row["sequence"]) for row in pass_rows) != list(range(50)):
                raise ValueError("sequence index is not a permutation of 0..49")
            for key in range(10):
                repetitions = sorted(
                    int(row["repetition"])
                    for row in pass_rows
                    if int(row["key"]) == key
                )
                if repetitions != [0, 1, 2, 3, 4]:
                    raise ValueError("per-key repetition labels are not [0, 1, 2, 3, 4]")
    if any(len(values) != 1 for values in digests.values()):
        raise ValueError("fixed RNG/message did not give one stable signature per key")
    for key in range(10):
        if digests[("baseline", key)] != digests[("d1", key)]:
            raise ValueError(f"baseline/D1 signature mismatch for key {key}")

    # The two binaries use the same compile-time schedule.  Pairing is accepted
    # only after the emitted schedule labels have been compared mechanically.
    paired_rows: list[dict[str, object]] = []
    by_sequence: dict[tuple[str, str, int], dict[str, object]] = {}
    for implementation in IMPLEMENTATIONS:
        for row in records[implementation]["samples"]:
            by_sequence[(implementation, str(row["pass"]), int(row["sequence"]))] = row
    for pass_label in PASSES:
        for sequence in range(50):
            baseline = by_sequence[("baseline", pass_label, sequence)]
            d1 = by_sequence[("d1", pass_label, sequence)]
            if (baseline["key"], baseline["repetition"]) != (
                d1["key"],
                d1["repetition"],
            ):
                raise ValueError("baseline/D1 emitted schedules differ")
            paired_rows.append(
                {
                    "pass": pass_label,
                    "sequence": sequence,
                    "key": int(baseline["key"]),
                    "repetition": int(baseline["repetition"]),
                    "sign_psp_delta_bytes": int(d1["sign_psp"])
                    - int(baseline["sign_psp"]),
                    "verify_psp_delta_bytes": int(d1["verify_psp"])
                    - int(baseline["verify_psp"]),
                    "sign_time_ratio": int(d1["sign_us"]) / int(baseline["sign_us"]),
                    "sign_time_delta_percent": 100.0
                    * (int(d1["sign_us"]) - int(baseline["sign_us"]))
                    / int(baseline["sign_us"]),
                }
            )
    if set(int(row["sign_psp_delta_bytes"]) for row in paired_rows) != {-3928}:
        raise ValueError("unexpected D1 Sign PSP delta")
    if set(int(row["verify_psp_delta_bytes"]) for row in paired_rows) != {0}:
        raise ValueError("unexpected D1 Verify PSP delta")

    per_key_rows: list[dict[str, object]] = []
    for implementation in IMPLEMENTATIONS:
        for pass_label in PASSES:
            for key in range(10):
                values = [
                    int(row["sign_us"])
                    for row in cells[(implementation, pass_label, key)]
                ]
                per_key_rows.append(
                    {
                        "implementation": implementation,
                        "pass": pass_label,
                        "key": key,
                        "samples": len(values),
                        "median_sign_us": statistics.median(values),
                        "mean_sign_us": statistics.mean(values),
                        "mad_sign_us": median_absolute_deviation(values),
                        "min_sign_us": min(values),
                        "max_sign_us": max(values),
                        "signature_fnv1a64": next(iter(digests[(implementation, key)])),
                    }
                )
    indexed_per_key = {
        (str(row["implementation"]), str(row["pass"]), int(row["key"])): row
        for row in per_key_rows
    }

    implementation_reports: dict[str, object] = {}
    detections: list[bool] = []
    for implementation in IMPLEMENTATIONS:
        pass_reports: dict[str, object] = {}
        median_vectors: dict[str, list[float]] = {}
        for pass_label in PASSES:
            medians = [
                float(indexed_per_key[(implementation, pass_label, key)]["median_sign_us"])
                for key in range(10)
            ]
            mads = [
                float(indexed_per_key[(implementation, pass_label, key)]["mad_sign_us"])
                for key in range(10)
            ]
            median_vectors[pass_label] = medians
            span = max(medians) - min(medians)
            central_median = statistics.median(medians)
            within_mad = statistics.median(mads)
            pass_reports[pass_label] = {
                "key_median_min_us": min(medians),
                "key_median_max_us": max(medians),
                "key_median_span_us": span,
                "key_median_span_fraction": span / central_median,
                "median_within_key_mad_us": within_mad,
                "span_to_within_key_mad_ratio": span / max(within_mad, 1.0),
                "fastest_key": medians.index(min(medians)),
                "slowest_key": medians.index(max(medians)),
            }
        correlation = spearman(median_vectors["A"], median_vectors["B"])
        detected = correlation >= 0.8 and all(
            float(pass_reports[label]["key_median_span_fraction"]) >= 0.01
            and float(pass_reports[label]["span_to_within_key_mad_ratio"]) >= 10.0
            for label in PASSES
        )
        detections.append(detected)
        implementation_reports[implementation] = {
            "passes": pass_reports,
            "between_pass_key_median_spearman": correlation,
            "predeclared_screen_rule": (
                "Spearman >= 0.8 for the two per-key median rankings; in both "
                "passes, between-key span >= 1% of the central median and >= "
                "10x the median within-key MAD"
            ),
            "fixed_key_associated_timing_detected": detected,
            "sign_us": distribution(
                [int(row["sign_us"]) for row in records[implementation]["samples"]]
            ),
            "verify_us": distribution(
                [int(row["verify_us"]) for row in records[implementation]["samples"]]
            ),
            "sign_psp_bytes": distribution(
                [int(row["sign_psp"]) for row in records[implementation]["samples"]]
            ),
            "verify_psp_bytes": distribution(
                [int(row["verify_psp"]) for row in records[implementation]["samples"]]
            ),
        }

    cross_implementation_correlations = {
        pass_label: spearman(
            [
                float(indexed_per_key[("baseline", pass_label, key)]["median_sign_us"])
                for key in range(10)
            ],
            [
                float(indexed_per_key[("d1", pass_label, key)]["median_sign_us"])
                for key in range(10)
            ],
        )
        for pass_label in PASSES
    }

    measurements_path = args.capture_dir / "measurements.csv"
    with measurements_path.open("w", newline="", encoding="utf-8") as handle:
        fieldnames = [
            "implementation",
            "pass",
            "sequence",
            "key",
            "repetition",
            "sign_us",
            "sign_psp",
            "verify_us",
            "verify_psp",
            "signature_fnv1a64",
        ]
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        for implementation in IMPLEMENTATIONS:
            writer.writerows(records[implementation]["samples"])

    per_key_path = args.capture_dir / "per-key.csv"
    with per_key_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(per_key_rows[0]))
        writer.writeheader()
        writer.writerows(per_key_rows)

    summary = {
        "schema": "sqisign-v3-rp2350-fixed-key-timing-analysis-v1",
        "status": "PASS",
        "campaign": args.capture_dir.name,
        "design": {
            "board_count": 1,
            "implementations": list(IMPLEMENTATIONS),
            "official_keys": 10,
            "fixed_message_vector": 0,
            "fixed_message_bytes": 33,
            "fixed_signing_rng": "a5-sequence-v1",
            "passes": list(PASSES),
            "repetitions_per_key_per_pass": 5,
            "samples_per_implementation": 100,
            "total_sign_samples": 200,
            "warmup_signs_per_implementation": 1,
            "outer_order": ["baseline", "d1"],
        },
        "firmware_commit": args.expected_firmware_commit,
        "official_source_commit": OFFICIAL_COMMIT,
        "d1_source_commit": args.expected_d1_commit,
        "all_trees_clean": True,
        "predeclared_design": {
            "filename": args.design.name,
            "sha256": sha256(args.design),
            "status": design_record["status"],
        },
        "validation": {
            "all_signatures_verified": True,
            "all_cells_balanced": True,
            "emitted_schedules_identical": True,
            "stable_signature_per_key_under_fixed_rng_and_message": True,
            "baseline_and_d1_signatures_identical": True,
            "direct_key_pointer_address_confound_removed": True,
            "xip_cache_state_reset_before_each_timed_operation": True,
            "all_psp_canaries_intact": True,
            "heap_section_bytes": 0,
        },
        "captures": records,
        "artifacts": artifacts,
        "implementations": implementation_reports,
        "paired_d1_minus_baseline": {
            "sign_psp_delta_bytes": distribution(
                [int(row["sign_psp_delta_bytes"]) for row in paired_rows]
            ),
            "verify_psp_delta_bytes": distribution(
                [int(row["verify_psp_delta_bytes"]) for row in paired_rows]
            ),
            "sign_time_ratio": distribution(
                [float(row["sign_time_ratio"]) for row in paired_rows]
            ),
            "sign_time_delta_percent": distribution(
                [float(row["sign_time_delta_percent"]) for row in paired_rows]
            ),
        },
        "baseline_d1_key_median_spearman": cross_implementation_correlations,
        "decision": {
            "repeatable_fixed_key_associated_rp2350_timing_observed": all(detections),
            "rp2350_m4f_wall_clock_timing_tested": True,
            "secret_dependent_control_or_address_trace_established": False,
            "physical_leakage_established": False,
            "key_recovery_established": False,
            "side_channel_resistance_established": False,
        },
        "claim_boundary": (
            "One RP2350 board, ten official p324_3 keys copied into one fixed-address "
            "buffer, an XIP-cache invalidation immediately before each timed operation, "
            "one fixed public message, "
            "one fixed signing RNG stream, two deterministic randomized schedules, "
            "and two samples per key per schedule.  This bounded screen can expose "
            "repeatable key-associated wall-clock time on this target; it is not a "
            "physical leakage test, a control/address trace, an attack, a worst-case "
            "timing claim, or evidence of side-channel resistance."
        ),
        "outputs": {
            "measurements_csv": {
                "filename": measurements_path.name,
                "sha256": sha256(measurements_path),
            },
            "per_key_csv": {
                "filename": per_key_path.name,
                "sha256": sha256(per_key_path),
            },
        },
    }
    summary_path = args.capture_dir / "summary.json"
    summary_path.write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    print(
        "v3 RP2350 fixed-key timing analysis: PASS "
        f"samples=200 repeatable_key_association={all(detections)} resistance=false"
    )
    for implementation in IMPLEMENTATIONS:
        report = implementation_reports[implementation]
        print(
            f"{implementation}: "
            f"spearman={report['between_pass_key_median_spearman']:.6f} "
            f"detected={report['fixed_key_associated_timing_detected']}"
        )
    print(f"summary={summary_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
