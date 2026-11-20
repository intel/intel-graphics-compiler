/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.h"
#include "spirv.h"

INLINE float OVERLOADABLE fast_normalize(float p) { return __spirv_ocl_fast_normalize(p); }

INLINE float2 OVERLOADABLE fast_normalize(float2 p) { return __spirv_ocl_fast_normalize(p); }

INLINE float3 OVERLOADABLE fast_normalize(float3 p) { return __spirv_ocl_fast_normalize(p); }

INLINE float4 OVERLOADABLE fast_normalize(float4 p) { return __spirv_ocl_fast_normalize(p); }
