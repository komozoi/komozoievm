"""Tests for subclassing StateProvider from Python."""

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


CALLER: str = "0x000000000000000000000000000000000000aaaa"
TARGET: str = "0x000000000000000000000000000000000000bbbb"


class RecordingProvider(evm.StateProvider):
    """StateProvider that tracks which addresses were queried.

    Used to verify that the C++ engine actually routes lookups back into
    Python when a custom provider is installed.
    """

    def __init__(self, balances: dict, codes: dict) -> None:
        super().__init__()
        self._balances: dict = balances
        self._codes: dict = codes
        self.account_queries: list = []
        self.code_queries: list = []

    @staticmethod
    def _key(address: object) -> str:
        # Accept either a wrapped Address (which exposes __bytes__) or a
        # hex string, returning a normalised lowercase hex key.
        if isinstance(address, str):
            return address[2:].lower() if address.startswith("0x") else address.lower()
        return bytes(address).hex()

    def get_account_info(self, address, block_number: int = 0) -> evm.AccountInfo:
        key: str = self._key(address)
        self.account_queries.append(key)
        balance: int = self._balances.get(key, 0)
        return evm.AccountInfo(address, balance=balance, next_nonce=0)

    def get_contract_code(self, address) -> bytes:
        key: str = self._key(address)
        self.code_queries.append(key)
        return self._codes.get(key, b"")


class TestPythonStateProvider:
    def test_bare_state_provider_requires_overrides(self) -> None:
        # Without a Python subclass providing overrides the trampoline raises
        # because the C++ side declares the lookups as pure virtual.
        import pytest
        provider: evm.StateProvider = evm.StateProvider()
        with pytest.raises(RuntimeError):
            provider.get_account_info(CALLER)

    def test_subclass_overrides_are_invoked_directly(self) -> None:
        balances: dict = {CALLER[2:]: 10 ** 18}
        provider: RecordingProvider = RecordingProvider(balances, {})

        info: evm.AccountInfo = provider.get_account_info(CALLER)
        assert int(info.balance) == 10 ** 18
        assert provider.account_queries == [CALLER[2:]]

    def test_subclass_called_during_simulation(self, default_block: evm.BlockInfo) -> None:
        # Seed both accounts: balance for the caller, a STOP at the target so
        # the simulator has bytecode to execute.
        balances: dict = {CALLER[2:]: 10 ** 18}
        codes: dict = {TARGET[2:]: b"\x00"}
        provider: RecordingProvider = RecordingProvider(balances, codes)

        engine: evm.EVM = evm.EVM(provider)
        tx: evm.Transaction = evm.Transaction.call(
            from_=CALLER,
            to=TARGET,
            data=b"",
            gas_limit=100_000,
            value=1,
        )
        result: evm.SimulationResult = engine.simulate(tx, default_block)

        assert result.success, "simulation failed: %r" % result.reason
        # The engine must have asked our Python provider for the recipient's
        # bytecode at least once during execution.
        assert TARGET[2:] in provider.code_queries
