# Python examples

## `contract_call_simulation.py`

Offline smoke test that runs a real contract through the simulator without
ever touching the network.  It loads the deployed WETH9 bytecode that is
committed to the repo at `bytecode/weth9.hex`, installs it into a
`MockChain` at WETH9's mainnet address, seeds storage slot 0 with the
Solidity short-string layout for `"Wrapped Ether"`, then calls the `name()`
view function and asserts that the ABI-decoded return value matches.
Any deviation raises and the script exits non-zero, so this doubles as an
end-to-end sanity check for an installed `komozoievm` wheel.

### Running

```bash
python contract_call_simulation.py
```

No environment variables, no dependencies beyond `komozoievm` itself.

### How the bytecode was captured

The hex blob in `bytecode/weth9.hex` is the deployed runtime bytecode of
WETH9 (`0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2`), fetched once via
`eth_getCode` against a public JSON-RPC node.  See
`scripts/fetch_bytecode.py` in the `coconut-voter` project for a similar
scraper-style approach against block explorers.

## `tracing_example.py`

End-to-end walk-through of the tracing API.  Runs three scenarios against
an in-memory `MockChain`:

1. A successful nested CALL, captured with the built-in `evm.CallTrace`
   aggregator.
2. The same outer contract pointed at an inner contract that REVERTs,
   showing how to read a failing entry from the trace.
3. A custom `evm.Tracer` subclass that streams both contract calls and
   event logs as they happen.

### Running

```bash
python tracing_example.py
```

No environment variables, no dependencies beyond `komozoievm` itself.

## `infura_fork_simulation.py`

Forks live Ethereum mainnet state from an Infura (or any JSON-RPC) endpoint and
simulates a transaction locally with Komozoi EVM.  The example pins the
simulation to a specific block, lazily pulls only the account, code and
storage data that the EVM touches, and caches everything in memory so each
piece of state is fetched at most once.

### Running

```bash
pip install -r requirements.txt
export INFURA_API_KEY=<your-project-id>
python infura_fork_simulation.py
```

Any JSON-RPC endpoint will work — point `INFURA_API_KEY` at a self-hosted
node by editing the `url` line in `main()`.

### How it works

- `JsonRpcClient` is a tiny `requests`-based JSON-RPC 2.0 client.
- `InfuraStateProvider` extends `komozoievm.StateProvider` and implements the
  read API (`get_account_info`, `get_contract_code`, `get_storage_slots`)
  against the JSON-RPC methods `eth_getBalance`, `eth_getTransactionCount`,
  `eth_getCode` and `eth_getStorageAt`.
- The provider also implements the write API (`update_account`,
  `save_contract_code`, `update_storage_slots`) as local cache updates; the
  EVM's writes never propagate back to the remote node, so simulations are
  side-effect free.
- `fetch_block_info` builds a `BlockInfo` from `eth_getBlockByNumber` so the
  simulation sees the same timestamp, base fee and gas limit as the forked
  block.

### Extending

- Swap the demo transaction in `main()` for your own — for example a contract
  call by passing `data=` to `provider.simulate(...)`.
- Pre-seed state by calling `provider.update_account(...)` or
  `provider.update_storage_slots(...)` before invoking `provider.simulate(...)`
  to model "what if" scenarios on top of real chain data.
- Pin `pinned_block` to a historical block number to reproduce a past
  transaction in a deterministic environment.
