#!/usr/bin/env python3
"""Validate two boots of the frozen v2 D13 RP2350 K/S/V image.

The comparison is deliberately narrow: both captures exercise the same
deterministic input and the same archived UF2 image.  Repetition can confirm
binary/path reproducibility, but it cannot establish an input distribution or
a worst-case stack bound.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import statistics
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
KV_RE = re.compile(r"([A-Za-z0-9_]+)=([^\s]+)")
EXPECTED_BANNER = "SQISIGN_RP2350_KSV_D13 v1"
EXPECTED_FIRMWARE = "dc3289add3213cc7671f9943dfaa3bac770b2709"
EXPECTED_SOURCE = "71099e0827d3f0a3b3c705d2eda592c401e0d57d"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def unique_line(lines: list[str], prefix: str, path: Path) -> str:
    matches = [line for line in lines if line.startswith(prefix)]
    require(len(matches) == 1, f"{path}: expected one {prefix!r} line")
    return matches[0]


def fields(lines: list[str], prefix: str, path: Path) -> dict[str, str]:
    return dict(KV_RE.findall(unique_line(lines, prefix, path)))


def expect(observed: dict[str, str], expected: dict[str, str], context: str) -> None:
    for key, value in expected.items():
        require(
            observed.get(key) == value,
            f"{context}: expected {key}={value}, observed {observed.get(key)!r}",
        )


def parse_capture(path: Path) -> dict[str, object]:
    raw = path.read_bytes()
    try:
        lines = raw.replace(b"\r", b"").decode("ascii").splitlines()
    except UnicodeDecodeError as error:
        raise ValueError(f"{path}: capture is not ASCII: {error}") from error

    require(lines and lines[0] == EXPECTED_BANNER, f"{path}: missing banner")
    require(lines.count(EXPECTED_BANNER) == 1, f"{path}: duplicate banner")
    require(lines[-1:] == ["status=PASS"], f"{path}: terminal PASS is absent")
    require(not any(" alive " in line for line in lines), f"{path}: merged alive line")

    identity = fields(lines, "board=", path)
    expect(
        identity,
        {
            "board": "pico2",
            "platform": "rp2350-arm-s",
            "sdk": "2.3.0",
            "source": EXPECTED_FIRMWARE,
            "compact": EXPECTED_SOURCE,
            "dirty": "0",
        },
        str(path),
    )
    expect(
        fields(lines, "cpuid=", path),
        {"cpuid": "0x411fd210", "clock_sys_hz": "150000000"},
        str(path),
    )
    expect(
        fields(lines, "bss_end=", path),
        {
            "bss_end": "0x2004c780",
            "stack_limit": "0x20080000",
            "stack_top": "0x20082000",
        },
        str(path),
    )
    expect(
        fields(lines, "mode_ok=", path),
        {
            "mode_ok": "1",
            "arena_bytes": "172080",
            "keygen_clear": "1",
            "sign_clear": "1",
        },
        str(path),
    )

    operations: dict[str, dict[str, int]] = {}
    for operation in ("keygen", "sign"):
        row = fields(lines, f"{operation}_result=", path)
        expect(
            row,
            {
                f"{operation}_result": "0",
                "psp_reserved_bytes": "131072",
            },
            f"{path}: {operation}",
        )
        require(int(row["psp_written_bytes"]) < 131072, f"{path}: PSP overflow")
        operations[operation] = {
            "time_us": int(row[f"{operation}_us"]),
            "psp_extent_bytes": int(row["psp_written_bytes"]),
        }
        if operation == "sign":
            require(row.get("smlen") == "180", f"{path}: unexpected signed length")

    verify = fields(lines, "verify_result=", path)
    expect(
        verify,
        {
            "verify_result": "0",
            "psp_reserved_bytes": "131072",
            "verify_member_clear": "1",
            "arena_clear": "1",
            "rng_unchanged": "1",
        },
        f"{path}: verify",
    )
    require(int(verify["psp_written_bytes"]) < 131072, f"{path}: Verify PSP overflow")
    operations["verify"] = {
        "time_us": int(verify["verify_us"]),
        "psp_extent_bytes": int(verify["psp_written_bytes"]),
    }

    msp = fields(lines, "msp_reserved_bytes=", path)
    expect(
        msp,
        {"msp_reserved_bytes": "8192", "msplim": "0x20080000"},
        f"{path}: MSP",
    )
    require(int(msp["msp_written_upper_bytes"]) < 8192, f"{path}: MSP overflow")

    keygen_digest = unique_line(lines, "keygen_transcript_shake256_256=", path).split(
        "=", 1
    )[1]
    sign_digest = unique_line(lines, "sign_transcript_shake256_256=", path).split(
        "=", 1
    )[1]
    require(len(keygen_digest) == 64 and len(sign_digest) == 64, f"{path}: bad digest")
    match = fields(lines, "keygen_match=", path)
    expect(
        match,
        {
            "keygen_match": "1",
            "sign_match": "1",
            "rng_results": "0,0",
            "heap_section_bytes": "0",
        },
        str(path),
    )

    return {
        "path": str(path.relative_to(ROOT)),
        "bytes": len(raw),
        "sha256": hashlib.sha256(raw).hexdigest(),
        "operations": operations,
        "msp_extent_upper_bytes": int(msp["msp_written_upper_bytes"]),
        "keygen_transcript_shake256_256": keygen_digest,
        "sign_transcript_shake256_256": sign_digest,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--first",
        type=Path,
        default=ROOT / "results/rp2350/ksv-d13-dc3289a.txt",
    )
    parser.add_argument(
        "--second",
        type=Path,
        default=ROOT / "results/rp2350/ksv-d13-repeat-2026-09-04.txt",
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        default=ROOT / "results/rp2350/ksv-d13-dc3289a-manifest.json",
    )
    parser.add_argument(
        "--uf2",
        type=Path,
        default=ROOT
        / "results/rp2350/artifacts/ksv-d13-dc3289a/sqisign_rp2350_ksv.uf2",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=ROOT / "results/rp2350/ksv-d13-repeat-2026-09-04-summary.json",
    )
    parser.add_argument(
        "--csv-output",
        type=Path,
        default=ROOT / "results/rp2350/ksv-d13-repeat-2026-09-04-measurements.csv",
    )
    args = parser.parse_args()

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    require(manifest.get("result") == "PASS", "frozen manifest is not PASS")
    require(manifest.get("source_dirty") is False, "frozen source is dirty")
    expected_uf2 = manifest["artifacts"]["uf2_sha256"]
    require(sha256(args.uf2) == expected_uf2, "archived UF2 differs from manifest")

    captures = [parse_capture(args.first), parse_capture(args.second)]
    require(
        captures[0]["keygen_transcript_shake256_256"]
        == captures[1]["keygen_transcript_shake256_256"],
        "KeyGen transcript differs across boots",
    )
    require(
        captures[0]["sign_transcript_shake256_256"]
        == captures[1]["sign_transcript_shake256_256"],
        "Sign transcript differs across boots",
    )
    for operation in ("keygen", "sign", "verify"):
        require(
            captures[0]["operations"][operation]["psp_extent_bytes"]
            == captures[1]["operations"][operation]["psp_extent_bytes"],
            f"{operation}: PSP extent differs across deterministic boots",
        )

    timing: dict[str, object] = {}
    rows: list[dict[str, object]] = []
    for operation in ("keygen", "sign", "verify"):
        values = [int(capture["operations"][operation]["time_us"]) for capture in captures]
        timing[operation] = {
            "n": 2,
            "values_us": values,
            "min_us": min(values),
            "median_us": statistics.median(values),
            "max_us": max(values),
            "second_vs_first_percent": 100.0 * (values[1] - values[0]) / values[0],
        }
        for boot, capture in enumerate(captures, start=1):
            rows.append(
                {
                    "boot": boot,
                    "operation": operation,
                    "time_us": capture["operations"][operation]["time_us"],
                    "psp_extent_bytes": capture["operations"][operation][
                        "psp_extent_bytes"
                    ],
                }
            )

    report = {
        "schema": "tinysqisign-v2-target-repeat-v1",
        "status": "PASS",
        "binary": {
            "path": str(args.uf2.relative_to(ROOT)),
            "sha256": expected_uf2,
            "firmware_commit": EXPECTED_FIRMWARE,
            "sqisign_commit": EXPECTED_SOURCE,
        },
        "design": {
            "board_count": 1,
            "boot_count": 2,
            "distinct_input_count": 1,
            "same_archived_uf2": True,
            "deterministic_test_rng": True,
        },
        "captures": captures,
        "timing": timing,
        "decision": {
            "both_boots_passed": True,
            "transcripts_equal_across_boots": True,
            "psp_extents_equal_across_boots": True,
            "input_distribution_established": False,
            "performance_distribution_established": False,
            "worst_case_stack_bound_established": False,
        },
        "claim_boundary": (
            "Two boots reproduce one deterministic K/S/V path with the same archived "
            "UF2. This is bounded repetition evidence, not a multiple-input campaign, "
            "performance distribution, or worst-case stack proof."
        ),
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    with args.csv_output.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(
            handle, fieldnames=("boot", "operation", "time_us", "psp_extent_bytes")
        )
        writer.writeheader()
        writer.writerows(rows)
    print(
        "v2 RP2350 repeat: PASS "
        f"(boots=2, inputs=1, UF2={expected_uf2[:12]}..., PSP extents equal)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
