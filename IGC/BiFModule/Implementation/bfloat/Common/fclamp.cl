/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

INLINE bfloat __attribute__((overloadable)) __spirv_ocl_fclamp(bfloat x, bfloat minval, bfloat maxval ){
    return __spirv_ocl_fclamp((float)x, (float)minval, (float)maxval);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_3ARGS( fclamp, bfloat, bfloat, )
