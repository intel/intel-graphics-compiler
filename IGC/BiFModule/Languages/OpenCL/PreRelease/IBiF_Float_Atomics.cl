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