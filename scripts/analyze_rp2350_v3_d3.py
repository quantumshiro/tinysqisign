#!/usr/bin/env python3
"""Validate one clean D3 RP2350 capture and bind it to archived artifacts."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE_COMMIT = "874658c64aa2e20f53b1f4d696144723d558ed5c"
OPERATIONS = ("keygen", "sign", "verify")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def file_record(path: Path) -> dict[str, object]:
    require(path.is_file(), f"missing artifact: {path}")
    return {
        "filename": path.name,
        "bytes": path.stat().st_size,
        "sha256": sha256(path),
    }


def fields(line: str) -> dict[str, str]:
    parsed: dict[str, str] = {}
    for token in line.split():
        if "=" in token:
            key, value = token.split("=", 1)
            parsed[key] = value
    return parsed


def generated_tree_digest(root: Path) -> str:
    records = bytearray()
    for path in sorted(item for item in root.rglob("*") if item.is_file()):
        relative = "./" + path.relative_to(root).as_posix()
        records.extend(f"{sha256(path)}  {relative}\n".encode("ascii"))
    require(bool(records), f"empty generated tree: {root}")
    return hashlib.sha256(records).hexdigest()


def elf_size(size_tool: Path, elf: Path) -> dict[str, int]:
    output = subprocess.run(
        [str(size_tool), str(elf)], check=True, text=True, capture_output=True
    ).stdout.splitlines()
    require(len(output) == 2, "unexpected size output")
    values = output[1].split()
    require(len(values) >= 6, "unexpected size row")
    return {"text_bytes": int(values[0]), "data_bytes": int(values[1]), "bss_bytes": int(values[2])}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture_dir", type=Path)
    parser.add_argument("--build-root", type=Path, default=ROOT / "build-rp2350-v3-d3")
    parser.add_argument("--source-root", type=Path, default=ROOT / "work/v3-lowmem-d3")
    parser.add_argument("--expected-firmware-commit", required=True)
    parser.add_argument(
        "--official-summary",
        type=Path,
        default=ROOT / "results/v3/rp2350/interleaved-clean-2026-09-04/summary.json",
    )
    parser.add_argument(
        "--host-validation",
        type=Path,
        default=ROOT / "results/v3/host/validation-all-params-2026-09-04.json",
    )
    parser.add_argument(
        "--frame-audit",
        type=Path,
        default=ROOT / "results/v3/analysis/lifetime-all-params-2026-09-04.json",
    )
    parser.add_argument(
        "--lifetime-check",
        type=Path,
        default=ROOT / "results/v3/analysis/two-function-lifetime-check.json",
    )
    parser.add_argument(
        "--size-tool",
        type=Path,
        default=Path("arm-none-eabi-size"),
    )
    args = parser.parse_args()

    capture = args.capture_dir / "capture.txt"
    lines = capture.read_text(encoding="ascii").splitlines()
    require(lines and lines[0] == "SQISIGN_RP2350_V3_D3 v1", "wrong capture banner")
    require(lines[-1] == "status=PASS", "capture did not terminate PASS")
    require(len(lines) == 12, f"unexpected capture line count: {len(lines)}")

    identity = fields(lines[1])
    board = fields(lines[2])
    source = fields(lines[3])
    source_hash = fields(lines[4])
    memory = fields(lines[5])
    precondition = fields(lines[6])
    operation_lines = {name: fields(lines[7 + index]) for index, name in enumerate(OPERATIONS)}
    msp = fields(lines[10])

    require(identity == {"scheme": "SQIsign-v3.0", "variant": "p324_3", "implementation": "m4f", "image": "d3"}, "wrong image identity")
    require(board.get("firmware") == args.expected_firmware_commit, "firmware commit mismatch")
    require(board.get("firmware_dirty") == "0", "firmware tree was dirty")
    require(source.get("v3_source") == SOURCE_COMMIT, "v3 source commit mismatch")
    require(source.get("v3_dirty") == "0", "v3 source tree was dirty")
    require(source.get("cpuid") == "0x411fd210", "unexpected processor")
    require(source.get("clock_sys_hz") == "150000000", "unexpected clock")
    require(precondition.get("kat_decoded") == "1" and precondition.get("mode_ok") == "1", "KAT precondition failed")
    require(precondition.get("heap_section_bytes") == "0", "heap section is nonzero")

    generated_root = args.source_root / "src/pqm4/sqisign_p324_3/m4f"
    source_head = subprocess.run(
        ["git", "-C", str(args.source_root), "rev-parse", "HEAD"],
        check=True,
        text=True,
        capture_output=True,
    ).stdout.strip()
    source_dirty = subprocess.run(
        ["git", "-C", str(args.source_root), "status", "--porcelain", "--untracked-files=no"],
        check=True,
        text=True,
        capture_output=True,
    ).stdout.strip()
    require(source_head == SOURCE_COMMIT and not source_dirty, "current v3 source checkout differs")
    expected_generated_hash = generated_tree_digest(generated_root)
    require(source_hash.get("v3_generated_tree_sha256") == expected_generated_hash, "generated source digest mismatch")

    expected_results = {"keygen": "0", "sign": "0", "verify": "0"}
    operations: dict[str, dict[str, int]] = {}
    for name in OPERATIONS:
        record = operation_lines[name]
        require(record.get(f"{name}_result") == expected_results[name], f"{name} failed")
        require(record.get(f"{name}_match") == "1", f"{name} did not match official response")
        require(record.get("psp_reserved_bytes") == "131072", f"{name} reservation changed")
        observed = int(record["psp_written_bytes"])
        require(0 < observed <= 131072, f"{name} PSP watermark out of bounds")
        operations[name] = {
            "elapsed_us": int(record[f"{name}_us"]),
            "observed_psp_bytes": observed,
            "reservation_bytes": 131072,
            "reservation_margin_bytes": 131072 - observed,
        }
    require(int(msp["msp_written_upper_bytes"]) <= int(msp["msp_reserved_bytes"]), "MSP canary exceeded reservation")

    official = json.loads(args.official_summary.read_text(encoding="utf-8"))
    host = json.loads(args.host_validation.read_text(encoding="utf-8"))
    frames = json.loads(args.frame_audit.read_text(encoding="utf-8"))
    lifetime = json.loads(args.lifetime_check.read_text(encoding="utf-8"))
    require(official.get("all_captures_passed") is True, "official comparison campaign failed")
    require(host.get("status") == "PASS", "all-parameter host validation failed")
    require(frames.get("status") == "PASS", "all-parameter frame audit failed")
    require(lifetime.get("status") == "PASS", "lifetime certificate check failed")

    baseline_psp = int(official["summary"]["sign"]["official"]["psp_extent_bytes"]["median"])
    d1_psp = int(official["summary"]["sign"]["d1"]["psp_extent_bytes"]["median"])
    d3_psp = operations["sign"]["observed_psp_bytes"]
    require(baseline_psp == 101060 and d1_psp == 97132, "comparison campaign changed")
    require(d3_psp < d1_psp < baseline_psp, "D3 did not improve the target watermark")

    artifact_root = args.capture_dir / "artifacts/d3"
    artifact_paths = {
        "elf": artifact_root / "sqisign_rp2350_v3_d3.elf",
        "uf2": artifact_root / "sqisign_rp2350_v3_d3.uf2",
        "map": artifact_root / "sqisign_rp2350_v3_d3.elf.map",
        "archive": artifact_root / "libsqisign_v3_p324_3_m4f.a",
        "capture": capture,
        "elf_audit": args.capture_dir / "elf-audit.txt",
        "uf2_info": args.capture_dir / "uf2-info.txt",
        "host_validation": args.host_validation,
        "frame_audit": args.frame_audit,
        "lifetime_check": args.lifetime_check,
        "source_patch": ROOT / "patches/0037-experiment-v3-two-function-lifetime.patch",
        "source_bundle": ROOT / "patches/v3-two-function-lifetime.bundle",
    }
    for name in ("elf", "uf2", "map", "archive"):
        built = args.build_root / artifact_paths[name].name
        require(sha256(artifact_paths[name]) == sha256(built), f"archived {name} differs from build")

    result = {
        "schema": "sqisign-v3-rp2350-d3-two-function-v1",
        "status": "PASS",
        "campaign": args.capture_dir.name,
        "provenance": {
            "firmware_commit": args.expected_firmware_commit,
            "firmware_dirty": 0,
            "v3_source_commit": SOURCE_COMMIT,
            "v3_source_dirty": 0,
            "generated_tree_sha256": expected_generated_hash,
        },
        "linked_size": elf_size(args.size_tool, artifact_paths["elf"]),
        "operations": operations,
        "msp": {
            "observed_upper_bytes": int(msp["msp_written_upper_bytes"]),
            "reservation_bytes": int(msp["msp_reserved_bytes"]),
        },
        "comparison": {
            "official_sign_psp_bytes": baseline_psp,
            "one_function_d1_sign_psp_bytes": d1_psp,
            "two_function_d3_sign_psp_bytes": d3_psp,
            "d3_reduction_from_official_bytes": baseline_psp - d3_psp,
            "d3_reduction_from_official_percent": round(100.0 * (baseline_psp - d3_psp) / baseline_psp, 4),
            "second_function_incremental_reduction_bytes": d1_psp - d3_psp,
            "timing_comparison_claimed": False,
        },
        "validation": {
            "official_vector_0_ksv_passed_on_rp2350": True,
            "all_three_parameter_sets_host_api_selftest_and_official_100_vector_kat_passed": True,
            "known_answer_vectors_passed_across_two_implementations": int(host["decision"]["known_answer_vectors_passed"]),
            "two_distinct_functions_lifetime_scheduled": True,
            "all_six_affected_arm_frames_reduced": True,
            "rp2350_parameter_sets_measured": ["p324_3"],
            "whole_program_worst_case_stack_bound_established": False,
            "side_channel_resistance_established": False,
        },
        "artifacts": {name: file_record(path) for name, path in artifact_paths.items()},
        "claim_boundary": (
            "D3 extends the v3 transfer from one to two functions. Host API, self-test, and official-response KAT checks cover all three official parameter sets; Arm frame audits cover both functions for all three sets. The clean RP2350 measurement covers only p324_3 and one deterministic vector-0 K/S/V boot. It is not a whole-program interrupt/MSP bound or a timing-distribution result."
        ),
    }
    output = args.capture_dir / "summary.json"
    output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print("v3 D3 clean RP2350 evidence: PASS")
    print(f"sign_psp={baseline_psp}->{d3_psp} reduction={baseline_psp - d3_psp}")
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
