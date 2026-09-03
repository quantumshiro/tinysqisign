# Side-channel screens

The first experiment is an intentionally narrow host timing screen for the
frozen D12c low-memory signer. The second localizes one variable-time primitive
on the physical RP2350 and supplies an external GPIO trigger. Neither is a
constant-time certificate, a key-recovery attack, or an analog power/EM result.

## Mandatory threat model and acceptance gates

Side-channel resistance is a release requirement, not a property inferred from
the fixed arena. The machine-readable policy is
`experiments/sca/resistance-plan.json`; `scripts/check_sca_resistance_plan.py`
fails closed if known leakage is relabeled as resistance. It protects the
long-term signing key, secret-derived Cornacchia inputs/exponents, intermediate
ideals/lattices/responses and RNG state against end-to-end timing, local
single-/multi-trace power or EM, and branch/address/allocation observers.

The current deployment status is deliberately
`combined_controls_integrated_residual_control_and_address_differences_reproduced_physical_pending`:
fixed public storage and heap-free closure gates pass, but the full-Sign host
timing gate fails, a two-run host edge/address screen is positive, and the
first divergences are in variable-control integer primitives. Fixed-work
integer integration, analog acquisition, attack-driven exponent recovery,
later-divergence localization and final-target full-Sign validation remain
open. The first countermeasure below is therefore an experimental component
gate, not a change to that deployment decision.

SCA-8 makes constant-work engineering a separate mandatory gate. Before a
production claim, the complete signing path must be audited for
secret-dependent control flow, iteration counts, memory-address sequences and
table/index choices, and each identified surface must be removed or protected.
Passing timing or analog leakage tests cannot substitute for that structural
audit; conversely, a constant-work trace does not remove value-dependent power
leakage and therefore cannot substitute for the analog and attack-driven
gates.

## Design

- Build: D12c `6b79cfb…`, Level I, reference implementation, RADIX64,
  Release/ThinLTO, GMP off, `ENABLE_ML2_PROFILE=ON`.
- Secret key and 32-byte message: fixed for every trace.
- Class A: one fixed deterministic signing-RNG stream.
- Class B control: the same fixed stream as A.
- Class B primary: 64 unique deterministic signing-RNG streams.
- Pair execution order alternates A/B and B/A.  Signature verification occurs
  only after both timed calls, so verification of one class does not set the
  cache state immediately before timing the other class.
- Each result is verified, fixed-stream signatures must remain byte-stable,
  and ML2 profile counts are reset/read for every invocation.
- The screen reports first-order Welch t, centered-square second-order Welch t,
  a paired t statistic, effect size, execution-position t, prefix stability and
  correlations with ML2 input/attempt counts.  The alert threshold is the
  conventional exploratory TVLA value `|t| > 4.5`.

The negative control is essential: a primary alert is interpreted only when
the corresponding fixed-vs-fixed control remains below the threshold.

## Frozen results

Two 64-pair runs use the same inputs and reverse the dataset order.

| Run | Dataset order | Control t1 / t2 | Primary t1 / t2 | A median | B median | B/A sample-SD ratio |
|---|---|---:|---:|---:|---:|---:|
| 1 | control, primary | `-0.002 / 0.768` | `3.590 / -5.393` | 802.092 ms | 642.841 ms | 161.59x |
| 2 | primary, control | `0.249 / -0.204` | `3.582 / -5.386` | 804.301 ms | 642.935 ms | 121.37x |

Both controls pass and both primary datasets exceed the second-order threshold.
For the same 64 class-B seeds, elapsed times across the two runs correlate at
Pearson `0.999958` and Spearman `0.999634`; path counters and signature digests
match exactly.  Median absolute cross-run deviation is 1.877 ms.  In contrast,
ML2 attempts take only values 17 or 18 and correlate weakly with elapsed time
(Pearson `-0.0226` and `-0.0232`).  Thus the coarse timing dependence is highly
repeatable but is not explained by this ML2 counter alone.

This is a positive host timing-leakage finding: the current signer exposes
substantial input-dependent execution-time variance.  It does not show that an
attacker can recover the long-term key, and it does not localize the responsible
operation.  Candidate causes include the already documented retry, Cornacchia,
division/GCD, lattice, theta and early-success paths.

Raw and derived artifacts are:

- `results/host/sca-d12c-sign-{control,fixed-random}.csv`
- `results/host/sca-d12c-sign-summary.json`
- `results/host/sca-d12c-sign-rerun-{control,fixed-random}.csv`
- `results/host/sca-d12c-sign-rerun-summary.json`
- `results/host/sca-d12c-sign-reproducibility.json`
- `results/host/artifacts/sca-d12c-sign-instrumented`
- `results/host/artifacts/sca-d12c-sign-rerun-instrumented`

## Reproduction

The run is nondeterministic in nanoseconds, so reproducing the experiment means
rebuilding the same instrumented binary and repeating the statistical test, not
regenerating byte-identical CSV files.  To avoid overwriting the frozen results:

```sh
SQISIGN_SCA_OUTPUT_DIR=/tmp/sca-run1 \
  SQISIGN_SCA_BINARY_DIR=/tmp/sca-run1 \
  SQISIGN_SCA_PAIRS=64 \
  ./scripts/run_sca_timing_experiment.sh

SQISIGN_SCA_OUTPUT_DIR=/tmp/sca-run2 \
  SQISIGN_SCA_BINARY_DIR=/tmp/sca-run2 \
  SQISIGN_SCA_OUTPUT_PREFIX=sca-d12c-sign-rerun \
  SQISIGN_SCA_DATASET_ORDER=primary-first \
  SQISIGN_SCA_PAIRS=64 \
  ./scripts/run_sca_timing_experiment.sh
```

`python3 scripts/check_sca_timing_artifacts.py` validates the archived hashes,
recomputes both summaries from raw rows and recomputes the cross-run comparison.

## D13 certified-sketch follow-up

The same 64-pair screen was repeated twice after the D13 certified-norm-sketch
change, again reversing dataset order.  The fixed-vs-fixed controls remain
below threshold, while both primary runs reproduce the second-order alert.

| Run | Dataset order | Control t1 / t2 | Primary t1 / t2 | A median | B median | B/A sample-SD ratio |
|---|---|---:|---:|---:|---:|---:|
| 1 | control, primary | `-0.559 / -0.816` | `3.570 / -5.410` | 802.337 ms | 643.751 ms | 125.18x |
| 2 | primary, control | `-0.152 / -0.432` | `3.576 / -5.374` | 809.075 ms | 651.178 ms | 110.18x |

For the same class-B seeds, elapsed times have Pearson `0.999929` and
Spearman `0.999771` correlation across runs; path counters and signature
digests are identical.  Median absolute cross-run deviation is 4.375 ms.
Thus D13 preserves the already-visible coarse timing dependence; it does not
provide a side-channel improvement.

A second D13-specific profile counts certified-sketch work while joining the
counts to the uninstrumented timing rows by deterministic seed.  Across 128
measured Sign invocations it observes 2,062,361 sort comparisons and 5,041
exact-replay ties, a `0.2444%` tie fraction.  Random-class search replay ranges
from 4 to 1,677 evaluations.  Pearson correlations with elapsed time are weak:
about `-0.130` for comparisons, `-0.217` for exact ties and `-0.106` for search
evaluations in both runs.  The sketch therefore adds an input-dependent trace,
but these counters do not explain the dominant fixed-vs-random timing signal;
the pre-existing variable-time signer remains the leading explanation.

Frozen D13 artifacts are:

- `results/host/sca-d13-sign-{control,fixed-random}.csv` and summary JSON;
- the corresponding `sca-d13-sign-rerun-*` rows and summary;
- `results/host/sca-d13-sign-reproducibility.json`;
- `results/host/sca-d13-sign-sketch-profile.{csv,json,log}` and its execution
  rows;
- the measured binaries under `results/host/artifacts/`.

`python3 scripts/check_sca_timing_artifacts.py` recomputes both D13 timing
reports, their cross-run comparison and the sketch-attribution report.  This is
host timing evidence only.  It is not an analog power/EM result, a Cortex-M33
full-Sign result, a constant-time certificate or a key-recovery attack.

## Full-Sign structural control/address screen

