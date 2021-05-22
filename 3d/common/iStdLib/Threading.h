/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#ifdef ISTDLIB_MT
#ifdef WIN32_NO_STATUS
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#define WIN32_NO_STATUS
#endif
#endif

#ifdef _WIN32
#   include "types.h"
#   include <process.h>
#   include <malloc.h>
#endif // _WIN32

namespace iSTD
{

#ifdef _WIN32

/*****************************************************************************\
    Critical Section types
\*****************************************************************************/
#ifdef ISTDLIB_MT
#define DECL_CRITICAL_SECTION(x)    CRITICAL_SECTION x
#define INIT_CRITICAL_SECTION(x)    ::InitializeCriticalSection( &(x) )
#define DELETE_CRITICAL_SECTION(x)  ::DeleteCriticalSection( &(x) )
#define ENTER_CRITICAL_SECTION(x)   ::EnterCriticalSection( &(x) )
#define LEAVE_CRITICAL_SECTION(x)   ::LeaveCriticalSection( &(x) )
#else
#define DECL_CRITICAL_SECTION(x)
#define INIT_CRITICAL_SECTION(x)
#define DELETE_CRITICAL_SECTION(x)
#define ENTER_CRITICAL_SECTION(x)
#define LEAVE_CRITICAL_SECTION(x)
#endif

/*****************************************************************************\
    Critical Section debug
\*****************************************************************************/
// The following macros are used to allow synchronizing a function
// to debug multithreading issues due to re-entrancy
#define ENABLE_THREADED_FUNCTION_SYNC   FALSE

#if defined(ISTDLIB_MT) && ENABLE_THREADED_FUNCTION_SYNC

// Structure to store data
struct THREADED_FUNCTION_DATA
{
    // switch to enable\disable synchronizing a function to debug
    // possible threading issues. intended to be toggled manually
    // using the debugger. always disabled by default.
    bool    IsEnabled;

    // critical section data to synchronize function
    DECL_CRITICAL_SECTION( CS );

    // default constructor to initialize data
    THREADED_FUNCTION_DATA()
    {
        IsEnabled = false;
        INIT_CRITICAL_SECTION( CS );
    };

