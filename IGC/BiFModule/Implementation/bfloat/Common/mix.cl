/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

INLINE bfloat __attribute__((overloadable)) __spirv_ocl_mix(bfloat x, bfloat y, bfloat a ){
    return __spirv_ocl_mad( ( y - x ), a, x );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( mix, bfloat, bfloat, )
