/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// SVML code

// This file defines target independent versions of OpenCL explicit
// long<->float and long<->double conversion functions.

#include "include/BiF_Definitions.cl"
#include "spirv.h"

long OVERLOADABLE convert_long(float _T) {
    return __spirv_ConvertFToS_Rlong( _T );
}

ulong OVERLOADABLE convert_ulong(float _T) {
    return __spirv_ConvertFToU_Rulong( _T );
}

float OVERLOADABLE convert_float(long _T) {
    return __spirv_ConvertSToF_Rfloat( _T );
}

float OVERLOADABLE convert_float_rtn(long _T) {
    return __spirv_ConvertSToF_Rfloat_rtn( _T );
}

float OVERLOADABLE convert_float_rtp(long _T) {
    return __spirv_ConvertSToF_Rfloat_rtp( _T );
}

float OVERLOADABLE convert_float_rtz(long _T) {
    return __spirv_ConvertSToF_Rfloat_rtz( _T );
}

float OVERLOADABLE convert_float(ulong _T) {
    return __spirv_ConvertUToF_Rfloat( _T );
}

float OVERLOADABLE convert_float_rtn(ulong _T) {
    return __spirv_ConvertUToF_Rfloat_rtn( _T );
}

float OVERLOADABLE convert_float_rtp(ulong _T) {
    return __spirv_ConvertUToF_Rfloat_rtp( _T );
}

float OVERLOADABLE convert_float_rtz(ulong _T) {
    return __spirv_ConvertUToF_Rfloat_rtz( _T );
}

#if defined(cl_khr_fp64)
double OVERLOADABLE convert_double_rtn(long _T) {
    return __spirv_ConvertSToF_Rdouble_rtn( _T );
}

double OVERLOADABLE convert_double_rtp(long _T) {
    return __spirv_ConvertSToF_Rdouble_rtp( _T );
}

double OVERLOADABLE convert_double_rtz(long _T) {
    return __spirv_ConvertSToF_Rdouble_rtz( _T );
}

double OVERLOADABLE convert_double_rtn(ulong _T) {
    return __spirv_ConvertUToF_Rdouble_rtn( _T );
}

double OVERLOADABLE convert_double_rtp(ulong _T) {
    return __spirv_ConvertUToF_Rdouble_rtp( _T );
}

double OVERLOADABLE convert_double_rtz(ulong _T) {
    return __spirv_ConvertUToF_Rdouble_rtz( _T );
}
#endif // defined(cl_khr_fp64)
