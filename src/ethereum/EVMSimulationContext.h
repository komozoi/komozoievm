// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-07-16
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

#ifndef CMEVBOT_EVMSIMULATIONCONTEXT_H
#define CMEVBOT_EVMSIMULATIONCONTEXT_H

#include <utility>
#include "util/Bytes.h"

#include "EthereumTransaction.h"
#include "interface/ChainProvider.h"
#include "bigint.h"
#include "ds/HashMap.h"
#include "ds/HashSet.h"
#include "EVMTracer.h"


typedef struct {
	tx_gas_info_t gas;
	block_info_t block;
	uint64_t stateBlockNumber;
} sim_tx_info_t;


static inline HashSet<EthereumAddress> getWarmAccounts(const sim_tx_info_t& txInfo, const EthereumAddress& src, const EthereumAddress& dst) {
	HashSet<EthereumAddress> out(16);
	out.add(src);
	out.add(dst);
	out.add(txInfo.block.coinbase);
	return out;
}



class EVMSimulationContext: public StateProvider {
public:

	EVMSimulationContext(StateProvider& chain, sim_tx_info_t& txInfo, const EthereumTransaction& tx)
		: chain(chain), returnContext(nullptr), origin(tx.sender()), caller(tx.sender()), address(tx.recipient()),
		  value(tx.value()), calldata(tx.calldata()), txInfo(txInfo),
		  accounts(16), runcodes(16), storage(16), transient(1), warmAccounts(getWarmAccounts(txInfo, caller, address)),
		  runcode(getContractCode(tx.recipient())) {

	}

	EVMSimulationContext(StateProvider& chain, sim_tx_info_t& txInfo, Bytes calldata, const EthereumAddress& caller, const EthereumAddress& address, uint256_t value, bool isStatic = false)
		: chain(chain), returnContext(nullptr), origin(caller), caller(caller), address(address),
		  value(std::move(value)), calldata(std::move(calldata)), txInfo(txInfo),
		  accounts(16), runcodes(16), storage(16), transient(1), warmAccounts(getWarmAccounts(txInfo, caller, address)),
		  runcode(getContractCode(this->address)), isStatic(isStatic) {
	}

	EVMSimulationContext(EVMSimulationContext& chain, sim_tx_info_t& txInfo, Bytes runcode, Bytes calldata, EthereumAddress caller, EthereumAddress address, uint256_t value, bool isStatic = false)
		: chain(chain), returnContext(&chain), origin(caller), caller(std::move(caller)), address(std::move(address)),
		  value(std::move(value)), calldata(std::move(calldata)), tracer(chain.tracer), txInfo(txInfo),
		  accounts(16), runcodes(16), storage(16), transient(1), warmAccounts(chain.warmAccounts),
		  runcode(std::move(runcode)), isStatic(isStatic) {

		warmAccounts.add(this->address);
		warmAccounts.add(this->caller);
	}

	void writeChangesTo(StateProvider& dst) const;
	void writeChangesTo(EVMSimulationContext& dst) const;

	// State management conveniences
	inline uint256_t pop() {
		return stack.pop();
	}

	inline void push(uint256_t&& value) {
		stack.add(value);
	}

	inline void push(const uint256_t& value) {
		stack.add(value);
	}

	uint256_t calldataLoad(int offset) const {
		int size = calldataSize();
		if (offset >= size - 31) {
			if (offset >= size)
				return 0;
			uint8_t buffer[32];
			memcpy(buffer, &calldata[offset], size - offset);
			bzero(&buffer[size - offset], 32 - size + offset);
			return uint256_t(buffer, true);
		}
		return uint256_t(&calldata[offset], true);
	}

	int calldataSize() const {
		return (int)calldata.size();
	}

	void warmAccount(const EthereumAddress& address) {
		if (!warmAccounts.add(address))
			gasUsed += 2500;
	}

	inline uint32_t remainingGas() const {
		return txInfo.gas.gasLimit - gasUsed;
	}

