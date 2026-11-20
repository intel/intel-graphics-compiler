/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file contains a set of macros and enums that should be included in each
// BiFImpl.

#ifndef _IBIF_HEADER_
#define _IBIF_HEADER_

#define SUPPORT_ACCESS_QUAL_OVERLOAD 1


#ifdef cl_khr_fp16
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#endif
#ifdef cl_khr_fp64
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#endif
#ifdef cl_khr_int64_base_atomics
#pragma OPENCL EXTENSION cl_khr_int64_base_atomics : enable
#endif
#ifdef cl_khr_int64_extended_atomics
#pragma OPENCL EXTENSION cl_khr_int64_extended_atomics : enable
#endif

#include "../../Implementation/IGCBiF_Intrinsics.h"

#include "IBiF_Macros.h"

#define INLINE __attribute__((always_inline))
#define OVERLOADABLE __attribute__((overloadable))


#include "spirv.h"
#include "IBiF_Intrinsics_Impl.h"

// Helpers shared across BiF modules.

int OVERLOADABLE __intel_relaxed_isnan(float x);
int OVERLOADABLE __intel_relaxed_isinf(float x);
int OVERLOADABLE __intel_relaxed_isfinite(float x);
int OVERLOADABLE __intel_relaxed_isnormal(float x);

#if defined(cl_khr_fp64)
int OVERLOADABLE __intel_relaxed_isnan(double x);
int OVERLOADABLE __intel_relaxed_isinf(double x);
int OVERLOADABLE __intel_relaxed_isfinite(double x);
int OVERLOADABLE __intel_relaxed_isnormal(double x);
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
int OVERLOADABLE __intel_relaxed_isinf(half x);
int OVERLOADABLE __intel_relaxed_isnan(half x);
int OVERLOADABLE __intel_relaxed_isfinite(half x);
int OVERLOADABLE __intel_relaxed_isnormal(half x);
#endif // defined(cl_khr_fp16)

#endif //_IBIF_HEADER_
