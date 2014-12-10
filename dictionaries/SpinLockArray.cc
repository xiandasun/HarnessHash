#include "SpinLock.h"
using namespace std;
#include <cassert>

template<typename K, typename T> class Dictionary {
    Spinlock lock;
    T **arr;

    Dictionary( Dictionary const & );			// prevent copying
    void operator=( Dictionary const & );
  public:
    Dictionary( unsigned int N ) : arr( new T *[N] ) {}

    ~Dictionary() { delete [] arr; }

    void put( unsigned int key, T *v ) {
        lock.acquire();
        arr[key] = v;
        lock.release();
    }

    T *tryGet( unsigned int key ) {
        lock.acquire();
        T *ret = arr[key];
        lock.release();
        return ret;
    }
};


// Local Variables: //
// compile-command: "g++ -std=c++11 -Wall -O3 -DNDEBUG -fno-reorder-functions -DPIN -DDictionary=SpinLockArray Harness.cc -lpthread -lm" //
// End: //
