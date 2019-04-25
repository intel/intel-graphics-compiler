/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#pragma once
#include <assert.h>
#include <stdio.h>
#include <string>
#include "usc_config.h"
#include "IGC/common/igc_debug.h"
#include "3d/common/iStdLib/types.h"

namespace iOpenCL
{

/*****************************************************************************\
Function: DebugMessage
\*****************************************************************************/
void DebugMessage( unsigned int ulDebugLevel, const char* str, ... );

void DebugMessageStr(std::string& output, unsigned int ulDebugLevel, const char* fmt, ...);

/*****************************************************************************\
MACRO: ICBE_DPF_STR
\*****************************************************************************/
#ifndef ICBE_DPF_STR
    #if defined(_DEBUG) || defined(_INTERNAL) || defined(_RELEASE_INTERNAL)
        #define ICBE_DPF_STR iOpenCL::DebugMessageStr
    #else   
        #if defined(ICBE_LHDM) || defined(_WIN32)
            #define ICBE_DPF_STR(output, format, args, ...)
        #else
            #define ICBE_DPF_STR(output, format, args...)
        #endif
    #endif // _DEBUG
#endif // ICBE_DPF_STR

/*****************************************************************************\
MACRO: ICBE_DPF
\*****************************************************************************/
#ifndef ICBE_DPF
    #if defined(_DEBUG)   
        #if defined(ICBE_LHDM) || defined(_WIN32)
            #define ICBE_DPF iOpenCL::DebugMessage
        #else
            #define ICBE_DPF(lvl, fmt, args...) fprintf(stderr, fmt, ## args)
        #endif
    #else  
        #if defined(ICBE_LHDM) || defined(_WIN32)
            #define ICBE_DPF(format, args, ...)
        #else
            #define ICBE_DPF(format, args...)
        #endif
    #endif // _DEBUG
#endif // ICBE_DPF

/*****************************************************************************\
MACRO: ICBE_DEBUG_BREAK
\*****************************************************************************/
#if defined(_DEBUG)
    #if defined(ICBE_LINUX) || defined(_WIN32)
        #define ICBE_DEBUG_BREAK __debugbreak();
    #else
        #define ICBE_DEBUG_BREAK assert(false);
    #endif
#else
    #define ICBE_DEBUG_BREAK
#endif // _DEBUG

/*****************************************************************************\
MACRO: ICBE_ASSERT
\*****************************************************************************/
#if defined(_DEBUG)
    #define ICBE_ASSERT( expr )                                         \
        if( !(expr) )                                                   \
        {                                                               \
            ICBE_DPF( GFXDBG_CRITICAL, "ASSERTION FAILURE:\n" );  \
            ICBE_DPF( GFXDBG_CRITICAL, "  ( "#expr" )\n" );       \
            ICBE_DPF( GFXDBG_CRITICAL, "  File:%s\n", __FILE__ ); \
            ICBE_DPF( GFXDBG_CRITICAL, "  Function:%s\n", __FUNCTION__ ); \
            ICBE_DPF( GFXDBG_CRITICAL, "  Line:%d\n", __LINE__ ); \
            if( true )                                                  \
            {                                                           \
                ICBE_DEBUG_BREAK;                                       \
            }                                                           \
            else                                                        \
            {                                                           \
                assert(0 && " Invalid expression");                     \
            }                                                           \
        }
#else
    #define ICBE_ASSERT( expr )
#endif // _DEBUG

} // namespace iOpenCL
