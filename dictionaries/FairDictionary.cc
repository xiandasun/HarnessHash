#include "Semaphore.h"
#include <unordered_map>

template<typename T> class FairDictionary {
	Semaphore mutexEntry, mutexCounter, mutexAccess;
	unsigned int readercount;
	
	std::unordered_map<unsigned int, T *> internalHashMap;
	Dictionary(Dictionary cont &);	// prevent copying
	void operator =(Dictionary const &);

  public:
	Dictionary() : mutexEntry(1), mutexCounter(1), mutexAccess(1), readercount(0) {};

	void put(unsigned int key, T *v) {
		mutexEntry.wait();
		mutexAccess.wait();
		mutexEntry.signal();	

		internalHashMap[key] = v;

		mutexAccess.signal();
	}

	T *tryGet(unsigned int key) {
		mutexEntry.wait();
		mutexCounter.wait();
		readercount++;
		if (readercount == 1) mutexAccess.wait();
		mutexCounter.signal();
		mutexEntry.signal();

		std::unordered_map::iterator ptr = internalHashMap.find(key);
		
		mutexCounter.wait();
		readercount--;
		if (readercount == 0) mutexAccess.signal();
		mutexCounter.signal();

		return ptr == internalHashMap.end() ? 0 : ptr->second;
	}
};

// Local Variables: //
// compile-command: "g++ -Wall -O3 -DNDEBUG -fno-reorder-functions -DPIN -DDictionary=FairDictionary Harness.cc -lpthread -lm" //
// End: //
