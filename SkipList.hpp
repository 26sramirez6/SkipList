/*
 * SkipList.hpp
 *
 *  Created on: Oct 4, 2018
 *      Author: 26sra
 *
Skip Lists: http://cs.ipm.ac.ir/asoc2016/Resources/Theartofmulticore.pdf Chapter 14
Motivation: Randomized datastructure with logarithmic depth. A simplier alternative
	to red-black trees or AVL trees when implemented with concurrency.

Concurrent SkipList with fine-grain lazy locking (incomplete).
To prevent changes to the node predecessors while the
node is being added, add() locks the predecessors, validates
that the locked pre-decessors still refer to their
successors, then adds the node in a manner similar to the
sequential add().

To maintain the skiplist property, a node
is not considered to be logically in the set until all references
to it at all levels have been properly set. Each node has an
additional flag, fullyLinked,set to true once it has been linked
in all its levels. We do not allow access to a node until it is fully
linked, so for example, the add() method, when trying to determine
whether the node it wishes to add is already in the list, must
spin waiting for it to become fully linked.
 */

#ifndef SKIPLIST_HPP_
#define SKIPLIST_HPP_

#include <iostream>
#include <limits>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <bits/stdc++.h>
#include <pthread.h>

// skip list implemented
// through singly-linked lists on each level
template<typename T>
class SkipList {
public:
	// current size
	unsigned int size;

	SkipList(const float probability=0.5,
			 const unsigned int maxLevel=16);

	~SkipList();

	// O(lgn) insertion of
	// sorted element with arbitrary data.
	// fails if element is already in
	bool
	Insert(T data, int key);

	// O(lgn) removal of
	// sorted element.
	// fails if element is already in
	bool
	Remove(int key);

	// O(lgn) search of element
	bool
	Contains(int key) const;

	// Identical to Contains() for data retrieval.
	// assigns item to value if found and returns
	// true else false.
	bool
	Get(int key, T * item) const;

	// Print skip list for debugging
	void
	Print() const;

private:
	// skiplist node data structure
	typedef struct node_t {
		// node to the right
		node_t * right;
		// node below
		node_t * down;
		// arbitrary data
		T data;
		// sorting key
		int key;
		// marks nodes to be lazily deleted.
		// allows for lock-free search.
		volatile bool marked;
		// fully linked - node is traversable
		volatile bool fullyLinked;
		// fine-grain lock
		pthread_mutex_t mutex;
	} node_t;

	// draw a randomized height according to
	// the skiplist implementation
	unsigned int
	GetRandHeight() const;

	// helper method to construct sentinel nodes
	node_t *
	ConstructSentinel() const;

	// For fine grain locking, need to
	// retrieve the neighbor nodes and
	// lock
	int
	GetNodesThatNeedLocking(
	int key, std::vector<SkipList::node_t*> &nodes) const;

	// initialize listed vars::
	// probability
	const float p;
	// max levels
	const unsigned int ml;
	// tree height
	unsigned int height;
	// pointer to top level
	node_t * top;
	// pointer to bottom level
	node_t * bot;
};

#endif /* SKIPLIST_HPP_ */
