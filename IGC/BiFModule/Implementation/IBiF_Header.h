/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This file contains a set of macros and enums that should be included in each
// BiFImpl.

#ifndef _IBIF_HEADER_
#define _IBIF_HEADER_


#ifdef cl_khr_fp16
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#endif
#ifdef cl_khr_fp64
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#endif

#include "IGCBiF_Intrinsics.h"

#include "spirv.h"
#include "IBiF_Macros.h"

#define INLINE __attribute__((always_inline))
#define OPTNONE __attribute__((optnone))
#define OVERLOADABLE __attribute__((overloadable))

// Undef cl_khr_fp64 since native fp64 builtins are not supported. For double support we need to use the placeholder.

// The IMF implementations set this pragma for themselves. When the sources were
// compiled together it applied to every source after them, so keep it for all
// SPIR-V sources to preserve their numerics.
#pragma OPENCL FP_CONTRACT OFF

// Helpers shared across BiF modules.

extern __constant int __SVML;

void __intel_workgroup_barrier(int Memory, int Semantics);

OVERLOADABLE int __intel_relaxed_isnan(float x);
OVERLOADABLE int __intel_relaxed_isinf(float x);
OVERLOADABLE int __intel_relaxed_isfinite(float x);
OVERLOADABLE int __intel_relaxed_isnormal(float x);

#if defined(cl_khr_fp64)
OVERLOADABLE int __intel_relaxed_isnan(double x);
OVERLOADABLE int __intel_relaxed_isinf(double x);
OVERLOADABLE int __intel_relaxed_isfinite(double x);
OVERLOADABLE int __intel_relaxed_isnormal(double x);
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
OVERLOADABLE int __intel_relaxed_isinf(half x);
OVERLOADABLE int __intel_relaxed_isnan(half x);
OVERLOADABLE int __intel_relaxed_isfinite(half x);
OVERLOADABLE int __intel_relaxed_isnormal(half x);
#endif // defined(cl_khr_fp16)

#endif //_IBIF_HEADER_
