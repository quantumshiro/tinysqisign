#!/usr/bin/env python3
"""Validate the frozen D13 lifetime certificate and its source anchors."""

from __future__ import annotations

import csv
import json
import re
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TREE = ROOT / "work/compact-d13"
CERTIFICATE = ROOT / "results/revision-2026-09-04/lifetime-certificate-v2.csv"
OUTPUT = ROOT / "results/revision-2026-09-04/lifetime-certificate-check.json"
COMMIT = "71099e0827d3f0a3b3c705d2eda592c401e0d57d"


def command(*args: str) -> str:
    return subprocess.check_output(args, cwd=ROOT, text=True).strip()


def require(condition: bool, message: str, checks: list[dict[str, str]]) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")
    checks.append({"check": message, "status": "PASS"})


def compact(text: str) -> str:
    return re.sub(r"\s+", "", text)


def has_all(text: str, snippets: list[str]) -> bool:
    normalized = compact(text)
    return all(compact(item) in normalized for item in snippets)


def main() -> int:
    checks: list[dict[str, str]] = []
    require(command("git", "-C", str(TREE), "rev-parse", "HEAD") == COMMIT,
            "D13 commit is frozen", checks)
    require(command("git", "-C", str(TREE), "status", "--porcelain") == "",
            "D13 worktree is clean", checks)

    with CERTIFICATE.open(encoding="utf-8", newline="") as handle:
        rows = list(csv.DictReader(handle))
    required_columns = {
        "object_name", "type", "size_bytes", "alignment_bytes", "arena_offset",
        "construct_site", "last_reference_site", "forbidden_overlap",
        "pointer_pass_and_escape", "normal_exit_clear", "failure_exit_clear",
        "retry_exit_clear", "static_or_runtime_check", "evidence_boundary",
    }
    require(set(rows[0]) == required_columns, "certificate has every required audit column", checks)
    require(len(rows) == 11, "certificate has eleven principal lifetime records", checks)
    require(all(all(row[column].strip() for column in required_columns) for row in rows),
            "certificate has no empty required field", checks)
    require(len({row["object_name"] for row in rows}) == len(rows),
            "certificate object names are unique", checks)

    layout = command("scripts/check_certified_norm_sketch_layout.sh")
    expected_layout = [
        "arm_candidate_bytes=14024", "arm_phase_bytes=94912",
        "arm_find_uv_bytes=172080", "arm_operation_bytes=172080",
        "certified norm-sketch layout PASS",
    ]
    require(all(item in layout for item in expected_layout),
            "Arm layout probe reproduces certified extents", checks)

    id_header = (TREE / "src/id2iso/ref/include/id2iso.h").read_text()
    signature_header = (TREE / "src/signature/ref/include/signature_lowmem.h").read_text()
    verify_header = (TREE / "src/verification/ref/include/verification_lowmem.h").read_text()
    keygen = (TREE / "src/signature/ref/lvlx/keygen_lowmem.c").read_text()
    sign = (TREE / "src/signature/ref/lvlx/sign.c").read_text()
    verify = (TREE / "src/verification/ref/lvlx/verify.c").read_text()
    finduv = (TREE / "src/id2iso/ref/lvlx/dim2id2iso.c").read_text()
    api = (TREE / "src/sqisign_lowmem.c").read_text()

    require(has_all(id_header, [
        "typedef struct find_uv_workspace", "find_uv_lattice_state_t lattice_state",
        "find_uv_phase_workspace_t phase", "must not alias any input or output object",
    ]), "find_uv ownership and non-alias contract is present", checks)
    require(has_all(signature_header, [
        "typedef union protocols_keygen_workspace", "typedef union protocols_sign_workspace",
        "typedef union protocols_operation_workspace", "At most one member may be active",
    ]), "top-level operation unions and ownership contract are present", checks)
    require(has_all(verify_header, [
        "typedef union protocols_verify_workspace", "ec_eval_even_workspace_t even_isogeny",
        "theta_chain_workspace_t theta", "securely cleared on every return",
    ]), "Verify phase union and clearing contract are present", checks)

    require(has_all(keygen, [
        "sizeof(protocols_keygen_workspace_t) == 172080",
        "_Alignof(protocols_keygen_workspace_t) == 8",
        "offsetof(protocols_keygen_workspace_t, find_uv) == 0",
        "offsetof(protocols_keygen_workspace_t, dlog) == 0",
        "sizeof(find_uv_lattice_state_t) == 77168",
        "offsetof(find_uv_workspace_t, phase) == 77168",
        "sizeof(find_uv_candidate_workspace_t) == 14024",
        "sizeof(find_uv_phase_workspace_t) == 94912",
    ]), "KeyGen/find_uv target static assertions are present", checks)
    require(has_all(sign, [
        "sizeof(protocols_sign_workspace_t) == 172080",
        "_Alignof(protocols_sign_workspace_t) == 8",
        "offsetof(protocols_sign_workspace_t, find_uv) == 0",
        "offsetof(protocols_sign_workspace_t, even_isogeny) == 0",
        "protocols_sign_workspace_clear(workspace)",
    ]), "Sign union offsets and terminal whole-owner clear are present", checks)
    require(has_all(verify, [
        "sizeof(protocols_verify_workspace_t) == 15428",
        "offsetof(protocols_verify_workspace_t, even_isogeny) == 0",
        "offsetof(protocols_verify_workspace_t, theta) == 0",
        "protocols_verify_workspace_clear(workspace)",
    ]), "Verify union offsets and terminal clear are present", checks)
    require(has_all(api, [
        "sizeof(protocols_operation_workspace_t) == 172080",
        "offsetof(protocols_operation_workspace_t, keygen) == 0",
        "offsetof(protocols_operation_workspace_t, sign) == 0",
        "offsetof(protocols_operation_workspace_t, verify) == 0",
    ]), "encoded API operation union layout is present", checks)
    require(has_all(finduv, [
        "sqisign_secure_clear(&workspace->phase, sizeof(workspace->phase))",
        "cleanup_workspace_fatal:", "result = ID2ISO_STATUS_FATAL",
        "find_uv_workspace_clear(workspace)", "return result",
    ]), "find_uv phase transition and terminal failure clear are present", checks)

    retry_body_match = re.search(r"while\s*\(!ret\)\s*\{(.*?)\n\s*\}\n\n\s*// Set to the signature", sign, re.S)
    require(retry_body_match is not None, "Sign retry loop is located", checks)
    retry_body = retry_body_match.group(1)
    require("protocols_sign_workspace_clear" not in retry_body,
            "certificate correctly states no whole-owner clear inside each Sign retry", checks)

    selected_sources = "\n".join([keygen, sign, verify, finduv, api])
    obvious_static_pointer = re.compile(
        r"\bstatic\s+(?:const\s+)?[A-Za-z_][A-Za-z0-9_\s]*\*\s*"
        r"[A-Za-z0-9_]*(?:workspace|arena)[A-Za-z0-9_]*\s*(?:=|;)",
        re.I,
    )
    require(obvious_static_pointer.search(selected_sources) is None,
            "bounded scan finds no obvious global/static workspace-pointer sink", checks)

    output = {
        "schema": "sqisign-lifetime-certificate-check-v1",
        "status": "PASS",
        "commit": COMMIT,
        "record_count": len(rows),
        "scope": (
            "schema, frozen source anchors, Arm layout, terminal clearing, and "
            "obvious pointer-sink scan; not sound whole-program alias analysis"
        ),
        "checks": checks,
    }
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(json.dumps(output, indent=2) + "\n", encoding="utf-8")
    print(f"lifetime certificate: PASS ({len(checks)} checks, {len(rows)} records)")
    print(OUTPUT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
