// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-07-14
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//  
//       http://www.apache.org/licenses/LICENSE-2.0
//  
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
//

#include "EVM.h"

#include <utility>
#include "EVMSimulationContext.h"
#include "util/keccak.h"
#include "precompiledContracts.h"


#define EXEC_OUTCOME_SUCCESS {true, nullptr, 0, nullptr}
#define EXEC_OUTCOME_RETURN(X, Y) {true, X, Y, nullptr}
#define EXEC_OUTCOME_OUT_OF_GAS {false, nullptr, 0, "Out of gas"}
#define EXEC_OUTCOME_INVALID_OPCODE {false, nullptr, 0, "Invalid opcode"}
#define EXEC_OUTCOME_NOT_IMPLEMENTED(X) {false, nullptr, 0, "Not implemented: " X}
#define EXEC_OUTCOME_REVERTED(X, Y) {false, X, Y, "Reverted"}
#define EXEC_OUTCOME_INVALID_JUMP {false, nullptr, 0, "Invalid jump"}
#define EXEC_OUTCOME_STATIC_STATE {false, nullptr, 0, "State change in STATICCALL"}
#define EXEC_OUTCOME_INSUFFICIENT_ETH {false, nullptr, 0, "Insufficient eth for transfer"}
#define EXEC_OUTCOME_STACK_UNDERFLOW {false, nullptr, 0, "Stack Underflow"};

//#define DEBUG_INSTRUCTIONS
//#define DEBUG_TRACE
//#define DEBUG_STORAGE

// This allows transactions to execute even though our
// code incorrectly computes gas usage
// Without this, transactions often run out of gas,
// even though they shouldn't.
#define EXTRA_GAS 0x2d0000



typedef struct {
	bool success;
	uint8_t* returnDataPtr;
	uint16_t returnDataSize;
	const char* message;
} tick_outcome_t;


#ifdef DEBUG_INSTRUCTIONS

static inline void dbgPrintInstruction(uint32_t pc, int depth, const char* name, const uint256_t& a, const uint256_t& b) {
	char b1[96];
	char b2[96];

	a.toStr(b1);
	b.toStr(b2);
	printf("%04x %02i %s %s, %s\n", pc, depth, name, b1, b2);
}

static inline void dbgPrintInstruction(uint32_t pc, int depth, const char* name, const uint256_t& a) {
	char b1[96];

	a.toStr(b1);
	printf("%04x %02i %s %s\n", pc, depth, name, b1);
}

static inline void dbgPrintInstruction(uint32_t pc, int depth, const char* name) {
	printf("%04x %02i %s\n", pc, depth, name);
}


#define DEBUG_INS2(NAME, A, B) dbgPrintInstruction(context.pc - 1, context.stack.size(), NAME, A, B)
#define DEBUG_INS1(NAME, A) dbgPrintInstruction(context.pc - 1, context.stack.size(), NAME, A)
#define DEBUG_INS0(NAME) dbgPrintInstruction(context.pc - 1, context.stack.size(), NAME)

#else

#define DEBUG_INS2(NAME, A, B) (void)0
#define DEBUG_INS1(NAME, A) (void)0
#define DEBUG_INS0(NAME) (void)0

#endif


#ifdef DEBUG_TRACE

static inline void dbgPrintStorage(uint32_t totalGasUsed, const char* label, const EthereumAddress& address, const uint256_t& where, const uint256_t& what) {
	char b1[96];
	char b2[96];
	char b3[96];

	where.toStr(b1);
	what.toStr(b2);
	address.toStr(b3, true);
	printf("% 6i %s %s[%s] = %s\n", totalGasUsed, label, b3, b1, b2);
}

static inline void dbgPrintCall(uint32_t totalGasUsed, const char* type, const EthereumAddress& callee) {
	char b1[96];

	callee.toStr(b1);
	printf("% 6i %s %s\n\n", totalGasUsed, type, b1);
}

static inline void dbgPrintCallOutcome(uint32_t totalGasUsed, uint32_t gasUsed, const EthereumAddress& callee, tick_outcome_t outcome, const uint8_t* args, int argSize) {
	char buffer[argSize*2 + outcome.returnDataSize*2 + 8];
	char b1[96];

	callee.toStr(b1);
	printf("\nReturn from call, made by %s\n", b1);
	printf("  Gas used by this call: %u\n", gasUsed);
	printf("  Total gas used: %u\n", totalGasUsed);

	toHex(args, argSize, buffer);
	printf("  calldata: %s\n", buffer);
	toHex(outcome.returnDataPtr, outcome.returnDataSize, buffer);
	printf("  returned: %s\n", buffer);
	printf("  %s : %s\n", outcome.success ? "success" : "failure", outcome.message);
}

