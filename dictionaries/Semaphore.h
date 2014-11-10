// Lock used to protect ready queue when shared access

#define CACHE_ALIGN 64
#define CALIGN __attribute__(( aligned (CACHE_ALIGN) ))

typedef uintptr_t TYPE;

// pause to prevent excess processor bus usage
#if defined( __sparc )
#define Pause() __asm__ __volatile__ ( "rd %ccr,%g0" )
#elif defined( __i386 ) || defined( __x86_64 )
#define Pause() __asm__ __volatile__ ( "pause" : : : )
#else
#error unsupported architecture
#endif

class Semaphore {
    volatile TYPE S
#if defined( __i386 ) || defined( __x86_64 )
	__attribute__(( aligned (128) ));		// Intel recommendation
#elif defined( __sparc )
	CALIGN;
#else
    #error unsupported architecture
#endif
  public:
    void wait() {
			enum { SPIN_START = 4, SPIN_END = 64 * 1024, };
			unsigned int spin = SPIN_START;

			int i = S;
			while ((S == 0) && !__sync_bool_compare_and_swap(&S, i, i - 1)) {
				for (unsigned int w = 0; w < spin; w++) Pause();
				spin += spin;
				if (spin > SPIN_END) spin = SPIN_START;
				Pause();	
				i = S;
			} // while
    } // Semaphore::wait

    void signal() {
			int i = S;
			while (!__sync_bool_compare_and_swap(&S, i, i + 1)) {
				i = S;
			}
    } // Semaphore::signal

    Semaphore(int init) : S( init ) {}
}; // Semaphore

// Local Variables: //
// mode: c++ //
// End: //
