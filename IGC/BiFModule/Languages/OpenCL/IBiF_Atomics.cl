/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===-  IGCBiF_Atomics.cl -===================================================//
//
// This file contains definitions of OpenCL Atomic built-in functions.
//
//===----------------------------------------------------------------------===//


//-----------------------------------------------------------------------------
// OpenCL 1.0  Atomics
//-----------------------------------------------------------------------------

// atom_*() functions need non-volatile overloads for backwards compatibility.

#define DEF_ATOM_1SRC(KEY, ADDRSPACE, OCL_TYPE, OPCODE, ADDRSPACE_ABBR, IGC_TYPE_ABBR, IGC_TYPE) \
INLINE OCL_TYPE OVERLOADABLE atom_##KEY(volatile __##ADDRSPACE OCL_TYPE *p) { \
    return __spirv_Atomic##OPCODE( \
            (__##ADDRSPACE IGC_TYPE *)p,                                                \
            Device,                                                                     \
            Relaxed);                                                                   \
} \
INLINE OCL_TYPE OVERLOADABLE atom_##KEY(__##ADDRSPACE OCL_TYPE *p) { \
    return __spirv_Atomic##OPCODE( \
            (__##ADDRSPACE IGC_TYPE *)p,                                                \
            Device,                                                                     \
            Relaxed);                                                                   \
}

#define DEF_ATOM_2SRC(KEY, ADDRSPACE, OCL_TYPE, OPCODE, ADDRSPACE_ABBR, IGC_TYPE_ABBR, IGC_TYPE) \
INLINE OCL_TYPE OVERLOADABLE atom_##KEY(volatile __##ADDRSPACE OCL_TYPE *p, OCL_TYPE val) { \
    return __spirv_Atomic##OPCODE( \
            (__##ADDRSPACE IGC_TYPE *)p,                                                 \
            Device,                                                                      \
            Relaxed,                                                                     \
            val);                                                                        \
} \
INLINE OCL_TYPE OVERLOADABLE atom_##KEY(__##ADDRSPACE OCL_TYPE *p, OCL_TYPE val) { \
    return __spirv_Atomic##OPCODE( \
            (__##ADDRSPACE IGC_TYPE *)p,                                                 \
            Device,                                                                      \
            Relaxed,                                                                     \
            val);                                                                        \
}

#define DEF_ATOM_3SRC(KEY, ADDRSPACE, OCL_TYPE, OPCODE, ADDRSPACE_ABBR, IGC_TYPE_ABBR, IGC_TYPE) \
INLINE OCL_TYPE OVERLOADABLE atom_##KEY(volatile __##ADDRSPACE OCL_TYPE *p, OCL_TYPE cmp, OCL_TYPE val) { \
    return __spirv_Atomic##OPCODE( \
            (__##ADDRSPACE IGC_TYPE *)p,                                                 \
            Device,                                                                      \
            Relaxed,                                                                     \
            Relaxed,                                                                     \
            val,                                                                         \
            cmp);                                                                        \
} \
INLINE OCL_TYPE OVERLOADABLE atom_##KEY(__##ADDRSPACE OCL_TYPE *p, OCL_TYPE cmp, OCL_TYPE val) { \
    return __spirv_Atomic##OPCODE( \
            (__##ADDRSPACE IGC_TYPE *)p,                                                 \
            Device,                                                                      \
            Relaxed,                                                                     \
            Relaxed,                                                                     \
            val,                                                                         \
            cmp);                                                                        \
}

// atomic_inc
DEF_ATOM_1SRC(inc, global, int, IIncrement, p1, i32, int)
DEF_ATOM_1SRC(inc, local, int, IIncrement, p3, i32, int)
DEF_ATOM_1SRC(inc, global, uint, IIncrement, p1, i32, int)
DEF_ATOM_1SRC(inc, local, uint, IIncrement, p3, i32, int)

// atomic_dec
DEF_ATOM_1SRC(dec, global, int, IDecrement, p1, i32, int)
DEF_ATOM_1SRC(dec, local, int, IDecrement, p3, i32, int)
DEF_ATOM_1SRC(dec, global, uint, IDecrement, p1, i32, int)
DEF_ATOM_1SRC(dec, local, uint, IDecrement, p3, i32, int)

// atomic_add
DEF_ATOM_2SRC(add, global, int, IAdd, p1, i32, int)
DEF_ATOM_2SRC(add, local, int, IAdd, p3, i32, int)
DEF_ATOM_2SRC(add, global, uint, IAdd, p1, i32, int)
DEF_ATOM_2SRC(add, local, uint, IAdd, p3, i32, int)

