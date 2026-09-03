#!/usr/bin/env python3
"""Generate a compile-time SQIsign v3 KAT subset from an official RSP file."""

from __future__ import annotations

import argparse
import hashlib
import re
from pathlib import Path


FIELDS = ("count", "seed", "mlen", "msg", "pk", "sk", "smlen", "sm")
HEX_FIELDS = ("seed", "msg", "pk", "sk", "sm")


def parse_rsp(path: Path) -> list[dict[str, str]]:
    records: list[dict[str, str]] = []
    current: dict[str, str] = {}
    for raw in path.read_text(encoding="ascii").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            if current:
                if set(current) != set(FIELDS):
                    missing = sorted(set(FIELDS) - set(current))
                    raise SystemExit(f"incomplete RSP record; missing={missing}")
                records.append(current)
                current = {}
            continue
        match = re.fullmatch(r"([a-z]+)\s*=\s*([0-9A-F]+)", line)
        if match is None or match.group(1) not in FIELDS:
            raise SystemExit(f"unexpected RSP line: {raw!r}")
        key, value = match.groups()
        if key in current:
            raise SystemExit(f"duplicate field {key}")
        current[key] = value
    if current:
        if set(current) != set(FIELDS):
            raise SystemExit("incomplete final RSP record")
        records.append(current)
    if not records:
        raise SystemExit("no RSP records")
    return records


def byte_values(hex_text: str) -> list[int]:
    if len(hex_text) % 2 or re.fullmatch(r"[0-9A-F]*", hex_text) is None:
        raise SystemExit("invalid uppercase hexadecimal field")
    return list(bytes.fromhex(hex_text))


def emit_array(name: str, values: list[int]) -> list[str]:
    rows = [f"static const unsigned char {name}[] = {{"]
    for start in range(0, len(values), 12):
        chunk = ", ".join(f"0x{value:02x}" for value in values[start : start + 12])
        rows.append(f"    {chunk},")
    rows.append("};")
    return rows


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--rsp", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--count", type=int, default=10)
    args = parser.parse_args()
    if args.count < 2:
        raise SystemExit("count must be at least two")
    records = parse_rsp(args.rsp)
    selected = records[: args.count]
    if len(selected) != args.count:
        raise SystemExit(f"requested {args.count} records, found {len(selected)}")

    decoded: list[dict[str, object]] = []
    for index, record in enumerate(selected):
        if int(record["count"]) != index:
            raise SystemExit(f"non-contiguous count at record {index}")
        values = {field: byte_values(record[field]) for field in HEX_FIELDS}
        mlen = int(record["mlen"])
        smlen = int(record["smlen"])
        if len(values["seed"]) != 48 or len(values["msg"]) != mlen:
            raise SystemExit(f"bad seed/message length at record {index}")
        if len(values["pk"]) != 83 or len(values["sk"]) != 270:
            raise SystemExit(f"bad key length at record {index}")
        if len(values["sm"]) != smlen or smlen != mlen + 200:
            raise SystemExit(f"bad signed-message length at record {index}")
        decoded.append({"count": index, "mlen": mlen, "smlen": smlen, **values})

    lines = [
        "// Generated from the official SQIsign v3 response file; do not edit.",
        "#ifndef SQISIGN_V3_KAT_SUBSET_H",
        "#define SQISIGN_V3_KAT_SUBSET_H",
        "",
        "#include <stddef.h>",
        "",
        "typedef struct {",
        "    unsigned count;",
        "    size_t message_length;",
        "    size_t signed_message_length;",
        "    const unsigned char *seed;",
        "    const unsigned char *message;",
        "    const unsigned char *public_key;",
        "    const unsigned char *secret_key;",
        "    const unsigned char *signed_message;",
        "} sqisign_v3_kat_vector_t;",
        "",
    ]
    for record in decoded:
        index = int(record["count"])
        for field in HEX_FIELDS:
            lines.extend(emit_array(f"sqisign_v3_kat_{index}_{field}", record[field]))  # type: ignore[arg-type]
        lines.append("")

    max_message = max(int(record["mlen"]) for record in decoded)
    lines.extend(
        [
            "static const sqisign_v3_kat_vector_t sqisign_v3_kat_vectors[] = {",
        ]
    )
    for record in decoded:
        index = int(record["count"])
        lines.extend(
            [
                "    {",
                f"        {index}, {record['mlen']}, {record['smlen']},",
                f"        sqisign_v3_kat_{index}_seed, sqisign_v3_kat_{index}_msg,",
                f"        sqisign_v3_kat_{index}_pk, sqisign_v3_kat_{index}_sk,",
                f"        sqisign_v3_kat_{index}_sm,",
                "    },",
            ]
        )
    rsp_digest = hashlib.sha256(args.rsp.read_bytes()).hexdigest()
    lines.extend(
        [
            "};",
            "",
            "enum {",
            f"    SQISIGN_V3_KAT_VECTOR_COUNT = {len(decoded)},",
            f"    SQISIGN_V3_KAT_MAX_MESSAGE_BYTES = {max_message},",
            "    SQISIGN_V3_KAT_SEED_BYTES = 48,",
            "};",
            f'#define SQISIGN_V3_KAT_RSP_SHA256 "{rsp_digest}"',
            "",
            "_Static_assert(sizeof(sqisign_v3_kat_vectors) /",
            "                   sizeof(sqisign_v3_kat_vectors[0]) ==",
            "                   SQISIGN_V3_KAT_VECTOR_COUNT,",
            '               "KAT vector table extent mismatch");',
            "",
            "#endif",
            "",
        ]
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines), encoding="ascii")
    print(
        f"generated {len(decoded)} KAT vectors; max_message={max_message}; "
        f"rsp_sha256={rsp_digest}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
