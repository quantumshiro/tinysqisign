#!/usr/bin/env python3
"""Run the official v3 host gates for two implementations and all parameters."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PARAMETERS = {
    "p324_3": 270,
    "p500_27": 417,
    "p664_17": 549,
}
DEFAULT_OFFICIAL_COMMIT = "6d017708db403bf83977fa70770fc4f7f9e9ff21"
DEFAULT_ADAPTED_COMMIT = "874658c64aa2e20f53b1f4d696144723d558ed5c"


def run(
    args: list[str | Path], cwd: Path | None = None, capture: bool = True
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(arg) for arg in args],
        cwd=cwd,
        check=True,
        text=True,
        capture_output=capture,
    )


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def git(path: Path, *args: str) -> str:
    return run(["git", "-C", path, *args]).stdout.strip()


def configure_and_build(source: Path, build: Path) -> None:
    run(
        [
            "cmake",
            "-S",
            source,
            "-B",
            build,
            "-G",
            "Ninja",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DSQISIGN_BUILD_TYPE=ref",
            "-DGF_RADIX=32",
            "-DENABLE_SIGN=ON",
            "-DENABLE_TESTS=ON",
            "-DENABLE_STRICT=ON",
        ],
        capture=False,
    )
    targets: list[str] = []
    for parameter in PARAMETERS:
        targets.extend(
            [
                f"example_nistapi_{parameter}",
                f"sqisign_test_scheme_{parameter}",
                f"sqisign_test_kat_{parameter}",
            ]
        )
    run(["cmake", "--build", build, "--target", *targets, "--parallel"], capture=False)


def execute(binary: Path, cwd: Path) -> tuple[str, float]:
    start = time.monotonic()
    completed = run([binary], cwd=cwd)
    elapsed = time.monotonic() - start
    return completed.stdout, elapsed


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--official-source", type=Path, default=ROOT / "work/official-v3"
    )
    parser.add_argument(
        "--adapted-source", type=Path, default=ROOT / "work/v3-lowmem-d3"
    )
    parser.add_argument("--official-commit", default=DEFAULT_OFFICIAL_COMMIT)
    parser.add_argument("--adapted-commit", default=DEFAULT_ADAPTED_COMMIT)
    parser.add_argument(
        "--official-build", type=Path, default=ROOT / "build-host-v3-allparams-official"
    )
    parser.add_argument(
        "--adapted-build", type=Path, default=ROOT / "build-host-v3-allparams-adapted"
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=ROOT / "results/v3/host/validation-all-params-2026-09-04.json",
    )
    args = parser.parse_args()

    variants = {
        "official": (args.official_source, args.official_build, args.official_commit),
        "two_function_lifetime": (
            args.adapted_source,
            args.adapted_build,
            args.adapted_commit,
        ),
    }
    provenance: dict[str, object] = {}
    for label, (source, build, expected_commit) in variants.items():
        observed_commit = git(source, "rev-parse", "HEAD")
        if observed_commit != expected_commit:
            raise ValueError(f"unexpected {label} commit: {observed_commit}")
        if git(source, "status", "--porcelain", "--untracked-files=no"):
            raise ValueError(f"tracked source is dirty: {source}")
        provenance[label] = {
            "commit": observed_commit,
            "tracked_source_clean": True,
        }
        configure_and_build(source, build)

    rows: list[dict[str, object]] = []
    for label, (source, build, _expected_commit) in variants.items():
        kat_cwd = source / "build/test"
        kat_cwd.mkdir(parents=True, exist_ok=True)
        for parameter, secret_key_bytes in PARAMETERS.items():
            response = source / "KAT" / f"PQCsignKAT_{secret_key_bytes}_SQIsign_{parameter}.rsp"
            count = sum(
                line.startswith("count = ")
                for line in response.read_text(encoding="ascii").splitlines()
            )
            if count != 100:
                raise ValueError(f"unexpected vector count in {response}: {count}")

            api_output, api_seconds = execute(
                build / "apps" / f"example_nistapi_{parameter}", build / "apps"
            )
            if "crypto_sign_open (with altered signature) -> OK" not in api_output:
                raise ValueError(f"negative verification gate failed for {label}/{parameter}")
            selftest_output, selftest_seconds = execute(
                build / "test" / f"sqisign_test_scheme_{parameter}", build / "test"
            )
            if "Testing length validation of Open, Verify" not in selftest_output:
                raise ValueError(f"scheme self-test failed for {label}/{parameter}")
            kat_output, kat_seconds = execute(
                build / "test" / f"sqisign_test_kat_{parameter}", kat_cwd
            )
            if "Known Answer Tests PASSED." not in kat_output:
                raise ValueError(f"known-answer test failed for {label}/{parameter}")
            rows.append(
                {
                    "variant": label,
                    "parameter": parameter,
                    "official_response_vectors": count,
                    "response_sha256": sha256(response),
                    "nist_api_exit": 0,
                    "nist_api_output_sha256": sha256_bytes(api_output.encode()),
                    "nist_api_seconds": round(api_seconds, 6),
                    "selftest_exit": 0,
                    "selftest_output_sha256": sha256_bytes(selftest_output.encode()),
                    "selftest_seconds": round(selftest_seconds, 6),
                    "known_answer_exit": 0,
                    "known_answer_output_sha256": sha256_bytes(kat_output.encode()),
                    "known_answer_seconds": round(kat_seconds, 6),
                    "valid_signatures_accepted": True,
                    "altered_signature_rejected": True,
                }
            )
            print(f"{label}/{parameter}: API, self-test, 100-vector known-answer PASS")

    for parameter in PARAMETERS:
        digests = {
            row["response_sha256"]
            for row in rows
            if row["parameter"] == parameter
        }
        if len(digests) != 1:
            raise ValueError(f"response oracle differs between trees for {parameter}")

    result = {
        "schema": "sqisign-v3-two-function-all-parameter-host-validation-v1",
        "status": "PASS",
        "configuration": {
            "build_type": "Release",
            "implementation": "ref",
            "radix": 32,
            "strict_checks": True,
        },
        "provenance": provenance,
        "rows": rows,
        "decision": {
            "all_three_official_parameter_sets_tested": True,
            "both_implementations_tested": True,
            "official_response_oracle_shared_between_trees": True,
            "known_answer_vectors_passed": 600,
            "api_and_scheme_selftests_passed": 12,
            "functional_equivalence_established_for_all_inputs": False,
        },
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(f"v3 all-parameter host validation: PASS ({len(rows)} rows)")
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
