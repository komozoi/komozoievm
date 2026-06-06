
#  Copyright 2025-2026 komozoi
#  Original Creation Date: 2026-5-31
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

"""Detailed example of tracing EVM execution with Komozoi EVM.

This script walks through the tracing surface of `komozoievm` end to end:

1. A successful call to an outer contract that performs a nested CALL into
   an inner contract.  The script prints the resulting trace via the
   built-in `CallTrace` aggregator.
2. A multi-level call chain (outer -> middle -> inner) where the
   innermost contract REVERTs and the middle contract propagates the
   failure back to the outer caller.  The trace surfaces every frame in
   the chain so the failure can be located precisely.
3. A custom `Tracer` subclass that captures both contract calls and event
   logs, demonstrating the lower-level callback interface.

The example is fully offline and uses only hand-written bytecode so the
mechanics of each opcode are visible.

Usage:

    python tracing_example.py
"""

from __future__ import annotations

import sys
from typing import List

import komozoievm as evm


# Three 20-byte addresses used throughout the example.  The choice is
# arbitrary; any well-formed hex string works.
CALLER: str = "0x000000000000000000000000000000000000aaaa"
OUTER: str = "0x000000000000000000000000000000000000b0b0"
MIDDLE: str = "0x000000000000000000000000000000000000111d"
INNER_OK: str = "0x000000000000000000000000000000000000c0de"
INNER_BAD: str = "0x000000000000000000000000000000000000dead"


def make_call_bytecode(inner: str) -> bytes:
	"""Return bytecode that performs a CALL into `inner` and STOPs.

	Stack layout for CALL (top of stack last):
	    gas, addr, value, argsOffset, argsSize, retOffset, retSize
	We push them in reverse order so they end up correct on the stack.
	"""
	addr: bytes = bytes.fromhex(inner[2:])
	return (
		b"\x60\x00"          # PUSH1 0x00  retSize
		b"\x60\x00"          # PUSH1 0x00  retOffset
		b"\x60\x00"          # PUSH1 0x00  argsSize
		b"\x60\x00"          # PUSH1 0x00  argsOffset
		b"\x60\x00"          # PUSH1 0x00  value
		b"\x73" + addr +     # PUSH20 inner
		b"\x63\xff\xff\xff\xff"  # PUSH4 gas
		b"\xf1"              # CALL
		b"\x00"              # STOP
	)


# Inner contract that emits a LOG1 and halts cleanly.
#   PUSH1 0x2a  PUSH1 0x00 MSTORE          ; memory[0..32] = 0x2a
#   PUSH1 0xaa PUSH1 0x20 PUSH1 0x00 LOG1  ; topic=0xaa, log mem[0..32]
#   STOP
INNER_OK_BYTECODE: bytes = (
	b"\x60\x2a\x60\x00\x52"
	b"\x60\xaa\x60\x20\x60\x00\xa1"
	b"\x00"
)

# Inner contract that reverts with a 32-byte payload (0x...beef).
#   PUSH32 0xbeef  PUSH1 0x00 MSTORE
#   PUSH1 0x20 PUSH1 0x00 REVERT
INNER_BAD_BYTECODE: bytes = (
	b"\x7f" + (0xBEEF).to_bytes(32, "big") +
	b"\x60\x00\x52"
	b"\x60\x20\x60\x00\xfd"
)


def make_propagating_call_bytecode(inner: str) -> bytes:
	"""Bytecode that CALLs `inner` and REVERTs if that call failed.

	This lets us build a multi-level call chain where a deep REVERT
	propagates back out to the top-level caller, instead of being
	swallowed by the parent frame (which is what `make_call_bytecode`
	does on purpose for demo #1).

	Per-byte offsets are annotated next to each opcode below.  The
	JUMPDEST that handles the failure path lives at 0x2a; the clean
	STOP path falls through at 0x29.
	"""
	addr: bytes = bytes.fromhex(inner[2:])
	return (
		b"\x60\x00"          # 0x00 PUSH1 0  retSize
		b"\x60\x00"          # 0x02 PUSH1 0  retOffset
		b"\x60\x00"          # 0x04 PUSH1 0  argsSize
		b"\x60\x00"          # 0x06 PUSH1 0  argsOffset
		b"\x60\x00"          # 0x08 PUSH1 0  value
		b"\x73" + addr +     # 0x0a PUSH20 inner
		b"\x63\xff\xff\xff\xff"  # 0x1f PUSH4 gas
		b"\xf1"              # 0x24 CALL  -> pushes success (0 or 1)
		b"\x15"              # 0x25 ISZERO
		b"\x60\x2a"          # 0x26 PUSH1 0x2a  (revert JUMPDEST offset)
		b"\x57"              # 0x28 JUMPI
		b"\x00"              # 0x29 STOP (clean path)
		b"\x5b"              # 0x2a JUMPDEST  (revert target)
		b"\x60\x00\x60\x00"  # 0x2b PUSH1 0  PUSH1 0
		b"\xfd"              # 0x2f REVERT
	)


