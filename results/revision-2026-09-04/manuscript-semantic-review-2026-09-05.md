# Japanese–English manuscript review (2026-09-05)

This is an AI-assisted editorial review, not an independent human review or a
proof of translation equivalence. The Japanese manuscript remains the source
for the full English translation; neither version is an abridgment.

## Scope

The read-through covered the abstract, body, propositions and proofs,
algorithms, figure and table text, discussion, limitations, and conclusion.
Checks focused on assumptions, negations, quantifiers, experimental variants,
sample counts, and distinctions between measurements and bounds. Structural
parity is checked separately by `scripts/check_manuscript_translation.py`.

## Corrections

- Identify the one-function adaptation explicitly in the v3 multi-input method
  and its timing interpretation, in both languages. These results must not be
  attributed to the separately defined two-function adaptation.
- Distinguish pairs of quaternion orders from the order of enumeration in the
  English algorithm, proposition, and correspondence table.
- State the candidate exclusion as all coordinates divisible by 2, or all
  coordinates divisible by 3, rather than an ambiguous mixed condition.
- Correct the English lifetime diagram: the lattice state remains live during
  `find_uv`; the label does not describe `find_uv` as the live object.
- Remove a duplicated English unit after the arena-size equation.
- Clarify that no stack-information records or indirect-call targets remain
  unresolved; no unknown stack size is assigned a value of zero.
- Describe the timing-test threshold comparison using the magnitude of the
  negative second-order statistic, in both languages.
- Correct exception direction in both languages: while Secure software is
  executing, entry into a Non-secure exception can require the larger frame.
  The 52-word allowance and additional alignment word are unchanged. This
  correction follows Table 3 of the cited
  [Arm article](https://developer.arm.com/community/arm-community-blogs/b/architectures-and-processors-blog/posts/how-much-stack-memory-do-i-need-for-my-arm-cortex--m-applications).

## Preserved boundaries

The v2 official-request differential test is not relabeled as agreement with
historical GMP responses. The one-function, two-function, and fixed-array v3
binaries remain distinct. PSP measurements are not worst-case bounds, and
fixed-image operation bounds do not cover interrupt-handler/MSP behavior.
Key-associated timing and structural differences are not claimed to isolate
secret-only leakage or establish physical resistance or key recovery.

Both PDFs are rebuilt by `make -C manuscript eprint-check`. The accompanying
translation-alignment JSON records the reviewed source hashes and structural
mapping. `SHA256SUMS` binds the PDFs and public supporting artifacts. No new
cryptographic or hardware experiment was performed for this editorial review.
