#!/usr/bin/env python3
"""Check frozen C anchors behind the D12c/D13 find_uv correspondence table.

This is deliberately a bounded source audit, not a C semantics proof.  It fails
closed when a frozen commit, source digest, control skeleton, failure edge, or
linked regression fixture changes.
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
SOURCE = Path("src/id2iso/ref/lvlx/dim2id2iso.c")
TEST_SOURCE = TREE / "src/id2iso/ref/lvlx/test/test_dim2id2iso.c"
REFERENCE_COMMIT = "6b79cfb5cfe1c756d7061b92038d5069bda66f72"
CANDIDATE_COMMIT = "71099e0827d3f0a3b3c705d2eda592c401e0d57d"
REFERENCE_SHA256 = "300f69982e66dca31364179d739ac75392b626badcda297f19a396ec203cfd2f"
CANDIDATE_SHA256 = "ffb99adf6b03af7f6e5a9c2fa1a14a4ee2edbccc9d0f0a911c472c6d0ab5693e"


def run(*args: str) -> str:
    return subprocess.check_output(args, cwd=ROOT, text=True).strip()


def digest(data: str) -> str:
    return hashlib.sha256(data.encode()).hexdigest()


def compact(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return re.sub(r"\s+", "", text)


def require(condition: bool, message: str, checks: list[dict[str, str]]) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")
    checks.append({"check": message, "status": "PASS"})


def has_all(text: str, snippets: list[str]) -> bool:
    normalized = compact(text)
    return all(compact(snippet) in normalized for snippet in snippets)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        type=Path,
        default=ROOT / "results/revision-2026-09-04/finduv-equivalence-check.json",
    )
    args = parser.parse_args()
    checks: list[dict[str, str]] = []

    head = run("git", "-C", str(TREE), "rev-parse", "HEAD")
    require(head == CANDIDATE_COMMIT, "candidate commit is frozen D13", checks)
    status = run("git", "-C", str(TREE), "status", "--porcelain")
    require(status == "", "candidate worktree has no tracked or untracked changes", checks)
    parent = run("git", "-C", str(TREE), "rev-parse", "HEAD^")
    require(parent == REFERENCE_COMMIT, "D12c is the direct D13 parent", checks)

    reference = run("git", "-C", str(TREE), "show", f"{REFERENCE_COMMIT}:{SOURCE}") + "\n"
    candidate = (TREE / SOURCE).read_text(encoding="utf-8")
    test_source = TEST_SOURCE.read_text(encoding="utf-8")
    require(digest(reference) == REFERENCE_SHA256, "reference source digest", checks)
    require(digest(candidate) == CANDIDATE_SHA256, "candidate source digest", checks)

    enumeration_skeleton = [
        "for (int x = -m; x <= 0; x++)",
        "for (int y = -m; y < m + 1; y++)",
        "if (x == 0 && y > 0) { break; }",
        "for (int z = -m; z < m + 1; z++)",
        "if (x == 0 && y == 0 && z > 0) { break; }",
        "for (int w = -m; w < m + 1; w++)",
        "if (x == 0 && y == 0 && z == 0 && w >= 0) { break; }",
        "if (!((x | y | z | w) & 1)) { continue; }",
        "if (x % 3 == 0 && y % 3 == 0 && z % 3 == 0 && w % 3 == 0) { continue; }",
        "if (!need_remove_symmetry || (check1 <= check2 && check1 <= check3))",
    ]
    require(has_all(reference, enumeration_skeleton), "reference row-enumeration skeleton", checks)
    require(has_all(candidate, enumeration_skeleton), "candidate row-enumeration skeleton", checks)
    require(
        has_all(reference, ["quat_qf_eval(&norm, gram, &point)", "ibz_div(&norm, &remain, &norm, adjusted_norm)", "ibz_mod_ui(&norm, 2) == 1"]),
        "reference exact norm and odd admission",
        checks,
    )
    require(
        has_all(candidate, ["quat_qf_eval(&eval->norm[slot], gram, &eval->point)", "ibz_div(&eval->norm[slot], &eval->remainder, &eval->norm[slot], adjusted_norm)", "ibz_mod_ui(&eval->norm[0], 2) == 1"]),
        "candidate exact norm and odd admission",
        checks,
    )

    require(
        has_all(reference, ["ibz_cmp(&norms[first], &norms[second])", "return (first > second) - (first < second)"]),
        "reference total key is full norm then enumeration index",
        checks,
    )
    require(
        has_all(candidate, ["bit_length[first]", "leading[first]", "finduv_eval_norm(context->eval, 0", "finduv_eval_norm(context->eval, 1", "ibz_cmp(&context->eval->norm[0], &context->eval->norm[1])", "return (first > second) - (first < second)"]),
        "candidate total key is certified sketch, exact replay, then index",
        checks,
    )
    require(
        has_all(candidate, ["context->fatal = 1", "return !context->fatal", "!finduv_sort_permutation", "return ID2ISO_STATUS_FATAL"]),
        "sort replay failure cannot publish fallback order",
        checks,
    )

    triangular = [
        "for (int j1 = 0; j1 < num_alternate_order + 1; j1++)",
        "for (int j2 = j1; j2 < num_alternate_order + 1; j2++)",
        "int is_diago = (j1 == j2)",
    ]
    require(has_all(reference, triangular), "reference triangular order-pair schedule", checks)
    require(has_all(candidate, triangular), "candidate triangular order-pair schedule", checks)

    index_schedule = [
        "for (int i1 = 0; i1 < index1; i1++)",
        "starting_index2 = i1",
        "starting_index2 = 0",
        "for (int i2 = starting_index2; i2 < index2; i2++)",
        "while (!found && cmp < 0)",
    ]
    require(has_all(reference, index_schedule), "reference row-index schedule", checks)
    require(has_all(candidate, index_schedule), "candidate row-index schedule", checks)
    require(
        has_all(reference, ["ibz_invmod(&remain, &small_norms2[i2], &small_norms1[i1])", "ibz_mod(v, v, &small_norms1[i1])", "ibz_add(v, v, &small_norms1[i1])"]),
        "reference congruence representative and stride",
        checks,
    )
    require(
        has_all(candidate, ["ibz_invmod(&remain, small_norm2, small_norm1)", "ibz_mod(v, v, small_norm1)", "ibz_add(v, v, small_norm1)"]),
        "candidate congruence representative and stride",
        checks,
    )
    require(
        has_all(candidate, ["is_diago, 0, &workspace->phase.candidates.eval"]),
        "production pair search disables optional sum-of-squares filter",
        checks,
    )

    terminal = [
        "if (list_status < 0)",
        "goto cleanup_workspace_fatal",
        "result = ID2ISO_STATUS_FATAL",
        "find_uv_workspace_clear(workspace)",
        "return result",
    ]
    require(has_all(reference, terminal), "reference status propagation and terminal clear", checks)
    require(has_all(candidate, terminal), "candidate status propagation and terminal clear", checks)
    require(
        has_all(test_source, ["invalid_status == ID2ISO_STATUS_FATAL", "isogeny_status == 0", "continue", "find_uv_workspace_is_clear", "find_uv_guards_are_intact"]),
        "runtime fixture covers Fatal, ordinary miss/retry, guards, and clearing",
        checks,
    )

    output = {
        "schema": "sqisign-finduv-equivalence-check-v1",
        "status": "PASS",
        "scope": "bounded frozen-source correspondence audit; not formal whole-program equivalence",
        "reference_commit": REFERENCE_COMMIT,
        "candidate_commit": CANDIDATE_COMMIT,
        "checks": checks,
    }
    output_path = args.output if args.output.is_absolute() else ROOT / args.output
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(output, indent=2) + "\n", encoding="utf-8")
    print(f"find_uv equivalence certificate: PASS ({len(checks)} checks)")
    print(output_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
