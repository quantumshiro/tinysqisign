#!/usr/bin/env python3
"""Materialize a p324_3 generated-source prototype without C VLAs.

This is an experiment-specific source-to-source pass over the official
generated m4f package.  Every replacement is fail-closed against exact source
anchors.  Bounds are p324_3/RADIX32 constants and are asserted at each public
or recursive entry that previously sized a VLA from an argument.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
from pathlib import Path


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def replace_exact(text: str, old: str, new: str, expected: int, label: str) -> str:
    count = text.count(old)
    if count != expected:
        raise SystemExit(f"FAIL: {label}: expected {expected} source anchor(s), found {count}")
    return text.replace(old, new)


def transform_simple(path: Path, replacements: list[tuple[str, str, int]], insert: tuple[str, str]) -> None:
    text = path.read_text(encoding="utf-8")
    anchor, declarations = insert
    text = replace_exact(text, anchor, anchor + declarations, 1, f"{path.name}: declarations")
    for old, new, count in replacements:
        text = replace_exact(text, old, new, count, f"{path.name}: {old.strip()}")
    path.write_text(text, encoding="utf-8")


def transform_mp(path: Path) -> None:
    text = path.read_text(encoding="utf-8")
    macro_anchor = "#define NUM_LIMBS(bits) ((int)(((bits) + (NUM_BITS_LIMB - 1)) / (NUM_BITS_LIMB)))\n"
    macros = """