// atomic_sub
DEF_ATOM_2SRC(sub, global, int, ISub, p1, i32, int)
DEF_ATOM_2SRC(sub, local, int, ISub, p3, i32, int)
DEF_ATOM_2SRC(sub, global, uint, ISub, p1, i32, int)
DEF_ATOM_2SRC(sub, local, uint, ISub, p3, i32, int)

// atomic_xchg
DEF_ATOM_2SRC(xchg, global, int, Exchange, p1, i32, int)
DEF_ATOM_2SRC(xchg, local, int, Exchange, p3, i32, int)
DEF_ATOM_2SRC(xchg, global, uint, Exchange, p1, i32, int)
DEF_ATOM_2SRC(xchg, local, uint, Exchange, p3, i32, int)

// atomic_min
DEF_ATOM_2SRC(min, global, int, SMin, p1, i32, int)
DEF_ATOM_2SRC(min, local, int, SMin, p3, i32, int)
DEF_ATOM_2SRC(min, global, uint, UMin, p1, i32, uint)
DEF_ATOM_2SRC(min, local, uint, UMin, p3, i32, uint)

// atomic_max
DEF_ATOM_2SRC(max, global, int, SMax, p1, i32, int)
DEF_ATOM_2SRC(max, local, int, SMax, p3, i32, int)
DEF_ATOM_2SRC(max, global, uint, UMax, p1, i32, uint)
DEF_ATOM_2SRC(max, local, uint, UMax, p3, i32, uint)

// atomic_and
DEF_ATOM_2SRC(and, global, int, And, p1, i32, int)
DEF_ATOM_2SRC(and, local, int, And, p3, i32, int)
DEF_ATOM_2SRC(and, global, uint, And, p1, i32, int)
DEF_ATOM_2SRC(and, local, uint, And, p3, i32, int)

// atomic_or
DEF_ATOM_2SRC(or, global, int, Or, p1, i32, int)
DEF_ATOM_2SRC(or, local, int, Or, p3, i32, int)
DEF_ATOM_2SRC(or, global, uint, Or, p1, i32, int)
DEF_ATOM_2SRC(or, local, uint, Or, p3, i32, int)

// atomic_xor
DEF_ATOM_2SRC(xor, global, int, Xor, p1, i32, int)
DEF_ATOM_2SRC(xor, local, int, Xor, p3, i32, int)
DEF_ATOM_2SRC(xor, global, uint, Xor, p1, i32, int)
DEF_ATOM_2SRC(xor, local, uint, Xor, p3, i32, int)

// atomic_cmpxchg
DEF_ATOM_3SRC(cmpxchg, global, int, CompareExchange, p1, i32, int)
DEF_ATOM_3SRC(cmpxchg, local, int, CompareExchange, p3, i32, int)
DEF_ATOM_3SRC(cmpxchg, global, uint, CompareExchange, p1, i32, int)
DEF_ATOM_3SRC(cmpxchg, local, uint, CompareExchange, p3, i32, int)

#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

// atomic_inc
DEF_ATOM_1SRC(inc, global, long, IIncrement, p1, i64, long)
DEF_ATOM_1SRC(inc, local, long, IIncrement, p3, i64, long)
DEF_ATOM_1SRC(inc, global, ulong, IIncrement, p1, i64, long)
DEF_ATOM_1SRC(inc, local, ulong, IIncrement, p3, i64, long)

// atomic_dec
DEF_ATOM_1SRC(dec, global, long, IDecrement, p1, i64, long)
DEF_ATOM_1SRC(dec, local, long, IDecrement, p3, i64, long)
DEF_ATOM_1SRC(dec, global, ulong, IDecrement, p1, i64, long)
DEF_ATOM_1SRC(dec, local, ulong, IDecrement, p3, i64, long)

// atom_add
DEF_ATOM_2SRC(add, global, long, IAdd, p1, i64, long)
DEF_ATOM_2SRC(add, local, long, IAdd, p3, i64, long)
DEF_ATOM_2SRC(add, global, ulong, IAdd, p1, i64, long)
DEF_ATOM_2SRC(add, local, ulong, IAdd, p3, i64, long)

// atom_sub
DEF_ATOM_2SRC(sub, global, long, ISub, p1, i64, long)
DEF_ATOM_2SRC(sub, local, long, ISub, p3, i64, long)
DEF_ATOM_2SRC(sub, global, ulong, ISub, p1, i64, long)
DEF_ATOM_2SRC(sub, local, ulong, ISub, p3, i64, long)

// atom_xchg
DEF_ATOM_2SRC(xchg, global, long, Exchange, p1, i64, long)
DEF_ATOM_2SRC(xchg, local, long, Exchange, p3, i64, long)
DEF_ATOM_2SRC(xchg, global, ulong, Exchange, p1, i64, long)
DEF_ATOM_2SRC(xchg, local, ulong, Exchange, p3, i64, long)

