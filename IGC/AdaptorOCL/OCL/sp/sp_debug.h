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
#include <string>
#include "IGC/common/igc_debug.h"

namespace iOpenCL
{

/*****************************************************************************\
Function: DebugMessage
\*****************************************************************************/

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

} // namespace iOpenCL