Timing alone does not show which execution structure changes, so the frozen
D13 signer is also rebuilt with Apple Clang SanitizerCoverage
`trace-pc-guard`, `trace-loads` and `trace-stores`.  The harness itself is
uninstrumented to avoid callback recursion, enables callbacks only around the
encoded Sign call, and verifies the signature outside the traced interval.
The Level-I key and message are fixed.  Class A uses signing-RNG seed 0; the
negative control repeats seed 0, while primary class B uses seeds 1, 2, 3 and
5.  Pair order alternates A/B and B/A.

Two independent processes execute the same frozen binary.  In each run:

- both fixed/fixed control pairs have identical signatures, edge-event counts,
  two edge-sequence hashes, load/store counts and two address-sequence hashes;
- all four fixed/random pairs have different signatures and differ in both
  full edge and full memory-address streams;
- the fixed seed-0 Sign executes 173,062,335 edge events, 572,012,107 loads and
  271,404,872 stores;
- the first saved edge divergence reproduces at event 938,604, 935,361,
  938,604 and 933,275 for seeds 1, 2, 3 and 5.  `atos` reports the nearest
  symbols as `ibz_div` or `ibz_cmp`; these are localization hints rather than
  exact source attribution.

The full streams are hashed online with two 64-bit diagnostic hashes.  The
saved four-million-edge and two-million-memory-event prefixes are truncated;
therefore the reports do not claim an exact first address-divergence location.
Hash/count equality is not a proof of constant time, while the paired
inequalities are positive structural evidence for this frozen four-seed host
corpus.  The instrumentation observes addresses but not values, register
Hamming weights or transitions, physical power/EM, or the final linked
Cortex-M33 behavior.

Frozen artifacts are:

- `results/host/sca-d13-full-sign-control-address.{csv,json}`;
- `results/host/sca-d13-full-sign-control-address-rerun.{csv,json}`;
- `results/host/artifacts/sca-d13-full-sign-control-address`;
- `experiments/sca/trace_sign_control_flow.c` and the build/analyze/run tools.

`python3 scripts/check_sca_control_flow_trace_artifacts.py` checks every hash,
recomputes both JSON reports byte-for-byte from the raw rows and rejects any
loss of the fixed/fixed controls or 4/4 primary edge/address differences.  A
new acquisition should use temporary output directories because a full run is
large and the default paths name the frozen evidence:

```sh
SQISIGN_SCA_OUTPUT_DIR=/tmp/sqisign-sca-structural \
SQISIGN_SCA_BINARY_DIR=/tmp/sqisign-sca-structural/bin \
SQISIGN_SCA_OUTPUT_PREFIX=full-sign-control-address-audit \
  ./scripts/run_sca_control_flow_trace.sh
```

This screen makes SCA-8 affirmatively positive on the host artifact.  It does
not establish physical observability or key recovery; remediation,
target-level repetition and power/EM campaigns remain mandatory.

### Exact first-edge localization

The bounded prefix was then replayed with SanitizerCoverage PC tables and LTO
debug line information. The key and message remain fixed, the fixed/fixed
control remains exact, and the four class-B seeds remain 1, 2, 3 and 5. Two
clean builds produce the same normalized source decisions:

- seeds 1 and 3 first diverge at edge event 938,604 in the high-zero
  `ibz_used_limbs` scan (`intbig.c:163-164`) while
  `ibz_divrem_unsigned_wide` computes the dividend size at line 250;
- seed 2 first diverges at event 935,361 and seed 5 at event 933,275 in the
  most-significant-limb early-return conditions of `ibz_cmp`
  (`intbig.c:992-996`).

The frozen inputs differ only in signing RNG, so this demonstrates
ephemeral-input-dependent compiled control. It neither proves that these first
branches reveal the long-term signing key nor enumerates later divergences.
The two binaries have different Mach-O UUID/build-path hashes, while the
source localization is identical; both binaries, DWARF files, raw rows and
reports are retained and hash-pinned.

Reproduce or audit with:

```sh
SQISIGN_SCA_OUTPUT_DIR=/tmp/sqisign-sca-localization \
SQISIGN_SCA_BINARY_DIR=/tmp/sqisign-sca-localization/bin \
SQISIGN_SCA_OUTPUT_PREFIX=full-sign-edge-localization-audit \
  ./scripts/run_sca_edge_localization.sh
python3 scripts/check_sca_edge_localization_artifacts.py
```

The checker also reruns the broader full-Sign structural checker. SCA-8 now
requires replacement or protection of these integer paths, continued
localization after each remediation, final Cortex-M33 trace review and the
physical campaigns; localization alone is not resistance.

### First-divergence caller stacks

The source lines identify the local decisions but not the correct ownership
boundary for a countermeasure. A second harness therefore takes one native
backtrace at each already-frozen first-difference event. It does not sample
arbitrary events or broaden the corpus. Two clean builds reproduce all eight
class-A/class-B stacks:

- the used-limb decision is owned by `ibz_divrem_unsigned_wide -> ibz_div ->
  ibz_div_floor -> ibz_mod -> quat_alg_norm_mod`;
- the early comparison is owned by `ibz_cmp -> ibz_rand_interval`;
- both paths meet at `quat_sampling_random_ideal_O0_given_norm_impl`, then
  return through `protocols_sign_with_workspace` and
  `sqisign_sign_with_workspace`.

This rejects a one-line “branchless compare” fix. The sampler also exposes a
variable-width modular reduction, rejection-dependent iteration/RNG count and
first-success publication. A protected variant must specify all of those as
one fixed-width/fixed-round boundary, including distribution and exhaustion
semantics. Reproduce or audit with:

```sh
SQISIGN_SCA_OUTPUT_DIR=/tmp/sqisign-sca-first-stack \
SQISIGN_SCA_BINARY_DIR=/tmp/sqisign-sca-first-stack/bin \
SQISIGN_SCA_OUTPUT_PREFIX=full-sign-first-stack-audit \
  ./scripts/run_sca_first_divergence_stack.sh
python3 scripts/check_sca_first_divergence_stack_artifacts.py
```

This remains a four-comparison host diagnostic. It is not an all-input trace
proof, a long-term-key leak, a Cortex-M33 result, power/EM evidence or a
completed countermeasure.

The follow-up design contract is
`random-ideal-constant-work-design.json`. A read-only LLDB profile of the
frozen twelve-seed Sign corpus observes gamma 1--4 attempts (mean `1.92`) and
beta one attempt, but labels those values descriptive only. The budget comes
from exact finite-field point counts plus an exhaustive reduced-domain check:
130 gamma candidates and one beta candidate. A fixed 769-bit wide reduction
per coordinate replaces inner rejection sampling, and a fixed primitive-lift
correction removes the `N^2` rejection. Outer exhaustion plus
coordinate-distribution bias is strictly below `2^-128`; this is not yet a
bound on the complete output-ideal distribution after the correction map.

Rebuild the LLDB corpus, or audit the frozen artifacts and design, with:

```sh
./scripts/run_sca_random_ideal_attempt_profile.sh
python3 scripts/check_sca_random_ideal_attempt_artifacts.py
./scripts/run_sca_random_ideal_fixed_budget_components.sh
python3 scripts/check_sca_random_ideal_fixed_budget_components.py
python3 scripts/check_sca_random_ideal_constant_work_design.py
```

The resulting schedule consumes 38,218 fixed random bytes per prime ideal and
still requires 130 fixed-work modular-square-root evaluations. It is an
intentionally conservative first Pareto point, not a performance result. The
769-bit reducer, fixed Level-I square root and primitive-lift components pass
host/sanitizer and two frozen Cortex-M33 individual-TU profiles. Their
workspaces are 2,808/4,320/6,480 bytes; entry frames are 32/48/88 bytes under
`-Os` and 32/56/88 bytes in the Pico-like profile, with aggregate text
1,104/1,192 bytes. The square-root identity is specific to the exact Level-I
prime `N=2^512+75`; it is not a general sampler integration. The complete
protected sampler, output-ideal distribution/security proof, integration,
target trace and physical campaign remain pending. Operand masking and
constant-power behavior are not claimed.

## RP2350 triggered powmod experiment