#define DEBUG_STORAGE(LABEL,WHERE,WHAT) dbgPrintStorage(context.totalGasUsed(), LABEL, context.address, WHERE, WHAT)
#define DEBUG_CALL(TYPE, CALLEE) dbgPrintCall(context.totalGasUsed(), TYPE, CALLEE)
#define DEBUG_CALL_OUTCOME(CALLEE, OUTCOME, CALLDATA, CALLDATASIZE) dbgPrintCallOutcome(subcontext.totalGasUsed(), subcontext.gasUsed, CALLEE, OUTCOME, CALLDATA, CALLDATASIZE)
#define DEBUG_CALL_OUTCOME_RAW(TOTAL_GAS, GAS_USED, CALLEE, OUTCOME, CALLDATA, CALLDATASIZE) dbgPrintCallOutcome(TOTAL_GAS, GAS_USED, CALLEE, OUTCOME, CALLDATA, CALLDATASIZE)


#elif defined(DEBUG_STORAGE)

static inline void dbgPrintStorage(uint32_t totalGasUsed, const char* label, const EthereumAddress& address, const uint256_t& where, const uint256_t& what) {
	char b1[96];
	char b2[96];
	char b3[96];

	where.toStr(b1);
	what.toStr(b2);
	address.toStr(b3, true);
	printf("% 6i %s %s[%s] = %s\n", totalGasUsed, label, b3, b1, b2);
}

#undef DEBUG_STORAGE
#define DEBUG_STORAGE(LABEL,WHERE,WHAT) dbgPrintStorage(context.totalGasUsed(), LABEL, context.address, WHERE, WHAT)
#define DEBUG_CALL(TYPE, CALLEE) (void)0
#define DEBUG_CALL_OUTCOME(CALLEE, OUTCOME, CALLDATA, CALLDATASIZE) (void)0
#define DEBUG_CALL_OUTCOME_RAW(TOTAL_GAS, GAS_USED, CALLEE, OUTCOME, CALLDATA, CALLDATASIZE) (void)0

#else

#define DEBUG_STORAGE(LABEL,WHERE,WHAT) (void)0
#define DEBUG_CALL(TYPE, CALLEE) (void)0
#define DEBUG_CALL_OUTCOME(CALLEE, OUTCOME, CALLDATA, CALLDATASIZE) (void)0
#define DEBUG_CALL_OUTCOME_RAW(TOTAL_GAS, GAS_USED, CALLEE, OUTCOME, CALLDATA, CALLDATASIZE) (void)0

#endif


typedef struct {
	uint8_t* calldata;
	int calldataSize;
	uint8_t* returndata;
	int returndataSize;
	uint256_t value;
	uint256_t gasLimit;
	bool isStatic, isDelegateCall;
} min_call_info_t;


template<class T>
static inline tick_outcome_t handleCopyInstruction(T& src, EVMSimulationContext& context) {
	uint256_t offDst256 = context.pop();
	uint256_t offSrc256 = context.pop();
	uint256_t size256 = context.pop();

	// Under special conditions, we handle the specific expansion for memory-memory copies.
	// This is necessary so we can correctly compute memory expansion gas costs.
	if (std::is_same<T, ArrayList<uint8_t>>::value && (void*)&src == &context.memory && offSrc256 > offDst256)
		if (!context.expandMemory(offSrc256, size256))
			return EXEC_OUTCOME_OUT_OF_GAS;

	uint8_t* memory = context.getMemoryStartingAt(offDst256, size256);
	if (!memory)
		return EXEC_OUTCOME_OUT_OF_GAS;

	int size = size256;
	if (size == 0)
		return EXEC_OUTCOME_SUCCESS;

	int srcOff = offSrc256;

	context.gasUsed += 3 * ((size + 31) / 32);

	// Compute how much we actually copy over, and zero
	// any extra data
	int cutoff = 0;
	if (srcOff < (int)src.size())
		cutoff = (int)src.size() - srcOff < size ? (int)src.size() - srcOff : size;

	if (cutoff > 0)
		memcpy(memory, &src.get(srcOff), cutoff);

	if (cutoff < size)
		bzero(&memory[cutoff], size - cutoff);

	return EXEC_OUTCOME_SUCCESS;
}


static tick_outcome_t runEVMRaw(EVMSimulationContext& context, bool payAccounts = true);


static tick_outcome_t callRaw(EVMSimulationContext& context, Bytes runcode, min_call_info_t callInfo, const EthereumAddress& src, const EthereumAddress& dst) {
	if (context.remainingGas() <= 0)
		return EXEC_OUTCOME_OUT_OF_GAS;

	Bytes calldata(callInfo.calldata, callInfo.calldataSize);

	sim_tx_info_t txInfo{context.txInfo};
	txInfo.gas.gasLimit = callInfo.gasLimit.constrain(0, (context.remainingGas() * 63) / 64);

	// Run EVM call
	EVMSimulationContext subcontext(context, txInfo, std::move(runcode), std::move(calldata), src, dst, callInfo.value, context.isStatic || callInfo.isStatic);
	tick_outcome_t callOutcome = runEVMRaw(subcontext, !callInfo.isDelegateCall);

	context.push(callOutcome.success ? 1 : 0);
	if (callOutcome.returnDataPtr && callOutcome.returnDataSize > 0) {
		if (callInfo.returndataSize) {
			if (callOutcome.returnDataSize > callInfo.returndataSize) {
				memcpy(callInfo.returndata, callOutcome.returnDataPtr, callInfo.returndataSize);
			} else {
				memcpy(callInfo.returndata, callOutcome.returnDataPtr, callOutcome.returnDataSize);
			}
		}
	}

	DEBUG_CALL_OUTCOME(context.address, callOutcome, callInfo.calldata, callInfo.calldataSize);

	if (callOutcome.success && !callInfo.isStatic)
		subcontext.writeChangesTo(context);

	context.returnData = Bytes(callOutcome.returnDataPtr, callOutcome.returnDataSize);

	if (context.tracer != nullptr)
		context.tracer->onContractCall(context, {src, dst, subcontext.calldata, context.returnData, subcontext.gasUsed, callOutcome.success});

	context.gasUsed += subcontext.gasUsed;

	return EXEC_OUTCOME_SUCCESS;
}


