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

#ifndef CMEVBOT_CONSTARRAY_H
#define CMEVBOT_CONSTARRAY_H

#include "stdio.h"
#include "MonkeyIterator.h"
#include "Range.h"

#include <new>


template <class T>
class ConstArray {
public:
	ConstArray() : length(0), elements(nullptr) {}

	ConstArray(void* memory, int size) : length(size) {
		elements = (const T*)memory;
	}

	ConstArray(const ConstArray<T>& other) : length(other.length) {
		elements = (T*)malloc(other.length * sizeof(T));
		for (int i = 0; i < other.length; i++)
			new ((T*)&elements[i]) T(other.elements[i]);
	}

	ConstArray(ConstArray<T>&& other) noexcept: length(other.length), elements(other.elements) {
		other.elements = nullptr;
	}

	ConstArray& operator=(const ConstArray& other) {
		if (&other != this) {
			// Delete old data
			if (elements) {
				for (int i = 0; i < length; i++)
					elements[i].~T();
				free((void*)elements);
			}

			// Copy over new data
			*(int*)&length = other.length;
			elements = (T*)malloc(other.length * sizeof(T));
			for (int i = 0; i < other.length; i++)
				new ((T*)&elements[i]) T(other.elements[i]);
		}
		return *this;
	}

	ConstArray& operator=(ConstArray&& other) noexcept {
		// Delete old data
		if (elements) {
			for (int i = 0; i < length; i++)
				elements[i].~T();
			free((void*)elements);
		}

		// Assign new pointers
		elements = other.elements;
		*(int*)&length = other.length;

		// Invalidate old array
		other.elements = nullptr;

		return *this;
	}

	ConstArray<T> addZeros(int i) const {
		int newLength = length + i;
		T* newElements = (T*)malloc(newLength * sizeof(T));

		// Copy over new data
		for (int i = 0; i < length; i++)
			new (&newElements[i]) T(elements[i]);

		// Pad with empty values
		for (int i = length; i < newLength; i++)
			new (&newElements[i]) T();

		return ConstArray<T>(newElements, newLength);
	}

	inline int size() const { return length; }
	inline const T& get(int i) const {
		if (i >= length || i < 0)
			printf("Out of bounds read of %i for ConstArray of length %i\n", i, length);
		return elements[i];
	}

	const T* getMemory() const {
		return elements;
	}

	const T* begin() const { return elements; }
	const T* end() const { return &(elements[length]); }

	template<class E>
	Range<MonkeyIterator<T*, E>> rangeOf() const {
		return Range<MonkeyIterator<T*, E>>(
				MonkeyIterator<T*, E>(elements),
				MonkeyIterator<T*, E>(&elements[length])
		);
	}

	~ConstArray() {
		if (elements) {
			for (int i = 0; i < length; i++)
				elements[i].~T();
			free((void*)elements);
		}
	}

private:
	const int length;
	const T* elements;
};


#endif //CMEVBOT_CONSTARRAY_H
