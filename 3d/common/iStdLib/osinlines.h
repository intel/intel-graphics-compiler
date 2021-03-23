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

#ifndef __OSINLINES_H__
#define __OSINLINES_H__

#ifdef _MSC_VER
/*****************************************************************************\
compile-time ASSERT
\*****************************************************************************/
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#endif // _MSC_VER

#if defined(__GNUC__)
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include <UFO/portable_compiler.h>

/*****************************************************************************\
String manipulation.
\*****************************************************************************/
inline int _vsnprintf(
   char *buffer,
   size_t count,
   const char *msg,
   va_list args)
{
    va_list args_cpy;

    // Copy `args' to prevent internal pointer modification from vsnprintf
    va_copy(args_cpy, args);
    int len = vsnprintf(buffer, count, msg, args_cpy);
    va_end(args_cpy);
    return len;
}

inline int _vscprintf(const char *msg, va_list args)
{
    char c;
    va_list args_cpy;

    // Copy `args' to prevent internal pointer modification from vsnprintf
    va_copy(args_cpy, args);
    int len = vsnprintf(&c, 1, msg, args_cpy);
    va_end(args_cpy);
    return len;
}

//from windows stdlib.h (not available by default in gcc)
void *_aligned_malloc(size_t _Size, size_t _Alignment);
void  _aligned_free(void * _Memory);

#endif // __GNUC__

#endif //__OSINLINES_H__