Full D12c Sign takes about 7,337 seconds on the Pico 2, so a statistically
useful first analog campaign cannot use thousands of complete signatures. The
physical diagnostic therefore targets `ibz_pow_mod`, which is used underneath
the modular-square-root/Cornacchia surface and executes ordinary
square-and-multiply with an exponent-bit branch.

The frozen image uses the production D12c Level-I/RADIX32 `intbig.c` at `-O3`,
not the host instrumented signer. It holds a fixed base and the 521-bit prime
`2^521-1`, varies only the 521-bit exponent, disables interrupts, raises GPIO 2,
calls `ibz_pow_mod` once, lowers GPIO 2, and restores interrupts. USB parsing and
reporting occur outside the trigger. The final ELF gate proves that ordering in
the linked disassembly. Commands are:

- `A` and `B`: identical fixed exponent, the negative control;
- `F`: the same fixed exponent used in the primary comparison;
- `R`: the next deterministic random exponent of the same 521-bit length;
- `L` and `H`: two- and 521-set-bit diagnostic extremes;
- `X`: reset the random schedule.

Two 32-pair acquisitions were captured on a 150-MHz RP2350. Both interleave
AB/BA control and FR/RF primary pairs; run 2 reverses both the within-pair phase
and whether control or primary executes first. The fixed-vector Python oracle,
result range and call status pass for every applicable row.

| Run | Control t1 / t2 | Fixed/random t1 / t2 | Weight/time Pearson | Slope µs/bit | R² |
|---|---:|---:|---:|---:|---:|
| 1 | `-0.518 / -0.144` | `0.661 / -3.906` | **0.999860** | **4,004.02** | **0.999720** |
| 2, reversed | `-0.603 / -1.405` | `0.661 / -3.906` | **0.999860** | **4,004.06** | **0.999720** |

For the same 32 random exponents, run-to-run elapsed time has Pearson
**0.999999999** correlation; the median absolute difference is 2 µs and the
maximum is 5 µs. Exponent and result digests are exact. The independent
low/high-weight diagnostics take 2.091150/4.172861 seconds in run 1 and
2.091152/4.172860 seconds in run 2, or about 4,011 µs per added set bit.

The ordinary two-class threshold does not fire because the fixed exponent has
weight 261, near the random distribution's center. The targeted regression is
nevertheless decisive: almost all observed random-input timing variance is
explained by exponent Hamming weight, and the independently chosen low/high
extremes give the same per-bit cost. This is direct RP2350 timing leakage from
the variable-time primitive. It still does not show key recovery or prove that
the complete Sign leakage is dominated by this one function.

Build, flash, capture and audit commands are:

```sh
./scripts/build_rp2350_sca_powmod.sh
build-rp2350-keygen/picotool-usb/picotool load -f -v -x \
  build-rp2350-sca/src/platform/rp2350/sqisign_rp2350_sca_powmod.uf2

./scripts/capture_rp2350_sca_powmod.py \
  --port /dev/cu.usbmodem11201 --pairs 32 \
  --output-dir results/rp2350 --prefix sca-d12c-powmod \
  --firmware results/rp2350/artifacts/sca-powmod-499787c/sqisign_rp2350_sca_powmod.uf2

./scripts/capture_rp2350_sca_powmod.py \
  --port /dev/cu.usbmodem11201 --pairs 32 \
  --output-dir results/rp2350 --prefix sca-d12c-powmod-rerun \
  --firmware results/rp2350/artifacts/sca-powmod-499787c/sqisign_rp2350_sca_powmod.uf2 \
  --dataset-order primary-control --order-offset 1

python3 scripts/analyze_rp2350_sca_powmod.py \
  --control results/rp2350/sca-d12c-powmod-control.csv \
  --fixed-random results/rp2350/sca-d12c-powmod-fixed-random.csv \
  --capture results/rp2350/sca-d12c-powmod-capture.json \
  --output results/rp2350/sca-d12c-powmod-summary.json

python3 scripts/compare_rp2350_sca_powmod_runs.py \
  --run1-primary results/rp2350/sca-d12c-powmod-fixed-random.csv \
  --run1-summary results/rp2350/sca-d12c-powmod-summary.json \
  --run2-primary results/rp2350/sca-d12c-powmod-rerun-fixed-random.csv \
  --run2-summary results/rp2350/sca-d12c-powmod-rerun-summary.json \
  --output results/rp2350/sca-d12c-powmod-reproducibility.json

python3 scripts/check_rp2350_sca_powmod_artifacts.py
```

The capture script records the board's coarse microsecond timer while emitting
the same GPIO pulses an oscilloscope or ChipWhisperer can trigger on. It does
not acquire analog samples.

## RP2350 exponent-branch countermeasure experiment

The first mitigation candidate attacks exactly the operation-count channel
above. `powmod_always_521.c` executes 521 public iterations from the most
significant bit, computes both the square and square-times-base result on every
iteration, and selects with a 27-limb mask. Thus it always performs 1,042
modular multiplications; the legacy diagnostic performs 520 squarings plus one
multiplication per set exponent bit (755--802 calls in the frozen random
corpus). It accepts only nonnegative 521-bit exponent/modulus inputs, preserves
output/input aliasing, and does not publish output on invalid input.

This is branch regularization, not a constant-time implementation. Its
`ibz_mul`, `ibz_mod`, division and used-limb descendants remain variable-time;
the selected intermediate value also still depends on the exponent bit and may
leak in analog traces. The candidate is not wired into Sign.

Host differential and sanitizer tests compare 68 boundary/random cases with
the unchanged D13 `ibz_pow_mod`; every result matches and every candidate call
records exactly 521 iterations/1,042 modular multiplications. The linked Arm
ELF has static frames of 1,128 bytes for the candidate, 232 bytes for its
modular-multiply helper and 32 bytes for the GPIO window. Its checker proves a
fixed `520..0` outer counter, exactly two helper call sites, one public
27-limb selection-loop branch, mask operations, no legacy-powmod edge, and the
same IRQ/GPIO trigger ordering.

Two 32-pair 150-MHz RP2350 captures use the exact legacy exponent sequence and
reverse dataset/pair order on run 2:

| Run | Control t1 / t2 | Fixed/random t1 / t2 | Weight/time Pearson | Slope µs/bit | R² |
|---|---:|---:|---:|---:|---:|
| 1 | `1.547 / -0.103` | `-1.233 / -4.434` | `-0.13110` | `-18.207` | `0.01719` |
| 2, reversed | `0.870 / 0.107` | `-1.232 / -4.435` | `-0.13136` | `-18.229` | `0.01726` |

Relative to legacy, the absolute weight slope falls by at least **219.66x** and
the low/high diagnostic slope by at least **333.82x**; the old 2.0817-second
low/high gap becomes about -6.235 ms. Exact result and exponent digests match.
The cost is real: median random-exponent time rises from about 3.1251 to
4.1837 seconds, a **1.33872x** local ratio, because the average legacy call
used 779.5 modular multiplications rather than 1,042.

Residual leakage is also real. Random-input sample SD is about 1.432 ms while
fixed-input SD is 1.5--2.1 µs, a ratio of at least **680x**. The same 32
random-input times correlate at **0.9999987** across the reversed runs (median
absolute difference 2 µs, maximum 5 µs). The two-class second-order statistic
lands just below the exploratory threshold, but the reproducible per-input
timing pattern forbids a constant-time or SPA-resistance conclusion. The next
software target is fixed-work modular multiplication/reduction/division; the
next physical target is analog acquisition and attack-driven exponent
recovery.

Reproduce and audit with:

```sh
./scripts/run_sca_powmod_countermeasure_test.sh
./scripts/build_rp2350_sca_powmod_always.sh
./scripts/flash_rp2350_sca_powmod_always.sh /dev/cu.usbmodem11201

# Capture commands are the same as the legacy experiment, using prefixes
# sca-d13-powmod-always and sca-d13-powmod-always-rerun; run 2 adds
# --dataset-order primary-control --order-offset 1.
python3 scripts/check_rp2350_sca_powmod_countermeasure_artifacts.py
python3 scripts/check_sca_resistance_plan.py
```

The raw CSV/capture/summary/comparison records are
`results/rp2350/sca-d13-powmod-always-*`; the exact ELF/UF2/BIN/map/disassembly
is under `results/rp2350/artifacts/sca-powmod-always-7c4484d8/`.

