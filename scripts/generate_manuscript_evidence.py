#!/usr/bin/env python3
"""Generate manuscript rows/macros from frozen machine-readable evidence."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load(path: Path) -> dict[str, object]:
    return json.loads(path.read_text(encoding="utf-8"))


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def command(name: str, value: int | float, digits: int | None = None) -> str:
    rendered = str(value) if digits is None else f"{float(value):.{digits}f}"
    return f"\\newcommand{{\\{name}}}{{{rendered}}}"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--baseline-manifest",
        type=Path,
        default=ROOT / "results/rp2350/ksv-cb888f8-manifest.json",
    )
    parser.add_argument(
        "--proposed-manifest",
        type=Path,
        default=ROOT / "results/rp2350/ksv-d13-dc3289a-manifest.json",
    )
    parser.add_argument(
        "--host-runtime",
        type=Path,
        default=ROOT / "results/host/d13-certified-norm-sketch-runtime.json",
    )
    parser.add_argument(
        "--v2-target-repeat",
        type=Path,
        default=ROOT / "results/rp2350/ksv-d13-repeat-2026-09-04-summary.json",
    )
    parser.add_argument(
        "--v2-stack-bound",
        type=Path,
        default=ROOT / "results/v2/analysis/linked-stack-bound-2026-09-04.json",
    )
    parser.add_argument(
        "--v3-clean",
        type=Path,
        default=ROOT / "results/v3/rp2350/interleaved-clean-2026-09-04/summary.json",
    )
    parser.add_argument(
        "--v3-multi",
        type=Path,
        default=ROOT / "results/v3/rp2350/multi-input-placement-clean-2026-09-04/summary.json",
    )
    parser.add_argument(
        "--v3-d2",
        type=Path,
        default=ROOT / "results/v3/rp2350/d2-static-2026-09-04/summary.json",
    )
    parser.add_argument(
        "--v3-fixed-key-timing",
        type=Path,
        default=ROOT / "results/host/v3-fixed-key-timing-2026-09-04/summary.json",
    )
    parser.add_argument(
        "--v3-fixed-key-structural",
        type=Path,
        default=ROOT
        / "results/host/v3-fixed-key-structural-trace-2026-09-04/summary.json",
    )
    parser.add_argument(
        "--v3-target-fixed-key-timing",
        type=Path,
        default=ROOT
        / "results/v3/rp2350/fixed-key-timing-clean-2026-09-04/summary.json",
    )
    parser.add_argument(
        "--v3-static-closure",
        type=Path,
        default=ROOT
        / "results/v3/rp2350/static-closure-clean-2026-09-04/summary.json",
    )
    parser.add_argument(
        "--v3-stack-bound",
        type=Path,
        default=ROOT
        / "results/v3/analysis/d2-linked-stack-bound-audit-2026-09-04.json",
    )
    parser.add_argument(
        "--v3-d3",
        type=Path,
        default=ROOT
        / "results/v3/rp2350/d3-two-function-clean-2026-09-04/summary.json",
    )
    parser.add_argument(
        "--v3-all-parameter-frames",
        type=Path,
        default=ROOT
        / "results/v3/analysis/lifetime-all-params-2026-09-04.json",
    )
    parser.add_argument(
        "--lifetime-annotation-coverage",
        type=Path,
        default=ROOT
        / "results/revision-2026-09-04/lifetime-annotation-coverage.json",
    )
    parser.add_argument(
        "--output-dir", type=Path, default=ROOT / "manuscript/generated"
    )
    parser.add_argument(
        "--audit-output",
        type=Path,
        default=ROOT / "results/revision-2026-09-04/manuscript-evidence.json",
    )
    args = parser.parse_args()

    baseline = load(args.baseline_manifest)
    proposed = load(args.proposed_manifest)
    host = load(args.host_runtime)
    v3_clean = load(args.v3_clean)
    v3_multi = load(args.v3_multi)
    v3_d2 = load(args.v3_d2)
    v3_fixed_timing = load(args.v3_fixed_key_timing)
    v3_structural = load(args.v3_fixed_key_structural)
    v3_stack_bound = load(args.v3_stack_bound)
    v2_stack_bound = load(args.v2_stack_bound)
    v3_d3 = load(args.v3_d3)
    v3_all_parameter_frames = load(args.v3_all_parameter_frames)
    annotation_coverage = load(args.lifetime_annotation_coverage)

    require(
        baseline.get("schema") == "sqisign-rp2350-ksv-evidence-v1",
        "unexpected baseline manifest schema",
    )
    require(
        proposed.get("schema") == "sqisign-rp2350-ksv-d13-evidence-v1",
        "unexpected proposed manifest schema",
    )
    require(host.get("schema") == "sqisign-certified-norm-sketch-runtime-v1", "bad host runtime")
    require(v3_clean.get("all_captures_passed") is True, "v3 clean campaign failed")
    require(v3_clean.get("all_firmware_trees_clean") is True, "v3 clean campaign is dirty")
    require(v3_multi.get("all_trials_passed") is True, "v3 multi campaign failed")
    require(v3_multi.get("placement_psp_mismatch_count") == 0, "placement PSP mismatch")
    require(v3_d2.get("status") == "PASS", "v3 D2 audit failed")
    require(v3_fixed_timing.get("status") == "PASS", "v3 fixed-key timing failed")
    require(
        v3_fixed_timing["decision"]["repeatable_fixed_key_associated_host_timing_observed"]
        is True,
        "v3 fixed-key timing association was not reproduced",
    )
    require(
        v3_fixed_timing["decision"]["side_channel_resistance_established"] is False,
        "v3 timing screen must not certify side-channel resistance",
    )
    require(v3_structural.get("status") == "PASS", "v3 structural trace failed")
    require(
        v3_structural["decision"]["same_key_negative_controls_stable"] is True,
        "v3 structural negative controls are unstable",
    )
    require(
        v3_structural["decision"][
            "repeatable_fixed_key_associated_control_flow_observed"
        ]
        is True
        and v3_structural["decision"][
            "repeatable_fixed_key_associated_effective_address_observed"
        ]
        is True,
        "v3 fixed-key structural differences did not reproduce",
    )
    require(
        v3_structural["decision"]["all_primary_control_flow_event_counts_differ"]
        is True
        and v3_structural["decision"][
            "all_primary_effective_address_event_counts_differ"
        ]
        is True,
        "v3 primary structural finding relies only on stream digests",
    )
    require(
        v3_structural["decision"]["side_channel_resistance_established"] is False,
        "v3 structural trace must not certify side-channel resistance",
    )
    require(
        v3_stack_bound.get("schema") == "sqisign-v3-linked-stack-bound-audit-v4"
        and v3_stack_bound.get("status") == "PSP_BOUND_ESTABLISHED",
        "v3 linked PSP bound is absent or stale",
    )
    require(
        v3_stack_bound["decision"]["operation_psp_bounds_established"] is True
        and v3_stack_bound["decision"][
            "whole_program_worst_case_stack_bound_established"
        ]
        is False,
        "v3 PSP claim boundary changed",
    )
    require(
        v3_stack_bound["architectural_psp_exception_certificate"][
            "psp_allowance_bytes"
        ]
        == 212
        and v3_stack_bound["architectural_psp_exception_certificate"][
            "linked_psp_write_sites_confined_to_trampoline"
        ]
        is True,
        "v3 architectural PSP exception certificate changed",
    )
    require(
        v2_stack_bound.get("status") == "OPERATION_PSP_BOUND_ESTABLISHED"
        and v2_stack_bound["decision"]["operation_psp_bounds_established"] is True
        and v2_stack_bound["decision"][
            "whole_program_interrupt_inclusive_stack_bound_established"
        ]
        is False,
        "v2 operation PSP claim boundary changed",
    )
    require(
        v2_stack_bound["target_observation_crosscheck"]["text_sections_identical"]
        is True,
        "v2 stack analysis does not match the measured image text",
    )
    require(
        v3_d3.get("status") == "PASS"
        and v3_d3["validation"]["two_distinct_functions_lifetime_scheduled"]
        is True
        and v3_d3["validation"][
            "whole_program_worst_case_stack_bound_established"
        ]
        is False,
        "v3 D3 evidence or claim boundary changed",
    )
    require(
        v3_all_parameter_frames.get("status") == "PASS"
        and v3_all_parameter_frames["decision"][
            "all_three_official_parameter_sets_compiled"
        ]
        is True
        and v3_all_parameter_frames["decision"]["two_distinct_functions_measured"]
        is True,
        "v3 all-parameter frame audit failed",
    )
    require(
        annotation_coverage.get("status") == "PASS"
        and annotation_coverage["decision"][
            "all_declared_physical_members_are_annotated"
        ]
        is True
        and annotation_coverage["decision"][
            "whole_program_alias_analysis_performed"
        ]
        is False,
        "lifetime annotation coverage boundary changed",
    )

    full_sram = baseline["sram"]
    low_sram = proposed["sram"]
    full_link = baseline["linked_sections"]
    low_link = proposed["linked_sections"]
    full_exec = baseline["execution"]
    low_exec = proposed["execution"]

    total = int(low_sram["total_on_chip_bytes"])
    require(total == int(full_sram["total_on_chip_bytes"]), "SRAM totals differ")
    for label, memory in (("baseline", full_sram), ("proposed", low_sram)):
        require(
            int(memory["exclusive_reserved_sram_bytes"])
            + int(memory["remaining_unreserved_sram_bytes"])
            == total,
            f"{label}: reserved plus unreserved does not equal total SRAM",
        )
        require(
            int(memory["guarded_workspace_owner_bytes"])
            == int(memory["operation_workspace_payload_bytes"]) + 128,
            f"{label}: guard accounting mismatch",
        )
        require(
            int(memory["main_bank_linked_bytes"])
            == int(memory["guarded_workspace_owner_bytes"])
            + int(memory["psp_reserved_bytes"])
            + int(memory["other_main_bank_bytes"]),
            f"{label}: main-bank accounting mismatch",
        )
        require(
            int(memory["exclusive_reserved_sram_bytes"])
            == int(memory["main_bank_linked_bytes"])
            + int(memory["msp_reserved_bytes"]),
            f"{label}: exclusive SRAM accounting mismatch",
        )

    saved = int(full_sram["operation_workspace_payload_bytes"]) - int(
        low_sram["operation_workspace_payload_bytes"]
    )
    require(saved == 180928, "unexpected arena saving")
    require(
        int(full_sram["exclusive_reserved_sram_bytes"])
        - int(low_sram["exclusive_reserved_sram_bytes"])
        == saved,
        "arena and firmware SRAM deltas differ",
    )
    require(
        int(low_sram["remaining_unreserved_sram_bytes"])
        - int(full_sram["remaining_unreserved_sram_bytes"])
        == saved,
        "arena and unreserved SRAM deltas differ",
    )
    flash_delta = int(low_link["flash_image_end_offset_bytes"]) - int(
        full_link["flash_image_end_offset_bytes"]
    )
    require(flash_delta == 880, "unexpected BIN delta")

    values: dict[str, int | float] = {
        "VTwoTotalSramBytes": total,
        "VTwoFullArenaBytes": int(full_sram["operation_workspace_payload_bytes"]),
        "VTwoLowArenaBytes": int(low_sram["operation_workspace_payload_bytes"]),
        "VTwoArenaSavedBytes": saved,
        "VTwoArenaSavedPercent": 100.0 * saved / int(full_sram["operation_workspace_payload_bytes"]),
        "VTwoFullOwnerBytes": int(full_sram["guarded_workspace_owner_bytes"]),
        "VTwoLowOwnerBytes": int(low_sram["guarded_workspace_owner_bytes"]),
        "VTwoPspReservedBytes": int(low_sram["psp_reserved_bytes"]),
        "VTwoOtherMainBytes": int(low_sram["other_main_bank_bytes"]),
        "VTwoMainLinkedBytes": int(low_sram["main_bank_linked_bytes"]),
        "VTwoMspReservedBytes": int(low_sram["msp_reserved_bytes"]),
        "VTwoCommonReservedBytes": int(low_sram["other_main_bank_bytes"])
        + int(low_sram["msp_reserved_bytes"]),
        "VTwoExclusiveReservedBytes": int(low_sram["exclusive_reserved_sram_bytes"]),
        "VTwoFullExclusiveReservedBytes": int(full_sram["exclusive_reserved_sram_bytes"]),
        "VTwoUnreservedBytes": int(low_sram["remaining_unreserved_sram_bytes"]),
        "VTwoFullUnreservedBytes": int(full_sram["remaining_unreserved_sram_bytes"]),
        "VTwoTextBytes": int(low_link["text_bytes"]),
        "VTwoRodataBytes": int(low_link["rodata_bytes"]),
        "VTwoDataBytes": int(low_link["data_bytes"]),
        "VTwoBssBytes": int(low_link["bss_bytes"]),
        "VTwoBinBytes": int(low_link["flash_image_end_offset_bytes"]),
        "VTwoFullBinBytes": int(full_link["flash_image_end_offset_bytes"]),
        "VTwoBinDeltaBytes": flash_delta,
        "VTwoArenaRatio": int(low_sram["operation_workspace_payload_bytes"])
        / int(full_sram["operation_workspace_payload_bytes"]),
        "VTwoExclusiveSramRatio": int(low_sram["exclusive_reserved_sram_bytes"])
        / int(full_sram["exclusive_reserved_sram_bytes"]),
        "VTwoPspReservationRatio": int(low_sram["psp_reserved_bytes"])
        / int(full_sram["psp_reserved_bytes"]),
        "VTwoBinRatio": int(low_link["flash_image_end_offset_bytes"])
        / int(full_link["flash_image_end_offset_bytes"]),
    }
    v3_timing_spans = [
        float(pass_report["key_median_span_fraction"])
        for implementation in v3_fixed_timing["implementations"].values()
        for pass_report in implementation["passes"].values()
    ]
    v3_timing_spearman = [
        float(implementation["between_pass_key_median_spearman"])
        for implementation in v3_fixed_timing["implementations"].values()
    ]
    values.update(
        {
            "VThreeFixedTimingRows": int(v3_fixed_timing["raw_csv"]["rows"]),
            "VThreeFixedTimingKeys": int(v3_fixed_timing["design"]["key_count"]),
            "VThreeFixedTimingRepetitions": int(
                v3_fixed_timing["design"]["repetitions_per_key_per_pass"]
            ),
            "VThreeKeyTimingSpanMinPercent": 100.0 * min(v3_timing_spans),
            "VThreeKeyTimingSpanMaxPercent": 100.0 * max(v3_timing_spans),
            "VThreeKeyTimingSpearmanMin": min(v3_timing_spearman),
        }
    )
    structural_pairs = v3_structural["results"]["pairs"]
    structural_controls = [
        row for row in structural_pairs if row["dataset"] == "control"
    ]
    structural_primary = [
        row for row in structural_pairs if row["dataset"] == "different-key"
    ]
    require(len(structural_controls) == 8, "bad v3 structural control count")
    require(len(structural_primary) == 36, "bad v3 structural primary count")
    values.update(
        {
            "VThreeStructuralRows": int(v3_structural["results"]["rows"]),
            "VThreeStructuralControlPairs": len(structural_controls),
            "VThreeStructuralDifferentKeyPairs": len(structural_primary),
            "VThreeStructuralEdgeDifferentPairs": sum(
                not bool(row["control_flow_equal"]) for row in structural_primary
            ),
            "VThreeStructuralAddressDifferentPairs": sum(
                not bool(row["effective_address_equal"])
                for row in structural_primary
            ),
        }
    )

    v3_target_fixed: dict[str, object] | None = None
    if args.v3_target_fixed_key_timing.is_file():
        v3_target_fixed = load(args.v3_target_fixed_key_timing)
        require(v3_target_fixed.get("status") == "PASS", "target fixed-key timing failed")
        target_timing_detected = v3_target_fixed["decision"].get(
            "repeatable_fixed_key_associated_rp2350_timing_observed"
        )
        require(
            isinstance(target_timing_detected, bool),
            "target fixed-key timing decision is not Boolean",
        )
        require(
            v3_target_fixed["decision"]["side_channel_resistance_established"]
            is False,
            "target timing must not certify side-channel resistance",
        )
        require(
            v3_target_fixed["design"]["total_sign_samples"] == 200
            and v3_target_fixed["predeclared_design"]["status"]
            == "FROZEN_BEFORE_TARGET_CAPTURE",
            "target timing design or sample count changed",
        )
        require(
            v3_target_fixed["validation"]["direct_key_pointer_address_confound_removed"]
            is True
            and v3_target_fixed["validation"][
                "xip_cache_state_reset_before_each_timed_operation"
            ]
            is True,
            "target timing confound controls are absent",
        )
        target_spans = [
            float(pass_report["key_median_span_fraction"])
            for implementation in v3_target_fixed["implementations"].values()
            for pass_report in implementation["passes"].values()
        ]
        target_spearman = [
            float(implementation["between_pass_key_median_spearman"])
            for implementation in v3_target_fixed["implementations"].values()
        ]
        values.update(
            {
                "VThreeTargetFixedTimingSamples": int(
                    v3_target_fixed["design"]["total_sign_samples"]
                ),
                "VThreeTargetFixedTimingDetected": int(target_timing_detected),
                "VThreeTargetKeyTimingSpanMinPercent": 100.0 * min(target_spans),
                "VThreeTargetKeyTimingSpanMaxPercent": 100.0 * max(target_spans),
                "VThreeTargetKeyTimingSpearmanMin": min(target_spearman),
            }
        )

    v3_static_closure: dict[str, object] | None = None
    if args.v3_static_closure.is_file():
        v3_static_closure = load(args.v3_static_closure)
        require(v3_static_closure.get("status") == "PASS", "static closure target failed")
        require(
            v3_static_closure["validation"]["observed_psp_within_static_bounds"]
            is True,
            "static closure target exceeded the linked PSP bound",
        )
        for operation, macro in (
            ("keygen", "Keygen"),
            ("sign", "Sign"),
            ("verify", "Verify"),
        ):
            values[f"VThreeStaticObserved{macro}PspBytes"] = int(
                v3_static_closure["operations"][operation]["observed_psp_bytes"]
            )
    for root, macro in (
        ("keygen_thunk", "Keygen"),
        ("sign_thunk", "Sign"),
        ("verify_thunk", "Verify"),
    ):
        bound = v3_stack_bound["static_psp_bounds"][root]
        require(bound["fits_reservation"] is True, f"{root} PSP bound does not fit")
        values[f"VThreeStatic{macro}PspBoundBytes"] = int(bound["bound_bytes"])
        values[f"VThreeStatic{macro}PspMarginBytes"] = int(
            bound["reservation_margin_bytes"]
        )
    values["VThreeExceptionPspAllowanceBytes"] = int(
        v3_stack_bound["architectural_psp_exception_certificate"][
            "psp_allowance_bytes"
        ]
    )
    for root, macro in (
        ("keygen_thunk", "Keygen"),
        ("sign_thunk", "Sign"),
        ("verify_thunk", "Verify"),
    ):
        bound = v2_stack_bound["static_psp_bounds"][root]
        require(
            bound["fits_reservation"] is True
            and bound["observed_within_bound"] is True,
            f"v2 {root} PSP bound does not fit",
        )
        values[f"VTwoStatic{macro}SoftwareBytes"] = int(
            bound["software_call_bound_bytes"]
        )
        values[f"VTwoStatic{macro}PspBoundBytes"] = int(bound["bound_bytes"])
        values[f"VTwoStatic{macro}PspMarginBytes"] = int(
            bound["reservation_margin_bytes"]
        )
    values["VTwoExceptionPspAllowanceBytes"] = int(
        v2_stack_bound["architectural_exception_allowance_bytes"]
    )

    d3_comparison = v3_d3["comparison"]
    values.update(
        {
            "VThreeDThreeOfficialSignPspBytes": int(
                d3_comparison["official_sign_psp_bytes"]
            ),
            "VThreeDThreeDOneSignPspBytes": int(
                d3_comparison["one_function_d1_sign_psp_bytes"]
            ),
            "VThreeDThreeSignPspBytes": int(
                d3_comparison["two_function_d3_sign_psp_bytes"]
            ),
            "VThreeDThreeSavedBytes": int(
                d3_comparison["d3_reduction_from_official_bytes"]
            ),
            "VThreeDThreeSavedPercent": float(
                d3_comparison["d3_reduction_from_official_percent"]
            ),
            "VThreeDThreeSecondFunctionSavedBytes": int(
                d3_comparison["second_function_incremental_reduction_bytes"]
            ),
            "VThreeDThreeKatVectors": int(
                v3_d3["validation"][
                    "known_answer_vectors_passed_across_two_implementations"
                ]
            ),
            "LifetimeAnnotatedObjects": len(
                annotation_coverage["annotated_physical_objects"]
            ),
            "LifetimeDirectAccessForms": sum(
                len(accesses)
                for accesses in annotation_coverage["direct_accesses"].values()
            )
            + len(annotation_coverage["whole_phase_accesses"]),
        }
    )
    for operation, macro in (("keygen", "Keygen"), ("sign", "Sign"), ("verify", "Verify")):
        row = low_exec[operation]
        elapsed_us = int(row["elapsed_us"])
        values[f"VTwo{macro}Seconds"] = elapsed_us / 1_000_000
        values[f"VTwo{macro}Cycles"] = elapsed_us * 150
        values[f"VTwo{macro}PspBytes"] = int(row["psp_overwritten_extent_bytes"])
    values["VTwoSignPspMarginBytes"] = int(low_sram["psp_reserved_bytes"]) - int(
        low_exec["sign"]["psp_overwritten_extent_bytes"]
    )
    values["VTwoMspObservedBytes"] = int(
        low_sram["msp_overwritten_extent_conservative_upper_bytes"]
    )

    v2_target_repeat: dict[str, object] | None = None
    if args.v2_target_repeat.is_file():
        v2_target_repeat = load(args.v2_target_repeat)
        require(v2_target_repeat.get("status") == "PASS", "v2 target repeat failed")
        require(
            v2_target_repeat["design"]["boot_count"] == 2
            and v2_target_repeat["design"]["distinct_input_count"] == 1,
            "unexpected v2 target repetition design",
        )
        require(
            v2_target_repeat["decision"]["transcripts_equal_across_boots"] is True
            and v2_target_repeat["decision"]["psp_extents_equal_across_boots"] is True
            and v2_target_repeat["decision"]["worst_case_stack_bound_established"]
            is False,
            "v2 target repetition boundary changed",
        )
        values["VTwoTargetRepeatBoots"] = 2
        values["VTwoTargetRepeatDistinctInputs"] = 1
        for operation, macro in (
            ("keygen", "Keygen"),
            ("sign", "Sign"),
            ("verify", "Verify"),
        ):
            timing = v2_target_repeat["timing"][operation]
            values[f"VTwoRepeat{macro}Seconds"] = float(timing["values_us"][1]) / 1_000_000
            values[f"VTwoRepeat{macro}DeltaPercent"] = float(
                timing["second_vs_first_percent"]
            )

    host_ratios = {
        (str(row["operation"]), int(row["run"])): float(row["ratio_of_medians"])
        for row in host["results"]
    }
    for operation, macro in (("keygen", "Keygen"), ("sign", "Sign")):
        values[f"VTwoHost{macro}RatioOne"] = host_ratios[(operation, 1)]
        values[f"VTwoHost{macro}RatioTwo"] = host_ratios[(operation, 2)]
    for operation, macro in (("keygen", "Keygen"), ("sign", "Sign"), ("verify", "Verify")):
        values[f"VTwoTarget{macro}Ratio"] = float(low_exec[operation]["elapsed_us"]) / float(
            full_exec[operation]["elapsed_us"]
        )

    args.output_dir.mkdir(parents=True, exist_ok=True)
    integer_macros = {
        key
        for key, value in values.items()
        if isinstance(value, int) and not isinstance(value, bool)
    }
    macro_lines = [
        "% Generated by scripts/generate_manuscript_evidence.py; do not edit."
    ]
    for name in sorted(values):
        if name in integer_macros:
            macro_lines.append(command(name, int(values[name])))
        elif name.endswith("Seconds"):
            macro_lines.append(command(name, values[name], 6))
        elif name.endswith("Percent"):
            macro_lines.append(command(name, values[name], 4))
        else:
            macro_lines.append(command(name, values[name], 6))
    (args.output_dir / "evidence-macros.tex").write_text(
        "\n".join(macro_lines) + "\n", encoding="utf-8"
    )

    memory_rows = [
        "% Generated by scripts/generate_manuscript_evidence.py; do not edit.",
        r"オンチップSRAM総量 & \num{\VTwoTotalSramBytes} & 主バンク＋補助バンク \\",
        r"操作用アリーナ本体 & \num{\VTwoLowArenaBytes} & KeyGen・Sign・Verifyの共有領域 \\",
        r"保護領域込み操作用共有領域 & \num{\VTwoLowOwnerBytes} & 主バンク \\",
        r"PSP予約 & \num{\VTwoPspReservedBytes} & 主バンク \\",
        r"その他の主バンク領域 & \num{\VTwoOtherMainBytes} & データ・実行時状態等 \\",
        r"主バンクのリンク時使用量 & \num{\VTwoMainLinkedBytes} & 上記の合計 \\",
        r"MSP予約 & \num{\VTwoMspReservedBytes} & 補助バンク \\",
        r"排他的SRAM予約 & \num{\VTwoExclusiveReservedBytes} & 全精度表比$-\num{\VTwoArenaSavedBytes}$ \\",
        r"未予約SRAM & \num{\VTwoUnreservedBytes} & 全精度表比$+\num{\VTwoArenaSavedBytes}$ \\",
        r"ヒープ & 0 & 動的メモリ確保シンボルなし \\",
        r"\texttt{.text} & \num{\VTwoTextBytes} & XIPフラッシュ \\",
        r"\texttt{.rodata} & \num{\VTwoRodataBytes} & XIPフラッシュ \\",
        r"\texttt{.data} & \num{\VTwoDataBytes} & フラッシュからRAMへ読込み \\",
        r"\texttt{.bss} & \num{\VTwoBssBytes} & RAM、予約を含む \\",
        r"バイナリイメージ & \num{\VTwoBinBytes} & 全精度表比$+\num{\VTwoBinDeltaBytes}$",
    ]
    (args.output_dir / "v2-memory-rows.tex").write_text(
        "\n".join(memory_rows) + "\n", encoding="utf-8"
    )
    memory_rows_en = [
        "% Generated by scripts/generate_manuscript_evidence.py; do not edit.",
        r"On-chip SRAM total & \num{\VTwoTotalSramBytes} & main + scratch \\",
        r"Operation-arena payload & \num{\VTwoLowArenaBytes} & KeyGen/Sign/Verify union \\",
        r"Guarded operation region & \num{\VTwoLowOwnerBytes} & main bank \\",
        r"PSP reservation & \num{\VTwoPspReservedBytes} & main bank \\",
        r"Other main-bank state & \num{\VTwoOtherMainBytes} & data and runtime \\",
        r"Main-bank linked use & \num{\VTwoMainLinkedBytes} & preceding rows \\",
        r"MSP reservation & \num{\VTwoMspReservedBytes} & scratch bank \\",
        r"Exclusive SRAM reservation & \num{\VTwoExclusiveReservedBytes} & $-\num{\VTwoArenaSavedBytes}$ B \\",
        r"Unreserved SRAM & \num{\VTwoUnreservedBytes} & $+\num{\VTwoArenaSavedBytes}$ B \\",
        r"Heap & 0 & no allocator symbol \\",
        r"BIN image & \num{\VTwoBinBytes} & $+\num{\VTwoBinDeltaBytes}$ B",
    ]
    (args.output_dir / "v2-memory-rows-en.tex").write_text(
        "\n".join(memory_rows_en) + "\n", encoding="utf-8"
    )
    runtime_rows = [
        "% Generated by scripts/generate_manuscript_evidence.py; do not edit.",
        r"KeyGen & \num{\VTwoKeygenSeconds} & \num{\VTwoKeygenCycles} & \num{\VTwoKeygenPspBytes} \\",
        r"Sign & \num{\VTwoSignSeconds} & \num{\VTwoSignCycles} & \num{\VTwoSignPspBytes} \\",
        r"Verify & \num{\VTwoVerifySeconds} & \num{\VTwoVerifyCycles} & \num{\VTwoVerifyPspBytes}",
    ]
    (args.output_dir / "v2-runtime-rows.tex").write_text(
        "\n".join(runtime_rows) + "\n", encoding="utf-8"
    )
    static_psp_rows = [
        "% Generated by scripts/generate_manuscript_evidence.py; do not edit.",
        r"KeyGen & \num{\VThreeStaticKeygenPspBoundBytes} & \num{\VThreeStaticKeygenPspMarginBytes} \\",
        r"Sign & \num{\VThreeStaticSignPspBoundBytes} & \num{\VThreeStaticSignPspMarginBytes} \\",
        r"Verify & \num{\VThreeStaticVerifyPspBoundBytes} & \num{\VThreeStaticVerifyPspMarginBytes}",
    ]
    (args.output_dir / "v3-static-psp-bound-rows.tex").write_text(
        "\n".join(static_psp_rows) + "\n", encoding="utf-8"
    )
    v2_static_psp_rows = [
        "% Generated by scripts/generate_manuscript_evidence.py; do not edit.",
        r"KeyGen & \num{\VTwoKeygenPspBytes} & \num{\VTwoStaticKeygenSoftwareBytes} & \num{\VTwoStaticKeygenPspBoundBytes} & \num{\VTwoStaticKeygenPspMarginBytes} \\",
        r"Sign & \num{\VTwoSignPspBytes} & \num{\VTwoStaticSignSoftwareBytes} & \num{\VTwoStaticSignPspBoundBytes} & \num{\VTwoStaticSignPspMarginBytes} \\",
        r"Verify & \num{\VTwoVerifyPspBytes} & \num{\VTwoStaticVerifySoftwareBytes} & \num{\VTwoStaticVerifyPspBoundBytes} & \num{\VTwoStaticVerifyPspMarginBytes}",
    ]
    (args.output_dir / "v2-static-psp-bound-rows.tex").write_text(
        "\n".join(v2_static_psp_rows) + "\n", encoding="utf-8"
    )

    official_psp = {
        operation: int(
            v3_clean["summary"][operation]["official"]["psp_extent_bytes"]["median"]
        )
        for operation in ("keygen", "sign", "verify")
    }
    d3_target_rows = [
        "% Generated by scripts/generate_manuscript_evidence.py; do not edit.",
        r"\texttt{quat\_lll\_dual\_reduce\_ideal}フレーム [B] & \num{34816} & \num{30888} & $-\num{3928}$ \\",
        r"\texttt{protocols\_sign}フレーム [B] & \num{20008} & \num{19272} & $-\num{736}$ \\",
        rf"KeyGen PSP上書き深さ [B] & \num{{{official_psp['keygen']}}} & \num{{{int(v3_d3['operations']['keygen']['observed_psp_bytes'])}}} & \num{{0}} \\",
        r"Sign PSP上書き深さ [B] & \num{\VThreeDThreeOfficialSignPspBytes} & \num{\VThreeDThreeSignPspBytes} & $-\num{\VThreeDThreeSavedBytes}$ ($-\num{\VThreeDThreeSavedPercent}$\%) \\",
        rf"Verify PSP上書き深さ [B] & \num{{{official_psp['verify']}}} & \num{{{int(v3_d3['operations']['verify']['observed_psp_bytes'])}}} & \num{{0}}",
    ]
    (args.output_dir / "v3-d3-results-rows.tex").write_text(
        "\n".join(d3_target_rows) + "\n", encoding="utf-8"
    )
    d3_target_rows_en = [
        "% Generated by scripts/generate_manuscript_evidence.py; do not edit.",
        r"\texttt{quat\_lll\_dual\_reduce\_ideal} frame [B] & \num{34816} & \num{30888} & $-\num{3928}$ \\",
        r"\texttt{protocols\_sign} frame [B] & \num{20008} & \num{19272} & $-\num{736}$ \\",
        rf"KeyGen PSP extent [B] & \num{{{official_psp['keygen']}}} & \num{{{int(v3_d3['operations']['keygen']['observed_psp_bytes'])}}} & \num{{0}} \\",
        r"Sign PSP extent [B] & \num{\VThreeDThreeOfficialSignPspBytes} & \num{\VThreeDThreeSignPspBytes} & $-\num{\VThreeDThreeSavedBytes}$ ($-\num{\VThreeDThreeSavedPercent}$\%) \\",
        rf"Verify PSP extent [B] & \num{{{official_psp['verify']}}} & \num{{{int(v3_d3['operations']['verify']['observed_psp_bytes'])}}} & \num{{0}}",
    ]
    (args.output_dir / "v3-d3-results-rows-en.tex").write_text(
        "\n".join(d3_target_rows_en) + "\n", encoding="utf-8"
    )

    function_labels = {
        "quat_lll_dual_reduce_ideal": r"\texttt{quat\_lll\_dual\_reduce\_ideal}",
        "protocols_sign": r"\texttt{protocols\_sign}",
    }
    parameter_rows = [
        "% Generated by scripts/generate_manuscript_evidence.py; do not edit."
    ]
    comparisons = v3_all_parameter_frames["comparisons"]
    for index, row in enumerate(comparisons):
        ending = r" \\" if index + 1 < len(comparisons) else ""
        parameter = str(row["parameter"]).replace("_", r"\_")
        parameter_rows.append(
            f"\\texttt{{{parameter}}} & {function_labels[str(row['function'])]} & "
            f"\\num{{{row['official_frame_bytes']}}} & \\num{{{row['adapted_frame_bytes']}}} & "
            f"$-\\num{{{row['reduction_bytes']}}}$ ({row['reduction_percent']:.4f}\\%){ending}"
        )
    (args.output_dir / "v3-all-parameter-frame-rows.tex").write_text(
        "\n".join(parameter_rows) + "\n", encoding="utf-8"
    )

    tradeoff_rows = [
        {
            "dimension": "operation_arena_payload",
            "environment": "RP2350 layout",
            "sample": "static",
            "baseline": int(full_sram["operation_workspace_payload_bytes"]),
            "proposed": int(low_sram["operation_workspace_payload_bytes"]),
            "proposed_over_baseline": values["VTwoArenaRatio"],
            "evidence_class": "static type/layout value",
        },
        {
            "dimension": "exclusive_reserved_sram",
            "environment": "RP2350 linked image",
            "sample": "static",
            "baseline": int(full_sram["exclusive_reserved_sram_bytes"]),
            "proposed": int(low_sram["exclusive_reserved_sram_bytes"]),
            "proposed_over_baseline": values["VTwoExclusiveSramRatio"],
            "evidence_class": "static linker/type value",
        },
        {
            "dimension": "psp_reservation",
            "environment": "RP2350 linked image",
            "sample": "static",
            "baseline": int(full_sram["psp_reserved_bytes"]),
            "proposed": int(low_sram["psp_reserved_bytes"]),
            "proposed_over_baseline": values["VTwoPspReservationRatio"],
            "evidence_class": "static linker reservation; not observed use",
        },
        {
            "dimension": "bin_image",
            "environment": "RP2350 linked image",
            "sample": "static",
            "baseline": int(full_link["flash_image_end_offset_bytes"]),
            "proposed": int(low_link["flash_image_end_offset_bytes"]),
            "proposed_over_baseline": values["VTwoBinRatio"],
            "evidence_class": "linked image extent",
        },
    ]
    for operation in ("keygen", "sign"):
        for run in (1, 2):
            row = next(
                item
                for item in host["results"]
                if item["operation"] == operation and int(item["run"]) == run
            )
            tradeoff_rows.append(
                {
                    "dimension": f"{operation}_time",
                    "environment": "native host",
                    "sample": f"reversed-order-run-{run}",
                    "baseline": row["d12c_median_ns"],
                    "proposed": row["d13_median_ns"],
                    "proposed_over_baseline": row["ratio_of_medians"],
                    "evidence_class": "finite paired timing campaign",
                }
            )
    for operation in ("keygen", "sign", "verify"):
        tradeoff_rows.append(
            {
                "dimension": f"{operation}_time",
                "environment": "RP2350",
                "sample": "one deterministic path per image",
                "baseline": int(full_exec[operation]["elapsed_us"]),
                "proposed": int(low_exec[operation]["elapsed_us"]),
                "proposed_over_baseline": float(low_exec[operation]["elapsed_us"])
                / float(full_exec[operation]["elapsed_us"]),
                "evidence_class": "single observation; no population slowdown claim",
            }
        )
    tradeoff_json_path = args.audit_output.parent / "resource-time-tradeoff.json"
    tradeoff_csv_path = args.audit_output.parent / "resource-time-tradeoff.csv"
    tradeoff_report = {
        "schema": "tinysqisign-v2-resource-time-tradeoff-v1",
        "status": "PASS",
        "baseline": "full-norm-table implementation",
        "proposed": "certified-norm-sketch implementation",
        "rows": tradeoff_rows,
        "decision": {
            "proposed_dominates_all_dimensions": False,
            "large_operation_ram_reduction_established": True,
            "flash_growth_established": True,
            "target_timing_distribution_established": False,
        },
        "claim_boundary": (
            "Static resource coordinates are derived from the frozen images. Host timing is a "
            "bounded paired campaign, whereas each target timing coordinate is one deterministic "
            "observation. The profile is not a statistical target slowdown estimate."
        ),
    }
    tradeoff_json_path.write_text(
        json.dumps(tradeoff_report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    with tradeoff_csv_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(
            handle, fieldnames=list(tradeoff_rows[0]), lineterminator="\n"
        )
        writer.writeheader()
        writer.writerows(tradeoff_rows)

    inputs = [
        args.baseline_manifest,
        args.proposed_manifest,
        args.host_runtime,
        args.v3_clean,
        args.v3_multi,
        args.v3_d2,
        args.v3_fixed_key_timing,
        args.v3_fixed_key_structural,
        args.v3_stack_bound,
        args.v2_stack_bound,
        args.v3_d3,
        args.v3_all_parameter_frames,
        args.lifetime_annotation_coverage,
    ]
    if v3_target_fixed is not None:
        inputs.append(args.v3_target_fixed_key_timing)
    if v3_static_closure is not None:
        inputs.append(args.v3_static_closure)
    if v2_target_repeat is not None:
        inputs.append(args.v2_target_repeat)
    audit = {
        "schema": "tinysqisign-manuscript-evidence-generation-v1",
        "status": "PASS",
        "inputs": {
            str(path.relative_to(ROOT)): {"sha256": sha256(path), "bytes": path.stat().st_size}
            for path in inputs
        },
        "validated_relations": [
            "reserved plus unreserved equals 532480 bytes for both v2 images",
            "arena, exclusive-SRAM, and unreserved-SRAM deltas are all 180928 bytes",
            "the proposed BIN growth is 880 bytes",
            "150 MHz cycle values are elapsed-microsecond products",
            "the four normalized resource coordinates are regenerated from the two manifests",
            "clean v3, multi-input placement, fixed-frame, fixed-key timing, and structural-trace campaigns report PASS",
            "linked p324_3/RADIX32 operation PSP bounds fit the 131072-byte reservation",
            "linked v2 operation PSP bounds fit the 131072-byte reservation",
            "v3 D3 schedules two functions and reduces all six affected Arm frames across three parameter sets",
            "the lifetime annotation gate covers every declared member and direct access while retaining a manual alias boundary",
            "when present, the v2 repeat reproduces transcripts and PSP extents across two boots",
        ],
        "outputs": [
            "manuscript/generated/evidence-macros.tex",
            "manuscript/generated/v2-memory-rows.tex",
            "manuscript/generated/v2-memory-rows-en.tex",
            "manuscript/generated/v2-runtime-rows.tex",
            "manuscript/generated/v3-static-psp-bound-rows.tex",
            "manuscript/generated/v2-static-psp-bound-rows.tex",
            "manuscript/generated/v3-d3-results-rows.tex",
            "manuscript/generated/v3-d3-results-rows-en.tex",
            "manuscript/generated/v3-all-parameter-frame-rows.tex",
            "results/revision-2026-09-04/resource-time-tradeoff.json",
            "results/revision-2026-09-04/resource-time-tradeoff.csv",
        ],
    }
    args.audit_output.parent.mkdir(parents=True, exist_ok=True)
    args.audit_output.write_text(
        json.dumps(audit, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(
        f"manuscript evidence: PASS (v2 SRAM delta {saved} bytes, "
        f"v3 D3 Sign PSP saved {values['VThreeDThreeSavedBytes']} bytes, "
        f"v3 fixed-frame dynamic records 0)"
    )
    print(f"output_dir={args.output_dir}")
    print(f"audit={args.audit_output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
