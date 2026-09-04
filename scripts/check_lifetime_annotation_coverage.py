#!/usr/bin/env python3
"""Fail closed when the v2 arena model omits a declared or accessed member.

This is a coverage gate for the human-written phase contract.  It does not
infer lifetimes or prove C alias semantics; it prevents a direct workspace
field from being silently absent from the annotation supplied to the layout
generator.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TREE = ROOT / "work/compact-d13"
SOURCE_COMMIT = "71099e0827d3f0a3b3c705d2eda592c401e0d57d"
HEADER = TREE / "src/id2iso/ref/include/id2iso.h"
SOURCE = TREE / "src/id2iso/ref/lvlx/dim2id2iso.c"
ANNOTATION = ROOT / "experiments/memory/finduv-lifetime-layout.json"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def git(*args: str) -> str:
    return subprocess.run(
        ["git", "-C", str(TREE), *args],
        check=True,
        text=True,
        capture_output=True,
    ).stdout.strip()


def typedef_body(text: str, kind: str, tag: str) -> str:
    match = re.search(
        rf"typedef\s+{kind}\s+{re.escape(tag)}\s*\{{(?P<body>.*?)\}}\s*"
        rf"{re.escape(tag)}_t\s*;",
        text,
        re.S,
    )
    if not match:
        raise ValueError(f"could not locate typedef {kind} {tag}")
    return match.group("body")


def declared_members(body: str) -> list[str]:
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    body = re.sub(r"//[^\n]*", "", body)
    members: list[str] = []
    for statement in body.split(";"):
        identifiers = re.findall(r"[A-Za-z_][A-Za-z0-9_]*", statement)
        if identifiers:
            members.append(identifiers[-1])
    return members


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        type=Path,
        default=ROOT
        / "results/revision-2026-09-04/lifetime-annotation-coverage.json",
    )
    args = parser.parse_args()

    if git("rev-parse", "HEAD") != SOURCE_COMMIT:
        raise ValueError("unexpected v2 source commit")
    if git("status", "--porcelain", "--untracked-files=no"):
        raise ValueError("tracked v2 source is dirty")

    annotation = json.loads(ANNOTATION.read_text(encoding="utf-8"))
    header = HEADER.read_text(encoding="utf-8")
    source = SOURCE.read_text(encoding="utf-8")

    struct_members = declared_members(typedef_body(header, "struct", "find_uv_workspace"))
    union_members = declared_members(typedef_body(header, "union", "find_uv_phase_workspace"))
    if struct_members != ["lattice_state", "phase"]:
        raise ValueError(f"find_uv workspace members changed: {struct_members}")
    if union_members != ["lattice_mul", "candidates", "fixed_degree"]:
        raise ValueError(f"find_uv phase members changed: {union_members}")

    declared_paths = {"lattice_state"} | {f"phase.{member}" for member in union_members}
    configured_paths = set(annotation["existing_c_layout"]["members"].values())
    if configured_paths != declared_paths:
        raise ValueError(
            f"annotation member inventory differs: declared={declared_paths}, "
            f"configured={configured_paths}"
        )

    # Capture every direct access rooted at the two find_uv top-level fields.
    # The longest dotted prefix determines the annotated physical object.
    access_pattern = re.compile(
        r"workspace->(?:lattice_state|phase)"
        r"(?:\.[A-Za-z_][A-Za-z0-9_]*)*"
    )
    accesses = sorted(set(access_pattern.findall(source)))
    if not accesses:
        raise ValueError("no direct find_uv workspace accesses found")

    classified: dict[str, list[str]] = {path: [] for path in sorted(declared_paths)}
    whole_phase_accesses: list[str] = []
    unclassified: list[str] = []
    for access in accesses:
        relative = access.removeprefix("workspace->")
        if relative == "phase":
            whole_phase_accesses.append(access)
            continue
        matches = [
            path
            for path in declared_paths
            if relative == path or relative.startswith(path + ".")
        ]
        if len(matches) != 1:
            unclassified.append(access)
        else:
            classified[matches[0]].append(access)
    if unclassified:
        raise ValueError(f"unclassified direct accesses: {unclassified}")
    if any(not values for values in classified.values()):
        raise ValueError(f"an annotated physical object has no direct source access: {classified}")

    annotation_objects = {item["name"] for item in annotation["objects"]}
    mapped_objects = set(annotation["existing_c_layout"]["members"])
    if annotation_objects != mapped_objects:
        raise ValueError(
            f"layout object inventory and C mapping differ: {annotation_objects} vs {mapped_objects}"
        )

    result = {
        "schema": "tinysqisign-lifetime-annotation-coverage-v1",
        "status": "PASS",
        "source_commit": SOURCE_COMMIT,
        "inputs": {
            "header": {"path": str(HEADER.relative_to(ROOT)), "sha256": sha256(HEADER)},
            "source": {"path": str(SOURCE.relative_to(ROOT)), "sha256": sha256(SOURCE)},
            "annotation": {
                "path": str(ANNOTATION.relative_to(ROOT)),
                "sha256": sha256(ANNOTATION),
            },
        },
        "declared_physical_objects": sorted(declared_paths),
        "annotated_physical_objects": sorted(configured_paths),
        "direct_accesses": classified,
        "whole_phase_accesses": whole_phase_accesses,
        "decision": {
            "all_declared_physical_members_are_annotated": True,
            "all_direct_source_accesses_are_classified": True,
            "object_mapping_is_bijective": True,
            "lifetimes_inferred_from_c": False,
            "whole_program_alias_analysis_performed": False,
        },
        "boundary": (
            "This fail-closed gate covers declared members and direct field accesses. "
            "Phase labels, indirect aliases, pointer escape, and clearing semantics remain "
            "human-reviewed obligations checked by the lifetime certificate and runtime gates."
        ),
    }
    output = args.output if args.output.is_absolute() else ROOT / args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(
        "lifetime annotation coverage: PASS "
        f"({len(declared_paths)} objects, {len(accesses)} direct access forms)"
    )
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
