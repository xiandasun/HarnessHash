// Harness program for Dictionaries 
// git clone https://github.com/xiandasun/HarnessHash.git

#include <iostream>
#include <iomanip>
#include <cstdio>										// perror
#include <cstdlib>										// atoi, abort
#include <cmath>										// sqrt
#include <cerrno>										// errno
using namespace std;
#include <malloc.h>										// memalign
#include <climits>										// UNINT_MAX
#include <stdint.h>										// intptr_t
#include <poll.h>										// poll

#define CACHE_ALIGN 64
#define CALIGN __attribute__(( aligned(CACHE_ALIGN) ))

// pause to prevent excess processor bus usage
#if defined( __sparc )
#define Pause() __asm__ __volatile__ ( "rd %ccr,%g0" )
#elif defined( __i386 ) || defined( __x86_64 )
#define Pause() __asm__ __volatile__ ( "pause" : : : )
#else
#error unsupported architecture
#endif

// likely
#define FASTPATH(x) __builtin_expect(!!(x), 1)
// unlikely
#define SLOWPATH(x) __builtin_expect(!!(x), 0)

//------------------------------------------------------------------------------

#if defined( __GNUC__ )									// GNU gcc compiler ?
// O(1) polymorphic integer log2, using clz, which returns the number of leading 0-bits, starting at the most
// significant bit (single instruction on x86)
#define Log2( n ) ( sizeof(n) * __CHAR_BIT__ - 1 - (			\
				  ( sizeof(n) ==  4 ) ? __builtin_clz( n ) :	\
				  ( sizeof(n) ==  8 ) ? __builtin_clzl( n ) :	\
				  ( sizeof(n) == 16 ) ? __builtin_clzll( n ) :	\
				  -1 ) )
#else
static int Log2( int n ) {								// fallback integer log2( n )
	return n > 1 ? 1 + Log2( n / 2 ) : n == 1 ? 0 : -1;
}
#endif // __GNUC__

static inline int Clog2( int n ) {						// integer ceil( log2( n ) )
	if ( n <= 0 ) return -1;
	int ln = Log2( n );
	return ln + ( (n - (1 << ln)) != 0 );				// check for any 1 bits to the right of the most significant bit
}

//------------------------------------------------------------------------------

typedef uintptr_t TYPE;									// atomically addressable word-size

//------------------------------------------------------------------------------

void affinity( pthread_t pthreadid, unsigned int tid ) {
	// There are many ways to assign threads to processors: cores, chips, etc.  On the AMD, we find starting at core 32
	// and sequential assignment is sufficient.  Below are alternative approaches.
#if defined( __linux ) && defined( PIN )
	cpu_set_t mask;

	CPU_ZERO( &mask );
	int cpu;

	enum { OFFSET = 32 };								// upper range of cores away from core 0
	cpu = tid + OFFSET;

	CPU_SET( cpu, &mask );
	int rc = pthread_setaffinity_np( pthreadid, sizeof(cpu_set_t), &mask );
	if ( rc != 0 ) {
		errno = rc;
		perror( "setaffinity" );
		abort();
	} // if
#endif // linux && PIN
} // affinity

//------------------------------------------------------------------------------
// Define the universe of possible elements
// Read / Write freqency of elements

static const unsigned int UNIVERSE_SIZE = 1000;
static unsigned int rIndex[UNIVERSE_SIZE * UNIVERSE_SIZE];
static unsigned int wIndex[UNIVERSE_SIZE * UNIVERSE_SIZE];

static unsigned long x = 123456789, y = 362436069, z = 521288629;
static unsigned long range = UNIVERSE_SIZE * UNIVERSE_SIZE;

// XorShift by George Marsaglia
inline unsigned long xorshf96(void) {		  //period 2^96-1
	unsigned long t;
	x ^= x << 16;
	x ^= x >> 5;
	x ^= x << 1;

	t = x;
	x = y;
	y = z;
	z = (t ^ x ^ y) % range;

	return z >= 0 ? z : -z;
} // xorshf96

// Approximate Zipf Distribute
// More reads, more writes
void generateZipfAccessPatterns() {
	range = 0;
	for (unsigned int i = 0; i < UNIVERSE_SIZE; i++) {
		for (unsigned int j = 0; j < UNIVERSE_SIZE / (i + 1); j++) {
			rIndex[range] = i;
			wIndex[range] = UNIVERSE_SIZE - 1 - i;
			range++;
		}
	}
} // generateAccessPatterns