	uint32_t totalGasUsed() const;

	inline bool payAccount(const EthereumAddress& address, const uint256_t& amount) {
		if (isStatic)
			return false;

		EthereumAccountInfo* ptr = accounts.getPtr(address);
		if (!ptr)
			ptr = &accounts.put(address, chain.getAccountInfo(address, txInfo.stateBlockNumber));

		ptr->balance += amount;
		return true;
	}

	inline bool deductAccount(const EthereumAddress& address, const uint256_t& amount) {
		if (isStatic)
			return false;

		EthereumAccountInfo* ptr = accounts.getPtr(address);
		if (!ptr)
			ptr = &accounts.put(address, chain.getAccountInfo(address, txInfo.stateBlockNumber));

		if (ptr->balance < amount)
			return false;

		ptr->balance -= amount;
		return true;
	}

	// Storage and transient state access
	uint256_t tload(const LongKey<256>& key);
	bool tstore(const LongKey<256>& key, const uint256_t& value);
	uint256_t sload(const LongKey<256>& key);
	bool sstore(const LongKey<256>& key, const uint256_t& value);
	void setStorageOverride(const EthereumAddress& contract, const LongKey<256>& key, const uint256_t& value);

	uint8_t* getMemoryStartingAt(const uint256_t& offset, const uint256_t& size);

	// Account info access
	EthereumAccountInfo getAccountInfo(const EthereumAddress& address, uint64_t blockNumber) override;
	Bytes getContractCode(const EthereumAddress& address) override;
	ArrayList<uint256_t> getStorageSlots(const EthereumAddress& address, const ArrayList<LongKey<256>> &slotKeys, uint64_t blockNumber) override;

	// Updates to internal/tracked state
	bool updateAccount(const EthereumAddress& key, const EthereumAccountInfo& account) override;
	bool saveContractCode(const EthereumAddress& key, Bytes runcode) override;
	bool updateStorageSlots(const EthereumAddress& key, const HashMap<LongKey<256>, uint256_t>& entries) override;

	bool updateTransientSlots(const EthereumAddress& key, const HashMap<LongKey<256>, uint256_t>& entries);

	// Code context
	StateProvider& chain;
	EVMSimulationContext* returnContext;
	const EthereumAddress origin;
	const EthereumAddress caller;
	const EthereumAddress address;
	const uint256_t value;
	Bytes calldata;
	Bytes returnData;

	EVMTracer* tracer = nullptr;

	// Block and transaction context data
	sim_tx_info_t& txInfo;

	// Overrides during this call
	HashMap<EthereumAddress, EthereumAccountInfo> accounts;
	HashMap<EthereumAddress, Bytes> runcodes;
	HashMap<EthereumAddress, HashMap<LongKey<256>, uint256_t>*> storage;
	HashMap<EthereumAddress, HashMap<LongKey<256>, uint256_t>*> transient;
	HashSet<EthereumAddress> warmAccounts;

	Bytes runcode;

	// Execution state
	uint32_t pc = 0;
	ArrayList<uint256_t> stack;
	ArrayList<uint8_t> memory;
	uint32_t gasUsed = 0;

	const bool isStatic = false;

	uint8_t revertBuffer[64];

	bool expandMemory(uint256_t offset256, uint256_t size256);

	~EVMSimulationContext();

private:

	EVMSimulationContext(EVMSimulationContext& parent, sim_tx_info_t& txInfo, Bytes calldata, EthereumAddress address, uint256_t value)
		: chain(parent.chain), returnContext(&parent), origin(parent.origin), caller(parent.address), address(std::move(address)),
		  value(std::move(value)), calldata(std::move(calldata)), txInfo(txInfo),
		  accounts(16), runcodes(16), storage(16), transient(1), warmAccounts(parent.warmAccounts),
		  runcode(getContractCode(this->address)) {
	}
};


#endif //CMEVBOT_EVMSIMULATIONCONTEXT_H
