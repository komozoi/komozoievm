"""End-to-end test: run real WETH9 bytecode through the simulator.

This is the regression counterpart to ``examples/python/contract_call_simulation.py``.
It uses the committed mainnet WETH9 runtime bytecode and verifies that a
``name()`` call returns the expected ABI-encoded "Wrapped Ether" string.
"""

#  Copyright 2025-2026 komozoi
#  Original Creation Date: 2026-5-30
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from __future__ import annotations

import komozoievm as evm


WETH9_ADDRESS: str = "0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2"
CALLER_ADDRESS: str = "0x000000000000000000000000000000000000aaaa"

# keccak256("name()")[:4]
NAME_SELECTOR: bytes = bytes.fromhex("06fdde03")
EXPECTED_NAME: str = "Wrapped Ether"


def _encode_short_string_slot(text: str) -> int:
    """Pack a <32-byte string into a single Solidity storage slot (pre-0.5)."""
    raw: bytes = text.encode("utf-8")
    if len(raw) >= 32:
        raise ValueError("short-string encoding only supports < 32 bytes")
    packed: bytes = raw + b"\x00" * (31 - len(raw)) + bytes([len(raw) * 2])
    return int.from_bytes(packed, "big")


def _decode_abi_string(data: bytes) -> str:
    offset: int = int.from_bytes(data[0:32], "big")
    length: int = int.from_bytes(data[offset:offset + 32], "big")
    start: int = offset + 32
    return data[start:start + length].decode("utf-8")


class TestWeth9NameCall:
    def test_name_returns_wrapped_ether(
            self,
            weth9_runtime_bytecode: bytes,
            mock_chain: evm.MockChain,
            default_block: evm.BlockInfo) -> None:
        mock_chain.set_code(WETH9_ADDRESS, weth9_runtime_bytecode)
        mock_chain.set_balance(CALLER_ADDRESS, 10 ** 18)
        mock_chain.set_storage(WETH9_ADDRESS, 0, _encode_short_string_slot(EXPECTED_NAME))

        engine: evm.EVM = evm.EVM(mock_chain)
        tx: evm.Transaction = evm.Transaction.call(
            from_=CALLER_ADDRESS,
            to=WETH9_ADDRESS,
            data=NAME_SELECTOR,
            gas_limit=1_000_000,
        )
        result: evm.SimulationResult = engine.simulate(tx, default_block)

        assert result.success, "simulation failed: %r" % result.reason
        return_data: bytes = bytes(result.return_data)
        assert len(return_data) >= 64, "return data too short: %d" % len(return_data)
        assert _decode_abi_string(return_data) == EXPECTED_NAME

    def test_unset_storage_returns_empty_string(
            self,
            weth9_runtime_bytecode: bytes,
            mock_chain: evm.MockChain,
            default_block: evm.BlockInfo) -> None:
        # Same call but without seeding storage slot 0: the contract should
        # still run successfully and report a zero-length name.
        mock_chain.set_code(WETH9_ADDRESS, weth9_runtime_bytecode)
        mock_chain.set_balance(CALLER_ADDRESS, 10 ** 18)

        engine: evm.EVM = evm.EVM(mock_chain)
        tx: evm.Transaction = evm.Transaction.call(
            from_=CALLER_ADDRESS,
            to=WETH9_ADDRESS,
            data=NAME_SELECTOR,
            gas_limit=1_000_000,
        )
        result: evm.SimulationResult = engine.simulate(tx, default_block)

        assert result.success
        return_data: bytes = bytes(result.return_data)
        assert _decode_abi_string(return_data) == ""
