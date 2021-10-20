/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_s_max, char, char, i8 )
GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_u_max, uchar, uchar, i8 )
GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_s_max, short, short, i16 )
GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_u_max, ushort, ushort, i16 )
GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_s_max, int, int, i32 )
GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_u_max, uint, uint, i32 )
GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_s_max, long, long, i64 )
GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_u_max, ulong, ulong, i64 )

