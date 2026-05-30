"""Fork mainnet state from Infura and simulate a transaction locally.

This example connects to an Infura (or any JSON-RPC compatible) Ethereum
endpoint, lazily pulls the account, code and storage data required by a
simulation, and runs the transaction through the Komozoi EVM as if it were
executed on top of the live chain at a chosen block.

Usage:

    export INFURA_API_KEY=<your-project-id>
    python infura_fork_simulation.py

The only external runtime dependency is `requests`.
"""

from __future__ import annotations

import os
import sys
from typing import Dict, List, Optional

import requests

import komozoievm as evm


# ---------------------------------------------------------------------------
# JSON-RPC client
# ---------------------------------------------------------------------------

class JsonRpcClient:
	"""Minimal JSON-RPC 2.0 client over HTTPS."""

	def __init__(self, url: str, timeout: float = 30.0) -> None:
		self.url = url
		self.timeout = timeout
		self._id = 0
		self._session = requests.Session()

	def call(self, method: str, params: list) -> object:
		self._id += 1
		payload = {
			"jsonrpc": "2.0",
			"id": self._id,
			"method": method,
			"params": params,
		}
		response = self._session.post(self.url, json=payload, timeout=self.timeout)
		response.raise_for_status()
		body = response.json()
		if "error" in body:
			raise RuntimeError("RPC error: " + str(body["error"]))
		return body["result"]


# ---------------------------------------------------------------------------
# StateProvider implementation backed by JSON-RPC
# ---------------------------------------------------------------------------

def _hex_to_int(value: str) -> int:
	return int(value, 16) if value else 0


def _block_tag(block_number: Optional[int]) -> str:
	if block_number is None or block_number == 0:
		return "latest"
	return hex(block_number)


class InfuraStateProvider(evm.StateProvider):
	"""StateProvider that lazily pulls chain state from a JSON-RPC endpoint.

	Results are cached in memory so repeated reads during a single simulation
	do not re-hit the network.  Writes performed by the EVM (account updates,
	code, storage) stay local to this instance and never propagate upstream.
	"""

	def __init__(self, rpc: JsonRpcClient, pinned_block: Optional[int] = None) -> None:
		super().__init__()
		self._rpc = rpc
		self._pinned_block = pinned_block
		self._accounts: Dict[str, evm.AccountInfo] = {}
		self._code: Dict[str, bytes] = {}
		self._storage: Dict[str, Dict[int, int]] = {}

	# StateProvider read API ------------------------------------------------

	def get_account_info(self, address, block_number: int = 0) -> evm.AccountInfo:
		key: str = str(evm.Address(address))
		cached: Optional[evm.AccountInfo] = self._accounts.get(key)
		if cached is not None:
			return cached

		tag: str = _block_tag(self._pinned_block if self._pinned_block else block_number)
		balance: int = _hex_to_int(self._rpc.call("eth_getBalance", [key, tag]))
		nonce: int = _hex_to_int(self._rpc.call("eth_getTransactionCount", [key, tag]))

		info: evm.AccountInfo = evm.AccountInfo(address=key, balance=balance, next_nonce=nonce)
		self._accounts[key] = info
		return info

	def get_contract_code(self, address) -> bytes:
		key: str = str(evm.Address(address))
		cached: Optional[bytes] = self._code.get(key)
		if cached is not None:
			return cached

		tag: str = _block_tag(self._pinned_block)
		hex_code: str = self._rpc.call("eth_getCode", [key, tag])
		raw: bytes = bytes.fromhex(hex_code[2:]) if hex_code.startswith("0x") else b""
		self._code[key] = raw
		return raw

	def get_storage_slots(self, address, slot_keys: List[int], block_number: int = 0) -> List[int]:
		key: str = str(evm.Address(address))
		account_storage: Dict[int, int] = self._storage.setdefault(key, {})
		tag: str = _block_tag(self._pinned_block if self._pinned_block else block_number)

		values: List[int] = []
		for slot in slot_keys:
			slot_int: int = int(slot)
			if slot_int in account_storage:
				values.append(account_storage[slot_int])
				continue
			padded: str = "0x" + format(slot_int, "064x")
			raw: str = self._rpc.call("eth_getStorageAt", [key, padded, tag])
			value: int = _hex_to_int(raw)
			account_storage[slot_int] = value
			values.append(value)
		return values

	# StateProvider write API (local-only) ----------------------------------

	def update_account(self, address, info: evm.AccountInfo) -> bool:
		self._accounts[str(evm.Address(address))] = info
		return True

	def save_contract_code(self, address, code) -> bool:
		self._code[str(evm.Address(address))] = bytes(code)
		return True

	def update_storage_slots(self, address, entries) -> bool:
		account_storage: Dict[int, int] = self._storage.setdefault(str(evm.Address(address)), {})
		for slot, value in entries.items():
			account_storage[int(slot)] = int(value)
		return True


