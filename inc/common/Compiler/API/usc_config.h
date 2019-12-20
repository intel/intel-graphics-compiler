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

#if (defined(_MSC_VER) && (_MSC_VER >= 1600))
#    define USC_AS_DLL
#endif

#ifdef USC_AS_DLL
#    ifdef USC_EXPORTS
#        if defined(_WIN32) || defined(_WIN64)
#            define USC_API __declspec(dllexport)
#        else
#            define USC_API __attribute__((visibility("default")))
#        endif
#    else
#        define USC_API __declspec(dllimport)
#    endif
#else // USC linked statically
#    define USC_API
#endif

#if defined (_MSC_VER)
    #define USC_API_CALL USC_API __stdcall
#else
    #define USC_API_CALL USC_API
#endif

/*****************************************************************************\
    Macros that are required to allow compilation by
    multiple comilers (eg. gcc, msvc).
\*****************************************************************************/
#ifdef _MSC_VER
    #define ALIGN( size )   __declspec(align(size))
#endif

#ifdef __GNUC__

    #include "../../UFO/portable_compiler.h"

    #if NO_EXCEPTION_HANDLING
      #define try         if (true)
      #define catch(x)    if (false)
    #endif

    #define ALIGN( size )   __attribute__((aligned(size)))

#endif // __GNUC__

