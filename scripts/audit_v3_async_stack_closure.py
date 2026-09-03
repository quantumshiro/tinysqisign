#!/usr/bin/env python3
"""Inventory the asynchronous/MSP stack obligations of the linked v3 image.

This audit deliberately stops short of a whole-program stack bound.  It closes
what can be learned from the linked direct-call graph and GCC stack-usage
records, then records every remaining indirect callback and architectural
exception/nesting obligation.  The output is therefore useful evidence of a
bounded claim, not a certificate that the complete MSP reservation is safe.
"""

from __future__ import annotations

import argparse
import json
import subprocess
from pathlib import Path

from audit_v3_stack_bound import (
    cmake_cache_values,
    frame_candidates,
    parse_disassembly,
    parse_stack_usage,
    reachable,
    sha256,
    shortest_paths,
    strongly_connected_components,
)


ASYNC_ROOTS = (
    "dcd_rp2040_irq",
    "usb_irq",
    "low_priority_worker_irq",
    "alarm_pool_irq_handler",
)


def require_source_patterns(
    path: Path, patterns: list[str], relative_to: Path
) -> dict[str, object]:
    text = path.read_text(encoding="utf-8")
    missing = [pattern for pattern in patterns if pattern not in text]
    if missing:
        raise ValueError(f"source registration patterns changed in {path}: {missing}")
    return {
        "path": str(path.relative_to(relative_to)),
        "sha256": sha256(path),
        "required_patterns": patterns,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--objdump", type=Path, required=True)
    parser.add_argument("--pico-sdk", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--expected-firmware-commit")
    args = parser.parse_args()

    cache_path = args.build_dir / "CMakeCache.txt"
    cache = cmake_cache_values(cache_path)
    if cache.get("SQISIGN_FIRMWARE_DIRTY") != "0":
        raise ValueError("async audit ELF was not built from a clean firmware tree")
    if cache.get("SQISIGN_V3_SOURCE_DIRTY") != "0":
        raise ValueError("async audit ELF was not built from a clean v3 source tree")
    firmware_commit = cache.get("SQISIGN_FIRMWARE_GIT_COMMIT")
    if (
        args.expected_firmware_commit is not None
        and firmware_commit != args.expected_firmware_commit
    ):
        raise ValueError(
            f"expected firmware commit {args.expected_firmware_commit}, "
            f"observed {firmware_commit}"
        )

    disassembly = subprocess.run(
        [str(args.objdump), "-d", str(args.elf)],
        check=True,
        text=True,
        capture_output=True,
    ).stdout
    (
        graph,
        indirect_calls,
        functions,
        _bodies,
        veneer_resolutions,
        _cross_symbol_conditionals,
    ) = parse_disassembly(disassembly)
    stack_records = parse_stack_usage(args.build_dir)
    resolved_veneers = {row["veneer"]: row for row in veneer_resolutions}

    registration_sources = {
        "tinyusb_device": require_source_patterns(
            args.pico_sdk
            / "lib/tinyusb/src/portable/raspberrypi/rp2040/dcd_rp2040.c",
            [
                "irq_add_shared_handler(USBCTRL_IRQ, dcd_rp2040_irq,",
                "irq_set_enabled(USBCTRL_IRQ, true);",
            ],
            args.pico_sdk,
        ),
        "stdio_usb": require_source_patterns(
            args.pico_sdk / "src/rp2_common/pico_stdio_usb/stdio_usb.c",
            [
                "irq_set_exclusive_handler(low_priority_irq_num, low_priority_worker_irq);",
                "irq_add_shared_handler(USBCTRL_IRQ, usb_irq,",
                "add_alarm_in_us(PICO_STDIO_USB_TASK_INTERVAL_US, timer_task, NULL, true)",
            ],
            args.pico_sdk,
        ),
        "alarm_pool": require_source_patterns(
            args.pico_sdk / "src/common/pico_time/time.c",
            [
                "static void alarm_pool_irq_handler(void)",
                "ta_enable_irq_handler(timer, hardware_alarm_num, alarm_pool_irq_handler);",
                "delta = earliest_entry->callback(id, earliest_entry->user_data);",
            ],
            args.pico_sdk,
        ),
    }

    root_reports: dict[str, object] = {}
    all_unresolved_indirect: list[dict[str, object]] = []
    all_unresolved_metadata: list[dict[str, object]] = []
    all_dynamic: set[str] = set()
    for root in ASYNC_ROOTS:
        if root not in functions:
            raise ValueError(f"linked asynchronous candidate root is absent: {root}")
        nodes = reachable(graph, root)
        paths = shortest_paths(graph, root)
        missing = []
        zero_frame_veneers = []
        dynamic = []
        for function in sorted(nodes):
            candidates = frame_candidates(function, stack_records)
            if not candidates:
                if function in resolved_veneers:
                    zero_frame_veneers.append(resolved_veneers[function])
                else:
                    row = {
                        "function": function,
                        "path": paths.get(function, [root, function]),
                    }
                    missing.append(row)
                    all_unresolved_metadata.append({"root": root, **row})
                continue
            if any("dynamic" in row["qualifiers"] for row in candidates):
                dynamic.append(function)
                all_dynamic.add(function)

        root_indirect = [
            {
                "function": function,
                "path": paths.get(function, [root, function]),
                "instructions": lines,
            }
            for function, lines in sorted(indirect_calls.items())
            if function in nodes
        ]
        for row in root_indirect:
            for instruction in row["instructions"]:
                all_unresolved_indirect.append(
                    {
                        "root": root,
                        "function": row["function"],
                        "path": row["path"],
                        "instruction": instruction,
                    }
                )
        root_reports[root] = {
            "reachable_functions": len(nodes),
            "recursive_sccs": strongly_connected_components(graph, nodes),
            "compiler_dynamic_frame_functions": dynamic,
            "resolved_literal_veneers_with_zero_local_frame": zero_frame_veneers,
            "missing_stack_metadata": missing,
            "missing_stack_metadata_count": len(missing),
            "unresolved_indirect_calls": root_indirect,
            "unresolved_indirect_callsite_count": sum(
                len(row["instructions"]) for row in root_indirect
            ),
            "direct_graph_closure_complete": not missing and not dynamic,
            "software_callgraph_closure_complete": (
                not missing and not dynamic and not root_indirect
            ),
        }

    unique_indirect_sites = {
        (row["function"], row["instruction"]) for row in all_unresolved_indirect
    }
    complete_handler_roots = [
        root
        for root, report in root_reports.items()
        if report["software_callgraph_closure_complete"]
    ]
    result = {
        "schema": "sqisign-v3-async-stack-closure-audit-v1",
        "status": "PARTIAL_BLOCKERS_ENUMERATED",
        "elf": {
            "filename": args.elf.name,
            "bytes": args.elf.stat().st_size,
            "sha256": sha256(args.elf),
        },
        "build_provenance": {
            "firmware_commit": firmware_commit,
            "firmware_dirty": 0,
            "v3_source_commit": cache.get("SQISIGN_V3_SOURCE_COMMIT"),
            "v3_source_dirty": 0,
            "generated_tree_sha256": cache.get("SQISIGN_V3_GENERATED_TREE_SHA256"),
            "cmake_cache_sha256": sha256(cache_path),
        },
        "candidate_root_basis": {
            "roots": list(ASYNC_ROOTS),
            "registration_sources": registration_sources,
            "interpretation": (
                "These are the linked USB CDC and default alarm-pool handlers named by "
                "the linked dependency sources. Runtime RAM-vector installation means "
                "this source inventory is not by itself a proof of exhaustive enabled IRQs."
            ),
        },
        "roots": root_reports,
        "aggregate": {
            "candidate_root_count": len(ASYNC_ROOTS),
            "roots_with_complete_direct_graph_metadata": sum(
                report["direct_graph_closure_complete"]
                for report in root_reports.values()
            ),
            "roots_without_unresolved_indirect_calls": len(complete_handler_roots),
            "roots_without_unresolved_indirect_calls_names": complete_handler_roots,
            "unique_missing_stack_metadata_count": len(
                {row["function"] for row in all_unresolved_metadata}
            ),
            "unique_unresolved_indirect_callsite_count": len(unique_indirect_sites),
            "compiler_dynamic_frame_function_count": len(all_dynamic),
        },
        "architectural_obligations_not_modeled": [
            "handler-side exception frames for nested IRQ/fault entries on MSP",
            "maximum enabled-IRQ and fault nesting under the configured priorities",
            "MSP depth already live at each asynchronous entry point",
            "runtime RAM-vector contents after all SDK initialization paths",
        ],
        "decision": {
            "linked_candidate_handler_roots_enumerated": True,
            "all_reachable_compiler_frames_static": not all_dynamic,
            "all_direct_graph_stack_metadata_closed": not all_unresolved_metadata,
            "all_indirect_callback_targets_resolved": not all_unresolved_indirect,
            "exception_entry_and_nesting_bound_established": False,
            "whole_program_worst_case_stack_bound_established": False,
            "reason": (
                f"The linked direct-call metadata is closed for "
                f"{sum(report['direct_graph_closure_complete'] for report in root_reports.values())}/"
                f"{len(ASYNC_ROOTS)} candidate roots and no dynamic compiler frame is reachable, "
                f"but {len(unique_indirect_sites)} unique indirect callback sites remain. "
                "Handler-side exception frames, interrupt nesting, live MSP depth, and "
                "runtime vector exhaustiveness are also not bounded."
            ),
        },
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(
        "v3 async stack closure audit: PARTIAL_BLOCKERS_ENUMERATED "
        f"roots={len(ASYNC_ROOTS)} "
        f"missing_metadata={result['aggregate']['unique_missing_stack_metadata_count']} "
        f"indirect_sites={result['aggregate']['unique_unresolved_indirect_callsite_count']} "
        "whole_program_bound=false"
    )
    for root, report in root_reports.items():
        print(
            f"{root}: reachable={report['reachable_functions']} "
            f"missing_su={report['missing_stack_metadata_count']} "
            f"indirect_calls={report['unresolved_indirect_callsite_count']}"
        )
    print(f"output={args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
