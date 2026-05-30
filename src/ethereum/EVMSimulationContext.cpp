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

#include "EVMSimulationContext.h"

void EVMSimulationContext::writeChangesTo(EVMSimulationContext& dst) const {
	writeChangesTo((StateProvider&)dst);

	dst.warmAccounts.addFrom(warmAccounts);

	for (int i = 0; i < transient.getCapacity(); i++)
		if (transient.presentAtIndex(i))
			dst.updateTransientSlots(transient.keyAtIndex(i), *transient.valueAtIndex(i));
}

void EVMSimulationContext::writeChangesTo(StateProvider& dst) const {
	for (int i = 0; i < accounts.getCapacity(); i++)
		if (accounts.presentAtIndex(i))
			dst.updateAccount(accounts.keyAtIndex(i), accounts.valueAtIndex(i));

	// for (int i = 0; i < runcodes.getCapacity(); i++)
	//	if (runcodes.presentAtIndex(i))
	//		dst.saveContractCode(runcodes.keyAtIndex(i), runcodes.valueAtIndex(i));

	for (int i = 0; i < storage.getCapacity(); i++)
		if (storage.presentAtIndex(i))
			dst.updateStorageSlots(storage.keyAtIndex(i), *storage.valueAtIndex(i));
}

uint32_t EVMSimulationContext::totalGasUsed() const {
	uint32_t total = gasUsed;
	if (returnContext)
		total += returnContext->gasUsed;
	return total;
}

uint256_t EVMSimulationContext::tload(const LongKey<256>& key) {
	HashMap<LongKey<256>, uint256_t>* map = transient.getOrDefault(address, nullptr);
	if (map == nullptr)
		map = transient.put(address, new HashMap<LongKey<256>, uint256_t>(4));
	else {
		uint256_t* valuePtr = map->getPtr(key);
		if (valuePtr)
			return *valuePtr;
	}

	if (!returnContext)
		return 0;
	return map->put(key, returnContext->tload(key));
}

bool EVMSimulationContext::tstore(const LongKey<256>& key, const uint256_t& value) {
	if (isStatic)
		return false;

	HashMap<LongKey<256>, uint256_t>* map = transient.getOrDefault(address, nullptr);
	if (map == nullptr)
		map = transient.put(address, new HashMap<LongKey<256>, uint256_t>(4));

	map->put(key, value);
	return true;
}

uint256_t EVMSimulationContext::sload(const LongKey<256>& key) {
	HashMap<LongKey<256>, uint256_t>* map = storage.getOrDefault(address, nullptr);
	if (map == nullptr)
		map = storage.put(address, new HashMap<LongKey<256>, uint256_t>(4));
	else {
		uint256_t* valuePtr = map->getPtr(key);
		if (valuePtr)
			return *valuePtr;
	}

	uint256_t storedValue;
	if (!chain.getStorageSlot(address, key, storedValue, txInfo.stateBlockNumber))
		storedValue = 0;
	gasUsed += 2000;
	return map->put(key, storedValue);
}

bool EVMSimulationContext::sstore(const LongKey<256>& key, const uint256_t& value) {
	if (isStatic)
		return false;

	HashMap<LongKey<256>, uint256_t>* map = storage.getOrDefault(address, nullptr);
	if (map == nullptr)
		map = storage.put(address, new HashMap<LongKey<256>, uint256_t>(4));

	uint256_t* valuePtr = map->getPtr(key);
	if (valuePtr) {
		*valuePtr = value;
	} else {
		uint256_t previousValue;
		chain.getStorageSlot(address, key, previousValue, txInfo.stateBlockNumber);
		map->put(key, value);

		// Gas calculations are complex...
		if (value == previousValue) {
			// no extra gas usage
		}/* else {
			// Extra gas costs
			if (value == originalValue) {
				if (originalValue.isZero())
					gasUsed += 19900;
				else
					gasUsed += 2800;
			}

			// Gas refunds
			if (previousValue == originalValue) {
				if (value.isZero() && !originalValue.isZero())
					gasRefund += 4800;
			} else {
				if (originalValue != 0)
					if (previousValue.isZero())
						gasRefund -= 4800;
					else if (value.isZero())
						gasRefund += 4800;
				if (value == originalValue)
					if (originalValue == 0)
						gasRefund += 20000 - 100;
					else
						gasRefund += 5000 - 2100 - 100;
			}
		}*/
	}

	return true;
}

void EVMSimulationContext::setStorageOverride(const EthereumAddress& contract, const LongKey<256>& key, const uint256_t& value) {
	HashMap<LongKey<256>, uint256_t> *map = storage.get(contract);
	if (map == nullptr)
		map = storage.put(contract, new HashMap<LongKey<256>, uint256_t>(4));

	uint256_t *valuePtr = map->getPtr(key);
	if (valuePtr)
		*valuePtr = value;
	else
		map->put(key, value);
}

