/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

bfloat __attribute__((overloadable)) __spirv_ocl_degrees(bfloat r ){
    return as_bfloat(__builtin_bf16_mul(as_ushort(r), as_ushort((bfloat)ONE_EIGHTY_OVER_PI_FLT)));
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( degrees, bfloat, bfloat, )
