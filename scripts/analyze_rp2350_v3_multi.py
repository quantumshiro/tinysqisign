#!/usr/bin/env python3
"""Validate the SQIsign-v3 multi-input/code-placement RP2350 campaign."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import statistics
import subprocess
from pathlib import Path


KV_RE = re.compile(r"([A-Za-z0-9_]+)=([^\s]+)")
CONDITIONS = ("baseline-a", "baseline-b", "d1-a", "d1-b")
OPERATIONS = ("keygen", "sign", "verify", "negative")
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
    return {
        "n": len(values),
        "min": min(values),
        "median": statistics.median(values),
        "mean": statistics.mean(values),
        "max": max(values),
        "sample_stdev": statistics.stdev(values) if len(values) > 1 else 0.0,
    }


def condition_parts(condition: str) -> tuple[str, str]:
    kind, placement = condition.split("-", 1)
    return kind, placement


def parse_capture(
    path: Path,
    condition: str,
    expected_firmware_commit: str,
    expected_d1_commit: str,
) -> dict[str, object]:
    kind, placement = condition_parts(condition)
    lines = path.read_text(encoding="ascii").splitlines()
    banner_kind = "BASELINE" if kind == "baseline" else "D1"
    expected_banner = (
        f"SQISIGN_RP2350_V3_{banner_kind}_MULTI_{placement.upper()} v1"
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
            "image": f"{kind}-multi",
            "placement": placement,
        },
        str(path),
    )
    board = kv_line(lines, "board=")
    require(
        board,
        {
            "board": "pico2",
            "platform": "rp2350-arm-s",
            "sdk": "2.3.0",
            "firmware": expected_firmware_commit,
            "firmware_dirty": "0",
        },
        str(path),
    )
    source = kv_line(lines, "v3_source=")
    require(
        source,
        {
            "v3_source": OFFICIAL_COMMIT if kind == "baseline" else expected_d1_commit,
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
            "max_mlen": "330",
        },
        str(path),
    )
    addresses = kv_line(lines, "placement_anchor=")
    memory = kv_line(lines, "bss_end=")
    require(memory, {"heap_section_bytes": "0"}, str(path))

    vectors: list[dict[str, object]] = []
    for vector in range(10):
        keygen = kv_line(lines, f"vector={vector} mlen=")
        sign = kv_line(lines, f"vector={vector} sign_result=")
        verify = kv_line(lines, f"vector={vector} verify_result=")
        status = kv_line(lines, f"vector={vector} vector_status=")
        require(
            keygen,
            {"keygen_result": "0", "keygen_match": "1"},
            f"{path}: vector {vector} KeyGen",
        )
        require(
            sign,
            {"sign_result": "0", "sign_match": "1"},
            f"{path}: vector {vector} Sign",
        )
        require(
            verify,
            {
                "verify_result": "0",
                "verify_match": "1",
                "negative_rejected": "1",
            },
            f"{path}: vector {vector} Verify",
        )
        require(status, {"vector_status": "PASS"}, f"{path}: vector {vector}")
        vectors.append(
            {
                "vector": vector,
                "message_bytes": int(keygen["mlen"]),
                "signed_message_bytes": int(sign["smlen"]),
                "operations": {
                    "keygen": {
                        "time_us": int(keygen["keygen_us"]),
                        "psp_extent_bytes": int(keygen["keygen_psp"]),
                    },
                    "sign": {
                        "time_us": int(sign["sign_us"]),
                        "psp_extent_bytes": int(sign["sign_psp"]),
                    },
                    "verify": {
                        "time_us": int(verify["verify_us"]),
                        "psp_extent_bytes": int(verify["verify_psp"]),
                    },
                    "negative": {
                        "time_us": int(verify["negative_us"]),
                        "psp_extent_bytes": int(verify["negative_psp"]),
                    },
                },
            }
        )

    campaign_summary = kv_line(lines, "summary vectors=")
    require(
        campaign_summary,
        {
            "vectors": "10",
            "passed": "10",
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
        "condition": condition,
        "kind": kind,
        "placement": placement,
        "capture": path.name,
        "capture_sha256": sha256(path),
        "generated_tree_sha256": kv_line(lines, "v3_generated_tree_sha256=")[
            "v3_generated_tree_sha256"
        ],
        "addresses": {key: int(value, 16) for key, value in addresses.items()},
        "bss_end": int(memory["bss_end"], 16),
        "msp_written_upper_bytes": int(msp["msp_written_upper_bytes"]),
        "vectors": vectors,
    }


def artifact_metadata(build_dir: Path, condition: str, size_tool: Path) -> dict[str, object]:
    kind, placement = condition_parts(condition)
    target = f"sqisign_rp2350_v3_{kind}_multi_{placement}"
    paths = {
        "elf": build_dir / f"{target}.elf",
        "uf2": build_dir / f"{target}.uf2",
        "archive": build_dir / "libsqisign_v3_p324_3_m4f.a",
    }
    for name, path in paths.items():
        if not path.is_file():
            raise ValueError(f"{condition}: missing {name}: {path}")
    size_output = subprocess.run(
        [str(size_tool), str(paths["elf"])],
        check=True,
        text=True,
        capture_output=True,
    ).stdout.splitlines()
    if len(size_output) < 2:
        raise ValueError(f"{condition}: cannot parse size output")
    values = size_output[-1].split()
    if len(values) < 6:
        raise ValueError(f"{condition}: malformed size output: {size_output[-1]}")
    text_bytes, data_bytes, bss_bytes = map(int, values[:3])

    stack_records = 0
    dynamic_records = 0
    for path in sorted(build_dir.rglob("*.su")):
        for line in path.read_text(encoding="utf-8").splitlines():
            if not line.strip():
                continue
            stack_records += 1
            fields = line.rsplit("\t", 2)
            if len(fields) == 3 and "dynamic" in fields[2].split(","):
                dynamic_records += 1
    if dynamic_records != 19:
        raise ValueError(
            f"{condition}: expected 19 dynamic stack records, observed {dynamic_records}"
        )

    return {
        "logical_build": condition,
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
            name: {
                "filename": path.name,
                "bytes": path.stat().st_size,
                "sha256": sha256(path),
            }
            for name, path in paths.items()
        },
    }


def vector_index(record: dict[str, object]) -> dict[int, dict[str, object]]:
    return {int(row["vector"]): row for row in record["vectors"]}


def latex_num(value: int | float, digits: int | None = None) -> str:
    if digits is None:
        return f"\\num{{{value}}}"
    return f"\\num{{{float(value):.{digits}f}}}"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture_dir", type=Path)
    parser.add_argument("--baseline-a-build", type=Path, required=True)
    parser.add_argument("--baseline-b-build", type=Path, required=True)
    parser.add_argument("--d1-a-build", type=Path, required=True)
    parser.add_argument("--d1-b-build", type=Path, required=True)
    parser.add_argument("--size-tool", type=Path, required=True)
    parser.add_argument("--expected-firmware-commit", required=True)
    parser.add_argument("--expected-d1-commit", required=True)
    parser.add_argument("--latex-output", type=Path)
    args = parser.parse_args()

    records = {
        condition: parse_capture(
            args.capture_dir / f"{condition}.txt",
            condition,
            args.expected_firmware_commit,
            args.expected_d1_commit,
        )
        for condition in CONDITIONS
    }
    builds = {
        "baseline-a": args.baseline_a_build,
        "baseline-b": args.baseline_b_build,
        "d1-a": args.d1_a_build,
        "d1-b": args.d1_b_build,
    }
    artifacts = {
        condition: artifact_metadata(builds[condition], condition, args.size_tool)
        for condition in CONDITIONS
    }

    # Variant B inserts exactly 512 Thumb NOPs before the crypto archive.
    address_checks: dict[str, object] = {}
    for kind in ("baseline", "d1"):
        addresses_a = records[f"{kind}-a"]["addresses"]
        addresses_b = records[f"{kind}-b"]["addresses"]
        deltas = {
            symbol: int(addresses_b[symbol]) - int(addresses_a[symbol])
            for symbol in addresses_a
        }
        expected = {
            "placement_anchor": 0,
            "crypto_sign_keypair": 1024,
            "crypto_sign": 1024,
            "crypto_sign_open": 1024,
        }
        if deltas != expected:
            raise ValueError(f"{kind}: unexpected placement deltas: {deltas}")
        address_checks[kind] = {"observed_delta_bytes": deltas, "passed": True}

    placement_psp_mismatches: list[dict[str, object]] = []
    implementation_deltas: list[dict[str, object]] = []
    for kind in ("baseline", "d1"):
        left = vector_index(records[f"{kind}-a"])
        right = vector_index(records[f"{kind}-b"])
        for vector in range(10):
            for operation in OPERATIONS:
                psp_a = int(left[vector]["operations"][operation]["psp_extent_bytes"])
                psp_b = int(right[vector]["operations"][operation]["psp_extent_bytes"])
                if psp_a != psp_b:
                    placement_psp_mismatches.append(
                        {
                            "kind": kind,
                            "vector": vector,
                            "operation": operation,
                            "a": psp_a,
                            "b": psp_b,
                        }
                    )
    if placement_psp_mismatches:
        raise ValueError(f"placement changed PSP use: {placement_psp_mismatches}")

    for placement in ("a", "b"):
        baseline = vector_index(records[f"baseline-{placement}"])
        d1 = vector_index(records[f"d1-{placement}"])
        for vector in range(10):
            for operation in OPERATIONS:
                baseline_row = baseline[vector]["operations"][operation]
                d1_row = d1[vector]["operations"][operation]
                implementation_deltas.append(
                    {
                        "placement": placement,
                        "vector": vector,
                        "operation": operation,
                        "psp_delta_bytes": int(d1_row["psp_extent_bytes"])
                        - int(baseline_row["psp_extent_bytes"]),
                        "time_delta_percent": 100.0
                        * (int(d1_row["time_us"]) - int(baseline_row["time_us"]))
                        / int(baseline_row["time_us"]),
                    }
                )

    expected_psp_deltas = {"keygen": 0, "sign": -3928, "verify": 0, "negative": 0}
    for row in implementation_deltas:
        if int(row["psp_delta_bytes"]) != expected_psp_deltas[str(row["operation"])]:
            raise ValueError(f"unexpected D1 PSP delta: {row}")

    summary: dict[str, object] = {}
    for operation in OPERATIONS:
        operation_summary: dict[str, object] = {}
        for kind in ("baseline", "d1"):
            observations = []
            for placement in ("a", "b"):
                for row in records[f"{kind}-{placement}"]["vectors"]:
                    observations.append(row["operations"][operation])
            operation_summary[kind] = {
                "time_us": distribution([int(row["time_us"]) for row in observations]),
                "psp_extent_bytes": distribution(
                    [int(row["psp_extent_bytes"]) for row in observations]
                ),
            }
        matching_deltas = [
            row for row in implementation_deltas if row["operation"] == operation
        ]
        operation_summary["paired_d1_minus_baseline"] = {
            "n": len(matching_deltas),
            "psp_delta_bytes": distribution(
                [int(row["psp_delta_bytes"]) for row in matching_deltas]
            ),
            "time_delta_percent": distribution(
                [float(row["time_delta_percent"]) for row in matching_deltas]
            ),
        }
        summary[operation] = operation_summary

    output = {
        "schema": "sqisign-v3-rp2350-multi-input-placement-v1",
        "campaign": args.capture_dir.name,
        "design": {
            "board_count": 1,
            "implementations": ["official baseline", "D1 lifetime overlay"],
            "placements": 2,
            "placement_shift_bytes": 1024,
            "vectors_per_condition": 10,
            "capture_count": 4,
            "positive_ksv_trials": 40,
            "negative_verify_trials": 40,
            "workload": "official v3 response vectors 0--9 in one boot per condition",
            "interpretation": "bounded multi-input and linked-placement robustness check; not a worst-case stack proof",
        },
        "firmware_commit": args.expected_firmware_commit,
        "official_source_commit": OFFICIAL_COMMIT,
        "d1_source_commit": args.expected_d1_commit,
        "all_trees_clean": True,
        "all_trials_passed": True,
        "address_checks": address_checks,
        "placement_psp_mismatch_count": len(placement_psp_mismatches),
        "records": records,
        "summary": summary,
        "artifacts": artifacts,
    }
    summary_path = args.capture_dir / "summary.json"
    summary_path.write_text(
        json.dumps(output, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    csv_path = args.capture_dir / "measurements.csv"
    with csv_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(
            [
                "condition",
                "kind",
                "placement",
                "vector",
                "message_bytes",
                "operation",
                "time_us",
                "psp_extent_bytes",
                "capture_sha256",
            ]
        )
        for condition in CONDITIONS:
            record = records[condition]
            for vector in record["vectors"]:
                for operation in OPERATIONS:
                    measurement = vector["operations"][operation]
                    writer.writerow(
                        [
                            condition,
                            record["kind"],
                            record["placement"],
                            vector["vector"],
                            vector["message_bytes"],
                            operation,
                            measurement["time_us"],
                            measurement["psp_extent_bytes"],
                            record["capture_sha256"],
                        ]
                    )

    if args.latex_output is not None:
        sign = summary["sign"]
        baseline_time = sign["baseline"]["time_us"]["median"] / 1_000_000
        d1_time = sign["d1"]["time_us"]["median"] / 1_000_000
        time_delta = sign["paired_d1_minus_baseline"]["time_delta_percent"]
        lines = [
            "% Generated by scripts/analyze_rp2350_v3_multi.py; do not edit.",
            r"正当なKeyGen・Sign・Verify & \num{20}/\num{20} & \num{20}/\num{20} \\",
            r"改変署名の拒否 & \num{20}/\num{20} & \num{20}/\num{20} \\",
            r"Sign PSP上書き深さ [B] & \num{101060} & \num{97132} \\",
            r"配置A/B間PSP不一致 & \num{0}/\num{40} & \num{0}/\num{40} \\",
            (
                "Sign時間の中央値 [s] & "
                + latex_num(baseline_time, 6)
                + " & "
                + latex_num(d1_time, 6)
                + r" \\") ,
            (
                "対応時間差の範囲 [\\%] & -- & "
                + latex_num(time_delta["min"], 4)
                + "--"
                + latex_num(time_delta["max"], 4)
            ),
        ]
        args.latex_output.parent.mkdir(parents=True, exist_ok=True)
        args.latex_output.write_text("\n".join(lines) + "\n", encoding="utf-8")

    print(
        "validated_conditions=4 vectors=40 negative_verifies=40 "
        "placement_psp_mismatches=0 status=PASS"
    )
    for operation in OPERATIONS:
        paired = summary[operation]["paired_d1_minus_baseline"]
        print(
            f"{operation}_psp_delta={paired['psp_delta_bytes']['median']} "
            f"time_delta_percent_median={paired['time_delta_percent']['median']:.6f} "
            f"range={paired['time_delta_percent']['min']:.6f}.."
            f"{paired['time_delta_percent']['max']:.6f}"
        )
    print(f"json={summary_path}")
    print(f"csv={csv_path}")
    if args.latex_output is not None:
        print(f"latex={args.latex_output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
