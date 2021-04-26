/*========================== begin_copyright_notice ============================

Copyright (c) 2016-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

// SVML code
/*
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 1996-2010 Intel Corporation. All Rights Reserved.
//
*/

extern __constant int __FlushDenormals;
extern __constant int __FastRelaxedMath;
extern __constant int __APIRS;
extern __constant int __SVML;
extern __constant int __CRMacros;
extern __constant int __UseMathWithLUT;

OVERLOADABLE int __intel_relaxed_isnan(float x );
OVERLOADABLE int __intel_relaxed_isinf(float x );
OVERLOADABLE int __intel_relaxed_isfinite(float x );
OVERLOADABLE int __intel_relaxed_isnormal(float x );

#if defined(cl_khr_fp16)
OVERLOADABLE int __intel_relaxed_isinf(half x );
OVERLOADABLE int __intel_relaxed_isnan(half x );
OVERLOADABLE int __intel_relaxed_isfinite(half x );
OVERLOADABLE int __intel_relaxed_isnormal(half x );
#endif // defined(cl_khr_fp16)


// Common file for intrinsics implementation used in Math library
#include "IBiF_Intrinsics_Impl.cl"

// Common
#include "Common/fclamp.cl"
#include "Common/degrees.cl"
#include "Common/fmax_common.cl"
#include "Common/fmin_common.cl"
#include "Common/mix.cl"
#include "Common/radians.cl"
#include "Common/sign.cl"
#include "Common/smoothstep.cl"
#include "Common/step.cl"

// Geometric
#include "Geometric/cross.cl"
#include "Geometric/distance.cl"
#include "Geometric/fast_distance.cl"
#include "Geometric/fast_length.cl"
#include "Geometric/fast_normalize.cl"
#include "ExternalLibraries/libclc/normalize.cl"
#include "ExternalLibraries/libclc/length.cl"

// Half
#include "Half/half_cos.cl"
#include "Half/half_divide.cl"
#include "Half/half_exp.cl"
#include "Half/half_exp10.cl"
#include "Half/half_exp2.cl"
#include "Half/half_log.cl"
#include "Half/half_log10.cl"
#include "Half/half_log2.cl"
#include "Half/half_powr.cl"
#include "Half/half_recip.cl"
#include "Half/half_rsqrt.cl"
#include "Half/half_sin.cl"
#include "Half/half_sqrt.cl"
#include "Half/half_tan.cl"

// Integer
#include "Integer/abs.cl"
#include "Integer/abs_diff.cl"
#include "Integer/add_sat.cl"
#include "Integer/clamp.cl"
#include "Integer/clz.cl"
#include "Integer/ctz.cl"
#include "Integer/hadd.cl"
#include "Integer/mad24.cl"
#include "Integer/mad_hi.cl"
#include "Integer/mad_sat.cl"
#include "Integer/max.cl"
#include "Integer/min.cl"
#include "Integer/mul24.cl"
#include "Integer/mul_hi.cl"
#include "Integer/popcnt.cl"
#include "Integer/rhadd.cl"
#include "Integer/rotate.cl"
#include "Integer/sub_sat.cl"
#include "Integer/upsample.cl"

// Math
#include "Math/math.cl"

// Native
#include "Native/native_cos.cl"
#include "Native/native_divide.cl"
#include "Native/native_exp.cl"
#include "Native/native_exp10.cl"
#include "Native/native_exp2.cl"
#include "Native/native_log.cl"
#include "Native/native_log10.cl"
#include "Native/native_log2.cl"
#include "Native/native_powr.cl"
#include "Native/native_recip.cl"
#include "Native/native_rsqrt.cl"
#include "Native/native_sin.cl"
#include "Native/native_sqrt.cl"
#include "Native/native_tan.cl"

// Relational
#include "Relational/bitselect.cl"
#include "Relational/isfinite.cl"
#include "Relational/isinf.cl"
#include "Relational/isnan.cl"
#include "Relational/select.cl"
