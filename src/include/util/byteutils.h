// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-07-12
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
#ifndef CMEVBOT_BYTEUTILS_H
#define CMEVBOT_BYTEUTILS_H

#include "stdint.h"
#include <string_view>
#include "ArrayList.h"
#include "strutil.h"


static inline ArrayList<uint8_t> parseHexToBytes(std::string_view s) {
	if (s.size() > 1 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
		s = s.substr(2);

	ArrayList<uint8_t> out((int)s.length() / 2);

	for (uint32_t i = 0; i < s.length() / 2; i++) {
		uint8_t val = parseHexDigit(s[i*2]) << 4;
		val |= parseHexDigit(s[i*2 + 1]);
		out.add(val);
	}

	return out;
}


#endif //CMEVBOT_BYTEUTILS_H
