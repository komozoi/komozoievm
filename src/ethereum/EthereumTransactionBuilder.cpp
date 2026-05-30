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

#include "EthereumTransactionBuilder.h"
#include "EVM.h"
#include "RLPEncoder.h"

ArrayList<uint8_t> EthereumTransactionBuilder::signAndExport(const Secp256k1PrivateKey& key, const uint256_t& k) const {
	ArrayList<uint8_t> out;
	RLPEncoder encoder(out);
	uint32_t chainId = 1;

	// Adds a placeholder slot for the transaction type and RLP length specifier
	out.addCopies(0, 6);

	encoder.encode(chainId);
	encoder.encode(nonce);
	encoder.encode(gasInfo.maxPriorityFeePerGas);
	encoder.encode(gasInfo.maxFeePerGas);
	encoder.encode(gasInfo.gasLimit);
	encoder.encode(dst);
	encoder.encode(value);
	encoder.encode(calldata.getMemory(), calldata.size());
	encoder.encode(accessList);

	int txTypeIdx = encoder.encodeListStartAtOffset(6) - 1;
	out.set(txTypeIdx, 0x02);

	Secp256k1Signature signature = key.sign(&out.get(txTypeIdx), out.size() - txTypeIdx, k);

	encoder.encode(signature.v);
	encoder.encode(signature.r);
	encoder.encode(signature.s);

	// We have to do this again because adding the signature changes the size of the RLP-encoded list
	txTypeIdx = encoder.encodeListStartAtOffset(6) - 1;
	out.set(txTypeIdx, 0x02);

	// Tell the caller where the transaction begins
	out.set(0, txTypeIdx);

	return out;
}

EthereumTransactionBuilder::operator bool() const {
	uint32_t initialGasCost = EVM::getInitialGasCost(calldata);

	for (const EthereumAccessListEntry& entry: accessList)
		initialGasCost += entry.initialGasCost();

	if (gasInfo.gasLimit < initialGasCost)
		return false;
	if (gasInfo.maxFeePerGas < gasInfo.maxPriorityFeePerGas)
		return false;
	return true;
}
