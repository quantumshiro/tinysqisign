#!/usr/bin/env python3
"""Check source-order and validation evidence for the extended v3 overlays."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TREE = ROOT / "work/v3-lowmem-d3"
COMMIT = "874658c64aa2e20f53b1f4d696144723d558ed5c"
CERTIFICATE = ROOT / "results/v3/analysis/two-function-lifetime-certificate.csv"
HOST = ROOT / "results/v3/host/validation-all-params-2026-09-04.json"
FRAMES = ROOT / "results/v3/analysis/lifetime-all-params-2026-09-04.json"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def git(*args: str) -> str:
    return subprocess.run(
        ["git", "-C", str(TREE), *args],
        check=True,
        text=True,
        capture_output=True,
    ).stdout.strip()


def ordered(text: str, fragments: list[str], label: str) -> list[int]:
    positions: list[int] = []
    cursor = 0
    for fragment in fragments:
        position = text.find(fragment, cursor)
        if position < 0:
            raise ValueError(f"{label}: missing or reordered source anchor {fragment!r}")
        positions.append(position)
        cursor = position + len(fragment)
    return positions


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        type=Path,
        default=ROOT / "results/v3/analysis/two-function-lifetime-check.json",
    )
    args = parser.parse_args()

    if git("rev-parse", "HEAD") != COMMIT:
        raise ValueError("unexpected extended v3 commit")
    if git("status", "--porcelain", "--untracked-files=no"):
        raise ValueError("tracked extended v3 source is dirty")

    lll_path = TREE / "src/quaternion/ref/lvlx/lll/lll_dim4.c"
    sign_path = TREE / "src/signature/ref/lvlx/sign.c"
    lll = lll_path.read_text(encoding="utf-8")
    sign = sign_path.read_text(encoding="utf-8")

    lll_reduction_order = ordered(
        lll,
        [
            "quat_lll_dual_phase_t phase;",
            "lll_dim4_qlapoty_reduce(&res, &phase.reduction",
            "ibz_copy(&phase.Aout[i][3 - c], &res.Ainv_total",
        ],
        "reduction/Aout overlay",
    )
    lll_subobject_order = ordered(
        lll,
        [
            "ibz_copy(&phase.Aout[i][3 - c], &res.Ainv_total",
            "ibz_t(*Bout)[4] = res.Ainv_total;",
            "ibz_copy(&Bout[r][c], &acc);",
        ],
        "Ainv_total/Bout overlay",
    )
    sign_order = ordered(
        sign,
        [
            "protocols_sign_phase_t phase;",
            "quat_ideal_init(&phase.commitment_ideal);",
            "ibz_gcd(&tmp_int, &ideal_skchall.norm, &phase.commitment_ideal.norm);",
            "compute_dim2_isogeny_challenge(&phase.output_product",
        ],
        "commitment/output overlay",
    )

    required_static_assert = (
        "_Static_assert(sizeof(((ct_mk_result_t *)0)->Ainv_total) == "
        "sizeof(ibz_t[4][4])"
    )
    if required_static_assert not in lll:
        raise ValueError("Ainv_total/Bout extent assertion is absent")

    with CERTIFICATE.open(encoding="utf-8", newline="") as handle:
        certificate_rows = list(csv.DictReader(handle))
    if len(certificate_rows) != 3 or len({row["function"] for row in certificate_rows}) != 2:
        raise ValueError("lifetime certificate must contain three sites in two functions")
    if any(not value.strip() for row in certificate_rows for value in row.values()):
        raise ValueError("lifetime certificate contains an empty field")

    host = json.loads(HOST.read_text(encoding="utf-8"))
    frames = json.loads(FRAMES.read_text(encoding="utf-8"))
    if host.get("status") != "PASS" or host["decision"]["known_answer_vectors_passed"] != 600:
        raise ValueError("all-parameter host gate is absent or stale")
    if frames.get("status") != "PASS" or not frames["decision"]["all_six_affected_frames_reduced"]:
        raise ValueError("all-parameter Arm frame gate is absent or stale")
    if (
        host["provenance"]["two_function_lifetime"]["commit"] != COMMIT
        or frames["provenance"]["adapted_commit"] != COMMIT
    ):
        raise ValueError("validation evidence names a different adapted commit")

    result = {
        "schema": "sqisign-v3-two-function-lifetime-check-v1",
        "status": "PASS",
        "commit": COMMIT,
        "source": {
            "lll_dim4.c": sha256(lll_path),
            "sign.c": sha256(sign_path),
        },
        "certificate": {
            "path": str(CERTIFICATE.relative_to(ROOT)),
            "sha256": sha256(CERTIFICATE),
            "overlay_site_count": 3,
            "function_count": 2,
        },
        "source_order_offsets": {
            "reduction_then_Aout": lll_reduction_order,
            "Ainv_total_then_Bout": lll_subobject_order,
            "commitment_then_output": sign_order,
        },
        "validation": {
            "host": {"sha256": sha256(HOST), "official_vectors_passed": 600},
            "arm_frames": {"sha256": sha256(FRAMES), "reduced_frame_count": 6},
        },
        "decision": {
            "source_order_matches_certificate": True,
            "two_functions_covered": True,
            "three_parameter_sets_covered": True,
            "all_inputs_proved_equivalent": False,
            "whole_program_alias_analysis_performed": False,
        },
    }
    output = args.output if args.output.is_absolute() else ROOT / args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print("v3 two-function lifetime certificate: PASS")
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
