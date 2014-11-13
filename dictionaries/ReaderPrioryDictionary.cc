#include "Semaphore.h"
#include <unordered_map>

template<typename T> class ReaderPrioryDictionary {
	Semaphore wrt, mutex;
	unsigned int readercount;
	
	std::unordered_map<unsigned int, T *> internalHashMap;
	Dictionary(Dictionary cont &);	// prevent copying
	void operator =(Dictionary const &);

  public:
	Dictionary() : wrt(1), mutex(1), readercount(0) {};

	void put(unsigned int key, T *v) {
		wrt.wait();
		internalHashMap[key] = v;
		wrt.signal();
	}

	T *tryGet(unsigned int key) {
		mutex.wait();
		readercount++;
		if (readercount == 1) {
			wrt.wait();
		}
		mutex.signal();

		std::unordered_map::iterator ptr = internalHashMap.find(key);
		
		mutex.wait();
		readercount--;
		if (readercount == 0) wrt.signal();
		mutex.signal();
		
		return ptr == internalHashMap.end() ? 0 : ptr->second;
	}
};

// Local Variables: //
// compile-command: "g++ -Wall -O3 -DNDEBUG -fno-reorder-functions -DPIN -DDictionary=ReaderPrioryDictionary Harness.cc -lpthread -lm" //
// End: //
