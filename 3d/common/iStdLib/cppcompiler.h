/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __CPPCOMPILER_H__
#define __CPPCOMPILER_H__

#if defined(_MSC_VER)
    #define ALIGN( size )   __declspec(align(size))
#else

    #include <UFO/portable_compiler.h>
    #include <UFO/fake_seh.h>

    #define ALIGN(size)     __attribute__((aligned(size)))

    #if NO_EXCEPTION_HANDLING
      #define try         if (true)
      #define catch(x)    if (false)
    #endif

#endif

#endif // __CPPCOMPILER_H__
