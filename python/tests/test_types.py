"""Tests for the basic value types exposed by the bindings."""

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

from __future__ import annotations

import pytest

import komozoievm as evm


ADDRESS_HEX: str = "0x000102030405060708090a0b0c0d0e0f10111213"
ADDRESS_RAW: bytes = bytes.fromhex(ADDRESS_HEX[2:])


class TestAddress:
    def test_from_hex_string(self) -> None:
        assert bytes(evm.Address(ADDRESS_HEX)) == ADDRESS_RAW

    def test_from_raw_bytes(self) -> None:
        assert bytes(evm.Address(ADDRESS_RAW)) == ADDRESS_RAW

    def test_str_round_trips_lowercase(self) -> None:
        addr: evm.Address = evm.Address(ADDRESS_HEX)
        assert str(addr).lower().endswith(ADDRESS_HEX[2:].lower())

    def test_equality(self) -> None:
        assert evm.Address(ADDRESS_HEX) == evm.Address(ADDRESS_RAW)

    def test_hashable(self) -> None:
        bucket: dict = {evm.Address(ADDRESS_HEX): 1}
        assert bucket[evm.Address(ADDRESS_RAW)] == 1

    def test_zero_address(self) -> None:
        zero: evm.Address = evm.Address("0x0000000000000000000000000000000000000000")
        assert bytes(zero) == b"\x00" * 20

    def test_from_none(self) -> None:
        assert bytes(evm.Address(None)) == b"\x00" * 20

    def test_from_address_object(self) -> None:
        addr1 = evm.Address(ADDRESS_HEX)
        addr2 = evm.Address(addr1)
        assert addr1 == addr2

    def test_invalid_buffer_size(self) -> None:
        with pytest.raises(ValueError, match="Address must be 20 bytes"):
            evm.Address(b"\x00" * 19)
        with pytest.raises(ValueError, match="Address must be 20 bytes"):
            evm.Address(b"\x00" * 21)

    def test_invalid_type(self) -> None:
        with pytest.raises(TypeError):
            evm.Address(123)


class TestU256:
    @pytest.mark.parametrize("value", [
        0,
        1,
        2 ** 64 - 1,
        2 ** 128 + 7,
        2 ** 200 - 13,
        2 ** 256 - 1,
    ])
    def test_round_trip(self, value: int) -> None:
        wrapped: evm.U256 = evm.U256(value)
        assert int(wrapped) == value
        assert wrapped.to_int() == value

    def test_default_is_zero(self) -> None:
        assert int(evm.U256()) == 0

    def test_repr_contains_value(self) -> None:
        assert "42" in repr(evm.U256(42))


class TestBytes:
    def test_empty(self) -> None:
        b: evm.Bytes = evm.Bytes(b"")
        assert len(b) == 0
        assert bytes(b) == b""

    def test_from_none(self) -> None:
        b: evm.Bytes = evm.Bytes(None)
        assert len(b) == 0
        assert bytes(b) == b""

    def test_round_trip(self) -> None:
        payload: bytes = b"\x00\x01\xfe\xff" * 8
        b: evm.Bytes = evm.Bytes(payload)
        assert len(b) == len(payload)
        assert bytes(b) == payload

    def test_accepts_bytearray(self) -> None:
        b: evm.Bytes = evm.Bytes(bytearray(b"abc"))
        assert bytes(b) == b"abc"


class TestBlockInfo:
    def test_defaults(self) -> None:
        block: evm.BlockInfo = evm.BlockInfo()
        assert block.number == 0
        assert block.timestamp == 0
        assert block.gas_limit == 30_000_000
        assert block.chain_id == 1

    def test_fields_writable(self) -> None:
        block: evm.BlockInfo = evm.BlockInfo()
        block.number = 99
        block.base_fee = 7
        block.randao = 2 ** 200
        assert block.number == 99
        assert block.base_fee == 7
        assert int(block.randao) == 2 ** 200


class TestAccountInfo:
    def test_construct_with_balance(self) -> None:
        info: evm.AccountInfo = evm.AccountInfo(ADDRESS_HEX, balance=10 ** 18, next_nonce=3)
        assert int(info.balance) == 10 ** 18
        assert info.next_nonce == 3

    def test_balance_mutable(self) -> None:
        info: evm.AccountInfo = evm.AccountInfo(ADDRESS_HEX)
        info.balance = 5
        assert int(info.balance) == 5


class TestAccessListEntry:
    def test_construct_with_slots(self) -> None:
        # Just verifies the constructor accepts addresses and int slot keys
        # without raising.  The struct is opaque from Python beyond that.
        entry: evm.AccessListEntry = evm.AccessListEntry(ADDRESS_HEX, [0, 1, 2 ** 250])
        assert entry is not None