## RP2350 fixed-work multiplication countermeasure experiment

The second candidate replaces the remaining variable-work big-integer
multiplication and reduction in the diagnostic with a narrow 521-bit word
implementation. The exponent loop always runs 521 rounds and invokes exactly
1,042 modular multiplications. Every multiplication runs all 521 multiplier
bits; every bit round executes two complete 17-word modular additions and
selects with masks. One powmod therefore executes 542,882 multiplier-bit
rounds and 1,085,764 modular additions. The accepted domain is deliberately
narrow: positive modulus at most 521 bits and operands already reduced below
the modulus.

This gives a fixed source-level instruction/address schedule for the audited
arithmetic, not a general constant-time proof. Operand values still change
register transitions, carries, switching activity and memory data. Compiler,
microarchitecture and analog leakage are not covered by source inspection, and
the candidate is not connected to the production Sign path.

The host oracle and fatal ASan/UBSan tests pass 128 multiplication cases and
eight powmod cases. Arm GNU 15.2.1 reports static frames of 88 bytes for the
modular adder, 56 for the fixed-work multiply core, 896 for its public wrapper,
1,000 for powmod and 32 for the GPIO window. The linked checker verifies the
521×521 schedules, branchless 17-word modular-add core, two fixed-length
compiler-generated `memset` calls, absence of legacy multiply/mod/div edges,
and the IRQ/GPIO trigger ordering.

Two 32-pair RP2350 captures use the exact exponent/result schedule. Run 2
reverses both dataset order and within-pair order:

| Run | Control t1 / t2 | Fixed/random t1 / t2 | Weight/time Pearson | Slope µs/bit | Fixed−random paired mean / median |
|---|---:|---:|---:|---:|---:|
| 1 | `-2.228 / -3.714` | `-5.654 / +2.444` | `+0.42355` | `+0.095689` | `-5.1875 / -5.0 µs` |
| 2, reversed | `+1.785 / +5.033` | `-5.511 / +2.310` | `+0.23621` | `+0.059788` | `-4.96875 / -5.0 µs` |

The 64 pooled pairs have mean/median fixed-minus-random offsets of
`-5.078125/-5.0 µs` and paired `t=-7.7963` (52 negative, ten positive, two
zero). Thus the class effect reproduces after order reversal. The second
control has a second-order alert, and execution-position statistics exceed the
exploratory threshold in both controls; this is an additional warning, not a
reason to subtract the effect.

Against the legacy diagnostic, the absolute exponent-weight slope falls by at
least **41,843.94x**. Median random-input time is **1.02426x** legacy and
**0.76510x** the square-and-multiply-always candidate. The linked diagnostic
text is 41,928 bytes, +400 bytes over legacy and +512 over the first
countermeasure. These are attractive component-level Pareto numbers, but the
reproducible class offset makes the fail-closed coarse timing decision
**positive**. It is not evidence of analog-SPA resistance, exponent-recovery
resistance, a protected signer or production readiness.

Reproduce and audit with:

```sh
./scripts/run_sca_powmod_fixedwork_test.sh
./scripts/build_rp2350_sca_powmod_fixedwork.sh
./scripts/flash_rp2350_sca_powmod_fixedwork.sh /dev/cu.usbmodem11201
python3 scripts/check_rp2350_sca_powmod_fixedwork_artifacts.py
python3 scripts/check_sca_resistance_plan.py
```

The raw runs and derived reports are
`results/rp2350/sca-d13-powmod-fixedwork-*`; the exact object, stack-use,
ELF/UF2/BIN/map and disassembly are under
`results/rp2350/artifacts/sca-powmod-fixedwork-67896440/`.

## Executed Sign-to-Cornacchia attack mapping

The fixed-work component is now connected to the exact software locus targeted
by Mukherjee et al. rather than being justified only by a synthetic exponent
benchmark.  Patch
`patches/0033-experiment-profile-Sign-Cornacchia-powmod-leakage.patch`
adds an opt-in, host-only record immediately after the modular square root in
`ibz_cornacchia_prime`.  The control is the clean D13 source; both builds use
the same Level-I fixture key/message and twelve deterministic signing streams.
Every signature verifies and the control/instrumented output is byte-identical.

The frozen corpus contains 361 executed Cornacchia square-root calls:

| Property | Frozen result |
|---|---:|
| Calls per Sign | 13--72, median 27 |
| Modulus width | 360--378 bits, median 368 |
| Tonelli--Shanks odd exponent `q` | 354--376 bits |
| Initial-root exponent `(q+1)/2` | 353--375 bits |
| Two-adicity | 2--12 |
| Modulus `1 mod 8` / `5 mod 8` | 171 / 190 |
| `5 mod 8` published linear-relation candidates | 190 |

For `m mod 8 = 5`, `m-1 = 4q` and the current initial-root exponent is
`(q+1)/2 = (m+3)/8`.  The exponent length, Hamming weight and number of
Cornacchia calls all vary in the executed signer.  This is an attack-surface
mapping, not an analog trace or proof that every variation independently
reveals the long-term key.

The measured 378-bit maximum is not used as a safety bound.  A separate
compiled-parameter/source derivation covers both production
`quat_represent_integer` call sites at Level I:

- fixed-degree IdealToIsogeny has `L <= 248-2 = 246`, so
  `4*u*(2^L-u) < 2^(2L) <= 2^492`;
- Sign's random auxiliary ideal has a 252-bit positive cofactor and norm below
  `2^126`, so its adjusted target is below `2^380`;
- Cornacchia subtracts a positive term, while each Tonelli--Shanks exponent is
  below the modulus and each powmod base is reduced first.

Consequently the 521-bit fixed-work domain covers these Level-I Cornacchia
powmods with 29 bits of margin.  A runtime guard remains mandatory and must
fail closed without a legacy variable-time fallback.  The result does not
cover Levels III/V, Miller--Rabin powmods, other modular-square-root callers,
the variable Tonelli--Shanks loop, or the rest of Sign.

Rebuild and verify the corpus and bound with:

```sh
./scripts/run_cornacchia_sca_profile.sh
./scripts/run_cornacchia_fixedwork_bound.sh
python3 scripts/check_cornacchia_sca_profile.py
python3 scripts/check_sca_resistance_plan.py
```

The raw profile, exact rows and bound proof are
`results/host/sca-d13-cornacchia-*`.

## Integrated Cornacchia fixed-work checkpoint

Commit `c2a80712e32891c5228da3f49f0148993a4ec560` implements the next
checkpoint as an opt-in Level-I profile. It routes all modular exponentiations
and Tonelli--Shanks multiplications in the production Cornacchia square root
through a caller-owned 521-bit fixed-work workspace. The 492-bit
source-derived bound is enforced at runtime; a violation is fatal and cannot
select the legacy implementation.

The frozen clean integration run gives:

| Property | Result |
|---|---:|
| Sign invocations / protected sqrt calls | 12 / 361 |
| Signature + next 64 RNG bytes | byte-identical to control |
| Successful / no-root / fatal sqrt results | 361 / 0 / 0 |
| Fixed pow / exponent rounds | 2,180 / 1,135,780 |
| Modular multiplication / multiplier rounds | 2,273,360 / 1,184,420,560 |
| Full modular additions | 2,368,841,120 |
| Control / protected aggregate host time | 7.94 / 75.01 s |
| Descriptive ratio | `9.447103x` |

The Arm object checker confirms the fixed public loop bounds, branch-free
exponent/multiplier selection, static audited frames and absence of accepted-
path legacy sqrt/pow/mul/div calls. This removes the direct exponent-bit
control branch targeted in ePrint 2025/830. It does **not** regularize
Tonelli--Shanks loop counts, non-residue search, `p-1` factoring, Euclid/half-
GCD, retries or the rest of Sign, and a fixed instruction sequence may still
have value-dependent power/EM. The roughly 9.45x result is one aggregate host
smoke and shows that this deliberately simple fixed-work construction is not
yet an efficient production design.

Reproduce or check the frozen result with:

```sh
./scripts/run_cornacchia_fixedwork_integration.sh
./scripts/check_cornacchia_fixedwork_integration.py
./scripts/run_cornacchia_fixedwork_arm.sh
./scripts/check_sca_resistance_plan.py
```

