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

/**
 *  This file provides portable implementation of CPUID functions:
 *
 *  void __cpuid(int info[4], int function_id);
 *  void __cpuidex(int info[4], int function_id, int subfunction_id);
 *  int __get_cpuid(unsigned level, unsigned *eax, unsigned *ebx, unsigned *ecx, unsigned *edx);
 */

#ifndef UFO_PORTABLE_CPUID_H
#define UFO_PORTABLE_CPUID_H

#if defined(_MSC_VER) || defined(_WIN32)

    #if defined (_WIN64) && defined (_In_)
        // NOTE: <math.h> is not necessary here.
        // This is only an ugly workaround for a VS2008 bug that causes the compilation
        // issue on 64-bit DEBUG configuration.
        // Including "math.h" before "intrin.h" helps to get rid of the following warning:
        // warning C4985: 'ceil': attributes not present on previous declaration.
        #include <math.h>
        #include <smmintrin.h>
    #else
        #include <intrin.h>
    #endif

    #if (_MSC_VER >= 1500)
        #pragma intrinsic (__cpuid)
    #endif

    #if defined(__cplusplus) && (_MSC_VER >= 1900)
        // __get_cpuid as lambda
        #define __get_cpuid(_level, _peax, _pebx, _pecx, _pedx) \
            [](unsigned level, unsigned *peax, unsigned *pebx, unsigned *pecx, unsigned *pedx) { \
                int tmp[4] = { 0 }; \
                __cpuid(tmp, level); \
                *(peax) = static_cast<unsigned>(tmp[0]); \
                *(pebx) = static_cast<unsigned>(tmp[1]); \
                *(pecx) = static_cast<unsigned>(tmp[2]); \
                *(pedx) = static_cast<unsigned>(tmp[3]); \
                return 1; \
            } (_level, _peax, _pebx, _pecx, _pedx)
    #else
        static __forceinline
        int __get_cpuid(unsigned level, unsigned *peax, unsigned *pebx, unsigned *pecx, unsigned *pedx)
        {
            int tmp[4] = { 0 };
            __cpuid(tmp, level);
            *(peax) = (unsigned)tmp[0];
            *(pebx) = (unsigned)tmp[1];
            *(pecx) = (unsigned)tmp[2];
            *(pedx) = (unsigned)tmp[3];
            return 1;
        }
    #endif

#elif defined (__GNUC__) || defined(__clang__)

    #include <cpuid.h>
    #undef __cpuid // drop internal definition from cpuid.h

    #define __cpuid(info, function) \
        (void)__get_cpuid((function), \
                          (unsigned *)(info)+0, (unsigned *)(info)+1, \
                          (unsigned *)(info)+2, (unsigned *)(info)+3)

    #define __cpuidex(info, function, sub_function) \
        do { \
            __cpuid_count((function), (sub_function), (info)[0], (info)[1], (info)[2], (info)[3]); \
        } while(0)
#else

    #error "FIXME unknown compiler"

#endif

#endif /* UFO_PORTABLE_CPUID_H */
