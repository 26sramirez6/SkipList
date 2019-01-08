/*
 * test_ds.cpp
 *
 *  Created on: Oct 5, 2018
 *      Author: 26sra
 *
 *      Driver for executing simple sequential and
 *      concurrent tests against SkipList
 */

#include <iostream>
#include <cstdlib>
#include <thread>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "SkipList.hpp"

constexpr int ADD_SIZE = 15;
constexpr int REMOVE_SIZE = 10;
constexpr int NUM_OPS_PER_THREAD = 20;
constexpr int NUM_OF_THREADS = 4;
constexpr int NUM_OF_LOOPS = 10;

// passed in thread constructor
struct ThreadContext {
	int tid; // thread id
	int inserts; // # of insert operations
	int deletes; // # of delete operations
	int searches; // # search operations
	std::shared_ptr<SkipList<int>> sList; // shares a single SkipList
};

// Thread code
static void Worker(ThreadContext& context)
{
	printf("Thread %d processing initialized. \n", context.tid);
	std::unique_ptr<int[]> addList { new int[NUM_OPS_PER_THREAD]() };
	if (addList == nullptr) {
		exit(-1);
	}

	for (int i=0; i<NUM_OPS_PER_THREAD; ++i) {
		addList[i] = i + (context.tid * NUM_OPS_PER_THREAD);
		if (context.sList->Insert(i, addList[i])) {
			++context.inserts;
		}
	}

	for (unsigned int i=0; i<NUM_OPS_PER_THREAD; ++i) {
		addList[i] = i + (context.tid * NUM_OPS_PER_THREAD);
		if (context.sList->Contains(addList[i])) {
			++context.searches;
		}
	}
	printf("Thread %d processing complete, exiting. \n", context.tid);
//	std::cout << "Thread " << context.tid << "completing process" <<  std::cend;
	//TODO: uncomment below once fine-grain lock complete on Remove()
//	i = 0;
//	while (i < numOfOps) {
//		if (i % 3 == 0) {
//		  c->sList->Delete(&l, addlist[i]);
//		  (c->deletes)++;
//		}
//		i++;
//	}
//
//	i = 0;
//	while (i < numOfOps) {
//		if (c->sList->Contains(i))
//		  (c->searches)++;
//		i++;
//	}
}

// Spin threads for concurrent tests
static void ParallelTests() {
	// the thread contexts can't be shared
	// or will incur race condition on counters
	ThreadContext c[NUM_OF_THREADS];
	std::thread workers[NUM_OF_THREADS];
	std::shared_ptr<SkipList<int>> sList { new SkipList<int> };

	int final_expected_count = 0;
	int final_expected_sc = 0;
	int final_expected_ic = 0;
	int final_expected_dc = 0;


	for (int i=0; i<NUM_OF_THREADS; ++i) {
		c[i].tid = i;
		c[i].inserts = 0;
		c[i].deletes = 0;
		c[i].searches = 0;
		c[i].sList = sList;
		workers[i] = std::thread(Worker, std::ref(c[i]));
	}

	for (int i=0; i<NUM_OF_THREADS; i++) {
		workers[i].join();
	}

	for (int i=0; i<NUM_OF_THREADS; i++) {
		final_expected_ic += c[i].inserts;
		final_expected_dc += c[i].deletes;
		final_expected_sc += c[i].searches;
	}
	assert(final_expected_ic==NUM_OF_THREADS*NUM_OPS_PER_THREAD);
	assert(final_expected_sc==NUM_OF_THREADS*NUM_OPS_PER_THREAD);
}

// Sequential tests
static void SequentialTests () {
	SkipList<int> sList = SkipList<int>(.5, 19);
	for (int i=0; i<ADD_SIZE; ++i) {
		std::cout << "------Inserting " << i << "------" << std::endl;
		sList.Insert(rand()%ADD_SIZE, i);
		sList.Print();
	}
	assert(sList.size==ADD_SIZE);
	std::unique_ptr<int[]> sRemovals { new int[REMOVE_SIZE]() };
	for (unsigned int i=0; i<REMOVE_SIZE; ++i) {
		sRemovals[i] = rand()%ADD_SIZE;
	}
	unsigned int trueSize = sList.size;
	for (unsigned int i=0; i<REMOVE_SIZE; ++i) {
		std::cout << "------Removing " << sRemovals[i] << "------" << std::endl;
		if (sList.Contains(sRemovals[i])) {
			assert(sList.Remove(sRemovals[i]));
			--trueSize;
		} else {
			sList.Contains(sRemovals[i]);
			assert(!sList.Remove(sRemovals[i]));
		}
		sList.Print();
		assert(!sList.Contains(sRemovals[i]));
	}
	assert(trueSize==sList.size);
}

int main()
{
	srand(static_cast<unsigned int>(time(nullptr)));
	std::cout << "Beginning Sequential SkipList tests..." << std::endl;
	for (int i=0; i<NUM_OF_LOOPS; ++i) {
		SequentialTests();
	}
	std::cout << "Successful completion of Sequential SkipList tests." << std::endl;
	std::cout << "Beginning Concurrent SkipList tests..." << std::endl;
	for (int i=0; i<NUM_OF_LOOPS; ++i) {
		ParallelTests();
	}
	std::cout << "Successful completion of Concurrent SkipList tests." << std::endl;
	return 0;
}
