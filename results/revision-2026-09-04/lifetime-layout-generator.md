# Annotation-driven lifetime layout prototype

Run:

```sh
python3 scripts/generate_lifetime_layout.py \
  experiments/memory/finduv-lifetime-layout.json \
  --output-dir results/revision-2026-09-04/generated-finduv-layout
```

The annotation lists typed objects, target ABI sizes/alignments, active phases,
pointer-escape declarations, and terminal clear obligations.  Phase
intersection produces the interference graph.  Deterministic aligned first-fit
placement emits `layout.json`, Graphviz `interference.dot`, and a C
`union`/`struct` plus `_Static_assert` declarations in `generated_layout.h`.

For the frozen v2 `find_uv` workspace, the generator independently reproduces
the checked manual layout: the 77,168-byte long-lived lattice state starts at
offset 0, while the 94,912-byte lattice-multiplication workspace, 14,024-byte
candidate workspace, and 94,912-byte fixed-degree workspace share offset
77,168.  The resulting extent is 172,080 bytes.

This is a prototype over reviewed annotations.  It rejects missing terminal
clear annotations and annotated pointer escape, but it does not infer C
lifetimes or prove absence of aliases.  `scripts/check_lifetime_certificate.py`
provides the separate frozen-source and dynamic-evidence layer.
