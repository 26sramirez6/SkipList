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
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <cstdlib>

#include "SkipList.hpp"
#define ADD_SIZE 15
#define REMOVE_SIZE 10
#define NUM_OPS_PER_THREAD 20
#define NUM_OF_THREADS 4
#define NUM_OF_LOOPS 10

// passed in thread constructor
typedef struct ThreadContext {
	int tid; // thread id
	int inserts; // # of insert operations
	int deletes; // # of delete operations
	int searches; // # search operations
	SkipList<int> * sList; // shared SkipList resource
} context_t;

// Thread code
static void * Worker(void * arg)
{
	context_t * c = static_cast<context_t *>(arg);
	int numOfOps = NUM_OPS_PER_THREAD;
	int * addlist = new int[numOfOps];
	if (addlist == NULL) {
		exit(-1);
	}

	for (int i=0; i<numOfOps; ++i) {
		addlist[i] = i + (c->tid * NUM_OPS_PER_THREAD);
		if (c->sList->Insert(i, addlist[i])) {
			++c->inserts;
		}
	}

	for (int i=0; i<numOfOps; ++i) {
		addlist[i] = i + (c->tid * NUM_OPS_PER_THREAD);
		if (c->sList->Contains(addlist[i])) {
			++c->inserts;
		}
	}
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

	delete addlist;
	return NULL;
}

// Concurrent tests
static void SpinThreads() {
	// the thread contexts can't be shared
	// or will incur race condition on counters
	context_t * c = new context_t[NUM_OF_THREADS];
	pthread_t * workers = new pthread_t[NUM_OF_THREADS];
	SkipList<int> * sList = new SkipList<int>();
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
		assert(pthread_create(&workers[i], NULL, Worker, &c[i])==0);
	}

	for (int i=0; i<NUM_OF_THREADS; i++) {
		pthread_join(workers[i], NULL);
	}

	for (int i=0; i<NUM_OF_THREADS; i++) {
		final_expected_ic += c[i].inserts;
		final_expected_dc += c[i].deletes;
		final_expected_sc += c[i].searches;
	}
	assert(final_expected_ic==NUM_OF_THREADS*NUM_OPS_PER_THREAD);
	assert(final_expected_sc==NUM_OF_THREADS*NUM_OPS_PER_THREAD);
	delete c;
	delete workers;
	delete sList;
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
	//sList->Print();

	int * sRemovals = new int[REMOVE_SIZE];
	for (int i=0; i<REMOVE_SIZE; ++i) {
		sRemovals[i] = rand()%ADD_SIZE;
	}
	unsigned int trueSize = sList.size;
	for (int i=0; i<REMOVE_SIZE; ++i) {
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
	delete [] sRemovals;
}

int main()
{
	srand(static_cast<unsigned int>(time(NULL)));
	std::cout << "Beginning Sequential SkipList tests..." << std::endl;
	for (int i=0; i<NUM_OF_LOOPS; ++i) {
		SequentialTests();
	}
	std::cout << "Successful completion of Sequential SkipList tests" << std::endl;
	std::cout << "Beginning Concurrent SkipList tests..." << std::endl;
	for (int i=0; i<NUM_OF_LOOPS; ++i) {
		SpinThreads();
	}
	std::cout << "Successful completion of Concurrent SkipList tests..." << std::endl;
	return 0;
}