// atom_min
DEF_ATOM_2SRC(min, global, long, SMin, p1, i64, long)
DEF_ATOM_2SRC(min, local, long, SMin, p3, i64, long)
DEF_ATOM_2SRC(min, global, ulong, UMin, p1, i64, ulong)
DEF_ATOM_2SRC(min, local, ulong, UMin, p3, i64, ulong)

// atom_max
DEF_ATOM_2SRC(max, global, long, SMax, p1, i64, long)
DEF_ATOM_2SRC(max, local, long, SMax, p3, i64, long)
DEF_ATOM_2SRC(max, global, ulong, UMax, p1, i64, ulong)
DEF_ATOM_2SRC(max, local, ulong, UMax, p3, i64, ulong)

// atom_and
DEF_ATOM_2SRC(and, global, long, And, p1, i64, long)
DEF_ATOM_2SRC(and, local, long, And, p3, i64, long)
DEF_ATOM_2SRC(and, global, ulong, And, p1, i64, long)
DEF_ATOM_2SRC(and, local, ulong, And, p3, i64, long)

// atom_or
DEF_ATOM_2SRC(or, global, long, Or, p1, i64, long)
DEF_ATOM_2SRC(or, local, long, Or, p3, i64, long)
DEF_ATOM_2SRC(or, global, ulong, Or, p1, i64, long)
DEF_ATOM_2SRC(or, local, ulong, Or, p3, i64, long)

// atom_xor
DEF_ATOM_2SRC(xor, global, long, Xor, p1, i64, long)
DEF_ATOM_2SRC(xor, local, long, Xor, p3, i64, long)
DEF_ATOM_2SRC(xor, global, ulong, Xor, p1, i64, long)
DEF_ATOM_2SRC(xor, local, ulong, Xor, p3, i64, long)

// atom_cmpxchg
DEF_ATOM_3SRC(cmpxchg, global, long, CompareExchange, p1, i64, long)
DEF_ATOM_3SRC(cmpxchg, local, long, CompareExchange, p3, i64, long)
DEF_ATOM_3SRC(cmpxchg, global, ulong, CompareExchange, p1, i64, long)
DEF_ATOM_3SRC(cmpxchg, local, ulong, CompareExchange, p3, i64, long)

#endif // if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

//-----------------------------------------------------------------------------
// OpenCL 1.1  Atomics
//-----------------------------------------------------------------------------

#define DEF_ATOMIC_1SRC(KEY, ADDRSPACE, OCL_TYPE, OPCODE, ADDRSPACE_ABBR, IGC_TYPE_ABBR, IGC_TYPE) \
INLINE OCL_TYPE OVERLOADABLE atomic_##KEY(volatile __##ADDRSPACE OCL_TYPE *p) { \
    return __spirv_Atomic##OPCODE( \
            (__##ADDRSPACE IGC_TYPE *)p,                                                \
            Device,                                                                     \
            Relaxed);                                                                   \
}

#define DEF_ATOMIC_2SRC(KEY, ADDRSPACE, OCL_TYPE, OPCODE, ADDRSPACE_ABBR, IGC_TYPE_ABBR, IGC_TYPE) \
INLINE OCL_TYPE OVERLOADABLE atomic_##KEY(volatile __##ADDRSPACE OCL_TYPE *p, OCL_TYPE val) { \
    return __spirv_Atomic##OPCODE( \
            (__##ADDRSPACE IGC_TYPE *)p,                                        \
            Device,                                                                      \
            Relaxed,                                                                     \
            val);                                                                        \
}

#define DEF_ATOMIC_3SRC(KEY, ADDRSPACE, OCL_TYPE, OPCODE, ADDRSPACE_ABBR, IGC_TYPE_ABBR, IGC_TYPE) \
INLINE OCL_TYPE OVERLOADABLE atomic_##KEY(volatile __##ADDRSPACE OCL_TYPE *p, OCL_TYPE cmp, OCL_TYPE val) { \
    return __spirv_Atomic##OPCODE( \
            (__##ADDRSPACE IGC_TYPE *)p,                                                 \
            Device,                                                                      \
            Relaxed,                                                                     \
            Relaxed,                                                                     \
            val,                                                                         \
            cmp);                                                                        \
}

// atomic_inc
DEF_ATOMIC_1SRC(inc, global, int, IIncrement, p1, i32, int)
DEF_ATOMIC_1SRC(inc, local, int, IIncrement, p3, i32, int)
DEF_ATOMIC_1SRC(inc, global, uint, IIncrement, p1, i32, int)
DEF_ATOMIC_1SRC(inc, local, uint, IIncrement, p3, i32, int)

