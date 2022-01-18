/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "types.h"

#if defined _WIN32
#   include <wtypes.h>
#   include <winbase.h>
#   include <intrin.h>
#else
#   include <time.h>
#   ifndef NSEC_PER_SEC
#       define NSEC_PER_SEC 1000000000L
#   endif
#endif

namespace iSTD
{
/*****************************************************************************\
Inline Function:
    Pause

Description:
    executes __asm pause
\*****************************************************************************/
__forceinline void Pause( void )
{
#if defined(_M_IX86) || defined (_M_AMD64) || \
    defined(__i386__) || defined(__x86_64__)
    _mm_pause();
#endif
}

#if defined _WIN32
/*****************************************************************************\
Inline Function:
    GetTimestampCounter

Description:
    Returns the number of CPU clock cycles since the last reset.
\*****************************************************************************/
__forceinline QWORD GetTimestampCounter( void )
{
    #ifdef ISTDLIB_UMD
    {
        QWORD count = 0;
        ::QueryPerformanceCounter( (LARGE_INTEGER*)&count );
        return count;
    }
    #else // !ISTDLIB_UMD
    {
        #ifdef _WIN64
        {
            return __rdtsc();
        }
        #else // !_WIN64
        {
            __asm rdtsc;
        }
        #endif // _WIN64
    }
    #endif // ISTDLIB_UMD
}

/*****************************************************************************\
Inline Function:
    GetTimestampFrequency

Description:
    Returns the frequency of the CPU clock cycles
\*****************************************************************************/
__forceinline QWORD GetTimestampFrequency( void )
{
    #ifdef ISTDLIB_UMD
    {
        QWORD frequency = 0;
        ::QueryPerformanceFrequency( (LARGE_INTEGER*)&frequency );
        return frequency;
    }
    #else // !ISTDLIB_UMD
    {
        // Note: Use the following for Conroe 2.4GHz
        // return 2394050000;

        return 0;
    }
    #endif // ISTDLIB_UMD
}

/*****************************************************************************\
Inline Function:
    Wait

Description:
    Waits for some number of milliseconds to complete
\*****************************************************************************/
__forceinline void Wait( const DWORD milliseconds )
{
    const QWORD clocksPerSecond = GetTimestampFrequency();
    const QWORD clocksPerMilliSecond = ( clocksPerSecond > 1000 ) ? clocksPerSecond / 1000 : 1;
    const QWORD clocks = milliseconds * clocksPerMilliSecond;

    const QWORD start = GetTimestampCounter();
    while( clocks < ( GetTimestampCounter() - start ) )
    {
        Pause();
    };
}

#else

/*****************************************************************************\
Inline Function:
    GetTimestampCounter

Description:
    Returns the value of high resolution performance counter
\*****************************************************************************/
__forceinline QWORD GetTimestampCounter( void )
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (QWORD)time.tv_nsec +
           (QWORD)time.tv_sec * (QWORD)NSEC_PER_SEC;

}

/*****************************************************************************\
Inline Function:
    GetTimestampFrequency

Description:
    Returns the frequency of high resolution performance counter.
    On Linux/Android we use timer with nsec accuracy
\*****************************************************************************/
__forceinline QWORD GetTimestampFrequency( void )
{
    return NSEC_PER_SEC;
}

#endif

} // iSTD
