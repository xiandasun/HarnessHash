#include "SpinLock.h"
#include <unordered_map>

template<typename T> class ReaderPrioryDictionary {
	Spinlock wrt, mutex;
	unsigned int readercount;
	
	std::unordered_map<unsigned int, T *> internalHashMap;
	Dictionary(Dictionary const &);	// prevent copying
	void operator =(Dictionary const &);

  public:
	Dictionary() : wrt(), mutex(), readercount(0) {};

	void put(unsigned int key, T *v) {
		wrt.acquire();
		internalHashMap[key] = v;
		wrt.release();
	}

	T *tryGet(unsigned int key) {
		mutex.acquire();
		readercount++;
		if (readercount == 1) {
			wrt.acquire();
		}
		mutex.release();

		typename std::unordered_map<unsigned int, T *>::iterator ptr = internalHashMap.find(key);
		
		mutex.acquire();
		readercount--;
		if (readercount == 0) wrt.release();
		mutex.release();
		
		return ptr == internalHashMap.end() ? 0 : ptr->second;
	}
};

// Local Variables: //
// compile-command: "g++ -Wall -O3 -DNDEBUG -fno-reorder-functions -DPIN -DDictionary=ReaderPrioryDictionary Harness.cc -lpthread -lm" //
// End: //
