"""Shared pytest fixtures for the komozoievm test suite."""

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

import os

import pytest

import komozoievm as evm


REPO_ROOT: str = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
WETH9_BYTECODE_PATH: str = os.path.join(REPO_ROOT, "examples", "python", "bytecode", "weth9.hex")


@pytest.fixture(scope="session")
def weth9_runtime_bytecode() -> bytes:
    """Deployed WETH9 runtime bytecode, captured offline from mainnet."""
    with open(WETH9_BYTECODE_PATH, "r") as f:
        text: str = f.read().strip()
    if text.startswith("0x") or text.startswith("0X"):
        text = text[2:]
    return bytes.fromhex(text)


@pytest.fixture()
def mock_chain() -> "evm.MockChain":
    """A fresh, empty in-memory chain for each test."""
    return evm.MockChain()


@pytest.fixture()
def default_block() -> "evm.BlockInfo":
    """A reasonable mainnet-like block context for simulation."""
    return evm.BlockInfo(
        number=18_000_000,
        timestamp=1_700_000_000,
        base_fee=0,
        gas_limit=30_000_000,
        chain_id=1,
    )
