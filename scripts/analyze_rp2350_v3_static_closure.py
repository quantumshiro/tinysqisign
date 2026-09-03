#!/usr/bin/env python3
"""Bind the v3 static-PSP certificate to a clean RP2350 K/S/V capture."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path


KV_RE = re.compile(r"([A-Za-z0-9_]+)=([^\s]+)")
OPERATIONS = ("keygen", "sign", "verify")
ROOT_FOR_OPERATION = {
    "keygen": "keygen_thunk",
    "sign": "sign_thunk",
    "verify": "verify_thunk",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def fields_for(lines: list[str], prefix: str) -> dict[str, str]:
    matches = [line for line in lines if line.startswith(prefix)]
    require(len(matches) == 1, f"expected one line beginning {prefix!r}")
    return dict(KV_RE.findall(matches[0]))


def cache_values(path: Path) -> dict[str, str]:
    result: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line or line.startswith(("#", "//")) or "=" not in line:
            continue
        key_and_type, value = line.split("=", 1)
        result[key_and_type.split(":", 1)[0]] = value
    return result


def file_record(path: Path) -> dict[str, object]:
    require(path.is_file(), f"missing artifact: {path}")
    return {"filename": path.name, "bytes": path.stat().st_size, "sha256": sha256(path)}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture", type=Path, required=True)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--stack-bound", type=Path, required=True)
    parser.add_argument("--async-stack-audit", type=Path, required=True)
    parser.add_argument("--elf-audit", type=Path, required=True)
    parser.add_argument("--uf2-info", type=Path, required=True)
    parser.add_argument("--size-tool", type=Path, required=True)
    parser.add_argument("--expected-firmware-commit", required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()

    cache_path = args.build_dir / "CMakeCache.txt"
    cache = cache_values(cache_path)
    require(cache.get("SQISIGN_FIRMWARE_DIRTY") == "0", "firmware tree is dirty")
    require(cache.get("SQISIGN_V3_SOURCE_DIRTY") == "0", "v3 source tree is dirty")
    require(
        cache.get("SQISIGN_FIRMWARE_GIT_COMMIT") == args.expected_firmware_commit,
        "unexpected firmware commit",
    )
    require(cache.get("SQISIGN_V3_REQUIRE_STATIC_STACK") == "ON", "static stack mode is off")

    stack_bound = json.loads(args.stack_bound.read_text(encoding="utf-8"))
    require(stack_bound.get("status") == "PSP_BOUND_ESTABLISHED", "PSP bound is absent")
    require(
        stack_bound.get("schema") == "sqisign-v3-linked-stack-bound-audit-v4",
        "unexpected PSP-bound schema",
    )
    require(
        stack_bound["build_provenance"]["firmware_commit"]
        == args.expected_firmware_commit,
        "PSP bound and capture firmware commits differ",
    )
    require(
        stack_bound["build_provenance"].get("pico_platform")
        == "rp2350-arm-s",
        "PSP bound does not identify the Secure Arm platform",
    )
    require(
        stack_bound["decision"]["operation_psp_bounds_established"] is True,
        "operation PSP bounds were not established",
    )
    require(
        stack_bound["architectural_psp_exception_certificate"][
            "psp_allowance_bytes"
        ]
        == 212
        and stack_bound["architectural_psp_exception_certificate"][
            "linked_psp_write_sites_confined_to_trampoline"
        ]
        is True,
        "architectural PSP exception certificate changed",
    )
    require(
        stack_bound["decision"]["whole_program_worst_case_stack_bound_established"]
        is False,
        "this analyzer must not promote the PSP result to a whole-program bound",
    )

    async_stack = json.loads(args.async_stack_audit.read_text(encoding="utf-8"))
    require(
        async_stack.get("schema") == "sqisign-v3-async-stack-closure-audit-v1"
        and async_stack.get("status") == "PARTIAL_BLOCKERS_ENUMERATED",
        "unexpected asynchronous stack-audit result",
    )
    require(
        async_stack["build_provenance"]["firmware_commit"]
        == args.expected_firmware_commit,
        "asynchronous audit and capture firmware commits differ",
    )
    require(
        async_stack["elf"]["sha256"] == stack_bound["elf"]["sha256"],
        "synchronous and asynchronous audits cover different ELFs",
    )
    require(
        async_stack["aggregate"]["unique_missing_stack_metadata_count"] == 0
        and async_stack["aggregate"]["unique_unresolved_indirect_callsite_count"]
        == 18,
        "asynchronous blocker inventory changed",
    )
    require(
        async_stack["decision"]["whole_program_worst_case_stack_bound_established"]
        is False,
        "asynchronous audit must retain the whole-program boundary",
    )

    lines = args.capture.read_text(encoding="ascii").splitlines()
    require(lines and lines[0] == "SQISIGN_RP2350_V3_D2_STATIC v1", "wrong capture banner")
    scheme = fields_for(lines, "scheme=")
    require(
        scheme.get("scheme") == "SQIsign-v3.0"
        and scheme.get("variant") == "p324_3"
        and scheme.get("implementation") == "m4f"
        and scheme.get("image") == "d2-static",
        "wrong scheme identity",
    )
    board = fields_for(lines, "board=")
    require(
        board.get("board") == "pico2"
        and board.get("platform") == "rp2350-arm-s"
        and board.get("sdk") == "2.3.0",
        "wrong target identity",
    )
    require(board.get("firmware") == args.expected_firmware_commit, "capture commit mismatch")
    require(board.get("firmware_dirty") == "0", "capture firmware was dirty")
    source = fields_for(lines, "v3_source=")
    require(
        source.get("v3_dirty") == "0"
        and source.get("cpuid") == "0x411fd210"
        and source.get("clock_sys_hz") == "150000000",
        "capture source or processor identity changed",
    )
    require(
        source.get("v3_source") == cache.get("SQISIGN_V3_SOURCE_COMMIT"),
        "capture/build v3 source mismatch",
    )
    require(
        fields_for(lines, "v3_generated_tree_sha256=").get("v3_generated_tree_sha256")
        == cache.get("SQISIGN_V3_GENERATED_TREE_SHA256"),
        "capture/build generated-tree digest mismatch",
    )
    stack_layout = fields_for(lines, "bss_end=")
    require(
        stack_layout.get("stack_limit") == "0x20080000"
        and stack_layout.get("stack_top") == "0x20082000",
        "capture stack reservation changed",
    )
    decoded = fields_for(lines, "kat_decoded=")
    require(
        decoded.get("kat_decoded") == "1"
        and decoded.get("mode_ok") == "1"
        and decoded.get("heap_section_bytes") == "0",
        "capture precondition failed",
    )

    operations: dict[str, object] = {}
    for operation in OPERATIONS:
        fields = fields_for(lines, f"{operation}_result=")
        require(
            fields.get(f"{operation}_result") == "0"
            and fields.get(f"{operation}_match") == "1",
            f"{operation} failed or did not match the official vector",
        )
        observed = int(fields["psp_written_bytes"])
        static_record = stack_bound["static_psp_bounds"][
            ROOT_FOR_OPERATION[operation]
        ]
        bound = int(static_record["bound_bytes"])
        reservation = int(fields["psp_reserved_bytes"])
        require(observed <= bound <= reservation, f"{operation}: PSP inequality failed")
        operations[operation] = {
            "elapsed_us": int(fields[f"{operation}_us"]),
            "observed_psp_bytes": observed,
            "software_call_bound_bytes": int(
                static_record["software_call_bound_bytes"]
            ),
            "exception_entry_allowance_bytes": int(
                static_record["exception_entry_allowance_bytes"]
            ),
            "static_psp_bound_bytes": bound,
            "bound_minus_observed_bytes": bound - observed,
            "reservation_bytes": reservation,
            "reservation_margin_bytes": reservation - bound,
        }
    msp = fields_for(lines, "msp_reserved_bytes=")
    require(
        msp.get("msp_reserved_bytes") == "8192"
        and msp.get("msplim") == "0x20080000"
        and int(msp["msp_written_upper_bytes"]) < 8192,
        "observed MSP reservation or canary extent is invalid",
    )
    require(fields_for(lines, "status=").get("status") == "PASS", "capture did not terminate PASS")

    target = args.build_dir / "sqisign_rp2350_v3_d2_static"
    artifacts = {
        "capture": file_record(args.capture),
        "elf": file_record(target.with_suffix(".elf")),
        "uf2": file_record(target.with_suffix(".uf2")),
        "map": file_record(Path(str(target) + ".elf.map")),
        "archive": file_record(args.build_dir / "libsqisign_v3_p324_3_m4f.a"),
        "elf_audit": file_record(args.elf_audit),
        "uf2_info": file_record(args.uf2_info),
        "stack_bound": file_record(args.stack_bound),
        "async_stack_audit": file_record(args.async_stack_audit),
        "cmake_cache": file_record(cache_path),
    }
    audit_text = args.elf_audit.read_text(encoding="utf-8")
    for needle in (
        "ELF audit PASS",
        "dynamic_stack_records=0",
        "heap_section_bytes=0",
    ):
        require(needle in audit_text, f"ELF audit lacks {needle!r}")
    uf2_info_text = args.uf2_info.read_text(encoding="utf-8")
    for needle in (
        "family ID 'rp2350-arm-s'",
        "target chip:         RP2350",
        "image type:          ARM Secure",
        "sdk version:         2.3.0",
        "pico_board:          pico2",
        "build attributes:    Release",
    ):
        require(needle in uf2_info_text, f"UF2 metadata lacks {needle!r}")

    size_lines = subprocess.run(
        [str(args.size_tool), str(target.with_suffix(".elf"))],
        check=True,
        text=True,
        capture_output=True,
    ).stdout.splitlines()
    size_fields = size_lines[-1].split()
    require(len(size_fields) >= 3, "malformed size output")

    output = {
        "schema": "sqisign-v3-rp2350-static-psp-closure-v1",
        "status": "PASS",
        "campaign": args.output_dir.name,
        "provenance": {
            "firmware_commit": args.expected_firmware_commit,
            "firmware_dirty": 0,
            "v3_source_commit": source["v3_source"],
            "v3_source_dirty": 0,
            "generated_tree_sha256": cache["SQISIGN_V3_GENERATED_TREE_SHA256"],
        },
        "linked_size": {
            "text_bytes": int(size_fields[0]),
            "data_bytes": int(size_fields[1]),
            "bss_bytes": int(size_fields[2]),
        },
        "validation": {
            "official_vector_0_ksv_passed": True,
            "operation_psp_bounds_established": True,
            "maximum_exception_entry_charged_to_psp": True,
            "observed_psp_within_static_bounds": True,
            "compiler_dynamic_stack_records": 0,
            "heap_section_bytes": 0,
            "msp_canary_within_reservation": True,
            "observed_msp_upper_bytes": int(msp["msp_written_upper_bytes"]),
            "all_synchronous_operation_metadata_and_calls_closed": True,
            "async_candidate_roots_audited": 4,
            "async_direct_call_missing_stack_metadata": 0,
            "async_unresolved_indirect_callback_sites": 18,
            "whole_program_worst_case_stack_bound_established": False,
        },
        "operations": operations,
        "artifacts": artifacts,
        "claim_boundary": (
            "The clean p324_3/RADIX32 D2 image passed one official vector-0 K/S/V path, "
            "and every observed Thread-mode PSP extent was below its linked conservative "
            "software-call bound plus one maximum Secure Armv8-M exception-entry frame. "
            "The result is image-specific and excludes handler call chains and asynchronous "
            "interrupt/MSP nesting; the companion "
            "audit leaves 18 unique indirect callback sites unresolved. It also excludes "
            "other parameter sets and side-channel resistance."
        ),
    }
    args.output_dir.mkdir(parents=True, exist_ok=True)
    output_path = args.output_dir / "summary.json"
    output_path.write_text(json.dumps(output, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        "v3 static PSP closure: PASS "
        + " ".join(
            f"{operation}={operations[operation]['static_psp_bound_bytes']}B"
            for operation in OPERATIONS
        )
        + " whole_program_bound=false"
    )
    print(f"summary={output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
