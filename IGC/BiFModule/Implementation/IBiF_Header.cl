/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

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

#include "IGCBiF_Intrinsics.cl"

#include "spirv.h"
#include "IBiF_Macros.cl"

#define INLINE __attribute__((always_inline))
#define OPTNONE __attribute__((optnone))
#define OVERLOADABLE __attribute__((overloadable))

//Undef cl_khr_fp64 since native fp64 builtins are not supported. For double support we need to use the placeholder.

#endif //_IBIF_HEADER_
