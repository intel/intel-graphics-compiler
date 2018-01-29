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

//===-  IGCBiF_Atomics.cl -===================================================//
//
// This file contain definitions of OpenCL 1.2 Atomic built-in functions.
//
//===----------------------------------------------------------------------===//

#undef atomic_add
#undef atomic_sub
#undef atomic_xchg
#undef atomic_min
#undef atomic_max
#undef atomic_and
#undef atomic_or
#undef atomic_xor
#undef atomic_inc
#undef atomic_dec
#undef atomic_cmpxchg

//-----------------------------------------------------------------------------
// OpenCL 1.2  Atomics
//-----------------------------------------------------------------------------

INLINE float OVERLOADABLE atomic_xchg(__global volatile float *p, float val) {
    return as_float( __builtin_IB_atomic_xchg_global_i32( (__global volatile int *)p, as_int(val) ) );
}

INLINE float OVERLOADABLE atomic_xchg(__local volatile float *p, float val) {
    return as_float( __builtin_IB_atomic_xchg_local_i32( (__local volatile int *)p, as_int(val) ) );
}


#define DEF_ATOMIC_1SRC(KEY, ADDRSPACE, TYPE, IGC_TYPE, IGC_CL_TYPE) \
INLINE TYPE OVERLOADABLE atomic_##KEY(volatile __##ADDRSPACE TYPE *p) { \
    return __builtin_IB_atomic_##KEY##_##ADDRSPACE##_##IGC_TYPE((volatile __##ADDRSPACE IGC_CL_TYPE *)p); \
}

#define DEF_ATOMIC_2SRC(KEY, ADDRSPACE, TYPE, IGC_TYPE, IGC_CL_TYPE) \
INLINE TYPE OVERLOADABLE atomic_##KEY(volatile __##ADDRSPACE TYPE *p, TYPE val) { \
    return __builtin_IB_atomic_##KEY##_##ADDRSPACE##_##IGC_TYPE((volatile __##ADDRSPACE IGC_CL_TYPE *)p, val); \
}

// Implement sub as negated add.
#define DEF_ATOMIC_SUB(KEY, ADDRSPACE, TYPE, IGC_TYPE, IGC_CL_TYPE) \
INLINE TYPE OVERLOADABLE atomic_##KEY(volatile __##ADDRSPACE TYPE *p, TYPE val) { \
    return __builtin_IB_atomic_add##_##ADDRSPACE##_##IGC_TYPE((volatile __##ADDRSPACE IGC_CL_TYPE *)p, -val); \
}

#define DEF_ATOMIC_3SRC(KEY, ADDRSPACE, TYPE, IGC_TYPE, IGC_CL_TYPE) \
INLINE TYPE OVERLOADABLE atomic_##KEY(volatile __##ADDRSPACE TYPE *p, TYPE cmp, TYPE val) { \
    return __builtin_IB_atomic_##KEY##_##ADDRSPACE##_##IGC_TYPE((volatile __##ADDRSPACE IGC_CL_TYPE *)p, cmp, val); \
}

// atom_*() functions need non-volatile overloads for backwards compatibility.

#define DEF_ATOM_1SRC(KEY, ADDRSPACE, TYPE, IGC_TYPE, IGC_CL_TYPE) \
INLINE TYPE OVERLOADABLE atom_##KEY(volatile __##ADDRSPACE TYPE *p) { \
    return __builtin_IB_atomic_##KEY##_##ADDRSPACE##_##IGC_TYPE((volatile __##ADDRSPACE IGC_CL_TYPE *)p); \
} \
INLINE TYPE OVERLOADABLE atom_##KEY(__##ADDRSPACE TYPE *p) { \
    return __builtin_IB_atomic_##KEY##_##ADDRSPACE##_##IGC_TYPE((volatile __##ADDRSPACE IGC_CL_TYPE *)p); \
}

#define DEF_ATOM_2SRC(KEY, ADDRSPACE, TYPE, IGC_TYPE, IGC_CL_TYPE) \
INLINE TYPE OVERLOADABLE atom_##KEY(volatile __##ADDRSPACE TYPE *p, TYPE val) { \
    return __builtin_IB_atomic_##KEY##_##ADDRSPACE##_##IGC_TYPE((volatile __##ADDRSPACE IGC_CL_TYPE *)p, val); \
} \
INLINE TYPE OVERLOADABLE atom_##KEY(__##ADDRSPACE TYPE *p, TYPE val) { \
    return __builtin_IB_atomic_##KEY##_##ADDRSPACE##_##IGC_TYPE((volatile __##ADDRSPACE IGC_CL_TYPE *)p, val); \
}

// Implement sub as negated add.
#define DEF_ATOM_SUB(KEY, ADDRSPACE, TYPE, IGC_TYPE, IGC_CL_TYPE) \
INLINE TYPE OVERLOADABLE atom_##KEY(volatile __##ADDRSPACE TYPE *p, TYPE val) { \
    return __builtin_IB_atomic_add##_##ADDRSPACE##_##IGC_TYPE((volatile __##ADDRSPACE IGC_CL_TYPE *)p, -val); \
} \
INLINE TYPE OVERLOADABLE atom_##KEY(__##ADDRSPACE TYPE *p, TYPE val) { \
    return __builtin_IB_atomic_add##_##ADDRSPACE##_##IGC_TYPE((volatile __##ADDRSPACE IGC_CL_TYPE *)p, -val); \
}

