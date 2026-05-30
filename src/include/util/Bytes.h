// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-10-12
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
#ifndef CMEVBOT_BYTES_H
#define CMEVBOT_BYTES_H

#include <atomic>
#include "stdlib.h"
#include "stdint.h"


class Bytes {
public:
	Bytes(const char* s);
	Bytes(void* buffer, size_t length);
	Bytes(size_t length);

	Bytes(const Bytes& other);
	Bytes(Bytes&& other) noexcept;

	inline Bytes() : storage(nullptr) {}

	Bytes& operator =(Bytes&& other) noexcept;
	Bytes& operator =(const Bytes& other);

	bool operator >(const Bytes& other) const;
	bool operator ==(const Bytes& other) const;

	inline bool operator <(const Bytes& other) const {
		return other > *this;
	}

	inline bool operator >=(const Bytes& other) const {
		return !(other > *this);
	}

	inline bool operator <=(const Bytes& other) const {
		return !(*this > other);
	}

	inline bool operator !=(const Bytes& other) const {
		return !(*this == other);
	}

	inline uint8_t& operator[](unsigned int index) {
		return ((uint8_t*)&storage[1])[index];
	}

	inline const uint8_t& operator[](unsigned int index) const {
		return ((uint8_t*)&storage[1])[index];
	}

	// For backward compatibility: implement get()

	inline uint8_t& get(unsigned int index) {
		return ((uint8_t*)&storage[1])[index];
	}

	inline const uint8_t& get(unsigned int index) const {
		return ((uint8_t*)&storage[1])[index];
	}

	inline explicit operator bool() const {
		return (bool)storage;
	}

	size_t size() const {
		return storage ? storage->length : 0;
	}

	~Bytes();

private:
	class ByteStorage {
	public:
		ByteStorage(size_t length) : refs(1), length(length) {}

		inline uint8_t* data() const { return (uint8_t*)&this[1]; }

		void operator --(int);

		std::atomic<int> refs;
		size_t length;
	};

	ByteStorage* storage;
};


#endif //CMEVBOT_BYTES_H
