"""Python bindings for the Komozoi EVM simulator.

The compiled extension module ``_komozoievm`` is exposed through this
package.  See ``docs/python/README.md`` for the full API reference.
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

from ._komozoievm import (  # noqa: F401
    __version__,
    Address,
    AccountInfo,
    AccessListEntry,
    BlockInfo,
    EVM,
    MockChain,
    SimulationResult,
    StateProvider,
    Transaction,
    TransactionBuilder,
    Tracer,
    CallTraceEntry,
    EventTrace,
    U256,
)


class CallTrace(Tracer):
    """Aggregating tracer that collects every contract call into a list.

    Pass an instance as ``trace=`` to :meth:`MockChain.simulate` or
    :meth:`MockChain.execute` to record the call hierarchy executed by the
    transaction.  The captured entries are available via the ``calls``
    attribute as a flat list, ordered by the EVM call order.
    """

    def __init__(self) -> None:
        super().__init__()
        self.calls: list = []

    def on_contract_call(self, chain, call_trace) -> None:  # noqa: D401
        self.calls.append(call_trace)

    def __iter__(self):
        return iter(self.calls)

    def __len__(self) -> int:
        return len(self.calls)

    def __repr__(self) -> str:
        return "CallTrace(calls=%d)" % len(self.calls)


__all__ = [
    "Address",
    "AccountInfo",
    "AccessListEntry",
    "BlockInfo",
    "EVM",
    "MockChain",
    "SimulationResult",
    "StateProvider",
    "Transaction",
    "TransactionBuilder",
    "Tracer",
    "CallTrace",
    "CallTraceEntry",
    "EventTrace",
    "U256",
    "__version__",
]
