// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-07-10
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

#ifndef CMEVBOT_ETHEREUMTRANSACTION_H
#define CMEVBOT_ETHEREUMTRANSACTION_H

#include <cstdlib>
#include <atomic>
#include "ethereum.h"
#include "ds/ArrayList.h"
#include "util/Bytes.h"

#define TRANSACTION_TYPE_LEGACY 0
#define TRANSACTION_TYPE_EIP2718 1
#define TRANSACTION_TYPE_EIP1559 2
#define TRANSACTION_TYPE_EIP4844 3


struct ethereum_tx_internal_data_ptr_s {
	EthereumTxHash hash;
	EthereumAddress src;
	EthereumAddress dst;
	tx_signature_t sig;
	tx_gas_info_t gas;
	uint8_t* encodedAccessList;
	uint64_t nonce;
	uint64_t value;
	int accessListLength;
	std::atomic<uint16_t> refCount;
	uint16_t inputLength;
	uint8_t type;
	char input[];

	/*static ethereum_tx_internal_data_ptr_s* initFrom(const EthereumTxHash& hash, const EthereumAddress& src, const EthereumAddress& dst, const void* data, uint16_t length) {
		ethereum_tx_internal_data_ptr_s* out = (ethereum_tx_internal_data_ptr_s*)malloc(sizeof(ethereum_tx_internal_data_ptr_s) + length);
		out->hash = hash;
		out->src = src;
		out->dst = dst;
		out->length = length;
		out->refCount = 1;
		memcpy(out->data, data, length);
		return out;
	}*/

	static ethereum_tx_internal_data_ptr_s* initFrom(const EthereumTxHash& hash, const EthereumAddress& src, const EthereumAddress& dst, tx_signature_t sig, tx_gas_info_t gas, ArrayList<uint8_t> input, uint64_t nonce, uint8_t type, uint64_t value) {
		ethereum_tx_internal_data_ptr_s* out = (ethereum_tx_internal_data_ptr_s*)malloc(sizeof(ethereum_tx_internal_data_ptr_s) + input.size());

		out->hash = hash;
		out->src = src;
		out->dst = dst;
		out->sig = sig;
		out->gas = gas;
		out->encodedAccessList = nullptr;
		out->accessListLength = 0;
		out->nonce = nonce;
		out->value = value;
		out->type = type;
		out->refCount = 1;

		out->inputLength = input.size();
		memcpy(out->input, input.getMemory(), input.size());

		return out;
	}

	static ethereum_tx_internal_data_ptr_s* initFrom(const EthereumTxHash& hash, const EthereumAddress& src, const EthereumAddress& dst, tx_signature_t sig, tx_gas_info_t gas, ArrayList<uint8_t> input, uint64_t nonce, uint8_t type, uint64_t value, uint8_t* encodedAccessList, int accessListLength) {
		ethereum_tx_internal_data_ptr_s* out = (ethereum_tx_internal_data_ptr_s*)malloc(sizeof(ethereum_tx_internal_data_ptr_s) + input.size() + accessListLength);

		out->hash = hash;
		out->src = src;
		out->dst = dst;
		out->sig = sig;
		out->gas = gas;
		out->encodedAccessList = (uint8_t*)&out->input[input.size()];
		out->accessListLength = accessListLength;
		out->nonce = nonce;
		out->value = value;
		out->type = type;
		out->refCount = 1;

		out->inputLength = input.size();
		memcpy(out->input, input.getMemory(), input.size());
		memcpy(out->encodedAccessList, encodedAccessList, accessListLength);

		return out;
	}
};


class EthereumTransaction {
public:
	// Default initialization specifically doesn't initialize anything
	// This is useful for creating arrays with new[] and initializing each
	// element later.
	EthereumTransaction() : data(nullptr) {}

	/*EthereumTransaction(const EthereumTxHash& hash, const EthereumAddress& src, const EthereumAddress& dst, const void* data, uint16_t length)
		: data(ethereum_tx_internal_data_ptr_s::initFrom(hash, src, dst, data, length)) {
	}*/

	EthereumTransaction(EthereumTxHash hash, EthereumAddress src, EthereumAddress dst, tx_signature_t sig, tx_gas_info_t gas, ArrayList<uint8_t> input, uint64_t nonce, uint8_t type, uint64_t value)
		: data(ethereum_tx_internal_data_ptr_s::initFrom(hash, src, dst, sig, gas, input, nonce, type, value)) {

	}

	EthereumTransaction(const EthereumTransaction& other)
		: data(other.data) {
		if (data != nullptr)
			data->refCount.fetch_add(1);
	}

	EthereumTransaction(EthereumTransaction&& other) noexcept
		: data(other.data) {
		other.data = nullptr;
	}

	EthereumTransaction& operator=(const EthereumTransaction& other) {
		if (&other != this && data != other.data) {
			if (data != nullptr && data->refCount.fetch_sub(1, std::memory_order_relaxed) == 1)
				free(data);
			data = other.data;
			if (data != nullptr)
				data->refCount.fetch_add(1);
		}
		return *this;
	}

	EthereumTransaction& operator=(EthereumTransaction&& other) noexcept {
		if (&other != this && data != other.data) {
			if (data != nullptr && data->refCount.fetch_sub(1, std::memory_order_relaxed) == 1)
				free(data);
			data = other.data;
			if (data != nullptr)
				data->refCount.fetch_add(1);
			other.data = nullptr;
		}
		return *this;
	}

	operator bool() {
		return data != nullptr;
	}

	inline const EthereumTxHash& hash() const {
		return data->hash;
	}

	inline const EthereumAddress& sender() const {
		return data->src;
	}

	inline const EthereumAddress& recipient() const {
		return data->dst;
	}

	inline uint256_t value() const {
		return data->value;
	}

	inline Bytes calldata() const {
		return Bytes(data->input, data->inputLength);
	}

	inline uint32_t gasLimit() const {
		return data->gas.gasLimit;
	}

	tx_gas_info_t gasInfo() const {
		return data->gas;
	}

	uint64_t nonce() const {
		return data->nonce;
	}

	bool isValid() const;

	inline ~EthereumTransaction() {
		if (data != nullptr && data->refCount.fetch_sub(1) == 1)
			free(data);
	}

private:
	ethereum_tx_internal_data_ptr_s* data;

	void encodeWithoutSignature(ArrayList<uint8_t> &out) const;
};


#endif //CMEVBOT_ETHEREUMTRANSACTION_H
