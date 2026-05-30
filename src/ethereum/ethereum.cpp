// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-08-07
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

#include "string.h"
#include "ctype.h"
#include "LongKey.h"
#include "util/keccak.h"


void toChecksumAddress(const char* inputAddress, char* outputBuffer) {
	// Skip "0x" prefix if present
	const char* addr = inputAddress;
	if (addr[0] == '0' && (addr[1] == 'x' || addr[1] == 'X')) {
		addr += 2;
	}

	size_t addrLen = strlen(addr);
	if (addrLen != 40) {
		// Invalid address length (must be 40 hex chars)
		outputBuffer[0] = '\0';
		return;
	}

	// Lowercase copy for hashing
	char lowerAddr[41];
	for (int i = 0; i < 40; ++i) {
		lowerAddr[i] = tolower(addr[i]);
	}
	lowerAddr[40] = '\0';

	// Compute Keccak256 hash of the lowercase address string
	LongKey<256> hash = keccak256((uint8_t*)lowerAddr, 40);

	// Start writing to output buffer
	outputBuffer[0] = '0';
	outputBuffer[1] = 'x';

	for (size_t i = 0; i < 40; ++i) {
		char c = addr[i];

		// Determine if current char is a letter (a-f or A-F)
		if (std::isalpha(c)) {
			// Hash nibble: one hex digit per character position
			size_t byteIdx = i / 2;
			bool highNibble = (i % 2 == 0);
			uint8_t nibble = highNibble
							 ? (hash.data.rawBytes[byteIdx] >> 4)
							 : (hash.data.rawBytes[byteIdx] & 0x0F);

			outputBuffer[2 + i] = (nibble >= 8)
								  ? toupper(static_cast<unsigned char>(c))
								  : tolower(static_cast<unsigned char>(c));
		} else {
			// Leave numeric digits as-is
			outputBuffer[2 + i] = c;
		}
	}

	outputBuffer[42] = '\0';
}
