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

#ifndef _VISA_DEFINES_
#define _VISA_DEFINES_

#ifdef __cplusplus
#pragma once
#endif

#ifdef __cplusplus
extern "C"
{
#include <wchar.h>
}
#include <climits>
#include <cstring>
#else
#include <wchar.h>
#include <limits.h>
#include <string.h>
#endif

#define DIR_WIN32_SEPARATOR   "\\"
#define DIR_UNIX_SEPARATOR   "/"

/* declspecs */
#ifdef _WIN32
#define _THREAD __declspec(thread)
#define DLL_EXPORT extern "C" __declspec(dllexport)
#define _PACKED
#else
#ifdef ANDROID
#define _THREAD
#else
#define _THREAD __thread
#endif

#define DLL_EXPORT extern "C" __attribute__((visibility("default")))
#define _PACKED
#endif

#if defined(_MSC_VER)
#define __restrict__ __restrict
#else
#define __restrict__
#endif

#if defined(DLL_MODE) && defined(vISA_LINK_DLL)
    #ifdef _WIN32
        #define VISA_BUILDER_API __declspec(dllexport)
    #else
        #define VISA_BUILDER_API __attribute__((visibility("default")))
    #endif
#else
#define VISA_BUILDER_API
#endif

#ifndef SNPRINTF
#if defined(ISTDLIB_KMD) || !defined(_WIN32)
#define SNPRINTF( dst, size, ... ) snprintf( (dst), (size), __VA_ARGS__  )
#else
#define SNPRINTF( dst, size, ... ) sprintf_s( (dst), (size), __VA_ARGS__ )
#endif
#endif

#endif /* _VISA_DEFINES_ */
