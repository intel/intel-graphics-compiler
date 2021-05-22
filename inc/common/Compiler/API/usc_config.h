/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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