static inline tick_outcome_t generateCall(EVMSimulationContext& context, bool localStorage, bool sameTxData) {
	uint256_t gasLimit256 = context.pop();
	EthereumAddress target = context.pop();
	uint256_t value = sameTxData ? context.value : context.pop();
	uint256_t argsOffset256 = context.pop();
	uint256_t argsSize256 = context.pop();
	uint256_t retOffset256 = context.pop();
	uint256_t retSize256 = context.pop();

	// Memory expansion can move pointers around, so we need
	// to process the expansion before getting our pointers
	if (!context.expandMemory(argsOffset256, argsSize256))
		return EXEC_OUTCOME_OUT_OF_GAS;
	if (!context.expandMemory(retOffset256, retSize256))
		return EXEC_OUTCOME_OUT_OF_GAS;

	uint8_t* argsMem = &context.memory.getMemory()[(int)argsOffset256];
	uint8_t* retMem = &context.memory.getMemory()[(int)retOffset256];

	DEBUG_CALL(sameTxData ? "DELEGATECALL" : "CALL", target);

	tick_outcome_t outcome;
	if (uint256_t(target) < 16) {
		int gasTmp = context.remainingGas();
		// TODO: Handle return data properly here
		const char* message = callPrecompiled(target.data.rawBytes[0], argsMem, argsSize256, retMem, retSize256, gasTmp);
		// context.returnData = Bytes(callOutcome.returnDataPtr, callOutcome.returnDataSize);
		if (message)
			printf("Error calling precompiled contract %#x: %s\n", (int)target.data.rawBytes[0], message);

		context.push(message ? 0 : 1);
		context.gasUsed += gasTmp;
		DEBUG_CALL_OUTCOME_RAW(context.totalGasUsed(), gasTmp, target, (tick_outcome_t{message == nullptr, retMem, retSize256, message}), argsMem, argsSize256);
		outcome = {true, nullptr, 0, message};
	} else {

		const EthereumAddress& src = sameTxData ? context.caller : context.address;
		const EthereumAddress& dst = sameTxData ? context.address : target;
		Bytes runcode = context.getContractCode(target);

		min_call_info_t callInfo{argsMem, argsSize256, retMem, retSize256, value, gasLimit256, false, sameTxData};

		outcome = callRaw(context, runcode, callInfo, src, dst);
	}

	return outcome;
}


static tick_outcome_t generateStaticCall(EVMSimulationContext& context) {
	uint256_t gasLimit256 = context.pop();
	EthereumAddress target = context.pop();
	uint256_t argsOffset256 = context.pop();
	uint256_t argsSize256 = context.pop();
	uint256_t retOffset256 = context.pop();
	uint256_t retSize256 = context.pop();

	// Memory expansion can move pointers around, so we need
	// to process the expansion before getting our pointers
	if (!context.expandMemory(argsOffset256, argsSize256))
		return EXEC_OUTCOME_OUT_OF_GAS;
	if (!context.expandMemory(retOffset256, retSize256))
		return EXEC_OUTCOME_OUT_OF_GAS;

	uint8_t* argsMem = &context.memory.getMemory()[(int)argsOffset256];
	uint8_t* retMem = &context.memory.getMemory()[(int)retOffset256];

	DEBUG_CALL("STATICCALL", target);

	tick_outcome_t outcome;
	if (uint256_t(target) < 16) {
		int gasTmp = context.remainingGas();
		const char* message = callPrecompiled(target.data.rawBytes[0], argsMem, argsSize256, retMem, retSize256, gasTmp);
		if (message)
			printf("Error calling precompiled contract %#x: %s\n", (int)target.data.rawBytes[0], message);

		context.push(message ? 0 : 1);
		context.gasUsed += gasTmp;
		DEBUG_CALL_OUTCOME_RAW(context.totalGasUsed(), gasTmp, target, (tick_outcome_t{message == nullptr, retMem, retSize256, message}), argsMem, argsSize256);
		outcome = {true, nullptr, 0, message};
	} else {

		min_call_info_t callInfo{argsMem, argsSize256, retMem, retSize256, 0, gasLimit256, true, false};
		Bytes runcode = context.getContractCode(target);

		outcome = callRaw(context, runcode, callInfo, context.address, target);
	}

	return outcome;
}


