#!/usr/bin/env python3
"""Build an auditable register for every manuscript future-work item."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def load(path: Path) -> dict[str, object]:
    return json.loads(path.read_text(encoding="utf-8"))


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def evidence(path: Path) -> dict[str, object]:
    require(path.is_file(), f"missing evidence: {path}")
    return {
        "path": str(path.relative_to(ROOT)),
        "bytes": path.stat().st_size,
        "sha256": sha256(path),
    }


def tex_escape(text: str) -> str:
    replacements = {
        "&": r"\&",
        "%": r"\%",
        "_": r"\_",
        "#": r"\#",
    }
    return "".join(replacements.get(character, character) for character in text)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--require-v2-repeat", action="store_true")
    parser.add_argument("--require-local-complete", action="store_true")
    parser.add_argument(
        "--output",
        type=Path,
        default=ROOT / "results/revision-2026-09-04/future-work-status.json",
    )
    parser.add_argument(
        "--markdown-output",
        type=Path,
        default=ROOT / "results/revision-2026-09-04/FUTURE_WORK_STATUS.md",
    )
    parser.add_argument(
        "--tex-output",
        type=Path,
        default=ROOT / "manuscript/generated/future-work-rows.tex",
    )
    args = parser.parse_args()

    paths = {
        "v2_repeat": ROOT / "results/rp2350/ksv-d13-repeat-2026-09-04-summary.json",
        "v2_capture": ROOT / "results/rp2350/ksv-d13-repeat-2026-09-04.txt",
        "v3_multi": ROOT
        / "results/v3/rp2350/multi-input-placement-clean-2026-09-04/summary.json",
        "v3_timing": ROOT
        / "results/v3/rp2350/multi-input-placement-clean-2026-09-04/timing-screen.json",
        "v3_fixed_timing": ROOT
        / "results/host/v3-fixed-key-timing-2026-09-04/summary.json",
        "v3_structural": ROOT
        / "results/host/v3-fixed-key-structural-trace-2026-09-04/summary.json",
        "v3_target_fixed_timing": ROOT
        / "results/v3/rp2350/fixed-key-timing-clean-2026-09-04/summary.json",
        "v3_target_fixed_design": ROOT
        / "experiments/sca/v3-rp2350-fixed-key-timing-design.json",
        "v3_d2": ROOT / "results/v3/rp2350/d2-static-2026-09-04/summary.json",
        "v3_bound": ROOT
        / "results/v3/analysis/d2-linked-stack-bound-audit-2026-09-04.json",
        "v3_async_bound": ROOT
        / "results/v3/analysis/d2-async-stack-closure-audit-2026-09-04.json",
        "v3_static_closure": ROOT
        / "results/v3/rp2350/static-closure-clean-2026-09-04/summary.json",
        "layout": ROOT
        / "results/revision-2026-09-04/generated-finduv-layout/layout.json",
        "lifetime_check": ROOT
        / "results/revision-2026-09-04/lifetime-certificate-check.json",
        "equivalence_check": ROOT
        / "results/revision-2026-09-04/finduv-equivalence-check.json",
        "manuscript_evidence": ROOT
        / "results/revision-2026-09-04/manuscript-evidence.json",
        "pareto": ROOT / "results/revision-2026-09-04/resource-time-pareto.json",
        "literature": ROOT / "results/literature/related-work-search-2026-09-04.md",
        "sca_combined": ROOT / "experiments/sca/combined-sign-protection-contract.json",
        "sca_residual": ROOT / "experiments/sca/combined-residual-trace-contract.json",
        "sca_inventory": ROOT / "experiments/sca/residual-surface-inventory.json",
        "sca_pareto": ROOT / "results/rp2350/sca-d13-powmod-fixedwork-pareto.json",
        "analog": ROOT / "results/rp2350/sca-analog-readiness-2026-09-04.json",
    }

    v3_multi = load(paths["v3_multi"])
    require(v3_multi.get("all_trials_passed") is True, "v3 multi campaign failed")
    require(v3_multi.get("all_trees_clean") is True, "v3 multi trees are dirty")
    require(v3_multi.get("placement_psp_mismatch_count") == 0, "v3 placement mismatch")
    design = v3_multi["design"]
    require(design["positive_ksv_trials"] == 40, "unexpected v3 positive-trial count")
    require(design["negative_verify_trials"] == 40, "unexpected v3 negative-trial count")

    v3_timing = load(paths["v3_timing"])
    require(
        v3_timing["decision"]["repeatable_input_associated_sign_timing_observed"]
        is True,
        "v3 input-associated timing was not reproduced",
    )
    require(
        v3_timing["decision"]["side_channel_resistance_established"] is False,
        "v3 timing screen must not certify side-channel resistance",
    )
    v3_fixed_timing = load(paths["v3_fixed_timing"])
    require(v3_fixed_timing.get("status") == "PASS", "v3 fixed-key timing failed")
    require(v3_fixed_timing["raw_csv"]["rows"] == 1200, "bad fixed-key row count")
    require(
        v3_fixed_timing["decision"]["repeatable_fixed_key_associated_host_timing_observed"]
        is True,
        "fixed-key timing association did not reproduce",
    )
    require(
        v3_fixed_timing["decision"]["side_channel_resistance_established"]
        is False,
        "fixed-key timing must not certify resistance",
    )
    v3_structural = load(paths["v3_structural"])
    require(v3_structural.get("status") == "PASS", "v3 structural trace failed")
    require(
        v3_structural["decision"]["same_key_negative_controls_stable"] is True,
        "v3 structural negative controls are unstable",
    )
    require(
        v3_structural["decision"]["side_channel_resistance_established"] is False,
        "v3 structural trace must not certify resistance",
    )
    require(
        v3_structural["decision"]["all_primary_control_flow_event_counts_differ"]
        is True
        and v3_structural["decision"][
            "all_primary_effective_address_event_counts_differ"
        ]
        is True,
        "v3 structural differences rely only on stream digests",
    )
    structural_edge = v3_structural["decision"][
        "repeatable_fixed_key_associated_control_flow_observed"
    ]
    structural_address = v3_structural["decision"][
        "repeatable_fixed_key_associated_effective_address_observed"
    ]
    structural_sentence = (
        "host構造traceを各実装2 processで実行し、同一鍵control 8組は一致。"
        f"鍵対応の制御差={str(structural_edge).lower()}、"
        f"address差={str(structural_address).lower()}。"
    )

    v3_d2 = load(paths["v3_d2"])
    require(v3_d2.get("status") == "PASS", "v3 D2 experiment failed")
    require(v3_d2["stack_usage"]["baseline_dynamic_records"] == 19, "bad baseline")
    require(v3_d2["stack_usage"]["d2_dynamic_records"] == 0, "D2 remains dynamic")
    v3_bound = load(paths["v3_bound"])
    require(
        v3_bound.get("status") == "PSP_BOUND_ESTABLISHED",
        "linked operation PSP bound was not established",
    )
    require(
        v3_bound.get("schema") == "sqisign-v3-linked-stack-bound-audit-v4",
        "stale linked stack-bound audit schema",
    )
    require(
        v3_bound["decision"]["all_recursive_sccs_have_rank_bound"] is True,
        "recursive rank bounds are incomplete",
    )
    require(
        len(v3_bound["unique_recursive_sccs"]) == 3,
        "unexpected recursive SCC inventory",
    )
    require(
        v3_bound["decision"]["whole_program_worst_case_stack_bound_established"]
        is False,
        "operation PSP audit must not claim an asynchronous whole-program bound",
    )
    require(
        v3_bound["decision"]["operation_psp_bounds_established"] is True,
        "operation PSP bounds are incomplete",
    )
    require(
        v3_bound["architectural_psp_exception_certificate"]["psp_allowance_bytes"]
        == 212
        and v3_bound["architectural_psp_exception_certificate"][
            "linked_psp_write_sites_confined_to_trampoline"
        ]
        is True,
        "architectural PSP exception allowance changed",
    )
    expected_bounds = {
        "keygen_thunk": 108300,
        "sign_thunk": 127932,
        "verify_thunk": 40468,
    }
    for root, expected_bound in expected_bounds.items():
        row = v3_bound["static_psp_bounds"][root]
        require(row["bound_bytes"] == expected_bound, f"unexpected {root} PSP bound")
        require(row["fits_reservation"] is True, f"{root} does not fit PSP reservation")
    require(
        all(report["missing_stack_metadata_count"] == 0 for report in v3_bound["roots"].values()),
        "reachable stack metadata is not closed",
    )
    require(
        all(report["indirect_callsite_count"] == 0 for report in v3_bound["roots"].values()),
        "reachable indirect callsites are not closed",
    )

    v3_async_bound = load(paths["v3_async_bound"])
    require(
        v3_async_bound.get("status") == "PARTIAL_BLOCKERS_ENUMERATED",
        "unexpected asynchronous stack audit status",
    )
    require(
        v3_async_bound.get("schema") == "sqisign-v3-async-stack-closure-audit-v1",
        "stale asynchronous stack audit schema",
    )
    require(
        v3_async_bound["aggregate"]["candidate_root_count"] == 4,
        "unexpected asynchronous candidate-root count",
    )
    require(
        v3_async_bound["aggregate"]["unique_missing_stack_metadata_count"] == 0,
        "asynchronous direct-call stack metadata is incomplete",
    )
    require(
        v3_async_bound["aggregate"]["unique_unresolved_indirect_callsite_count"] == 18,
        "asynchronous callback-site inventory changed",
    )
    require(
        v3_async_bound["decision"]["whole_program_worst_case_stack_bound_established"]
        is False,
        "asynchronous audit must not claim a whole-program bound",
    )

    static_closure_evidence = [
        evidence(paths["v3_d2"]),
        evidence(paths["v3_bound"]),
        evidence(paths["v3_async_bound"]),
    ]
    if paths["v3_static_closure"].is_file():
        v3_static_closure = load(paths["v3_static_closure"])
        require(v3_static_closure.get("status") == "PASS", "static closure target run failed")
        require(
            v3_static_closure["validation"]["observed_psp_within_static_bounds"] is True,
            "target PSP exceeded its linked bound",
        )
        require(
            v3_static_closure["validation"]["whole_program_worst_case_stack_bound_established"]
            is False,
            "static closure must retain its MSP/interrupt boundary",
        )
        static_closure_evidence.append(evidence(paths["v3_static_closure"]))
        static_target_sentence = "clean closure imageの公式vector 0でも全観測PSPが上界内。"
    else:
        static_target_sentence = "clean closure imageの実機cross-checkは測定待ち。"

    target_fixed_design = load(paths["v3_target_fixed_design"])
    require(
        target_fixed_design.get("status") == "FROZEN_BEFORE_TARGET_CAPTURE"
        and target_fixed_design["design"].get("total_sign_samples") == 200,
        "RP2350 fixed-key timing design is not frozen",
    )
    target_fixed_evidence: list[dict[str, object]] = [
        evidence(paths["v3_target_fixed_design"])
    ]
    target_fixed_complete = False
    if paths["v3_target_fixed_timing"].is_file():
        v3_target_fixed = load(paths["v3_target_fixed_timing"])
        require(v3_target_fixed.get("status") == "PASS", "RP2350 fixed-key timing failed")
        target_fixed_detected = v3_target_fixed["decision"].get(
            "repeatable_fixed_key_associated_rp2350_timing_observed"
        )
        require(
            isinstance(target_fixed_detected, bool),
            "RP2350 fixed-key timing decision is not Boolean",
        )
        require(
            v3_target_fixed["decision"]["side_channel_resistance_established"] is False,
            "RP2350 timing must not certify resistance",
        )
        target_fixed_evidence.append(evidence(paths["v3_target_fixed_timing"]))
        target_fixed_complete = True
        target_fixed_samples = int(v3_target_fixed["design"]["total_sign_samples"])
        require(target_fixed_samples == 200, "unexpected RP2350 fixed-key sample count")
        if target_fixed_detected:
            target_fixed_sentence = (
                f"RP2350でも同じ10鍵・固定message/RNGの{target_fixed_samples} Signを測定し、"
                "二順序で鍵対応timingを検出した。"
            )
        else:
            target_fixed_sentence = (
                f"RP2350でも同じ10鍵・固定message/RNGの{target_fixed_samples} Signを測定したが、"
                "事前規則を満たす反復可能な鍵対応timingは検出しなかった。"
            )
    else:
        target_fixed_sentence = "RP2350固定鍵screenは測定待ち。"

    layout = load(paths["layout"])
    require(layout.get("status") == "PASS", "lifetime layout generation failed")
    require(layout.get("extent_bytes") == 172080, "generated layout extent changed")
    require(len(layout.get("interference_edges", [])) == 3, "bad interference graph")
    lifetime_check = load(paths["lifetime_check"])
    equivalence_check = load(paths["equivalence_check"])
    manuscript_evidence = load(paths["manuscript_evidence"])
    pareto = load(paths["pareto"])
    require(lifetime_check.get("status") == "PASS", "lifetime certificate failed")
    require(equivalence_check.get("status") == "PASS", "equivalence audit failed")
    require(manuscript_evidence.get("status") == "PASS", "manuscript evidence failed")
    require(pareto.get("status") == "PASS", "Pareto evidence generation failed")
    require(
        pareto["decision"]["proposed_dominates_all_dimensions"] is False,
        "Pareto profile must retain the flash/time trade-off",
    )

    literature_text = paths["literature"].read_text(encoding="utf-8")
    require("2026-09-04" in literature_text, "literature freeze date is absent")
    require("no indexed public item met all six" in literature_text, "decision is absent")

    sca_combined = load(paths["sca_combined"])
    sca_residual = load(paths["sca_residual"])
    sca_inventory = load(paths["sca_inventory"])
    sca_pareto = load(paths["sca_pareto"])
    require(
        sca_combined.get("status")
        == "combined_software_protection_verified_physical_pending",
        "unexpected combined-protection status",
    )
    require(
        sca_residual.get("status")
        == "residual_control_and_address_differences_reproduced_physical_pending",
        "unexpected residual-trace status",
    )
    require(len(sca_inventory.get("surfaces", [])) == 12, "bad residual-surface count")
    require(
        sca_inventory["decision"]["side_channel_resistant"] is False,
        "SCA inventory must not claim resistance",
    )
    require(
        sca_pareto["decision"]["fixedwork_has_fixed_source_level_operation_schedule"]
        is True,
        "fixed-work schedule evidence is absent",
    )
    require(
        sca_pareto["decision"]["full_sign_resistance_established"] is False,
        "fixed-work prototype must not certify full Sign",
    )

    analog = load(paths["analog"])
    require(
        analog.get("schema") == "sqisign-sca-analog-readiness-v3",
        "unexpected analog readiness schema",
    )
    require(
        analog["machine_observation"][
            "only_pico_target_and_ordinary_usb_infrastructure_detected"
        ]
        is True,
        "readiness inventory no longer describes the recorded host",
    )
    require(
        analog["decision"]["analog_capture_run"] is False,
        "readiness record unexpectedly claims an analog capture",
    )
    require(
        analog["decision"]["physical_trace_count"] == 0
        and analog["decision"]["external_hardware_blocker_recorded"] is True,
        "readiness record does not establish the external hardware blocker",
    )
    require(
        analog["decision"]["side_channel_resistance_established"] is False,
        "analog readiness must not certify resistance",
    )

    if paths["v2_repeat"].is_file():
        v2_repeat = load(paths["v2_repeat"])
        require(v2_repeat.get("status") == "PASS", "v2 repeat analysis failed")
        require(v2_repeat["design"]["boot_count"] == 2, "bad v2 boot count")
        require(v2_repeat["design"]["distinct_input_count"] == 1, "bad input count")
        require(
            v2_repeat["decision"]["worst_case_stack_bound_established"] is False,
            "v2 repeat must not certify worst-case stack",
        )
        v2_status = "bounded_complete"
        v2_label = "限定完了"
        v2_finding = (
            "同一UF2・同一決定的入力を2 bootで完走し、transcriptとPSP深さが一致。"
            "複数入力・最悪上界ではない。"
        )
        v2_evidence = [evidence(paths["v2_repeat"])]
    else:
        require(not args.require_v2_repeat, "required v2 repeat summary is absent")
        capture_text = paths["v2_capture"].read_text(encoding="ascii")
        require(capture_text.startswith("SQISIGN_RP2350_KSV_D13 v1"), "v2 run not started")
        v2_status = "in_progress"
        v2_label = "測定中"
        v2_finding = "同一UF2による第2 bootを実行中。完了前のcaptureは結果に用いない。"
        v2_evidence = [evidence(paths["v2_capture"])]

    static_closure_complete = paths["v3_static_closure"].is_file()
    local_bounded_complete = (
        v2_status == "bounded_complete"
        and static_closure_complete
        and target_fixed_complete
    )
    require(
        not args.require_local_complete or local_bounded_complete,
        "one or more locally executable bounded campaigns are incomplete",
    )

    items = [
        {
            "id": "FW-V2-TARGET-REPEAT",
            "work": "v2実機反復",
            "status": v2_status,
            "label_ja": v2_label,
            "finding": v2_finding,
            "evidence": v2_evidence,
        },
        {
            "id": "FW-V3-MULTI-PLACEMENT",
            "work": "v3複数入力・複数配置",
            "status": "bounded_complete",
            "label_ja": "限定完了",
            "finding": (
                "10公式ベクトルを2実装×2配置で検査。正当K/S/V 40件と改変拒否40件が通り、"
                "1024-byte配置移動後もPSP深さの不一致は0/80。"
            ),
            "evidence": [evidence(paths["v3_multi"]), evidence(paths["v3_timing"])],
        },
        {
            "id": "FW-V3-STACK-BOUND",
            "work": "v3 stack上界",
            "status": "bounded_complete" if static_closure_complete else "in_progress",
            "label_ja": "限定完了" if static_closure_complete else "測定中",
            "finding": (
                "固定frame試作で動的frame記録を19件から0件へ変更したが、実測PSPは減らなかった。"
                "link済みK/S/V根のsoftware callと最大例外entryを含むPSP上界は"
                "108300/127932/40468 bytesで、"
                "全て128 KiB予約内。" + static_target_sentence
                + "割込み候補4根の直接call metadataは閉じたが、handler側間接callback "
                "18地点とIRQ/MSP nestingが残るため全program上界ではない。"
            ),
            "evidence": static_closure_evidence,
        },
        {
            "id": "FW-AUTOMATION",
            "work": "配置・図表の自動生成",
            "status": "prototype_complete",
            "label_ja": "試作完了",
            "finding": (
                "annotation駆動配置生成器が172080 bytesを再現し、証拠生成器がSRAM・flash・"
                "cycle関係とPareto座標を再計算する。annotation自体の完全性は人手監査に依存する。"
            ),
            "evidence": [
                evidence(paths["layout"]),
                evidence(paths["manuscript_evidence"]),
                evidence(paths["pareto"]),
            ],
        },
        {
            "id": "FW-LITERATURE",
            "work": "v3公開後の文献再監査",
            "status": "complete",
            "label_ja": "完了",
            "finding": (
                "2026-09-04時点の検索式・データベース・除外基準・最近接反例を保存。"
                "否定的検索結果はv2の六条件の論理積だけに限定した。"
            ),
            "evidence": [evidence(paths["literature"])],
        },
        {
            "id": "FW-V2-SCA-HARDENING",
            "work": "v2固定work・漏洩再検査",
            "status": "completed_negative_result",
            "label_ja": "実験完了（負）",
            "finding": (
                "固定budget samplerとCornacchia固定scheduleを統合した。"
                "全Signの制御・address差は再現し、残存面12件が開いているため耐性は不成立。"
            ),
            "evidence": [
                evidence(paths["sca_combined"]),
                evidence(paths["sca_residual"]),
                evidence(paths["sca_inventory"]),
                evidence(paths["sca_pareto"]),
            ],
        },
        {
            "id": "FW-V3-SCA",
            "work": "v3漏洩評価",
            "status": (
                "completed_negative_result" if target_fixed_complete else "in_progress"
            ),
            "label_ja": "実験完了（負）" if target_fixed_complete else "測定中",
            "finding": (
                "targetの10入力順位に加え、hostでmessage・署名乱数を固定した10鍵×30反復×"
                "2順序を実施し、公式版とv3適応版の双方で鍵別順位を再現した。"
                + target_fixed_sentence
                + structural_sentence
                + "物理漏洩は未判定。"
            ),
            "evidence": [
                evidence(paths["v3_timing"]),
                evidence(paths["v3_fixed_timing"]),
                evidence(paths["v3_structural"]),
            ] + target_fixed_evidence,
        },
        {
            "id": "FW-PHYSICAL-SCA",
            "work": "電力・EM実験",
            "status": "external_hardware_blocker",
            "label_ja": "外部機材待ち",
            "finding": (
                "取得firmware・解析器・合成positive controlは準備済み。"
                "電流/EM probeとscope/SCA装置が未接続で、物理traceは0件。"
            ),
            "evidence": [evidence(paths["analog"])],
        },
        {
            "id": "FW-CROSS-SCHEME-MCU",
            "work": "他PQC・複数マイコン",
            "status": "deferred_by_author_scope",
            "label_ja": "対象外",
            "finding": "著者方針に従い本改訂では実施しない。一般性の主張にも用いない。",
            "evidence": [],
        },
    ]

    report = {
        "schema": "tinysqisign-future-work-execution-register-v3",
        "freeze_date": "2026-09-04",
        "status": (
            "BOUNDED_LOCAL_PROGRAM_COMPLETE_WITH_EXTERNAL_HARDWARE_BLOCKER"
            if local_bounded_complete
            else "IN_PROGRESS_WITH_EXTERNAL_HARDWARE_BLOCKER"
        ),
        "status_definitions": {
            "complete": "the planned bounded check completed",
            "bounded_complete": "the planned finite campaign completed without a general proof claim",
            "prototype_complete": "the tool/prototype target completed; soundness boundary remains explicit",
            "partial": "a bounded first experiment completed and named work remains",
            "completed_negative_result": "the bounded experiment completed but the target property did not hold",
            "in_progress": "the experiment started but has no terminal result",
            "external_hardware_blocker": "software is ready but required acquisition hardware is absent",
            "deferred_by_author_scope": "the author explicitly excluded the item from this revision",
        },
        "items": items,
        "decision": {
            "all_listed_items_accounted_for": True,
            "all_locally_executable_bounded_campaigns_started": True,
            "all_locally_executable_bounded_campaigns_completed": local_bounded_complete,
            "all_research_goals_achieved": False,
            "linked_synchronous_thread_mode_operation_psp_bounds_established": True,
            "whole_program_worst_case_stack_bound_established": False,
            "side_channel_resistance_established": False,
            "physical_power_or_em_campaign_run": False,
        },
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.markdown_output.parent.mkdir(parents=True, exist_ok=True)
    args.tex_output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    markdown = [
        "# 今後の研究工程：実施状況台帳",
        "",
        "Freeze date: **2026-09-04 (Asia/Tokyo)**",
        "",
        (
            "この台帳は、論文の旧「今後の研究工程」に列挙した項目を一件ずつ追跡します。"
            "有限の試験を完了したことと、一般的な性質を証明したことを区別します。"
        ),
        "",
        "| ID | 工程 | 状態 | 結果と境界 |",
        "|---|---|---|---|",
    ]
    for item in items:
        markdown.append(
            f"| `{item['id']}` | {item['work']} | {item['label_ja']} | {item['finding']} |"
        )
    markdown.extend(
        [
            "",
            "## 総合判定",
            "",
            (
                (
                    "ローカルで実行できる有限の工程はすべて完了しました。"
                    if local_bounded_complete
                    else "ローカルで実行できる有限の工程にはすべて着手しました。"
                )
                + "link済みK/S/Vのsoftware callと最大例外entryを含むPSP上界は成立しましたが、"
                "handler call・非同期割込み/MSP nestingを含む"
                "全プログラム最悪スタック上界とサイドチャネル耐性は成立していません。"
                "物理電力・電磁波実験は取得機材がないため実行していません。"
            ),
            "",
            "機械可読な状態と各証拠のSHA-256は `future-work-status.json` に保存します。",
        ]
    )
    args.markdown_output.write_text("\n".join(markdown) + "\n", encoding="utf-8")

    tex_lines = ["% Generated by scripts/generate_future_work_status.py; do not edit."]
    for item in items:
        tex_lines.append(
            f"{tex_escape(item['work'])} & {tex_escape(item['label_ja'])} & "
            f"{tex_escape(item['finding'])} \\\\"
        )
    tex_lines[-1] = tex_lines[-1][:-3]
    args.tex_output.write_text("\n".join(tex_lines) + "\n", encoding="utf-8")
    print(f"future-work register: {report['status']} ({len(items)} items)")
    print(f"json={args.output}")
    print(f"markdown={args.markdown_output}")
    print(f"tex={args.tex_output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
