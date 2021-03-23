/*========================== begin_copyright_notice ============================

Copyright (c) 2019-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

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