static tick_outcome_t tickEVMRaw(Bytes code, EVMSimulationContext& context) {
	if ((uint32_t)code.size() <= context.pc)
		return EXEC_OUTCOME_RETURN(context.memory.getMemory(), 0);
	uint8_t opcode = code.get(context.pc++);

	uint256_t a, b, N;
	int count, size;
	uint8_t* memory;
	EthereumAddress address;

	switch (opcode) {
		case 0x00:
			return EXEC_OUTCOME_RETURN(context.memory.getMemory(), 0);
		case 0x01:
			a = context.pop();
			b = context.pop();
			context.push(a + b);
			DEBUG_INS2("add", a, b);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x02:
			a = context.pop();
			b = context.pop();
			context.push(a * b);
			DEBUG_INS2("mul", a, b);
			context.gasUsed += 5;
			return EXEC_OUTCOME_SUCCESS;
		case 0x03:
			a = context.pop();
			b = context.pop();
			context.push(a - b);
			DEBUG_INS2("sub", a, b);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x04:
			a = context.pop();
			b = context.pop();
			context.push(a / b);
			DEBUG_INS2("div", a, b);
			context.gasUsed += 5;
			return EXEC_OUTCOME_SUCCESS;
		case 0x05:
			a = context.pop();
			b = context.pop();
			context.push(a.sdiv(b));
			DEBUG_INS2("sdiv", a, b);
			context.gasUsed += 5;
			return EXEC_OUTCOME_SUCCESS;
		case 0x06:
			a = context.pop();
			b = context.pop();
			context.push(a % b);
			DEBUG_INS2("mod", a, b);
			context.gasUsed += 5;
			return EXEC_OUTCOME_SUCCESS;
		case 0x07:
			a = context.pop();
			b = context.pop();
			context.push(a.smod(b));
			DEBUG_INS2("smod", a, b);
			context.gasUsed += 5;
			return EXEC_OUTCOME_SUCCESS;
		case 0x08:
			a = context.pop();
			b = context.pop();
			N = context.pop();
			{
				UnsignedFixedWidthBigInt<5> bigA = a;
				bigA = bigA + UnsignedFixedWidthBigInt<5>(b);
				context.push(uint256_t(bigA % N));
			}
			DEBUG_INS2("addmod", a, b);
			context.gasUsed += 8;
			return EXEC_OUTCOME_SUCCESS;
		case 0x09:
			a = context.pop();
			b = context.pop();
			N = context.pop();
			{
				UnsignedFixedWidthBigInt<8> bigA = a;
				bigA = bigA * UnsignedFixedWidthBigInt<8>(b);
				context.push(uint256_t(bigA % N));
			}
			DEBUG_INS2("mulmod", a, b);
			context.gasUsed += 8;
			return EXEC_OUTCOME_SUCCESS;
		case 0x0A:
			a = 1;
			b = context.pop();
			N = context.pop();
			DEBUG_INS2("exp", b, N);
			count = 0;
			for (; count < 32 && N.data.rawBytes[count]; count++);

			if (!a.isZero()) {
				while (N.getBit(0) == 0 && !N.isZero()) {
					b = b * b;
					N = N >> 1;
				}
				while (!N.isZero()) {
					a = a * b;
					N = N - uint256_t(1);
				}
			}
			context.push(a);
			DEBUG_INS1("*exp", a);
			context.gasUsed += 10 + count * 50;
			return EXEC_OUTCOME_SUCCESS;
		case 0x0B:
			// Sign extension
			a = context.pop();
			b = context.pop();
			DEBUG_INS2("signextend", a, b);
			if (a < 32) {
				count = (int)a.data.rawBytes[0];
				const uint8_t idx = 8 * count + 7;
				const uint8_t sign = b.getBit(idx);
				const uint256_t mask = uint256_t(-1) >> (256 - idx);
				b = (uint256_t(-sign) << idx) | (b & mask);
			}
			context.push(b);
			DEBUG_INS1("*signextend", b);
			context.gasUsed += 5;
			return EXEC_OUTCOME_SUCCESS;
		case 0x10:
			a = context.pop();
			b = context.pop();
			context.push((a < b) ? 1 : 0);
			DEBUG_INS2("lt", a, b);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x11:
			a = context.pop();
			b = context.pop();
			context.push((a > b) ? 1 : 0);
			DEBUG_INS2("gt", a, b);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x12:
			a = context.pop();
			b = context.pop();
			context.push(a.signedLessThan(b) ? 1 : 0);
			DEBUG_INS2("slt", a, b);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x13:
			a = context.pop();
			b = context.pop();
			context.push(b.signedLessThan(a) ? 1 : 0);
			DEBUG_INS2("sgt", a, b);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x14:
			a = context.pop();
			b = context.pop();
			context.push(a == b ? 1 : 0);
			DEBUG_INS2("eq", a, b);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x15:
			a = context.pop();
			context.push(a.isZero() ? 1 : 0);
			DEBUG_INS1("iszero", a);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x16:
			a = context.pop();
			b = context.pop();
			context.push(a & b);
			DEBUG_INS2("and", a, b);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x17:
			a = context.pop();
			b = context.pop();
			context.push(a | b);
			DEBUG_INS2("or", a, b);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x18:
			a = context.pop();
			b = context.pop();
			context.push(a ^ b);
			DEBUG_INS2("xor", a, b);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x19:
			a = context.pop();
			context.push(~a);
			DEBUG_INS1("neg", a);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x1A:
			b = context.pop();
			a = context.pop();
			if (b >= 32)
				context.push(0);
			else
				// Internally we use little endian encoding, but
				// EVM uses big endian, so we must reverse the byte order.
				context.push(a.data.rawBytes[31 - b.data.rawBytes[0]]);
			DEBUG_INS2("byte", b, a);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x1B:
			b = context.pop();
			a = context.pop();
			if (b >= 256)
				context.push(0);
			else
				context.push(a << b.data.rawBytes[0]);
			DEBUG_INS2("shl", b, a);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x1C:
			b = context.pop();
			a = context.pop();
			if (b >= 256)
				context.push(0);
			else
				context.push(a >> b.data.rawBytes[0]);
			DEBUG_INS2("shr", b, a);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x1D:
			b = context.pop();
			a = context.pop();
			if (b >= 256)
				context.push(-(a.data.rawBytes[31] >> 7));
			else
				context.push(a.sar(b.data.rawBytes[0]));
			DEBUG_INS2("sar", b, a);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x20:
			b = context.pop();
			count = (int)context.pop();

			// Get memory region and check we didn't run out of gas
			// This protects our software from memory allocation issues and segfaults.
			memory = context.getMemoryStartingAt(b, count);
			if (memory == nullptr)
				return EXEC_OUTCOME_OUT_OF_GAS;

			context.push(uint256_t(keccak256(memory, count), true));
			DEBUG_INS2("sha3", b, count);

			// context.getMemoryStartingAt already handles the memory expansion cost for us
			context.gasUsed += 30;
			return EXEC_OUTCOME_SUCCESS;

		case 0x30:
			context.push(uint256_t(context.address));
			DEBUG_INS0("address");
			context.gasUsed += 2;
			return EXEC_OUTCOME_SUCCESS;
		case 0x31:
			address = (a = context.pop());
			context.warmAccount(address);
			context.push(b = context.getAccountInfo(address, 0).balance);
			DEBUG_INS2("balance", a, b);
			context.gasUsed += 100;
			return EXEC_OUTCOME_SUCCESS;
		case 0x32:
			context.push(uint256_t(context.origin));
			DEBUG_INS0("origin");
			context.gasUsed += 2;
			return EXEC_OUTCOME_SUCCESS;
		case 0x33:
			context.push(uint256_t(context.caller));
			DEBUG_INS0("caller");
			context.gasUsed += 2;
			return EXEC_OUTCOME_SUCCESS;
		case 0x34:
			context.push(context.value);
			DEBUG_INS0("value");
			context.gasUsed += 2;
			return EXEC_OUTCOME_SUCCESS;
		case 0x35:
			b = context.pop();
			if (b >= context.calldata.size())
				context.push(0);
			else
				context.push(context.calldataLoad(b));
			DEBUG_INS1("calldataload", b);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x36:
			context.push(context.calldataSize());
			DEBUG_INS0("calldatasize");
			context.gasUsed += 2;
			return EXEC_OUTCOME_SUCCESS;
		case 0x37:
			DEBUG_INS0("calldatacopy*");
			context.gasUsed += 3;
			return handleCopyInstruction(context.calldata, context);
		case 0x38:
			context.push(code.size());
			DEBUG_INS0("codesize");
			context.gasUsed += 2;
			return EXEC_OUTCOME_SUCCESS;
		case 0x39:
			DEBUG_INS0("codecopy*");
			context.gasUsed += 3;
			return handleCopyInstruction(code, context);
		case 0x3A:
			context.push(context.txInfo.gas.gasPrice);
			DEBUG_INS0("gasprice");
			context.gasUsed += 2;
			return EXEC_OUTCOME_SUCCESS;
		case 0x3B:
			address = (a = context.pop());
			context.push(context.getContractCode(address).size());
			DEBUG_INS1("extcodesize", a);
			context.gasUsed += 100;
			return EXEC_OUTCOME_SUCCESS;
		case 0x3C:
			address = (a = context.pop());
			DEBUG_INS1("extcodecopy*", a);
			context.gasUsed += 100;
			{
				Bytes codeTmp = context.getContractCode(address);
				return handleCopyInstruction(codeTmp, context);
			}
		case 0x3D:
			context.gasUsed += 2;
			context.push(context.returnData.size());
			DEBUG_INS0("returndatasize");
			return EXEC_OUTCOME_SUCCESS;
		case 0x3E:
			DEBUG_INS0("returndatacopy*");
			context.gasUsed += 3;
			return handleCopyInstruction(context.returnData, context);
		case 0x3F:
			DEBUG_INS0("extcodehash");
			return EXEC_OUTCOME_NOT_IMPLEMENTED("EXTCODEHASH (0x3F)");
		case 0x40:
			DEBUG_INS0("blockhash");
			return EXEC_OUTCOME_NOT_IMPLEMENTED("BLOCKHASH (0x40)");
		case 0x41:
			context.push(uint256_t(context.txInfo.block.coinbase));
			DEBUG_INS0("coinbase");
			context.gasUsed += 2;
			return EXEC_OUTCOME_SUCCESS;
		case 0x42:
			context.push(context.txInfo.block.timestamp);
			DEBUG_INS0("timestamp");
			context.gasUsed += 2;
			return EXEC_OUTCOME_SUCCESS;
		case 0x43:
			context.push(context.txInfo.block.number);
			DEBUG_INS0("number");
			context.gasUsed += 2;
			return EXEC_OUTCOME_SUCCESS;
		case 0x44:
			context.push(context.txInfo.block.randao);
			DEBUG_INS0("prevrandao");
			context.gasUsed += 2;
			return EXEC_OUTCOME_SUCCESS;
		case 0x45:
			context.push(context.txInfo.gas.gasLimit);
			DEBUG_INS0("gaslimit");
			context.gasUsed += 2;
			return EXEC_OUTCOME_SUCCESS;
		case 0x46:
			context.push(context.txInfo.block.chainId);
			DEBUG_INS0("chainid");
			context.gasUsed += 2;
			return EXEC_OUTCOME_SUCCESS;
		case 0x47:
			context.push(context.getAccountInfo(context.address, 0).balance);
			DEBUG_INS0("balance");
			context.gasUsed += 5;
			return EXEC_OUTCOME_SUCCESS;
		case 0x48:
			context.push(context.txInfo.block.baseFee);
			DEBUG_INS0("basefee");
			context.gasUsed += 2;
			return EXEC_OUTCOME_SUCCESS;
		case 0x49:
			DEBUG_INS0("blobhash");
			return EXEC_OUTCOME_NOT_IMPLEMENTED("BLOBHASH (0x49)");
		case 0x4A:
			DEBUG_INS0("blobbasefee");
			return EXEC_OUTCOME_NOT_IMPLEMENTED("BLOBBASEFEE (0x4A)");
		case 0x50:
			a = context.pop();
			DEBUG_INS1("pop", a);
			context.gasUsed += 2;
			return EXEC_OUTCOME_SUCCESS;
		case 0x51:
			a = context.pop();
			context.gasUsed += 3;
			memory = context.getMemoryStartingAt(a, 32);
			if (!memory)
				return EXEC_OUTCOME_OUT_OF_GAS;
			context.push(b = uint256_t(memory, true));
			DEBUG_INS2("mload", a, b);
			return EXEC_OUTCOME_SUCCESS;
		case 0x52:
			a = context.pop();
			b = context.pop();
			context.gasUsed += 3;
			memory = context.getMemoryStartingAt(a, 32);
			if (!memory)
				return EXEC_OUTCOME_OUT_OF_GAS;
			b.toBytes(memory, true);
			DEBUG_INS2("mstore", a, b);
			return EXEC_OUTCOME_SUCCESS;
		case 0x53:
			a = context.pop();
			b = context.pop();
			DEBUG_INS2("mstore8", a, b);
			context.gasUsed += 3;
			memory = context.getMemoryStartingAt(a, 1);
			if (!memory)
				return EXEC_OUTCOME_OUT_OF_GAS;
			*memory = b.data.rawBytes[0];
			return EXEC_OUTCOME_SUCCESS;
		case 0x54:
			a = context.pop();
			b = context.sload(a);
			context.gasUsed += 100;
			context.push(b);
			DEBUG_INS2("sload", a, b);
			DEBUG_STORAGE("SLOAD", a, b);
			return EXEC_OUTCOME_SUCCESS;
		case 0x55:
			a = context.pop();
			b = context.pop();
			context.gasUsed += 100;
			DEBUG_INS2("sstore", a, b);
			DEBUG_STORAGE("SSTORE", a, b);
			if (!context.sstore(a, b))
				return EXEC_OUTCOME_STATIC_STATE;
			return EXEC_OUTCOME_SUCCESS;
		case 0x56:
			a = context.pop();
			DEBUG_INS1("jump", a);
			context.gasUsed += 8;
			if (a >= code.size())
				return EXEC_OUTCOME_INVALID_JUMP;
			context.pc = a;
			if (code.get(context.pc) != 0x5b)
				return EXEC_OUTCOME_INVALID_JUMP;
			return EXEC_OUTCOME_SUCCESS;
		case 0x57:
			a = context.pop();
			b = context.pop();
			DEBUG_INS2("jumpi", a, b);
			context.gasUsed += 10;
			if (b.isZero())
				return EXEC_OUTCOME_SUCCESS;
			if (a >= code.size())
				return EXEC_OUTCOME_INVALID_JUMP;
			context.pc = a;
			if (code.get(context.pc) != 0x5b)
				return EXEC_OUTCOME_INVALID_JUMP;
			return EXEC_OUTCOME_SUCCESS;
		case 0x58:
			context.push(context.pc - 1);
			DEBUG_INS0("pc");
			context.gasUsed += 2;
			return EXEC_OUTCOME_SUCCESS;
		case 0x59:
			context.push(context.memory.size());
			DEBUG_INS0("msize");
			context.gasUsed += 2;
			return EXEC_OUTCOME_SUCCESS;
		case 0x5A:
			context.gasUsed += 2;
			context.push(context.remainingGas());
			DEBUG_INS0("gas");
			return EXEC_OUTCOME_SUCCESS;
		case 0x5B:
			DEBUG_INS0("jumpdest");
			context.gasUsed++;
			return EXEC_OUTCOME_SUCCESS;
		case 0x5C:
			a = context.pop();
			context.gasUsed += 100;
			context.push(b = context.tload(a));
			DEBUG_INS2("tload", a, b);
			DEBUG_STORAGE("TLOAD", a, b);
			return EXEC_OUTCOME_SUCCESS;
		case 0x5D:
			a = context.pop();
			b = context.pop();
			DEBUG_INS2("tstore", a, b);
			DEBUG_STORAGE("TSTORE", a, b);
			context.gasUsed += 100;
			if (!context.tstore(a, b))
				return EXEC_OUTCOME_STATIC_STATE;
			return EXEC_OUTCOME_SUCCESS;
		case 0x5E:
			DEBUG_INS0("mcopy*");
			context.gasUsed += 3;
			return handleCopyInstruction(context.memory, context);
		case 0x5F:
		case 0x60:
		case 0x61:
		case 0x62:
		case 0x63:
		case 0x64:
		case 0x65:
		case 0x66:
		case 0x67:
		case 0x68:
		case 0x69:
		case 0x6A:
		case 0x6B:
		case 0x6C:
		case 0x6D:
		case 0x6E:
		case 0x6F:
		case 0x70:
		case 0x71:
		case 0x72:
		case 0x73:
		case 0x74:
		case 0x75:
		case 0x76:
		case 0x77:
		case 0x78:
		case 0x79:
		case 0x7A:
		case 0x7B:
		case 0x7C:
		case 0x7D:
		case 0x7E:
		case 0x7F:
			count = opcode - 0x5F;
			context.push(a = uint256_t(&code.get(context.pc), count, true));
			DEBUG_INS1("push", a);
			context.pc += count;
			context.gasUsed += count == 0 ? 2 : 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87:
		case 0x88:
		case 0x89:
		case 0x8A:
		case 0x8B:
		case 0x8C:
		case 0x8D:
		case 0x8E:
		case 0x8F:
			count = opcode - 0x7F;
			if (context.stack.size() < count)
				return EXEC_OUTCOME_STACK_UNDERFLOW;
			context.push(a = context.stack.get(context.stack.size() - count));
			DEBUG_INS1("dup", a);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0x90:
		case 0x91:
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0x96:
		case 0x97:
		case 0x98:
		case 0x99:
		case 0x9A:
		case 0x9B:
		case 0x9C:
		case 0x9D:
		case 0x9E:
		case 0x9F:
			count = context.stack.size() - opcode + 0x8F - 1;
			a = context.stack.get(count);
			context.stack.set(count, b = context.stack.get(context.stack.size() - 1));
			context.stack.set(context.stack.size() - 1, a);
			DEBUG_INS2("swap", a, b);
			context.gasUsed += 3;
			return EXEC_OUTCOME_SUCCESS;
		case 0xA0:
		case 0xA1:
		case 0xA2:
		case 0xA3:
		case 0xA4:
			count = opcode - 0xA0;
			a = context.pop();
			b = context.pop();
			memory = context.getMemoryStartingAt(a, b);
			if (!memory)
				return EXEC_OUTCOME_OUT_OF_GAS;
			size = b;

			{
				event_trace_t trace{context.address, {}, Bytes(), (uint8_t) count};
				for (int i = 0; i < count; i++)
					trace.topics[i] = context.pop();

				if (context.tracer != nullptr) {
					trace.data = Bytes(memory, size);
					context.tracer->onEventLog(context, trace);
				}
			}

			DEBUG_INS0("logx");

			context.gasUsed += 375 + 375 * count + 8 * size;
			return EXEC_OUTCOME_SUCCESS;
		case 0xF0:
			DEBUG_INS0("create");
			return EXEC_OUTCOME_NOT_IMPLEMENTED("CREATE (0xF0)");
		case 0xF1:
			DEBUG_INS0("call*");
			context.gasUsed += 100;
			return generateCall(context, false, false);
		case 0xF2:
			DEBUG_INS0("callcode*");
			return generateCall(context, true, false);
		case 0xF3:
			a = context.pop();
			b = context.pop();
			DEBUG_INS2("return", a, b);
			return EXEC_OUTCOME_RETURN(context.getMemoryStartingAt(a, b), (uint16_t)b);
		case 0xF4:
			DEBUG_INS0("delegatecall*");
			context.gasUsed += 100;
			return generateCall(context, true, true);
		case 0xF5:
			DEBUG_INS0("create2");
			return EXEC_OUTCOME_NOT_IMPLEMENTED("CREATE2 (0xF5)");
		case 0xFA:
			DEBUG_INS0("staticcall*");
			context.gasUsed += 100;
			return generateStaticCall(context);
		case 0xFD:
			a = context.pop();
			b = context.pop();
			DEBUG_INS2("revert", a, b);
			if ((a > 14000) || (b > 14000)) {
				a.toBytes(context.revertBuffer, true);
				b.toBytes(&context.revertBuffer[32], true);
				return EXEC_OUTCOME_REVERTED(context.revertBuffer, 64);
			}
			return EXEC_OUTCOME_REVERTED(context.getMemoryStartingAt(a, b), (uint16_t)b);
		case 0xFE:
			DEBUG_INS0("assert false");
			context.gasUsed = context.txInfo.gas.gasLimit;
			return EXEC_OUTCOME_REVERTED(context.memory.getMemory(), 0);
		case 0xFF:
			DEBUG_INS0("selfdestruct");
			return EXEC_OUTCOME_NOT_IMPLEMENTED("SELFDESTRUCT (0xFF)");
		default:
			DEBUG_INS0("[invalid]");
			return EXEC_OUTCOME_INVALID_OPCODE;
	}
}


