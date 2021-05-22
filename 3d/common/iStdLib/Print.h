/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#ifdef __cplusplus
#ifdef _WIN32

#include <stdio.h>

#if defined ISTDLIB_KMD
#   include <stdarg.h>
#endif

#if defined ISTDLIB_UMD && !defined UNIT_TESTING
#   include <wtypes.h>
#   include <winbase.h>
#endif

namespace iSTD
{

    /*****************************************************************************\
    Extern: EngDebugPrint
    Defined in winddi.h
    \*****************************************************************************/
#if defined(ISTDLIB_KMD)
    extern "C" void APIENTRY EngDebugPrint( PCHAR, PCHAR, va_list );
#endif

    /*****************************************************************************\
    Inline Function: PrintMessage
    PURPOSE: Prints a message for both debug and release drivers
    \*****************************************************************************/
    inline void __cdecl PrintMessage( char* str, ... )
    {
        if( str )
        {
            va_list args;
            va_start( args, str );

            const size_t length = ::_vscprintf( str, args ) + 1;
            char* temp = new char[ length ];

            if( temp )
            {
#if defined(ISTDLIB_KMD)
                // Send message to kernel debugger
                ::_vsnprintf( temp, length, str, args );
                EngDebugPrint( "INTC: ", "%s", (PCHAR)&temp );

#elif defined(ISTDLIB_UMD)
#ifndef UNIT_TESTING
                ::_vsnprintf_s( temp, length, length, str, args );
                OutputDebugStringA( "INTC: " );
                OutputDebugStringA( temp );
#endif
#endif
                delete[] temp;
            }

            va_end( args );
        }
    }

} // namespace iSTD

#endif // _WIN32
#endif // __cplusplus
