/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


bfloat __attribute__((overloadable)) __spirv_ocl_length(bfloat p) {
    return __spirv_ocl_fabs(p);
}

// TODO: Should we implement it more efficiently without conversions to float vectors?

bfloat __attribute__((overloadable)) __spirv_ocl_length(bfloat2 p) {
    // clang does not support bfloat -> float conversion for vector types directly
    float2 fp = { (float)p.x, (float)p.y };
    return __spirv_ocl_length(fp);
}

bfloat __attribute__((overloadable)) __spirv_ocl_length(bfloat3 p) {
    float3 fp = { (float)p.x, (float)p.y, (float)p.z };
    return __spirv_ocl_length(fp);
}

bfloat __attribute__((overloadable)) __spirv_ocl_length(bfloat4 p) {
    float4 fp = { (float)p.x, (float)p.y, (float)p.z, (float)p.w };
    return __spirv_ocl_length(fp);
}