/* p324_3/RADIX32 bounds used by the static-stack audit prototype. */
#define V3_STATIC_MP_LIMBS IBZ_NLIMBS
#define V3_STATIC_MP_LIMBS_PLUS_2 (IBZ_NLIMBS + 2)
#define V3_STATIC_MP_DOUBLE_PLUS_4 (2 * (IBZ_NLIMBS + 2))
#define V3_STATIC_MP_TRIPLE_PLUS_6 (3 * (IBZ_NLIMBS + 2))
#define V3_STATIC_ULONG_LIMBS 2
#define V3_STATIC_REQUIRE(condition) do { if (!(condition)) __builtin_trap(); } while (0)
_Static_assert(IBZ_NLIMBS == 60, "prototype is frozen to p324_3/RADIX32");
_Static_assert(sizeof(unsigned long int) <= 8, "unexpected unsigned-long width");
"""
    text = replace_exact(text, macro_anchor, macro_anchor + macros, 1, "mp.c: bound macros")

    one_limb = {
        "vlen",
        "ulen + 1",
        "n",
        "n2 + 1",
        "un_len",
        "n + 1",
        "qbuf_len",
        "ulen - vlen + 1",
        "reslen",
        "len1 + 1",
        "len2 + 1",
        "xlen + 1",
        "L",
        "m + 1",
    }
    double_limb = {
        "2 * n + 1",
        "n + n2",
        "2 * n",
        "next",
        "2 * m + 2",
        "2 * (m + 1)",
        "2 * (m + 1) + 2",
        "2 * m",
    }
    triple_limb = {"3 * (m + 1)"}
    ulong_limb = {"((int)(8 * sizeof(unsigned long int)) + NUM_BITS_LIMB - 1) / NUM_BITS_LIMB"}
    replaced = 0
    output_lines: list[str] = []
    declaration = re.compile(r"^(\s*digit_t\s+.*;)(\s*(?://.*)?)$")
    bracket = re.compile(r"\[([^\]]+)\]")
    for line in text.splitlines():
        if declaration.match(line):
            def repl(match: re.Match[str]) -> str:
                nonlocal replaced
                expression = " ".join(match.group(1).split())
                if expression in one_limb:
                    replaced += 1
                    return "[V3_STATIC_MP_LIMBS_PLUS_2]"
                if expression in double_limb:
                    replaced += 1
                    return "[V3_STATIC_MP_DOUBLE_PLUS_4]"
                if expression in triple_limb:
                    replaced += 1
                    return "[V3_STATIC_MP_TRIPLE_PLUS_6]"
                if expression in ulong_limb:
                    replaced += 1
                    return "[V3_STATIC_ULONG_LIMBS]"
                return match.group(0)
            line = bracket.sub(repl, line)
        output_lines.append(line)
    if replaced != 53:
        raise SystemExit(f"FAIL: mp.c: expected 53 array declarators, replaced {replaced}")
    text = "\n".join(output_lines) + "\n"

    assertions = [
        (
            "int qbuf_len = mp_div_qlen(ulen, vlen);\n",
            "int qbuf_len = mp_div_qlen(ulen, vlen);\n        V3_STATIC_REQUIRE(qbuf_len > 0 && qbuf_len <= V3_STATIC_MP_LIMBS_PLUS_2);\n",
            5,
        ),
        (
            "mp_div_d3n2n(digit_t *Qhat, digit_t *R, const digit_t *A, const digit_t *B, int n)\n{\n",
            "mp_div_d3n2n(digit_t *Qhat, digit_t *R, const digit_t *A, const digit_t *B, int n)\n{\n    V3_STATIC_REQUIRE(n > 0 && n <= V3_STATIC_MP_LIMBS);\n",
            1,
        ),
        (
            "mp_div_d2n1n(digit_t *Q, digit_t *R, const digit_t *A, const digit_t *B, int n)\n{\n",
            "mp_div_d2n1n(digit_t *Q, digit_t *R, const digit_t *A, const digit_t *B, int n)\n{\n    V3_STATIC_REQUIRE(n > 0 && n <= V3_STATIC_MP_LIMBS);\n",
            1,
        ),
        (
            "    assert(vlen >= 1);\n    assert(v[vlen - 1] != 0);\n",
            "    assert(vlen >= 1);\n    V3_STATIC_REQUIRE(ulen >= vlen && ulen <= V3_STATIC_MP_LIMBS);\n    V3_STATIC_REQUIRE(vlen <= V3_STATIC_MP_LIMBS);\n    assert(v[vlen - 1] != 0);\n",
            2,
        ),
        (
            "    int neg1 = (A < 0), neg2 = (B < 0);\n",
            "    V3_STATIC_REQUIRE(reslen > 0 && reslen <= V3_STATIC_MP_LIMBS_PLUS_2);\n    V3_STATIC_REQUIRE(len1 > 0 && len1 <= V3_STATIC_MP_LIMBS);\n    V3_STATIC_REQUIRE(len2 > 0 && len2 <= V3_STATIC_MP_LIMBS);\n    int neg1 = (A < 0), neg2 = (B < 0);\n",
            1,
        ),
        (
            "    int c0 = la + lb - keep - 2;\n",
            "    V3_STATIC_REQUIRE(la > 0 && la <= V3_STATIC_MP_LIMBS_PLUS_2);\n    V3_STATIC_REQUIRE(lb > 0 && lb <= V3_STATIC_MP_LIMBS_PLUS_2);\n    V3_STATIC_REQUIRE(keep <= V3_STATIC_MP_DOUBLE_PLUS_4);\n    int c0 = la + lb - keep - 2;\n",
            1,
        ),
    ]
    for old, new, count in assertions:
        text = replace_exact(text, old, new, count, "mp.c: bound assertion")
    path.write_text(text, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    args = parser.parse_args()
    source = args.input.resolve()
    output = args.output.resolve()
    manifest_path = args.manifest.resolve()
    if output.exists():
        raise SystemExit(f"FAIL: output already exists: {output}")
    shutil.copytree(source, output)
    targets = ["isog.c", "theta_isogenies.c", "fp2.c", "gluing.c", "biextension.c", "encode_secret.c", "mp.c"]
    before = {name: digest(source / name) for name in targets}

    transform_simple(
        output / "isog.c",
        [
            ("    ec_point_t splits[space];", "    ec_point_t splits[V3_STATIC_CHAIN_SPACE];", 1),
            ("    uint16_t todo[space];", "    uint16_t todo[V3_STATIC_CHAIN_SPACE];", 1),
            ("    for (int i = 1; i < isog_len; i *= 2)\n        ++space;", "    for (int i = 1; i < isog_len; i *= 2)\n        ++space;\n    V3_STATIC_REQUIRE(space <= V3_STATIC_CHAIN_SPACE);", 1),
            ("{\n    ec_curve_normalize_A24(curve);", "{\n    V3_STATIC_REQUIRE(isog_len > 0 && isog_len <= V3_STATIC_MAX_ISOG_LEN);\n    ec_curve_normalize_A24(curve);", 1),
        ],
        ("#include <assert.h>\n", "\n#define V3_STATIC_REQUIRE(condition) do { if (!(condition)) __builtin_trap(); } while (0)\nenum { V3_STATIC_MAX_ISOG_LEN = 326, V3_STATIC_CHAIN_SPACE = 10 };\n"),
    )
    transform_simple(
        output / "theta_isogenies.c",
        [
            ("    theta_point_t pts[numP ? numP : 1];", "    theta_point_t pts[V3_STATIC_MAX_PUSH_POINTS];", 1),
            ("    uint16_t todo[space];", "    uint16_t todo[V3_STATIC_CHAIN_SPACE];", 1),
            ("    theta_point_t thetaQ1[space], thetaQ2[space];", "    theta_point_t thetaQ1[V3_STATIC_CHAIN_SPACE], thetaQ2[V3_STATIC_CHAIN_SPACE];", 1),
            ("{\n    ec_curve_normalize_A24(&E12->E1);", "{\n    V3_STATIC_REQUIRE(n > 0 && n <= V3_STATIC_MAX_THETA_LEN);\n    V3_STATIC_REQUIRE(numP <= V3_STATIC_MAX_PUSH_POINTS);\n    ec_curve_normalize_A24(&E12->E1);", 1),
        ],
        ("#include \"splitting.h\"\n", "\n#include <assert.h>\n#define V3_STATIC_REQUIRE(condition) do { if (!(condition)) __builtin_trap(); } while (0)\nenum { V3_STATIC_MAX_THETA_LEN = 326, V3_STATIC_CHAIN_SPACE = 10, V3_STATIC_MAX_PUSH_POINTS = 3 };\n"),
    )
    transform_simple(
        output / "fp2.c",
        [
            ("    fp2_t t1[len], t2[len];", "    fp2_t t1[V3_STATIC_FP2_BATCH], t2[V3_STATIC_FP2_BATCH];", 1),
            ("{\n    fp2_t t1[V3_STATIC_FP2_BATCH]", "{\n    V3_STATIC_REQUIRE(len > 0 && len <= V3_STATIC_FP2_BATCH);\n    fp2_t t1[V3_STATIC_FP2_BATCH]", 1),
        ],
        ("#include <fp2.h>\n", "\n#include <assert.h>\n#define V3_STATIC_REQUIRE(condition) do { if (!(condition)) __builtin_trap(); } while (0)\nenum { V3_STATIC_FP2_BATCH = 12 };\n"),
    )
    transform_simple(
        output / "gluing.c",
        [
            ("    fp2_t mem[len];", "    fp2_t mem[V3_STATIC_FP2_BATCH];", 1),
            ("    theta_couple_point_t T_11[space], T_12[space], T_21[space], T_22[space];", "    theta_couple_point_t T_11[V3_STATIC_CHAIN_SPACE], T_12[V3_STATIC_CHAIN_SPACE], T_21[V3_STATIC_CHAIN_SPACE], T_22[V3_STATIC_CHAIN_SPACE];", 1),
            ("{\n    fp2_t mem[V3_STATIC_FP2_BATCH];", "{\n    V3_STATIC_REQUIRE(len > 0 && len <= V3_STATIC_FP2_BATCH);\n    fp2_t mem[V3_STATIC_FP2_BATCH];", 1),
            ("{\n    int current = 0;\n\n    // T_11", "{\n    V3_STATIC_REQUIRE(space > 0 && space <= V3_STATIC_CHAIN_SPACE);\n    V3_STATIC_REQUIRE(numP <= 3);\n    int current = 0;\n\n    // T_11", 1),
        ],
        ("#include \"gluing.h\"\n", "\n#include <assert.h>\n#define V3_STATIC_REQUIRE(condition) do { if (!(condition)) __builtin_trap(); } while (0)\nenum { V3_STATIC_FP2_BATCH = 12, V3_STATIC_CHAIN_SPACE = 10 };\n"),
    )
    transform_simple(
        output / "biextension.c",
        [
            ("    fp2_t pows_f[log], pows_g[log];", "    fp2_t pows_f[V3_STATIC_DLOG_LEVELS], pows_g[V3_STATIC_DLOG_LEVELS];", 1),
            ("    log += 1;\n\n    fp2_t pows_f", "    log += 1;\n    V3_STATIC_REQUIRE(log > 0 && log <= V3_STATIC_DLOG_LEVELS);\n\n    fp2_t pows_f", 1),
        ],
        ("#include <biextension.h>\n", "\n#define V3_STATIC_REQUIRE(condition) do { if (!(condition)) __builtin_trap(); } while (0)\nenum { V3_STATIC_DLOG_LEVELS = 10 };\n"),
    )
    transform_simple(
        output / "encode_secret.c",
        [
            ("    digit_t d[digits];", "    V3_STATIC_REQUIRE(digits <= V3_STATIC_ENCODE_DIGITS);\n    digit_t d[V3_STATIC_ENCODE_DIGITS];", 1),
            ("    digit_t d[ndigits];", "    V3_STATIC_REQUIRE(ndigits <= V3_STATIC_ENCODE_DIGITS);\n    digit_t d[V3_STATIC_ENCODE_DIGITS];", 1),
        ],
        ("// ibz_t: only works for positive inputs\n", "\n#define V3_STATIC_REQUIRE(condition) do { if (!(condition)) __builtin_trap(); } while (0)\nenum { V3_STATIC_ENCODE_DIGITS = 11 };\n"),
    )
    transform_mp(output / "mp.c")

    dynamic_decl = re.compile(r"^\s*[A-Za-z_][A-Za-z0-9_\s*]+\s+[A-Za-z_][A-Za-z0-9_]*\[[^\]]*[a-z][^\]]*\]", re.M)
    residual = {name: dynamic_decl.findall((output / name).read_text(encoding="utf-8")) for name in targets}
    residual = {name: rows for name, rows in residual.items() if rows}
    if residual:
        raise SystemExit(f"FAIL: residual dynamic declarators: {residual}")

    after = {name: digest(output / name) for name in targets}
    manifest = {
        "schema": "sqisign-v3-p3243-static-stack-materialization-v1",
        "status": "PASS",
        "scope": "generated p324_3/m4f source prototype; not an upstream patch or whole-program stack proof",
        "bounds": {
            "IBZ_NLIMBS_RADIX32": 60,
            "max_chain_length": 326,
            "max_chain_space": 10,
            "max_theta_push_points": 3,
            "max_fp2_batch": 12,
            "max_dlog_levels": 10,
            "max_encoded_digits": 11
        },
        "files": {name: {"before_sha256": before[name], "after_sha256": after[name]} for name in targets},
    }
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"v3 static-stack prototype materialized: PASS ({len(targets)} files)")
    print(f"output={output}")
    print(f"manifest={manifest_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
