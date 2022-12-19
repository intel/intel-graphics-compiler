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
    return SPIRV_BUILTIN(ConvertFToS, _i64_f32, _Rlong)( _T );
}

ulong OVERLOADABLE convert_ulong(float _T) {
    return SPIRV_BUILTIN(ConvertFToU, _i64_f32, _Rulong)( _T );
}

float OVERLOADABLE convert_float(long _T) {
    return SPIRV_BUILTIN(ConvertSToF, _f32_i64, _Rfloat)( _T );
}

float OVERLOADABLE convert_float_rtn(long _T) {
    return SPIRV_BUILTIN(ConvertSToF, _RTN_f32_i64, _Rfloat_rtn)( _T );
}

float OVERLOADABLE convert_float_rtp(long _T) {
    return SPIRV_BUILTIN(ConvertSToF, _RTP_f32_i64, _Rfloat_rtp)( _T );
}

float OVERLOADABLE convert_float_rtz(long _T) {
    return SPIRV_BUILTIN(ConvertSToF, _RTZ_f32_i64, _Rfloat_rtz)( _T );
}

float OVERLOADABLE convert_float(ulong _T) {
    return SPIRV_BUILTIN(ConvertUToF, _f32_i64, _Rfloat)( _T );
}

float OVERLOADABLE convert_float_rtn(ulong _T) {
    return SPIRV_BUILTIN(ConvertUToF, _RTN_f32_i64, _Rfloat_rtn)( _T );
}

float OVERLOADABLE convert_float_rtp(ulong _T) {
    return SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i64, _Rfloat_rtp)( _T );
}

float OVERLOADABLE convert_float_rtz(ulong _T) {
    return SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i64, _Rfloat_rtz)( _T );
}

#if defined(cl_khr_fp64)
double OVERLOADABLE convert_double_rtn(long _T) {
    return SPIRV_BUILTIN(ConvertSToF, _RTN_f64_i64, _Rdouble_rtn)( _T );
}

double OVERLOADABLE convert_double_rtp(long _T) {
    return SPIRV_BUILTIN(ConvertSToF, _RTP_f64_i64, _Rdouble_rtp)( _T );
}

double OVERLOADABLE convert_double_rtz(long _T) {
    return SPIRV_BUILTIN(ConvertSToF, _RTZ_f64_i64, _Rdouble_rtz)( _T );
}

double OVERLOADABLE convert_double_rtn(ulong _T) {
    return SPIRV_BUILTIN(ConvertUToF, _RTN_f64_i64, _Rdouble_rtn)( _T );
}

double OVERLOADABLE convert_double_rtp(ulong _T) {
    return SPIRV_BUILTIN(ConvertUToF, _RTP_f64_i64, _Rdouble_rtp)( _T );
}

double OVERLOADABLE convert_double_rtz(ulong _T) {
    return SPIRV_BUILTIN(ConvertUToF, _RTZ_f64_i64, _Rdouble_rtz)( _T );
}
#endif // defined(cl_khr_fp64)
