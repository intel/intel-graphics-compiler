/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

INLINE float OVERLOADABLE half_exp(float x) { return __spirv_ocl_half_exp(x); }

INLINE float2 OVERLOADABLE half_exp(float2 x) { return __spirv_ocl_half_exp(x); }

INLINE float3 OVERLOADABLE half_exp(float3 x) { return __spirv_ocl_half_exp(x); }

INLINE float4 OVERLOADABLE half_exp(float4 x) { return __spirv_ocl_half_exp(x); }

INLINE float8 OVERLOADABLE half_exp(float8 x) { return __spirv_ocl_half_exp(x); }

INLINE float16 OVERLOADABLE half_exp(float16 x) { return __spirv_ocl_half_exp(x); }