The patch is
`patches/0034-experiment-protect-Cornacchia-pow-schedule.patch`; raw outputs,
logs, work rows, summary and binaries are named
`results/host/sca-d13-cornacchia-fixedwork-*`. The analog follow-up is
preregistered in `cornacchia-analog-plan.json`. It is still unexecuted because
no shunt/current or near-field EM acquisition instrument is connected.

### RP2350 first calibration image: historical positive control-flow result

The protected square root is now linked into a dedicated RP2350 Arm-S image
with 16 public 368-bit residue/prime fixtures. GPIO 2 is high only around one
protected square-root call, with interrupts masked; USB output, result
verification and the 4,344-byte workspace-clear check occur outside that
window. The final-ELF gate confirms one protected call, five linked fixed-pow
sites, five O3-folded fixed Tonelli--Shanks multiply sites, one exact
4,344-byte clear and no legacy fallback.

The image was flashed and a five-command serial smoke passed. All five roots,
PSP calls and clear checks succeeded. Target elapsed times nevertheless span
16.004361--70.418552 seconds; the identical-input A/B control took
32.024229/32.024063 seconds. This is direct evidence that residual
Tonelli--Shanks/non-residue control flow remains variable even after removing
the published exponent-bit branch. It is not an analog trace or a key-recovery
experiment.

An exact reconstruction of the public algorithmic work gives 5,210--22,924
fixed-base modular multiplications for those records and correlates with the
five target times at Pearson `0.999999999981`. The dominant causes are the
variable number of Legendre tests needed to find the first quadratic
non-residue and the variable Tonelli--Shanks multiplication count. This makes
the next countermeasure requirement concrete: equalize whole-operation call
counts, not only each inner exponentiation.

```sh
./scripts/build_rp2350_sca_cornacchia_fixedwork.sh
./scripts/flash_rp2350_sca_cornacchia_fixedwork.sh /dev/cu.usbmodem11201
./scripts/capture_rp2350_sca_cornacchia_fixedwork.py \
  --port /dev/cu.usbmodem11201
```

The build record is
`results/rp2350/sca-d13-cornacchia-fixedwork-build.json`; the serial-only
smoke is `results/rp2350/sca-d13-cornacchia-fixedwork-calibration-smoke.json`.
The causal work profile is
`results/rp2350/sca-d13-cornacchia-fixedwork-work-profile.json`.
`analog_capture_run=false` in all three records.

### RP2350 bounded-outer and fixed-input-reduction follow-up

The next WIP adds fixed-loop Montgomery arithmetic, a bounded 32-round
Tonelli--Shanks schedule, a fixed scan of 32 public non-residue candidates and
a 521-round signed reduction at the protected input boundary. The exact
flashed calibration image is archived under
`results/rp2350/artifacts/sca-cornacchia-fixed-reduce-0898c657/`; its UF2
SHA-256 is
`0898c6579cecf33f32715b45dbb5accda73d4e518b33b2b23f74a1001db7110a`.
The linked image has 52,772 bytes of text, zero data bytes and 24,504 bytes of
BSS. The experimental square-root workspace is 4,488 bytes.

Before replacing the initial `ibz_mod`, two independent L/H batches showed a
mean H-minus-L difference of `+14.7667 us`, Welch `t=6.637997`, with 29 of 30
paired differences positive. With the fixed signed reduction, the same 30-pair
screen gives `-2.5333 us`, `t=-0.332420`, with 11 positive, 17 negative and two
zero pairs; the LH and HL batch means have opposite signs. All 16 public
fixtures span 49 us around a roughly 768.108 ms mean. Thus the previous L/H
effect is no longer statistically resolved in this frozen public corpus.

These are target-reported elapsed times, not power/EM samples or an equivalence
test. In particular, square-and-multiply-always still selects with an
exponent-bit mask and processes exponent-dependent intermediate values. A
regular instruction count can therefore retain exploitable switching leakage.
The finite 32-prime non-residue bound also remains a fail-closed experimental
domain restriction rather than an unconditional proof for all possible
Level-I inputs.

Raw timing rows, the before/after report and firmware provenance are:

- `results/rp2350/sca-ct3-fixed-{outer,reduce}-*.json`;
- `results/rp2350/sca-ct3-fixed-reduce-timing-summary.json`;
- `results/rp2350/sca-ct3-fixed-reduce-build.json`.

### RP2350 one-exponentiation acquisition image

The current primary analog target removes the surrounding square-root work
from the GPIO-high region.  It parses one of the same 16 public fixtures,
derives `(p-1)/2`, and prepares the Montgomery context before the trigger.
With interrupts masked, GPIO 2 then brackets exactly one call to
`sqisign_sca_mont_pow_521_with_context`; validation, the complete 1,248-byte
secret-material clear, and USB reporting occur after GPIO goes low.  The final
ELF gate verifies this ordering, one protected-pow edge, the absence of legacy
pow/sqrt/div fallbacks, and static 40/104-byte window/pow frames.

The exact flashed UF2 is
`1f8b6ddc81acd98ebc6667d89135c1080de48a29953e4dd3a9e00d2d0c95681e`.
All 16 fixtures passed once, then a ten-cycle forward/reverse schedule passed
160/160 result and clear checks.  One exponentiation takes 62,042--62,053 us,
compared with roughly 768 ms for the integrated square-root image.  The
cycle-centered timing/exponent-weight correlation is `0.0866`; the
minimum-versus-maximum-weight Welch statistic is `-0.579`.  These are coarse
integer-microsecond observations over public inputs, not an equivalence test,
power trace, or resistance result.

Public attack-scoring truth is independently derived from the archived
fixture header.  Each 367-bit `(p-1)/2` value is represented in the full
521-round schedule, and its Hamming weight and 17-word FNV digest must match
both serial corpora.  The attack scorer rejects any CSV whose claimed truth
differs from this frozen manifest.

```sh
./scripts/build_rp2350_sca_cornacchia_pow_segment.sh
./scripts/flash_rp2350_sca_cornacchia_pow_segment.sh \
  /dev/cu.usbmodem11201
./scripts/capture_rp2350_sca_cornacchia_pow_segment.py \
  --port /dev/cu.usbmodem11201 --cycles 10 --alternating-order \
  --output results/rp2350/sca-ct3-cornacchia-pow-segment-timing-10cycles.json \
  --firmware results/rp2350/artifacts/sca-cornacchia-pow-segment-1f8b6ddc/sqisign_rp2350_sca_cornacchia_pow_segment.uf2
python3 scripts/check_sca_cornacchia_pow_segment_ground_truth.py
python3 scripts/check_sca_resistance_plan.py
```

The firmware quartet, archived source and stack records are under
`results/rp2350/artifacts/sca-cornacchia-pow-segment-1f8b6ddc/`.  Build,
serial and timing provenance are
`results/rp2350/sca-ct3-cornacchia-pow-segment-{build,smoke,timing-10cycles,timing-summary}.json`;
the public truth is
`experiments/sca/cornacchia-pow-segment-ground-truth.json`.

### Residual surface gate

The fixed 521-round schedule closes only the published operation-count
branch.  It does not mask the exponent.  In the frozen source,
`select_words` forms a 32-bit all-zero or all-one word from every exponent
bit; the selected prefix then becomes the next round's Montgomery operand.
The frozen GCC 15.2.1 ELF likewise keeps the bit as a live register value and
lowers selection to arithmetic on `bit` and `bit-1`.  Fixed branches and
addresses therefore coexist with first-order selector and operand/state
leakage hypotheses.

`residual-surface-inventory.json` records twelve open surfaces from that
selector through Tonelli--Shanks, fixed-half-GCD operand/mask switching,
RepresentInteger searches, retries and complete Sign.  Its idealized
source-level mask model recovers 16/16 public fixture exponents if the mask
Hamming weight is observed perfectly.  This is a structural rejection of the
claim “fixed schedule implies power resistance”; it is not a simulated or
physical attack result.  The gate is:

```sh
python3 scripts/check_sca_residual_surfaces.py
python3 scripts/check_sca_residual_surfaces.py \
  --objdump /path/to/arm-none-eabi-objdump
```

Both commands must continue to report all twelve surfaces as open until their
specified attack-driven evidence exists.  The aggregate
`check_sca_resistance_plan.py` runs the source-level form automatically.