inline unsigned int nextReadKey() {
	unsigned long r = xorshf96();
//	return rIndex[r];
	return r;
} // nextReadKey

inline unsigned int nextWriteKey() {
	unsigned w = xorshf96();
	return wIndex[w];
} // nextWriteKey

//------------------------------------------------------------------------------

struct Node
#ifdef __U_COLLECTION_H__
		: public uColable
#endif // __U_COLLECTION_H__
{														// data structure
//	unsigned int key;									// key field
	unsigned int owner;									// dummy field used for checking

	Node() : owner( UINT_MAX ) {}
}; // Node

//------------------------------------------------------------------------------

#define xstr(s) str(s)
#define str(s) #s
#include xstr(Dictionary.cc)							// include dictionary for testing

//------------------------------------------------------------------------------

#ifndef DELAY
#define DELAY 100
#endif // DELAY

struct Reader {
	Dictionary<unsigned int, Node> &dictionary;
	pthread_t pthreadid;								// pthread pid
	unsigned int tid;									// simulation tid
	size_t entries;										// number of put/get in T seconds
	size_t getf;										// get fails
	volatile size_t stop;								// indicates experiment is over
	Node entry;											// node for the list
	enum { Delay = DELAY };

	static void *main( void *arg ) {
		Reader &This = *(Reader *)arg;					// "This" is Reader object

#ifdef USING_URCU_LIB
		rcu_register_thread();
#endif	

		for ( volatile unsigned int i = 0; i < This.tid * 100; i += 1 ); // random start

		This.dictionary.put( This.tid, &This.entry );	// insert my node
		int fred = 0;
		for ( ;; ) {
		  if ( This.stop != 0 ) break;					// done experiment ?
		    unsigned int key = fred;
			fred = (fred + 1) % range;
			Node *node = This.dictionary.tryGet( key );	// get arbitrary node
//			Node *node = This.dictionary.tryGet( This.tid ); // get my node
			if ( node != 0 ) {
//				if ( node->owner != UINT_MAX ) abort();	// precheck
//				node->owner = This.tid;					// set check
				for ( volatile unsigned int i = 0; i < Delay; i += 1 ) { // delay
//					if ( node->owner != This.tid ) abort();
				} // for
//				node->owner = UINT_MAX;					// reset check
//				This.dictionary.put( This.tid, node ); // get my node
				This.entries += 1;
			} else {
				This.getf += 1;							// count get failures
			} // if
		} // for

#ifdef USING_URCU_LIB	
		rcu_unregister_thread();
#endif
		return 0;
	} // Reader::main

	Reader( unsigned int tid, Dictionary<unsigned int, Node> &dictionary )
		: dictionary( dictionary ), tid( tid ), getf( 0 ), stop( 0 ) {
			int rc = pthread_create( &pthreadid, NULL, Reader::main, (void *)this );
			if ( rc != 0 ) {
				errno = rc;
				perror( "pthread create" );
				abort();
			} // if
			affinity( pthreadid, tid );					// establish CPU affinitiy (if appropriate)
		} // Reader::Reader

	void join() {
		int rc = pthread_join( pthreadid, NULL );
		if ( rc != 0 ) {
			errno = rc;
			perror( "pthread create" );
			abort();
		} // if
	} // Reader::join
}; // Reader

//------------------------------------------------------------------------------

struct Writer {
	Dictionary<unsigned int, Node> &dictionary;
	pthread_t pthreadid;								// pthread pid
	unsigned int tid;									// simulation tid
	size_t entries;										// number of put/get in T seconds
	size_t getf;										// get fails
	volatile size_t stop;								// indicates experiment is over
	static Node *entry[UNIVERSE_SIZE];					// never CHANGED once initialized, used to avoid the time of memory allocating

	static void *main( void *arg ) {
		Writer &This = *(Writer *)arg;					// "This" is Writer object

		for ( volatile unsigned int i = 0; i < This.tid * 100; i += 1 ); // random start

		for ( ;; ) {
			if ( This.stop != 0 ) break;					// done experiment ?
			unsigned int key = nextWriteKey();
			for (unsigned testi = 0; testi < 1000; testi++);
			This.dictionary.put( key, entry[key] );				// put arbitrary node
			This.entries += 1;
		} // for
		return 0;
	} // Writer::main

	Writer( unsigned int tid, Dictionary<unsigned int, Node> &dictionary )
		: dictionary( dictionary ), tid( tid ), getf( 0 ), stop( 0 ) {
			int rc = pthread_create( &pthreadid, NULL, Writer::main, (void *)this );
			if ( rc != 0 ) {
				errno = rc;
				perror( "pthread create" );
				abort();
			} // if
			affinity( pthreadid, tid );						// establish CPU affinitiy (if appropriate)
		} // Writer::Writer

