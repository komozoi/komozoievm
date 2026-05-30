// Copyright 2025-2026 komozoi
// Original Creation Date: 2025-08-12
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

#include "util/FibonacciHeap.h"


FibonacciTree::FibonacciTree(const void* data, size_t dataSize, double priority, size_t prioritySize)
		: dataSize(dataSize), prioritySize(prioritySize), degree(0), marked(false),
		  parent(nullptr), child(nullptr)
{
	this->data = malloc(dataSize);
	this->priority = priority;
	memcpy(this->data, data, dataSize);
	left = right = this;
}

const void* FibonacciTree::getData() const { return data; }
double FibonacciTree::getPriority() const { return priority; }
int FibonacciTree::getDegree() const { return degree; }

FibonacciTree* FibonacciTree::getLeft() const { return left; }
FibonacciTree* FibonacciTree::getRight() const { return right; }
void FibonacciTree::setLeft(FibonacciTree* l) { left = l; }
void FibonacciTree::setRight(FibonacciTree* r) { right = r; }

FibonacciTree* FibonacciTree::getChild() const { return child; }
void FibonacciTree::setChild(FibonacciTree* c) { child = c; }

FibonacciTree* FibonacciTree::getParent() const { return parent; }
void FibonacciTree::setParent(FibonacciTree* p) { parent = p; }

void* FibonacciTree::getRaw() { return data; }
size_t FibonacciTree::getRawSize() const { return dataSize; }

bool FibonacciTree::isMarked() const { return marked; }
void FibonacciTree::setMarked(bool m) { marked = m; }

void FibonacciTree::addChild(FibonacciTree* node) {
	if (!child) {
		child = node;
		node->left = node->right = node;
	} else {
		FibonacciTree* c = child;
		node->left = c;
		node->right = c->right;
		c->right->left = node;
		c->right = node;
	}
	node->setParent(this);
	degree++;
}

bool FibonacciTree::lessThan(const FibonacciTree* a, const FibonacciTree* b, size_t prioritySize) {
	return a->priority < b->priority;
}



FibonacciTree::~FibonacciTree() {
	// Delete all children
	FibonacciTree* child = getChild();
	if (child) {
		FibonacciTree* curr = child;
		do {
			FibonacciTree* next = curr->getRight();
			delete curr;
			curr = next;
		} while (curr != child);
	}

	// Unlink node completely
	setLeft(nullptr);
	setRight(nullptr);
	setChild(nullptr);
	setParent(nullptr);

	free(data);
}


FibonacciHeapImpl::FibonacciHeapImpl(int elementSize, int prioritySize)
		: min(nullptr), count(0), dataSize(elementSize), prioritySize(prioritySize) {}

void FibonacciHeapImpl::insert(const void* data, double priority) {
	FibonacciTree* node = new FibonacciTree(data, dataSize, priority, prioritySize);
	roots.add(node);
	if (!min || FibonacciTree::lessThan(node, min, prioritySize))
		min = node;
	count++;
}

void FibonacciHeapImpl::getMin(void* outData) const {
	if (!min) return;
	memcpy(outData, min->getData(), dataSize);
}

double FibonacciHeapImpl::deleteMin(void* outData) {
	if (!min)
		return 0;

	memcpy(outData, min->getData(), dataSize);
	double priority = min->getPriority();

	// Move all children to root list
	FibonacciTree* child = min->getChild();
	if (child) {
		FibonacciTree* start = child;
		do {
			FibonacciTree* next = child->getRight();
			child->setParent(nullptr);
			child->setLeft(nullptr);
			child->setRight(nullptr);
			roots.add(child);
			child = next;
		} while (child != start);
	}

	min->setChild(nullptr);

	// Remove min from root list
	for (int i = 0; i < roots.size(); i++) {
		if (roots.get(i) == min) {
			roots.unorderedRemove(i);
			break;
		}
	}

	delete min;
	min = nullptr;
	count--;

	consolidate();

	return priority;
}

bool FibonacciHeapImpl::isEmpty() const {
	return count == 0;
}

int FibonacciHeapImpl::size() const {
	return count;
}

void FibonacciHeapImpl::consolidate() {
	if (count == 0)
		return;

	int maxDegree = (int)(log2(count)) + 2;
	ArrayList<FibonacciTree*> aux(maxDegree);
	aux.addCopies(nullptr, maxDegree);

	for (int i = 0; i < roots.size(); i++) {
		FibonacciTree* x = roots.get(i);
		int d = x->getDegree();
		while (aux.get(d)) {
			FibonacciTree* y = aux.get(d);
			if (FibonacciTree::lessThan(y, x, prioritySize)) {
				FibonacciTree* temp = x;
				x = y;
				y = temp;
			}
			x->addChild(y);
			aux.set(d, nullptr);
			d++;
		}
		aux.set(d, x);
	}

	roots.clear();
	min = nullptr;
	for (int i = 0; i < aux.size(); i++) {
		FibonacciTree* t = aux.get(i);
		if (t) {
			roots.add(t);
			if (!min || FibonacciTree::lessThan(t, min, prioritySize))
				min = t;
		}
	}
}

FibonacciHeapImpl::~FibonacciHeapImpl() {
	for (FibonacciTree* tree: roots)
		delete tree;

	// We don't delete min because it is already present in roots
	min = nullptr;
}

