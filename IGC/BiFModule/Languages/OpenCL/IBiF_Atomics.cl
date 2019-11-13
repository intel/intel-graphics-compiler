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
// This file contains definitions of OpenCL Atomic built-in functions.
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
// OpenCL 1.0  Atomics
//-----------------------------------------------------------------------------


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

#if defined(cl_intel_64bit_global_atomics_placeholder)

// atom_add
DEF_ATOM_2SRC(add, global, long, i64, long)
DEF_ATOM_2SRC(add, global, ulong, i64, long)

// atom_sub
DEF_ATOM_SUB(sub, global, long, i64, long)
DEF_ATOM_SUB(sub, global, ulong, i64, long)

// atom_xchg
DEF_ATOM_2SRC(xchg, global, long, i64, long)
DEF_ATOM_2SRC(xchg, global, ulong, i64, long)

// atom_min
DEF_ATOM_2SRC(min, global, long, i64, long)
DEF_ATOM_2SRC(min, global, ulong, u64, ulong)

// atom_max
DEF_ATOM_2SRC(max, global, long, i64, long)
DEF_ATOM_2SRC(max, global, ulong, u64, ulong)

// atom_and
DEF_ATOM_2SRC(and, global, long, i64, long)
DEF_ATOM_2SRC(and, global, ulong, i64, long)

// atom_or
DEF_ATOM_2SRC(or, global, long, i64, long)
DEF_ATOM_2SRC(or, global, ulong, i64, long)

// atom_xor
DEF_ATOM_2SRC(xor, global, long, i64, long)
DEF_ATOM_2SRC(xor, global, ulong, i64, long)

// atom_inc
DEF_ATOM_1SRC(inc, global, long, i64, long)
DEF_ATOM_1SRC(inc, global, ulong, i64, long)

// atom_cmpxchg
DEF_ATOM_3SRC(cmpxchg, global, long, i64, long)
DEF_ATOM_3SRC(cmpxchg, global, ulong, i64, long)

// atom_dec
DEF_ATOM_1SRC(dec, global, long, i64, long)
DEF_ATOM_1SRC(dec, global, ulong, i64, long)

#endif // if defined(cl_intel_64bit_global_atomics_placeholder)

//-----------------------------------------------------------------------------
// OpenCL 1.1  Atomics
//-----------------------------------------------------------------------------

INLINE float OVERLOADABLE atomic_xchg(__global volatile float *p, float val) {
    return as_float( __builtin_IB_atomic_xchg_global_i32( (__global volatile int *)p, as_int(val) ) );
}

INLINE float OVERLOADABLE atomic_xchg(__local volatile float *p, float val) {
    return as_float( __builtin_IB_atomic_xchg_local_i32( (__local volatile int *)p, as_int(val) ) );
}

#if defined(cl_intel_64bit_global_atomics_placeholder)
INLINE float OVERLOADABLE atomic_xchg(__global volatile double *p, double val) {
    return as_double( __builtin_IB_atomic_xchg_global_i64( (__global volatile long *)p, as_long(val) ) );
}
#endif // if defined(cl_intel_64bit_global_atomics_placeholder)

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

//-----------------------------------------------------------------------------
// OpenCL 2.0  Atomics (c1x)
//-----------------------------------------------------------------------------

// TODO:
// - implement old atomics in terms of new atomics.
// - disallow 64-bit functions (move size_t funcs to size_t.cl).
// - what to do about atomic_compare_exchange_strong/weak?
// - instruction select everything.