# ---------------------------------------------------------------------------
# Block context helpers
# ---------------------------------------------------------------------------

def fetch_block_info(rpc: JsonRpcClient, block_number: Optional[int]) -> evm.BlockInfo:
	"""Pull the header fields needed to build a `BlockInfo` for simulation."""

	tag: str = _block_tag(block_number)
	header: dict = rpc.call("eth_getBlockByNumber", [tag, False])
	return evm.BlockInfo(
		number=_hex_to_int(header["number"]),
		timestamp=_hex_to_int(header["timestamp"]),
		base_fee=_hex_to_int(header.get("baseFeePerGas", "0x0")),
		gas_limit=_hex_to_int(header["gasLimit"]),
		chain_id=1,
		coinbase=header.get("miner", "0x" + "00" * 20),
	)


# ---------------------------------------------------------------------------
# Demo entry point
# ---------------------------------------------------------------------------

def main() -> int:
	api_key: Optional[str] = os.environ.get("INFURA_API_KEY")
	if not api_key:
		print("Set INFURA_API_KEY in the environment.", file=sys.stderr)
		return 1

	url: str = "https://mainnet.infura.io/v3/" + api_key
	rpc: JsonRpcClient = JsonRpcClient(url)

	# Pin to a recent block so the simulation is reproducible.
	pinned: int = _hex_to_int(rpc.call("eth_blockNumber", []))
	provider: InfuraStateProvider = InfuraStateProvider(rpc, pinned_block=pinned)
	block: evm.BlockInfo = fetch_block_info(rpc, pinned)

	# Send 1 wei from a funded whale to a fresh recipient.  The whale's
	# balance and nonce come straight from Infura on first access.
	whale: str = "0x00000000219ab540356cBB839Cbe05303d7705Fa"  # Beacon deposit contract
	recipient: str = "0x000000000000000000000000000000000000dEaD"

	whale_info: evm.AccountInfo = provider.get_account_info(whale)

	builder: evm.TransactionBuilder = evm.TransactionBuilder(
		to=recipient,
		nonce=whale_info.next_nonce,
		gas_limit=21000,
		value=1,
		max_fee_per_gas=block.base_fee * 2 + 1,
		max_priority_fee_per_gas=1,
	)
	# The bindings currently accept a built transaction object only after the
	# native builder finalizes it; for a value transfer the calldata is empty
	# and the builder can be passed straight through.  Adapt as needed when
	# additional transaction helpers land.

	engine: evm.EVM = evm.EVM(provider)
	result: evm.SimulationResult = engine.simulate(builder, block)

	print("Forked block:", block.number)
	print("Whale balance:", whale_info.balance)
	print("Success:", result.success)
	print("Gas used:", result.gas_used)
	print("Reason:", result.reason)
	print("Return data:", result.return_data.hex())
	return 0 if result.success else 2


if __name__ == "__main__":
	sys.exit(main())
