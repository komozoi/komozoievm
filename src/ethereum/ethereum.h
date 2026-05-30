// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-07-04
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

#ifndef CMEVBOT_ETHEREUM_H
#define CMEVBOT_ETHEREUM_H

#include "LongKey.h"
#include "bigint.h"
#include "ds/ArrayList.h"


typedef LongKey<160> EthereumAddress;
typedef LongKey<256> EthereumTxHash;
typedef LongKey<256> EthereumBlockHash;

typedef struct {
	LongKey<256> r, s;
	uint8_t yParity, v;
} tx_signature_t;

typedef struct {
	uint32_t gasLimit;
	uint64_t gasPrice;
	uint64_t maxFeePerGas;
	uint64_t maxPriorityFeePerGas;
} tx_gas_info_t;

typedef struct {
	uint256_t randao;
	EthereumAddress coinbase;
	uint64_t number;
	uint64_t timestamp;
	uint64_t gasLimit;
	uint64_t baseFee;
	uint32_t chainId;
} block_info_t;


class EthereumAccessListEntry {
public:
	EthereumAccessListEntry(const EthereumAddress& address) : address(address) {}

	inline uint32_t initialGasCost() const {
		return 2400 + 1900 * storageKeys.size();
	}

	EthereumAddress address;
	ArrayList<LongKey<256>> storageKeys;
};


class EthereumAccountInfo {
public:
	inline EthereumAccountInfo() : isValid(false) {}

	inline explicit EthereumAccountInfo(EthereumAddress&& address, uint256_t&& balance = 0, uint64_t nextNonce = 0)
		: balance(balance), address(address), nextNonce(nextNonce) {}

	inline explicit EthereumAccountInfo(const EthereumAddress& address, const uint256_t& balance = 0, uint64_t nextNonce = 0)
		: balance(balance), address(address), nextNonce(nextNonce) {}

	inline EthereumAccountInfo(EthereumAccountInfo&& other) noexcept = default;

	inline EthereumAccountInfo(const EthereumAccountInfo& other) = default;

	EthereumAccountInfo& operator=(const EthereumAccountInfo& other) {
		if (&other != this) {
			balance = other.balance;
			*(EthereumAddress*)&address = other.address;
			nextNonce = other.nextNonce;
		}
		return *this;
	}

	EthereumAccountInfo& operator=(EthereumAccountInfo&& other)  noexcept {
		balance = other.balance;
		*(EthereumAddress*)&address = other.address;
		nextNonce = other.nextNonce;
		return *this;
	}

	inline operator bool() const {
		return isValid;
	}

	uint256_t balance;
	const EthereumAddress address;
	uint64_t nextNonce;
	const bool isValid = true;
};


void toChecksumAddress(const char* inputAddress, char* outputBuffer);


#endif //CMEVBOT_ETHEREUM_H
