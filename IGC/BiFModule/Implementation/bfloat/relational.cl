/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "spirv_bfloat.h"
#include "include/BiF_Definitions.cl"

bool  __attribute__((overloadable)) __spirv_IsNan(bfloat x)
{
    return x != x;
}
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsNan, __bool, bfloat, )
bool __attribute__((overloadable)) __spirv_IsInf(bfloat x)
{
    return __spirv_ocl_fabs(x) == (bfloat)(INFINITY);
}
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsInf, __bool, bfloat, )
bool __attribute__((overloadable)) __spirv_IsFinite(bfloat x)
{
    return __spirv_ocl_fabs(x) < (bfloat)(INFINITY);
}
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsFinite, __bool, bfloat, )
bool __attribute__((overloadable)) __spirv_IsNormal(bfloat x)
{
    return __spirv_IsFinite(x) & (__spirv_ocl_fabs(x) >= (bfloat)FLT_MIN);
}
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsNormal, __bool, bfloat, )
bool __attribute__((overloadable)) __spirv_SignBitSet(bfloat x)
{
    return (as_ushort(x) & BFLOAT_SIGN_MASK) != 0;
}
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(SignBitSet, __bool, bfloat, )
bool __attribute__((overloadable)) __spirv_LessOrGreater(bfloat x, bfloat y)
{
    return (x < y) | (x > y);
}
SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(LessOrGreater, __bool, bfloat, )

#include "Relational/bitselect.cl"
#include "Relational/isnan.cl"
#include "Relational/select.cl"
