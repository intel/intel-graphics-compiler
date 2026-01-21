/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "spirv_bfloat.h"

INLINE bfloat3 __attribute__((overloadable)) __spirv_ocl_cross(bfloat3 p0, bfloat3 p1 ){
    float3 fp0 = { (float)p0.x, (float)p0.y, (float)p0.z };
    float3 fp1 = { (float)p1.x, (float)p1.y, (float)p1.z };
    float3 result = __spirv_ocl_cross(fp0, fp1);
    return (bfloat3)( (bfloat)result.x, (bfloat)result.y, (bfloat)result.z );
}

INLINE bfloat4 __attribute__((overloadable)) __spirv_ocl_cross(bfloat4 p0, bfloat4 p1 ){
    float4 fp0 = { (float)p0.x, (float)p0.y, (float)p0.z, (float)p0.w };
    float4 fp1 = { (float)p1.x, (float)p1.y, (float)p1.z, (float)p1.w };
    float4 result = __spirv_ocl_cross(fp0, fp1);
    return (bfloat4)( (bfloat)result.x, (bfloat)result.y, (bfloat)result.z, (bfloat)result.w );
}
