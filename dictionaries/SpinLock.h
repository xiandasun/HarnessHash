// Lock used to protect ready queue when shared access

class Spinlock {					// SPINLOCK, with optional exponential backoff
    volatile TYPE lock
#if defined( __i386 ) || defined( __x86_64 )
	__attribute__(( aligned (128) ))		// Intel recommendation
#elif defined( __sparc )
	CALIGN
#else
    #error unsupported architecture
#endif
    ;
  public:
    void acquire() {
#ifndef NOEXPBACK
	enum { SPIN_START = 4, SPIN_END = 64 * 1024, };
	unsigned int spin = SPIN_START;
#endif // ! NOEXPBACK

	for ( unsigned int i = 0;; i += 1 ) {
	  if ( lock == 0 && __atomic_test_and_set( &lock, __ATOMIC_SEQ_CST ) == 0 ) break;
#ifndef NOEXPBACK
	    for ( unsigned int s = 0; s < spin; s += 1 ) Pause(); // exponential spin
	    spin += spin;				// powers of 2
//	    if ( i % 40 ) spin += spin;			// slowly increase by powers of 2
	    if ( spin > SPIN_END ) spin = SPIN_START;	// prevent overflow
#else
	    Pause();
#endif // ! NOEXPBACK
	} // for
    } // Spinlock::acquire

    void release() {
	__atomic_clear( &lock, __ATOMIC_SEQ_CST );
    } // Spinlock::release

    Spinlock() : lock( 0 ) {}
}; // Spinlock

// Local Variables: //
// mode: c++ //
// End: //
