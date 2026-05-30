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

#include "EthereumTransaction.h"
#include "util/keccak.h"


// WARNING: RLP VERY LIKELY DOES NOT WORK YET!!!

static inline void rlpEncodeBytes(const uint8_t* data, size_t len, ArrayList<uint8_t>& out) {
	if (len == 1 && data[0] < 0x80) {
		out.add(data[0]);
		return;
	}

	if (len < 56) {
		out.add(0x80 + len);
	} else {
		uint8_t lenBytes[8];
		int lenLen = 0;
		for (size_t i = len; i > 0; i >>= 8) {
			lenBytes[lenLen] = (i >> ((lenLen) * 8)) & 0xFF;
			lenLen++;
		}
		out.add(0xB7 + lenLen);
		for (int i = lenLen - 1; i >= 0; --i)
			out.add(lenBytes[i]);
	}
	out.addMany(data, len);
}

static inline void rlpEncodeUint64(uint64_t value, ArrayList<uint8_t>& out) {
	uint8_t buf[8];
	int len = 0;
	for (int i = 7; i >= 0; --i) {
		if (value >> (i * 8)) {
			buf[len++] = (value >> (i * 8)) & 0xFF;
		}
	}
	if (len == 0) {
		buf[0] = 0;
		len = 1;
	}
	rlpEncodeBytes(buf, len, out);
}

static inline void rlpEncodeListHeader(size_t payloadLength, ArrayList<uint8_t>& out) {
	if (payloadLength < 56) {
		out.add(0xC0 + payloadLength);
	} else {
		uint8_t lenBytes[8];
		int lenLen = 0;
		for (size_t i = payloadLength; i > 0; i >>= 8) {
			lenBytes[lenLen] = (i >> ((lenLen) * 8)) & 0xFF;
			lenLen++;
		}
		out.add(0xF7 + lenLen);
		for (int i = lenLen - 1; i >= 0; --i)
			out.add(lenBytes[i]);
	}
}

static inline void rlpEncodeAddress(const EthereumAddress& addr, ArrayList<uint8_t>& out) {
	out.add(0x94); // RLP string with length 20
	out.addMany(addr.data.rawBytes, 20);
}

bool EthereumTransaction::isValid() const {
	if (data == nullptr)
		return false;

	const LongKey<256>& s = data->sig.s;
	const LongKey<256>& r = data->sig.r;
	//const LongKey<256>& v = data->sig.v;

	if (s.isZero() || r.isZero())
		return false;

	/*ArrayList<uint8_t> encodedTransaction(0);
	this->encodeWithoutSignature(encodedTransaction);

	LongKey<256> txHash = keccak256(encodedTransaction);
	if (txHash.compare(data->hash) != 0)
		return false;*/

	/*secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);



	// Convert r, s, v into a signature
	ecdsa_signature_t signature = createECDSASignature(r, s, v);

	// Recover public key from signature
	secp256k1_pubkey pubkey;
	if (!secp256k1_ecdsa_recover(ctx, &pubkey, &signature, txHash))
		return false;

	uint8_t pubkeyBytes[65];
	secp256k1_ec_pubkey_serialize(ctx, pubkeyBytes, ..., &pubkey, SECP256K1_EC_UNCOMPRESSED);

	// Hash pubkey and derive Ethereum address
	uint8_t hash[32];
	keccak256(pubkeyBytes + 1, 64, hash);

	uint8_t recoveredAddress[20];
	memcpy(recoveredAddress, hash + 12, 20);

	if (memcmp(recoveredAddress, data->src.bytes, 20) != 0)
		return false;*/

	return true;
}

void EthereumTransaction::encodeWithoutSignature(ArrayList<uint8_t>& out) const {
	// All transactions contain these at the beginning
	if (data->type != TRANSACTION_TYPE_LEGACY) {
		out.add(data->type);

		// Chain ID
		out.add(1);
	}

	rlpEncodeUint64(data->nonce, out);

	if (data->type == TRANSACTION_TYPE_LEGACY || data->type == TRANSACTION_TYPE_EIP2718)
		rlpEncodeUint64(data->gas.gasPrice, out);
	else if (data->type == TRANSACTION_TYPE_EIP1559) {
		rlpEncodeUint64(data->gas.maxPriorityFeePerGas, out);
		rlpEncodeUint64(data->gas.maxFeePerGas, out);
	}

	rlpEncodeUint64(data->gas.gasLimit, out);
	rlpEncodeAddress(data->dst, out);
	rlpEncodeUint64(data->value, out);
	rlpEncodeBytes((uint8_t*)data->input, data->inputLength, out);

	if (data->type == TRANSACTION_TYPE_EIP2718 || data->type == TRANSACTION_TYPE_EIP1559)
		// Encode access list
		out.addMany(data->encodedAccessList, data->accessListLength);
}
