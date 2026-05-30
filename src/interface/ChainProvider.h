// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-07-15
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

#ifndef CMEVBOT_CHAINPROVIDER_H
#define CMEVBOT_CHAINPROVIDER_H

#include "ethereum/ethereum.h"
#include "util/Bytes.h"
#include "util/ConstArray.h"
#include "ds/ArrayList.h"
#include "ds/HashMap.h"
#include "bigint.h"
#include "ethereum/TransactionReceipt.h"

#define PENDING_BLOCK 0x8000000000000000


class StateProvider {
public:
	// Account information
	virtual EthereumAccountInfo getAccountInfo(const EthereumAddress& address, uint64_t blockNumber = 0) = 0;
	virtual EthereumAccountInfo getAccountInfo(EthereumAddress&& address, uint64_t blockNumber = 0) { return getAccountInfo(address, blockNumber); }
	virtual Bytes getContractCode(const EthereumAddress& address) = 0;
	virtual Bytes getContractCode(EthereumAddress&& address) { return getContractCode(address); }
	virtual ArrayList<uint256_t> getStorageSlots(const EthereumAddress& address, const ArrayList<LongKey<256>>& slotKeys, uint64_t blockNumber = 0) = 0;
	virtual ArrayList<uint256_t> getStorageSlots(EthereumAddress&& address, const ArrayList<LongKey<256>>& slotKeys, uint64_t blockNumber = 0) {
		return getStorageSlots(address, slotKeys, blockNumber);
	}

	virtual bool getStorageSlot(const EthereumAddress& address, const LongKey<256>& slot, uint256_t& valueOut, uint64_t blockNumber = 0) {
		ArrayList<LongKey<256>> slots(slot);
		ArrayList<uint256_t> values = getStorageSlots(address, slots, blockNumber);
		if (values.size() != 1)
			return false;

		valueOut = values.get(0);
		return true;
	}

	virtual bool updateAccount(const EthereumAddress& key, const EthereumAccountInfo& account) { return false; }
	virtual bool saveContractCode(const EthereumAddress& key, Bytes runcode) { return false; }
	virtual bool updateStorageSlots(const EthereumAddress& key, const HashMap<LongKey<256>, uint256_t>& entries) { return false; }

	virtual uint32_t numCachedSlotsFor(const EthereumAddress& address) { return 0; }

	virtual ~StateProvider() = default;
};


class ChainProvider: public StateProvider {
public:

	virtual block_info_t getBlockInfoByNumber(uint64_t blockNumber) = 0;
	virtual ConstArray<TransactionReceipt> getReceiptsFromBlockByNumber(uint64_t blockNumber) = 0;

	virtual ConstArray<Bytes> bulkGetContractCode(const ArrayList<EthereumAddress>& toLoad) = 0;
	virtual ConstArray<EthereumAccountInfo> bulkGetAccountInfo(const ArrayList<EthereumAddress>& toLoad, uint64_t blockNumber = 0) = 0;

	//virtual HashMap<EthereumAddress, HashMap<LongKey<256>, uint256_t>> getStorageChangesFromBlock(uint64_t blockNumber) = 0;
};

#endif //CMEVBOT_CHAINPROVIDER_H
