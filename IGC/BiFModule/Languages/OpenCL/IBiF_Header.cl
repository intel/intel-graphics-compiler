/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

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

#include "../../Implementation/IGCBiF_Intrinsics.cl"

#include "IBiF_Macros.cl"

#define INLINE __attribute__((always_inline))
#define OVERLOADABLE __attribute__((overloadable))



#endif //_IBIF_HEADER_
