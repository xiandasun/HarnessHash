#include "SpinLock.h"
#include <unordered_map>

template<typename T> class Dictionary {
	Spinlock lock;
	
	std::unordered_map<unsigned int, T *> internalHashMap;
	Dictionary(Dictionary const &);	// prevent copying
	void operator =(Dictionary const &);

  public:
	Dictionary() {};

	void put(unsigned int key, T *v) {
	  	lock.acquire();
		internalHashMap[key] = v;
		lock.release();
	}

	T *tryGet(unsigned int key) {
		lock.acquire();
		typename std::unordered_map<unsigned int, T *>::iterator ptr = internalHashMap.find(key);
		return ptr == internalHashMap.end() ? 0 : ptr->second;
	}
};

// Local Variables: //
// compile-command: "g++ -Wall -O3 -DNDEBUG -fno-reorder-functions -DPIN -DDictionary=SpinLockDictionary Harness.cc -lpthread -lm" //
// End: //
