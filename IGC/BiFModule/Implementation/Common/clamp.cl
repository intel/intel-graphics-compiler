/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_fclamp, float, float, f32 )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VSS( __builtin_spirv_OpenCL_fclamp, float, float, float, f32 )

#if defined(cl_khr_fp64)

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_fclamp, double, double, f64 )

#endif // defined(cl_khr_fp64)

#ifdef cl_khr_fp16

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_fclamp, half, half, f16 )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VSS( __builtin_spirv_OpenCL_fclamp, half, half, half, f16 )

#endif // defined(cl_khr_fp16)
