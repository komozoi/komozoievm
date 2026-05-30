// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-08-05
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

#include "secrandom.h"
#include "modbigint.h"
#include "fs/FdHandle.h"
#include "fcntl.h"
#include "universaltime.h"


// (1 << 512) - 0x2000000 - 937  (probably prime)
static const UnsignedFixedWidthBigInt<8> N("0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffdfffc57");
static UnsignedModBigInt<8, N> state;
static std::mutex randomnessLock;


void seedSecureRandom() {
	FdHandle fd = FdHandle::open("/dev/urandom", O_RDONLY);
	if (fd)
		fd.read(state.data);
	fd.close();

	state = state * UnsignedFixedWidthBigInt<8>(microseconds_since_day_started());
}

void getSecureRandomBytes(void* dst, unsigned nBytes) {
	std::lock_guard _(randomnessLock);

	uint64_t seed1 = (uint64_t)dst;
	uint64_t seed2 = nBytes;

	for (unsigned i = 0; i < nBytes; i += (state.data.rawBytes[0] & 63) + 1) {
		unsigned toCopy = 64;
		if (toCopy + i > nBytes)
			toCopy = nBytes - i;

		state = (state + (N >> (int)((i * 977 + 93) & 511))) * UnsignedFixedWidthBigInt<8>(seed1);

		memcpy(&((char*)dst)[i], state.data.rawBytes, toCopy);

		state = state * UnsignedFixedWidthBigInt<8>(seed2);

		seed2 *= seed1;

		// (1 << 32) - 0x10000 - 1009  (prime)
		seed1 *= (0xfffefc0f << (i & 31));
	}

	state = state * UnsignedFixedWidthBigInt<8>(microseconds_since_day_started());
}
