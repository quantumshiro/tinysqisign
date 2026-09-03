#!/usr/bin/env python3
"""LLDB callback for read-only profiling of SQIsign ibz_pow_mod calls.

Import from LLDB, set a breakpoint on sqisign_gen_ibz_pow_mod, and attach
`powmod_breakpoint` as its Python callback.  The callback reads the positive
Level-I exponent/modulus at function entry and emits one machine-readable row.
It never changes target memory or registers.
"""

from __future__ import annotations

import json


IBZ_LIMBS = 27
IBZ_BYTES = IBZ_LIMBS * 8


def _read_unsigned(process, address: int) -> int:
    import lldb

    status = lldb.SBError()
    raw = process.ReadMemory(address, IBZ_BYTES, status)
    if not status.Success() or len(raw) != IBZ_BYTES:
        raise RuntimeError(
            f"cannot read {IBZ_BYTES} bytes at 0x{address:x}: {status}"
        )
    return int.from_bytes(raw, "little", signed=False)


def _frame_names(frame) -> list[str]:
    names: list[str] = []
    thread = frame.GetThread()
    for index in range(min(thread.GetNumFrames(), 32)):
        name = thread.GetFrameAtIndex(index).GetFunctionName()
        names.append(name if name is not None else "<unknown>")
    return names


def powmod_breakpoint(frame, _location, _internal_dict) -> bool:
    """Emit one JSON row and continue without stopping."""

    process = frame.GetThread().GetProcess()
    exponent_address = frame.FindRegister("x2").GetValueAsUnsigned()
    modulus_address = frame.FindRegister("x3").GetValueAsUnsigned()
    exponent = _read_unsigned(process, exponent_address)
    modulus = _read_unsigned(process, modulus_address)
    stack = _frame_names(frame)

    row = {
        "event": "ibz_pow_mod",
        "exponent_bits": exponent.bit_length(),
        "exponent_weight": bin(exponent).count("1"),
        "modulus_bits": modulus.bit_length(),
        "modulus_mod_8": modulus & 7,
        "in_sign": any("sqisign_sign_with_workspace" in name for name in stack),
        "in_cornacchia": any("ibz_cornacchia_prime" in name for name in stack),
        "in_sqrt_mod": any("ibz_sqrt_mod_p" in name for name in stack),
        "stack": stack[:12],
    }
    print("SQISIGN_POWMOD_PROFILE " + json.dumps(row, separators=(",", ":")), flush=True)
    return False


def sqrt_mod_breakpoint(frame, _location, _internal_dict) -> bool:
    """Profile one modular-square-root entry and continue.

    At AArch64 function entry x1 is the radicand and x2 is the modulus for
    ibz_sqrt_mod_p(sqrt, a, p).  The derived q/e values correspond to this
    implementation's Tonelli--Shanks setup and make the published attack's
    exponent/modulus relationship explicit without stopping at every powmod.
    """

    process = frame.GetThread().GetProcess()
    radicand = _read_unsigned(
        process, frame.FindRegister("x1").GetValueAsUnsigned()
    )
    modulus = _read_unsigned(
        process, frame.FindRegister("x2").GetValueAsUnsigned()
    )
    stack = _frame_names(frame)
    if modulus > 1:
        q = modulus - 1
        two_adicity = 0
        while (q & 1) == 0:
            q >>= 1
            two_adicity += 1
        x_exponent = (q + 1) >> 1
    else:
        q = 0
        two_adicity = 0
        x_exponent = 0

    row = {
        "event": "ibz_sqrt_mod_p",
        "radicand_bits": radicand.bit_length(),
        "modulus_bits": modulus.bit_length(),
        "modulus_mod_8": modulus & 7,
        "two_adicity": two_adicity,
        "legendre_exponent_bits": ((modulus - 1) >> 1).bit_length()
        if modulus > 1
        else 0,
        "q_exponent_bits": q.bit_length(),
        "q_exponent_weight": bin(q).count("1"),
        "x_exponent_bits": x_exponent.bit_length(),
        "x_exponent_weight": bin(x_exponent).count("1"),
        "in_sign": any("sqisign_sign" in name for name in stack),
        "in_cornacchia": any("ibz_cornacchia_prime" in name for name in stack),
        "stack": stack[:12],
    }
    print("SQISIGN_SQRT_PROFILE " + json.dumps(row, separators=(",", ":")), flush=True)
    return False
