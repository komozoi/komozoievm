"""Tests for EVM.simulate / EVM.execute on basic transactions."""

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


SENDER: str = "0x000000000000000000000000000000000000aaaa"
RECIPIENT: str = "0x000000000000000000000000000000000000bbbb"


# Minimal contract code that simply halts successfully.  The simulator
# refuses to call into an address without any deployed bytecode, so even a
# plain value transfer needs the recipient to carry at least a STOP.
STOP_BYTECODE: bytes = b"\x00"


@pytest.fixture()
def funded_chain(mock_chain: evm.MockChain) -> evm.MockChain:
    mock_chain.set_balance(SENDER, 10 ** 20)
    mock_chain.set_code(RECIPIENT, STOP_BYTECODE)
    return mock_chain


class TestInitialGasCost:
    def test_empty_payload(self) -> None:
        # An empty payload still has the base 21000 transaction gas cost.
        assert evm.EVM.initial_gas_cost(b"") >= 21000

    def test_zero_bytes_cheaper_than_nonzero(self) -> None:
        zeros: int = evm.EVM.initial_gas_cost(b"\x00" * 32)
        nonzero: int = evm.EVM.initial_gas_cost(b"\xff" * 32)
        assert nonzero > zeros


class TestValueTransferSimulation:
    def test_success_with_sufficient_balance(
            self, funded_chain: evm.MockChain, default_block: evm.BlockInfo) -> None:
        engine: evm.EVM = evm.EVM(funded_chain)
        tx: evm.Transaction = evm.Transaction.call(
            from_=SENDER, to=RECIPIENT, data=b"", gas_limit=100_000, value=1,
        )
        result: evm.SimulationResult = engine.simulate(tx, default_block)
        assert result.success is True, "simulation failed: %r" % result.reason
        assert isinstance(bytes(result.return_data), bytes)

    def test_simulate_does_not_mutate_chain(
            self, funded_chain: evm.MockChain, default_block: evm.BlockInfo) -> None:
        engine: evm.EVM = evm.EVM(funded_chain)
        tx: evm.Transaction = evm.Transaction.call(
            from_=SENDER, to=RECIPIENT, data=b"", gas_limit=100_000, value=42,
        )
        engine.simulate(tx, default_block)
        # Simulation must leave the backing store untouched.
        assert int(funded_chain.get_account_info(SENDER).balance) == 10 ** 20
        assert int(funded_chain.get_account_info(RECIPIENT).balance) == 0


class TestTransactionFactory:
    def test_fields_round_trip(self) -> None:
        tx: evm.Transaction = evm.Transaction.call(
            from_=SENDER,
            to=RECIPIENT,
            data=b"\xde\xad\xbe\xef",
            nonce=7,
            gas_limit=250_000,
            value=10 ** 17,
        )
        assert tx.nonce == 7
        assert tx.gas_limit == 250_000
        assert int(tx.value) == 10 ** 17
        assert bytes(tx.calldata) == b"\xde\xad\xbe\xef"

    def test_data_defaults_to_empty(self) -> None:
        tx: evm.Transaction = evm.Transaction.call(from_=SENDER, to=RECIPIENT)
        assert bytes(tx.calldata) == b""


class TestTransactionBuilder:
    def test_construct_with_data(self) -> None:
        builder: evm.TransactionBuilder = evm.TransactionBuilder(
            to=RECIPIENT, nonce=3, gas_limit=21000, value=5, data=b"\x01\x02",
        )
        assert builder.nonce == 3

    def test_builder_build(self) -> None:
        builder = evm.TransactionBuilder(
            to=RECIPIENT,
            nonce=1,
            gas_limit=100000,
            value=123
        )
        tx = builder.build(from_=SENDER)
        assert tx.nonce == 1
        assert tx.gas_limit == 100000
        assert int(tx.value) == 123
        assert str(tx.sender).lower() == SENDER.lower()
        assert str(tx.recipient).lower() == RECIPIENT.lower()

    def test_builder_sign_and_encode(self) -> None:
        builder = evm.TransactionBuilder(
            to=RECIPIENT,
            nonce=1,
            gas_limit=21000,
            value=10**18
        )
        # A random private key
        priv_key = bytes.fromhex("4c0883a69102937d6231471b5dbb6204fe5129617082792ae468d01a3f362318")
        encoded = builder.sign_and_encode(priv_key)
        assert len(encoded) > 0
        assert isinstance(encoded, bytes)

    def test_evm_simulate_with_builder(
            self, funded_chain: evm.MockChain, default_block: evm.BlockInfo) -> None:
        builder = evm.TransactionBuilder(
            to=RECIPIENT,
            nonce=0,
            gas_limit=21000,
            value=0
        )
        engine = evm.EVM(funded_chain)
        result = engine.simulate(builder, default_block)
        assert result.success is True


class TestExecution:
    def test_execute_mutates_chain_and_returns_data(
            self, funded_chain: evm.MockChain, default_block: evm.BlockInfo) -> None:
        # PUSH1 0x42 PUSH1 0x00 MSTORE PUSH1 0x20 PUSH1 0x00 RETURN
        # This code returns 32 bytes containing 0x42 at the end.
        return_42_bytecode: bytes = b"\x60\x42\x60\x00\x52\x60\x20\x60\x00\xf3"
        mock_addr: str = "0x000000000000000000000000000000000000cccc"
        funded_chain.set_code(mock_addr, return_42_bytecode)

        engine: evm.EVM = evm.EVM(funded_chain)
        tx: evm.Transaction = evm.Transaction.call(
            from_=SENDER, to=mock_addr, data=b"", gas_limit=100_000, value=7,
        )

        result: evm.SimulationResult = engine.execute(tx, default_block)

        assert result.success is True
        assert bytes(result.return_data) == b"\x00" * 31 + b"\x42"
        # Check that state was mutated (value transfer)
        assert int(funded_chain.get_account_info(mock_addr).balance) == 7