class VerboseTracer(evm.Tracer):
	"""Custom tracer that prints calls and events as they happen.

	Subclass `evm.Tracer` directly when the flat list provided by
	`evm.CallTrace` is not enough — for example to stream events to a log
	file or to maintain a stack of nested calls.
	"""

	def __init__(self) -> None:
		super().__init__()
		self.calls: List[evm.CallTraceEntry] = []
		self.events: List[evm.EventTrace] = []

	def on_contract_call(self, chain: evm.StateProvider,
			call_trace: evm.CallTraceEntry) -> None:
		self.calls.append(call_trace)
		status: str = "ok" if call_trace.success else "FAIL"
		print("  [call] %s -> %s  gas=%d  %s"
			% (str(call_trace.src), str(call_trace.dst),
				call_trace.gas_used, status))

	def on_event_log(self, chain: evm.StateProvider,
			event_trace: evm.EventTrace) -> None:
		self.events.append(event_trace)
		topics: str = ",".join("0x%x" % t for t in event_trace.topics)
		print("  [log ] from=%s topics=[%s] data=%s"
			% (str(event_trace.src), topics, bytes(event_trace.data).hex()))


def print_call_trace(trace: evm.CallTrace) -> None:
	"""Print the entries collected by an `evm.CallTrace` aggregator."""
	if len(trace) == 0:
		print("  (no nested calls were recorded)")
		return
	for i, entry in enumerate(trace.calls):
		status: str = "ok" if entry.success else "FAIL"
		print("  %d. %s -> %s  gas=%d  %s"
			% (i, str(entry.src), str(entry.dst), entry.gas_used, status))
		if len(entry.calldata) > 0:
			print("     calldata = %s" % bytes(entry.calldata).hex())
		if len(entry.returndata) > 0:
			print("     returndata = %s" % bytes(entry.returndata).hex())


def build_chain() -> evm.MockChain:
	"""Create a fresh `MockChain` with the inner contracts installed."""
	chain: evm.MockChain = evm.MockChain()
	chain.set_balance(CALLER, 10 ** 20)
	chain.set_code(INNER_OK, INNER_OK_BYTECODE)
	chain.set_code(INNER_BAD, INNER_BAD_BYTECODE)
	return chain


def demo_successful_nested_call() -> None:
	print("=== 1. Successful nested call (CallTrace aggregator) ===")
	chain: evm.MockChain = build_chain()
	chain.set_code(OUTER, make_call_bytecode(INNER_OK))

	trace: evm.CallTrace = evm.CallTrace()
	result: evm.SimulationResult = chain.simulate(OUTER, CALLER, trace=trace)

	print("outer success=%s gas_used=%d reason=%r"
		% (result.success, result.gas_used, result.reason))
	print_call_trace(trace)


def demo_failing_nested_call() -> None:
	print("\n=== 2. Multi-level call chain ending in a REVERT ===")
	chain: evm.MockChain = build_chain()

	# Build the chain:  CALLER -> OUTER -> MIDDLE -> INNER_BAD
	# Both OUTER and MIDDLE use `make_propagating_call_bytecode` so the
	# deepest REVERT bubbles all the way back to the top-level caller.
	chain.set_code(MIDDLE, make_propagating_call_bytecode(INNER_BAD))
	chain.set_code(OUTER, make_propagating_call_bytecode(MIDDLE))

	trace: evm.CallTrace = evm.CallTrace()
	# `execute` would commit successful state to the chain; here we use
	# `simulate` because we only want to observe behavior.
	result: evm.SimulationResult = chain.simulate(OUTER, CALLER, trace=trace)

	# Because every frame propagates the inner failure, the top-level
	# result is also a revert.  The trace pinpoints which frames failed
	# and which one originated the failure (the deepest entry).
	print("outer success=%s gas_used=%d reason=%r"
		% (result.success, result.gas_used, result.reason))
	print("top-level return_data = %s" % bytes(result.return_data).hex())
	print_call_trace(trace)


def demo_custom_tracer() -> None:
	print("\n=== 3. Custom Tracer subclass (calls + event logs) ===")
	chain: evm.MockChain = build_chain()
	chain.set_code(OUTER, make_call_bytecode(INNER_OK))

	tracer: VerboseTracer = VerboseTracer()
	result: evm.SimulationResult = chain.simulate(OUTER, CALLER, trace=tracer)

	print("outer success=%s gas_used=%d" % (result.success, result.gas_used))
	print("captured %d call(s), %d event(s)"
		% (len(tracer.calls), len(tracer.events)))


def main() -> int:
	demo_successful_nested_call()
	demo_failing_nested_call()
	demo_custom_tracer()
	return 0


if __name__ == "__main__":
	sys.exit(main())
