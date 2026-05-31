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
import pytest

SENDER = "0x1111111111111111111111111111111111111111"
RECIPIENT = "0x2222222222222222222222222222222222222222"

class MyTracer(evm.Tracer):
    def __init__(self):
        super().__init__()
        self.calls = []
        self.logs = []

    def on_contract_call(self, chain, call_trace):
        self.calls.append({
            "src": str(call_trace.src),
            "dst": str(call_trace.dst),
            "calldata": call_trace.calldata,
            "success": call_trace.success
        })

    def on_event_log(self, chain, event_trace):
        self.logs.append({
            "src": str(event_trace.src),
            "topics": [int(t) for t in event_trace.topics],
            "data": event_trace.data
        })

def test_execute_with_revert(mock_chain, default_block):
    # REVERT(0, 0)
    revert_bytecode = b"\x60\x00\x60\x00\xfd"
    mock_chain.set_code(RECIPIENT, revert_bytecode)
    
    engine = evm.EVM(mock_chain)
    tx = evm.Transaction.call(from_=SENDER, to=RECIPIENT)
    
    result = engine.execute(tx, default_block)
    
    assert result.success is False
    assert result.reason == "Reverted"
    assert result.return_data == b""

def test_execute_with_revert_reason(mock_chain, default_block):
    # PUSH1 0x42 PUSH1 0x00 MSTORE PUSH1 0x20 PUSH1 0x00 REVERT
    # Reverts with 32 bytes of 0x42
    revert_reason_bytecode = b"\x60\x42\x60\x00\x52\x60\x20\x60\x00\xfd"
    mock_chain.set_code(RECIPIENT, revert_reason_bytecode)
    
    engine = evm.EVM(mock_chain)
    tx = evm.Transaction.call(from_=SENDER, to=RECIPIENT)
    
    result = engine.execute(tx, default_block)
    
    assert result.success is False
    assert result.reason == "Reverted"
    assert result.return_data == b"\x00" * 31 + b"\x42"

def test_execute_with_tracer(mock_chain, default_block):
    # PUSH1 0x01 PUSH1 0x00 MSTORE 
    # LOG1: topic=0, size=32, offset=0
    # PUSH1 0x00 (topic) PUSH1 0x20 (size) PUSH1 0x00 (offset) LOG1
    log_bytecode = b"\x60\x01\x60\x00\x52\x60\x00\x60\x20\x60\x00\xa1"
    mock_chain.set_code(RECIPIENT, log_bytecode)
    
    tracer = MyTracer()
    engine = evm.EVM(mock_chain)
    tx = evm.Transaction.call(from_=SENDER, to=RECIPIENT)
    
    result = engine.execute(tx, default_block, tracer=tracer)
    
    assert result.success is True, f"Execution failed: {result.reason}"
    # In current implementation, top-level call might not be traced via on_contract_call 
    # if it's handled specially. Let's see if on_event_log works.
    assert len(tracer.logs) == 1
    assert tracer.logs[0]["src"].lower() == RECIPIENT.lower()
    assert tracer.logs[0]["topics"] == [0]
    assert tracer.logs[0]["data"] == b"\x00" * 31 + b"\x01"

def test_execute_nested_call_tracing(mock_chain, default_block):
    inner_addr = "0x3333333333333333333333333333333333333333"
    # STOP
    mock_chain.set_code(inner_addr, b"\x00")
    
    # PUSH1 0x00 (ret_size) PUSH1 0x00 (ret_offset) PUSH1 0x00 (args_size) PUSH1 0x00 (args_offset)
    # PUSH1 0x00 (value) PUSH20 inner_addr PUSH4 0xffffffff (gas) CALL
    call_bytecode = b"\x60\x00\x60\x00\x60\x00\x60\x00\x60\x00\x73" + bytes.fromhex(inner_addr[2:]) + b"\x63\xff\xff\xff\xff\xf1"
    mock_chain.set_code(RECIPIENT, call_bytecode)
    
    tracer = MyTracer()
    engine = evm.EVM(mock_chain)
    tx = evm.Transaction.call(from_=SENDER, to=RECIPIENT)
    
    result = engine.execute(tx, default_block, tracer=tracer)
    
    assert result.success is True, f"Execution failed: {result.reason}"
    # Should see the nested call
    assert len(tracer.calls) >= 1
    assert any(c["dst"].lower() == inner_addr.lower() for c in tracer.calls)

def test_execute_at_end(mock_chain, default_block):
    # LOG1 at the end of code
    # PUSH1 0x00 (topic) PUSH1 0x20 (size) PUSH1 0x00 (offset) LOG1
    log_bytecode = b"\x60\x01\x60\x00\x52\x60\x00\x60\x20\x60\x00\xa1"
    mock_chain.set_code(RECIPIENT, log_bytecode)
    
    engine = evm.EVM(mock_chain)
    tx = evm.Transaction.call(from_=SENDER, to=RECIPIENT)
    
    result = engine.execute(tx, default_block)
    # This currently fails with "Invalid jump"
    assert result.success is True, f"Execution failed: {result.reason}"
