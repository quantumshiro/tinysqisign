#!/usr/bin/env python3
"""Compute linked v2 operation-PSP bounds for the frozen RP2350 image.

The audit combines the linked call graph, GCC ``.su`` files, a source-ranked
bound for the only recursive routine, and small linked-library frame records.
It covers every branch in the three Thread-mode operation closures.  Handler
software frames and interrupt nesting use MSP and remain a separate claim.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
from collections import defaultdict
from pathlib import Path

from audit_v3_stack_bound import (
    ARCH_EXCEPTION_PSP_ALLOWANCE_BYTES,
    cmake_cache_values,
    dlog_depth,
    frame_candidates,
    parse_disassembly,
    parse_stack_usage,
    reachable,
    sha256,
    shortest_paths,
    strongly_connected_components,
)


ROOTS = ("keygen_thunk", "sign_thunk", "verify_thunk")
EXPECTED_SOURCE_COMMIT = "71099e0827d3f0a3b3c705d2eda592c401e0d57d"
PSP_RESERVATION_BYTES = 128 * 1024

# These functions are linked from newlib or the RP2350 double-precision
# wrappers and therefore have no GCC stack-usage row in this build.  Each
# value is a conservative maximum for the linked body.  The 40-byte DCP rows
# include a 4-byte saved LR and the 32-byte generic_save_state area, rounded
# up to the 8-byte ABI alignment.
MANUAL_FRAME_BYTES = {
    "____wrap___aeabi_d2lz_veneer": 0,
    "____wrap___aeabi_dmul_veneer": 0,
    "__frexp_veneer": 0,
    "__llround_veneer": 0,
    "__wrap___aeabi_d2lz": 0,
    "__wrap___aeabi_ul2d": 0,
    "__wrap___aeabi_dadd": 40,
    "__wrap___aeabi_dsub": 40,
    "__wrap___aeabi_dmul": 40,
    "__wrap___aeabi_ddiv": 40,
    "__wrap___aeabi_dcmpeq": 40,
    "__wrap___aeabi_dcmplt": 40,
    "__wrap___aeabi_dcmpgt": 40,
    "__wrap___aeabi_dcmpun": 40,
    "frexp": 16,
    "llround": 16,
    "abort": 8,
    "raise": 16,
    "_getpid_r": 0,
    "_kill_r": 16,
}

FRAME_FRAGMENTS = {
    "____wrap___aeabi_d2lz_veneer": ("ldr.w\tpc", ".word\t0x"),
    "____wrap___aeabi_dmul_veneer": ("ldr.w\tpc", ".word\t0x"),
    "__frexp_veneer": ("ldr.w\tpc", ".word\t0x"),
    "__llround_veneer": ("ldr.w\tpc", ".word\t0x"),
    "__wrap___aeabi_d2lz": ("movs\tr2, #0",),
    "__wrap___aeabi_ul2d": ("movs\tr2, #0",),
    "__wrap___aeabi_dadd": ("mrc2", "bx\tlr"),
    "__wrap___aeabi_dsub": ("mrc2", "bx\tlr"),
    "__wrap___aeabi_dmul": ("mrc2", "push\t{r4, lr}", "pop\t{r4, pc}"),
    "__wrap___aeabi_ddiv": ("mrc2", "bx\tlr"),
    "__wrap___aeabi_dcmpeq": ("mrc2", "bx\tlr"),
    "__wrap___aeabi_dcmplt": ("mrc2", "bx\tlr"),
    "__wrap___aeabi_dcmpgt": ("mrc2", "bx\tlr"),
    "__wrap___aeabi_dcmpun": ("mrc2", "bx\tlr"),
    "frexp": ("push\t{r4, r5, r6, lr}", "pop\t{r4, r5, r6, pc}"),
    "llround": ("push\t{r3, r4, r5, lr}", "pop\t{r3, r4, r5, pc}"),
    "abort": ("push\t{r3, lr}", "<raise>"),
    "raise": ("push\t{r4, lr}", "sub\tsp, #8"),
    "_getpid_r": ("<_getpid>",),
    "_kill_r": ("push\t{r3, r4, r5, lr}", "pop\t{r3, r4, r5, pc}"),
}


def command(*args: str | Path) -> str:
    return subprocess.run(
        [str(arg) for arg in args], check=True, text=True, capture_output=True
    ).stdout


def linked_symbols(nm: Path, elf: Path) -> dict[str, int]:
    symbols: dict[str, int] = {}
    for line in command(nm, "-a", "-n", elf).splitlines():
        fields = line.split()
        if len(fields) >= 3 and re.fullmatch(r"[0-9a-fA-F]+", fields[0]):
            symbols[fields[-1]] = int(fields[0], 16)
    return symbols


def initialized_word(objdump: Path, elf: Path, address: int) -> int:
    text = command(
        objdump,
        "-s",
        f"--start-address=0x{address:08x}",
        f"--stop-address=0x{address + 4:08x}",
        elf,
    )
    match = re.search(rf"^\s*{address:08x}\s+([0-9a-fA-F]{{8}})", text, re.MULTILINE)
    if not match:
        raise ValueError(f"could not read initialized word at 0x{address:08x}")
    return int.from_bytes(bytes.fromhex(match.group(1)), "little")


def section_payload(objdump: Path, elf: Path, section: str) -> bytes:
    text = command(objdump, "-s", "-j", section, elf)
    payload = bytearray()
    for line in text.splitlines():
        match = re.match(
            r"^\s*[0-9a-fA-F]+\s+([0-9a-fA-F]{8})(?:\s+([0-9a-fA-F]{8}))?"
            r"(?:\s+([0-9a-fA-F]{8}))?(?:\s+([0-9a-fA-F]{8}))?",
            line,
        )
        if match:
            for group in match.groups():
                if group is not None:
                    payload.extend(bytes.fromhex(group))
    if not payload:
        raise ValueError(f"could not extract {section} from {elf}")
    return bytes(payload)


def manual_frame_candidates(function: str, records: dict[str, list[dict[str, object]]]) -> list[dict[str, object]]:
    candidates = frame_candidates(function, records)
    if candidates:
        return candidates
    if function in MANUAL_FRAME_BYTES:
        return [
            {
                "frame_bytes": MANUAL_FRAME_BYTES[function],
                "qualifiers": ["static", "linked-body-certificate"],
                "record": f"linked-body:{function}",
            }
        ]
    return []


def max_frame(function: str, records: dict[str, list[dict[str, object]]]) -> int:
    candidates = manual_frame_candidates(function, records)
    if not candidates:
        raise ValueError(f"missing stack record for {function}")
    return max(int(row["frame_bytes"]) for row in candidates)


def certify_manual_frames(
    bodies: dict[str, list[str]], reachable_nodes: set[str]
) -> dict[str, object]:
    rows: dict[str, object] = {}
    for function in sorted(reachable_nodes & MANUAL_FRAME_BYTES.keys()):
        body = "\n".join(bodies.get(function, [])) + "\n"
        if not body.strip():
            raise ValueError(f"linked body missing for {function}")
        missing = [fragment for fragment in FRAME_FRAGMENTS[function] if fragment not in body]
        if missing:
            raise ValueError(f"linked body changed for {function}: missing {missing}")
        rows[function] = {
            "frame_bytes": MANUAL_FRAME_BYTES[function],
            "body_sha256": hashlib.sha256(body.encode()).hexdigest(),
            "required_fragments": list(FRAME_FRAGMENTS[function]),
        }
    return {
        "basis": (
            "all linked body stack adjustments inspected; DCP wrapper rows include "
            "the fallback save area and are rounded to ABI alignment"
        ),
        "functions": rows,
    }


def static_bound(
    graph: dict[str, set[str]],
    nodes: set[str],
    cycles: list[list[str]],
    records: dict[str, list[dict[str, object]]],
    dlog_frames: int,
    root: str,
) -> dict[str, object]:
    components = [tuple(component) for component in cycles]
    recursive = set(components)
    assigned = {member for component in components for member in component}
    components.extend((node,) for node in sorted(nodes - assigned))
    component_of = {
        member: index
        for index, component in enumerate(components)
        for member in component
    }
    edges: dict[int, set[int]] = defaultdict(set)
    for source in nodes:
        for target in graph.get(source, ()):
            if target in nodes and component_of[source] != component_of[target]:
                edges[component_of[source]].add(component_of[target])

    def weight(component: tuple[str, ...]) -> tuple[int, str]:
        if component in recursive:
            if len(component) != 1 or not component[0].startswith("fp2_dlog_2e_rec"):
                raise ValueError(f"unrecognized recursive component: {component}")
            return dlog_frames * max_frame(component[0], records), "dlog rank bound"
        if len(component) != 1:
            raise ValueError(f"unexpected component: {component}")
        return max_frame(component[0], records), "local frame"

    visiting: set[int] = set()
    memo: dict[int, tuple[int, list[dict[str, object]]]] = {}

    def visit(index: int) -> tuple[int, list[dict[str, object]]]:
        if index in memo:
            return memo[index]
        if index in visiting:
            raise ValueError("condensed graph still contains a cycle")
        visiting.add(index)
        child_bound, child_path = max(
            (visit(child) for child in edges[index]),
            key=lambda item: item[0],
            default=(0, []),
        )
        local, basis = weight(components[index])
        result = (
            local + child_bound,
            [
                {
                    "component": list(components[index]),
                    "frame_bound_bytes": local,
                    "basis": basis,
                }
            ]
            + child_path,
        )
        visiting.remove(index)
        memo[index] = result
        return result

    software, witness = visit(component_of[root])
    total = software + ARCH_EXCEPTION_PSP_ALLOWANCE_BYTES
    return {
        "software_call_bound_bytes": software,
        "exception_entry_allowance_bytes": ARCH_EXCEPTION_PSP_ALLOWANCE_BYTES,
        "bound_bytes": total,
        "psp_reservation_bytes": PSP_RESERVATION_BYTES,
        "reservation_margin_bytes": PSP_RESERVATION_BYTES - total,
        "fits_reservation": total <= PSP_RESERVATION_BYTES,
        "witness": witness,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--source-dir", type=Path, required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--rebuilt-elf", type=Path, required=True)
    parser.add_argument("--observed-summary", type=Path, required=True)
    parser.add_argument("--observed-manifest", type=Path, required=True)
    parser.add_argument("--objdump", type=Path, required=True)
    parser.add_argument("--nm", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--expected-firmware-commit", required=True)
    args = parser.parse_args()

    cache = cmake_cache_values(args.build_dir / "CMakeCache.txt")
    if cache.get("SQISIGN_FIRMWARE_DIRTY") != "0":
        raise ValueError("firmware tree was dirty")
    if cache.get("SQISIGN_FIRMWARE_GIT_COMMIT") != args.expected_firmware_commit:
        raise ValueError("firmware commit does not match")
    if cache.get("SQISIGN_COMPACT_D13_COMMIT") != EXPECTED_SOURCE_COMMIT:
        raise ValueError("v2 source commit does not match")
    if cache.get("PICO_PLATFORM") != "rp2350-arm-s":
        raise ValueError("unexpected platform")
    if command("git", "-C", args.source_dir, "status", "--porcelain", "--untracked-files=no").strip():
        raise ValueError("tracked v2 source is dirty")

    observed_summary = json.loads(args.observed_summary.read_text(encoding="utf-8"))
    observed_manifest = json.loads(args.observed_manifest.read_text(encoding="utf-8"))
    if observed_summary.get("status") != "PASS":
        raise ValueError("observed target summary did not pass")
    if sha256(args.elf) != observed_manifest["artifacts"]["elf_sha256"]:
        raise ValueError("observed ELF digest does not match the target manifest")
    if (
        observed_summary["binary"]["firmware_commit"]
        != observed_manifest["firmware_source_commit"]
        or observed_summary["binary"]["sqisign_commit"]
        != observed_manifest["sqisign_source"]["commit"]
    ):
        raise ValueError("target summary and manifest name different source commits")
    observed_text = section_payload(args.objdump, args.elf, ".text")
    rebuilt_text = section_payload(args.objdump, args.rebuilt_elf, ".text")
    if observed_text != rebuilt_text:
        raise ValueError("analysis rebuild and observed ELF have different .text bytes")

    disassembly = command(args.objdump, "-d", args.elf)
    graph, indirect, functions, bodies, veneers, cross_conditionals = parse_disassembly(disassembly)
    records = parse_stack_usage(args.build_dir)
    symbols = linked_symbols(args.nm, args.elf)

    mem_source = (args.source_dir / "src/common/generic/mem.c").read_text(encoding="utf-8")
    required_mem_fragments = (
        "static volatile memset_t memset_func = memset;",
        "memset_func(mem, 0, size);",
    )
    if not all(fragment in mem_source for fragment in required_mem_fragments):
        raise ValueError("secure-clear source binding changed")
    pointer = initialized_word(args.objdump, args.elf, symbols["memset_func.0"])
    if (pointer & ~1) != symbols["memset"]:
        raise ValueError("secure-clear function pointer is not initialized to linked memset")
    graph["sqisign_gen_sqisign_secure_clear"].add("memset")
    indirect.pop("sqisign_gen_sqisign_secure_clear", None)

    # raise(SIGABRT) can call a user-installed signal handler.  No signal() or
    # _signal_r entry point is linked, and the linked _reent signal-table
    # pointer starts at zero.  Under the ordinary no-memory-corruption model,
    # the indirect handler branch is therefore unreachable in this image.
    if any(name in symbols for name in ("signal", "_signal_r")):
        raise ValueError("a signal-handler registration API is linked")
    if initialized_word(args.objdump, args.elf, symbols["_impure_ptr"]) != symbols["_impure_data"]:
        raise ValueError("unexpected _impure_ptr initialization")
    signal_table_pointer = initialized_word(
        args.objdump, args.elf, symbols["_impure_data"] + 0x1F8
    )
    if signal_table_pointer != 0:
        raise ValueError("signal table is initialized")
    indirect.pop("raise", None)

    all_rows = [row for values in records.values() for row in values]
    dynamic = [row for row in all_rows if "dynamic" in row["qualifiers"]]
    if dynamic:
        raise ValueError("compiler reported dynamic stack records")

    all_reachable = set().union(*(reachable(graph, root) for root in ROOTS))
    manual_certificate = certify_manual_frames(bodies, all_reachable)
    dlog_source = args.source_dir / "src/ec/ref/lvlx/biextension.c"
    dlog_text = dlog_source.read_text(encoding="utf-8")
    for fragment in (
        "if (len == 0)",
        "else if (len == 1)",
        "long right = (double)len * 0.5;",
        "long left = len - right;",
        "fp2_dlog_2e_rec(dlp1, right, pows_f, pows_g, stacklen + 1)",
        "fp2_dlog_2e_rec(dlp2, left, pows_f, pows_g, stacklen)",
        "e > TORSION_EVEN_POWER",
    ):
        if fragment not in dlog_text:
            raise ValueError(f"dlog source rank pattern missing: {fragment}")
    ec_parameters = (args.source_dir / "src/precomp/ref/lvl1/include/ec_params.h").read_text()
    match = re.search(r"^#define TORSION_EVEN_POWER\s+(\d+)$", ec_parameters, re.MULTILINE)
    if not match:
        raise ValueError("TORSION_EVEN_POWER missing")
    maximum_dlog_length = int(match.group(1))
    dlog_frames = dlog_depth(maximum_dlog_length)

    root_reports: dict[str, object] = {}
    bounds: dict[str, object] = {}
    all_cycles: set[tuple[str, ...]] = set()
    for root in ROOTS:
        if root not in functions:
            raise ValueError(f"missing operation root {root}")
        nodes = reachable(graph, root)
        paths = shortest_paths(graph, root)
        cycles = strongly_connected_components(graph, nodes)
        all_cycles.update(tuple(component) for component in cycles)
        missing = [
            {"function": function, "path": paths.get(function, [root, function])}
            for function in sorted(nodes)
            if not manual_frame_candidates(function, records)
        ]
        unresolved = [
            {"function": function, "instructions": indirect[function]}
            for function in sorted(nodes)
            if indirect.get(function)
        ]
        if missing or unresolved:
            raise ValueError(f"incomplete {root} closure: missing={missing}, indirect={unresolved}")
        root_reports[root] = {
            "reachable_functions": len(nodes),
            "recursive_sccs": cycles,
            "missing_stack_metadata_count": 0,
            "unresolved_indirect_callsite_count": 0,
        }
        bounds[root] = static_bound(graph, nodes, cycles, records, dlog_frames, root)

    expected_cycles = {
        ("fp2_dlog_2e_rec",),
        ("fp2_dlog_2e_rec.constprop.0",),
    }
    if all_cycles != expected_cycles:
        raise ValueError(f"recursive component inventory changed: {all_cycles}")
    if not all(record["fits_reservation"] for record in bounds.values()):
        raise ValueError(f"a PSP bound exceeds the reservation: {bounds}")

    observed_by_root = {
        "keygen_thunk": max(
            capture["operations"]["keygen"]["psp_extent_bytes"]
            for capture in observed_summary["captures"]
        ),
        "sign_thunk": max(
            capture["operations"]["sign"]["psp_extent_bytes"]
            for capture in observed_summary["captures"]
        ),
        "verify_thunk": max(
            capture["operations"]["verify"]["psp_extent_bytes"]
            for capture in observed_summary["captures"]
        ),
    }
    for root, observed in observed_by_root.items():
        bounds[root]["observed_psp_extent_bytes"] = observed
        bounds[root]["observed_within_bound"] = observed <= bounds[root]["bound_bytes"]
        bounds[root]["observed_equals_software_call_bound"] = (
            observed == bounds[root]["software_call_bound_bytes"]
        )
        if not bounds[root]["observed_within_bound"]:
            raise ValueError(f"observed PSP extent exceeds the {root} bound")

    result = {
        "schema": "sqisign-v2-linked-stack-bound-audit-v1",
        "status": "OPERATION_PSP_BOUND_ESTABLISHED",
        "elf": {
            "filename": args.elf.name,
            "bytes": args.elf.stat().st_size,
            "sha256": sha256(args.elf),
            "firmware_commit": observed_summary["binary"]["firmware_commit"],
        },
        "analysis_rebuild": {
            "filename": args.rebuilt_elf.name,
            "bytes": args.rebuilt_elf.stat().st_size,
            "sha256": sha256(args.rebuilt_elf),
            "text_sha256": hashlib.sha256(rebuilt_text).hexdigest(),
            "text_identical_to_observed_elf": True,
        },
        "provenance": {
            "firmware_commit": args.expected_firmware_commit,
            "firmware_dirty": 0,
            "v2_source_commit": EXPECTED_SOURCE_COMMIT,
            "v2_tracked_source_clean": True,
            "compiler": command(cache["CMAKE_C_COMPILER"], "--version").splitlines()[0],
            "cmake_cache_sha256": sha256(args.build_dir / "CMakeCache.txt"),
        },
        "compiler_stack_usage": {
            "files": len(list(args.build_dir.rglob("*.su"))),
            "records": len(all_rows),
            "dynamic_records": 0,
        },
        "roots": root_reports,
        "static_psp_bounds": bounds,
        "recursion_certificate": {
            "source": str(dlog_source.relative_to(args.source_dir)),
            "source_sha256": sha256(dlog_source),
            "maximum_input_length": maximum_dlog_length,
            "rank": "positive len; each recursive child receives floor(len/2) or ceil(len/2)",
            "maximum_active_frames": dlog_frames,
            "all_recursive_components_covered": True,
        },
        "manual_frame_certificate": manual_certificate,
        "indirect_call_resolutions": {
            "secure_clear": {
                "target": "memset",
                "initialized_pointer": f"0x{pointer:08x}",
                "source_sha256": sha256(args.source_dir / "src/common/generic/mem.c"),
            },
            "raise_signal_handler": {
                "registration_symbols_linked": False,
                "initial_signal_table_pointer": 0,
                "assumption": "ordinary execution without arbitrary memory corruption",
            },
        },
        "literal_veneer_resolutions": veneers,
        "cross_symbol_conditional_branches": cross_conditionals,
        "architectural_exception_allowance_bytes": ARCH_EXCEPTION_PSP_ALLOWANCE_BYTES,
        "target_observation_crosscheck": {
            "summary": str(args.observed_summary),
            "summary_sha256": sha256(args.observed_summary),
            "manifest": str(args.observed_manifest),
            "manifest_sha256": sha256(args.observed_manifest),
            "observed_text_sha256": hashlib.sha256(observed_text).hexdigest(),
            "analysis_rebuild_text_sha256": hashlib.sha256(rebuilt_text).hexdigest(),
            "text_sections_identical": True,
            "all_observed_psp_extents_within_bounds": True,
            "all_observed_psp_extents_equal_software_call_bounds": all(
                record["observed_equals_software_call_bound"] for record in bounds.values()
            ),
        },
        "decision": {
            "all_input_dependent_thread_mode_paths_in_linked_graph_covered": True,
            "operation_psp_bounds_established": True,
            "all_operation_psp_bounds_fit_128_kib": True,
            "whole_program_interrupt_inclusive_stack_bound_established": False,
            "boundary": (
                "The bounds cover the linked KeyGen, Sign, and Verify Thread-mode call "
                "closures plus one maximum exception-entry frame charged to PSP. "
                "Handler software frames, enabled-interrupt nesting, and live MSP depth "
                "are outside this certificate."
            ),
        },
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print("v2 linked operation PSP audit: PASS")
    for root, record in bounds.items():
        print(
            f"{root}: software={record['software_call_bound_bytes']} "
            f"total={record['bound_bytes']} margin={record['reservation_margin_bytes']}"
        )
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
