#include "SpinLock.h"
#include <unordered_map>

template<typename T> class FairDictionary {
	Spinlock mutexEntry, mutexCounter, mutexAccess;
	unsigned int readercount;
	
	std::unordered_map<unsigned int, T *> internalHashMap;
	Dictionary(Dictionary const &);	// prevent copying
	void operator =(Dictionary const &);

  public:
	Dictionary() : mutexEntry(), mutexCounter(), mutexAccess(), readercount(0) {};

	void put(unsigned int key, T *v) {
		mutexEntry.acquire();
		mutexAccess.acquire();
		mutexEntry.release();	

		internalHashMap[key] = v;

		mutexAccess.release();
	}

	T *tryGet(unsigned int key) {
		mutexEntry.acquire();
		mutexCounter.acquire();
		readercount++;
		if (readercount == 1) mutexAccess.acquire();
		mutexCounter.release();
		mutexEntry.release();

		typename std::unordered_map<unsigned int, T *>::iterator ptr = internalHashMap.find(key);
		
		mutexCounter.acquire();
		readercount--;
		if (readercount == 0) mutexAccess.release();
		mutexCounter.release();

		return ptr == internalHashMap.end() ? 0 : ptr->second;
	}
};

// Local Variables: //
// compile-command: "g++ -Wall -O3 -DNDEBUG -fno-reorder-functions -DPIN -DDictionary=FairDictionary Harness.cc -lpthread -lm" //
// End: //
