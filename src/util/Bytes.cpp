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
#include "util/Bytes.h"

#include "string.h"
#include <new>


Bytes::Bytes(const char* s) {
	// We don't allocate space for the terminator, since we don't need it.
	size_t length = strlen(s);
	storage = (ByteStorage*)malloc(sizeof(ByteStorage) + length);
	new (storage) ByteStorage(length);
	memcpy((void*)&storage[1], s, length);
}

Bytes::Bytes(void* buffer, size_t length) {
	storage = (ByteStorage*)malloc(sizeof(ByteStorage) + length);
	new (storage) ByteStorage(length);
	memcpy((void*)&storage[1], buffer, length);
}

Bytes::Bytes(size_t length) {
	storage = (ByteStorage*)malloc(sizeof(ByteStorage) + length);
	new (storage) ByteStorage(length);
}

Bytes::Bytes(const Bytes& other) : storage(other.storage) {
	if (storage)
		storage->refs.fetch_add(1);
}

Bytes::Bytes(Bytes&& other) noexcept : storage(other.storage) {
	other.storage = nullptr;
}

Bytes& Bytes::operator=(Bytes&& other) noexcept {
	if (storage)
		(*storage)--;
	storage = other.storage;
	other.storage = nullptr;

	return *this;
}

Bytes& Bytes::operator=(const Bytes& other) {
	if (&other == this || other.storage == storage)
		return *this;

	if (storage)
		(*storage)--;

	storage = other.storage;
	storage->refs.fetch_add(1);

	return *this;
}


bool Bytes::operator>(const Bytes& other) const {
	if (storage == nullptr || other.storage == nullptr)
		return false;
	if (other.storage == storage)
		return false;

	size_t minLength = other.size() > size() ? size() : other.size();
	int memcmpResult = memcmp(&(*this)[0], &other[0], minLength);

	// If the initial memory is the same, then go by length.
	if (memcmpResult == 0)
		return size() > other.size();

	return memcmpResult > 0;
}

bool Bytes::operator==(const Bytes& other) const {
	if (storage == nullptr || other.storage == nullptr)
		return false;
	if (size() == other.size())
		return true;

	return memcmp(&(*this)[0], &other[0], size()) == 0;
}

Bytes::~Bytes() {
	if (storage) {
		(*storage)--;
	}
}

void Bytes::ByteStorage::operator--(int) {
	if (refs.fetch_sub(1) == 1) {
		this->~ByteStorage();
		free(this);
	}
}
