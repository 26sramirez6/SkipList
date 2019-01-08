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
#include <thread>
#include <mutex>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <bits/stdc++.h>

// skip list implemented
// through singly-linked lists on each level
template<typename T>
class SkipList {
public:
	// current size
	unsigned int size;

	// constructs top and bot sentinel nodes
	// and seeds the RNG
	SkipList(const float probability=0.5, const unsigned int maxLevel=16) :
			size(0), p(probability), ml(maxLevel), height(0) {
		srand(static_cast<unsigned int>(time(nullptr)));
		// since we cant pass in a nullptr for T
		T defaultValue;
		// top sentinel node
		this->top = this->ConstructSentinel();
		// bot sentinel node
		this->bot = this->ConstructSentinel();
	}

	// destroys nodes structures
	// top to bottom. Caller responsible for
	// deallocating data pointers
	~SkipList() {
		node_t * level = this->top;
		node_t * nextLevel = nullptr;
		while (level != nullptr) {
			nextLevel = level->down;
			node_t * current = level;
			node_t * prev = nullptr;
			while (current != nullptr) {
				prev = current;
				current = current->right;
				delete prev;
			}
			level = nextLevel;
		}
		delete this->bot;
	}

	// O(lgn) insertion of
	// sorted element with arbitrary data.
	// fails if element is already in
	// TODO: needs Backoff() mechanism to fix
	// deadlocks when all threads sleep
	// from cyclical locks.
	bool
	Insert(T data, int key) {
		unsigned int randHeight = this->GetRandHeight();
		node_t * lastInsert = nullptr;
		node_t * newInsert = nullptr;
		std::vector<SkipList::node_t*> nodesToGetLocked;
		if (randHeight>this->height) {
			// construct a new level.
			// first handle the sentinel
			if (this->height>0) {
				node_t * sentinel = this->ConstructSentinel();
				sentinel->down = this->top;
				this->top->mutex.lock();
				// critical section for updating top
				// node pointer
				this->top = sentinel;
				this->top->mutex.unlock();
			}
			++this->height;
		}

		// if a level is returned, then a node with
		// the same key is in process of being added
		// by another thread so return false. So
		// (slightly unintuitive) we want the returned level
		// to be -1
		if (this->GetNodesThatNeedLocking(key, nodesToGetLocked)!=-1) {
			return false;
		}

		// lock the target nodes.
		// TODO: Need a backoff() mechanism
		// to release lock if deadlock ensues
		for (unsigned int i=0; i<nodesToGetLocked.size(); ++i) {
			nodesToGetLocked[i]->mutex.lock();
		}
		// To maintain the skiplist property, a node
		// is not considered to be logically in the set until all references
		// to it at all levels have been properly set
		// ******ENTER CRITICAL SECTION******
		node_t * current = this->top;
		for (unsigned int i=this->height;current!=nullptr;--i) {
			// go as right as possible until the next
			// node is greater than the key
			while (current->right != nullptr &&
					key > current->right->key) {
				current = current->right;
			}
			// so we are in the right spot to go down now.
			// if the rand height is currently greater than i
			// then insert the new node
			if (i<=randHeight) {
				newInsert = this->ConstructNode(current->right, nullptr, key, data);

				// current right is now the new insert
				current->right = newInsert;

				// if we performed an insert on the previous
				// level, then set that node's down pointer
				// to the new node we just created.
				if (lastInsert!=nullptr) {
					lastInsert->down = newInsert;
				}
				lastInsert = newInsert;
			}
			current = current->down;
		}
		// *******EXIT CRITICAL SECTION*******
		for (unsigned int i=0; i<nodesToGetLocked.size(); ++i) {
			nodesToGetLocked[i]->mutex.unlock();
		}
		++this->size;
		return true;
	}