// atomic_init()

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define atomic_init_function_addrspace(TYPE, ADDRSPACE) \
INLINE void OVERLOADABLE atomic_init(volatile ADDRSPACE atomic_##TYPE *object, TYPE value) \
{ \
  volatile ADDRSPACE TYPE *nonatomic = (volatile ADDRSPACE TYPE*)object; \
  *nonatomic = value; \
}

#define atomic_init_function(TYPE) \
atomic_init_function_addrspace(TYPE, generic)

atomic_init_function(int)
atomic_init_function(uint)
atomic_init_function(float)

// atomic_fetch()

#define atomic_fetch_function_addrspace(KEY, TYPE, OPTYPE, FUNC, ADDRSPACE, ABBR_ADDRSPACE, ABBR_TYPE, PTR_TYPE) \
INLINE TYPE OVERLOADABLE atomic_fetch_##KEY(volatile ADDRSPACE atomic_##TYPE *object, OPTYPE operand) \
{ \
  return atomic_fetch_##KEY##_explicit(object, operand, memory_order_seq_cst); \
} \
INLINE TYPE OVERLOADABLE atomic_fetch_##KEY##_explicit(volatile ADDRSPACE atomic_##TYPE *object, OPTYPE operand, memory_order order) \
{ \
  return atomic_fetch_##KEY##_explicit(object, operand, order, memory_scope_device); \
} \
INLINE TYPE OVERLOADABLE atomic_fetch_##KEY##_explicit(volatile ADDRSPACE atomic_##TYPE *object, OPTYPE operand, memory_order order, memory_scope scope) \
{ \
  return __builtin_spirv_##FUNC##_##ABBR_ADDRSPACE##ABBR_TYPE##_i32_i32_##ABBR_TYPE((volatile ADDRSPACE PTR_TYPE*)object, get_spirv_mem_scope(scope), get_spirv_mem_order(order) | get_spirv_mem_fence(get_fence((const ADDRSPACE void*)object)), operand);\
}

#define atomic_fetch_function(KEY, TYPE, OPTYPE, FUNC) \
atomic_fetch_function_addrspace(KEY, TYPE, OPTYPE, FUNC, generic, p4, i32, uint)

#define atomic_fetch_supported_types(KEY, FUNC) \
atomic_fetch_function(KEY, int, int, FUNC) \
atomic_fetch_function(KEY, uint, uint, FUNC) \
atomic_fetch_function(KEY, uint, int, FUNC) // the (size_t, ptrdiff_t) variety for 32-bit

atomic_fetch_supported_types(add, OpAtomicIAdd)
atomic_fetch_supported_types(sub, OpAtomicISub)
atomic_fetch_supported_types(or, OpAtomicOr)
atomic_fetch_supported_types(xor, OpAtomicXor)
atomic_fetch_supported_types(and, OpAtomicAnd)
atomic_fetch_function(max, int, int, OpAtomicSMax)
atomic_fetch_function(max, uint, uint, OpAtomicUMax)
atomic_fetch_function(min, int, int, OpAtomicSMin)
atomic_fetch_function(min, uint, uint, OpAtomicUMin)

// atomic_store()

// TODO: should we use  __c11_atomic_store(object, operand, order) ?

// Also need to force fences on raw stores and loads regardless of memory scope or order.
// Atomically! replace the value pointed to by object with desired.
// Since there is no difference between an ordinary store an an atomic one on
// Gen HW we use the first one.

#define atomic_store_function_addrspace_int(TYPE, ABBR_TYPE, ADDRSPACE, ABBR_ADDRSPACE) \
INLINE void OVERLOADABLE atomic_store(volatile ADDRSPACE atomic_##TYPE *object, TYPE operand) \
{ \
  atomic_store_explicit(object, operand, memory_order_seq_cst); \
} \
INLINE void OVERLOADABLE atomic_store_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE operand, memory_order order) \
{ \
  atomic_store_explicit(object, operand, order, memory_scope_device); \
} \
INLINE void OVERLOADABLE atomic_store_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE operand, memory_order order, memory_scope scope) \
{ \
  __builtin_spirv_OpAtomicStore_##ABBR_ADDRSPACE##ABBR_TYPE##_i32_i32_##ABBR_TYPE((volatile ADDRSPACE uint*)object, get_spirv_mem_scope(scope), get_spirv_mem_order(order) | get_spirv_mem_fence(get_fence((const ADDRSPACE void*)object)) , operand);\
}