// atomic_dec
DEF_ATOMIC_1SRC(dec, global, int, IDecrement, p1, i32, int)
DEF_ATOMIC_1SRC(dec, local, int, IDecrement, p3, i32, int)
DEF_ATOMIC_1SRC(dec, global, uint, IDecrement, p1, i32, int)
DEF_ATOMIC_1SRC(dec, local, uint, IDecrement, p3, i32, int)

// atomic_add
DEF_ATOMIC_2SRC(add, global, int, IAdd, p1, i32, int)
DEF_ATOMIC_2SRC(add, local, int, IAdd, p3, i32, int)
DEF_ATOMIC_2SRC(add, global, uint, IAdd, p1, i32, int)
DEF_ATOMIC_2SRC(add, local, uint, IAdd, p3, i32, int)

// atomic_sub
DEF_ATOMIC_2SRC(sub, global, int, ISub, p1, i32, int)
DEF_ATOMIC_2SRC(sub, local, int, ISub, p3, i32, int)
DEF_ATOMIC_2SRC(sub, global, uint, ISub, p1, i32, int)
DEF_ATOMIC_2SRC(sub, local, uint, ISub, p3, i32, int)

// atomic_xchg
DEF_ATOMIC_2SRC(xchg, global, int, Exchange, p1, i32, int)
DEF_ATOMIC_2SRC(xchg, local, int, Exchange, p3, i32, int)
DEF_ATOMIC_2SRC(xchg, global, uint, Exchange, p1, i32, int)
DEF_ATOMIC_2SRC(xchg, local, uint, Exchange, p3, i32, int)
DEF_ATOMIC_2SRC(xchg, global, float, Exchange, p1, f32, float)
DEF_ATOMIC_2SRC(xchg, local, float, Exchange, p3, f32, float)
#if defined(cl_khr_int64_base_atomics)
DEF_ATOMIC_2SRC(xchg, global, double, Exchange, p1, f64, double)
DEF_ATOMIC_2SRC(xchg, local, double, Exchange, p3, f64, double)
#endif // if defined(cl_khr_int64_base_atomics)

// atomic_min
DEF_ATOMIC_2SRC(min, global, int, SMin, p1, i32, int)
DEF_ATOMIC_2SRC(min, local, int, SMin, p3, i32, int)
DEF_ATOMIC_2SRC(min, global, uint, UMin, p1, i32, uint)
DEF_ATOMIC_2SRC(min, local, uint, UMin, p3, i32, uint)

// atomic_max
DEF_ATOMIC_2SRC(max, global, int, SMax, p1, i32, int)
DEF_ATOMIC_2SRC(max, local, int, SMax, p3, i32, int)
DEF_ATOMIC_2SRC(max, global, uint, UMax, p1, i32, uint)
DEF_ATOMIC_2SRC(max, local, uint, UMax, p3, i32, uint)

// atomic_and
DEF_ATOMIC_2SRC(and, global, int, And, p1, i32, int)
DEF_ATOMIC_2SRC(and, local, int, And, p3, i32, int)
DEF_ATOMIC_2SRC(and, global, uint, And, p1, i32, int)
DEF_ATOMIC_2SRC(and, local, uint, And, p3, i32, int)

// atomic_or
DEF_ATOMIC_2SRC(or, global, int, Or, p1, i32, int)
DEF_ATOMIC_2SRC(or, local, int, Or, p3, i32, int)
DEF_ATOMIC_2SRC(or, global, uint, Or, p1, i32, int)
DEF_ATOMIC_2SRC(or, local, uint, Or, p3, i32, int)

// atomic_xor
DEF_ATOMIC_2SRC(xor, global, int, Xor, p1, i32, int)
DEF_ATOMIC_2SRC(xor, local, int, Xor, p3, i32, int)
DEF_ATOMIC_2SRC(xor, global, uint, Xor, p1, i32, int)
DEF_ATOMIC_2SRC(xor, local, uint, Xor, p3, i32, int)

// atomic_cmpxchg
DEF_ATOMIC_3SRC(cmpxchg, global, int, CompareExchange, p1, i32, int)
DEF_ATOMIC_3SRC(cmpxchg, local, int, CompareExchange, p3, i32, int)
DEF_ATOMIC_3SRC(cmpxchg, global, uint, CompareExchange, p1, i32, int)
DEF_ATOMIC_3SRC(cmpxchg, local, uint, CompareExchange, p3, i32, int)

//-----------------------------------------------------------------------------
// OpenCL 2.0  Atomics (c1x)
//-----------------------------------------------------------------------------

