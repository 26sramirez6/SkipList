/*
 * SkipList2.cpp
 *
 *  Created on: Oct 4, 2018
 *      Author: 26sra
 *
 *      SkipList Fine-Grain Locking Implementation
 */

#include "SkipList.hpp"

#define MAX(a,b) ((a) > (b) ? (a) : (b))

// constructs top and bot sentinel nodes
// and seeds the RNG
template<typename T>
SkipList<T>::SkipList(const float probability, const unsigned int maxLevel) :
		size(0), p(probability), ml(maxLevel), height(0) {
	srand(static_cast<unsigned int>(time(NULL)));
	// top sentinel node
	this->top = this->ConstructSentinel();
	//this->top->down = this->bot;
	this->bot = this->ConstructSentinel();
}

// destroys nodes structures
// top to bottom. Caller responsible for
// deallocating data pointers
template<typename T>
SkipList<T>::~SkipList() {
	node_t * level = this->top;
	node_t * nextLevel = NULL;
	while (level != NULL) {
		nextLevel = level->down;
		node_t * current = level;
		node_t * prev = NULL;
		while (current != NULL) {
			prev = current;
			current = current->right;
			delete prev;
		}
		level = nextLevel;
	}
	delete this->bot;
}

// builds sentinel nodes used at every level
template<typename T>
typename SkipList<T>::node_t *
SkipList<T>::ConstructSentinel() const {
	node_t * ret = new node_t();
	ret->key = std::numeric_limits<int>::min();
	ret->down = NULL;
	ret->right = NULL;
	ret->marked = false;
	pthread_mutex_init(&ret->mutex, NULL);
	return ret;
}

// Scans for item in O(lgn)
template<typename T>
bool
SkipList<T>::Contains(int key) const {
	node_t * current = this->top;
	while (current != NULL) {
		while (current->right != NULL && key > current->right->key) {
			current = current->right;
		}
		if (current->right != NULL &&
			current->right->key == key) {
			return true;
		}
		current = current->down;
	}
	return false;
}

// Identical to Contains, populates item if
// found and returns true, else false.
// TODO: Implement Finger Search to benefit
// from spatial/temporal locality
template<typename T>
bool
SkipList<T>::Get(int key, T* item) const {
	node_t * current = this->top;
	while (current != NULL) {
		while (current->right != NULL && key > current->right->key) {
			current = current->right;
		}
		if (current->right != NULL &&
			current->right->key == key) {
			*item = current->right->data;
			return true;
		}
		current = current->down;
	}
	return false;
}

// Generate randomized height. Called
// on every Insert(). Can't go past 1 + height
template<typename T>
unsigned int
SkipList<T>::GetRandHeight() const {
	unsigned int ret = 1;
	while (this->p>rand()/(RAND_MAX + 1.0) &&
			ret<this->ml &&
			ret<this->height+1) {
		++ret;
	}
	return ret;
}

