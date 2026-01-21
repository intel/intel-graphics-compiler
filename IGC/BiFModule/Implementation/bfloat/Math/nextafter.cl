/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

bfloat __attribute__((overloadable)) __spirv_ocl_nextafter( bfloat x, bfloat y )
{
    const short maxneg = BFLOAT_SIGN_MASK;

    short smix = as_short(x);
    short nx = maxneg - smix;
    short tcix = ( smix < 0 ) ? nx : smix;

    short smiy = as_short(y);
    short ny = maxneg - smiy;
    short tciy = ( smiy < 0 ) ? ny : smiy;

    short delta = ( tcix < tciy ) ? 1 : -1;

    short tcir = tcix + delta;
    short nr = maxneg - tcir;
    short smir = ( tcir < 0 ) ? nr : tcir;

    bfloat result = as_bfloat(smir);
    result = (tcix == tciy) ? y : result;

    {
        // Use __spirv_ocl_nan once it's implemented for bfloat
        //bfloat n = __spirv_ocl_nan(0);
        bfloat n = as_bfloat( BFLOAT_QUIET_NAN );
        int test = __spirv_IsNan(x) | __spirv_IsNan(y);
        result = test ? n : result;
    }

    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS( nextafter, bfloat, bfloat, )
