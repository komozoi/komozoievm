// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-08-06
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

#ifndef CMEVBOT_ETHEREUMTRANSACTIONBUILDE_H
#define CMEVBOT_ETHEREUMTRANSACTIONBUILDE_H

#include <utility>

#include "crypto/Secp256k1.h"


class EthereumTransactionBuilder {
public:
	EthereumTransactionBuilder(const EthereumAddress& dst, uint64_t nonce, uint32_t gasLimit)
		: dst(dst), nonce(nonce) {
		gasInfo.gasLimit = gasLimit;
	}

	ArrayList<uint8_t> signAndExport(const Secp256k1PrivateKey& key, const uint256_t& k = 0) const;

	operator bool() const;

	EthereumAddress dst;
	uint256_t value;
	uint64_t nonce;
	tx_gas_info_t gasInfo = {0, 0, 0, 0};
	ArrayList<uint8_t> calldata;
	ArrayList<EthereumAccessListEntry> accessList;
};


#endif //CMEVBOT_ETHEREUMTRANSACTIONBUILDE_H