// TODO: needs Backoff() mechanism to fix
// deadlocks when all threads sleep
// from cyclical locks.
template<typename T>
bool
SkipList<T>::Insert(T data, int key) {
	unsigned int randHeight = this->GetRandHeight();
	node_t * lastInsert = NULL;
	node_t * newInsert = NULL;
	std::vector<SkipList::node_t*> nodesToGetLocked;
	if (randHeight>this->height) {
		// construct a new level.
		// first handle the sentinel
		if (this->height>0) {
			node_t * sentinel = this->ConstructSentinel();
			sentinel->down = this->top;
			pthread_mutex_lock(&this->top->mutex);
			// critical section for updating top
			// node pointer
			this->top = sentinel;
			pthread_mutex_unlock(&this->top->mutex);
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
		pthread_mutex_lock(&nodesToGetLocked[i]->mutex);
	}
	// To maintain the skiplist property, a node
	// is not considered to be logically in the set until all references
	// to it at all levels have been properly set
	// ******ENTER CRITICAL SECTION******
	// ******ENTER CRITICAL SECTION******
	// ******ENTER CRITICAL SECTION******
	node_t * current = this->top;
	for (unsigned int i=this->height;current!=NULL;--i) {
		// go as right as possible until the next
		// node is greater than the key
		while (current->right != NULL &&
				key > current->right->key) {
			current = current->right;
		}
		// so we are in the right spot to go down now.
		// if the rand height is currently greater than i
		// then insert the new node
		if (i<=randHeight) {
			//a node needs to be inserted on this level
			newInsert = new node_t();
			// its right pointer is the next one on the right
			newInsert->right = current->right;
			// give it the key
			newInsert->key = key;
			// data member
			newInsert->data = data;
			// initialize lock
			pthread_mutex_init(&newInsert->mutex, NULL);
			// current right is now the new insert
			current->right = newInsert;
			// always set the new node's down pointer
			// to NULL since it has to be set on the
			// next iteration when we move down a level
			// and duplicate this node.
			newInsert->down = NULL;
			// if we performed an insert on the previous
			// level, then set that node's down pointer
			// to the new node we just created.
			if (lastInsert!=NULL) {
				lastInsert->down = newInsert;
			}
			lastInsert = newInsert;
		}
		current = current->down;
	}
	// *******EXIT CRITICAL SECTION*******
	// *******EXIT CRITICAL SECTION*******
	// *******EXIT CRITICAL SECTION*******
	for (unsigned int i=0; i<nodesToGetLocked.size(); ++i) {
		pthread_mutex_unlock(&nodesToGetLocked[i]->mutex);
	}
	++this->size;
	return true;
}

// O(lgn) removal time
// TODO: Need to implement lazy deletion using node.marked
// for optimistic synchronization. Currently not thread-safe.
template<typename T>
bool
SkipList<T>::Remove(int key) {
	node_t * newRemove = NULL;
	node_t * current = this->top;
	bool ret = false;
	unsigned int reduce = 0;
	while (current!=NULL) {
		// go as right as possible until the next
		// node is greater than the key
		while (current->right != NULL &&
				key > current->right->key) {
			current = current->right;
		}
		// if the next is key, we can remove everything down,
		// but its also possible that we've iterated to the end
		// of the current level and the next node is just NULL
		// so short circuit check first
		if (current->right != NULL && current->right->key==key) {
			// we are okay to check the 2nd right because
			// its confirmed the one to the right is the removal
			// target. If there is nothing to the right of
			// the removal target then this whole level needs to
			// be removed
			if (this->top==current && current->right->right==NULL) {
				reduce = 1;
			}
			newRemove = current->right;
			current->right = newRemove->right;
			assert(newRemove!=NULL);
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
	this->height = MAX(1, this->height-reduce);
	if (ret) --this->size;
	return ret;
}

// for fine grain locking, the predecessors
// and successors of the insert nodes of
// each level need to be locked during
// the insertion/removal so this populates
// the vector with those nodes.
template<typename T>
int
SkipList<T>::GetNodesThatNeedLocking(int key,
		std::vector<SkipList::node_t*> &nodes) const {
	int ret = -1;
	node_t * current = this->top;
	for (unsigned int lvl=this->ml; current != NULL; --lvl) {
		while (current->right != NULL && key > current->right->key) {
			current = current->right;
		}
		// TODO: pushing back nodes from
		// ALL levels. Most of the time a new
		// insert doesn't even go to the top level,
		// so those nodes don't need to be locked. Needs fixing.
		nodes.push_back(current);

		if (current->right != NULL) {
			if (current->right->key==key && ret==-1) {
				ret = static_cast<int>(lvl);
			}
			if (current->right->right != NULL) {
				nodes.push_back(current->right->right);
			}
		}
		current = current->down;
	}
	return ret;
}

template<typename T>
void
SkipList<T>::Print() const {
	printf("SkipList<%d, %d>:\n", this->height, this->size);
	node_t * next = this->top;
	while (next != NULL) {
		node_t * current = next->right;
		// go as right as possible until the next
		// node is greater than the key
		printf("-inf");
		while (current != NULL) {
			printf("--->%d", current->key);
			current = current->right;
		}
		printf("\n");
		next = next->down;
	}
	printf("\n");
}

// explicit instantiation for tests
template class SkipList<int>;