The paper-specific closure gate makes the distinction concrete.  It rebuilds
the normal D13 and experimental protected Cornacchia chains as like-for-like
Cortex-M33 objects.  The normal object must retain the legacy secret-bit
exponentiation branch; the protected accepted path must use the fixed-work
pow and fixed-round half-GCD without a legacy fallback.  Frozen runtime
evidence additionally fixes 12 Sign executions, 361 modular square roots and
2,180 exponentiations (75--409 per Sign).  The gate deliberately reports
`published_attack_blocked=false`.  It now also verifies the paper's three
exponent-to-input relations on 24/24 frozen public fixtures, verifies that the
isolated current-C Legendre exponent reveals 16/16 fixture moduli as
`m=2e+1`, and replays the authors' 54-bit Sage toy backtracking through its
valid-signature marker.  This does not acquire a physical trace, recover an
exponent from RP2350 leakage or recover a key from the current C signer; the
protected operands remain unmasked.

```sh
python3 scripts/check_sca_published_spa_attack_closure.py
ARM_TOOLCHAIN_ROOT=/path/to/arm-gnu-toolchain-extracted \
  ./scripts/run_sca_published_spa_attack_closure_arm.sh
```

The isolated attack scorer is separately self-tested with exact, one-bit
corrupt and no-candidate synthetic predictions.  Synthetic reports are marked
ineligible for campaign evidence.  The physical campaign is split into
`ANALOG-1A` (current-C Legendre `m=2e+1`) and `ANALOG-1B` (the paper's
`4e+1`, `8e-3`, `8e+5` candidates), each at 1-, 10- and 100-trace budgets in
two independent acquisitions.

### Published countermeasure comparison

`published-countermeasure-comparison.json` turns the literature review into a
dependency and coverage gate.  Five primary papers and two pinned upstream
source snapshots are separated into eight candidate families.  Every frozen
surface must be mapped, but every `closes_surface` value remains false until
implementation and the surface-specific evidence exist.

The fixed-width integer substrate has now been built, and its square-root and
fixed 1,421-round C-halfgcd subset is integrated on the opt-in experimental
Level-I Cornacchia route.  The selected next package is therefore the
C-halfgcd physical-leakage gate, not another outer-loop port. The exact source
recurrence now has an independent 644-active-round sufficient bound inside the
1,421-round schedule; compiled-program refinement remains unproved.
The reviewed quaternion repository still excludes underlying GMP and LLL from
its component-level constant-time result.  The dimension-4 lattice source is
pinned separately but assumes 64-bit little-endian arithmetic, uses GMP
support and is GPL-2.0, so it is not a drop-in 32-bit target component.

Run the self-contained gate with:

```sh
python3 scripts/check_sca_countermeasure_comparison.py
```

If the reviewed source checkouts are available, bind the literature claims to
their exact commits and source markers as well:

```sh
python3 scripts/check_sca_countermeasure_comparison.py \
  --ct-quaternion-root /path/to/Constant-time-Quaternion-SQIsign \
  --ctlll-root /path/to/CTlll-SQIsign
```

This comparison is a selection artifact, not countermeasure evidence.  It
does not change the current `known_leakage` deployment decision.

### Rejected standalone exponent-blinding design

`cornacchia-exponent-blinding-design.json` evaluates the tempting candidate
`E=e+r*(p-1)` before any firmware implementation.  It preserves modular
exponentiation, but for the attacked affine relation `e=(p+k1)/k2` it also
leaves the exact identity
`k2*E-(k1+1)=(1+k2*r)*(p-1)`.  Recovering `E` therefore leaves a structured
factorization problem; the entropy of `r` cannot be reported as the security
level.  A 128-bit blind has only about a 40.7-bit leading-ECM-term proxy in the
frozen comparison, while the first table row above a 128-bit proxy expands the
fixed schedule to 1,408 rounds and needs about 20.35 KiB of fresh randomness
per average Sign.  Neither number is a concrete attack estimate.

The executed square-root path also gives an attacker repeated equations under
one modulus: a Legendre power, `z^q`, `n^((q+1)/2)`, and `n^q`.  Conditional
on exact SPA recovery of those four independently blinded exponents, the GCD
of their transformed values directly recovered `p-1` in 4,017/4,096
deterministic trials at 128 blind bits and 4,032/4,096 trials at 896 bits.
Thus merely increasing the blind width does not fix the multi-observation
construction.  The experiment deliberately does not assign a probability to
recovering the exponents from physical traces.

Run the fail-closed design gate with:

```sh
python3 scripts/check_cornacchia_exponent_blinding.py
```

It exhausts 349,568 reduced modular cases, checks all 16 public acquisition
fixtures and 8,192 multi-observation trials, and requires adoption, physical resistance and published-attack
closure to remain false.  This is a useful rejected-design result, not a
blinding implementation or cryptographic reduction.

### Rejected naive selector masking

