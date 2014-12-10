#include "SpinLock.h"
#include <unordered_map>

template<typename K, typename T> class Dictionary : public std::unordered_map<K, T *> {
    Spinlock lock;

    Dictionary( Dictionary const & );			// prevent copying
    void operator=( Dictionary const & );
  public:
    Dictionary( typename std::unordered_map<K, T *>::size_type n ) /* : std::unordered_map<K, T *>( n ) */ {};

    void put( K key, T *v ) {
        lock.acquire();
        (*this)[key] = v;
        lock.release();
    } // Dictionary::put

    T *tryGet( K key ) {
        lock.acquire();
        T *ret = (*this)[key];
        lock.release();
        return ret;
    } // Dictionary::tryGet
}; // Dictionary



// Local Variables: //
// compile-command: "g++ -std=c++11 -Wall -O3 -DNDEBUG -fno-reorder-functions -DPIN -DDictionary=SpinLockDictionary Harness.cc -lpthread -lm" //
// End: //
