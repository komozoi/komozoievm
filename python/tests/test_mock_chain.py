"""Tests for the in-memory MockChain state provider."""

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

import pytest

import komozoievm as evm


ALICE: str = "0x000000000000000000000000000000000000aaaa"
BOB: str = "0x000000000000000000000000000000000000bbbb"
CONTRACT: str = "0x00000000000000000000000000000000000c0de1"
INNER: str = "0x0000000000000000000000000000000000002000"

# STOP: a contract that immediately halts successfully.
STOP_BYTECODE: bytes = b"\x00"


class TestMockChainAccounts:
    def test_unknown_account_returns_zero(self, mock_chain: evm.MockChain) -> None:
        info: evm.AccountInfo = mock_chain.get_account_info(ALICE)
        assert int(info.balance) == 0

    def test_set_balance(self, mock_chain: evm.MockChain) -> None:
        mock_chain.set_balance(ALICE, 10 ** 20)
        assert int(mock_chain.get_account_info(ALICE).balance) == 10 ** 20

    def test_balance_per_address(self, mock_chain: evm.MockChain) -> None:
        mock_chain.set_balance(ALICE, 1)
        mock_chain.set_balance(BOB, 2)
        assert int(mock_chain.get_account_info(ALICE).balance) == 1
        assert int(mock_chain.get_account_info(BOB).balance) == 2

    def test_overwrite_balance(self, mock_chain: evm.MockChain) -> None:
        mock_chain.set_balance(ALICE, 5)
        mock_chain.set_balance(ALICE, 9)
        assert int(mock_chain.get_account_info(ALICE).balance) == 9

    def test_unknown_account_next_nonce_is_zero(self, mock_chain: evm.MockChain) -> None:
        # A freshly-defaulted AccountInfo must report a zero nonce
        # rather than uninitialized memory.
        info: evm.AccountInfo = mock_chain.get_account_info(ALICE)
        assert info.next_nonce == 0

    def test_set_balance_zeroes_nonce(self, mock_chain: evm.MockChain) -> None:
        mock_chain.set_balance(ALICE, 1)
        assert mock_chain.get_account_info(ALICE).next_nonce == 0


class TestKomozoiReprs:
    """Every public komozoi struct exposes a useful __repr__/__str__."""

    def test_account_info_repr_lists_fields(self, mock_chain: evm.MockChain) -> None:
        mock_chain.set_balance(ALICE, 1234)
        info: evm.AccountInfo = mock_chain.get_account_info(ALICE)
        text: str = repr(info)
        assert text.startswith("AccountInfo(")
        assert "balance=1234" in text
        assert "next_nonce=0" in text
        # __str__ falls back to __repr__ and must not return the
        # default <... object at 0x...> form.
        assert "object at 0x" not in str(info)

    def test_block_info_repr(self) -> None:
        block: evm.BlockInfo = evm.BlockInfo(number=42, timestamp=7, base_fee=3)
        text: str = repr(block)
        assert text.startswith("BlockInfo(")
        assert "number=42" in text
        assert "timestamp=7" in text

    def test_simulation_result_repr(self, mock_chain: evm.MockChain) -> None:
        mock_chain.set_balance(ALICE, 10 ** 18)
        result: evm.SimulationResult = mock_chain.execute(BOB, ALICE)
        text: str = repr(result)
        assert text.startswith("SimulationResult(")
        assert "success=True" in text
        assert "gas_used=" in text

    def test_call_trace_entry_repr(self, mock_chain: evm.MockChain) -> None:
        mock_chain.set_balance(ALICE, 10 ** 18)
        mock_chain.set_code(INNER, STOP_BYTECODE)
        # Outer contract performs CALL into INNER, then STOPs.
        call_code: bytes = (
            b"\x60\x00\x60\x00\x60\x00\x60\x00\x60\x00\x73"
            + bytes.fromhex(INNER[2:])
            + b"\x63\xff\xff\xff\xff\xf1\x00"
        )
        mock_chain.set_code(CONTRACT, call_code)
        trace: evm.CallTrace = evm.CallTrace()
        mock_chain.execute(CONTRACT, ALICE, trace=trace)
        assert len(trace.calls) >= 1
        text: str = repr(trace.calls[0])
        assert text.startswith("CallTraceEntry(")
        assert "success=True" in text


class TestMockChainCode:
    def test_unknown_code_is_empty(self, mock_chain: evm.MockChain) -> None:
        code: bytes = mock_chain.get_contract_code(CONTRACT)
        assert len(code) == 0

    def test_set_and_get_code(self, mock_chain: evm.MockChain) -> None:
        payload: bytes = b"\x60\x01\x60\x02\x01"
        mock_chain.set_code(CONTRACT, payload)
        assert bytes(mock_chain.get_contract_code(CONTRACT)) == payload


class TestMockChainStorage:
    def test_set_storage_does_not_raise(self, mock_chain: evm.MockChain) -> None:
        # The set_storage helper only seeds backing state; reads happen
        # through the EVM during simulation, not directly from Python.
        mock_chain.set_storage(CONTRACT, 0, 0xDEADBEEF)
        mock_chain.set_storage(CONTRACT, 2 ** 200, 1)