static tick_outcome_t runEVMRaw(EVMSimulationContext& context, bool payAccounts) {
	tick_outcome_t outcome;

	if (payAccounts) {
		// Process transfers first
		if (!context.value.isZero()) {
			if (!context.payAccount(context.address, context.value))
				return EXEC_OUTCOME_STATIC_STATE;
			if (!context.deductAccount(context.caller, context.value))
				return EXEC_OUTCOME_INSUFFICIENT_ETH;
		}
	}

	if (!context.runcode) {
		// No code at the destination: treat as an EOA call.  Value (if any)
		// has already been transferred above; nothing else to do.
		return EXEC_OUTCOME_SUCCESS;
	} else if (context.runcode.size() > 22 && context.runcode.get(0) == 0xef) {
		context.runcode = context.getContractCode(EthereumAddress(&context.runcode.get(3), true));
	}

	// Only execute if there is contract code
	if (context.runcode.size() > 0) {
		while (true) {
			outcome = tickEVMRaw(context.runcode, context);
			if (outcome.returnDataPtr || !outcome.success)
				return outcome;
			if (context.gasUsed >= context.txInfo.gas.gasLimit)
				return EXEC_OUTCOME_OUT_OF_GAS;
		}
	}

	return EXEC_OUTCOME_SUCCESS;
}