#define atomic_store_function_addrspace(TYPE, ABBR_TYPE, ADDRSPACE, ABBR_ADDRSPACE) \
INLINE void OVERLOADABLE atomic_store(volatile ADDRSPACE atomic_##TYPE *object, TYPE operand) \
{ \
  atomic_store_explicit(object, operand, memory_order_seq_cst); \
} \
INLINE void OVERLOADABLE atomic_store_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE operand, memory_order order) \
{ \
  atomic_store_explicit(object, operand, order, memory_scope_device); \
} \
INLINE void OVERLOADABLE atomic_store_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE operand, memory_order order, memory_scope scope) \
{ \
  __builtin_spirv_OpAtomicStore_##ABBR_ADDRSPACE##ABBR_TYPE##_i32_i32_##ABBR_TYPE((volatile ADDRSPACE TYPE*)object, get_spirv_mem_scope(scope), get_spirv_mem_order(order) | get_spirv_mem_fence(get_fence((const ADDRSPACE void*)object)), operand);\
}

#define atomic_store_function(TYPE, ABBR_TYPE) \
atomic_store_function_addrspace(TYPE, ABBR_TYPE, generic, p4)

atomic_store_function_addrspace_int(int,i32, generic, p4)
atomic_store_function(uint,i32)
// TODO: are any float types supported?
atomic_store_function(float,f32)

// atomic_load()

// Also need to force fences on raw stores and loads regardless of memory scope or order.

