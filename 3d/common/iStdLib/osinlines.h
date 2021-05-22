/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

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