// The atomic_init Function
// void atomic_init(volatile A *obj, C value)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define ATOMIC_INIT_FUNCTION(TYPE) \
INLINE void OVERLOADABLE atomic_init(volatile generic atomic_##TYPE *object, TYPE value) \
{ \
  volatile generic TYPE *nonatomic = (volatile generic TYPE*)object; \
  *nonatomic = value; \
}

ATOMIC_INIT_FUNCTION(int)
ATOMIC_INIT_FUNCTION(uint)
ATOMIC_INIT_FUNCTION(float)
#if defined(cl_khr_int64_base_atomics) && defined(cl_khr_int64_extended_atomics)
ATOMIC_INIT_FUNCTION(long)
ATOMIC_INIT_FUNCTION(ulong)
ATOMIC_INIT_FUNCTION(double)
#endif // defined(cl_khr_int64_base_atomics) && defined(cl_khr_int64_extended_atomics)

// The atomic_fetch Functions
// C atomic_fetch_key(volatile A *object, M operand)
// C atomic_fetch_key_explicit(volatile A *object,
//   M operand,
//   memory_order order)
// C atomic_fetch_key_explicit(volatile A *object,
//   M operand,
//   memory_order order,
//   memory_scope scope)
#define ATOMIC_FETCH_FUNCTION_ADDRSPACE(KEY, OCL_TYPE, OPCODE, IGC_TYPE_ABBR, IGC_TYPE, ADDRSPACE, ABBR_ADDRSPACE) \
INLINE OCL_TYPE OVERLOADABLE atomic_fetch_##KEY(volatile ADDRSPACE atomic_##OCL_TYPE *object, OCL_TYPE operand) \
{ \
  return atomic_fetch_##KEY##_explicit(object, operand, memory_order_seq_cst); \
} \
INLINE OCL_TYPE OVERLOADABLE atomic_fetch_##KEY##_explicit(volatile ADDRSPACE atomic_##OCL_TYPE *object, OCL_TYPE operand, memory_order order) \
{ \
  return atomic_fetch_##KEY##_explicit(object, operand, order, memory_scope_device); \
} \
INLINE OCL_TYPE OVERLOADABLE atomic_fetch_##KEY##_explicit(volatile ADDRSPACE atomic_##OCL_TYPE *object, OCL_TYPE operand, memory_order order, memory_scope scope) \
{ \
  return __spirv_Atomic##OPCODE( \
            (ADDRSPACE IGC_TYPE *)object,                                                \
            get_spirv_mem_scope(scope),                                                  \
            get_spirv_mem_order(order) |                                                 \
                get_spirv_mem_fence(get_fence((const ADDRSPACE void *)object)),          \
            operand);                                                                    \
}

#define ATOMIC_FETCH_FUNCTION(KEY, OCL_TYPE, OPCODE, IGC_TYPE_ABBR, IGC_TYPE) \
ATOMIC_FETCH_FUNCTION_ADDRSPACE(KEY, OCL_TYPE, OPCODE, IGC_TYPE_ABBR, IGC_TYPE, generic, p4)

#if defined(cl_khr_int64_base_atomics) && defined(cl_khr_int64_extended_atomics)
#define ATOMIC_FETCH_SUPPORTED_TYPES(KEY, OPCODE) \
ATOMIC_FETCH_FUNCTION(KEY, int, OPCODE, i32, int) \
ATOMIC_FETCH_FUNCTION(KEY, uint, OPCODE, i32, int) \
ATOMIC_FETCH_FUNCTION(KEY, long, OPCODE, i64, long) \
ATOMIC_FETCH_FUNCTION(KEY, ulong, OPCODE, i64, long)
#else
#define ATOMIC_FETCH_SUPPORTED_TYPES(KEY, OPCODE) \
ATOMIC_FETCH_FUNCTION(KEY, int, OPCODE, i32, int) \
ATOMIC_FETCH_FUNCTION(KEY, uint, OPCODE, i32, int)
#endif // defined(cl_khr_int64_base_atomics) && defined(cl_khr_int64_extended_atomics)

