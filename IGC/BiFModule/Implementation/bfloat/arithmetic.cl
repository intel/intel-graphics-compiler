/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


// Arithmetic Instructions - BFloat16 Dot Product

bfloat __attribute__((overloadable)) __spirv_Dot(bfloat2 Vector1, bfloat2 Vector2)
{
    return __spirv_ocl_fma(Vector1.x,  Vector2.x, (Vector1.y * Vector2.y));
}

bfloat __attribute__((overloadable)) __spirv_Dot(bfloat3 Vector1, bfloat3 Vector2)
{
    return __spirv_ocl_fma(Vector1.x, Vector2.x,
           __spirv_ocl_fma(Vector1.y, Vector2.y, (Vector1.z * Vector2.z)));
}

bfloat __attribute__((overloadable)) __spirv_Dot(bfloat4 Vector1, bfloat4 Vector2)
{
    return __spirv_ocl_fma(Vector1.x, Vector2.x,
           __spirv_ocl_fma(Vector1.y, Vector2.y,
           __spirv_ocl_fma(Vector1.z, Vector2.z,
                                           (Vector1.w * Vector2.w))));
}

// TODO: should we support beyond vec4 which is what OCL is limited to?
#if 0

bfloat __attribute__((overloadable)) __spirv_Dot(bfloat8 Vector1, bfloat8 Vector2)
{
}

bfloat __attribute__((overloadable)) __spirv_Dot(bfloat16 Vector1, bfloat16 Vector2)
{
}

#endif
