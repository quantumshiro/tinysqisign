#!/usr/bin/env python3
"""Validate and summarize the frozen D13 100-vector low-memory KAT."""

from __future__ import annotations

import argparse
import hashlib
import json
import platform
import re
import subprocess
from pathlib import Path


EXPECTED_D13_COMMIT = "71099e0827d3f0a3b3c705d2eda592c401e0d57d"
EXPECTED_REQUEST_SHA256 = (
    "81ff60e3ef698751e5572f0bb7f831f069605229c220ee1cf27a92572d6ebc7e"
)
LEGACY_RESPONSE_SHA256 = (
    "03898a4b415c0fd038137b1e5716be07ea7641622161aef0777035a96fc9bba8"
)
VECTOR_RE = re.compile(
    r"^KAT_VECTOR count=(\d+) keygen=PASS sign=PASS open=PASS "
    r"tamper=PASS workspace=PASS$",
    re.MULTILINE,
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


def git_output(source: Path, *args: str) -> str:
    return subprocess.check_output(
        ["git", "-C", str(source), *args], text=True
    ).strip()


def parse_records(path: Path, expected_vectors: int) -> list[dict[str, str]]:
    records: list[dict[str, str]] = []
    current: dict[str, str] = {}
    for raw_line in path.read_text(encoding="ascii").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        key, separator, value = line.partition(" = ")
        require(bool(separator), f"malformed response line in {path}: {line}")
        if key == "count" and current:
            records.append(current)
            current = {}
        require(key not in current, f"duplicate {key} in {path}")
        current[key] = value
    if current:
        records.append(current)

    require(len(records) == expected_vectors, f"{path} record count")
    expected_fields = {"count", "seed", "mlen", "msg", "pk", "sk", "smlen", "sm"}
    for index, record in enumerate(records):
        require(set(record) == expected_fields, f"{path} fields at {index}")
        require(int(record["count"]) == index, f"{path} count order at {index}")
        message_length = int(record["mlen"])
        signed_length = int(record["smlen"])
        require(message_length == 33 * (index + 1), f"{path} mlen at {index}")
        require(signed_length == message_length + 148, f"{path} smlen at {index}")
        require(len(record["seed"]) == 96, f"{path} seed size at {index}")
        require(len(record["msg"]) == 2 * message_length, f"{path} msg size at {index}")
        require(len(record["pk"]) == 2 * 65, f"{path} pk size at {index}")
        require(len(record["sk"]) == 2 * 353, f"{path} sk size at {index}")
        require(len(record["sm"]) == 2 * signed_length, f"{path} sm size at {index}")
        require(record["sm"].endswith(record["msg"]), f"{path} signed-message suffix at {index}")
    return records


def check_log(path: Path, expected_vectors: int) -> None:
    text = path.read_text(encoding="utf-8")
    counts = [int(match) for match in VECTOR_RE.findall(text)]
    require(counts == list(range(expected_vectors)), f"{path} PASS vector lines")
    expected_summary = (
        f'D13_LOWMEM_KAT {{"vectors":{expected_vectors},'
        f'"keygen":{expected_vectors},"sign":{expected_vectors},'
        f'"open":{expected_vectors},"tamper_reject":{expected_vectors},'
        f'"workspace_contract":{expected_vectors},"result":"PASS"}}'
    )
    require(expected_summary in text, f"{path} final PASS summary")


def artifact(path: Path, project_root: Path) -> dict[str, object]:
    resolved = path.resolve()
    try:
        display_path = str(resolved.relative_to(project_root.resolve()))
    except ValueError:
        display_path = str(resolved)
    return {
        "path": display_path,
        "bytes": path.stat().st_size,
        "sha256": sha256(path),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-root", type=Path, required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--official-request", type=Path, required=True)
    parser.add_argument("--legacy-response", type=Path, required=True)
    parser.add_argument("--reference-request", type=Path, required=True)
    parser.add_argument("--reference-response", type=Path, required=True)
    parser.add_argument("--lowmem-response-a", type=Path, required=True)
    parser.add_argument("--lowmem-response-b", type=Path, required=True)
    parser.add_argument("--lowmem-log-a", type=Path, required=True)
    parser.add_argument("--lowmem-log-b", type=Path, required=True)
    parser.add_argument("--reference-binary", type=Path, required=True)
    parser.add_argument("--lowmem-binary", type=Path, required=True)
    parser.add_argument("--harness-source", type=Path, required=True)
    parser.add_argument("--harness-cmake", type=Path, required=True)
    parser.add_argument("--runner", type=Path, required=True)
    parser.add_argument("--checker", type=Path, required=True)
    parser.add_argument("--reference-cache", type=Path, required=True)
    parser.add_argument("--lowmem-cache", type=Path, required=True)
    parser.add_argument("--lowmem-compile-commands", type=Path, required=True)
    parser.add_argument("--symbols", type=Path, required=True)
    parser.add_argument("--vectors", type=int, default=100)
    args = parser.parse_args()

    paths = [
        args.official_request,
        args.legacy_response,
        args.reference_request,
        args.reference_response,
        args.lowmem_response_a,
        args.lowmem_response_b,
        args.lowmem_log_a,
        args.lowmem_log_b,
        args.reference_binary,
        args.lowmem_binary,
        args.harness_source,
        args.harness_cmake,
        args.runner,
        args.checker,
        args.reference_cache,
        args.lowmem_cache,
        args.lowmem_compile_commands,
        args.symbols,
    ]
    for path in paths:
        require(path.is_file(), f"missing artifact: {path}")

    commit = git_output(args.source, "rev-parse", "HEAD")
    status = git_output(args.source, "status", "--porcelain")
    require(commit == EXPECTED_D13_COMMIT, "unexpected D13 source commit")
    require(status == "", "D13 source tree is dirty")

    require(sha256(args.official_request) == EXPECTED_REQUEST_SHA256,
            "archived official request hash")
    official_request_bytes = args.official_request.read_bytes()
    reference_request_bytes = args.reference_request.read_bytes()
    require(official_request_bytes == reference_request_bytes + b"\n",
            "generated request differs from the official request by more than its final blank line")
    require(sha256(args.legacy_response) == LEGACY_RESPONSE_SHA256,
            "archived legacy response hash")

    reference_bytes = args.reference_response.read_bytes()
    lowmem_a_bytes = args.lowmem_response_a.read_bytes()
    lowmem_b_bytes = args.lowmem_response_b.read_bytes()
    require(lowmem_a_bytes == reference_bytes,
            "D13 low-memory response differs from D13 normal reference")
    require(lowmem_b_bytes == reference_bytes,
            "D13 low-memory rerun differs from D13 normal reference")

    parse_records(args.reference_response, args.vectors)
    parse_records(args.lowmem_response_a, args.vectors)
    parse_records(args.lowmem_response_b, args.vectors)
    check_log(args.lowmem_log_a, args.vectors)
    check_log(args.lowmem_log_b, args.vectors)

    symbols_text = args.symbols.read_text(encoding="utf-8", errors="replace")
    required_symbols = [
        "sqisign_keypair_with_workspace",
        "sqisign_sign_with_workspace",
        "sqisign_open_with_workspace",
        "sqisign_verify_with_workspace",
        "protocols_keygen_with_workspace",
        "protocols_sign_with_workspace",
        "protocols_verify_with_workspace",
    ]
    for symbol in required_symbols:
        require(symbol in symbols_text, f"low-memory binary lacks {symbol}")

    reference_cache = args.reference_cache.read_text(encoding="utf-8")
    lowmem_cache = args.lowmem_cache.read_text(encoding="utf-8")
    require("CMAKE_BUILD_TYPE:STRING=Release" in reference_cache,
            "reference build type")
    require("GF_RADIX:STRING=32" in reference_cache, "reference radix")
    require("ENABLE_GMP:BOOL=OFF" in reference_cache, "reference GMP policy")
    require("SQISIGN_LOWMEM_ONLY:BOOL=OFF" in reference_cache,
            "reference API profile")
    require("CMAKE_BUILD_TYPE:STRING=Release" in lowmem_cache,
            "low-memory build type")
    require("GF_RADIX:STRING=32" in lowmem_cache, "low-memory radix")
    require("ENABLE_GMP:BOOL=OFF" in lowmem_cache, "low-memory GMP policy")
    require("SQISIGN_LOWMEM_ONLY:BOOL=ON" in lowmem_cache,
            "low-memory API profile")
    compile_commands = args.lowmem_compile_commands.read_text(encoding="utf-8")
    require("d13_lowmem_kat.c" in compile_commands,
            "low-memory harness compile command")
    for definition in [
        "-DENABLE_SIGN",
        "-DRADIX_32",
        "-DSQISIGN_BUILD_TYPE_REF",
        "-DSQISIGN_LOWMEM_ONLY",
    ]:
        require(definition in compile_commands,
                f"low-memory compile definition {definition}")

    response_hash = sha256(args.reference_response)
    legacy_match = response_hash == LEGACY_RESPONSE_SHA256
    result = {
        "schema": "sqisign-d13-lowmem-kat-evidence-v1",
        "result": "PASS",
        "vectors": args.vectors,
        "parameter_set": "SQIsign Level I",
        "radix": 32,
        "source": {
            "checkpoint": "D13",
            "commit": commit,
            "tree_clean": True,
        },
        "method": {
            "request_schedule": "archived official NIST-v2 100-vector request",
            "oracle": "same-commit D13 normal encoded API",
            "implementation_under_test": "D13 caller-owned low-memory encoded KeyGen/Sign/Open/Verify API",
            "checks_per_vector": [
                "byte-identical response against D13 normal API",
                "valid signed-message open and message equality",
                "one deterministic signature-bit corruption rejected",
                "workspace guards intact",
                "owned workspace cleared after every operation",
            ],
            "independent_lowmem_runs": 2,
        },
        "claim_boundary": {
            "lowmem_input_official_request_exact_hash": True,
            "reference_request_records_match": True,
            "reference_request_byte_match": False,
            "reference_request_serialization_difference": "The current generator omits the archived request's one final blank-line LF; all 100 count/seed/mlen/msg records are byte-identical.",
            "d13_normal_lowmem_byte_match": True,
            "lowmem_rerun_byte_match": True,
            "legacy_nist_v2_response_match": legacy_match,
            "legacy_response_is_oracle": False,
            "reason": "Compact fixed-precision KeyGen intentionally consumes a different random transcript from the legacy GMP implementation.",
        },
        "host": {
            "platform": platform.platform(),
            "python": platform.python_version(),
        },
        "artifacts": {
            "official_request": artifact(args.official_request, args.project_root),
            "legacy_response": artifact(args.legacy_response, args.project_root),
            "reference_request": artifact(args.reference_request, args.project_root),
            "reference_response": artifact(args.reference_response, args.project_root),
            "lowmem_response_run_1": artifact(args.lowmem_response_a, args.project_root),
            "lowmem_response_run_2": artifact(args.lowmem_response_b, args.project_root),
            "lowmem_log_run_1": artifact(args.lowmem_log_a, args.project_root),
            "lowmem_log_run_2": artifact(args.lowmem_log_b, args.project_root),
            "reference_binary": artifact(args.reference_binary, args.project_root),
            "lowmem_binary": artifact(args.lowmem_binary, args.project_root),
            "harness_source": artifact(args.harness_source, args.project_root),
            "harness_cmake": artifact(args.harness_cmake, args.project_root),
            "runner": artifact(args.runner, args.project_root),
            "checker": artifact(args.checker, args.project_root),
            "reference_cache": artifact(args.reference_cache, args.project_root),
            "lowmem_cache": artifact(args.lowmem_cache, args.project_root),
            "lowmem_compile_commands": artifact(args.lowmem_compile_commands, args.project_root),
            "lowmem_symbols": artifact(args.symbols, args.project_root),
        },
    }
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