ATOMIC_FETCH_SUPPORTED_TYPES(add, IAdd)
ATOMIC_FETCH_SUPPORTED_TYPES(sub, ISub)
ATOMIC_FETCH_SUPPORTED_TYPES(or, Or)
ATOMIC_FETCH_SUPPORTED_TYPES(xor, Xor)
ATOMIC_FETCH_SUPPORTED_TYPES(and, And)
ATOMIC_FETCH_FUNCTION(max, int, SMax, i32, uint)
ATOMIC_FETCH_FUNCTION(max, uint, UMax, i32, uint)
ATOMIC_FETCH_FUNCTION(min, int, SMin, i32, uint)
ATOMIC_FETCH_FUNCTION(min, uint, UMin, i32, uint)
#if defined(cl_khr_int64_base_atomics) && defined(cl_khr_int64_extended_atomics)
ATOMIC_FETCH_FUNCTION(max, long, SMax, i64, ulong)
ATOMIC_FETCH_FUNCTION(max, ulong, UMax, i64, ulong)
ATOMIC_FETCH_FUNCTION(min, long, SMin, i64, ulong)
ATOMIC_FETCH_FUNCTION(min, ulong, UMin, i64, ulong)
#endif // defined(cl_khr_int64_base_atomics) && defined(cl_khr_int64_extended_atomics)
ATOMIC_FETCH_FUNCTION_ADDRSPACE(add, float, FAdd, f32, float, global, p1)
ATOMIC_FETCH_FUNCTION_ADDRSPACE(sub, float, FSub, f32, float, global, p1)

// The atomic_store Functions
// void atomic_store(volatile A *object, C desired)
// void atomic_store_explicit(volatile A *object,
//                            C desired,
//                            memory_order order)
// void atomic_store_explicit(volatile A *object,
//                            C desired,
//                            memory_order order,
//                            memory_scope scope)

