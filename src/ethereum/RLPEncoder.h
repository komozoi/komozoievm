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

#ifndef CMEVBOT_RLPENCODER_H
#define CMEVBOT_RLPENCODER_H

#include "stdint.h"
#include "ds/ArrayList.h"
#include "bigint.h"
#include "ethereum.h"


class RLPEncoder {
public:
	explicit RLPEncoder(ArrayList<uint8_t>& dst)
		: dst(dst) {}

	void encode(uint64_t number);
	void encode(const uint8_t* s, int n);
	void encodeReversed(const uint8_t* s, int n);

	template<int N>
	void encode(const UnsignedFixedWidthBigInt<N>& number) {
		int nBytes = (number.countBits() + 7) >> 3;

		encodeReversed(number.data.rawBytes, nBytes);
	}

	template<int N>
	void encode(const LongKey<N>& number) {
		encodeReversed(number.data.rawBytes, N>>3);
	}

	template<class T>
	void encode(const ArrayList<T>& list) {
		int totalLength = 0;
		for (const T& item : list)
			totalLength += lengthOf(item);

		encodeLength(totalLength, 0xc0);

		for (const T& item : list)
			encode(item);
	}

	void encode(const EthereumAccessListEntry& entry);

	inline void startList(int nBytesToEncode) {
		encodeLength(nBytesToEncode, 0xc0);
	}

	static inline int lengthOf(uint64_t number) {
		return lengthOf(nullptr, (countBits(number) - 1) >> 3);
	}

	static inline int lengthOf(const uint8_t* s, int n) {
		(void)s;
		return n + lengthOfLength(n);
	}

	template<int N>
	static inline int lengthOf(UnsignedFixedWidthBigInt<N>& number) {
		return lengthOf(nullptr, (number.countBits() - 1) >> 3);
	}

	template<int N>
	static inline int lengthOf(const LongKey<N>& number) {
		return lengthOf(nullptr, N >> 3);
	}

	template<class T>
	static inline int lengthOf(const ArrayList<T>& list) {
		int totalLength = 0;
		for (const T& item : list)
			totalLength += lengthOf(item);

		return lengthOfLength(totalLength) + totalLength;
	}

	static inline int lengthOf(const EthereumAccessListEntry& entry) {
		int bytesToEncode = lengthOf(entry.storageKeys);
		bytesToEncode += lengthOf(entry.address);
		return lengthOfLength(bytesToEncode);
	}

	int encodeListStartAtOffset(int listStartOffset);

private:
	void encodeLength(int n, uint8_t offset);
	static inline int lengthOfLength(int n) {
		return n < 56 ? 1 : (countBits(n) + 15) >> 3;
	}

	ArrayList<uint8_t>& dst;
};


#endif //CMEVBOT_RLPENCODER_H
