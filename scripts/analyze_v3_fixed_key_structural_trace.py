#!/usr/bin/env python3
"""Validate the bounded SQIsign-v3 fixed-key structural trace screen."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
IMPLEMENTATIONS = ("official", "d1")
RUNS = ("a", "b")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def parse_int(row: dict[str, str], key: str, base: int = 10) -> int:
    try:
        return int(row[key], base)
    except (KeyError, ValueError) as error:
        raise ValueError(f"invalid {key}: {row.get(key)!r}") from error


def load_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    require(rows, "trace CSV is empty")
    expected = {
        "implementation",
        "run",
        "dataset",
        "pair",
        "class",
        "execution_order",
        "key",
        "guard_count",
        "edge_events",
        "edge_unique",
        "edge_hash1",
        "edge_hash2",
        "edge_capture_events",
        "edge_capture_truncated",
        "load_events",
        "store_events",
        "memory_hash1",
        "memory_hash2",
        "memory_capture_events",
        "memory_capture_truncated",
        "signature_fnv64",
        "pair_edge_equal",
        "pair_memory_equal",
        "first_edge_diff_index",
        "first_edge_a_pc",
        "first_edge_b_pc",
        "first_memory_diff_index",
        "first_memory_a_event",
        "first_memory_b_event",
        "image_base",
        "status",
    }
    require(set(rows[0]) == expected, "trace CSV schema changed")
    require(all(set(row) == expected for row in rows), "inconsistent CSV rows")
    require(all(row["status"] == "PASS" for row in rows), "a trace failed")
    return rows


def summarize_pair(rows: list[dict[str, str]]) -> dict[str, object]:
    require(len(rows) == 2, "each pair must contain exactly two rows")
    classes = {row["class"]: row for row in rows}
    require(set(classes) == {"A", "B"}, "pair classes are not A/B")
    a, b = classes["A"], classes["B"]
    stable_fields = (
        "implementation",
        "run",
        "dataset",
        "pair",
        "guard_count",
        "pair_edge_equal",
        "pair_memory_equal",
        "first_edge_diff_index",
        "first_edge_a_pc",
        "first_edge_b_pc",
        "first_memory_diff_index",
        "first_memory_a_event",
        "first_memory_b_event",
        "image_base",
    )
    require(
        all(a[field] == b[field] for field in stable_fields),
        "pair comparison fields disagree",
    )
    require(
        {parse_int(a, "execution_order"), parse_int(b, "execution_order")} == {0, 1},
        "pair is not order balanced",
    )
    require(parse_int(a, "key") == 0, "class A must use key 0")
    require(parse_int(a, "guard_count") > 0, "coverage guards are absent")
    require(
        parse_int(a, "edge_events") > 0
        and parse_int(b, "edge_events") > 0
        and parse_int(a, "load_events") + parse_int(a, "store_events") > 0
        and parse_int(b, "load_events") + parse_int(b, "store_events") > 0,
        "edge or address instrumentation is absent",
    )
    edge_count_equal = parse_int(a, "edge_events") == parse_int(b, "edge_events")
    edge_equal = (
        edge_count_equal
        and a["edge_hash1"] == b["edge_hash1"]
        and a["edge_hash2"] == b["edge_hash2"]
    )
    address_count_equal = (
        parse_int(a, "load_events") == parse_int(b, "load_events")
        and parse_int(a, "store_events") == parse_int(b, "store_events")
    )
    address_equal = (
        address_count_equal
        and a["memory_hash1"] == b["memory_hash1"]
        and a["memory_hash2"] == b["memory_hash2"]
    )
    require(
        bool(parse_int(a, "pair_edge_equal")) == edge_equal,
        "edge equality flag disagrees with trace digest",
    )
    require(
        bool(parse_int(a, "pair_memory_equal")) == address_equal,
        "address equality flag disagrees with trace digest",
    )
    dataset = a["dataset"]
    key_b = parse_int(b, "key")
    pair = parse_int(a, "pair")
    if dataset == "control":
        require(key_b == 0 and pair in (0, 1), "bad control pair")
    else:
        require(dataset == "different-key", "unknown dataset")
        require(key_b == pair + 1 and 1 <= key_b <= 9, "bad primary pair")
    return {
        "implementation": a["implementation"],
        "run": a["run"],
        "dataset": dataset,
        "pair": pair,
        "key_a": 0,
        "key_b": key_b,
        "execution_order_a": parse_int(a, "execution_order"),
        "execution_order_b": parse_int(b, "execution_order"),
        "signature_equal": a["signature_fnv64"] == b["signature_fnv64"],
        "control_flow_event_count_equal": edge_count_equal,
        "control_flow_equal": edge_equal,
        "effective_address_event_count_equal": address_count_equal,
        "effective_address_equal": address_equal,
        "edge_events_a": parse_int(a, "edge_events"),
        "edge_events_b": parse_int(b, "edge_events"),
        "load_events_a": parse_int(a, "load_events"),
        "load_events_b": parse_int(b, "load_events"),
        "store_events_a": parse_int(a, "store_events"),
        "store_events_b": parse_int(b, "store_events"),
        "edge_capture_truncated": bool(
            parse_int(a, "edge_capture_truncated")
            or parse_int(b, "edge_capture_truncated")
        ),
        "memory_capture_truncated": bool(
            parse_int(a, "memory_capture_truncated")
            or parse_int(b, "memory_capture_truncated")
        ),
        "first_edge_difference": {
            "event_index": parse_int(a, "first_edge_diff_index"),
            "a_pc": a["first_edge_a_pc"],
            "b_pc": a["first_edge_b_pc"],
        },
        "first_memory_difference": {
            "event_index": parse_int(a, "first_memory_diff_index"),
            "a_event": a["first_memory_a_event"],
            "b_event": a["first_memory_b_event"],
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.raw = args.raw.resolve()
    args.manifest = args.manifest.resolve()
    args.output = args.output.resolve()

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    require(
        manifest.get("schema") == "sqisign-v3-fixed-key-structural-trace-campaign-v1",
        "unexpected campaign manifest",
    )
    require(manifest["design"]["fixed_key_buffer_address"] is True, "key buffer confound")
    require(manifest["design"]["process_runs_per_implementation"] == 2, "bad run count")
    require(all(not row["tracked_dirty"] for row in manifest["conditions"]), "dirty source")
    require(manifest["outputs"]["raw_csv_sha256"] == sha256(args.raw), "raw digest mismatch")

    rows = load_rows(args.raw)
    require(len(rows) == 88, f"expected 88 trace rows, observed {len(rows)}")
    grouped: dict[tuple[str, str, str, int], list[dict[str, str]]] = defaultdict(list)
    for row in rows:
        implementation = row["implementation"]
        run = row["run"]
        require(implementation in IMPLEMENTATIONS, "unknown implementation")
        require(run in RUNS, "unknown process run")
        grouped[(implementation, run, row["dataset"], parse_int(row, "pair"))].append(row)
    expected_group_count = len(IMPLEMENTATIONS) * len(RUNS) * (2 + 9)
    require(len(grouped) == expected_group_count, "missing trace pair")
    pairs = [summarize_pair(group) for _, group in sorted(grouped.items())]

    reports: dict[str, object] = {}
    repeatable_edge_keys: dict[str, list[int]] = {}
    repeatable_address_keys: dict[str, list[int]] = {}
    all_controls_stable = True
    for implementation in IMPLEMENTATIONS:
        run_reports: dict[str, object] = {}
        edge_sets: list[set[int]] = []
        address_sets: list[set[int]] = []
        for run in RUNS:
            selected = [
                row
                for row in pairs
                if row["implementation"] == implementation and row["run"] == run
            ]
            controls = [row for row in selected if row["dataset"] == "control"]
            primary = [row for row in selected if row["dataset"] == "different-key"]
            controls_stable = len(controls) == 2 and all(
                row["signature_equal"]
                and row["control_flow_equal"]
                and row["effective_address_equal"]
                for row in controls
            )
            all_controls_stable &= controls_stable
            require(controls_stable, f"{implementation}/{run}: negative control unstable")
            require(
                len(primary) == 9 and all(not row["signature_equal"] for row in primary),
                f"{implementation}/{run}: primary key/signature design failed",
            )
            edge_keys = {
                int(row["key_b"])
                for row in primary
                if not row["control_flow_equal"]
            }
            address_keys = {
                int(row["key_b"])
                for row in primary
                if not row["effective_address_equal"]
            }
            edge_sets.append(edge_keys)
            address_sets.append(address_keys)
            run_reports[run] = {
                "control_pairs_stable": 2,
                "different_key_pairs": 9,
                "control_flow_different_keys": sorted(edge_keys),
                "effective_address_different_keys": sorted(address_keys),
                "control_flow_difference_count": len(edge_keys),
                "effective_address_difference_count": len(address_keys),
            }
        repeatable_edge_keys[implementation] = sorted(set.intersection(*edge_sets))
        repeatable_address_keys[implementation] = sorted(set.intersection(*address_sets))
        reports[implementation] = {
            "runs": run_reports,
            "repeatable_control_flow_different_keys": repeatable_edge_keys[implementation],
            "repeatable_effective_address_different_keys": repeatable_address_keys[
                implementation
            ],
        }

    edge_detected = all(bool(repeatable_edge_keys[name]) for name in IMPLEMENTATIONS)
    address_detected = all(
        bool(repeatable_address_keys[name]) for name in IMPLEMENTATIONS
    )
    all_primary_edge_counts_differ = all(
        not bool(row["control_flow_event_count_equal"])
        for row in pairs
        if row["dataset"] == "different-key"
    )
    all_primary_address_counts_differ = all(
        not bool(row["effective_address_event_count_equal"])
        for row in pairs
        if row["dataset"] == "different-key"
    )
    result = {
        "schema": "sqisign-v3-fixed-key-structural-trace-analysis-v1",
        "status": "PASS",
        "design": manifest["design"],
        "provenance": {
            "campaign_manifest": str(args.manifest.relative_to(ROOT)),
            "campaign_manifest_sha256": sha256(args.manifest),
            "raw_csv": str(args.raw.relative_to(ROOT)),
            "raw_csv_sha256": sha256(args.raw),
            "harness": manifest["inputs"]["harness"],
            "compiler": manifest["compiler"],
            "conditions": manifest["conditions"],
        },
        "results": {
            "rows": len(rows),
            "pairs": pairs,
            "implementations": reports,
        },
        "decision": {
            "same_key_negative_controls_stable": all_controls_stable,
            "repeatable_fixed_key_associated_control_flow_observed": edge_detected,
            "repeatable_fixed_key_associated_effective_address_observed": address_detected,
            "all_primary_control_flow_event_counts_differ": all_primary_edge_counts_differ,
            "all_primary_effective_address_event_counts_differ": all_primary_address_counts_differ,
            "physical_leakage_established": False,
            "key_recovery_established": False,
            "constant_time_established": False,
            "side_channel_resistance_established": False,
        },
        "claim_boundary": (
            "Two native host processes per implementation; ten official p324_3 keys; "
            "one fixed public message and signing RNG stream.  Keys are copied into one "
            "fixed-address buffer before tracing.  SanitizerCoverage observes compiled "
            "edge IDs and load/store effective addresses during Sign only.  It does not "
            "observe values, register switching, Cortex-M33 code generation, power/EM, "
            "or key recovery; trace equality would not prove constant time."
        ),
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(
        "v3 fixed-key structural trace: PASS "
        f"controls_stable={all_controls_stable} "
        f"repeatable_edge={edge_detected} repeatable_address={address_detected} "
        "physical=false resistance=false"
    )
    for implementation in IMPLEMENTATIONS:
        print(
            f"{implementation}: edge_keys={repeatable_edge_keys[implementation]} "
            f"address_keys={repeatable_address_keys[implementation]}"
        )
    print(f"output={args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
