#!/usr/bin/env python3
"""Validate and summarize paired official-v3/v3-D1 RP2350 captures."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import statistics
from pathlib import Path


SOURCE_COMMIT = "6d017708db403bf83977fa70770fc4f7f9e9ff21"
CAPTURE_RE = re.compile(r"round-(\d+)-(official|d1)\.txt$")
KV_RE = re.compile(r"([A-Za-z0-9_]+)=([^\s]+)")
OPERATIONS = ("keygen", "sign", "verify")


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
    for key, value in expected.items():
        actual = fields.get(key)
        if actual != value:
            raise ValueError(
                f"{context}: expected {key}={value}, observed {actual!r}"
            )


def parse_capture(path: Path) -> dict[str, object]:
    match = CAPTURE_RE.match(path.name)
    if match is None:
        raise ValueError(f"unexpected capture name: {path.name}")
    round_number = int(match.group(1))
    kind = match.group(2)
    text = path.read_text(encoding="ascii")
    lines = text.splitlines()

    expected_banner = (
        "SQISIGN_RP2350_V3_BASELINE v1"
        if kind == "official"
        else "SQISIGN_RP2350_V3_D1 v1"
    )
    if not lines or lines[0] != expected_banner:
        raise ValueError(f"{path}: wrong or missing banner")

    identity = kv_line(lines, "scheme=")
    require(
        identity,
        {
            "scheme": "SQIsign-v3.0",
            "variant": "p324_3",
            "implementation": "m4f",
            "image": "baseline" if kind == "official" else "d1",
        },
        str(path),
    )
    board = kv_line(lines, "board=")
    require(
        board,
        {"board": "pico2", "platform": "rp2350-arm-s", "sdk": "2.3.0"},
        str(path),
    )
    source = kv_line(lines, "v3_source=")
    require(
        source,
        {
            "v3_source": SOURCE_COMMIT,
            "v3_dirty": "0" if kind == "official" else "1",
            "clock_sys_hz": "150000000",
        },
        str(path),
    )
    gate = kv_line(lines, "kat_decoded=")
    require(
        gate,
        {"kat_decoded": "1", "mode_ok": "1", "heap_section_bytes": "0"},
        str(path),
    )
    status = kv_line(lines, "status=")
    require(status, {"status": "PASS"}, str(path))

    operations: dict[str, dict[str, int]] = {}
    for operation in OPERATIONS:
        fields = kv_line(lines, f"{operation}_result=")
        require(
            fields,
            {
                f"{operation}_result": "0",
                f"{operation}_match": "1",
                "psp_reserved_bytes": "131072",
            },
            str(path),
        )
        operations[operation] = {
            "time_us": int(fields[f"{operation}_us"]),
            "psp_extent_bytes": int(fields["psp_written_bytes"]),
        }

    order_in_round = 1 if (round_number % 2 == 1) == (kind == "official") else 2
    return {
        "round": round_number,
        "order_in_round": order_in_round,
        "kind": kind,
        "capture": str(path),
        "capture_sha256": sha256(path),
        "firmware_commit": board["firmware"],
        "firmware_dirty": int(board["firmware_dirty"]),
        "operations": operations,
    }


def distribution(values: list[int]) -> dict[str, float | int]:
    return {
        "n": len(values),
        "min": min(values),
        "median": statistics.median(values),
        "mean": statistics.mean(values),
        "max": max(values),
        "sample_stdev": statistics.stdev(values) if len(values) > 1 else 0.0,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture_dir", type=Path)
    parser.add_argument("--official-elf", type=Path, required=True)
    parser.add_argument("--official-uf2", type=Path, required=True)
    parser.add_argument("--d1-elf", type=Path, required=True)
    parser.add_argument("--d1-uf2", type=Path, required=True)
    args = parser.parse_args()

    capture_paths = sorted(args.capture_dir.glob("round-*.txt"))
    records = [parse_capture(path) for path in capture_paths]
    rounds = sorted({int(record["round"]) for record in records})
    expected_keys = {(round_number, kind) for round_number in rounds for kind in ("official", "d1")}
    actual_keys = {(int(record["round"]), str(record["kind"])) for record in records}
    if actual_keys != expected_keys or len(records) != 2 * len(rounds):
        raise ValueError("campaign does not contain exactly one official/d1 pair per round")
    if rounds != list(range(1, len(rounds) + 1)):
        raise ValueError("round numbers must be contiguous and start at one")

    indexed = {
        (int(record["round"]), str(record["kind"])): record for record in records
    }
    summary: dict[str, object] = {}
    for operation in OPERATIONS:
        operation_summary: dict[str, object] = {}
        for kind in ("official", "d1"):
            times = [
                int(indexed[(round_number, kind)]["operations"][operation]["time_us"])
                for round_number in rounds
            ]
            stacks = [
                int(indexed[(round_number, kind)]["operations"][operation]["psp_extent_bytes"])
                for round_number in rounds
            ]
            operation_summary[kind] = {
                "time_us": distribution(times),
                "psp_extent_bytes": distribution(stacks),
            }

        paired_rows = []
        for round_number in rounds:
            official = int(
                indexed[(round_number, "official")]["operations"][operation]["time_us"]
            )
            d1 = int(indexed[(round_number, "d1")]["operations"][operation]["time_us"])
            paired_rows.append(
                {
                    "round": round_number,
                    "delta_us": d1 - official,
                    "ratio": d1 / official,
                    "delta_percent": 100.0 * (d1 - official) / official,
                }
            )
        operation_summary["paired_d1_minus_official"] = {
            "delta_us": distribution([row["delta_us"] for row in paired_rows]),
            "ratio": distribution([row["ratio"] for row in paired_rows]),
            "delta_percent": distribution([row["delta_percent"] for row in paired_rows]),
            "pairs": paired_rows,
        }
        summary[operation] = operation_summary

    artifact_paths = {
        "official_elf": args.official_elf,
        "official_uf2": args.official_uf2,
        "d1_elf": args.d1_elf,
        "d1_uf2": args.d1_uf2,
    }
    artifacts = {
        name: {"path": str(path), "sha256": sha256(path)}
        for name, path in artifact_paths.items()
    }

    output = {
        "schema": "sqisign-v3-rp2350-interleaved-campaign-v1",
        "campaign": args.capture_dir.name,
        "design": {
            "board_count": 1,
            "paired_rounds": len(rounds),
            "capture_count": len(records),
            "order": "odd rounds official-d1; even rounds d1-official",
            "workload": "official KAT vector 0, deterministic KeyGen-Sign-Verify, one boot per capture",
            "interpretation": "descriptive paired repetition; not an independent-board or workload-distribution study",
        },
        "source_commit": SOURCE_COMMIT,
        "all_captures_passed": True,
        "records": records,
        "summary": summary,
        "artifacts": artifacts,
    }

    json_path = args.capture_dir / "summary.json"
    json_path.write_text(
        json.dumps(output, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    csv_path = args.capture_dir / "measurements.csv"
    with csv_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(
            [
                "round",
                "order_in_round",
                "kind",
                "operation",
                "time_us",
                "psp_extent_bytes",
                "capture_sha256",
            ]
        )
        for record in sorted(
            records, key=lambda item: (int(item["round"]), int(item["order_in_round"]))
        ):
            for operation in OPERATIONS:
                writer.writerow(
                    [
                        record["round"],
                        record["order_in_round"],
                        record["kind"],
                        operation,
                        record["operations"][operation]["time_us"],
                        record["operations"][operation]["psp_extent_bytes"],
                        record["capture_sha256"],
                    ]
                )

    print(f"validated_captures={len(records)} paired_rounds={len(rounds)} status=PASS")
    for operation in OPERATIONS:
        paired = summary[operation]["paired_d1_minus_official"]
        print(
            f"{operation}_delta_percent_median={paired['delta_percent']['median']:.6f} "
            f"min={paired['delta_percent']['min']:.6f} "
            f"max={paired['delta_percent']['max']:.6f}"
        )
    print(f"json={json_path}")
    print(f"csv={csv_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
