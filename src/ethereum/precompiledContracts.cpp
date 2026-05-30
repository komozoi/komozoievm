// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-07-23
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

#include <cstdio>
#include "precompiledContracts.h"
#include "bigint.h"
#include "crypto/Secp256k1.h"
#include "crypto/picosha2.h"


#define MIN(X, Y) ((X) > (Y) ? Y : X)


static int precompiledEcRecover(const uint8_t* argsMem, int argsSize, uint8_t* retMem, int retSize, int gasLeft) {
	struct {
		uint256_t hash;
		uint256_t v;
		uint256_t r;
		uint256_t s;
	} in {
		uint256_t(&argsMem[0x00], MIN(0x20, argsSize + 0x00), true),
		uint256_t(&argsMem[0x20], MIN(0x20, argsSize + 0x20), true),
		uint256_t(&argsMem[0x40], MIN(0x20, argsSize + 0x40), true),
		uint256_t(&argsMem[0x60], MIN(0x20, argsSize + 0x60), true)
	};

	bzero(retMem, retSize);

	if (in.v >= 27 && in.v <= 28) {
		int v = in.v;
		Secp256k1Signature sig(in.r, in.s, (uint8_t)(v - 27));
		if (sig.isValid()) {
			if (Secp256k1PublicKey rec = sig.recover(in.hash)) {
				rec.toAddress().toBytes(&retMem[12], MIN(retSize, 0x20));
				return 3000;
			}
		}
	}

	return 3000;
}


static int precompiledSha256(const uint8_t* argsMem, int argsSize, uint8_t* retMem, int retSize, int gasLeft) {
	// Compute gas cost
	int words = (argsSize + 31) / 32;
	int gasCost = 60 + 12 * words;

	if (gasCost < gasLeft) {
		// Compute SHA-256 hash
		sha2(argsMem, argsSize).toBytes(retMem, false);

		if (retSize > 32)
			bzero(&retMem[32], retSize - 32);
	}

	return gasCost;
}


static int precompiledMemcpy(const uint8_t* argsMem, int argsSize, uint8_t* retMem, int retSize, int gasLeft) {
	// TODO: This may return more data, but we don't have a way to accept it yet.
	//       This is a known bug.
	if (argsSize > retSize)
		argsSize = retSize;

	// Compute gas cost
	int words = (argsSize + 31) / 32;
	int gasCost = 15 + 3 * words;

	if (gasCost < gasLeft)
		memcpy(retMem, argsMem, argsSize);

	return gasCost;
}


const char* callPrecompiled(int index, const uint8_t* argsMem, int argsSize, uint8_t* retMem, int retSize, int& gas) {
	int gasUsed;
	switch (index) {
		case 0x01:
			gasUsed = precompiledEcRecover(argsMem, argsSize, retMem, retSize, gas);
			break;
		case 0x02:
			gasUsed = precompiledSha256(argsMem, argsSize, retMem, retSize, gas);
			break;
		/*case 0x03:
			gasUsed = precompiledRipeMD5(argsMem, argsSize, retMem, retSize, gasLeft);
			break;*/
		case 0x04:
			gasUsed = precompiledMemcpy(argsMem, argsSize, retMem, retSize, gas);
			break;
		/*case 0x05:
			gasUsed = precompiledModexp(argsMem, argsSize, retMem, retSize, gasLeft);
			break;
		case 0x06:
			gasUsed = precompiledEcAdd(argsMem, argsSize, retMem, retSize, gasLeft);
			break;
		case 0x07:
			gasUsed = precompiledEcMul(argsMem, argsSize, retMem, retSize, gasLeft);
			break;
		case 0x08:
			gasUsed = precompiledEcPairing(argsMem, argsSize, retMem, retSize, gasLeft);
			break;
		case 0x09:
			gasUsed = precompiledBlake2F(argsMem, argsSize, retMem, retSize, gasLeft);
			break;
		case 0x0A:
			gasUsed = precompiledCheckProof(argsMem, argsSize, retMem, retSize, gasLeft);
			break;*/
		default:
			printf("Use of unknown precompile %x\n", index);
			return "Unknown precompile";
	}

	if (gasUsed > gas) {
		gas = gasUsed;
		return "Out of gas";
	}

	// Success
	gas = gasUsed;
	return nullptr;
}