    // default destructor to clean-up data
    ~THREADED_FUNCTION_DATA()
    {
        DELETE_CRITICAL_SECTION( CS );
    };
};

// Macro for entry-point of threaded function
// Each function has a unique (static) enable and critical section
#define THREADED_FUNCTION_ENTER                 \
    static iSTD::THREADED_FUNCTION_DATA sTFD;   \
    if( sTFD.IsEnabled )                        \
    {                                           \
        ENTER_CRITICAL_SECTION( sTFD.CS );      \
    }

// Macro for exit-point of threaded function
#define THREADED_FUNCTION_EXIT                  \
    if( sTFD.IsEnabled )                        \
    {                                           \
        LEAVE_CRITICAL_SECTION( sTFD.CS );      \
    }

#else

#define THREADED_FUNCTION_ENTER
#define THREADED_FUNCTION_EXIT

#endif

/*****************************************************************************\
    Mutex debug
\*****************************************************************************/
// The following macros are used to check that classes\functions that are not
// re-entrant, are never executed concurently for a single instance
#define ENABLE_DEBUG_MUTEX  FALSE

#if defined(ISTDLIB_MT) && ENABLE_DEBUG_MUTEX

// Structure to store data needed by the debug mutex macros
// 1) a mutex handle to ensure only a single thread exists between Acquire and Release
// 2) a counter for the number of threads trying to enter
struct DEBUG_MUTEX_DATA
{
    HANDLE  Mutex;
    (unsigned __int64*) pCounter;
};

// Declare the data structure
#define DECL_DEBUG_MUTEX(x)     iSTD::DEBUG_MUTEX_DATA  x

// Initialize the debug mutex data
// Read Access counter is x.pCounter[0]
// Write Access counter is x.pCounter[1]
#define INIT_DEBUG_MUTEX(x)                                         \
{                                                                   \
    x.Mutex = ::CreateMutex( NULL, FALSE, NULL );                   \
    x.pCounter = (unsigned __int64*)::_aligned_malloc(              \
        2 * sizeof(unsigned __int64), sizeof(unsigned __int64) );   \
    x.pCounter[0] = 0;                                              \
    x.pCounter[1] = 0;                                              \
}

// Clean-up the debug mutex data
#define DELETE_DEBUG_MUTEX(x)                                       \
{                                                                   \
    ::CloseHandle( x.Mutex );                                       \
    ::_aligned_free( x.pCounter );                                  \
}

// Increment the counter of the number of threads entering with read access
// Acquire the mutex and break if another thread is waiting for write access
#define ACQUIRE_DEBUG_MUTEX_READ(x)                                  \
{                                                                    \
    ::InterlockedIncrement( (unsigned __int64*)&( x.pCounter[0] ) ); \
    while( WAIT_OBJECT_0 != ::WaitForSingleObject( x.Mutex, 1 ) )    \
        if ( 0 != ::InterlockedOr( (unsigned __int64*)&( x.pCounter[1] ), 0 ) ) \
            __debugbreak();                                              \
}

// Break if there is another thread waiting for write access
// Decrement read access counter
// Release the mutex
#define RELEASE_DEBUG_MUTEX_READ(x)                                  \
{                                                                    \
    if( 0 != ::InterlockedOr( (unsigned __int64*)&( x.pCounter[1] ), 0 ) ) \
        __debugbreak();                                              \
    ::InterlockedDecrement( (unsigned __int64*)&( x.pCounter[0] ) ); \
    ::ReleaseMutex( x.Mutex );                                       \
}

// Increment the counter of the number of threads entering with write access
// Acquire the mutex and break if another thread owns the mutex
#define ACQUIRE_DEBUG_MUTEX_WRITE(x)                                 \
{                                                                    \
    ::InterlockedIncrement( (unsigned __int64*)&( x.pCounter[1] ) ); \
    while( WAIT_OBJECT_0 != ::WaitForSingleObject( x.Mutex, 1 ) )    \
        __debugbreak();                                              \
}

// Break if there is another thread waiting for read access
// Decrement the counter and break if there is another thread waiting for write access
// Release the mutex
#define RELEASE_DEBUG_MUTEX_WRITE(x)                                 \
{                                                                    \
    if( 0 != ::InterlockedOr( (unsigned __int64*)&( x.pCounter[0] ), 0 ) ) \
        __debugbreak();                                              \
    if( 0 != ::InterlockedDecrement( (unsigned __int64*)&( x.pCounter[1] ) ) ) \
        __debugbreak();                                              \
    ::ReleaseMutex( x.Mutex );                                       \
}

#else
#define DECL_DEBUG_MUTEX(x)
#define INIT_DEBUG_MUTEX(x)
#define DELETE_DEBUG_MUTEX(x)
#define ACQUIRE_DEBUG_MUTEX_READ(x)
#define RELEASE_DEBUG_MUTEX_READ(x)
#define ACQUIRE_DEBUG_MUTEX_WRITE(x)
#define RELEASE_DEBUG_MUTEX_WRITE(x)
#endif

#ifdef ISTDLIB_MT

/*****************************************************************************\
    Thread Management types
\*****************************************************************************/
// These are heavy-weight threads; consider using Thread-Pools instead
#define THREAD_ARGUMENT     void*
typedef unsigned int (__stdcall *THREAD_FUNCTION)( THREAD_ARGUMENT );

#ifdef ISTDLIB_MT
#define THREAD_HANDLE       HANDLE
#else
struct THREAD_DATA
{
    THREAD_FUNCTION     func;
    THREAD_ARGUMENT     arg;
};
#define THREAD_HANDLE       THREAD_DATA*
#endif

/*****************************************************************************\
Function:
    CreateThreads

Input:
    const DWORD numThreads - number of threads to create
    THREAD_FUNCTION func - pointer to function to execute
    THREAD_ARGUMENT* args - array of per-thread arguments

Output:
    THREAD_HANDLE* threads - array of per-thread handles
    THREAD_HANDLE* beginEvents - array of per-thread events to signal the
        thread to begin
    THREAD_HANDLE* endEvents - array of per-thread events the thread sets
        when complete
    HRESULT

\*****************************************************************************/
inline HRESULT CreateThreads(
    const DWORD numThreads,
    THREAD_FUNCTION func,
    THREAD_ARGUMENT* args,
    THREAD_HANDLE* threads,
    THREAD_HANDLE* beginEvents,
    THREAD_HANDLE* endEvents )
{
    HRESULT hr = S_OK;

    for( DWORD i = 0; i < numThreads; ++i )
    {
#ifdef ISTDLIB_MT

        beginEvents[i] = ::CreateEvent( NULL, FALSE, FALSE, NULL );
        endEvents[i] = ::CreateEvent( NULL, FALSE, FALSE, NULL );

        threads[i] = ( THREAD_HANDLE )::_beginthreadex(
            NULL,
            0,
            func,
            args[i],
            CREATE_SUSPENDED,
            NULL );

        if( threads[i] )
        {
            ::ResumeThread( threads[i] );
        }
        else
        {
            hr = E_FAIL;
        }

#else

        threads[i] = (THREAD_DATA*)malloc( sizeof(THREAD_DATA) );

        if( threads[i] )
        {
            threads[i]->func = func;
            threads[i]->arg = args[i];

            beginEvents[i] = threads[i];
            endEvents[i] = NULL;
        }
        else
        {
            hr = E_FAIL;
        }

#endif
    }

    return hr;
}

/*****************************************************************************\
    StartThreads
\*****************************************************************************/
inline void StartThreads(
    const DWORD numThreads,
    THREAD_HANDLE* beginEvents )
{
    for( DWORD i = 0; i < numThreads; ++i )
    {
#ifdef ISTDLIB_MT

        ::SetEvent( beginEvents[i] );

#else

        beginEvents[i]->func( beginEvents[i]->arg );

#endif
    }
}

/*****************************************************************************\
    WaitForThreads
\*****************************************************************************/
inline void WaitForThreads(
    const DWORD numThreads,
    THREAD_HANDLE* endEvents )
{
#ifdef ISTDLIB_MT

#ifdef _DEBUG

    // deadlock detection
    DWORD result = 0;
    do
    {
        result = ::WaitForMultipleObjects(
            numThreads,
            endEvents,
            TRUE,
            1000 );
        result &= 0xfffffff0;
        ASSERT( WAIT_OBJECT_0 == result );
    }
    while( WAIT_OBJECT_0 != result );

#else

    ::WaitForMultipleObjects(
        numThreads,
        endEvents,
        TRUE,
        INFINITE );

#endif

#endif
}

/*****************************************************************************\
    DeleteThreads
\*****************************************************************************/
inline void DeleteThreads(
    const DWORD numThreads,
    THREAD_HANDLE* threads,
    THREAD_HANDLE* beginEvents,
    THREAD_HANDLE* endEvents )
{
    for( DWORD i = 0; i < numThreads; ++i )
    {
#ifdef ISTDLIB_MT

        ::CloseHandle( threads[i] );
        ::CloseHandle( endEvents[i] );
        ::CloseHandle( beginEvents[i] );

#else

        free( threads[i] );
        threads[i] = NULL;
        endEvents[i] = NULL;
        beginEvents[i] = NULL;

#endif
    }
}

/*****************************************************************************\
    Thread-Pool Management types
\*****************************************************************************/
#if defined(ISTDLIB_MT) && (_WIN32_WINNT >= 0x0600)
// Minimum supported client: Windows Vista (_WIN32_WINNT_VISTA = 0x0600)

/*****************************************************************************\

Function:
    CreateThreadPool

Description:
    Creates a thread pool

Input:
    PTP_CALLBACK_ENVIRON ptrPoolEnv - pointer to storage where thread pool
        environment will be stored
    DWORD minThreads - minimum number of requested threads in the thread pool.
    DWORD maxThreads - maximum number of requested threads in the thread pool.

Output:
    BOOL - Success or Fail

Notes:
    Client is responsible for management of PTP_CALLBACK_ENVIRON memory

\*****************************************************************************/
inline BOOL CreateThreadPool(
    PTP_CALLBACK_ENVIRON ptrPoolEnv,
    const DWORD minThreads,
    const DWORD maxThreads )
{
    if( ptrPoolEnv )
    {
        InitializeThreadpoolEnvironment( ptrPoolEnv );

        PTP_POOL ptrPool = CreateThreadpool( NULL );
        if( ptrPool )
        {
            SetThreadpoolThreadMinimum( ptrPool, minThreads );
            SetThreadpoolThreadMaximum( ptrPool, maxThreads );

            SetThreadpoolCallbackRunsLong( ptrPoolEnv );
            SetThreadpoolCallbackPool( ptrPoolEnv, ptrPool );
            return TRUE;
        }
    }
    return FALSE;
}

/*****************************************************************************\

Function:
    CreateThreadPool

Description:
    Creates a thread pool

Input:
    PTP_CALLBACK_ENVIRON ptrPoolEnv - pointer to storage where thread pool 
        environment will be stored

Output:
    BOOL - Success or Fail

Notes:
    Client is responsible for management of PTP_CALLBACK_ENVIRON memory

\*****************************************************************************/
inline BOOL CreateThreadPool(
    PTP_CALLBACK_ENVIRON ptrPoolEnv )
{
    SYSTEM_INFO si = { 0 };
    ::GetSystemInfo( &si );
    return CreateThreadPool( ptrPoolEnv, si.dwNumberOfProcessors, si.dwNumberOfProcessors );
}

/*****************************************************************************\

Function:
    DeleteThreadPool

Description:
    Deletes a thread pool

Input:
    PTP_CALLBACK_ENVIRON ptrPoolEnv - pointer to storage where thread pool
        environment is stored

Output:
    none

Notes:
    Client is responsible for management of PTP_CALLBACK_ENVIRON memory

\*****************************************************************************/
inline void DeleteThreadPool(
    PTP_CALLBACK_ENVIRON ptrPoolEnv )
{
    if( ptrPoolEnv )
    {
        if( ptrPoolEnv->Pool )
        {
            CloseThreadpool( ptrPoolEnv->Pool );
        }
    }
}

/*****************************************************************************\

Function:
    CreateThreadPoolWork

Description:
    Creates a thread pool work item

Input:
    PTP_WORK_CALLBACK pWorkFunc - pointer to function to add to thread pool
    PVOID pWorkData - optional data to pass to function
    PTP_CALLBACK_ENVIRON ptrPoolEnv - thread pool environment

Output:
    PTP_WORK - pointer to work item

\*****************************************************************************/
inline PTP_WORK CreateThreadPoolWork(
    PTP_WORK_CALLBACK pWorkFunc,
    PVOID pWorkData,
    PTP_CALLBACK_ENVIRON ptrPoolEnv )
{
    return CreateThreadpoolWork( pWorkFunc, pWorkData, ptrPoolEnv );
}

/*****************************************************************************\

Function:
    SubmitThreadPoolWork

Description:
   Submits a thread pool work item to the thread pool

Input:
    PTP_WORK - pointer to work item

Output:
    none

\*****************************************************************************/
inline void SubmitThreadPoolWork(
    PTP_WORK pWorkItem )
{
    if( pWorkItem )
    {
        SubmitThreadpoolWork( pWorkItem );
    }
}

/*****************************************************************************\

Function:
    WaitForThreadPoolWork

Description:
    Waits for the thread pool work item to complete

Input:
    PTP_WORK ptrWork - pointer to work item

Output:
    none

\*****************************************************************************/
inline void WaitForThreadPoolWork(
    PTP_WORK ptrWork )
{
    if( ptrWork )
    {
        WaitForThreadpoolWorkCallbacks( ptrWork, FALSE );
    }
}

/*****************************************************************************\

Function:
    DeleteThreadPoolWork

Description:
    Deletes the thread pool work item to complete

Input:
    PTP_WORK ptrWork - pointer to work item

Output:
    none

\*****************************************************************************/
inline void DeleteThreadPoolWork(
    PTP_WORK& ptrWork )
{
    if( ptrWork )
    {
        CloseThreadpoolWork( ptrWork );
        ptrWork = NULL;
    }
}

#endif //defined(ISTDLIB_MT) && (_WIN32_WINNT >= 0x0600)

#endif //ISTDLIB_MT

#else   // _WIN32

#define THREADED_FUNCTION_ENTER
#define THREADED_FUNCTION_EXIT

#define DECL_CRITICAL_SECTION(x)
#define INIT_CRITICAL_SECTION(x)
#define DELETE_CRITICAL_SECTION(x)
#define ENTER_CRITICAL_SECTION(x)
#define LEAVE_CRITICAL_SECTION(x)

#define DECL_DEBUG_MUTEX(x)
#define INIT_DEBUG_MUTEX(x)
#define DELETE_DEBUG_MUTEX(x)
#define ACQUIRE_DEBUG_MUTEX_READ(x)
#define RELEASE_DEBUG_MUTEX_READ(x)
#define ACQUIRE_DEBUG_MUTEX_WRITE(x)
#define RELEASE_DEBUG_MUTEX_WRITE(x)

#endif  // _WIN32

} // iSTD