#define DEF_ATOM_3SRC(KEY, ADDRSPACE, TYPE, IGC_TYPE, IGC_CL_TYPE) \
INLINE TYPE OVERLOADABLE atom_##KEY(volatile __##ADDRSPACE TYPE *p, TYPE cmp, TYPE val) { \
    return __builtin_IB_atomic_##KEY##_##ADDRSPACE##_##IGC_TYPE((volatile __##ADDRSPACE IGC_CL_TYPE *)p, cmp, val); \
} \
INLINE TYPE OVERLOADABLE atom_##KEY(__##ADDRSPACE TYPE *p, TYPE cmp, TYPE val) { \
    return __builtin_IB_atomic_##KEY##_##ADDRSPACE##_##IGC_TYPE((volatile __##ADDRSPACE IGC_CL_TYPE *)p, cmp, val); \
}

// atomic_add
DEF_ATOMIC_2SRC(add, global, int, i32, int)
DEF_ATOMIC_2SRC(add, global, uint, i32, int)
DEF_ATOMIC_2SRC(add, local, int, i32, int)
DEF_ATOMIC_2SRC(add, local, uint, i32, int)

// atomic_sub
DEF_ATOMIC_SUB(sub, global, int, i32, int)
DEF_ATOMIC_SUB(sub, global, uint, i32, int)
DEF_ATOMIC_SUB(sub, local, int, i32, int)
DEF_ATOMIC_SUB(sub, local, uint, i32, int)

// atomic_xchg
DEF_ATOMIC_2SRC(xchg, global, int, i32, int)
DEF_ATOMIC_2SRC(xchg, global, uint, i32, int)
DEF_ATOMIC_2SRC(xchg, local, int, i32, int)
DEF_ATOMIC_2SRC(xchg, local, uint, i32, int)

// atomic_min
DEF_ATOMIC_2SRC(min, global, int, i32, int)
DEF_ATOMIC_2SRC(min, global, uint, u32, uint)
DEF_ATOMIC_2SRC(min, local, int, i32, int)
DEF_ATOMIC_2SRC(min, local, uint, u32, uint)

// atomic_max
DEF_ATOMIC_2SRC(max, global, int, i32, int)
DEF_ATOMIC_2SRC(max, global, uint, u32, uint)
DEF_ATOMIC_2SRC(max, local, int, i32, int)
DEF_ATOMIC_2SRC(max, local, uint, u32, uint)

// atomic_and
DEF_ATOMIC_2SRC(and, global, int, i32, int)
DEF_ATOMIC_2SRC(and, global, uint, i32, int)
DEF_ATOMIC_2SRC(and, local, int, i32, int)
DEF_ATOMIC_2SRC(and, local, uint, i32, int)

// atomic_or
DEF_ATOMIC_2SRC(or, global, int, i32, int)
DEF_ATOMIC_2SRC(or, global, uint, i32, int)
DEF_ATOMIC_2SRC(or, local, int, i32, int)
DEF_ATOMIC_2SRC(or, local, uint, i32, int)

// atomic_xor
DEF_ATOMIC_2SRC(xor, global, int, i32, int)
DEF_ATOMIC_2SRC(xor, global, uint, i32, int)
DEF_ATOMIC_2SRC(xor, local, int, i32, int)
DEF_ATOMIC_2SRC(xor, local, uint, i32, int)

// atomic_inc
DEF_ATOMIC_1SRC(inc, global, int, i32, int)
DEF_ATOMIC_1SRC(inc, global, uint, i32, int)
DEF_ATOMIC_1SRC(inc, local, int, i32, int)
DEF_ATOMIC_1SRC(inc, local, uint, i32, int)

// atomic_dec
DEF_ATOMIC_1SRC(dec, global, int, i32, int)
DEF_ATOMIC_1SRC(dec, global, uint, i32, int)
DEF_ATOMIC_1SRC(dec, local, int, i32, int)
DEF_ATOMIC_1SRC(dec, local, uint, i32, int)

// atomic_cmpxchg
DEF_ATOMIC_3SRC(cmpxchg, global, int, i32, int)
DEF_ATOMIC_3SRC(cmpxchg, global, uint, i32, int)
DEF_ATOMIC_3SRC(cmpxchg, local, int, i32, int)
DEF_ATOMIC_3SRC(cmpxchg, local, uint, i32, int)

// atom_add
DEF_ATOM_2SRC(add, global, int, i32, int)
DEF_ATOM_2SRC(add, global, uint, i32, int)
DEF_ATOM_2SRC(add, local, int, i32, int)
DEF_ATOM_2SRC(add, local, uint, i32, int)

// atom_sub
DEF_ATOM_SUB(sub, global, int, i32, int)
DEF_ATOM_SUB(sub, global, uint, i32, int)
DEF_ATOM_SUB(sub, local, int, i32, int)
DEF_ATOM_SUB(sub, local, uint, i32, int)

