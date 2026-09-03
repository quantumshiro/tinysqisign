#!/usr/bin/env python3
"""Generate an interference graph and typed C layout from lifetime annotations.

The input is deliberately small and reviewable.  Phase intersection and any
explicitly forced pair create an interference edge.  A deterministic first-fit
allocator then assigns aligned byte offsets.  Secret objects must name every
terminal clear edge and must be annotated as non-escaping.

This proves properties of the supplied annotation model, not arbitrary C
semantics.  Frozen-source and runtime checks remain separate evidence layers.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(f"FAIL: {message}")


def align_up(value: int, alignment: int) -> int:
    return (value + alignment - 1) // alignment * alignment


def is_power_of_two(value: int) -> bool:
    return value > 0 and value & (value - 1) == 0


def intervals_overlap(a_offset: int, a_size: int, b_offset: int, b_size: int) -> bool:
    return a_offset < b_offset + b_size and b_offset < a_offset + a_size


def normalize_pair(a: str, b: str) -> tuple[str, str]:
    return tuple(sorted((a, b)))  # type: ignore[return-value]


def validate(config: dict[str, Any]) -> list[dict[str, Any]]:
    if config.get("schema") != "tinysqisign-lifetime-layout-v1":
        fail("unsupported schema")
    objects = config.get("objects")
    if not isinstance(objects, list) or not objects:
        fail("objects must be a non-empty list")
    required = {
        "name",
        "c_type",
        "size_bytes",
        "alignment_bytes",
        "phases",
        "secret",
        "pointer_escape",
        "clear_on",
    }
    names: set[str] = set()
    terminal = set(config.get("terminal_exits", []))
    if not terminal:
        fail("terminal_exits must be non-empty")
    for obj in objects:
        missing = required - set(obj)
        if missing:
            fail(f"{obj.get('name', '<unnamed>')}: missing {sorted(missing)}")
        name = obj["name"]
        if not isinstance(name, str) or not name.isidentifier() or name in names:
            fail(f"invalid or duplicate object name: {name!r}")
        names.add(name)
        size = obj["size_bytes"]
        alignment = obj["alignment_bytes"]
        if not isinstance(size, int) or size <= 0:
            fail(f"{name}: size must be positive")
        if not isinstance(alignment, int) or not is_power_of_two(alignment):
            fail(f"{name}: alignment must be a positive power of two")
        if not obj["phases"] or len(set(obj["phases"])) != len(obj["phases"]):
            fail(f"{name}: phases must be a non-empty set-like list")
        if obj["secret"]:
            missing_clears = terminal - set(obj["clear_on"])
            if missing_clears:
                fail(f"{name}: secret object misses terminal clears {sorted(missing_clears)}")
            if obj["pointer_escape"]:
                fail(f"{name}: secret object is annotated as escaping")
    return objects


def build_edges(config: dict[str, Any], objects: list[dict[str, Any]]) -> set[tuple[str, str]]:
    by_name = {obj["name"]: obj for obj in objects}
    edges: set[tuple[str, str]] = set()
    for index, left in enumerate(objects):
        for right in objects[index + 1 :]:
            if set(left["phases"]) & set(right["phases"]):
                edges.add(normalize_pair(left["name"], right["name"]))
    for raw_pair in config.get("forced_interference", []):
        if not isinstance(raw_pair, list) or len(raw_pair) != 2:
            fail(f"invalid forced interference pair: {raw_pair!r}")
        left, right = raw_pair
        if left not in by_name or right not in by_name or left == right:
            fail(f"unknown/self forced interference pair: {raw_pair!r}")
        edges.add(normalize_pair(left, right))
    return edges


def place(objects: list[dict[str, Any]], edges: set[tuple[str, str]]) -> dict[str, int]:
    offsets: dict[str, int] = {}
    by_name = {obj["name"]: obj for obj in objects}
    for obj in objects:
        candidates = {0}
        for placed_name, placed_offset in offsets.items():
            placed = by_name[placed_name]
            candidates.add(align_up(placed_offset + placed["size_bytes"], obj["alignment_bytes"]))
        selected: int | None = None
        for candidate in sorted(candidates):
            valid = True
            for placed_name, placed_offset in offsets.items():
                if normalize_pair(obj["name"], placed_name) not in edges:
                    continue
                placed = by_name[placed_name]
                if intervals_overlap(
                    candidate,
                    obj["size_bytes"],
                    placed_offset,
                    placed["size_bytes"],
                ):
                    valid = False
                    break
            if valid:
                selected = candidate
                break
        if selected is None:
            fail(f"could not place {obj['name']}")
        offsets[obj["name"]] = selected
    return offsets


def c_identifier(name: str) -> str:
    return "".join(char if char.isalnum() or char == "_" else "_" for char in name)


def generate_header(config: dict[str, Any], objects: list[dict[str, Any]], offsets: dict[str, int]) -> str:
    layout_name = c_identifier(config["layout_name"])
    by_offset: dict[int, list[dict[str, Any]]] = {}
    for obj in objects:
        by_offset.setdefault(offsets[obj["name"]], []).append(obj)
    lines = [
        "/* Generated by scripts/generate_lifetime_layout.py; do not edit. */",
        "#ifndef TINYSQISIGN_GENERATED_FINDUV_LAYOUT_H",
        "#define TINYSQISIGN_GENERATED_FINDUV_LAYOUT_H",
        "#include <stddef.h>",
        "",
    ]
    slot_types: dict[int, str] = {}
    for offset, members in sorted(by_offset.items()):
        if len(members) == 1:
            slot_types[offset] = members[0]["c_type"]
            continue
        slot_name = f"{layout_name}_slot_{offset}_t"
        slot_types[offset] = slot_name
        lines.append("typedef union {")
        for member in members:
            lines.append(f"    {member['c_type']} {member['name']};")
        lines.append(f"}} {slot_name};")
        lines.append("")
    lines.append("typedef struct {")
    cursor = 0
    for offset, members in sorted(by_offset.items()):
        if offset < cursor:
            fail("generated physical slots overlap")
        if offset > cursor:
            lines.append(f"    unsigned char _pad_{cursor}[{offset - cursor}];")
        field_name = members[0]["name"] if len(members) == 1 else f"slot_{offset}"
        lines.append(f"    {slot_types[offset]} {field_name};")
        cursor = max(offset + member["size_bytes"] for member in members)
    lines.append(f"}} {layout_name}_t;")
    lines.append("")
    expected = config["expected_extent_bytes"]
    lines.append(f'_Static_assert(sizeof({layout_name}_t) == {expected}, "generated extent");')
    for offset, members in sorted(by_offset.items()):
        field_name = members[0]["name"] if len(members) == 1 else f"slot_{offset}"
        lines.append(
            f'_Static_assert(offsetof({layout_name}_t, {field_name}) == {offset}, "generated offset {field_name}");'
        )
    existing = config.get("existing_c_layout", {})
    if existing:
        lines.append(
            f'_Static_assert(sizeof({existing["type"]}) == {existing["extent_bytes"]}, "existing extent");'
        )
    lines.extend(["", "#endif"])
    return "\n".join(lines) + "\n"


def generate_dot(objects: list[dict[str, Any]], edges: set[tuple[str, str]], offsets: dict[str, int]) -> str:
    lines = ["graph lifetime_interference {", "  graph [overlap=false];", "  node [shape=box];"]
    for obj in objects:
        label = f"{obj['name']}\\n{obj['size_bytes']} B @ {offsets[obj['name']]}"
        lines.append(f'  {obj["name"]} [label="{label}"];')
    for left, right in sorted(edges):
        lines.append(f"  {left} -- {right};")
    lines.append("}")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("annotation", type=Path)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    annotation = args.annotation if args.annotation.is_absolute() else ROOT / args.annotation
    output_dir = args.output_dir if args.output_dir.is_absolute() else ROOT / args.output_dir
    config = json.loads(annotation.read_text(encoding="utf-8"))
    objects = validate(config)
    edges = build_edges(config, objects)
    offsets = place(objects, edges)
    extent = max(offsets[obj["name"]] + obj["size_bytes"] for obj in objects)

    expected_offsets = config.get("expected_offsets", {})
    if offsets != expected_offsets:
        fail(f"offset mismatch: generated={offsets}, expected={expected_offsets}")
    if extent != config["expected_extent_bytes"]:
        fail(f"extent mismatch: generated={extent}, expected={config['expected_extent_bytes']}")
    for left, right in edges:
        left_obj = next(obj for obj in objects if obj["name"] == left)
        right_obj = next(obj for obj in objects if obj["name"] == right)
        if intervals_overlap(
            offsets[left], left_obj["size_bytes"], offsets[right], right_obj["size_bytes"]
        ):
            fail(f"interfering objects overlap: {left}, {right}")

    output_dir.mkdir(parents=True, exist_ok=True)
    result = {
        "schema": "tinysqisign-lifetime-layout-result-v1",
        "status": "PASS",
        "annotation": str(annotation.relative_to(ROOT)),
        "layout_name": config["layout_name"],
        "extent_bytes": extent,
        "offsets": offsets,
        "interference_edges": [list(pair) for pair in sorted(edges)],
        "overlay_pairs": [
            [left["name"], right["name"]]
            for index, left in enumerate(objects)
            for right in objects[index + 1 :]
            if offsets[left["name"]] == offsets[right["name"]]
        ],
        "terminal_clear_obligations": {
            obj["name"]: obj["clear_on"] for obj in objects if obj["secret"]
        },
        "claim_boundary": config["claim_boundary"],
    }
    (output_dir / "layout.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    (output_dir / "interference.dot").write_text(generate_dot(objects, edges, offsets), encoding="utf-8")
    (output_dir / "generated_layout.h").write_text(
        generate_header(config, objects, offsets), encoding="utf-8"
    )
    print(f"lifetime layout: PASS ({extent} bytes, {len(edges)} interference edges)")
    print(f"output={output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