uint32_t EVM::getInitialGasCost(Bytes calldata) {
	uint32_t total = 21000;

	for (size_t i = 0; i < calldata.size(); i++)
		total += calldata.get(i) ? 16 : 4;

	return total;
}


uint32_t EVM::getInitialGasCost(const ArrayList<uint8_t>& calldata) {
	uint32_t total = 21000;

	for (int i = 0; i < calldata.size(); i++)
		total += calldata.get(i) ? 16 : 4;

	return total;
}


EVMSimulationOutput EVM::simulate(EVMSimulationContext& context) {
	context.gasUsed = getInitialGasCost(context.calldata);

	tick_outcome_t output = runEVMRaw(context);

	return EVMSimulationOutput(output.success, Bytes(output.returnDataPtr, output.returnDataSize), output.message);
}


EVMSimulationOutput EVM::simulate(const EthereumTransaction& transaction, const block_info_t& blockInfo, EVMTracer* tracer) {
	ArrayList<call_trace_t> callTraces(32);

	sim_tx_info_t txInfo{transaction.gasInfo(), blockInfo, blockInfo.number - 1};
	txInfo.gas.gasLimit += EXTRA_GAS;

	EVMSimulationContext context(chain, txInfo, transaction);
	context.gasUsed = getInitialGasCost(transaction.calldata());
	context.tracer = tracer;

	tick_outcome_t output = runEVMRaw(context, true);

	return EVMSimulationOutput(output.success, Bytes(output.returnDataPtr, output.returnDataSize), output.message);
}


evm_execution_outcome_t EVM::execute(const EthereumTransaction& transaction, const block_info_t& blockInfo, EVMTracer* tracer) {
	sim_tx_info_t txInfo{transaction.gasInfo(), blockInfo, blockInfo.number - 1};
	txInfo.gas.gasLimit += EXTRA_GAS;

	EVMSimulationContext context(chain, txInfo, transaction);
	context.gasUsed = getInitialGasCost(transaction.calldata());
	context.tracer = tracer;

	tick_outcome_t output = runEVMRaw(context, true);

	if (output.success)
		context.writeChangesTo(chain);

	return {context.gasUsed, output.success, output.message, Bytes(output.returnDataPtr, output.returnDataSize)};
}
