/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
// SVML code
/*
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 1996-2010 Intel Corporation. All Rights Reserved.
//
*/

//===-  IBiF_Conversions_long_emulation.cl -==========================//
//
// This file defines target independent versions of OpenCL explicit
// long<->float and long<->double conversion functions.
//
//===----------------------------------------------------------------------===//

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
    return SPIRV_BUILTIN(ConvertSToF, _RTN_f32_i64, _rtn_Rfloat)( _T );
}

float OVERLOADABLE convert_float_rtp(long _T) {
    return SPIRV_BUILTIN(ConvertSToF, _RTP_f32_i64, _rtp_Rfloat)( _T );
}

float OVERLOADABLE convert_float_rtz(long _T) {
    return SPIRV_BUILTIN(ConvertSToF, _RTZ_f32_i64, _rtz_Rfloat)( _T );
}

float OVERLOADABLE convert_float(ulong _T) {
    return SPIRV_BUILTIN(ConvertUToF, _f32_i64, _Rfloat)( _T );
}

float OVERLOADABLE convert_float_rtn(ulong _T) {
    return SPIRV_BUILTIN(ConvertUToF, _RTN_f32_i64, _rtn_Rfloat)( _T );
}

float OVERLOADABLE convert_float_rtp(ulong _T) {
    return SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i64, _rtp_Rfloat)( _T );
}

float OVERLOADABLE convert_float_rtz(ulong _T) {
    return SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i64, _rtz_Rfloat)( _T );
}

#if defined(cl_khr_fp64)
double OVERLOADABLE convert_double_rtn(long _T) {
    return SPIRV_BUILTIN(ConvertSToF, _RTN_f64_i64, _rtn_Rdouble)( _T );
}

double OVERLOADABLE convert_double_rtp(long _T) {
    return SPIRV_BUILTIN(ConvertSToF, _RTP_f64_i64, _rtp_Rdouble)( _T );
}

double OVERLOADABLE convert_double_rtz(long _T) {
    return SPIRV_BUILTIN(ConvertSToF, _RTZ_f64_i64, _rtz_Rdouble)( _T );
}

double OVERLOADABLE convert_double_rtn(ulong _T) {
    return SPIRV_BUILTIN(ConvertUToF, _RTN_f64_i64, _rtn_Rdouble)( _T );
}

double OVERLOADABLE convert_double_rtp(ulong _T) {
    return SPIRV_BUILTIN(ConvertUToF, _RTP_f64_i64, _rtp_Rdouble)( _T );
}

double OVERLOADABLE convert_double_rtz(ulong _T) {
    return SPIRV_BUILTIN(ConvertUToF, _RTZ_f64_i64, _rtz_Rdouble)( _T );
}
#endif // defined(cl_khr_fp64)
