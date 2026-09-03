#!/usr/bin/env python3
"""Certify linked v3 Thread-mode operation PSP bounds and remaining limits.

This combines linked Arm disassembly with GCC ``.su`` records, source-ranked
recursion certificates, and ELF-bound manual assembly frames.  It deliberately
does not promote the operation PSP result to a whole-program bound because
asynchronous interrupt/MSP nesting remains outside the analysis.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
from collections import defaultdict, deque
from pathlib import Path


FUNCTION_RE = re.compile(r"^([0-9a-fA-F]+) <([^>]+)>:$")
DIRECT_BRANCH_RE = re.compile(
    r"\s(?P<opcode>bl|blx|b\.w|b)\s+(?:0x)?[0-9a-fA-F]+\s+<(?P<target>[^>]+)>"
)
INDIRECT_TRANSFER_RE = re.compile(
    r"\s(?P<opcode>blx(?:\.w)?|bx(?:\.w)?)\s+"
    r"(?P<register>r(?:1[0-5]|[0-9])|ip|sp|lr|pc)\b"
)
LITERAL_WORD_RE = re.compile(r"\s\.word\s+0x([0-9a-fA-F]+)\b")
CONDITIONAL_BRANCH_RE = re.compile(
    r"\s(?P<opcode>b(?:eq|ne|cs|cc|mi|pl|vs|vc|hi|ls|ge|lt|gt|le)"
    r"(?:\.[nw])?)\s+(?:0x)?(?P<address>[0-9a-fA-F]+)\s+<(?P<target>[^>]+)>"
)
INSTRUCTION_ADDRESS_RE = re.compile(r"^\s*([0-9a-fA-F]+):\s")
INDIRECT_PC_LOAD_RE = re.compile(r"\sldr(?:\.w)?\s+pc\s*,")
ROOTS = ("keygen_thunk", "sign_thunk", "verify_thunk")
CERTIFIED_CROSS_SYMBOL_CONDITIONALS = {
    ("__wrap___aeabi_dmul", "__wrap___aeabi_dsub+0x20"),
    ("__wrap___aeabi_i2d", "__wrap___aeabi_ddiv+0x94"),
    ("__wrap___aeabi_d2iz", "__wrap___aeabi_ui2d+0x1c"),
}
CERTIFIED_DCP_CONTINUATIONS = {
    "__wrap___aeabi_dmul": "__wrap___aeabi_dmul+0x6",
    "__wrap___aeabi_i2d": "__wrap___aeabi_i2d+0x6",
    "__wrap___aeabi_d2iz": "double2int_z_entry",
}

# These routines come from fixed linked assembly or newlib objects and do not
# have GCC .su records.  The byte counts are conservative maxima obtained by
# inspecting every SP-decrementing instruction in the linked function body.
# The audit binds this table to a digest of each body and checks characteristic
# instructions below; it is not portable to a different ELF.
MANUAL_FRAME_BYTES = {
    "____aeabi_uldivmod_veneer": 0,
    "____clrsbdi2_veneer": 0,
    "__memcpy_veneer": 0,
    "__memmove_veneer": 0,
    "__memset_veneer": 0,
    "__aeabi_uldivmod": 16,
    "__aeabi_idiv0": 0,
    "__udivmoddi4": 40,
    "__clrsbdi2": 0,
    "memcpy": 0,
    "memmove": 16,
    "memset": 12,
    "strlen": 0,
    # The d2iz global entry and its local double2int_z_entry label form one
    # wrapper bundle.  Forty bytes include the four-byte fallback link word and
    # generic_save_state's 32-byte save area, rounded upward to 8-byte ABI
    # alignment; no graph edge is needed through the local label.
    "__wrap___aeabi_d2iz": 40,
    # These DCP wrappers branch backwards to an assembler fallback prefix that
    # objdump attributes to the preceding symbol.  The prefix keeps the
    # original four-byte LR word live, generic_save_state reaches 32 bytes,
    # and the continuation can add an eight-byte body frame.  The exact peak
    # is at most 36 bytes; 40 conservatively preserves eight-byte alignment.
    "__wrap___aeabi_dmul": 40,
    "__wrap___aeabi_i2d": 40,
    "generic_save_state": 32,
}

MANUAL_FRAME_FRAGMENTS = {
    "____aeabi_uldivmod_veneer": ["ldr.w\tpc, [pc]", ".word\t0x"],
    "____clrsbdi2_veneer": ["ldr.w\tpc, [pc]", ".word\t0x"],
    "__memcpy_veneer": ["ldr.w\tpc, [pc]", ".word\t0x"],
    "__memmove_veneer": ["ldr.w\tpc, [pc]", ".word\t0x"],
    "__memset_veneer": ["ldr.w\tpc, [pc]", ".word\t0x"],
    "__aeabi_uldivmod": ["strd\tip, lr, [sp, #-16]!", "bl\t", "add\tsp, #16"],
    "__aeabi_idiv0": ["bx\tlr"],
    "__udivmoddi4": ["stmdb\tsp!", "sub\tsp, #8", "add\tsp, #8"],
    "__clrsbdi2": ["clz\t", "bx\tlr"],
    "memcpy": ["mov\tip, r0", "bx\tlr"],
    "memmove": ["push\t{r4, r5, r6, lr}", "pop\t{r4, r5, r6, pc}"],
    "memset": ["push\t{r4, r5, lr}", "pop\t{r4, r5, pc}"],
    "strlen": ["clz\t", "bx\tlr"],
    "__wrap___aeabi_d2iz": ["mrc2\t", "bmi.n\t", "push\t{lr}", "bl\t"],
    "__wrap___aeabi_dmul": ["push\t{r4, lr}", "push\t{lr}", "bl\t"],
    "__wrap___aeabi_i2d": ["push\t{lr}", "bl\t"],
    "generic_save_state": ["sub\tsp, #24", "push\t{r0, r1}", "blx\tlr"],
}

MANUAL_FRAME_BODY_ALIASES = {
    "__wrap___aeabi_d2iz": ("__wrap___aeabi_d2iz", "double2int_z_entry"),
}

MANUAL_SP_DECREMENT_COUNTS = {
    "____aeabi_uldivmod_veneer": 0,
    "____clrsbdi2_veneer": 0,
    "__memcpy_veneer": 0,
    "__memmove_veneer": 0,
    "__memset_veneer": 0,
    "__aeabi_uldivmod": 1,
    "__aeabi_idiv0": 0,
    "__udivmoddi4": 2,
    "__clrsbdi2": 0,
    "memcpy": 0,
    "memmove": 1,
    "memset": 1,
    "strlen": 0,
    "__wrap___aeabi_d2iz": 1,
    "__wrap___aeabi_dmul": 2,
    "__wrap___aeabi_i2d": 1,
    "generic_save_state": 2,
}


def sp_decrementing_instructions(lines: list[str]) -> list[str]:
    result = []
    for line in lines:
        instruction = line.split("\t", 2)[-1]
        if (
            re.search(r"\bpush(?:\.w)?\s", instruction)
            or re.search(r"\bstmdb(?:\.w)?\s+sp!", instruction)
            or re.search(r"\bsub(?:s|\.w)?\s+sp\s*,", instruction)
            or re.search(r"\[sp\s*,\s*#-\d+\]!", instruction)
            or re.search(r"\bvpush\s", instruction)
        ):
            result.append(instruction)
    return result


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def cmake_cache_values(path: Path) -> dict[str, str]:
    values = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line or line.startswith(("#", "//")) or "=" not in line:
            continue
        key_and_type, value = line.split("=", 1)
        key = key_and_type.split(":", 1)[0]
        values[key] = value
    return values


def parse_stack_usage(build_dir: Path) -> dict[str, list[dict[str, object]]]:
    records: dict[str, list[dict[str, object]]] = defaultdict(list)
    for path in sorted(build_dir.rglob("*.su")):
        for line in path.read_text(encoding="utf-8").splitlines():
            if not line.strip():
                continue
            fields = line.rsplit("\t", 2)
            if len(fields) != 3:
                raise ValueError(f"malformed .su row: {line!r}")
            function = fields[0].rsplit(":", 1)[-1]
            records[function].append(
                {
                    "frame_bytes": int(fields[1]),
                    "qualifiers": fields[2].split(","),
                    "record": line,
                }
            )
    return records


def parse_disassembly(
    text: str,
) -> tuple[
    dict[str, set[str]],
    dict[str, list[str]],
    set[str],
    dict[str, list[str]],
    list[dict[str, object]],
    list[dict[str, str]],
]:
    graph: dict[str, set[str]] = defaultdict(set)
    indirect: dict[str, list[str]] = defaultdict(list)
    functions: set[str] = set()
    bodies: dict[str, list[str]] = defaultdict(list)
    address_to_function: dict[int, str] = {}
    literal_tail_calls: list[tuple[str, int, str]] = []
    cross_symbol_conditionals: list[dict[str, str]] = []
    ordered_instructions: list[tuple[int, str]] = []
    current: str | None = None
    for line in text.splitlines():
        match = FUNCTION_RE.match(line)
        if match:
            current = match.group(2)
            functions.add(current)
            address_to_function[int(match.group(1), 16)] = current
            continue
        if current is None:
            continue
        instruction_address = INSTRUCTION_ADDRESS_RE.match(line)
        if instruction_address:
            ordered_instructions.append((int(instruction_address.group(1), 16), line.strip()))
        if line.strip():
            bodies[current].append(line.strip())
        direct = DIRECT_BRANCH_RE.search(line)
        if direct:
            opcode = direct.group("opcode")
            target = direct.group("target").split("+", 1)[0]
            # ``objdump`` renders ordinary branches within a function as, for
            # example, ``b ... <foo+0x2a>``.  Such an instruction is not a
            # recursive call.  Keep BL/BLX calls unconditionally, and keep a B
            # only when it is a tail call to a different linked symbol.
            if opcode in {"bl", "blx"} or target != current:
                graph[current].add(target)
        conditional = CONDITIONAL_BRANCH_RE.search(line)
        if conditional:
            target_expression = conditional.group("target")
            target = target_expression.split("+", 1)[0]
            if target != current:
                cross_symbol_conditionals.append(
                    {
                        "function": current,
                        "opcode": conditional.group("opcode"),
                        "target": target,
                        "target_expression": target_expression,
                        "target_address": f"0x{int(conditional.group('address'), 16):08x}",
                        "instruction": line.strip(),
                    }
                )
        literal = LITERAL_WORD_RE.search(line)
        if current.endswith("_veneer") and literal:
            literal_tail_calls.append(
                (current, int(literal.group(1), 16) & ~1, line.strip())
            )
        transfer = INDIRECT_TRANSFER_RE.search(line)
        if transfer:
            opcode = transfer.group("opcode").split(".", 1)[0]
            register = transfer.group("register")
            ordinary_return = opcode == "bx" and register == "lr"
            certified_continuation = (
                current == "generic_save_state"
                and opcode == "blx"
                and register == "lr"
            )
            if not ordinary_return and not certified_continuation:
                indirect[current].append(line.strip())
    resolutions = []
    for veneer, target_address, instruction in literal_tail_calls:
        target = address_to_function.get(target_address)
        if target is None:
            raise ValueError(
                f"cannot resolve {veneer} literal target 0x{target_address:08x}"
            )
        graph[veneer].add(target)
        resolutions.append(
            {
                "veneer": veneer,
                "target_address": f"0x{target_address:08x}",
                "target": target,
                "literal": instruction,
            }
        )
    instruction_index = {
        address: index for index, (address, _) in enumerate(ordered_instructions)
    }
    for record in cross_symbol_conditionals:
        pair = (record["function"], record["target_expression"])
        if pair not in CERTIFIED_CROSS_SYMBOL_CONDITIONALS:
            continue
        target_address = int(record["target_address"], 16)
        if target_address not in instruction_index:
            raise ValueError(
                f"conditional fallback target absent: {record['target_address']}"
            )
        start = instruction_index[target_address]
        prefix = [line for _, line in ordered_instructions[start : start + 3]]
        if (
            len(prefix) != 3
            or "push\t{lr}" not in prefix[0]
            or "bl\t" not in prefix[1]
            or "<generic_save_state>" not in prefix[1]
            or "b.n\t" not in prefix[2]
            or f"<{CERTIFIED_DCP_CONTINUATIONS[record['function']]}>" not in prefix[2]
        ):
            raise ValueError(
                f"DCP fallback prefix changed for {record['function']}: {prefix}"
            )
        record["fallback_prefix_instructions"] = prefix
        record["fallback_peak_derivation"] = (
            "max(4-byte saved LR + 32-byte generic_save_state, "
            "4-byte saved LR + 24-byte retained state + 8-byte body) "
            "<= 36 bytes; manual frame bound rounds this to 40 bytes"
        )
    return (
        graph,
        indirect,
        functions,
        bodies,
        resolutions,
        cross_symbol_conditionals,
    )


def reachable(graph: dict[str, set[str]], root: str) -> set[str]:
    seen: set[str] = set()
    pending = [root]
    while pending:
        node = pending.pop()
        if node in seen:
            continue
        seen.add(node)
        pending.extend(graph.get(node, ()))
    return seen


def strongly_connected_components(
    graph: dict[str, set[str]], nodes: set[str]
) -> list[list[str]]:
    index = 0
    indexes: dict[str, int] = {}
    lowlinks: dict[str, int] = {}
    stack: list[str] = []
    on_stack: set[str] = set()
    components: list[list[str]] = []

    def visit(node: str) -> None:
        nonlocal index
        indexes[node] = index
        lowlinks[node] = index
        index += 1
        stack.append(node)
        on_stack.add(node)
        for target in graph.get(node, ()):
            if target not in nodes:
                continue
            if target not in indexes:
                visit(target)
                lowlinks[node] = min(lowlinks[node], lowlinks[target])
            elif target in on_stack:
                lowlinks[node] = min(lowlinks[node], indexes[target])
        if lowlinks[node] == indexes[node]:
            component: list[str] = []
            while True:
                member = stack.pop()
                on_stack.remove(member)
                component.append(member)
                if member == node:
                    break
            if len(component) > 1 or node in graph.get(node, set()):
                components.append(sorted(component))

    for node in sorted(nodes):
        if node not in indexes:
            visit(node)
    return sorted(components)


def frame_candidates(
    function: str, records: dict[str, list[dict[str, object]]]
) -> list[dict[str, object]]:
    if function in records:
        return records[function]
    normalized = re.sub(r"\.\d+$", "", function)
    if normalized in records:
        return records[normalized]
    if function.startswith("____") and function.endswith("_veneer"):
        base = function[4:-7]
        if base in records:
            return records[base]
    if function in MANUAL_FRAME_BYTES:
        return [
            {
                "frame_bytes": MANUAL_FRAME_BYTES[function],
                "qualifiers": ["static", "linked-disassembly-certificate"],
                "record": f"manual:{function}",
            }
        ]
    return []


def certify_manual_frames(
    bodies: dict[str, list[str]], reachable_functions: set[str]
) -> dict[str, object]:
    certificates: dict[str, object] = {}
    for function in sorted(reachable_functions & set(MANUAL_FRAME_BYTES)):
        body_names = MANUAL_FRAME_BODY_ALIASES.get(function, (function,))
        body_lines = [
            line for name in body_names for line in bodies.get(name, [])
        ]
        if not body_lines:
            raise ValueError(f"manual frame function absent from disassembly: {function}")
        body = "\n".join(body_lines) + "\n"
        missing = [
            fragment
            for fragment in MANUAL_FRAME_FRAGMENTS[function]
            if fragment not in body
        ]
        if missing:
            raise ValueError(
                f"manual frame pattern changed for {function}: missing {missing}"
            )
        decrements = sp_decrementing_instructions(body_lines)
        expected_decrements = MANUAL_SP_DECREMENT_COUNTS[function]
        if len(decrements) != expected_decrements:
            raise ValueError(
                f"manual frame SP-decrement inventory changed for {function}: "
                f"expected {expected_decrements}, observed {decrements}"
            )
        certificates[function] = {
            "frame_bytes": MANUAL_FRAME_BYTES[function],
            "body_sha256": hashlib.sha256(body.encode("utf-8")).hexdigest(),
            "required_instruction_fragments": MANUAL_FRAME_FRAGMENTS[function],
            "sp_decrementing_instructions": decrements,
            "scope": "this linked ELF body only",
            "objdump_body_symbols": list(body_names),
        }
    return {
        "method": (
            "manual conservative inspection of all SP-decrementing instructions; "
            "instruction fragments and normalized objdump-body digests bind the result"
        ),
        "generic_save_state_blx_lr": (
            "the BL sets LR to the wrapper continuation; generic_save_state saves 32 "
            "bytes and BLX LR enters that continuation, so it is not an unknown callback"
        ),
        "functions": certificates,
    }


def parse_define(text: str, name: str) -> int:
    matches = re.findall(rf"^\s*#define\s+{re.escape(name)}\s+(\d+)\s*$", text, re.MULTILINE)
    if len(matches) != 1:
        raise ValueError(f"expected one integer definition for {name}, found {matches}")
    return int(matches[0])


def require_fragments(path: Path, fragments: list[str]) -> str:
    text = path.read_text(encoding="utf-8")
    missing = [fragment for fragment in fragments if fragment not in text]
    if missing:
        raise ValueError(f"source proof pattern missing from {path}: {missing}")
    return text


def dlog_depth(length: int) -> int:
    """Maximum simultaneously active source frames for the two sequential calls."""
    if length <= 1:
        return 1
    right = length // 2
    left = length - right
    return 1 + max(dlog_depth(right), dlog_depth(left))


def bz_path(n: int) -> list[tuple[str, int]]:
    """Deepest D2n1n/D3n2n mutual-recursion path for one positive n."""
    path = [("mp_div_d2n1n", n)]
    while n >= 2 and n % 2 == 0:
        n //= 2
        path.extend((("mp_div_d3n2n", n), ("mp_div_d2n1n", n)))
    return path


def max_frame_bytes(
    function: str, records: dict[str, list[dict[str, object]]]
) -> int:
    candidates = frame_candidates(function, records)
    if not candidates:
        raise ValueError(f"missing compiler frame record for recursive function {function}")
    return max(int(row["frame_bytes"]) for row in candidates)


def derive_recursion_certificates(
    source_dir: Path,
    build_dir: Path,
    stack_records: dict[str, list[dict[str, object]]],
    cycles: list[list[str]],
) -> dict[str, object]:
    """Check source rank functions for every recursive SCC in this linked image.

    This closes only the recursion-depth obligation.  It is intentionally not a
    whole-program stack proof: library/assembly frames and indirect calls are
    reported separately by the caller.
    """
    impl = source_dir / "src/pqm4/sqisign_p324_3/m4f"
    biextension = impl / "biextension.c"
    mp = impl / "mp.c"
    encoded = impl / "encoded_sizes.h"
    ec_params = impl / "ec_params.h"
    keygen = impl / "keygen.c"
    sign = impl / "sign.c"

    biextension_text = require_fragments(
        biextension,
        [
            "if (len == 0)",
            "else if (len == 1)",
            "long right = (double)len * 0.5;",
            "long left = len - right;",
            "fp2_dlog_2e_rec(&dlp1, right, pows_f, pows_g, stacklen + 1)",
            "fp2_dlog_2e_rec(&dlp2, left, pows_f, pows_g, stacklen)",
        ],
    )
    mp_text = require_fragments(
        mp,
        [
            "#define MP_BZ_SHOULD_RECURSE(n) (((n) % 2 == 0) && ((n) >= 2))",
            "int n2 = n / 2;",
            "mp_div_d3n2n(Q1, R1, A1, B, n2);",
            "mp_div_d3n2n(Q2, R, AA, B, n2);",
            "mp_div_d2n1n(Qhat, R1, AH, B1, n);",
        ],
    )
    encoded_text = encoded.read_text(encoding="utf-8")
    ec_text = ec_params.read_text(encoding="utf-8")
    require_fragments(
        keygen,
        ["CHALLENGE_BITS + EC_EXTRA_TORSION"],
    )
    require_fragments(
        sign,
        [
            "int reduced_order = RESPONSE_BITS + HD_EXTRA_TORSION;",
            "Eaux2_Echall2.E2, reduced_order);",
        ],
    )

    build_ninja = (build_dir / "build.ninja").read_text(encoding="utf-8")
    if "-DRADIX_32" not in build_ninja:
        raise ValueError("recursion certificate currently expects the linked RADIX_32 build")
    radix32_match = re.search(
        r"#elif\s+RADIX\s*==\s*32\s*\n#define\s+IBZ_NLIMBS\s+(\d+)",
        encoded_text,
    )
    if not radix32_match:
        raise ValueError("could not derive the RADIX_32 IBZ_NLIMBS bound")

    challenge_bits = parse_define(encoded_text, "CHALLENGE_BITS")
    response_bits = parse_define(encoded_text, "RESPONSE_BITS")
    ec_extra = parse_define(ec_text, "EC_EXTRA_TORSION")
    hd_extra = parse_define(ec_text, "HD_EXTRA_TORSION")
    dlog_entries = {
        "KeyGen": challenge_bits + ec_extra,
        "Sign": response_bits + hd_extra,
    }
    dlog_max = max(dlog_entries.values())
    dlog_frames = dlog_depth(dlog_max)

    mp_limbs = int(radix32_match.group(1))
    candidate_paths = [bz_path(n) for n in range(1, mp_limbs + 1)]
    deepest_bz_path = max(candidate_paths, key=len)
    d2_frame = max_frame_bytes("mp_div_d2n1n", stack_records)
    d3_frame = max_frame_bytes("mp_div_d3n2n", stack_records)
    bz_recursive_frame_sum = sum(
        d2_frame if function == "mp_div_d2n1n" else d3_frame
        for function, _ in deepest_bz_path
    )

    linked_dlog_symbols = sorted(
        {
            member
            for component in cycles
            for member in component
            if member.startswith("fp2_dlog_2e_rec")
        }
    )
    dlog_frame_max = max(
        max_frame_bytes(function, stack_records) for function in linked_dlog_symbols
    )
    recognized = []
    for component in cycles:
        if len(component) == 1 and component[0].startswith("fp2_dlog_2e_rec"):
            recognized.append(component)
        elif component == ["mp_div_d2n1n", "mp_div_d3n2n"]:
            recognized.append(component)
    if len(recognized) != len(cycles):
        unrecognized = [component for component in cycles if component not in recognized]
        raise ValueError(f"unrecognized recursive SCCs need a rank proof: {unrecognized}")

    return {
        "status": "ESTABLISHED_FOR_LINKED_CONFIGURATION",
        "scope": (
            "source-level recursion depth for the p324_3 RADIX_32 D2 linked K/S/V roots; "
            "not a whole-program stack bound"
        ),
        "all_linked_recursive_sccs_recognized": True,
        "all_recursive_sccs_have_rank_bound": True,
        "false_positive_guard": (
            "B/B.W to the current symbol plus an offset is treated as an intra-function "
            "branch; BL/BLX and cross-symbol tail calls remain graph edges"
        ),
        "dlog": {
            "symbols": linked_dlog_symbols,
            "entry_lengths": dlog_entries,
            "maximum_entry_length": dlog_max,
            "rank": "positive len; each child is floor(len/2) or ceil(len/2)",
            "maximum_active_source_frames": dlog_frames,
            "maximum_compiler_frame_bytes_among_linked_variants": dlog_frame_max,
            "conservative_recursive_frame_bytes": dlog_frames * dlog_frame_max,
            "source": {
                "path": str(biextension.relative_to(source_dir)),
                "sha256": sha256(biextension),
            },
        },
        "burnikel_ziegler_division": {
            "symbols": ["mp_div_d2n1n", "mp_div_d3n2n"],
            "radix": 32,
            "maximum_entry_limbs": mp_limbs,
            "rank": "D2 parameter n; every D2->D3->D2 cycle replaces n by n/2",
            "maximum_active_source_frames": len(deepest_bz_path),
            "witness_path": [
                {"function": function, "n": n} for function, n in deepest_bz_path
            ],
            "compiler_frame_bytes": {
                "mp_div_d2n1n": d2_frame,
                "mp_div_d3n2n": d3_frame,
            },
            "recursive_frame_bytes_on_witness": bz_recursive_frame_sum,
            "source": {
                "path": str(mp.relative_to(source_dir)),
                "sha256": sha256(mp),
            },
        },
    }


def shortest_paths(graph: dict[str, set[str]], root: str) -> dict[str, list[str]]:
    paths = {root: [root]}
    pending = deque([root])
    while pending:
        node = pending.popleft()
        for target in sorted(graph.get(node, ())):
            if target not in paths:
                paths[target] = paths[node] + [target]
                pending.append(target)
    return paths


def static_psp_bound(
    graph: dict[str, set[str]],
    nodes: set[str],
    recursive_components: list[list[str]],
    stack_records: dict[str, list[dict[str, object]]],
    recursion_certificates: dict[str, object],
    root: str,
) -> dict[str, object]:
    """Compute a conservative longest active-frame path for one PSP root.

    Recursive SCCs are collapsed and assigned the source-level rank bound
    validated by ``derive_recursion_certificates``.  Calls outside an SCC are
    sequential, so only the largest child bound can be live at once.
    """

    component_members: list[tuple[str, ...]] = [
        tuple(component) for component in recursive_components
    ]
    recursive_component_set = set(component_members)
    assigned = {member for component in component_members for member in component}
    component_members.extend((node,) for node in sorted(nodes - assigned))
    component_of = {
        member: index
        for index, component in enumerate(component_members)
        for member in component
    }
    component_edges: dict[int, set[int]] = defaultdict(set)
    for source in nodes:
        for target in graph.get(source, ()):
            if target not in nodes:
                continue
            source_component = component_of[source]
            target_component = component_of[target]
            if source_component != target_component:
                component_edges[source_component].add(target_component)

    dlog = recursion_certificates["dlog"]
    bz = recursion_certificates["burnikel_ziegler_division"]

    def local_weight(component: tuple[str, ...]) -> tuple[int, str]:
        if (
            component in recursive_component_set
            and len(component) == 1
            and component[0].startswith("fp2_dlog_2e_rec")
        ):
            return (
                int(dlog["conservative_recursive_frame_bytes"]),
                "dlog rank certificate",
            )
        if component in recursive_component_set and set(component) == {
            "mp_div_d2n1n",
            "mp_div_d3n2n",
        }:
            return (
                int(bz["recursive_frame_bytes_on_witness"]),
                "Burnikel--Ziegler rank certificate",
            )
        if len(component) != 1:
            raise ValueError(f"unrecognized recursive component: {component}")
        return max_frame_bytes(component[0], stack_records), "local frame"

    visiting: set[int] = set()
    memo: dict[int, tuple[int, list[dict[str, object]]]] = {}

    def visit(component_index: int) -> tuple[int, list[dict[str, object]]]:
        if component_index in memo:
            return memo[component_index]
        if component_index in visiting:
            raise ValueError("condensation graph unexpectedly contains a cycle")
        visiting.add(component_index)
        children = [visit(child) for child in component_edges.get(component_index, ())]
        child_bound, child_witness = max(children, key=lambda item: item[0], default=(0, []))
        members = component_members[component_index]
        weight, basis = local_weight(members)
        witness = [
            {
                "component": list(members),
                "local_or_recursive_frame_bound_bytes": weight,
                "basis": basis,
            }
        ] + child_witness
        result = (weight + child_bound, witness)
        visiting.remove(component_index)
        memo[component_index] = result
        return result

    total, witness = visit(component_of[root])
    return {
        "bound_bytes": total,
        "psp_reservation_bytes": 128 * 1024,
        "reservation_margin_bytes": 128 * 1024 - total,
        "fits_reservation": total <= 128 * 1024,
        "witness": witness,
        "scope": (
            "linked p324_3/RADIX32 Thread-mode call graph rooted at the operation "
            "thunk, with rank-bounded recursive SCCs; excludes asynchronous MSP use"
        ),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--source-dir", type=Path, required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--objdump", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--expected-firmware-commit")
    args = parser.parse_args()

    cache_path = args.build_dir / "CMakeCache.txt"
    cache = cmake_cache_values(cache_path)
    if cache.get("SQISIGN_FIRMWARE_DIRTY") != "0":
        raise ValueError("stack-bound ELF was not built from a clean firmware tree")
    if cache.get("SQISIGN_V3_SOURCE_DIRTY") != "0":
        raise ValueError("stack-bound ELF was not built from a clean v3 source tree")
    firmware_commit = cache.get("SQISIGN_FIRMWARE_GIT_COMMIT")
    if args.expected_firmware_commit is not None and firmware_commit != args.expected_firmware_commit:
        raise ValueError(
            f"expected firmware commit {args.expected_firmware_commit}, observed {firmware_commit}"
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
        bodies,
        veneer_resolutions,
        cross_symbol_conditionals,
    ) = parse_disassembly(disassembly)
    stack_records = parse_stack_usage(args.build_dir)

    root_reports: dict[str, object] = {}
    all_cycles: set[tuple[str, ...]] = set()
    for root in ROOTS:
        if root not in functions:
            raise ValueError(f"missing linked root: {root}")
        nodes = reachable(graph, root)
        cycles = strongly_connected_components(graph, nodes)
        all_cycles.update(tuple(component) for component in cycles)
        paths = shortest_paths(graph, root)
        missing = []
        dynamic = []
        for function in sorted(nodes):
            candidates = frame_candidates(function, stack_records)
            if not candidates:
                missing.append(
                    {"function": function, "path": paths.get(function, [root, function])}
                )
                continue
            if any("dynamic" in row["qualifiers"] for row in candidates):
                dynamic.append(function)
        root_indirect = [
            {"function": function, "instructions": lines}
            for function, lines in sorted(indirect_calls.items())
            if function in nodes
        ]
        root_reports[root] = {
            "reachable_functions": len(nodes),
            "recursive_sccs": cycles,
            "recursive_scc_count": len(cycles),
            "reachable_dynamic_frame_functions": dynamic,
            "missing_stack_metadata": missing,
            "missing_stack_metadata_count": len(missing),
            "indirect_calls": root_indirect,
            "indirect_callsite_count": sum(
                len(record["instructions"]) for record in root_indirect
            ),
        }

    all_rows = [row for rows in stack_records.values() for row in rows]
    dynamic_rows = [row for row in all_rows if "dynamic" in row["qualifiers"]]
    unique_cycles = [list(component) for component in sorted(all_cycles)]
    all_reachable = set().union(
        *(reachable(graph, root) for root in ROOTS)
    )
    reachable_indirect_pc_loads = [
        {"function": function, "instruction": line}
        for function in sorted(all_reachable)
        for line in bodies.get(function, [])
        if INDIRECT_PC_LOAD_RE.search(line)
    ]
    resolved_veneer_names = {row["veneer"] for row in veneer_resolutions}
    unmodeled_indirect_pc_loads = [
        row
        for row in reachable_indirect_pc_loads
        if row["function"] not in resolved_veneer_names
    ]
    if unmodeled_indirect_pc_loads:
        raise ValueError(
            f"unmodeled reachable indirect PC loads: {unmodeled_indirect_pc_loads}"
        )
    reachable_cross_symbol_conditionals = [
        row for row in cross_symbol_conditionals if row["function"] in all_reachable
    ]
    observed_conditional_pairs = {
        (row["function"], row["target_expression"])
        for row in reachable_cross_symbol_conditionals
    }
    if observed_conditional_pairs != CERTIFIED_CROSS_SYMBOL_CONDITIONALS:
        raise ValueError(
            "reachable cross-symbol conditional-branch inventory changed: "
            f"{sorted(observed_conditional_pairs)}"
        )
    manual_frame_certificates = certify_manual_frames(bodies, all_reachable)
    recursion_certificates = derive_recursion_certificates(
        args.source_dir, args.build_dir, stack_records, unique_cycles
    )
    linked_psp_closure_complete = all(
        report["missing_stack_metadata_count"] == 0
        and report["indirect_callsite_count"] == 0
        and not report["reachable_dynamic_frame_functions"]
        for report in root_reports.values()
    )
    psp_bounds = {}
    if linked_psp_closure_complete:
        for root in ROOTS:
            root_nodes = reachable(graph, root)
            psp_bounds[root] = static_psp_bound(
                graph,
                root_nodes,
                strongly_connected_components(graph, root_nodes),
                stack_records,
                recursion_certificates,
                root,
            )
    all_psp_bounds_fit = bool(psp_bounds) and all(
        report["fits_reservation"] for report in psp_bounds.values()
    )
    result = {
        "schema": "sqisign-v3-linked-stack-bound-audit-v3",
        "status": "PSP_BOUND_ESTABLISHED" if all_psp_bounds_fit else "PARTIAL",
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
        "compiler_stack_usage": {
            "files": len(list(args.build_dir.rglob("*.su"))),
            "records": len(all_rows),
            "dynamic_records": len(dynamic_rows),
        },
        "roots": root_reports,
        "unique_recursive_sccs": unique_cycles,
        "recursion_bound_certificates": recursion_certificates,
        "manual_frame_certificates": manual_frame_certificates,
        "literal_veneer_resolutions": veneer_resolutions,
        "indirect_pc_load_certificate": {
            "records": reachable_indirect_pc_loads,
            "all_are_resolved_literal_veneers": True,
        },
        "cross_symbol_conditional_branch_certificate": {
            "records": reachable_cross_symbol_conditionals,
            "interpretation": (
                "all are DCP floating-point wrapper continuations; their maximum "
                "save area is included in the corresponding manual frame bound"
            ),
        },
        "static_psp_bounds": psp_bounds,
        "decision": {
            "all_compiler_local_frames_static": len(dynamic_rows) == 0,
            "linked_psp_metadata_and_calls_closed": linked_psp_closure_complete,
            "all_recursive_sccs_have_rank_bound": recursion_certificates[
                "all_recursive_sccs_have_rank_bound"
            ],
            "operation_psp_bounds_established": all_psp_bounds_fit,
            "whole_program_worst_case_stack_bound_established": False,
            "reason": (
                "The linked Thread-mode operation roots have conservative PSP bounds, "
                "including source-ranked recursion and ELF-bound manual assembly frames. "
                "A whole-program bound is still withheld because asynchronous interrupt "
                "nesting and MSP call chains are outside this analysis."
                if all_psp_bounds_fit
                else "The linked operation-root closure is incomplete or one conservative "
                "PSP bound exceeds its reservation."
            ),
            "next_proof_obligations": [
                "add interrupt/MSP bounds separately from the operation PSP bound",
                "compare the resulting static bound with guarded multi-input PSP watermarks",
            ],
        },
    }
    if not result["decision"]["all_compiler_local_frames_static"]:
        raise ValueError("the D2 build still contains compiler-reported dynamic frames")
    if not all_cycles:
        raise ValueError("expected recursive proof obligations were not found")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(
        f"v3 linked stack-bound audit: {result['status']} "
        f"dynamic_records=0 recursive_sccs={len(all_cycles)} "
        "recursive_rank_bounds=true "
        f"operation_psp_bounds={str(all_psp_bounds_fit).lower()} "
        "whole_program_bound=false"
    )
    for root, report in root_reports.items():
        print(
            f"{root}: reachable={report['reachable_functions']} "
            f"cycles={report['recursive_scc_count']} "
            f"missing_su={report['missing_stack_metadata_count']} "
            f"indirect_calls={report['indirect_callsite_count']}"
        )
    for root, report in psp_bounds.items():
        print(
            f"{root}_psp_bound={report['bound_bytes']} "
            f"margin={report['reservation_margin_bytes']} "
            f"fits={report['fits_reservation']}"
        )
    print(f"output={args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
