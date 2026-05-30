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

#include "gtest/gtest.h"
#include "util/FibonacciHeap.h"


// Test 1: Insert one element and check min
TEST(FibonacciHeapTest, SingleInsert) {
	FibonacciHeap<int, int> heap;
	heap.insert(42, 5);
	EXPECT_EQ(heap.getMin(), 42);
	EXPECT_EQ(heap.size(), 1);
	EXPECT_FALSE(heap.isEmpty());
}

// Test 2: Insert multiple with different priorities
TEST(FibonacciHeapTest, MinCorrectness) {
	FibonacciHeap<int, int> heap;
	heap.insert(10, 100);
	heap.insert(20, 50);
	heap.insert(30, 25); // This should become the min

	EXPECT_EQ(heap.getMin(), 30);
	EXPECT_EQ(heap.size(), 3);
}

// Test 3: Delete min and get next min
TEST(FibonacciHeapTest, DeleteMin) {
	FibonacciHeap<int, int> heap;
	heap.insert(10, 100);   // P = 100
	heap.insert(20, 50);    // P = 50
	heap.insert(30, 25);    // P = 25

	int min = heap.deleteMin();  // should remove 30
	EXPECT_EQ(min, 30);
	EXPECT_EQ(heap.getMin(), 20); // next smallest priority
	EXPECT_EQ(heap.size(), 2);
}

// Test 4: Insert in reverse priority order
TEST(FibonacciHeapTest, ReverseInsertOrder) {
	FibonacciHeap<int, int> heap;

	heap.insert(1, 300);
	heap.insert(2, 200);
	heap.insert(3, 100);

	EXPECT_EQ(heap.getMin(), 3);
	int val = heap.deleteMin();
	EXPECT_EQ(val, 3);
	EXPECT_EQ(heap.getMin(), 2);
	val = heap.deleteMin();
	EXPECT_EQ(val, 2);
	EXPECT_EQ(heap.getMin(), 1);
}

// Test 5: Insert multiple elements with same priority
TEST(FibonacciHeapTest, EqualPriority) {
	FibonacciHeap<int, int> heap;
	heap.insert(1, 50);
	heap.insert(3, 50);
	heap.insert(2, 50);

	EXPECT_TRUE(heap.getMin() == 1 || heap.getMin() == 2 || heap.getMin() == 3);
	EXPECT_EQ(heap.size(), 3);
}

// Test 6: Delete all elements
TEST(FibonacciHeapTest, DeleteAll) {
	FibonacciHeap<int, int> heap;
	heap.insert(5, 5);
	heap.insert(3, 3);
	heap.insert(2, 2);
	heap.insert(9, 9);
	heap.insert(8, 8);
	heap.insert(-20, -20);
	heap.insert(-10, -10);

	EXPECT_EQ(heap.deleteMin(), -20);
	EXPECT_EQ(heap.deleteMin(), -10);
	EXPECT_EQ(heap.deleteMin(), 2);
	EXPECT_EQ(heap.deleteMin(), 3);
	EXPECT_EQ(heap.deleteMin(), 5);
	EXPECT_EQ(heap.deleteMin(), 8);
	EXPECT_EQ(heap.deleteMin(), 9);
	EXPECT_TRUE(heap.isEmpty());
}

// Test 7: Large number of inserts
TEST(FibonacciHeapTest, ManyInserts) {
	FibonacciHeap<int, int> heap;

	const int count = 100;
	for (int i = 0; i < count; ++i) {
		heap.insert(i, count - i); // smallest priority = largest i
	}
	EXPECT_EQ(heap.getMin(), 99);

	for (int i = 99; i >= 0; --i) {
		int val = heap.deleteMin();
		EXPECT_EQ(val, i);
	}
	EXPECT_TRUE(heap.isEmpty());
}

// Basic test with 1 million inserts and deletes
/*TEST(FibonacciHeapTest, InsertDeleteMillion) {
	const int N = 1000000;

	FibonacciHeap<int, double> heap;
	for (int i = 0; i < N; i++) {
		heap.insert(i, N - i - 1);
	}

	EXPECT_EQ(heap.size(), N);

	// All deletes should return elements in increasing order
	for (int i = 0; i < N; i++) {
		int val = heap.deleteMin();
		EXPECT_EQ(val, N - i - 1);
	}
	EXPECT_TRUE(heap.isEmpty());
}*/

// Test randomized priority values
TEST(FibonacciHeapTest, RandomPrioritiesCorrectness) {
	const int N = 10000;
	FibonacciHeap<int, double> heap;
	ArrayList<int> inserted;
	ArrayList<double> priorities;

	srand(42);
	for (int i = 0; i < N; i++) {
		int val = i;
		double prio = (double)(rand() % 100000);
		heap.insert(val, prio);
		inserted.add(val);
		priorities.add(prio);
	}

	// Find min manually
	int expectedMin = inserted.get(0);
	double minPrio = priorities.get(0);
	for (int i = 1; i < inserted.size(); i++) {
		if (priorities.get(i) < minPrio) {
			expectedMin = inserted.get(i);
			minPrio = priorities.get(i);
		}
	}

	EXPECT_EQ(heap.getMin(), expectedMin);
	EXPECT_EQ(heap.size(), N);
}

// Test inserting sorted, then verifying min and order
TEST(FibonacciHeapTest, SortedInsertDescending) {
	const int N = 10000;
	FibonacciHeap<int, double> heap;

	for (int i = N; i > 0; i--) {
		heap.insert(i, (double)i);
	}

	for (int i = 1; i <= N; i++) {
		EXPECT_EQ(heap.deleteMin(), i);
	}
	EXPECT_TRUE(heap.isEmpty());
}

// Timing test
/*TEST(FibonacciHeapTest, PerformanceInsertAndDelete) {
	const int N = 200000;
	FibonacciHeap<int, double> heap;

	uint64_t start = millis_since_epoch();

	for (int i = 0; i < N; i++)
		heap.insert(i, rand());

	for (int i = 0; i < N; i++)
		heap.deleteMin();

	uint64_t end = millis_since_epoch();
	uint64_t elapsed = end - start;

	printf("Time taken for %i elements: %lums (%fms per element)\n", N, elapsed, (double)elapsed / N);
}*/
