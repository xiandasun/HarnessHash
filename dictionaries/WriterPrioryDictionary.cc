#include "SpinLock.h"
#include <unordered_map>

template<typename T> class WriterPrioryDictionary {
    Spinlock wrt, mutex_rdcnt, mutex_wtcnt;
    unsigned int readercount, writercount;

    std::unordered_map<unsigned int, T *> internalHashMap;
    Dictionary(Dictionary const &);	// prevent copying
    void operator =(Dictionary const &);

    public:
    Dictionary() : wrt(), mutex_rdcnt(), mutex_wtcnt(), readercount(0), writercount(0) {};

    void put(unsigned int key, T *v) {
        mutex_wtcnt.acquire();
        writercount++;
        if (writercount == 1) wrt.acquire();
        mutex_wtcnt.release();

        internalHashMap[key] = v;

        mutex_wtcnt.acquire();
        writercount--;
        if (writercount == 0) wrt.release();
        mutex_wtcnt.release();
    }

    T *tryGet(unsigned int key) {
        mutex_rdcnt.acquire();
        readercount++;
        if (readercount == 1) {
            wrt.acquire();
        }
        mutex_rdcnt.release();

        typename std::unordered_map<unsigned int, T *>::iterator ptr = internalHashMap.find(key);

        mutex_rdcnt.acquire();
        readercount--;
        if (readercount == 0) wrt.release();
        mutex_rdcnt.release();

        return ptr == internalHashMap.end() ? 0 : ptr->second;
    }
};

// Local Variables: //
// compile-command: "g++ -Wall -O3 -DNDEBUG -fno-reorder-functions -DPIN -DDictionary=WriterPrioryDictionary Harness.cc -lpthread -lm" //
// End: //
