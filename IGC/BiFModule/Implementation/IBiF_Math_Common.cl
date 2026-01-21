/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// SVML code

extern __constant int __SVML;

OVERLOADABLE int __intel_relaxed_isnan(float x );
OVERLOADABLE int __intel_relaxed_isinf(float x );
OVERLOADABLE int __intel_relaxed_isfinite(float x );
OVERLOADABLE int __intel_relaxed_isnormal(float x );

#if defined(cl_khr_fp64)
OVERLOADABLE int __intel_relaxed_isnan(double x );
OVERLOADABLE int __intel_relaxed_isinf(double x );
OVERLOADABLE int __intel_relaxed_isfinite(double x );
OVERLOADABLE int __intel_relaxed_isnormal(double x );
#endif // defined(cl_khr_fp64)

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
#include "ExternalLibraries/libclc/doubles.cl"

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
#include "Math/acos.cl"
#include "Math/acosh.cl"
#include "Math/acospi.cl"
#include "Math/asin.cl"
#include "Math/asinh.cl"
#include "Math/asinpi.cl"
#include "Math/atan.cl"
#include "Math/atan2.cl"
#include "Math/atan2pi.cl"
#include "Math/atanh.cl"
#include "Math/atanpi.cl"
#include "Math/cbrt.cl"
#include "Math/ceil.cl"
#include "Math/copysign.cl"
#include "Math/cos.cl"
#include "Math/cosh.cl"
#include "Math/cospi.cl"
#include "Math/divide_cr.cl"
#include "Math/erf.cl"
#include "Math/erfc.cl"
#include "Math/exp.cl"
#include "Math/exp10.cl"
#include "Math/exp2.cl"
#include "Math/expm1.cl"
#include "Math/fabs.cl"
#include "Math/fdim.cl"
#include "Math/floor.cl"
#include "Math/fma.cl"
#include "Math/fmax.cl"
#include "Math/fmin.cl"
#include "Math/fmod.cl"
#include "Math/hypot.cl"
#include "Math/ilogb.cl"
#include "Math/ldexp.cl"
#include "Math/lgamma.cl"
#include "Math/log.cl"
#include "Math/log10.cl"
#include "Math/log1p.cl"
#include "Math/log2.cl"
#include "Math/logb.cl"
#include "Math/mad.cl"
#include "Math/maxmag.cl"
#include "Math/minmag.cl"
#include "Math/nan.cl"
#include "Math/nextafter.cl"
#include "Math/pow.cl"
#include "Math/pown.cl"
#include "Math/powr.cl"
#include "Math/remainder.cl"
#include "Math/rint.cl"
#include "Math/rootn.cl"
#include "Math/round.cl"
#include "Math/rsqrt.cl"
#include "Math/sin.cl"
#include "Math/sinh.cl"
#include "Math/sinpi.cl"
#include "Math/sqrt.cl"
#include "Math/sqrt_cr.cl"
#include "Math/tan.cl"
#include "Math/tanh.cl"
#include "Math/sigm.cl"
#include "Math/tanpi.cl"
#include "Math/tgamma.cl"
#include "Math/trunc.cl"

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

#include "Math/fract.cl"
#include "Math/frexp.cl"
#include "Math/lgamma_r.cl"
#include "Math/modf.cl"
#include "Math/remquo.cl"
#include "Math/sincos.cl"

// IMF
#include "IMF/IBiF_IMF_All.cl"
