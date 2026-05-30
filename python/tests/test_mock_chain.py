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

import komozoievm as evm


ALICE: str = "0x000000000000000000000000000000000000aaaa"
BOB: str = "0x000000000000000000000000000000000000bbbb"
CONTRACT: str = "0x00000000000000000000000000000000000c0de1"


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


class TestMockChainCode:
    def test_unknown_code_is_empty(self, mock_chain: evm.MockChain) -> None:
        code: evm.Bytes = mock_chain.get_contract_code(CONTRACT)
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