The transition-aware criteria are anchored in ePrint
[2014/413](https://eprint.iacr.org/2014/413),
[2022/1546](https://eprint.iacr.org/2022/1546), and
[2024/755](https://eprint.iacr.org/2024/755).  They cover transition-based
leakage, scalar-microarchitecture share placement and hardened masked software,
but none supplies a drop-in masked SQIsign big-integer exponentiation.

`cornacchia-selector-masking-design.json` evaluates a second tempting
shortcut: Boolean-share the all-zero/all-one selector as a uniform `share0`
and `share1=share0 XOR mask`.  Either share and its Hamming weight are
secret-independent in isolation.  But if a Cortex-M33 register or data bus
handles the shares consecutively, their Hamming distance is exactly 0 for a
zero exponent bit and 32 for a one bit.  The transition model therefore
recovers the bit perfectly.

Run the exact reduced-domain gate with:

```sh
python3 scripts/check_cornacchia_selector_masking.py
```

It exhausts 16,380 assignments across widths 1--12 and checks the 32-bit
identity symbolically.  The result rejects this sequential schedule, not all
masking.  Any replacement must mask the arithmetic state as well as the
selector, control compiler share placement, and pass final-assembly and
transition-aware physical testing.

### Required scope for masked dynamic-modulus Montgomery arithmetic

The exponent selector is only one node in the attacked computation.  The
temporary Cornacchia modulus is secret-derived, and the frozen implementation
uses it unshared to derive the Montgomery inverse, `R mod M` and `R^2 mod M`.
Its exponentiation then processes unshared exponent/base words, selector and
prefix state together with product, reduction factor, carry, borrow and output
state.  Protecting only one of these categories leaves another direct
secret-dependent value path.

The source-bound gate is:

```sh
python3 scripts/check_cornacchia_masked_montgomery_scope.py
```

It verifies the exact archived C/header hashes, proves all 18 inventoried
sensitive nodes are reachable from the three secret roots in the frozen
dependency graph, and checks the uncovered set for six rejected partial
countermeasures.  The required future scope extends through context setup,
composable 521-bit add/subtract/select/multiply/reduction gadgets, base and
exponent handling, Tonelli--Shanks composition, final assembly, randomness
accounting and physical attacks.  This is a fail-closed design prerequisite,
not a masked implementation, physical observation or proof that hiding cannot
be used instead.

### Reduced arithmetic-masking prototype direction

The complete scope does not imply that a bit-sliced implementation is
practical.  The next design gate is:

```sh
python3 scripts/check_cornacchia_masked_arithmetic_design.py
```

It exhausts a two-share secure-AND and a masked full adder.  Every individual
wire has a secret-independent marginal in the abstract value model, while the
Hamming distance between adjacent complementary shares reconstructs the
unmasked input/output, sum or carry.  Thus a pure-C implementation or a
per-share marginal test is not an acceptance criterion.

The same contract records a deliberately naive full-Boolean cost proxy:
1,479,680 secure ANDs and 184,960 fresh random bytes per 544-bit product, or
183.800 MiB for the 1,042 products in one fixed power call.  It is not a lower
bound and excludes Montgomery reduction and transition hardening.  The only
candidate advanced to implementation study is a two- or three-limb
word-arithmetic sharing with composable Boolean carry/borrow/comparison/select
boundaries.  Its first functional checkpoint is now reproducible with:

```sh
python3 scripts/check_cornacchia_masked_word_prototype.py
```

The two-limb `Z/(2^64)` C oracle implements share, recombination, share-wise
addition, refresh and multiplication with one fresh 64-bit ring mask.  It
passes Release, fatal ASan/UBSan and Cortex-M33 compilation plus 1,082,400
reduced assignments.  Five direct adjacent-share channels fail the HD model;
only an ideal zero-precharge abstraction passes.  The C object is therefore
not transition-approved and may not be scaled or integrated until composable
conversion, final-assembly transition, randomness, memory, cycle and physical
gates pass.

The first final-assembly subprimitive is checked separately with:

```sh
python3 scripts/check_cornacchia_masked_refresh_m33.py
```

It freezes a 66-byte/23-instruction Cortex-M33 refresh of two 32-bit
arithmetic shares.  The checker pins Arm GNU 15.2.1 output, executes the exact
assembly for 36 edge cases and 10,000 deterministic cases on QEMU
`mps2-an505`, and exhausts 299,592 reduced assignments.  Direct adjacent
shares are secret-dependent in the HD model; the explicit zero-precharge
register/ALU/load/store schedule passes that reduced model.  A separate
exact-order expansion checks 37,448 assignments over all 55 adjacent modeled
ALU A/B/result, selected-register-write and load/store-data transitions and
rejects direct ALU/load/store share-to-share mutations.  QEMU is only an
instruction-set functional test, and the model omits real RP2350 coupling,
pipeline, interrupts and analog noise.  A separate byte-audited RP2350
acquisition image has been built, flashed and serial-smoked for 12 records.
Every A/B/R class executes the same three RNG calls, and `r12` is precharged
before GPIO-high.  Its GPIO window contains one exact refresh; RNG, share
preparation, checks, clear and output are outside.  Those conditions remove
identified acquisition-history confounds but are not an analog result.  The
primitive therefore remains research-only until `ANALOG-5` completes two
physical first-order campaigns, second-order characterization and direct HD
attacks.  It does not implement masked multiplication, conversions,
Montgomery arithmetic or SQIsign.

`masked-refresh-analog-dataset-template.json` and
`scripts/score_masked_refresh_m33_hd.py` freeze the physical analysis input:
balanced AB/BA pairs, exact serial ground truth, all share/recombination
formulas, point-wise first-order and centered-square TVLA, a bounded
cross-time centered-product second-order scan between two disjoint windows
selected from public timing before class analysis, twelve secret/diagnostic
HW, HD, carry and borrow models, and two permuted null controls.  Run
the tooling regression with:

```sh
python3 scripts/check_masked_refresh_analog_tooling.py
```

The regression injects synthetic direct-share leakage and a second-order-only
pair whose two pointwise marginals and centered squares remain quiet.  It must
recover both the frozen sample and the cross-time pair.  The resulting
synthetic report is forced to
`campaign_evidence_eligible=false`; only real power/EM manifests with at least
10,000 traces per class can enter the two-campaign physical decision.  The
separate decision template and comparator require six distinct reports: two
campaigns each for fixed/random, fixed-A/fixed-B and the fixed/fixed class
control.  One-run peaks cannot reject or approve the primitive.  Any
attack-model or pointwise/cross-time TVLA alert in the fixed/fixed dataset is a
control failure and makes the result inconclusive; it is never counted as a
primitive-rejection signal.

### Fixed-width CT integer foundation

`ctint1728-foundation-contract.json` freezes the implemented core slices of
`WP-CTINT-FOUNDATION`.  It covers 54-by-32-bit compare/cmov, sign/absolute
value, bit length, lossless/logical shifts, signed add/subtract, and fixed
schoolbook multiply/square plus 1,728-round signed truncating division.  It
also includes floor-style signed modular reduction with fixed sign adjustment
and fixed 521-round square-and-multiply-always modular exponentiation.  The
pow contract accepts nonnegative exponents up to 521 bits and positive moduli
up to 863 bits; its 3,240-byte caller workspace is cleared on every return.
The square-root slice provides fixed 864-round radix-4 exact and floor variants
with an 864-byte workspace.  The final component slice is a fixed 1,421-round
shift/subtract Cornacchia half-GCD for at most 492-bit moduli with a 3,456-byte
workspace; negative, out-of-domain, nonsquare, or unfinished cases execute the
defined schedule and fail without publishing output.
`cornacchia-halfgcd-bound-contract.json` separately checks the exact
`10/17` squared-norm contraction, derives the 644-round sufficient bound with
integer arithmetic, exhausts all 523,776 reduced inputs below 10 bits, and
checks 10,000 deterministic 492-bit model inputs against ordinary Euclid.
Independent branchy-reference tests cover division/reduction/pow boundaries,
aliases and nonpublication, a differential bridge checks current Level-I
`ibz_div`, `ibz_mod`, and `ibz_pow_mod`, and fatal ASan/UBSan plus exact Arm
GNU 15.2.1 gates cover two Cortex-M33 profiles.

Run the full gate with:

```sh
./scripts/run_ctint1728_foundation.sh
python3 scripts/check_cornacchia_halfgcd_bound.py
```

The 21-operation component set is complete.  The contract remains a
standalone component record, while byte-identical copies of the square-root
and half-GCD subset are integrated only on the opt-in experimental Cornacchia
route.  Both records explicitly deny whole-Sign constant-time, formal
noninterference, masking, power/EM resistance, published-attack closure, and
production use.

### Fixed-round half-GCD target checkpoint

`cornacchia-halfgcd-integration.json` freezes the Sign-route differential and
the exact fixed-width sources. The bound checker establishes that 644 active
rounds suffice for that frozen recurrence; the compiled C/assembly is not
formally verified against the model. Twelve deterministic seeds execute 361
Cornacchia calls and 512,981 half-GCD rounds with byte-identical signed-message
and post-RNG output.  The RP2350 component image places GPIO 2 around one call;
its linked core has exactly one public loop back edge and no legacy division,
register-indexed memory, indirect branch, or table branch.

The 30-pair A/B, L/H, and F/R serial corpus is deliberately coarse.  In the
fixed firmware it gives H−L `+0.200 us` with approximate 95% CI
`[-0.530,+0.930]`.  A separately frozen, otherwise matched variable-Euclid
positive-control image gives H−L `+1476.867 us` with CI
`[+1474.901,+1478.833]` for the same public 95-versus-122-step classes; its
same-input control still crosses zero.  Thus the timer design detects the
known iteration channel while the fixed schedule removes it at this
granularity.  No equivalence margin exists and no analog samples were
captured.  The next gate in `cornacchia-halfgcd-analog-plan.json` is to acquire
the frozen legacy image as the **analog** positive control, then run two
independent protected sessions and the preregistered per-round mask/state
classifier.

The reproducible component flow is:

```sh
./scripts/run_cornacchia_halfgcd_integration.sh
./scripts/build_rp2350_sca_cornacchia_halfgcd.sh
./scripts/flash_rp2350_sca_cornacchia_halfgcd.sh
./scripts/capture_rp2350_sca_cornacchia_halfgcd.py \
  --port /dev/cu.usbmodem11201 --pairs 30 --order-offset 1 \
  --prefix sca-cornacchia-halfgcd-30pairs
python3 scripts/analyze_rp2350_sca_cornacchia_halfgcd.py
python3 scripts/check_rp2350_sca_cornacchia_halfgcd_artifacts.py

./scripts/build_rp2350_sca_cornacchia_euclid_legacy.sh
./scripts/flash_rp2350_sca_cornacchia_euclid_legacy.sh \
  /dev/cu.usbmodem11201
python3 scripts/capture_rp2350_sca_cornacchia_euclid_legacy.py \
  --port /dev/cu.usbmodem11201 --pairs 30 --order-offset 1 \
  --prefix sca-cornacchia-euclid-legacy-30pairs
python3 scripts/analyze_rp2350_sca_cornacchia_euclid_legacy.py
python3 scripts/compare_rp2350_sca_cornacchia_halfgcd_positive_control.py
python3 scripts/check_rp2350_sca_cornacchia_euclid_legacy_artifacts.py
```

The capture command emits GPIO pulses but records only USB timer metadata.
An oscilloscope/SCA instrument must acquire the physical channel separately.

## Combined-protection residual structural trace

The fixed-budget commitment sampler, fixed 521-round Cornacchia square root
and fixed 1,421-round half-GCD were next executed together in the low-memory
Level-I/RADIX32 Sign closure. The residual screen uses exact SanitizerCoverage
PC tables and DWARF source locations. In each of two independent processes,
one seed-0/seed-0 control remains identical for the first 4,000,000 edge
events. All four seed-0/distinct-seed pairs differ. The first differences are
at events 1,767,695--1,769,252 and map to the bit-length maximum scan in
`ideal.c:168-169` or sign/absolute-value handling in `intbig.c:385/428`.

A second experiment instruments only `ideal.c`, `ibz_bitsize` and `ibz_mul`
families for load/store addresses. Its fixed/fixed control again remains
identical, while all 4/4 primary pairs differ in both selected control flow and
effective-address sequence in both processes. The first address differences
occur at events 3,026,298--3,033,316. They localize to the sign test in
`ibz_bitsize` (`intbig.c:401`) or the operand used-limb scans in `ibz_mul`
(`intbig.c:839-840`). Every selected-address capture reaches its public
4,000,000-event bound, so the result is a reproducible bounded prefix, not a
complete trace.

This result means that removing the published Cornacchia exponent schedule is
necessary but not sufficient: the generic fixed-storage integer backend still
uses secret-reachable sign, bit length and used-limb values to select branches,
loop counts and addresses. It does **not** demonstrate long-term-key leakage,
physical leakage or current-protected-C key recovery. Reproduce the frozen
records and their fail-closed interpretation with:

```sh
python3 scripts/check_sca_combined_residual_trace.py
```

The complete artifact/hash contract is
`experiments/sca/combined-residual-trace-contract.json`. Full remeasurement is
available through `scripts/run_sca_combined_control_flow_trace.sh` and
`scripts/run_sca_combined_focused_address.sh`; both are substantially slower
than the metadata checker. The next software step is a fixed-schedule generic
integer layer for the localized operations, followed by the same two traces.
RP2350 power/EM TVLA and attack-driven exponent/key-recovery experiments remain
separate mandatory gates.

## Required analog follow-up

Connect GPIO 2 and ground to an external acquisition instrument and measure a
shunt/current or near-field EM probe. Start with the frozen 62-ms one-pow
image: at pure execution time, 20,000 TVLA operations require about 20.7
minutes before acquisition/transport overhead. High-rate full-window traces
can still be large, so sample rate, crop/segmentation and alignment must be
preregistered and raw unfiltered data retained. Acquire a fixture-0 split
fixed-vs-fixed negative control and fixture-0 versus balanced fixtures 1--15
primary data with AB/BA ordering. Run point-wise first-/second-order Welch
screens twice independently, and separately attempt the preregistered
one-/ten-/hundred-trace exponent recovery. Repeat interpretable findings in
the integrated 768-ms square-root image before any whole-Sign conclusion. A
later full-Sign experiment must use coarse public phase triggers or segmented
capture and keep all serial output outside measured intervals. GPIO
instrumentation must be removed from production.

The half-GCD campaign uses the separate frozen 275-ms component image and
`analog-trace-dataset-halfgcd-template.json`.  Its fixed schedule makes
alignment easier but does not mask update decisions or arithmetic states.
Protected-firmware non-detection is inconclusive unless a matched legacy
variable-Euclid image demonstrates that the same setup can resolve a known
positive control.  That matched image and a strong coarse-timer separation
are now frozen; it must still be reacquired through the shunt/current or EM
channel because timer sensitivity does not validate analog sensitivity.

The exact ANALOG-0/1/2 datasets, trace budgets, controls, failure conditions,
metadata and decision rules are frozen in `cornacchia-analog-plan.json`.
`analog-trace-dataset-template.json` binds raw traces, aligned derivatives,
labels, instrument settings and exact firmware. Run
`scripts/analyze_sca_analog_tvla.py` to recompute point-wise first- and
centered-square second-order Welch screens. The attack-specific
`exponent-recovery-campaign-template.json` and
`scripts/score_sca_exponent_recovery.py` require every one-, ten- and
hundred-trace classifier result, including failures, to be reported against all
16 public fixtures.  Ground-truth exponents cannot be supplied ad hoc: they
are bound to `cornacchia-pow-segment-ground-truth.json` and recomputed by
`scripts/check_sca_cornacchia_pow_segment_ground_truth.py`.

Any future exponent-randomized candidate must also execute `ANALOG-3` on the
integrated square-root image.  It groups the Legendre, `c`, `x`, and `t`
power candidates by modulus, applies their public affine transformations and
tests the repeated-`q` and four-call GCDs.  A reproducible recovery of `p-1`
rejects the construction even if the isolated one-pow TVLA is negative.  This
gate is conditional and unexecuted because no randomized candidate or analog
traces currently exist.

Any future masked candidate must execute `ANALOG-4`: freeze its final
Cortex-M33 share placement, inspect recombination/register reuse/spills, test
static-HW and consecutive-HD hypotheses, run cross-share higher-order tests,
and attempt horizontal bit/state recovery.  It too remains conditional and
unexecuted.

The latest fail-closed inventory, frozen for the equal-history masked-refresh
target on 2026-08-23, found the RP2350 serial target but no analog scope/SCA device,
current or EM probe, or acquisition software. Consequently no
physical campaign, TVLA result or exponent-recovery attempt is claimed yet.
Notably, a TVLA-style non-detection alone cannot pass the attack-driven gate,
and even passing both campaigns would not close the remaining whole-Sign
variable-control-flow work.

## v3 fixed-key host timing screen

The v3 multi-vector target run changed seed, key and message together.  A
follow-up native-host screen therefore fixes the 33-byte message, resets the
same 48-byte signing-RNG seed before each signature, and copies each selected
key into the same fixed-address buffer.  It selects among ten official
`p324_3` KAT secret keys.  Official and D1 builds each run 30
repetitions per key in two independently shuffled schedules.  All 1,200
signatures verify and the two implementations produce identical signed
messages under matched inputs.

The two key-median rankings have Spearman correlation 1.0 for the official
build and 0.987879 for D1; the fastest-to-slowest span is 52.63--53.52% of the
central median.  The frozen
analysis consequently records
`repeatable_fixed_key_associated_host_timing_observed=true` and
`side_channel_resistance_established=false`.  The screen uses native
Darwin/arm64 reference code, so it does not establish the corresponding
RP2350/m4f trace, attribution to the non-public part of the encoded secret key, physical
leakage, exploitability, or key recovery.  Reproduce it with:

```sh
python3 scripts/run_v3_fixed_key_timing.py --repetitions 30
python3 scripts/analyze_v3_fixed_key_timing.py
```

The full design, archive hashes and claim boundary are in
`results/host/v3-fixed-key-timing-2026-09-04/manifest.json` and
`summary.json`.

## v3 fixed-key host structural trace

The controlled v3 design was also repeated with Apple Clang
SanitizerCoverage.  Edge-guard and load/store-address callbacks are enabled
only around `crypto_sign`.  Before tracing, each KAT secret key is copied into
the same fixed-address buffer; this prevents a comparison from detecting only
the different addresses of rows in the generated KAT table.  The message and
signing-RNG stream remain fixed.

Official and D1 executables each run in two fresh processes.  Every process
contains two same-key negative-control pairs and nine key-0-versus-key-j
comparisons.  All eight controls have equal event counts and dual stream
digests.  All 36 different-key comparisons have different control-flow and
effective-address event counts or digests in both process runs; their event
counts themselves differ, so the positive finding does not rely only on hash
collision resistance.  The first two million edge and memory events are kept
for bounded prefix inspection, while counts and dual 64-bit digests cover the
complete streams (up to about 70 million edges and 358 million memory events).

This is a key-associated native-host structural result, not a secret-only
attribution: the encoded secret key includes its corresponding public key.  It
does not observe loaded values, register transitions, Cortex-M33 code, power,
EM, exploitability, or key recovery, and trace equality would not prove
constant time.  Reproduce the two clean-source builds and four process runs
with:

```sh
python3 scripts/run_v3_fixed_key_structural_trace.py
```

The raw 88 rows, exact build commands, source/archive digests, and fail-closed
analysis are in
`results/host/v3-fixed-key-structural-trace-2026-09-04/`.