	// O(lgn) removal time
	// TODO: Need to implement lazy deletion using node.marked
	// for optimistic synchronization. Currently not thread-safe.
	bool
	Remove(int key) {
		node_t * newRemove = nullptr;
		node_t * current = this->top;
		bool ret = false;
		unsigned int reduce = 0;
		while (current!=nullptr) {
			// go as right as possible until the next
			// node is greater than the key
			while (current->right != nullptr &&
					key > current->right->key) {
				current = current->right;
			}
			// if the next is key, we can remove everything down,
			// but its also possible that we've iterated to the end
			// of the current level and the next node is just nullptr
			// so short circuit check first
			if (current->right != nullptr && current->right->key==key) {
				// we are okay to check the 2nd right because
				// its confirmed the one to the right is the removal
				// target. If there is nothing to the right of
				// the removal target then this whole level needs to
				// be removed
				if (this->top==current && current->right->right==nullptr) {
					reduce = 1;
				}
				newRemove = current->right;
				current->right = newRemove->right;
				assert(newRemove!=nullptr);
				delete newRemove;
				ret = true;
			}
			current = current->down;
		}
		// also remove the top sentinel
		// and bring it down 1 level
		// no need to reduce size here since
		// its just a sentinel
		if (reduce) {
			newRemove = this->top;
			this->top = this->top->down;
			delete newRemove;
		}
		this->height = std::max(unsigned(1), this->height-reduce);
		if (ret) --this->size;
		return ret;
	}

	// O(lgn) search of element
	bool
	Contains(int key) const {
		node_t * current = this->top;
		while (current != nullptr) {
			while (current->right != nullptr && key > current->right->key) {
				current = current->right;
			}
			if (current->right != nullptr &&
				current->right->key == key) {
				return true;
			}
			current = current->down;
		}
		return false;
	}

	// Identical to Contains() for data retrieval.
	// assigns item to value if found and returns
	// true else false.
	// TODO: Implement Finger Search to benefit
	// from spatial/temporal locality
	bool
	Get(int key, T* item) const {
		node_t * current = this->top;
		while (current != nullptr) {
			while (current->right != nullptr && key > current->right->key) {
				current = current->right;
			}
			if (current->right != nullptr &&
				current->right->key == key) {
				*item = current->right->data;
				return true;
			}
			current = current->down;
		}
		return false;
	}

	// Print skip list for debugging
	void
	Print() const {
		printf("SkipList<%d, %d>:\n", this->height, this->size);
		node_t * next = this->top;
		while (next != nullptr) {
			node_t * current = next->right;
			// go as right as possible until the next
			// node is greater than the key
			printf("-inf");
			while (current != nullptr) {
				printf("--->%d", current->key);
				current = current->right;
			}
			putchar('\n');
			next = next->down;
		}
		putchar('\n');
	}

private:
	// skiplist node data structure
	struct node_t {
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
//		volatile bool marked;
		// fully linked - node is traversable
//		volatile bool fullyLinked;
		// fine-grain lock
		std::mutex mutex;
	};

	// draw a randomized height according to
	// the skiplist implementation. Called
	// on every Insert(). Can't go past 1 + height
	unsigned int
	GetRandHeight() const {
		unsigned int ret = 1;
		while (this->p>rand()/(RAND_MAX + 1.0) &&
				ret<this->ml &&
				ret<this->height+1) {
			++ret;
		}
		return ret;
	}

	// constructs node
	node_t *
	ConstructNode(node_t * right, node_t * down, int key, T data) const {
		node_t * ret = new node_t();
		ret->right = right;
		ret->down = down;
		ret->key = key;
		ret->data = data;
//		ret->marked = false;
		return ret;
	}

	// constructs sentinel node,
	// leaving data element uninititialized
	node_t *
	ConstructSentinel() const {
		node_t * ret = new node_t();
		ret->right = nullptr;
		ret->down = nullptr;
		ret->key = std::numeric_limits<int>::min();
//		ret->marked = false;
		return ret;
	}

	// For fine grain locking, need to
	// retrieve the neighbor nodes and
	// lock. the predecessors
	// and successors of the insert nodes of
	// each level need to be locked during
	// the insertion/removal so this populates
	// the vector with those nodes.
	int
	GetNodesThatNeedLocking(int key,
			std::vector<SkipList::node_t*> &nodes) const {
		int ret = -1;
		node_t * current = this->top;
		for (unsigned int lvl=this->ml; current != nullptr; --lvl) {
			while (current->right != nullptr && key > current->right->key) {
				current = current->right;
			}
			// TODO: pushing back nodes from
			// ALL levels. Most of the time a new
			// insert doesn't even go to the top level,
			// so those nodes don't need to be locked. Needs fixing.
			nodes.push_back(current);

			if (current->right != nullptr) {
				if (current->right->key==key && ret==-1) {
					ret = static_cast<int>(lvl);
				}
				if (current->right->right != nullptr) {
					nodes.push_back(current->right->right);
				}
			}
			current = current->down;
		}
		return ret;
	}

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