// atom_xchg
DEF_ATOM_2SRC(xchg, global, int, i32, int)
DEF_ATOM_2SRC(xchg, global, uint, i32, int)
DEF_ATOM_2SRC(xchg, local, int, i32, int)
DEF_ATOM_2SRC(xchg, local, uint, i32, int)

// atom_min
DEF_ATOM_2SRC(min, global, int, i32, int)
DEF_ATOM_2SRC(min, global, uint, u32, uint)
DEF_ATOM_2SRC(min, local, int, i32, int)
DEF_ATOM_2SRC(min, local, uint, u32, uint)

// atom_max
DEF_ATOM_2SRC(max, global, int, i32, int)
DEF_ATOM_2SRC(max, global, uint, u32, uint)
DEF_ATOM_2SRC(max, local, int, i32, int)
DEF_ATOM_2SRC(max, local, uint, u32, uint)

// atom_and
DEF_ATOM_2SRC(and, global, int, i32, int)
DEF_ATOM_2SRC(and, global, uint, i32, int)
DEF_ATOM_2SRC(and, local, int, i32, int)
DEF_ATOM_2SRC(and, local, uint, i32, int)

// atom_or
DEF_ATOM_2SRC(or, global, int, i32, int)
DEF_ATOM_2SRC(or, global, uint, i32, int)
DEF_ATOM_2SRC(or, local, int, i32, int)
DEF_ATOM_2SRC(or, local, uint, i32, int)

// atom_xor
DEF_ATOM_2SRC(xor, global, int, i32, int)
DEF_ATOM_2SRC(xor, global, uint, i32, int)
DEF_ATOM_2SRC(xor, local, int, i32, int)
DEF_ATOM_2SRC(xor, local, uint, i32, int)

// atom_inc
DEF_ATOM_1SRC(inc, global, int, i32, int)
DEF_ATOM_1SRC(inc, global, uint, i32, int)
DEF_ATOM_1SRC(inc, local, int, i32, int)
DEF_ATOM_1SRC(inc, local, uint, i32, int)

// atom_cmpxchg
DEF_ATOM_3SRC(cmpxchg, global, int, i32, int)
DEF_ATOM_3SRC(cmpxchg, global, uint, i32, int)
DEF_ATOM_3SRC(cmpxchg, local, int, i32, int)
DEF_ATOM_3SRC(cmpxchg, local, uint, i32, int)

// atom_dec
DEF_ATOM_1SRC(dec, global, int, i32, int)
DEF_ATOM_1SRC(dec, global, uint, i32, int)
DEF_ATOM_1SRC(dec, local, int, i32, int)
DEF_ATOM_1SRC(dec, local, uint, i32, int)


// The below functions were added because of the clang 4.0 itanium mangling update
// See http://llvm.org/viewvc/llvm-project?view=revision&revision=262414


INLINE int _Z10atomic_decPVU3AS3i(volatile __local int *p)
{
    return atom_dec(p);
}
INLINE int _Z10atomic_minPVU3AS3ii(volatile __local int *p, int val)
{
    return atom_min(p,val);
}
INLINE int _Z10atomic_incPVU3AS1j(volatile __global uint *p)
{
    return atom_inc(p);
}
INLINE int _Z10atomic_incPVU3AS3j(volatile __local uint *p)
{
    return atom_inc(p);
}
INLINE int _Z10atomic_incPVU3AS3i(volatile __local int *p)
{
    return atom_inc(p);
}
INLINE int _Z10atomic_decPVU3AS1j(volatile __global uint *p)
{
    return atom_dec(p);
}
INLINE int _Z10atomic_minPVU3AS1jj(volatile __global uint *p, uint val)
{
    return atom_min(p,val);
}
INLINE int _Z10atomic_maxPVU3AS1jj(volatile __global uint *p, uint val)
{
    return atom_max(p,val);
}
INLINE int _Z10atomic_addPVU3AS1jj(volatile __global uint *p, uint val)
{
    return atom_add(p,val);
}
INLINE int _Z10atomic_minPVU3AS1ii(volatile __global int *p, int val)
{
    return atom_min(p,val);
}
INLINE int _Z10atomic_maxPVU3AS1ii(volatile __global int *p, int val)
{
    return atom_max(p,val);
}
INLINE int _Z10atomic_addPVU3AS1ii(volatile __global int *p, int val)
{
     return atom_add(p,val);
}
INLINE int _Z10atomic_minPVU3AS3jj(volatile __local uint *p, uint val)
{
    return atom_min(p,val);
}
INLINE int _Z10atomic_addPVU3AS3jj(volatile __local uint *p, uint val)
{
    return atom_add(p,val);
}
INLINE int _Z14atomic_cmpxchgPVU3AS3jjj(volatile __local uint *p, uint cmp, uint val)
{
    return atom_cmpxchg(p,cmp,val);
}
INLINE int _Z14atomic_cmpxchgPVU3AS1jjj(volatile __global uint *p, uint cmp, uint val)
{
    return atom_cmpxchg(p,cmp,val);
}




