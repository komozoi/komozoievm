// Copyright 2025-2026 komozoi
// Original Creation Date: 2026-05-30
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
/** libkeccak-tiny
 *
 * A single-file implementation of SHA-3 and SHAKE.
 *
 * Implementor: David Leon Gil
 * License: CC0, attribution kindly requested. Blame taken too,
 * but not liability.
 */

#ifndef KECCAK_FIPS202_H
#define KECCAK_FIPS202_H
#define __STDC_WANT_LIB_EXT1__ 1
#include <stdint.h>
#include <stdlib.h>
#include "LongKey.h"
#include "ds/ArrayList.h"
#include "util/ConstArray.h"

#define decshake(bits) \
  int shake##bits(uint8_t*, size_t, const uint8_t*, size_t);

#define decsha3(bits) \
  int sha3_##bits(uint8_t*, size_t, const uint8_t*, size_t);

#define deckeccack(bits) \
  int keccack_##bits(uint8_t*, size_t, const uint8_t*, size_t);

decshake(128)
decshake(256)
decsha3(224)
decsha3(256)
decsha3(384)
decsha3(512)
deckeccack(256)


static inline LongKey<256> keccak256(const uint8_t* data, size_t length) {
	uint8_t buffer[32];
	keccack_256(buffer, 32, data, length);
	return LongKey<256>(buffer, false);
}

static inline LongKey<256> keccak256(const ArrayList<uint8_t>& data) {
	uint8_t buffer[32];
	keccack_256(buffer, 32, data.getMemory(), data.size());
	return LongKey<256>(buffer, false);
}

static inline LongKey<256> keccak256(const ConstArray<uint8_t>& data) {
	uint8_t buffer[32];
	keccack_256(buffer, 32, data.getMemory(), data.size());
	return LongKey<256>(buffer, false);
}

#ifdef CMEVBOT_BYTES_H
static inline LongKey<256> keccak256(Bytes data) {
	uint8_t buffer[32];
	keccack_256(buffer, 32, &data[0], data.size());
	return LongKey<256>(buffer, false);
}
#endif

static inline LongKey<256> keccak256message(const char* text) {
	char* tmp = (char*)malloc(strlen(text) + 36);
	sprintf(tmp, "\x19\x45thereum Signed Message:\n%i%s", strlen(text), text);

	uint8_t buffer[32];
	keccack_256(buffer, 32, (const uint8_t*)tmp, strlen(tmp));
	return LongKey<256>(buffer, false);
}

#endif
