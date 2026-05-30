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

#ifndef CMEVBOT_FIBONACCIHEAP_H
#define CMEVBOT_FIBONACCIHEAP_H

#include "ds/ArrayList.h"


class FibonacciTree {
public:
	FibonacciTree(const void* data, size_t dataSize, double priority, size_t prioritySize);

	// Getters
	const void* getData() const;
	double getPriority() const;
	int getDegree() const;

	// Sibling manipulation (for root list)
	FibonacciTree* getLeft() const;
	FibonacciTree* getRight() const;
	void setLeft(FibonacciTree* left);
	void setRight(FibonacciTree* right);

	// Child manipulation
	FibonacciTree* getChild() const;
	void addChild(FibonacciTree* child);
	void setChild(FibonacciTree* child);

	FibonacciTree* getParent() const;
	void setParent(FibonacciTree* parent);

	void* getRaw();
	size_t getRawSize() const;

	bool isMarked() const;
	void setMarked(bool marked);

	// Priority comparison
	static bool lessThan(const FibonacciTree* a, const FibonacciTree* b, size_t prioritySize);

	~FibonacciTree();

private:
	void* data;
	double priority;
	size_t dataSize;
	size_t prioritySize;

	int degree;
	bool marked;

	FibonacciTree* parent;
	FibonacciTree* child;
	FibonacciTree* left;
	FibonacciTree* right;
};


class FibonacciHeapImpl {
public:
	FibonacciHeapImpl(int elementSize, int prioritySize);

	void insert(const void* data, double priority);
	void getMin(void* outData) const;
	double deleteMin(void* outData);

	bool isEmpty() const;
	int size() const;

	~FibonacciHeapImpl();

private:
	void consolidate();

	ArrayList<FibonacciTree*> roots;
	FibonacciTree* min;
	int count;

	const int dataSize;
	const int prioritySize;
};


template <class T, class P>
class FibonacciHeap {
public:
	FibonacciHeap(int expectedSize = 16)
			: impl(sizeof(T), sizeof(P)) {}

	void insert(const T& value, const P& priority) {
		impl.insert(&value, (double)priority);
	}

	T getMin() const {
		T out;
		impl.getMin(&out);
		return out;
	}

	T deleteMin() {
		T out;
		impl.deleteMin(&out);
		return out;
	}

	bool deleteMin(T* valOut, P* priorityOut) {
		if (impl.isEmpty())
			return false;

		*priorityOut = (P)impl.deleteMin(valOut);
		return true;
	}

	bool isEmpty() const {
		return impl.isEmpty();
	}

	int size() const {
		return impl.size();
	}

private:
	FibonacciHeapImpl impl;
};


#endif //CMEVBOT_FIBONACCIHEAP_H
