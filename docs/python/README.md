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

src = "0x0000000000000000000000000000000000000001"
dst = "0x0000000000000000000000000000000000000002"

# An in-memory chain populated by Python code.  Addresses must be valid
# 20-byte hex strings; malformed values raise ``ValueError``.
chain = evm.MockChain()
chain.set_balance(src, 10 * 10**18)
chain.set_code(dst, bytes.fromhex("6001600101"))

# Simulate a call without mutating state.
block = evm.BlockInfo(number=1, timestamp=1_700_000_000, base_fee=10)
result = chain.simulate(dst, src, b"", 0, block)
print(result.success, result.return_data.hex())

# ``execute`` performs the same call but commits the resulting state.
result = chain.execute(dst, src, value=1_000_000_000_000_000_000)
```

## Module layout

`komozoievm` is a single top-level module.  Names listed below are the public
Python surface; everything not listed should be treated as internal and is
subject to change.

### Types

| Python type | Description                                                                                                                                                                |
|-------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `Address`   | 20-byte Ethereum address.  Accepts `bytes`, `bytearray`, hex `str` (with or without `0x`), `None`, or another `Address`.  `str(address)` returns the EIP-55 checksum form. |
| `U256`      | 256-bit unsigned integer.  Implicitly converts from Python `int`; values out of range raise `OverflowError`.                                                               |

`Address` and `U256` are thin wrappers; functions that accept them also accept
the plain Python equivalents (`bytes`, `str`, `int`).

### Data classes

- `BlockInfo(number, timestamp, base_fee=0, gas_limit=30_000_000,
  chain_id=1, coinbase=..., randao=0)` — fields exposed for the block context.
- `AccountInfo(address, balance=0, next_nonce=0)` — minimal account state.
- `AccessListEntry(address, storage_keys=())`.
- `SimulationResult` with attributes: `success: bool`, `return_data: bytes`,
  `reason: str`, `gas_used: int`.
- `CallTrace` — an aggregating tracer that records every contract call
  performed during a transaction.  Pass an instance as ``trace=`` to
  ``simulate``/``execute``; the captured entries are available on the
  ``calls`` attribute.
- `CallTraceEntry` with attributes: `src: Address`, `dst: Address`,
  `calldata: bytes`, `returndata: bytes`, `gas_used: int`, `success: bool`.
- `EventTrace` with attributes: `src: Address`, `topics: list[int]`, `data: bytes`.

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

For common contract calls, you can use the `Transaction.call` helper:

```python
tx = evm.Transaction.call(
    from_=address,
    to=address,
    value=0,
    data=b"",
    gas_limit=100_000
)
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

The high-level entry points on any `StateProvider` (including `MockChain`)
hide transaction construction behind keyword arguments:

```python
result = chain.simulate(dst, src, data=b"", value=0, block=None,
    gas=3_000_000, trace=None)
result = chain.execute(dst, src, data=b"", value=0, block=None,
    gas=3_000_000, trace=None)
gas = evm.EVM.initial_gas_cost(calldata)
```

All arguments after `src` are optional.  `block` defaults to a sensible
context; `value` and `gas` accept either `int` or `float` (e.g. `1e18`
for one ETH, `1.5e6` for 1.5M gas) — floats are rounded to the nearest
integer for `gas` and truncated toward zero for `value`.  `data` defaults
to empty calldata, and `gas` defaults to 3,000,000.

If `dst` has no contract code installed it is treated as an Externally
Owned Account: the call succeeds immediately after any value transfer,
without executing any bytecode.  Installing code via `set_code` turns
the address into a contract from that point on.

`simulate` does not mutate the underlying provider.  `execute` writes the
resulting state back through the provider's `update_*` callbacks.

For lower-level control you can build a `Transaction` (or
`TransactionBuilder`) explicitly and run it through the `EVM` class:

```python
machine = evm.EVM(chain)
tx = evm.Transaction.call(from_=src, to=dst, data=b"", gas_limit=100_000)
result = machine.simulate(tx, block_info, tracer=None)
```

### Tracing

The simplest way to capture every contract call made by a transaction is
`evm.CallTrace`:

```python
trace = evm.CallTrace()
chain.execute(dst, src, trace=trace)
for entry in trace.calls:
    print(entry.src, "->", entry.dst, entry.success)
```

For custom logic, subclass `evm.Tracer` and override the callbacks:

```python
class MyTracer(evm.Tracer):
    def on_contract_call(self, chain, call_trace):
        print(f"Call from {call_trace.src} to {call_trace.dst}")

    def on_event_log(self, chain, event_trace):
        print(f"Log from {event_trace.src} with {len(event_trace.topics)} topics")

chain.execute(dst, src, trace=MyTracer())
```

## Threading and memory

The bindings release the GIL during long-running EVM execution so that Python
threads can make progress in parallel.  Callbacks into Python (state provider
methods, tracers) reacquire the GIL automatically.

## Versioning

Wheels are tagged with the upstream Komozoi EVM version followed by a build
metadata suffix when applicable.  Breaking changes to the Python API are only
introduced on a major version bump.
