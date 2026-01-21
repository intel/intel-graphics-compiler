/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


INLINE bfloat __attribute__((overloadable)) __spirv_ocl_normalize(bfloat p ){
    return __spirv_ocl_sign(p);
}

// TODO: Below functions can probably be implemented more efficiently using native bfloat operations

bfloat2 __attribute__((overloadable)) __spirv_ocl_normalize(bfloat2 p ){
  float2 fp = { (float)p.x, (float)p.y };
  float2 result = __spirv_ocl_normalize(fp);
  return (bfloat2)( (bfloat)result.x, (bfloat)result.y );
}

bfloat3 __attribute__((overloadable)) __spirv_ocl_normalize(bfloat3 p ){
  float3 fp = { (float)p.x, (float)p.y, (float)p.z };
  float3 result = __spirv_ocl_normalize(fp);
  return (bfloat3)( (bfloat)result.x, (bfloat)result.y, (bfloat)result.z );
}

bfloat4 __attribute__((overloadable)) __spirv_ocl_normalize(bfloat4 p ){
  float4 fp = { (float)p.x, (float)p.y, (float)p.z, (float)p.w };
  float4 result = __spirv_ocl_normalize(fp);
  return (bfloat4)( (bfloat)result.x, (bfloat)result.y, (bfloat)result.z, (bfloat)result.w );
}
