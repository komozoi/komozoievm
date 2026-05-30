"""Simulate a call against real, committed contract bytecode.

This example is fully offline: it loads the deployed bytecode of WETH9 from
``bytecode/weth9.hex`` (originally taken from mainnet via Etherscan/eth_getCode),
installs it into a `MockChain`, and calls the `name()` view function.  The
return value is ABI-decoded and compared to the well-known string
"Wrapped Ether".  Any mismatch or simulation failure raises and the script
exits with a non-zero status, so this script doubles as a smoke test for an
installed `komozoievm` wheel.

Usage:

    python contract_call_simulation.py
"""

from __future__ import annotations

import os
import sys

import komozoievm as evm


BYTECODE_PATH: str = os.path.join(os.path.dirname(__file__), "bytecode", "weth9.hex")

# Mainnet WETH9.  The address is arbitrary for an offline simulation but
# matches the source so the example reads naturally.
WETH9_ADDRESS: str = "0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2"

# Any 20-byte EOA works as the simulated caller.
CALLER_ADDRESS: str = "0x000000000000000000000000000000000000aaaa"

# Function selector for `name()` = keccak256("name()")[:4]
NAME_SELECTOR: bytes = bytes.fromhex("06fdde03")

EXPECTED_NAME: str = "Wrapped Ether"


def encode_short_string_slot(text: str) -> int:
	"""Pack a <32-byte string into a single Solidity storage slot.

	Pre-0.5 Solidity stores short `string`/`bytes` state variables inline:
	the UTF-8 bytes are left-aligned in the slot and the lowest byte holds
	`length * 2`.  WETH9 uses this layout for `name`, `symbol`, etc.
	"""
	raw: bytes = text.encode("utf-8")
	if len(raw) >= 32:
		raise ValueError("short-string encoding only supports < 32 bytes")
	packed: bytes = raw + b"\x00" * (31 - len(raw)) + bytes([len(raw) * 2])
	return int.from_bytes(packed, "big")


def load_bytecode(path: str) -> bytes:
	"""Read a hex-encoded contract bytecode file into raw bytes."""
	with open(path, "r") as f:
		text: str = f.read().strip()
	if text.startswith("0x") or text.startswith("0X"):
		text = text[2:]
	return bytes.fromhex(text)


def decode_abi_string(data: bytes) -> str:
	"""Decode a single ABI-encoded `string` return value."""
	if len(data) < 64:
		raise ValueError("return data too short to be an ABI string: %d bytes" % len(data))
	offset: int = int.from_bytes(data[0:32], "big")
	if offset + 32 > len(data):
		raise ValueError("ABI string offset %d out of range" % offset)
	length: int = int.from_bytes(data[offset:offset + 32], "big")
	start: int = offset + 32
	end: int = start + length
	if end > len(data):
		raise ValueError("ABI string body truncated")
	return data[start:end].decode("utf-8")


def main() -> int:
	runtime_code: bytes = load_bytecode(BYTECODE_PATH)
	print("Loaded WETH9 bytecode: %d bytes" % len(runtime_code))

	chain: evm.MockChain = evm.MockChain()
	chain.set_code(WETH9_ADDRESS, runtime_code)
	chain.set_balance(CALLER_ADDRESS, 10 ** 18)
	# WETH9 stores `string public name` in storage slot 0 using the
	# pre-0.5 Solidity short-string layout.  We seed it so `name()` has
	# something to return.
	chain.set_storage(WETH9_ADDRESS, 0, encode_short_string_slot(EXPECTED_NAME))

	block: evm.BlockInfo = evm.BlockInfo(
		number=18_000_000,
		timestamp=1_700_000_000,
		base_fee=0,
		gas_limit=30_000_000,
		chain_id=1,
	)

	tx: evm.Transaction = evm.Transaction.call(
		from_=CALLER_ADDRESS,
		to=WETH9_ADDRESS,
		data=NAME_SELECTOR,
		gas_limit=1_000_000,
	)

	engine: evm.EVM = evm.EVM(chain)
	result: evm.SimulationResult = engine.simulate(tx, block)

	if not result.success:
		raise RuntimeError("Simulation failed: %s" % (result.reason or "<no reason>"))

	return_data: bytes = bytes(result.return_data)
	if not return_data:
		raise RuntimeError("Simulation returned no data; bytecode may be incomplete")

	decoded: str = decode_abi_string(return_data)
	print("Decoded name():", repr(decoded))

	if decoded != EXPECTED_NAME:
		raise AssertionError(
			"Unexpected return: expected %r, got %r" % (EXPECTED_NAME, decoded))

	print("OK: simulator reproduced WETH9.name() == %r" % EXPECTED_NAME)
	return 0


if __name__ == "__main__":
	sys.exit(main())
