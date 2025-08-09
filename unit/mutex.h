/* Mutex

    Types:
        - <mutex>
            type for a mutex

    Functions:
        - void <mutex_init>(<mutex>*)
        - void <mutex_deinit>(<mutex>*)
        - void <mutex_lock>(<mutex>*)
        - void <mutex_unlock>(<mutex>*)
*/

#ifndef UNIT_NO_MULTITHREAD
#ifdef _WIN32
// win32
typedef void* unit_mutex_t; // equivalent to an SRWLOCK)
/*
typedef struct _RTL_SRWLOCK {
        PVOID Ptr;
} RTL_SRWLOCK, *PRTL_SRWLOCK;
*/
/*
#define RTL_SRWLOCK_INIT {0}
*/
#ifndef _SYNCHAPI_H_
// avoid including all the Windows.h stuff
typedef struct _RTL_SRWLOCK RTL_SRWLOCK;
typedef RTL_SRWLOCK SRWLOCK, *PSRWLOCK;

__declspec( dllimport )
void
__stdcall
InitializeSRWLock(PSRWLOCK SRWLock);

__declspec( dllimport )
void
__stdcall
ReleaseSRWLockExclusive(PSRWLOCK SRWLock);

__declspec( dllimport )
void
__stdcall
AcquireSRWLockExclusive(PSRWLOCK SRWLock);
#endif

static inline void unit_mutex_init(unit_mutex_t* mut) {
    InitializeSRWLock((PSRWLOCK)mut);
}

static inline void unit_mutex_deinit(unit_mutex_t* mut) {
    // do nothing
    UNIT_IGNORE_UNUSED(mut);
}

static inline void unit_mutex_lock(unit_mutex_t* mut) {
    AcquireSRWLockExclusive((PSRWLOCK)mut);
}

static inline void unit_mutex_unlock(unit_mutex_t* mut) {
    ReleaseSRWLockExclusive((PSRWLOCK)mut);
}

#else
// pthread
#error "TODO!!!!"

#endif
#else // #ifndef UNIT_NO_MULTITHREAD
typedef struct {} unit_mutex_t;
static inline void unit_mutex_init(unit_mutex_t*) {
    // do nothing
}

static inline void unit_mutex_deinit(unit_mutex_t*) {
    // do nothing
}

static inline void unit_mutex_lock(unit_mutex_t* mut) {
    // do nothing
}

static inline void unit_mutex_unlock(unit_mutex_t* mut) {
    // do nothing
}
#endif
