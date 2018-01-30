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

#ifndef _CM_PORTABILITY_
#define _CM_PORTABILITY_

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

#ifdef _WIN32
#define DIR_SEPARATOR   "\\"
#else
#define DIR_SEPARATOR   "/"
#endif

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
        #define CM_BUILDER_API __declspec(dllexport)
    #else
        #define CM_BUILDER_API __attribute__((visibility("default")))
    #endif
#else
#define CM_BUILDER_API 
#endif

#ifndef CM_INLINE
#if !defined(__GNUC__)
#define CM_INLINE __forceinline
#else
#define CM_INLINE inline __attribute__((always_inline))
#endif
#endif

// FIXME: the following _s functions are copied from secure_mem.h and secure_string.h,
// because I don't have access the gfxdev files in CMRT environment. They should be removed
// when CMRT is checked into the driver.
#ifndef _WIN32
#include <errno.h>
#include <cstring>

using std::memcpy;
using std::strncpy;

typedef int errno_t;
static errno_t memcpy_s(void *dst, size_t numberOfElements, const void *src, size_t count)
{
    if ((dst == NULL) || (src == NULL))
    {
        return EINVAL;
    }
    if (numberOfElements < count)
    {
        return ERANGE;
    }
    memcpy(dst, src, count);
    return 0;
}

static inline int
strcpy_s(char *strDestination, size_t numberOfElements, const char *strSource)
{
    strncpy(strDestination, strSource, numberOfElements);
    strDestination[numberOfElements - 1] = '\0';
    return 0;
}

static inline int
strncpy_s(char *strDestination, size_t numberOfElements, const char *strSource, size_t count)
{
    if( numberOfElements - 1 > count ) {
        strncpy(strDestination, strSource, count);
        strDestination[count] = '\0';
    }
    else {
        strncpy(strDestination, strSource, numberOfElements - 1);
        strDestination[numberOfElements - 1] = '\0';
    }
    return 0;
}

#endif

#ifndef SNPRINTF
#if defined(ISTDLIB_KMD) || !defined(_WIN32)
#define SNPRINTF( dst, size, ... ) snprintf( (dst), (size), __VA_ARGS__  )
#else
#define SNPRINTF( dst, size, ... ) sprintf_s( (dst), (size), __VA_ARGS__ )
#endif
#endif

/* stdio.h portability code end */

/* stdint.h portability code start */
#ifndef _WIN32
// FIXME: do all of our configs support inttypes.h?
#include <stdint.h>
#else
#include <inttypes.h>
#endif /* _WIN32 */
/* stdint.h portability code end */

/* Windows types for non-Windows start */
#if ! defined (_WIN32) && ! defined (_WIN64)
#define SUCCEED 1
#define ERROR   0

typedef char      CHAR;
typedef int       INT;
typedef short     SHORT;
typedef long      LONG;
typedef long long LONGLONG;

typedef uint32_t  UINT32;
typedef uint32_t  DWORD;
typedef uint16_t  WORD;

typedef double    DOUBLE;
typedef float     FLOAT;

union LARGE_INTEGER
{
  struct dummy
  {
    DWORD LowPart;
    LONG HighPart;
  };

  struct u
  {
    DWORD LowPart;
    LONG HighPart;
  };

  LONGLONG QuadPart;
};

#endif /* Windows types for non-Windows end */

#endif /* _CM_PORTABILITY_ */

