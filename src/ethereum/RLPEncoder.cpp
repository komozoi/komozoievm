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

#include "RLPEncoder.h"

void RLPEncoder::encode(uint64_t number) {
	uint8_t buffer[8];

	int i = 7;
	for (; i >= 0 && number != 0; i--) {
		buffer[i] = number & 0xFF;
		number = number >> 8;
	}

	encode(&buffer[i + 1], 7 - i);
}

void RLPEncoder::encode(const uint8_t* s, int n) {
	if (n == 1 && *s < 0x80)
		dst.add(*s);
	else {
		encodeLength(n, 0x80);
		dst.addMany(s, n);
	}
}

void RLPEncoder::encodeReversed(const uint8_t* s, int n) {
	if (n == 1 && *s < 0x80)
		dst.add(*s);
	else {
		encodeLength(n, 0x80);
		for (const uint8_t* p = &s[n - 1]; p >= s; p--)
			dst.add(*p);
	}
}

void RLPEncoder::encode(const EthereumAccessListEntry& entry) {
	int bytesToEncode = lengthOf(entry.storageKeys);
	bytesToEncode += lengthOf(entry.address);
	startList(bytesToEncode);
	encode(entry.address);
	encode(entry.storageKeys);
}

void RLPEncoder::encodeLength(int n, uint8_t offset) {
	if (n < 56)
		dst.add(n + offset);
	else {
		uint8_t buffer[4];
		int i = 3;
		for (; i >= 0; i--) {
			buffer[i] = ((n >> 8 * i) & 0xFF);
			if (buffer[i] == 0)
				break;
		}

		uint8_t preLenByte = offset + 55 + 3 - i;
		dst.add(preLenByte);
		dst.addMany(&buffer[i + 1], 3 - i);
	}
}

int RLPEncoder::encodeListStartAtOffset(int listStartOffset) {
	int listLengthBytes = dst.size() - listStartOffset;
	int nLenBytes = lengthOfLength(listLengthBytes);
	int lengthStartOffset = listStartOffset - nLenBytes;

	int wrIdx = lengthStartOffset;
	if (listLengthBytes < 56)
		dst.set(wrIdx, listLengthBytes + 0xC0);
	else {
		uint8_t preLenByte = 0xC0 + 55 + nLenBytes - 1;
		dst.set(wrIdx++, preLenByte);
		int i = 0;
		for (; wrIdx < listStartOffset; i++)
			dst.set(wrIdx++, ((listLengthBytes >> (8 * i)) & 0xFF));
	}

	return lengthStartOffset;
}