#define ATOMIC_STORE_FUNCTION(OCL_TYPE, IGC_TYPE_ABBR, IGC_TYPE) \
INLINE void OVERLOADABLE atomic_store(volatile generic atomic_##OCL_TYPE *object, OCL_TYPE operand) \
{ \
  atomic_store_explicit(object, operand, memory_order_seq_cst); \
} \
INLINE void OVERLOADABLE atomic_store_explicit(volatile generic atomic_##OCL_TYPE *object, OCL_TYPE operand, memory_order order) \
{ \
  atomic_store_explicit(object, operand, order, memory_scope_device); \
} \
INLINE void OVERLOADABLE atomic_store_explicit(volatile generic atomic_##OCL_TYPE *object, OCL_TYPE operand, memory_order order, memory_scope scope) \
{ \
   __spirv_AtomicStore( \
            (generic IGC_TYPE *)object,                                                          \
            get_spirv_mem_scope(scope),                                                          \
            get_spirv_mem_order(order) |                                                         \
                get_spirv_mem_fence(get_fence((const generic void *)object)),                    \
            operand);                                                                            \
}

ATOMIC_STORE_FUNCTION(int, i32, int)
ATOMIC_STORE_FUNCTION(uint, i32, int)
ATOMIC_STORE_FUNCTION(float, f32, float)
#if defined(cl_khr_int64_base_atomics) && defined(cl_khr_int64_extended_atomics)
ATOMIC_STORE_FUNCTION(long, i64, long)
ATOMIC_STORE_FUNCTION(ulong, i64, long)
ATOMIC_STORE_FUNCTION(double, f64, double)
#endif // defined(cl_khr_int64_base_atomics) && defined(cl_khr_int64_extended_atomics)
#if defined(cl_intel_bfloat16_atomics)
ATOMIC_STORE_FUNCTION(ushort, i16, short)
#endif // defined(cl_intel_bfloat16_atomics)

// The atomic_load Functions
// C atomic_load(volatile A *object)
// C atomic_load_explicit(volatile A *object,
//                        memory_order order)
// C atomic_load_explicit(volatile A *object,
//                        memory_order order,
//                        memory_scope scope)

#define ATOMIC_LOAD_FUNCTION(OCL_TYPE, IGC_TYPE_ABBR, IGC_TYPE) \
INLINE OCL_TYPE OVERLOADABLE atomic_load(volatile generic atomic_##OCL_TYPE *object) \
{ \
  return atomic_load_explicit(object, memory_order_seq_cst);    \
    } \
INLINE OCL_TYPE OVERLOADABLE atomic_load_explicit(volatile generic atomic_##OCL_TYPE *object, memory_order order) \
{ \
  return atomic_load_explicit(object, order, memory_scope_device); \
} \
INLINE OCL_TYPE OVERLOADABLE atomic_load_explicit(volatile generic atomic_##OCL_TYPE *object, memory_order order, memory_scope scope) \
{ \
  return __spirv_AtomicLoad( \
            (generic IGC_TYPE *)object,                                                          \
            get_spirv_mem_scope(scope),                                                          \
            get_spirv_mem_order(order) |                                                         \
                get_spirv_mem_fence(get_fence((const generic void *)object)));                   \
}

ATOMIC_LOAD_FUNCTION(int, i32, int)
ATOMIC_LOAD_FUNCTION(uint, i32, int)
ATOMIC_LOAD_FUNCTION(float, f32, float)
#if defined(cl_khr_int64_base_atomics) && defined(cl_khr_int64_extended_atomics)
ATOMIC_LOAD_FUNCTION(long, i64, long)
ATOMIC_LOAD_FUNCTION(ulong, i64, long)
ATOMIC_LOAD_FUNCTION(double, f64, double)
#endif // defined(cl_khr_int64_base_atomics) && defined(cl_khr_int64_extended_atomics)
#if defined(cl_intel_bfloat16_atomics)
ATOMIC_LOAD_FUNCTION(ushort, i16, short)
#endif // defined(cl_intel_bfloat16_atomics)

// The atomic_exchange Functions
// C atomic_exchange(volatile A *object, C desired)
// C atomic_exchange_explicit(volatile A *object,
//                            C desired,
//                            memory_order order)
// C atomic_exchange_explicit(volatile A *object,
//                            C desired,
//                            memory_order order,
//                            memory_scope scope)

#define ATOMIC_EXCHANGE_FUNCTION(OCL_TYPE, IGC_TYPE_ABBR, IGC_TYPE) \
INLINE OCL_TYPE OVERLOADABLE atomic_exchange(volatile generic atomic_##OCL_TYPE *object, OCL_TYPE desired) \
{ \
  return atomic_exchange_explicit(object, desired, memory_order_seq_cst, memory_scope_device); \
} \
INLINE OCL_TYPE OVERLOADABLE atomic_exchange_explicit(volatile generic atomic_##OCL_TYPE *object, OCL_TYPE desired, memory_order order) \
{ \
  return atomic_exchange_explicit(object, desired, order, memory_scope_device); \
} \
INLINE OCL_TYPE OVERLOADABLE atomic_exchange_explicit(volatile generic atomic_##OCL_TYPE *object, OCL_TYPE desired, memory_order order, memory_scope scope) \
{ \
  return __spirv_AtomicExchange( \
            (generic IGC_TYPE *)object,                                                \
            get_spirv_mem_scope(scope),                                                \
            get_spirv_mem_order(order) |                                               \
                get_spirv_mem_fence(get_fence((const generic void *)object)),          \
            desired);                                                                  \
}

ATOMIC_EXCHANGE_FUNCTION(int, i32, int)
ATOMIC_EXCHANGE_FUNCTION(uint, i32, int)
ATOMIC_EXCHANGE_FUNCTION(float, f32, float)
#if defined(cl_khr_int64_base_atomics) && defined(cl_khr_int64_extended_atomics)
ATOMIC_EXCHANGE_FUNCTION(long, i64, long)
ATOMIC_EXCHANGE_FUNCTION(ulong, i64, long)
ATOMIC_EXCHANGE_FUNCTION(double, f64, double)
#endif // defined(cl_khr_int64_base_atomics) && defined(cl_khr_int64_extended_atomics)

// The atomic_compare_exchange Functions
// bool atomic_compare_exchange_strong(
//   volatile A *object,
//   C *expected, C desired)
// bool atomic_compare_exchange_strong_explicit(
//   volatile A *object,
//   C *expected,
//   C desired,
//   memory_order success,
//   memory_order failure)
// bool atomic_compare_exchange_strong_explicit(
//   volatile A *object,
//   C *expected,
//   C desired,
//   memory_order success,
//   memory_order failure,
//   memory_scope scope)
// bool atomic_compare_exchange_weak(
//   volatile A *object,
//   C *expected, C desired)
// bool atomic_compare_exchange_weak_explicit(
//   volatile A *object,
//   C *expected,
//   C desired,
//   memory_order success,
//   memory_order failure)
// bool atomic_compare_exchange_weak_explicit(
//   volatile A *object,
//   C *expected,
//   C desired,
//   memory_order success,
//   memory_order failure,

#define ATOMIC_COMPARE_EXCHANGE_FUNCTION(OCL_TYPE, STRENGTH, OPCODE, IGC_TYPE_ABBR, IGC_TYPE) \
INLINE bool OVERLOADABLE atomic_compare_exchange_##STRENGTH(volatile generic atomic_##OCL_TYPE *object, generic OCL_TYPE *expected, OCL_TYPE desired) \
{ \
  return atomic_compare_exchange_##STRENGTH##_explicit(object, expected, desired, memory_order_seq_cst, memory_order_seq_cst, memory_scope_device); \
} \
INLINE bool OVERLOADABLE atomic_compare_exchange_##STRENGTH##_explicit(volatile generic atomic_##OCL_TYPE *object, generic OCL_TYPE *expected, \
                                                                       OCL_TYPE desired, memory_order success, memory_order failure) \
{ \
  return atomic_compare_exchange_##STRENGTH##_explicit(object, expected, desired, success, failure, memory_scope_device); \
} \
INLINE bool OVERLOADABLE atomic_compare_exchange_##STRENGTH##_explicit(volatile generic atomic_##OCL_TYPE *object, generic OCL_TYPE *expected, \
                                                                       OCL_TYPE desired, memory_order success, memory_order failure, memory_scope scope) \
{ \
  OCL_TYPE expected_start = (*expected);\
  IGC_TYPE before =                                                                 \
            __spirv_Atomic##OPCODE( \
                (volatile generic IGC_TYPE *)object,                                  \
                get_spirv_mem_scope(scope),                                           \
                success,                                                              \
                failure,                                                              \
                as_##IGC_TYPE(desired),                                               \
                as_##IGC_TYPE(expected_start)); \
  bool ret = false; \
  if (before == as_##IGC_TYPE(expected_start))                                        \
        { \
    ret = true; \
  } \
  else { \
    *expected = as_##OCL_TYPE(before); \
    ret = false; \
  } \
  if( success != memory_order_release ) \
  { \
    atomic_work_item_fence( CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE, success, scope); \
  } \
  return ret; \
}

ATOMIC_COMPARE_EXCHANGE_FUNCTION(int, strong, CompareExchange, i32, int)
ATOMIC_COMPARE_EXCHANGE_FUNCTION(uint, strong, CompareExchange, i32, int)
ATOMIC_COMPARE_EXCHANGE_FUNCTION(float, strong, CompareExchange, f32, float)
ATOMIC_COMPARE_EXCHANGE_FUNCTION(int, weak, CompareExchangeWeak, i32, int)
ATOMIC_COMPARE_EXCHANGE_FUNCTION(uint, weak, CompareExchangeWeak, i32, int)
ATOMIC_COMPARE_EXCHANGE_FUNCTION(float, weak, CompareExchangeWeak, i32, int)
#if defined(cl_khr_int64_base_atomics) && defined(cl_khr_int64_extended_atomics)
ATOMIC_COMPARE_EXCHANGE_FUNCTION(long, strong, CompareExchange, i64, long)
ATOMIC_COMPARE_EXCHANGE_FUNCTION(ulong, strong, CompareExchange, i64, long)
ATOMIC_COMPARE_EXCHANGE_FUNCTION(long, weak, CompareExchangeWeak, i64, long)
ATOMIC_COMPARE_EXCHANGE_FUNCTION(ulong, weak, CompareExchangeWeak, i64, long)
#endif // defined(cl_khr_int64_base_atomics) && defined(cl_khr_int64_extended_atomics)

// The atomic_flag_test_and_set Functions
// bool atomic_flag_test_and_set(
//   volatile atomic_flag *object)
// bool atomic_flag_test_and_set_explicit(
//   volatile atomic_flag *object,
//   memory_order order)
// bool atomic_flag_test_and_set_explicit(
//   volatile atomic_flag *object,
//   memory_order order,
//   memory_scope scope)

bool __attribute__((overloadable)) atomic_flag_test_and_set(volatile generic atomic_flag *object)
{
  return atomic_flag_test_and_set_explicit(object, memory_order_seq_cst);
}
bool __attribute__((overloadable)) atomic_flag_test_and_set_explicit(volatile generic atomic_flag *object, memory_order order)
{
  return atomic_flag_test_and_set_explicit(object, order, memory_scope_device);
}
bool __attribute__((overloadable)) atomic_flag_test_and_set_explicit(volatile generic atomic_flag *object, memory_order order, memory_scope scope)
{
    return __spirv_AtomicFlagTestAndSet(
        (volatile generic uint *)object,
        get_spirv_mem_scope(scope),
        get_spirv_mem_order(order) |
            get_spirv_mem_fence(get_fence((const generic void *)object)));
}

// The atomic_flag_clear Functions
// void atomic_flag_clear(volatile atomic_flag *object)
// void atomic_flag_clear_explicit(
//   volatile atomic_flag *object,
//   memory_order order)
// void atomic_flag_clear_explicit(
//   volatile atomic_flag *object,
//   memory_order order,
//   memory_scope scope)

void __attribute__((overloadable)) atomic_flag_clear(volatile generic atomic_flag *object)
{
  atomic_flag_clear_explicit(object, memory_order_seq_cst);
}
void __attribute__((overloadable)) atomic_flag_clear_explicit(volatile generic atomic_flag *object, memory_order order)
{
  atomic_flag_clear_explicit(object, order, memory_scope_device);
}
void __attribute__((overloadable)) atomic_flag_clear_explicit(volatile generic atomic_flag *object, memory_order order, memory_scope scope)
{
    __spirv_AtomicFlagClear(
        (volatile generic uint *)object,
        get_spirv_mem_scope(scope),
        get_spirv_mem_order(order) |
            get_spirv_mem_fence(get_fence((const generic void *)object)));
}

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
