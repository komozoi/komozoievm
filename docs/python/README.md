# Komozoi EVM — Python Bindings

`komozoievm` is a Python package that exposes the Komozoi EVM C++ engine for
simulating Ethereum transactions and contract execution against either a
user-supplied mock chain state or a real chain provider.

The package is built from the same C++ sources as the native library and is
distributed as binary wheels on PyPI.

## Installation

```bash
pip install komozoievm
```

Building from a source checkout requires a C++17 compiler and CMake:

```bash
pip install .
```

## Quick start

```python
import komozoievm as evm

# An in-memory chain populated by Python code.
chain = evm.MockChain()
chain.set_balance("0xabcd...01", 10 * 10**18)
chain.set_code("0xabcd...02", bytes.fromhex("6001600101"))

# Build a transaction and run it through the EVM.
tx = evm.TransactionBuilder(
    to="0xabcd...02",
    nonce=0,
    gas_limit=100_000,
).build()

block = evm.BlockInfo(number=1, timestamp=1_700_000_000, base_fee=10)
result = evm.EVM(chain).simulate(tx, block)

print(result.success, result.return_data.hex())
```

## Module layout

`komozoievm` is a single top-level module.  Names listed below are the public
Python surface; everything not listed should be treated as internal and is
subject to change.

### Types

| Python type | Description                                                                                                                                     |
|-------------|-------------------------------------------------------------------------------------------------------------------------------------------------|
| `Address`   | 20-byte Ethereum address.  Accepts `bytes`, `bytearray`, or hex `str` (with or without `0x`).  `str(address)` returns the EIP-55 checksum form. |
| `U256`      | 256-bit unsigned integer.  Implicitly converts from Python `int`; values out of range raise `OverflowError`.                                    |
| `Bytes`     | Immutable byte buffer.  Interoperable with Python `bytes`/`bytearray`.                                                                          |

`Address` and `U256` are thin wrappers; functions that accept them also accept
the plain Python equivalents (`bytes`, `str`, `int`).

### Data classes

- `BlockInfo(number, timestamp, base_fee=0, gas_limit=30_000_000,
  chain_id=1, coinbase=..., randao=0)` — fields exposed for the block context.
- `AccountInfo(address, balance=0, next_nonce=0)` — minimal account state.
- `AccessListEntry(address, storage_keys=())`.
- `SimulationResult` with attributes: `success: bool`, `return_data: bytes`,
  `reason: str | None`, `gas_used: int`.

### Transaction construction

```python
tx = evm.TransactionBuilder(
    to=address,
    nonce=int,
    gas_limit=int,
    value=0,
    gas_price=0,
    max_fee_per_gas=0,
    max_priority_fee_per_gas=0,
    data=b"",
    access_list=[],
).build()
```

For signed transactions, pass a 32-byte private key:

```python
raw = tx.sign_and_encode(private_key=bytes.fromhex(...))
```

### Chain providers

`StateProvider` is an abstract base class that Python code can subclass to
serve as the chain backend.  Override:

- `get_account_info(self, address: Address, block_number: int = 0) -> AccountInfo`
- `get_contract_code(self, address: Address) -> bytes`
- `get_storage_slots(self, address: Address, slot_keys: list[int],
  block_number: int = 0) -> list[int]`
- `update_account(self, address: Address, info: AccountInfo) -> bool`
- `save_contract_code(self, address: Address, code: bytes) -> bool`
- `update_storage_slots(self, address: Address, entries: dict[int, int]) -> bool`

`MockChain` is a built-in `StateProvider` implementation backed by in-memory
hash tables.  It is the recommended starting point for test suites and is what
the examples in this document use.

### Running the EVM

```python
machine = evm.EVM(chain)
result = machine.simulate(tx, block_info)
result = machine.execute(tx, block_info)   # like simulate, but commits state
gas = evm.EVM.initial_gas_cost(calldata)
```

`simulate` does not mutate the underlying provider.  `execute` writes the
resulting state back through the provider's `update_*` callbacks.

### Tracing

A trace callback can be registered to inspect each opcode:

```python
def on_step(ctx):
    print(ctx.pc, ctx.opcode, ctx.gas_remaining)

machine.simulate(tx, block_info, tracer=on_step)
```

## Threading and memory

The bindings release the GIL during long-running EVM execution so that Python
threads can make progress in parallel.  Callbacks into Python (state provider
methods, tracers) reacquire the GIL automatically.

## Versioning

Wheels are tagged with the upstream Komozoi EVM version followed by a build
metadata suffix when applicable.  Breaking changes to the Python API are only
introduced on a major version bump.
