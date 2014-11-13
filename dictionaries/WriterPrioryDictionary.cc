#include "Semaphore.h"
#include <unordered_map>

template<typename T> class WriterPrioryDictionary {
	Semaphore wrt, mutex_rdcnt, mutex_wtcnt;
	unsigned int readercount, writercount;
	
	std::unordered_map<unsigned int, T *> internalHashMap;
	Dictionary(Dictionary cont &);	// prevent copying
	void operator =(Dictionary const &);

  public:
	Dictionary() : wrt(1), mutex_rdcnt(1), mutex_wtcnt(1), readercount(0), writercount(0) {};

	void put(unsigned int key, T *v) {
  		mutex_wtcnt.wait();
  		writercount++;
		if (writercount == 1) wrt.wait();
		mutex_wtcnt.signal();

		internalHashMap[key] = v;

		mutex_wtcnt.wait();
		writercount--;
		if (writercount == 0) wrt.signal();
		mutex_wtcnt.signal();
	}

	T *tryGet(unsigned int key) {
		mutex_rdcnt.wait();
		readercount++;
		if (readercount == 1) {
			wrt.wait();
		}
		mutex_rdcnt.signal();

		std::unordered_map::iterator ptr = internalHashMap.find(key);
		
		mutex_rdcnt.wait();
		readercount--;
		if (readercount == 0) wrt.signal();
		mutex_rdcnt.signal();
		
		return ptr == internalHashMap.end() ? 0 : ptr->second;
	}
};

// Local Variables: //
// compile-command: "g++ -Wall -O3 -DNDEBUG -fno-reorder-functions -DPIN -DDictionary=WriterPrioryDictionary Harness.cc -lpthread -lm" //
// End: //
