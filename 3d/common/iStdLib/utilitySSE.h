/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#if defined(_WIN32)
    #include <intrin.h>
#else
    #include <x86intrin.h>
#endif

#include "types.h"


namespace iSTD
{

/*****************************************************************************\
Inline Function:
    FastClamp

Description:
    Fast clamping implementation(s) for 4xfloats
\*****************************************************************************/
__forceinline void FastClampF(  const __m128 &inMins, 
                              const __m128 &inMaxs, 
                              float* oDest)
{
    // load data to be clamped into 128 register
    __m128 vals = _mm_loadu_ps(oDest);

    // clamp
    vals = _mm_min_ps(inMaxs, _mm_max_ps(vals, inMins));

    // load into output
    _mm_storeu_ps(oDest, vals);
}

__forceinline void FastClampF(  const __m128 &inMins, 
                                const __m128 &inMaxs, 
                                float* oDest, 
                                const float* inSrc )
{
    // load data to be clamped into 128 register
    __m128 vals = _mm_loadu_ps(inSrc);

    // clamp
    vals = _mm_min_ps(inMaxs, _mm_max_ps(vals, inMins));

    // load into output
    _mm_storeu_ps(oDest, vals);
}

__forceinline void FastClampF(  const __m128 &inMins, 
                              const __m128 &inMaxs, 
                              float* oDest, 
                              const __m128 &inSrc )
{
    // clamp
    __m128 vals = _mm_min_ps(inMaxs, _mm_max_ps(inSrc, inMins));

    // load into output
    _mm_storeu_ps(oDest, vals);
}

} // iSTD