#define atomic_load_function_addrspace_int(TYPE, ABBR_TYPE, ADDRSPACE, ABBR_ADDRSPACE) \
INLINE TYPE OVERLOADABLE atomic_load(volatile ADDRSPACE atomic_##TYPE *object) \
{ \
  return atomic_load_explicit(object, memory_order_seq_cst, memory_scope_device); \
} \
INLINE TYPE OVERLOADABLE atomic_load_explicit(volatile ADDRSPACE atomic_##TYPE *object, memory_order order) \
{ \
  return atomic_load_explicit(object, order, memory_scope_device); \
} \
INLINE TYPE OVERLOADABLE atomic_load_explicit(volatile ADDRSPACE atomic_##TYPE *object, memory_order order, memory_scope scope) \
{ \
  return __builtin_spirv_OpAtomicLoad_##ABBR_ADDRSPACE##ABBR_TYPE##_i32_i32((volatile ADDRSPACE uint*)object, get_spirv_mem_scope(scope), get_spirv_mem_order(order) | get_spirv_mem_fence(get_fence((const ADDRSPACE void*)object)));\
}

#define atomic_load_function_addrspace(TYPE, ABBR_TYPE, ADDRSPACE, ABBR_ADDRSPACE) \
INLINE TYPE OVERLOADABLE atomic_load(volatile ADDRSPACE atomic_##TYPE *object) \
{ \
  return atomic_load_explicit(object, memory_order_seq_cst, memory_scope_device); \
} \
INLINE TYPE OVERLOADABLE atomic_load_explicit(volatile ADDRSPACE atomic_##TYPE *object, memory_order order) \
{ \
  return atomic_load_explicit(object, order, memory_scope_device); \
} \
INLINE TYPE OVERLOADABLE atomic_load_explicit(volatile ADDRSPACE atomic_##TYPE *object, memory_order order, memory_scope scope) \
{ \
  return __builtin_spirv_OpAtomicLoad_##ABBR_ADDRSPACE##ABBR_TYPE##_i32_i32((volatile ADDRSPACE TYPE*)object, get_spirv_mem_scope(scope), get_spirv_mem_order(order) | get_spirv_mem_fence(get_fence((const ADDRSPACE void*)object)));\
}

#define atomic_load_function(TYPE, ABBR_TYPE) \
atomic_load_function_addrspace(TYPE, ABBR_TYPE, generic, p4)

atomic_load_function_addrspace_int(int, i32, generic, p4)
atomic_load_function(uint,i32)
atomic_load_function(float,f32)

// atomic_exchange()

#define atomic_exchange_function_addrspace_int(TYPE, ABBR_TYPE, ADDRSPACE, ABBR_ADDRSPACE) \
INLINE TYPE OVERLOADABLE atomic_exchange(volatile ADDRSPACE atomic_##TYPE *object, TYPE desired) \
{ \
  return atomic_exchange_explicit(object, desired, memory_order_seq_cst, memory_scope_device); \
} \
INLINE TYPE OVERLOADABLE atomic_exchange_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE desired, memory_order order) \
{ \
  return atomic_exchange_explicit(object, desired, order, memory_scope_device); \
} \
INLINE TYPE OVERLOADABLE atomic_exchange_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE desired, memory_order order, memory_scope scope) \
{ \
  return __builtin_spirv_OpAtomicExchange_##ABBR_ADDRSPACE##ABBR_TYPE##_i32_i32_##ABBR_TYPE((volatile ADDRSPACE uint*)object, get_spirv_mem_scope(scope), get_spirv_mem_order(order) | get_spirv_mem_fence(get_fence((const ADDRSPACE void*)object)), desired);\
}

#define atomic_exchange_function_addrspace(TYPE, ABBR_TYPE, ADDRSPACE, ABBR_ADDRSPACE) \
INLINE TYPE OVERLOADABLE atomic_exchange(volatile ADDRSPACE atomic_##TYPE *object, TYPE desired) \
{ \
  return atomic_exchange_explicit(object, desired, memory_order_seq_cst, memory_scope_device); \
} \
INLINE TYPE OVERLOADABLE atomic_exchange_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE desired, memory_order order) \
{ \
  return atomic_exchange_explicit(object, desired, order, memory_scope_device); \
} \
INLINE TYPE OVERLOADABLE atomic_exchange_explicit(volatile ADDRSPACE atomic_##TYPE *object, TYPE desired, memory_order order, memory_scope scope) \
{ \
  return __builtin_spirv_OpAtomicExchange_##ABBR_ADDRSPACE##ABBR_TYPE##_i32_i32_##ABBR_TYPE((volatile ADDRSPACE TYPE*)object, get_spirv_mem_scope(scope), get_spirv_mem_order(order) | get_spirv_mem_fence(get_fence((const ADDRSPACE void*)object)), desired);\
}

#define atomic_exchange_function(TYPE, ABBR_TYPE) \
atomic_exchange_function_addrspace(TYPE, ABBR_TYPE, generic, p4)

atomic_exchange_function_addrspace_int(int,i32, generic, p4)
atomic_exchange_function(uint,i32)
atomic_exchange_function(float,f32)

// atomic_compare_exchange_strong() and atomic_compare_exchange_weak()

#define atomic_compare_exchange_strength_function_addrspace(TYPE, ADDRSPACE, ADDRSPACE2, STRENGTH, FUNC, ABBR_ADDRSPACE) \
INLINE bool OVERLOADABLE atomic_compare_exchange_##STRENGTH(volatile ADDRSPACE atomic_##TYPE *object, ADDRSPACE2 TYPE *expected, TYPE desired) \
{ \
  return atomic_compare_exchange_##STRENGTH##_explicit(object, expected, desired, memory_order_seq_cst, memory_order_seq_cst, memory_scope_device); \
} \
INLINE bool OVERLOADABLE atomic_compare_exchange_##STRENGTH##_explicit(volatile ADDRSPACE atomic_##TYPE *object, ADDRSPACE2 TYPE *expected, \
                                                                       TYPE desired, memory_order success, memory_order failure) \
{ \
  return atomic_compare_exchange_##STRENGTH##_explicit(object, expected, desired, success, failure, memory_scope_device); \
} \
INLINE bool OVERLOADABLE atomic_compare_exchange_##STRENGTH##_explicit(volatile ADDRSPACE atomic_##TYPE *object, ADDRSPACE2 TYPE *expected, \
                                                                       TYPE desired, memory_order success, memory_order failure, memory_scope scope) \
{ \
  TYPE expected_start = (*expected);\
  uint before = __builtin_spirv_##FUNC##_##ABBR_ADDRSPACE##i32_i32_i32_i32_i32_i32((volatile ADDRSPACE uint*)object, get_spirv_mem_scope(scope), success, failure, as_uint(desired),as_uint(expected_start)); \
  bool ret = false; \
  if ( before == as_uint(expected_start)) { \
    ret = true; \
  } \
  else { \
    *expected = as_##TYPE(before); \
    ret = false; \
  } \
  if( success != memory_order_release ) \
  { \
    atomic_work_item_fence( CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE, success, scope); \
  } \
  return ret; \
}
// __builtin_IB_atomic_cmpxchg_i32_##ADDRSPACE((ADDRSPACE TYPE*)object, *expected, desired); \
// bool val = __c11_atomic_compare_exchange_##STRENGTH(object, expected, desired, success, failure); \
// TODO: success vs. failure: can we always just use success?  More control flow would
// be necessary to check result.  Also current cmpxchg instruction isn't the same as
// what this function does.  Also, is currently implementation sufficient?

#define atomic_compare_exchange_strength_function(TYPE, STRENGTH, FUNC) \
atomic_compare_exchange_strength_function_addrspace(TYPE, generic, generic, STRENGTH, FUNC, p4)

atomic_compare_exchange_strength_function(int, strong, OpAtomicCompareExchange)
atomic_compare_exchange_strength_function(uint, strong, OpAtomicCompareExchange)
atomic_compare_exchange_strength_function(int, weak, OpAtomicCompareExchangeWeak)
atomic_compare_exchange_strength_function(uint, weak, OpAtomicCompareExchangeWeak)
atomic_compare_exchange_strength_function(float, strong, OpAtomicCompareExchange)
atomic_compare_exchange_strength_function(float, weak, OpAtomicCompareExchangeWeak)

// atomic_flag_test_and_set()

#define ATOMIC_FLAG_TRUE 1
#define ATOMIC_FLAG_FALSE 0

#define atomic_flag_test_and_set_function_addrspace(ADDRSPACE, ABBR_ADDRSPACE) \
bool __attribute__((overloadable)) atomic_flag_test_and_set(volatile ADDRSPACE atomic_flag *object) \
{ \
  return atomic_flag_test_and_set_explicit(object, memory_order_seq_cst); \
} \
bool __attribute__((overloadable)) atomic_flag_test_and_set_explicit(volatile ADDRSPACE atomic_flag *object, memory_order order) \
{ \
  return atomic_flag_test_and_set_explicit(object, order, memory_scope_device); \
} \
bool __attribute__((overloadable)) atomic_flag_test_and_set_explicit(volatile ADDRSPACE atomic_flag *object, memory_order order, memory_scope scope) \
{ \
  return __builtin_spirv_OpAtomicFlagTestAndSet_##ABBR_ADDRSPACE##i32_i32_i32((volatile ADDRSPACE uint*)object, get_spirv_mem_scope(scope), get_spirv_mem_order(order) | get_spirv_mem_fence(get_fence((const ADDRSPACE void*)object))); \
}

#define atomic_flag_test_and_set_function() \
atomic_flag_test_and_set_function_addrspace(generic, p4)

atomic_flag_test_and_set_function()

// atomic_flag_clear()

#define atomic_flag_clear_function_addrspace(ADDRSPACE, ABBR_ADDRSPACE) \
void __attribute__((overloadable)) atomic_flag_clear(volatile ADDRSPACE atomic_flag *object) \
{ \
  atomic_flag_clear_explicit(object, memory_order_seq_cst); \
} \
void __attribute__((overloadable)) atomic_flag_clear_explicit(volatile ADDRSPACE atomic_flag *object, memory_order order) \
{ \
  atomic_flag_clear_explicit(object, order, memory_scope_device); \
} \
void __attribute__((overloadable)) atomic_flag_clear_explicit(volatile ADDRSPACE atomic_flag *object, memory_order order, memory_scope scope) \
{ \
  __builtin_spirv_OpAtomicFlagClear_##ABBR_ADDRSPACE##i32_i32_i32((volatile ADDRSPACE uint*)object, get_spirv_mem_scope(scope), get_spirv_mem_order(order) | get_spirv_mem_fence(get_fence((const ADDRSPACE void*)object))); \
}

#define atomic_flag_clear_function() \
atomic_flag_clear_function_addrspace(generic, p4)

atomic_flag_clear_function()

#undef ATOMIC_FLAG_TRUE
#undef ATOMIC_FLAG_FALSE

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
