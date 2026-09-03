#!/usr/bin/env python3
"""Read-only LLDB callbacks for the prime random-ideal rejection loops.

The callbacks stop at the first executable line of the gamma and beta loops
in the frozen D13 ``normeq.c``.  They read debug metadata only, emit one JSON
event, and immediately continue.  Target registers and memory are never
modified.
"""

from __future__ import annotations

import json


IBZ_LIMBS = 27
IBZ_BYTES = IBZ_LIMBS * 8

_seed = -1
_call_index = 0
_phase = "none"
_gamma_attempt = 0
_beta_attempt = 0


def set_seed(seed: int) -> None:
    """Reset callback state before launching one deterministic Sign run."""

    global _seed, _call_index, _phase, _gamma_attempt, _beta_attempt
    _seed = int(seed)
    _call_index = 0
    _phase = "none"
    _gamma_attempt = 0
    _beta_attempt = 0


def _read_unsigned(process, address: int) -> int:
    import lldb

    status = lldb.SBError()
    raw = process.ReadMemory(address, IBZ_BYTES, status)
    if not status.Success() or len(raw) != IBZ_BYTES:
        raise RuntimeError(
            f"cannot read {IBZ_BYTES} bytes at 0x{address:x}: {status}"
        )
    return int.from_bytes(raw, "little", signed=False)


def _norm(frame) -> int:
    value = frame.FindVariable("norm")
    address = value.GetValueAsUnsigned()
    if address == 0:
        raise RuntimeError("LLDB could not resolve the random-ideal norm")
    return _read_unsigned(frame.GetThread().GetProcess(), address)


def _stack(frame) -> list[str]:
    thread = frame.GetThread()
    result: list[str] = []
    for index in range(min(thread.GetNumFrames(), 12)):
        name = thread.GetFrameAtIndex(index).GetFunctionName()
        result.append(name if name is not None else "<unknown>")
    return result


def _emit(frame, phase: str, attempt: int) -> None:
    norm = _norm(frame)
    row = {
        "event": "random_ideal_candidate",
        "seed": _seed,
        "call_index": _call_index,
        "phase": phase,
        "attempt": attempt,
        "norm_bits": norm.bit_length(),
        "norm_hex": format(norm, "x"),
        "stack": _stack(frame),
    }
    print(
        "SQISIGN_RANDOM_IDEAL_PROFILE "
        + json.dumps(row, separators=(",", ":")),
        flush=True,
    )


def gamma_breakpoint(frame, _location, _internal_dict) -> bool:
    """Count one entry into ``while (!found)`` and continue."""

    global _call_index, _phase, _gamma_attempt, _beta_attempt
    if _phase != "gamma":
        _call_index += 1
        _phase = "gamma"
        _gamma_attempt = 0
        _beta_attempt = 0
    _gamma_attempt += 1
    _emit(frame, "gamma", _gamma_attempt)
    return False


def beta_breakpoint(frame, _location, _internal_dict) -> bool:
    """Count one entry into ``while (!beta_ok)`` and continue."""

    global _phase, _beta_attempt
    if _phase != "beta":
        if _phase != "gamma":
            raise RuntimeError("beta loop reached without a gamma loop")
        _phase = "beta"
        _beta_attempt = 0
    _beta_attempt += 1
    _emit(frame, "beta", _beta_attempt)
    return False
