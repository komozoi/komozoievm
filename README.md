# Komozoi EVM

Komozoi EVM is a high-performance Ethereum Virtual Machine simulator written in
C++17, with first-class Python bindings.  It is designed to let applications
build a mock blockchain (or attach to a real one), seed its state, and execute
transactions and smart-contract calls against it as if they had been mined.

Typical uses include:

- Building and back-testing MEV bots and trading strategies.
- Writing fast, deterministic integration tests for smart contracts.
- Researching chain behavior, gas accounting, and execution traces.
- Prototyping new chain logic on top of an Ethereum-compatible execution layer.

I originally built this for my MEV bot.  No, I didn't make any money.  The reason
I didn't use existing projects is that they integrate poorly with my codebases.

This is in the early stages, do not expect it to be perfect.  This was tested on
a lot of real transactions but is known to have limitations in some very few cases.
Please report issues if you find them.

## Features

- Full EVM execution with gas accounting and access-list support.
- Pluggable `StateProvider` / `ChainProvider` interface — back the simulator
  with an in-memory mock chain, your own database, or a live RPC endpoint.
- Ethereum transaction builder and signer (secp256k1).
- Tracing hooks for contract calls and event logs.
- Built on [libexcessive](https://gitea.com/komozoi/excessive) for allocators,
  containers, hashing, and IO.
- Python bindings will be published as binary wheels on PyPI.

## Repository layout

```
src/
  crypto/        secp256k1 and hashing primitives
  ethereum/      EVM, transactions, simulation context
  interface/     ChainProvider / StateProvider abstractions
  util/          Bytes, math, and supporting utilities
  python.cpp     pybind11 entry point for the Python module
python/          Python package sources and tests
docs/            Documentation (see docs/python/README.md for the Python API)
```

## Building the C++ library

Requirements: a C++17 compiler, CMake 3.14+, and GoogleTest for the test
target.  libexcessive is fetched automatically by CMake.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target komozoievm
```

To build and run the test suite:

```bash
cmake --build build --target evm_tests
./build/evm_tests
```

### CMake options

| Option                   | Default | Description                                            |
|--------------------------|---------|--------------------------------------------------------|
| `BUILD_CXX_TESTS`        | `ON`    | Build the `evm_tests` GoogleTest executable.           |
| `BUILD_PYTHON_BINDINGS`  | `OFF`   | Build the `_komozoievm` Python extension module.       |

## Python bindings

Install from PyPI:

```bash
pip install komozoievm
```

Or build from a source checkout:

```bash
pip install .
```

A minimal example:

```python
import komozoievm as evm

src = "0x00000000000000000000000000000000000000ab"
dst = "0x00000000000000000000000000000000000000cd"

chain = evm.MockChain()
chain.set_balance(src, 10 * 10**18)
chain.set_code(dst, b"\x00")  # STOP

block = evm.BlockInfo(number=1, timestamp=1_700_000_000, base_fee=10)
result = chain.simulate(dst, src, b"", 0, block)
print(result.success, result.return_data.hex())
```

The full Python API reference lives in [`docs/python/README.md`](docs/python/README.md).
Runnable examples — including a script that forks mainnet state from Infura
and simulates a transaction on top of it — are in
[`examples/python/`](examples/python/README.md).

## Contributing

Contributions are welcome.  Please read [`CONTRIBUTING.md`](CONTRIBUTING.md)
before opening a pull request, and report security issues privately as
described in [`SECURITY.md`](SECURITY.md).

## License

Komozoi EVM is distributed under the Apache License, Version 2.0.  See the
[`LICENSE`](LICENSE) file for details.
