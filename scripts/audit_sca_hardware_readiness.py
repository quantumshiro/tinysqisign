#!/usr/bin/env python3
"""Freeze a conservative host-local inventory for an analog SCA campaign.

USB and software discovery is automatic.  Passive probes and instruments that
are not visible to the host must be supplied as explicit inventory statements;
the output keeps those declarations separate from machine observations.
"""

from __future__ import annotations

import argparse
import datetime as dt
import glob
import hashlib
import importlib.util
import json
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
INSTRUMENT_KEYWORDS = (
    "picoscope",
    "saleae",
    "chipwhisperer",
    "oscilloscope",
    "lecroy",
    "tektronix",
    "keysight",
    "national instruments",
)
COMMANDS = ("sigrok-cli", "picoscope", "saleae", "chipwhisperer")
MODULES = ("chipwhisperer", "numpy", "scipy", "pyvisa", "picosdk")
PREPARED_FILES = (
    "experiments/sca/cornacchia-analog-plan.json",
    "experiments/sca/analog-trace-dataset-template.json",
    "scripts/analyze_sca_analog_tvla.py",
    "experiments/sca/exponent-recovery-campaign-template.json",
    "scripts/score_sca_exponent_recovery.py",
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def run_ioreg() -> str:
    command = ["ioreg", "-p", "IOUSB", "-w", "0", "-l", "-r", "-c", "IOUSBHostDevice"]
    if shutil.which(command[0]) is None:
        return ""
    return subprocess.run(
        command, check=False, text=True, capture_output=True
    ).stdout


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--output",
        type=Path,
        default=ROOT / "results/rp2350/sca-analog-readiness-2026-09-04.json",
    )
    parser.add_argument(
        "--declared-passive-probes",
        choices=("absent", "present", "unknown"),
        required=True,
    )
    parser.add_argument(
        "--declared-acquisition-instrument",
        choices=("absent", "present", "unknown"),
        required=True,
    )
    parser.add_argument(
        "--current-target-firmware",
        default="unknown",
        help="Human-readable identity only; no claim that this is acquisition firmware.",
    )
    args = parser.parse_args()

    usb_text = run_ioreg()
    usb_lower = usb_text.lower()
    serial_devices = sorted(glob.glob("/dev/cu.usbmodem*"))
    instrument_keyword_hits = sorted(
        keyword for keyword in INSTRUMENT_KEYWORDS if keyword in usb_lower
    )
    pico_detected = "pico" in usb_lower and bool(serial_devices)

    prepared = {}
    for relative in PREPARED_FILES:
        path = ROOT / relative
        if not path.is_file():
            raise FileNotFoundError(path)
        prepared[relative] = {
            "bytes": path.stat().st_size,
            "sha256": sha256(path),
        }

    commands = {name: shutil.which(name) for name in COMMANDS}
    modules = {name: importlib.util.find_spec(name) is not None for name in MODULES}
    now = dt.datetime.now(dt.timezone.utc).replace(microsecond=0)
    passive_absent = args.declared_passive_probes == "absent"
    instrument_absent = args.declared_acquisition_instrument == "absent"
    executable = (
        args.declared_passive_probes == "present"
        and args.declared_acquisition_instrument == "present"
    )

    report = {
        "schema": "sqisign-sca-analog-readiness-v3",
        "observed_utc": now.isoformat().replace("+00:00", "Z"),
        "host_timezone": "Asia/Tokyo",
        "machine_observation": {
            "serial_devices": serial_devices,
            "pico_usb_target_detected": pico_detected,
            "usb_instrument_keyword_hits": instrument_keyword_hits,
            "usb_scope_or_sca_device_detected": bool(instrument_keyword_hits),
            "passive_probe_presence_machine_detectable": False,
            "only_pico_target_and_ordinary_usb_infrastructure_detected": (
                pico_detected and not instrument_keyword_hits
            ),
        },
        "declared_equipment_inventory": {
            "passive_current_or_near_field_probe": args.declared_passive_probes,
            "scope_or_sca_acquisition_instrument": (
                args.declared_acquisition_instrument
            ),
            "declaration_is_not_machine_detection": True,
        },
        "target": {
            "current_firmware": args.current_target_firmware,
            "current_firmware_is_analog_acquisition_image": False,
        },
        "software_inventory": {
            "commands": commands,
            "python_modules": modules,
        },
        "prepared_analysis": prepared,
        "decision": {
            "analog_capture_run": False,
            "physical_trace_count": 0,
            "analog_campaign_executable_in_current_hardware_state": executable,
            "external_hardware_blocker_recorded": passive_absent and instrument_absent,
            "side_channel_resistance_established": False,
        },
        "scope": (
            "One host-local USB/software inventory plus explicit author equipment "
            "declarations. Passive hardware cannot be discovered by this script. No "
            "power or EM sample was acquired, so this is a blocker record rather than "
            "a side-channel result."
        ),
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        "analog readiness audit: "
        f"pico={str(pico_detected).lower()} "
        f"usb_instrument={str(bool(instrument_keyword_hits)).lower()} "
        f"physical_traces=0 executable={str(executable).lower()}"
    )
    print(f"output={args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