	void join() {
		int rc = pthread_join( pthreadid, NULL );
		if ( rc != 0 ) {
			errno = rc;
			perror( "pthread create" );
			abort();
		} // if
	} // Writer::join

	static void Initialize() {
		for (unsigned int i = 0; i < UNIVERSE_SIZE; i++) {
			entry[i] = new Node;
		}
	}
}; // Writer

Node *Writer::entry[UNIVERSE_SIZE];

//------------------------------------------------------------------------------

int main( int argc, char *argv[] ) {
	int Time = 2, Writers = 1, Readers = 16;							// default values

	switch ( argc ) {									// argument check
	  case 4:
		Writers = atoi( argv[3] );
	  case 3:
		Readers = atoi( argv[2] );
	  case 2:
		Time = atoi( argv[1] );
		if ( Time < 1 || Writers < 0 ) goto usage;
	  case 1:
		break;
	  usage:
	  default:
		cout << "Usage: " << argv[0] << " "
			 << Time << " (total experiment duration) "
			 << Writers << " (Writer number) "
			 << Readers << " (Reader number) "
			 << endl;
		exit( EXIT_FAILURE );
	} // switch

	cout << Readers << "\t" <<  Writers << "\t" << Time << "\t";

	// start up

	range = Writers + Readers;
//	generateZipfAccessPatterns(); 
//	Dictionary<unsigned int, Node> dictionary;			// only one global dictionary
	Dictionary<unsigned int, Node> dictionary( Writers + Readers );	// only one global dictionary

//	Writer::Initialize();

//	for ( unsigned int i = 0; i < UNIVERSE_SIZE; i++ ) {
//		dictionary.put( i, Writer::entry[i] );
//	}

	Reader *reader[Readers];
	Writer *writer[Writers];
	size_t subtotalReads[Readers], subtotalWrites[Writers], totalReads = 0, totalWrites = 0, getf = 0;

	for ( int tid = 0; tid < Writers; tid += 1 ) {
		writer[tid] = new Writer( tid, dictionary );
	} // for

	for ( int tid = Writers; tid < Writers + Readers; tid += 1) {
		reader[tid - Writers] = new Reader( tid, dictionary );
	} // for

	poll( NULL, 0, Time * 1000 );						// delay until experiment over

	// shut down

	for ( int tid = 0; tid < Writers; tid += 1 ) {
		writer[tid]->stop = 1;
	} // for

	for ( int tid = 0; tid < Readers; tid += 1 ) {
		reader[tid]->stop = 1;
	} // for

	for ( int tid = 0; tid < Writers; tid += 1 ) {
		writer[tid]->join();
		subtotalWrites[tid] = writer[tid]->entries;
		totalWrites += subtotalWrites[tid];
		getf += writer[tid]->getf;
	} // for

	for ( int tid = 0; tid < Readers; tid += 1 ) {
		reader[tid]->join();
		subtotalReads[tid] = reader[tid]->entries;
		totalReads += subtotalReads[tid];
		getf += reader[tid]->getf;
	} // for

	for ( int tid = 0; tid < Writers; tid += 1 ) {
		delete writer[tid];
	} // for

	for ( int tid = 0; tid < Readers; tid += 1 ) {
		delete reader[tid];
	} // for

	// output

	double avgWrites = (double)totalWrites / Writers;				// average
	double avgReads = (double)totalReads / Readers;				// average
	double sum = 0.0;
	for ( int tid = 0; tid < Writers; tid += 1 ) {		// sum squared differences from average
		double diff = subtotalWrites[tid] - avgWrites;
		sum += diff * diff;
	} // for
	double stdWrites = sqrt( sum / Writers );
	sum = 0.0;
	for ( int tid = 0; tid < Readers; tid += 1 ) {		// sum squared differences from average
		double diff = subtotalReads[tid] - avgReads;
		sum += diff * diff;
	} // for
	double stdReads = sqrt( sum / Readers );

	cout << fixed << setprecision(1);
	cout << totalReads << "\t" << totalWrites << "\t" << getf << "\t" << avgReads << "\t" << stdReads / avgReads * 100 << "%\t" <<  avgWrites << "\t" << stdWrites / avgWrites * 100 << "%" << endl;

	//	cout << endl;
	//	malloc_stats();
	//	cout << endl;
} // main

// Local Variables: //
// tab-width: 4 //
// End: //