class TestMockChainCallAPI:
    """High-level ``simulate``/``execute`` entry points that skip the builder."""

    def _funded(self, chain: evm.MockChain) -> None:
        chain.set_balance(ALICE, 10 ** 20)
        chain.set_code(CONTRACT, STOP_BYTECODE)

    def test_simulate_returns_result_types(self, mock_chain: evm.MockChain) -> None:
        self._funded(mock_chain)
        result: evm.SimulationResult = mock_chain.simulate(CONTRACT, ALICE)
        assert isinstance(result.success, bool)
        assert isinstance(result.return_data, bytes)
        assert isinstance(result.gas_used, int)
        assert isinstance(result.reason, str)
        assert result.success is True

    def test_simulate_default_block(self, mock_chain: evm.MockChain) -> None:
        self._funded(mock_chain)
        # No block argument: an internal default is used.
        assert mock_chain.simulate(CONTRACT, ALICE).success is True

    def test_simulate_does_not_mutate_state(self, mock_chain: evm.MockChain) -> None:
        self._funded(mock_chain)
        mock_chain.simulate(CONTRACT, ALICE, value=42)
        assert int(mock_chain.get_account_info(ALICE).balance) == 10 ** 20
        assert int(mock_chain.get_account_info(CONTRACT).balance) == 0

    def test_execute_mutates_balances(self, mock_chain: evm.MockChain) -> None:
        self._funded(mock_chain)
        result: evm.SimulationResult = mock_chain.execute(CONTRACT, ALICE, value=7)
        assert result.success is True
        assert int(mock_chain.get_account_info(CONTRACT).balance) == 7

    def test_execute_accepts_float_value(self, mock_chain: evm.MockChain) -> None:
        self._funded(mock_chain)
        # 1e18 wei = 1 ETH; the API converts the float to int internally.
        result: evm.SimulationResult = mock_chain.execute(CONTRACT, ALICE, value=1e18)
        assert result.success is True
        assert int(mock_chain.get_account_info(CONTRACT).balance) == 10 ** 18

    def test_execute_accepts_data_kwarg(self, mock_chain: evm.MockChain) -> None:
        self._funded(mock_chain)
        result: evm.SimulationResult = mock_chain.execute(CONTRACT, ALICE, data=b"\xab\xcd\xef")
        assert result.success is True

    def test_execute_to_eoa_succeeds(self, mock_chain: evm.MockChain) -> None:
        # An address with no contract code is an EOA: the call must
        # succeed and transfer value, not raise "Out of gas".
        mock_chain.set_balance(ALICE, 10 ** 20)
        result: evm.SimulationResult = mock_chain.execute(BOB, ALICE, value=42)
        assert result.success is True
        assert result.reason == ""
        assert int(mock_chain.get_account_info(BOB).balance) == 42

    def test_set_code_disables_eoa_path(self, mock_chain: evm.MockChain) -> None:
        # Installing code turns the address into a contract: an INVALID
        # opcode now causes the call to fail instead of taking the
        # zero-code EOA shortcut.
        mock_chain.set_balance(ALICE, 10 ** 20)
        mock_chain.set_code(BOB, b"\xfe")
        result: evm.SimulationResult = mock_chain.execute(BOB, ALICE)
        assert result.success is False

    def test_execute_gas_default_changes_with_kwarg(self, mock_chain: evm.MockChain) -> None:
        # An infinite loop runs until gas is exhausted; the default 3M
        # budget must consume more gas than an explicit smaller budget.
        # JUMPDEST then PUSH1 0x00 JUMP (jumps to PC 0).
        loop_code: bytes = b"\x5b\x60\x00\x56"
        mock_chain.set_balance(ALICE, 10 ** 20)
        mock_chain.set_code(CONTRACT, loop_code)
        default_result: evm.SimulationResult = mock_chain.execute(CONTRACT, ALICE)
        small_result: evm.SimulationResult = mock_chain.execute(CONTRACT, ALICE, gas=100_000)
        assert default_result.success is False
        assert small_result.success is False
        # Explicit 100k budget consumes ~2.9M fewer gas than the 3M default.
        assert default_result.gas_used > small_result.gas_used + 2_500_000

    def test_execute_accepts_float_gas(self, mock_chain: evm.MockChain) -> None:
        # Floats are rounded to the nearest integer.
        self._funded(mock_chain)
        result: evm.SimulationResult = mock_chain.execute(CONTRACT, ALICE, gas=1e6)
        assert result.success is True

    def test_trace_captures_nested_calls(self, mock_chain: evm.MockChain) -> None:
        mock_chain.set_balance(ALICE, 10 ** 20)
        mock_chain.set_code(INNER, STOP_BYTECODE)
        # Outer contract performs CALL into INNER, then STOP.
        call_code: bytes = (
            b"\x60\x00\x60\x00\x60\x00\x60\x00\x60\x00\x73"
            + bytes.fromhex(INNER[2:])
            + b"\x63\xff\xff\xff\xff\xf1\x00"
        )
        mock_chain.set_code(CONTRACT, call_code)

        trace: evm.CallTrace = evm.CallTrace()
        result: evm.SimulationResult = mock_chain.execute(CONTRACT, ALICE, trace=trace)
        assert result.success is True
        assert len(trace) >= 1
        assert any(str(c.dst).lower() == INNER.lower() for c in trace.calls)


class TestAddressValidation:
    """Malformed addresses must be rejected at the Python boundary."""

    def test_ellipsis_in_address_is_rejected(self, mock_chain: evm.MockChain) -> None:
        with pytest.raises(ValueError):
            mock_chain.set_balance("0xabcd...01", 1)

    def test_short_hex_is_rejected(self, mock_chain: evm.MockChain) -> None:
        with pytest.raises(ValueError):
            mock_chain.set_balance("0x123", 1)

    def test_invalid_hex_chars_are_rejected(self, mock_chain: evm.MockChain) -> None:
        with pytest.raises(ValueError):
            mock_chain.set_balance("0x" + "G" * 40, 1)
