/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===-  IBiF_Float_Atomics.cl -===================================================//
//
// This file contain definitions of OpenCL 1.2 Atomic built-in functions.
//
//===----------------------------------------------------------------------===//
#include "IBiF_Header.cl"

#define DEF_ATOM_2SRC(KEY, ADDRSPACE, TYPE, IGC_TYPE, IGC_CL_TYPE) \
INLINE TYPE OVERLOADABLE atom_##KEY(volatile __##ADDRSPACE TYPE *p, TYPE val) { \
    return __builtin_IB_atomic_##KEY##_##ADDRSPACE##_##IGC_TYPE((volatile __##ADDRSPACE IGC_CL_TYPE *)p, val); \
} \
INLINE TYPE OVERLOADABLE atom_##KEY(__##ADDRSPACE TYPE *p, TYPE val) { \
    return __builtin_IB_atomic_##KEY##_##ADDRSPACE##_##IGC_TYPE((volatile __##ADDRSPACE IGC_CL_TYPE *)p, val); \
}

#define DEF_ATOMIC_2SRC(KEY, ADDRSPACE, TYPE, IGC_TYPE, IGC_CL_TYPE) \
INLINE TYPE OVERLOADABLE atomic_##KEY(volatile __##ADDRSPACE TYPE *p, TYPE val) { \
    return __builtin_IB_atomic_##KEY##_##ADDRSPACE##_##IGC_TYPE((volatile __##ADDRSPACE IGC_CL_TYPE *)p, val); \
} \

#define DEF_ATOMIC_3SRC(KEY, ADDRSPACE, TYPE, IGC_TYPE, IGC_CL_TYPE) \
INLINE TYPE OVERLOADABLE atomic_##KEY(volatile __##ADDRSPACE TYPE *p, TYPE cmp, TYPE val) { \
    return __builtin_IB_atomic_##KEY##_##ADDRSPACE##_##IGC_TYPE((volatile __##ADDRSPACE IGC_CL_TYPE *)p, cmp, val); \
}

#if defined(cl_intel_global_float_atomics)
//atom_add
DEF_ATOM_2SRC(add, global, float, f32, float)
DEF_ATOMIC_2SRC(add, global, float, f32, float)
//atom_sub
DEF_ATOM_2SRC(sub, global, float, f32, float)
DEF_ATOMIC_2SRC(sub, global, float, f32, float)
#endif // defined(cl_intel_global_float_atomics)

//atom_min
DEF_ATOM_2SRC(min, global, float, f32, float)
DEF_ATOM_2SRC(min, local, float, f32, float)

//atom_max
DEF_ATOM_2SRC(max, global, float, f32, float)
DEF_ATOM_2SRC(max, local, float, f32, float)

//atomic_min
DEF_ATOMIC_2SRC(min, global, float, f32, float)
DEF_ATOMIC_2SRC(min, local, float, f32, float)

//atomic_max
DEF_ATOMIC_2SRC(max, global, float, f32, float)
DEF_ATOMIC_2SRC(max, local, float, f32, float)

// atomic_cmpxchg
DEF_ATOMIC_3SRC(cmpxchg, global, float, f32, float)
DEF_ATOMIC_3SRC(cmpxchg, local, float, f32, float)
