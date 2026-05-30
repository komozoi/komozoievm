// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-07-30
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

#ifndef CMEVBOT_TRANSACTIONRECEIPT_H
#define CMEVBOT_TRANSACTIONRECEIPT_H

#include <utility>

#include "stdint.h"

#include "ethereum.h"
#include "util/Bytes.h"
#include "util/ConstArray.h"


class EthereumEventLog {
public:
	inline EthereumEventLog(EthereumAddress address, Bytes data, ConstArray<LongKey<256>> topics)
		: address(std::move(address)), data(std::move(data)), topics(std::move(topics)) {}

	EthereumAddress address;
	Bytes data;
	ConstArray<LongKey<256>> topics;
};


class TransactionReceipt {
public:
	TransactionReceipt() = default;

	TransactionReceipt(TransactionReceipt&& other) noexcept
			: blockNumber(other.blockNumber), gasUsed(other.gasUsed), gasPrice(other.gasPrice),
			  src(other.src), dst(other.dst), blockIndex(other.blockIndex), didSucceed(other.didSucceed),
			  hash(other.hash), logs(std::move(other.logs)) {}

	TransactionReceipt(const TransactionReceipt& other) noexcept
			: blockNumber(other.blockNumber), gasUsed(other.gasUsed), gasPrice(other.gasPrice),
			  src(other.src), dst(other.dst), blockIndex(other.blockIndex), didSucceed(other.didSucceed),
			  hash(other.hash), logs(other.logs) {}

	uint64_t blockNumber;
	uint64_t gasUsed;
	uint64_t gasPrice;
	EthereumAddress src, dst;
	uint32_t blockIndex;
	bool didSucceed;
	EthereumTxHash hash;
	ConstArray<EthereumEventLog> logs;
};


#endif //CMEVBOT_TRANSACTIONRECEIPT_H