bool EVMSimulationContext::expandMemory(uint256_t offset256, uint256_t size256) {
	// Not willing to allocate this much RAM
	if (offset256 + size256 > 1482849)
		return false;

	int offset = offset256;
	int size = size256;
	if (offset < 0 || size < 0)
		return false;

	if (offset + size > memory.size()) {
		// Compute expansion costs
		int lastWordAccessed = (offset + size + 31) >> 5;
		int expansionCost = ((lastWordAccessed * lastWordAccessed) >> 9) + (3 * lastWordAccessed);
		lastWordAccessed = (memory.size() + 31) >> 5;
		expansionCost -= ((lastWordAccessed * lastWordAccessed) >> 9) + (3 * lastWordAccessed);
		gasUsed += expansionCost;
		if (gasUsed > txInfo.gas.gasLimit)
			return false;

		memory.addCopies(0, offset + size - memory.size());
	}

	return true;
}

uint8_t* EVMSimulationContext::getMemoryStartingAt(const uint256_t& offset256, const uint256_t& size256) {
	if (!expandMemory(offset256, size256))
		return nullptr;

	return &memory.getMemory()[(int)offset256];
}

EthereumAccountInfo EVMSimulationContext::getAccountInfo(const EthereumAddress& address, uint64_t blockNumber) {
	blockNumber = blockNumber ? blockNumber : txInfo.stateBlockNumber;

	EthereumAccountInfo* ptr = accounts.getPtr(address);
	if (!ptr)
		return accounts.put(address, chain.getAccountInfo(address, blockNumber));
	return *ptr;
}

Bytes EVMSimulationContext::getContractCode(const EthereumAddress& address) {
	Bytes* code = runcodes.getPtr(address);

	if (!code) {
		warmAccount(address);
		Bytes codeL = chain.getContractCode(address);
		if (!codeL) {
			char buf[128];
			address.toStr(buf);
			printf("Error getting contract bytecode for %s\n", buf);
		}
		return runcodes.put(address, std::move(codeL));
	}

	return *code;
}

ArrayList<uint256_t> EVMSimulationContext::getStorageSlots(const EthereumAddress& address, const ArrayList<LongKey<256>>& slotKeys, uint64_t blockNumber) {
	blockNumber = blockNumber ? blockNumber : txInfo.stateBlockNumber;

	HashMap<LongKey<256>, uint256_t>* slots = storage.getOrDefault(address, nullptr);
	if (!slots)
		slots = storage.put(address, new HashMap<LongKey<256>, uint256_t>(16));

	ArrayList<uint256_t> out(slotKeys.size());
	for (int i = 0; i < slotKeys.size(); i++) {
		LongKey<256>& key = slotKeys.get(i);
		uint256_t* value = slots->getPtr(key);
		if (!value) {
			value = &slots->put(key, uint256_t());
			if (!chain.getStorageSlot(address, key, *value, blockNumber))
				// Error
				return out;
		}
		out.add(*value);
	}

	return out;
}

bool EVMSimulationContext::updateAccount(const EthereumAddress& key, const EthereumAccountInfo& account) {
	if (isStatic)
		return false;

	accounts.put(key, account);
	return true;
}

bool EVMSimulationContext::saveContractCode(const EthereumAddress& key, Bytes runcode) {
	if (isStatic)
		return false;

	runcodes.put(key, runcode);
	return true;
}

bool EVMSimulationContext::updateStorageSlots(const EthereumAddress& key, const HashMap<LongKey<256>, uint256_t>& entries) {
	if (isStatic)
		return false;

	HashMap<LongKey<256>, uint256_t>* dst = storage.get(key);
	if (dst)
		dst->putFrom(entries);
	else
		storage.put(key, new HashMap(entries));
	return true;
}

bool EVMSimulationContext::updateTransientSlots(const EthereumAddress& key, const HashMap<LongKey<256>, uint256_t>& entries) {
	if (isStatic)
		return false;

	HashMap<LongKey<256>, uint256_t>* dst = transient.get(key);
	if (dst)
		dst->putFrom(entries);
	else
		transient.put(key, new HashMap(entries));
	return true;
}

EVMSimulationContext::~EVMSimulationContext() {
	for (MapElement<EthereumAddress, HashMap<LongKey<256>, uint256_t>*> entries: storage)
		delete entries.value;
	for (MapElement<EthereumAddress, HashMap<LongKey<256>, uint256_t>*> entries: transient)
		delete entries.value;
}
