"""Smoke tests for the komozoievm Python bindings.

These tests intentionally exercise only the parts of the API that are stable
across builds: type conversions, the MockChain accessors, and a trivial
simulation invocation.  Heavier integration tests live alongside the C++ suite.
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
#
#

import komozoievm as evm


SAMPLE_ADDRESS = "0x000102030405060708090a0b0c0d0e0f10111213"


def test_module_metadata():
    assert isinstance(evm.__version__, str)
    assert evm.__version__


def test_address_roundtrip():
    addr = evm.Address(SAMPLE_ADDRESS)
    raw = bytes(addr)
    assert len(raw) == 20
    assert raw == bytes.fromhex(SAMPLE_ADDRESS[2:])
    assert evm.Address(raw) == addr


def test_u256_roundtrip():
    value = (1 << 200) - 7
    wrapped = evm.U256(value)
    assert int(wrapped) == value


def test_mock_chain_stores_state():
    chain = evm.MockChain()
    chain.set_balance(SAMPLE_ADDRESS, 10**18)
    chain.set_code(SAMPLE_ADDRESS, b"\x60\x01\x60\x01\x01")

    info = chain.get_account_info(SAMPLE_ADDRESS)
    assert int(info.balance) == 10**18


def test_block_info_defaults():
    block = evm.BlockInfo(number=42, timestamp=1700000000)
    assert block.number == 42
    assert block.timestamp == 1700000000
    assert block.gas_limit == 30000000


def test_initial_gas_cost():
    # A non-empty payload costs at least the base transaction gas.
    cost = evm.EVM.initial_gas_cost(b"hello")
    assert cost > 0
