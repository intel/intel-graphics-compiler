/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Atomic Instructions

#include "../Headers/spirv.h"
#include "include/atomic_fence_impl.h"

#define ATOMIC_FLAG_TRUE 1
#define ATOMIC_FLAG_FALSE 0

#define SEMANTICS_PRE_OP_NEED_FENCE ( Release | AcquireRelease | SequentiallyConsistent )

#define SEMANTICS_POST_OP_NEEDS_FENCE ( Acquire | AcquireRelease | SequentiallyConsistent )

__local int* __builtin_IB_get_local_lock();
__global int* __builtin_IB_get_global_lock();
void __builtin_IB_eu_thread_pause(uint value);
void __intel_memfence_handler(bool flushRW, bool isGlobal, bool invalidateL1, bool evictL1, Scope_t scope);

#define LOCAL_SPINLOCK_START() \
  { \
  volatile bool done = false; \
  while(!done) { \
       if(BIF_FLAG_CTRL_GET(HasThreadPauseSupport)) \
            __builtin_IB_eu_thread_pause(32); \
       if(__spirv_AtomicCompareExchange(__builtin_IB_get_local_lock(), Device, Relaxed, Relaxed, 1, 0) == 0) {

#define LOCAL_SPINLOCK_END() \
            done = true; \
            __spirv_AtomicStore(__builtin_IB_get_local_lock(), Device, SequentiallyConsistent | WorkgroupMemory, 0); \
  }}}

#define GLOBAL_SPINLOCK_START() \
  { \
  volatile bool done = false; \
  while(!done) { \
       __builtin_IB_eu_thread_pause(32); \
       if(__spirv_AtomicCompareExchange(__builtin_IB_get_global_lock(), Device, Relaxed, Relaxed, 1, 0) == 0) {

#define GLOBAL_SPINLOCK_END() \
            done = true; \
            __spirv_AtomicStore(__builtin_IB_get_global_lock(), Device, SequentiallyConsistent | CrossWorkgroupMemory, 0); \
  }}}

// This fencing scheme allows us to obey the memory model when coherency is
// enabled or disabled.  Because the L3$ has 2 pipelines (cohereny&atomics and
// non-coherant) the fences guarentee the memory model is followed when coherency
// is disabled.
//
// When coherency is enabled, though, all HDC traffic uses the same L3$ pipe so
// these fences would not be needed.  The compiler is agnostic to coherency
// being enabled or disbled so we asume the worst case.


#define atomic_operation_1op( INTRINSIC, TYPE, Pointer, Scope, Semantics, Value, isGlobal )   \
{                                                                                             \
    FENCE_PRE_OP((Scope), (Semantics), isGlobal)                                              \
    TYPE result = INTRINSIC( (Pointer), (Value) );                                            \
    FENCE_POST_OP((Scope), (Semantics), isGlobal)                                             \
    return result;                                                                            \
}

#define atomic_operation_1op_as_float( INTRINSIC, TYPE, Pointer, Scope, Semantics, Value, isGlobal )\
{                                                                                             \
    FENCE_PRE_OP((Scope), (Semantics), isGlobal)                                              \
    TYPE result = as_float(INTRINSIC( (Pointer), (Value) ));                                  \
    FENCE_POST_OP((Scope), (Semantics), isGlobal)                                             \
    return result;                                                                            \
}

#define atomic_operation_1op_as_double( INTRINSIC, TYPE, Pointer, Scope, Semantics, Value, isGlobal )\
{                                                                                             \
    FENCE_PRE_OP((Scope), (Semantics), isGlobal)                                              \
    TYPE result = as_double(INTRINSIC( (Pointer), (Value) ));                                  \
    FENCE_POST_OP((Scope), (Semantics), isGlobal)                                             \
    return result;                                                                            \
}

#define atomic_operation_1op_as_half( INTRINSIC, TYPE, Pointer, Scope, Semantics, Value, isGlobal )\
{                                                                                             \
    FENCE_PRE_OP((Scope), (Semantics), isGlobal)                                              \
    TYPE result = as_half(INTRINSIC( (Pointer), (Value) ));                                  \
    FENCE_POST_OP((Scope), (Semantics), isGlobal)                                             \
    return result;                                                                            \
}

#define atomic_operation_0op( INTRINSIC, TYPE, Pointer, Scope, Semantics, isGlobal )          \
{                                                                                             \
    FENCE_PRE_OP((Scope), (Semantics), isGlobal)                                              \
    TYPE result = INTRINSIC( (Pointer) );                                                     \
    FENCE_POST_OP((Scope), (Semantics), isGlobal)                                             \
    return result;                                                                            \
}

#define atomic_cmpxhg( INTRINSIC, TYPE, Pointer, Scope, Semantics, Value, Comp, isGlobal )\
{                                                                                         \
    FENCE_PRE_OP((Scope), (Semantics), isGlobal)                                          \
    TYPE result = INTRINSIC( (Pointer), (Comp), (Value) );                                \
    FENCE_POST_OP((Scope), (Semantics), isGlobal)                                         \
    return result;                                                                        \
}

#define atomic_cmpxhg_as_float( INTRINSIC, TYPE, Pointer, Scope, Semantics, Value, Comp, isGlobal )\
{                                                                                         \
    FENCE_PRE_OP((Scope), (Semantics), isGlobal)                                          \
    TYPE result = as_float(INTRINSIC( (Pointer), (Comp), (Value) ));                      \
    FENCE_POST_OP((Scope), (Semantics), isGlobal)                                         \
    return result;                                                                        \
}


// Atomic loads/stores must be implemented with an atomic operation - While our HDC has an in-order
// pipeline the L3$ has 2 pipelines - coherant and non-coherant.  Even when coherency is disabled atomics
// will still go down the coherant pipeline.  The 2 L3$ pipes do not guarentee order of operations between
// themselves.

// Since we dont have specialized atomic load/store HDC message we're using atomic_or( a, 0x0 ) to emulate
// an atomic load since it does not modify the in memory value and returns the 'old' value. atomic store
// can be implemented with an atomic_exchance with the return value ignored.

int __attribute__((overloadable)) __spirv_AtomicLoad( __private int *Pointer, int Scope, int Semantics )
{
    return *Pointer;
}

int __attribute__((overloadable)) __spirv_AtomicLoad( __global int *Pointer, int Scope, int Semantics )
{
    return __spirv_AtomicOr( Pointer, Scope, Semantics, 0 );
}

int __attribute__((overloadable)) __spirv_AtomicLoad( __local int *Pointer, int Scope, int Semantics )
{
    return __spirv_AtomicOr( Pointer, Scope, Semantics, 0 );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int __attribute__((overloadable)) __spirv_AtomicLoad( __generic int *Pointer, int Scope, int Semantics )
{
    return __spirv_AtomicOr( Pointer, Scope, Semantics, 0 );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

long __attribute__((overloadable)) __spirv_AtomicLoad( __private long *Pointer, int Scope, int Semantics )
{
    return *Pointer;
}

long __attribute__((overloadable)) __spirv_AtomicLoad( __global long *Pointer, int Scope, int Semantics )
{
    return __spirv_AtomicOr( Pointer, Scope, Semantics, 0 );
}

long __attribute__((overloadable)) __spirv_AtomicLoad( __local long *Pointer, int Scope, int Semantics )
{
    return __spirv_AtomicOr( Pointer, Scope, Semantics, 0 );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long __attribute__((overloadable)) __spirv_AtomicLoad( __generic long *Pointer, int Scope, int Semantics )
{
    return __spirv_AtomicOr( Pointer, Scope, Semantics, 0 );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

#if defined(cl_intel_bfloat16_atomics)
short __attribute__((overloadable)) __spirv_AtomicLoad( __private short *Pointer, int Scope, int Semantics )
{
    return *Pointer;
}

short __attribute__((overloadable)) __spirv_AtomicLoad( __global short *Pointer, int Scope, int Semantics )
{
    return __spirv_AtomicOr( Pointer, Scope, Semantics, 0 );
}

short __attribute__((overloadable)) __spirv_AtomicLoad( __local short *Pointer, int Scope, int Semantics )
{
    return __spirv_AtomicOr( Pointer, Scope, Semantics, 0 );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

short __attribute__((overloadable)) __spirv_AtomicLoad( __generic short *Pointer, int Scope, int Semantics )
{
    return __spirv_AtomicOr( Pointer, Scope, Semantics, 0 );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_intel_bfloat16_atomics)

float __attribute__((overloadable)) __spirv_AtomicLoad( __private float *Pointer, int Scope, int Semantics )
{
    return *Pointer;
}


float __attribute__((overloadable)) __spirv_AtomicLoad( __global float *Pointer, int Scope, int Semantics )
{
    return as_float( __spirv_AtomicOr( (__global int*)Pointer, Scope, Semantics, 0 ) );
}

float __attribute__((overloadable)) __spirv_AtomicLoad( __local float *Pointer, int Scope, int Semantics )
{
    return as_float( __spirv_AtomicOr( (__local int*)Pointer, Scope, Semantics, 0 ) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float __attribute__((overloadable)) __spirv_AtomicLoad( __generic float *Pointer, int Scope, int Semantics )
{
    return as_float( __spirv_AtomicOr( (volatile __generic int*)Pointer, Scope, Semantics, 0 ) );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp64)
#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
double __attribute__((overloadable)) __spirv_AtomicLoad( __private double *Pointer, int Scope, int Semantics )
{
    return *Pointer;
}


double __attribute__((overloadable)) __spirv_AtomicLoad( __global double *Pointer, int Scope, int Semantics )
{
    return as_double( __spirv_AtomicOr( (__global long*)Pointer, Scope, Semantics, 0 ) );
}

double __attribute__((overloadable)) __spirv_AtomicLoad( __local double *Pointer, int Scope, int Semantics )
{
    return as_double( __spirv_AtomicOr( (__local long*)Pointer, Scope, Semantics, 0 ) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double __attribute__((overloadable)) __spirv_AtomicLoad( __generic double *Pointer, int Scope, int Semantics )
{
    return as_double( __spirv_AtomicOr( (__generic long*)Pointer, Scope, Semantics, 0 ) );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
half __attribute__((overloadable)) __spirv_AtomicLoad(__private half* Pointer, int Scope, int Semantics)
{
    return *Pointer;
}

half __attribute__((overloadable)) __spirv_AtomicLoad(__global half* Pointer, int Scope, int Semantics)
{
    atomic_operation_1op_as_half(__builtin_IB_atomic_or_global_i16, half, (__global short*)Pointer, Scope, Semantics, 0, true);
}

half __attribute__((overloadable)) __spirv_AtomicLoad(__local half* Pointer, int Scope, int Semantics)
{
    atomic_operation_1op_as_half(__builtin_IB_atomic_or_local_i16, half, (__local short*)Pointer, Scope, Semantics, 0, false);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

half __attribute__((overloadable)) __spirv_AtomicLoad(__generic half* Pointer, int Scope, int Semantics)
{
    __builtin_assume((__local half*)Pointer != 0);
    if (__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicLoad((__local half*)Pointer, Scope, Semantics);
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicLoad((__private int*)Pointer, Scope, Semantics);
    }
    else
    {
        return __spirv_AtomicLoad((__global half*)Pointer, Scope, Semantics);
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_fp16)

// Atomic Stores


void __attribute__((overloadable)) __spirv_AtomicStore( __private int *Pointer, int Scope, int Semantics, int Value )
{
    *Pointer = Value;
}


void __attribute__((overloadable)) __spirv_AtomicStore( __global int *Pointer, int Scope, int Semantics, int Value )
{
    __spirv_AtomicExchange( Pointer, Scope, Semantics, Value );
}


void __attribute__((overloadable)) __spirv_AtomicStore( __local int *Pointer, int Scope, int Semantics, int Value )
{
    __spirv_AtomicExchange( Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void __attribute__((overloadable)) __spirv_AtomicStore( __generic int *Pointer, int Scope, int Semantics, int Value )
{
    __spirv_AtomicExchange( Pointer, Scope, Semantics, Value );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

void __attribute__((overloadable)) __spirv_AtomicStore( __private long *Pointer, int Scope, int Semantics, long Value )
{
    *Pointer = Value;
}


void __attribute__((overloadable)) __spirv_AtomicStore( __global long *Pointer, int Scope, int Semantics, long Value )
{
    __spirv_AtomicExchange( Pointer, Scope, Semantics, Value );
}


void __attribute__((overloadable)) __spirv_AtomicStore( __local long *Pointer, int Scope, int Semantics, long Value )
{
    __spirv_AtomicExchange( Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void __attribute__((overloadable)) __spirv_AtomicStore( __generic long *Pointer, int Scope, int Semantics, long Value )
{
    __spirv_AtomicExchange( Pointer, Scope, Semantics, Value );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

#if defined(cl_intel_bfloat16_atomics)
void __attribute__((overloadable)) __spirv_AtomicStore( __private short *Pointer, int Scope, int Semantics, short Value )
{
    *Pointer = Value;
}

void __attribute__((overloadable)) __spirv_AtomicStore( __global short *Pointer, int Scope, int Semantics, short Value )
{
    __spirv_AtomicExchange( Pointer, Scope, Semantics, Value );
}

void __attribute__((overloadable)) __spirv_AtomicStore( __local short *Pointer, int Scope, int Semantics, short Value )
{
    __spirv_AtomicExchange( Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void __attribute__((overloadable)) __spirv_AtomicStore( __generic short *Pointer, int Scope, int Semantics, short Value )
{
    __spirv_AtomicExchange( Pointer, Scope, Semantics, Value );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_intel_bfloat16_atomics)


void __attribute__((overloadable)) __spirv_AtomicStore( __private float *Pointer, int Scope, int Semantics, float Value )
{
    __spirv_AtomicExchange( Pointer, Scope, Semantics, Value );
}


void __attribute__((overloadable)) __spirv_AtomicStore( __global float *Pointer, int Scope, int Semantics, float Value )
{
    __spirv_AtomicExchange( Pointer, Scope, Semantics, Value );
}


void __attribute__((overloadable)) __spirv_AtomicStore( __local float *Pointer, int Scope, int Semantics, float Value )
{
    __spirv_AtomicExchange( Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void __attribute__((overloadable)) __spirv_AtomicStore( __generic float *Pointer, int Scope, int Semantics, float Value )
{
    __spirv_AtomicExchange( Pointer, Scope, Semantics, Value );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp64)
#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

void __attribute__((overloadable)) __spirv_AtomicStore( __private double *Pointer, int Scope, int Semantics, double Value )
{
    __spirv_AtomicExchange( Pointer, Scope, Semantics, Value );
}


void __attribute__((overloadable)) __spirv_AtomicStore( __global double *Pointer, int Scope, int Semantics, double Value )
{
    __spirv_AtomicExchange( Pointer, Scope, Semantics, Value );
}


void __attribute__((overloadable)) __spirv_AtomicStore( __local double *Pointer, int Scope, int Semantics, double Value )
{
    __spirv_AtomicExchange( Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void __attribute__((overloadable)) __spirv_AtomicStore( __generic double *Pointer, int Scope, int Semantics, double Value )
{
    __spirv_AtomicExchange( Pointer, Scope, Semantics, Value );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
void __attribute__((overloadable)) __spirv_AtomicStore(__private half* Pointer, int Scope, int Semantics, half Value)
{
    __spirv_AtomicExchange(Pointer, Scope, Semantics, Value);
}

void __attribute__((overloadable)) __spirv_AtomicStore(__global half* Pointer, int Scope, int Semantics, half Value)
{
    __spirv_AtomicExchange(Pointer, Scope, Semantics, Value);
}

void __attribute__((overloadable)) __spirv_AtomicStore(__local half* Pointer, int Scope, int Semantics, half Value)
{
    __spirv_AtomicExchange(Pointer, Scope, Semantics, Value);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void __attribute__((overloadable)) __spirv_AtomicStore(__generic half* Pointer, int Scope, int Semantics, half Value)
{
    __spirv_AtomicExchange(Pointer, Scope, Semantics, Value);
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_fp16)

// Atomic Exchange


int __attribute__((overloadable)) __spirv_AtomicExchange( __private int *Pointer, int Scope, int Semantics, int Value )
{
    uint orig = *Pointer;
    *Pointer = Value;
    return orig;
}


int __attribute__((overloadable)) __spirv_AtomicExchange( __global int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xchg_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}


int __attribute__((overloadable)) __spirv_AtomicExchange( __local int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xchg_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int __attribute__((overloadable)) __spirv_AtomicExchange( __generic int *Pointer, int Scope, int Semantics, int Value )
{
    __builtin_assume((__local int*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_xchg_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value, false );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicExchange((__private int*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_xchg_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
    }

}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_base_atomics)
long __attribute__((overloadable)) __spirv_AtomicExchange( __private long *Pointer, int Scope, int Semantics, long Value )
{
    ulong orig = *Pointer;
    *Pointer = Value;
    return orig;
}


long __attribute__((overloadable)) __spirv_AtomicExchange( __global long *Pointer, int Scope, int Semantics, long Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xchg_global_i64, ulong, (global long*)Pointer, Scope, Semantics, Value, true );
}

enum IntAtomicOp
{
    ATOMIC_IADD64,
    ATOMIC_SUB64,
    ATOMIC_XCHG64,
    ATOMIC_AND64,
    ATOMIC_OR64,
    ATOMIC_XOR64,
    ATOMIC_IMIN64,
    ATOMIC_IMAX64,
    ATOMIC_UMAX64,
    ATOMIC_UMIN64
};

// handle int64 SLM atomic add/sub/xchg/and/or/xor/umax/umin
ulong OVERLOADABLE __intel_atomic_binary( enum IntAtomicOp atomicOp, volatile __local ulong *Pointer,
    uint Scope, uint Semantics, ulong Value )
{
    if (BIF_FLAG_CTRL_GET(HasInt64SLMAtomicCAS))
    {
        ulong orig, newVal;
        FENCE_PRE_OP(Scope, Semantics, false)
        do
        {
            orig = *Pointer;
            switch (atomicOp)
            {
                case ATOMIC_UMIN64: newVal = ( orig < Value ) ? orig : Value; break;
                case ATOMIC_UMAX64: newVal = ( orig > Value ) ? orig : Value; break;
                default: break; // What should we do here? OCL doesn't have assert
            }
        } while (__builtin_IB_atomic_cmpxchg_local_i64(Pointer, orig, newVal) != orig);
        FENCE_POST_OP(Scope, Semantics, false)
        return orig;
    }

    ulong orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    LOCAL_SPINLOCK_START();
    orig = *Pointer;
    switch (atomicOp)
    {
        case ATOMIC_UMIN64: *Pointer = ( orig < Value ) ? orig : Value; break;
        case ATOMIC_UMAX64: *Pointer = ( orig > Value ) ? orig : Value; break;
        default: break; // What should we do here? OCL doesn't have assert
    }
    LOCAL_SPINLOCK_END();
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}

// handle int64 SLM atomic IMin and IMax
long OVERLOADABLE __intel_atomic_binary( enum IntAtomicOp atomicOp, volatile __local long *Pointer,
    uint Scope, uint Semantics, long Value )
{
    if (BIF_FLAG_CTRL_GET(HasInt64SLMAtomicCAS))
    {
        long orig, newVal;
        FENCE_PRE_OP(Scope, Semantics, false)
        do
        {
            orig = *Pointer;
            newVal = orig;
            switch (atomicOp)
            {
                case ATOMIC_IADD64: newVal += Value; break;
                case ATOMIC_SUB64:  newVal -= Value; break;
                case ATOMIC_AND64:  newVal &= Value; break;
                case ATOMIC_OR64:   newVal |= Value; break;
                case ATOMIC_XOR64:  newVal ^= Value; break;
                case ATOMIC_XCHG64: newVal = Value; break;
                case ATOMIC_IMIN64: newVal = ( orig < Value ) ? orig : Value; break;
                case ATOMIC_IMAX64: newVal = ( orig > Value ) ? orig : Value; break;
                default: break; // What should we do here? OCL doesn't have assert
            }
        } while (__builtin_IB_atomic_cmpxchg_local_i64(Pointer, orig, newVal) != orig);
        FENCE_POST_OP(Scope, Semantics, false)
        return orig;
    }

    long orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    LOCAL_SPINLOCK_START()
    orig = *Pointer;
    switch (atomicOp)
    {
        case ATOMIC_IADD64: *Pointer += Value; break;
        case ATOMIC_SUB64:  *Pointer -= Value; break;
        case ATOMIC_AND64:  *Pointer &= Value; break;
        case ATOMIC_OR64:   *Pointer |= Value; break;
        case ATOMIC_XOR64:  *Pointer ^= Value; break;
        case ATOMIC_XCHG64: *Pointer = Value; break;
        case ATOMIC_IMIN64: *Pointer = ( orig < Value ) ? orig : Value; break;
        case ATOMIC_IMAX64: *Pointer = ( orig > Value ) ? orig : Value; break;
        default: break; // What should we do here? OCL doesn't have assert
    }
    LOCAL_SPINLOCK_END()
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}

// handle uint64 SLM atomic inc/dec
ulong OVERLOADABLE __intel_atomic_unary( bool isInc, volatile __local ulong *Pointer, uint Scope, uint Semantics )
{
    if (BIF_FLAG_CTRL_GET(HasInt64SLMAtomicCAS))
    {
        long orig, newVal;
        FENCE_PRE_OP(Scope, Semantics, false)
        do
        {
            orig = *Pointer;
            newVal = isInc ? orig + 1 : orig - 1;
        } while (__builtin_IB_atomic_cmpxchg_local_i64(Pointer, orig, newVal) != orig);
        FENCE_POST_OP(Scope, Semantics, false)
        return orig;
    }

    ulong orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    LOCAL_SPINLOCK_START()
    orig = *Pointer;
    *Pointer = isInc ? orig + 1 : orig - 1;
    LOCAL_SPINLOCK_END()
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}

long __attribute__((overloadable)) __spirv_AtomicExchange( __local long *Pointer, int Scope, int Semantics, long Value )
{
    return __intel_atomic_binary(ATOMIC_XCHG64, Pointer, Scope, Semantics, Value);
}


#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long __attribute__((overloadable)) __spirv_AtomicExchange( __generic long *Pointer, int Scope, int Semantics, long Value )
{
    __builtin_assume((__local long*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicExchange((__local long*)Pointer, Scope, Semantics, Value);
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicExchange((__private long*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicExchange((__global long*)Pointer, Scope, Semantics, Value);
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics)

#if defined(cl_intel_bfloat16_atomics)
short __attribute__((overloadable)) __spirv_AtomicExchange( __private short *Pointer, int Scope, int Semantics, short Value )
{
    short orig = *Pointer;
    *Pointer = Value;
    return orig;
}

short __attribute__((overloadable)) __spirv_AtomicExchange( __global short *Pointer, int Scope, int Semantics, short Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xchg_global_i16, short, (global short*)Pointer, Scope, Semantics, Value, true );
}


short __attribute__((overloadable)) __spirv_AtomicExchange( __local short *Pointer, int Scope, int Semantics, short Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xchg_local_i16, short, (local short*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

short __attribute__((overloadable)) __spirv_AtomicExchange( __generic short *Pointer, int Scope, int Semantics, short Value )
{
    __builtin_assume((__local short*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_xchg_local_i16, short, (__local short*)Pointer, Scope, Semantics, Value, false );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicExchange((__private short*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_xchg_global_i16, short, (__global short*)Pointer, Scope, Semantics, Value, true );
    }

}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_intel_bfloat16_atomics)

float __attribute__((overloadable)) __spirv_AtomicExchange( __private float *Pointer, int Scope, int Semantics, float Value)
{
    float orig = *Pointer;

    *Pointer = Value;

    return orig;
}

float __attribute__((overloadable)) __spirv_AtomicExchange( __global float *Pointer, int Scope, int Semantics, float Value)
{
    atomic_operation_1op_as_float( __builtin_IB_atomic_xchg_global_i32, float, (global int*)Pointer, Scope, Semantics, as_int(Value), true );
}


float __attribute__((overloadable)) __spirv_AtomicExchange( __local float *Pointer, int Scope, int Semantics, float Value)
{
    atomic_operation_1op_as_float( __builtin_IB_atomic_xchg_local_i32, float, (local int*)Pointer, Scope, Semantics, as_int(Value), false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float __attribute__((overloadable)) __spirv_AtomicExchange( __generic float *Pointer, int Scope, int Semantics, float Value)
{
    __builtin_assume((__local float*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op_as_float( __builtin_IB_atomic_xchg_local_i32, float, (local int*)Pointer, Scope, Semantics, as_int(Value), false );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicExchange((__private int*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        atomic_operation_1op_as_float( __builtin_IB_atomic_xchg_global_i32, float, (global int*)Pointer, Scope, Semantics, as_int(Value), true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp64)
#if defined(cl_khr_int64_base_atomics)

double __attribute__((overloadable)) __spirv_AtomicExchange( __private double *Pointer, int Scope, int Semantics, double Value)
{
    return as_double(__spirv_AtomicExchange((__private long*) Pointer, Scope, Semantics, as_long(Value)));
}

double __attribute__((overloadable)) __spirv_AtomicExchange( __global double *Pointer, int Scope, int Semantics, double Value)
{
    return as_double(__spirv_AtomicExchange((__global long*) Pointer, Scope, Semantics, as_long(Value)));
}


double __attribute__((overloadable)) __spirv_AtomicExchange( __local double *Pointer, int Scope, int Semantics, double Value)
{
    return as_double(__spirv_AtomicExchange((__local long*) Pointer, Scope, Semantics, as_long(Value)));
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double __attribute__((overloadable)) __spirv_AtomicExchange( __generic double *Pointer, int Scope, int Semantics, double Value)
{
    __builtin_assume((__local double*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicExchange((__local double*) Pointer, Scope, Semantics, Value);
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicExchange((__private double*) Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicExchange((__global double*) Pointer, Scope, Semantics, Value);
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics)
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
half __attribute__((overloadable)) __spirv_AtomicExchange(__private half* Pointer, int Scope, int Semantics, half Value)
{
    half orig = *Pointer;

    *Pointer = Value;

    return orig;
}

half __attribute__((overloadable)) __spirv_AtomicExchange(__global half* Pointer, int Scope, int Semantics, half Value)
{
    atomic_operation_1op_as_half(__builtin_IB_atomic_xchg_global_i16, half, (__global short*)Pointer, Scope, Semantics, as_short(Value), true);
}

half __attribute__((overloadable)) __spirv_AtomicExchange(__local half* Pointer, int Scope, int Semantics, half Value)
{
    atomic_operation_1op_as_half(__builtin_IB_atomic_xchg_local_i16, half, (__local short*)Pointer, Scope, Semantics, as_short(Value), false);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

half __attribute__((overloadable)) __spirv_AtomicExchange(__generic half* Pointer, int Scope, int Semantics, half Value)
{
    __builtin_assume((__local half*)Pointer != 0);
    if (__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicExchange((__local half*)Pointer, Scope, Semantics, Value);
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicExchange((__private half*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicExchange((__global half*)Pointer, Scope, Semantics, Value);
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_fp16)

// Atomic Compare Exchange

// ============================================================================================
// Implementation of 16-bit AtomicCompareExchange from SPV_INTEL_16bit_atomics extension
// ============================================================================================

short __attribute__((overloadable)) __spirv_AtomicCompareExchange( __private short *Pointer, int Scope, int Equal, int Unequal, short Value, short Comparator)
{
    short orig = *Pointer;
    if( orig == Comparator )
    {
        *Pointer = Value;
    }
    return orig;
}


short __attribute__((overloadable)) __spirv_AtomicCompareExchange( __global short *Pointer, int Scope, int Equal, int Unequal, short Value, short Comparator)
{
    atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_global_i16, ushort, (global short*)Pointer, Scope, Equal, Value, Comparator, true );
}


short __attribute__((overloadable)) __spirv_AtomicCompareExchange( __local short *Pointer, int Scope, int Equal, int Unequal, short Value, short Comparator)
{
    atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_local_i16, ushort, (local short*)Pointer, Scope, Equal, Value, Comparator, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

short __attribute__((overloadable)) __spirv_AtomicCompareExchange( __generic short *Pointer, int Scope, int Equal, int Unequal, short Value, short Comparator)
{
        __builtin_assume((__local long*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicCompareExchange( (__local short*)Pointer, Scope, Equal, Unequal, Value, Comparator );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicCompareExchange((__private short*)Pointer, Scope, Equal, Unequal, Value, Comparator);
    }
    else
    {
        return __spirv_AtomicCompareExchange( (__global short*)Pointer, Scope, Equal, Unequal, Value, Comparator );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

// ============================================================================================
// End of 16-bit AtomicCompareExchange from SPV_INTEL_16bit_atomics extension
// ============================================================================================

int __attribute__((overloadable)) __spirv_AtomicCompareExchange( __private int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator)
{
    int orig = *Pointer;
    if( orig == Comparator )
    {
        *Pointer = Value;
    }
    return orig;
}


int __attribute__((overloadable)) __spirv_AtomicCompareExchange( __global int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator)
{
    atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_global_i32, uint, (global int*)Pointer, Scope, Equal, Value, Comparator, true );
}


int __attribute__((overloadable)) __spirv_AtomicCompareExchange( __local int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator)
{
    atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_local_i32, uint, (local int*)Pointer, Scope, Equal, Value, Comparator, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int __attribute__((overloadable)) __spirv_AtomicCompareExchange( __generic int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator)
{
    __builtin_assume((__local int*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_local_i32, uint, (__local int*)Pointer, Scope, Equal, Value, Comparator, false );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicCompareExchange((__private int*)Pointer, Scope, Equal, Unequal, Value, Comparator);
    }
    else
    {
        atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_global_i32, uint, (__global int*)Pointer, Scope, Equal, Value, Comparator, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


#if defined(cl_khr_int64_base_atomics)
long __attribute__((overloadable)) __spirv_AtomicCompareExchange( __private long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator)
{
    long orig = *Pointer;
    if( orig == Comparator )
    {
        *Pointer = Value;
    }
    return orig;
}


long __attribute__((overloadable)) __spirv_AtomicCompareExchange( __global long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator)
{
    atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_global_i64, ulong, (global long*)Pointer, Scope, Equal, Value, Comparator, true );
}


long __attribute__((overloadable)) __spirv_AtomicCompareExchange( __local long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator)
{
    if (BIF_FLAG_CTRL_GET(HasInt64SLMAtomicCAS))
    {
        atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_local_i64, ulong, (local long*)Pointer, Scope, Equal, Value, Comparator, false );
    }
    ulong orig;
    FENCE_PRE_OP(Scope, Equal, false)
    LOCAL_SPINLOCK_START()
    orig = *Pointer;
    if( orig == Comparator )
    {
        *Pointer = Value;
    }
    LOCAL_SPINLOCK_END()
    FENCE_POST_OP(Scope, Equal, false)
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long __attribute__((overloadable)) __spirv_AtomicCompareExchange( __generic long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator)
{
    __builtin_assume((__local long*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicCompareExchange( (__local long*)Pointer, Scope, Equal, Unequal, Value, Comparator );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicCompareExchange((__private long*)Pointer, Scope, Equal, Unequal, Value, Comparator);
    }
    else
    {
        return __spirv_AtomicCompareExchange( (__global long*)Pointer, Scope, Equal, Unequal, Value, Comparator );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics)


int __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak( __private int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator)
{
    return __spirv_AtomicCompareExchange( Pointer, Scope, Equal, Unequal, Value, Comparator );
}


int __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak( __global int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator)
{
    return __spirv_AtomicCompareExchange( Pointer, Scope, Equal, Unequal, Value, Comparator );
}


int __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak( __local int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator)
{
    return __spirv_AtomicCompareExchange( Pointer, Scope, Equal, Unequal, Value, Comparator );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak( __generic int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator)
{
    return __spirv_AtomicCompareExchange( Pointer, Scope, Equal, Unequal, Value, Comparator );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

// ================================================================================================
// Implementation of 16-bit AtomicCompareExchangeWeak from SPV_INTEL_16bit_atomics extension
// ================================================================================================

short __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak( __private short *Pointer, int Scope, int Equal, int Unequal, short Value, short Comparator)
{
    return __spirv_AtomicCompareExchange( Pointer, Scope, Equal, Unequal, Value, Comparator );
}


short __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak( __global short *Pointer, int Scope, int Equal, int Unequal, short Value, short Comparator)
{
    return __spirv_AtomicCompareExchange( Pointer, Scope, Equal, Unequal, Value, Comparator );
}


short __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak( __local short *Pointer, int Scope, int Equal, int Unequal, short Value, short Comparator)
{
    return __spirv_AtomicCompareExchange( Pointer, Scope, Equal, Unequal, Value, Comparator );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

short __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak( __generic short *Pointer, int Scope, int Equal, int Unequal, short Value, short Comparator)
{
    return __spirv_AtomicCompareExchange( Pointer, Scope, Equal, Unequal, Value, Comparator );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

// ============================================================================================
// End of 16-bit AtomicCompareExchangeWeak from SPV_INTEL_16bit_atomics extension
// ============================================================================================

#if defined(cl_khr_int64_base_atomics)
long __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak( __private long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator)
{
    return __spirv_AtomicCompareExchange( Pointer, Scope, Equal, Unequal, Value, Comparator );
}


long __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak( __global long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator)
{
    return __spirv_AtomicCompareExchange( Pointer, Scope, Equal, Unequal, Value, Comparator );
}


long __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak( __local long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator)
{
    return __spirv_AtomicCompareExchange( Pointer, Scope, Equal, Unequal, Value, Comparator );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak( __generic long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator)
{
    return __spirv_AtomicCompareExchange( Pointer, Scope, Equal, Unequal, Value, Comparator );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_int64_base_atomics)

// Atomic Increment


int __attribute__((overloadable)) __spirv_AtomicIIncrement( __private int *Pointer, int Scope, int Semantics )
{
    uint orig = *Pointer;
    *Pointer += 1;
    return orig;
}


int __attribute__((overloadable)) __spirv_AtomicIIncrement( __global int *Pointer, int Scope, int Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_inc_global_i32, uint, (global int*)Pointer, Scope, Semantics, true );
}


int __attribute__((overloadable)) __spirv_AtomicIIncrement( __local int *Pointer, int Scope, int Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_inc_local_i32, uint, (local int*)Pointer, Scope, Semantics, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int __attribute__((overloadable)) __spirv_AtomicIIncrement( __generic int *Pointer, int Scope, int Semantics )
{
    __builtin_assume((__local int*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_0op( __builtin_IB_atomic_inc_local_i32, uint, (__local int*)Pointer, Scope, Semantics, false );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicIIncrement((__private int*)Pointer, Scope, Semantics);
    }
    else
    {
        atomic_operation_0op( __builtin_IB_atomic_inc_global_i32, uint, (__global int*)Pointer, Scope, Semantics, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_base_atomics)
long __attribute__((overloadable)) __spirv_AtomicIIncrement( __private long *Pointer, int Scope, int Semantics )
{
    ulong orig = *Pointer;
    *Pointer += 1;
    return orig;
}


long __attribute__((overloadable)) __spirv_AtomicIIncrement( __global long *Pointer, int Scope, int Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_inc_global_i64, ulong, (global int*)Pointer, Scope, Semantics, true );
}


long __attribute__((overloadable)) __spirv_AtomicIIncrement( __local long *Pointer, int Scope, int Semantics )
{
    return __intel_atomic_unary(true, Pointer, Scope, Semantics);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long __attribute__((overloadable)) __spirv_AtomicIIncrement( __generic long *Pointer, int Scope, int Semantics )
{
    __builtin_assume((__local long*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicIIncrement((__local long*)Pointer, Scope, Semantics );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicIIncrement((__private long*)Pointer, Scope, Semantics);
    }
    else
    {
        return __spirv_AtomicIIncrement((__global long*)Pointer, Scope, Semantics );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_int64_base_atomics)

// Atomic Decrement


int __attribute__((overloadable)) __spirv_AtomicIDecrement( __private int *Pointer, int Scope, int Semantics )
{
    uint orig = *Pointer;

    *Pointer -= 1;

    return orig;
}

int __attribute__((overloadable)) __spirv_AtomicIDecrement( __global int *Pointer, int Scope, int Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_dec_global_i32, uint, (global int*)Pointer, Scope, Semantics, true );
}

int __attribute__((overloadable)) __spirv_AtomicIDecrement( __local int *Pointer, int Scope, int Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_dec_local_i32, uint, (local int*)Pointer, Scope, Semantics, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int __attribute__((overloadable)) __spirv_AtomicIDecrement( __generic int *Pointer, int Scope, int Semantics )
{
    __builtin_assume((__local int*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_0op( __builtin_IB_atomic_dec_local_i32, uint, (__local int*)Pointer, Scope, Semantics, false );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicIDecrement((__private int*)Pointer, Scope, Semantics);
    }
    else
    {
        atomic_operation_0op( __builtin_IB_atomic_dec_global_i32, uint, (__global int*)Pointer, Scope, Semantics, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_base_atomics)
long __attribute__((overloadable)) __spirv_AtomicIDecrement( __private long *Pointer, int Scope, int Semantics )
{
    ulong orig = *Pointer;
    *Pointer -= 1;
    return orig;
}

long __attribute__((overloadable)) __spirv_AtomicIDecrement( __global long *Pointer, int Scope, int Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_dec_global_i64, ulong, (global long*)Pointer, Scope, Semantics, true );
}

long __attribute__((overloadable)) __spirv_AtomicIDecrement( __local long *Pointer, int Scope, int Semantics )
{
    return __intel_atomic_unary(false, Pointer, Scope, Semantics);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long __attribute__((overloadable)) __spirv_AtomicIDecrement( __generic long *Pointer, int Scope, int Semantics )
{
    __builtin_assume((__local long*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicIDecrement( (__local long*)Pointer, Scope, Semantics );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicIDecrement((__private long*)Pointer, Scope, Semantics);
    }
    else
    {
        return __spirv_AtomicIDecrement( (__global long*)Pointer, Scope, Semantics );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_int64_base_atomics)


// Atomic IAdd


int __attribute__((overloadable)) __spirv_AtomicIAdd( __private int *Pointer, int Scope, int Semantics, int Value )
{
    uint orig = *Pointer;

    *Pointer += Value;

    return orig;
}


int __attribute__((overloadable)) __spirv_AtomicIAdd( __global int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_add_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}

int __attribute__((overloadable)) __spirv_AtomicIAdd( __local int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_add_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int __attribute__((overloadable)) __spirv_AtomicIAdd( __generic int *Pointer, int Scope, int Semantics, int Value )
{
    __builtin_assume((__local int*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_add_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value, false );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicIAdd((__private int*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_add_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
    }
}

#if defined(cl_khr_int64_base_atomics)
long __attribute__((overloadable)) __spirv_AtomicIAdd( __private long *Pointer, int Scope, int Semantics, long Value )
{
    ulong orig = *Pointer;
    *Pointer += Value;
    return orig;
}

long __attribute__((overloadable)) __spirv_AtomicIAdd( __global long *Pointer, int Scope, int Semantics, long Value )
{
    atomic_operation_1op( __builtin_IB_atomic_add_global_i64, ulong, (__global ulong*)Pointer, Scope, Semantics, Value, true );
}

long __attribute__((overloadable)) __spirv_AtomicIAdd( __local long *Pointer, int Scope, int Semantics, long Value )
{
    return __intel_atomic_binary(ATOMIC_IADD64, Pointer, Scope, Semantics, Value);
}


long __attribute__((overloadable)) __spirv_AtomicIAdd( __generic long *Pointer, int Scope, int Semantics, long Value )
{
    __builtin_assume((__local long*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicIAdd((__local long*)Pointer, Scope, Semantics, Value);
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicIAdd((__private long*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicIAdd((__global long*)Pointer, Scope, Semantics, Value);
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_int64_base_atomics)

// Atomic ISub

int __attribute__((overloadable)) __spirv_AtomicISub( __private int *Pointer, int Scope, int Semantics, int Value )
{
    uint orig = *Pointer;

    *Pointer -= Value;

    return orig;
}


int __attribute__((overloadable)) __spirv_AtomicISub( __global int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_sub_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}

int __attribute__((overloadable)) __spirv_AtomicISub( __local int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_sub_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int __attribute__((overloadable)) __spirv_AtomicISub( __generic int *Pointer, int Scope, int Semantics, int Value )
{
    __builtin_assume((__local int*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_sub_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value, false );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicISub((__private int*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_sub_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_base_atomics)
long __attribute__((overloadable)) __spirv_AtomicISub( __private long *Pointer, int Scope, int Semantics, long Value )
{
    ulong orig = *Pointer;
    *Pointer -= Value;
    return orig;
}


long __attribute__((overloadable)) __spirv_AtomicISub( __global long *Pointer, int Scope, int Semantics, long Value )
{
    atomic_operation_1op( __builtin_IB_atomic_sub_global_i64, ulong, (global long*)Pointer, Scope, Semantics, Value, true );
}


long __attribute__((overloadable)) __spirv_AtomicISub( __local long *Pointer, int Scope, int Semantics, long Value )
{
    return __intel_atomic_binary(ATOMIC_SUB64, Pointer, Scope, Semantics, Value);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long __attribute__((overloadable)) __spirv_AtomicISub( __generic long *Pointer, int Scope, int Semantics, long Value )
{
    __builtin_assume((__local long*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicISub((__local long*)Pointer, Scope, Semantics, Value );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicISub((__private long*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicISub((__global long*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics)


// Atomic SMin


int __attribute__((overloadable)) __spirv_AtomicSMin( __private int *Pointer, int Scope, int Semantics, int Value)
{
    int orig = *Pointer;
    *Pointer = ( orig < Value ) ? orig : Value;
    return orig;
}

int __attribute__((overloadable)) __spirv_AtomicSMin( __global int *Pointer, int Scope, int Semantics, int Value)
{
    atomic_operation_1op( __builtin_IB_atomic_min_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
}

int __attribute__((overloadable)) __spirv_AtomicSMin( __local int *Pointer, int Scope, int Semantics, int Value)
{
    atomic_operation_1op( __builtin_IB_atomic_min_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int __attribute__((overloadable)) __spirv_AtomicSMin( __generic int *Pointer, int Scope, int Semantics, int Value)
{
    __builtin_assume((__local int*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_min_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value, false );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicSMin((__private int*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_min_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_extended_atomics)

long __attribute__((overloadable)) __spirv_AtomicSMin( __private long *Pointer, int Scope, int Semantics, long Value)
{
    long orig = *Pointer;
    *Pointer = ( orig < Value ) ? orig : Value;
    return orig;
}

long __attribute__((overloadable)) __spirv_AtomicSMin( __global long *Pointer, int Scope, int Semantics, long Value)
{
    atomic_operation_1op( __builtin_IB_atomic_min_global_i64, ulong, (__global long*)Pointer, Scope, Semantics, Value, true );
}

long __attribute__((overloadable)) __spirv_AtomicSMin( __local long *Pointer, int Scope, int Semantics, long Value)
{
    return __intel_atomic_binary(ATOMIC_IMIN64, (volatile __local long *)Pointer, Scope, Semantics, Value);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long __attribute__((overloadable)) __spirv_AtomicSMin( __generic long *Pointer, int Scope, int Semantics, long Value)
{
    __builtin_assume((__local long*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicSMin((__local long*)Pointer, Scope, Semantics, Value );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicSMin((__private long*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicSMin((__global long*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

uint __attribute__((overloadable)) __spirv_AtomicUMin( __private uint *Pointer, int Scope, int Semantics, uint Value )
{
    uint orig = *Pointer;

    *Pointer = ( orig < Value ) ? orig : Value;

    return orig;
}

uint __attribute__((overloadable)) __spirv_AtomicUMin( __global uint *Pointer, int Scope, int Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_min_global_u32, uint, Pointer, Scope, Semantics, Value, true );
}

uint __attribute__((overloadable)) __spirv_AtomicUMin( __local uint *Pointer, int Scope, int Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_min_local_u32, uint, Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __attribute__((overloadable)) __spirv_AtomicUMin( __generic uint *Pointer, int Scope, int Semantics, uint Value )
{
    __builtin_assume((__local uint*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_min_local_u32, uint, (__local uint*)Pointer, Scope, Semantics, Value, false );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicUMin((__private uint*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_min_global_u32, uint, (__global uint*)Pointer, Scope, Semantics, Value, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_extended_atomics)

ulong __attribute__((overloadable)) __spirv_AtomicUMin( __private ulong *Pointer, int Scope, int Semantics, ulong Value )
{
    ulong orig = *Pointer;
    *Pointer = ( orig < Value ) ? orig : Value;
    return orig;
}

ulong __attribute__((overloadable)) __spirv_AtomicUMin( __global ulong *Pointer, int Scope, int Semantics, ulong Value )
{
    atomic_operation_1op( __builtin_IB_atomic_min_global_u64, ulong, Pointer, Scope, Semantics, Value, true );
}

ulong __attribute__((overloadable)) __spirv_AtomicUMin( __local ulong *Pointer, int Scope, int Semantics, ulong Value )
{
    return __intel_atomic_binary(ATOMIC_UMIN64, Pointer, Scope, Semantics, Value);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

ulong __attribute__((overloadable)) __spirv_AtomicUMin( __generic ulong *Pointer, int Scope, int Semantics, ulong Value )
{
    __builtin_assume((__local ulong*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicUMin( (__local ulong*)Pointer, Scope, Semantics, Value );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicUMin((__private ulong*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicUMin( (__global ulong*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

// Atomic SMax


int __attribute__((overloadable)) __spirv_AtomicSMax( __private int *Pointer, int Scope, int Semantics, int Value)
{
    int orig = *Pointer;
    *Pointer = ( orig > Value ) ? orig : Value;
    return orig;
}

int __attribute__((overloadable)) __spirv_AtomicSMax( __global int *Pointer, int Scope, int Semantics, int Value)
{
    atomic_operation_1op( __builtin_IB_atomic_max_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}

int __attribute__((overloadable)) __spirv_AtomicSMax( __local int *Pointer, int Scope, int Semantics, int Value)
{
    atomic_operation_1op( __builtin_IB_atomic_max_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int __attribute__((overloadable)) __spirv_AtomicSMax( __generic int *Pointer, int Scope, int Semantics, int Value)
{
    __builtin_assume((__local int*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_max_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value, false );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicSMax((__private int*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_max_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_extended_atomics)

long __attribute__((overloadable)) __spirv_AtomicSMax( __private long *Pointer, int Scope, int Semantics, long Value)
{
    long orig = *Pointer;
    *Pointer = ( orig > Value ) ? orig : Value;
    return orig;
}

long __attribute__((overloadable)) __spirv_AtomicSMax( __global long *Pointer, int Scope, int Semantics, long Value)
{
    atomic_operation_1op( __builtin_IB_atomic_max_global_i64, ulong, (global long*)Pointer, Scope, Semantics, Value, true );
}

long __attribute__((overloadable)) __spirv_AtomicSMax( __local long *Pointer, int Scope, int Semantics, long Value)
{
    return __intel_atomic_binary(ATOMIC_IMAX64, (volatile __local long *)Pointer, Scope, Semantics, Value);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long __attribute__((overloadable)) __spirv_AtomicSMax( __generic long *Pointer, int Scope, int Semantics, long Value)
{
    __builtin_assume((__local long*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicSMax( (__local long*)Pointer, Scope, Semantics, Value );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicSMax((__private long*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicSMax( (__global long*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

// Atomic UMax


uint __attribute__((overloadable)) __spirv_AtomicUMax( __private uint *Pointer, int Scope, int Semantics, uint Value )
{
    uint orig = *Pointer;

    *Pointer = ( orig > Value ) ? orig : Value;

    return orig;
}

uint __attribute__((overloadable)) __spirv_AtomicUMax( __global uint *Pointer, int Scope, int Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_max_global_u32, uint, Pointer, Scope, Semantics, Value, true );
}

uint __attribute__((overloadable)) __spirv_AtomicUMax( __local uint *Pointer, int Scope, int Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_max_local_u32, uint, Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __attribute__((overloadable)) __spirv_AtomicUMax( __generic uint *Pointer, int Scope, int Semantics, uint Value )
{
    __builtin_assume((__local uint*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_max_local_u32, uint, (__local uint*)Pointer, Scope, Semantics, Value, false );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicUMax((__private uint*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_max_global_u32, uint, (__global uint*)Pointer, Scope, Semantics, Value, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_extended_atomics)

ulong __attribute__((overloadable)) __spirv_AtomicUMax( __private ulong *Pointer, int Scope, int Semantics, ulong Value )
{
    ulong orig = *Pointer;
    *Pointer = ( orig > Value ) ? orig : Value;
    return orig;
}

ulong __attribute__((overloadable)) __spirv_AtomicUMax( __global ulong *Pointer, int Scope, int Semantics, ulong Value )
{
    atomic_operation_1op( __builtin_IB_atomic_max_global_u64, ulong, Pointer, Scope, Semantics, Value, true );
}

ulong __attribute__((overloadable)) __spirv_AtomicUMax( __local ulong *Pointer, int Scope, int Semantics, ulong Value )
{
    return __intel_atomic_binary(ATOMIC_UMAX64, Pointer, Scope, Semantics, Value);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

ulong __attribute__((overloadable)) __spirv_AtomicUMax( __generic ulong *Pointer, int Scope, int Semantics, ulong Value )
{
    __builtin_assume((__local ulong*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicUMax( (__local ulong*)Pointer, Scope, Semantics, Value );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicUMax((__private ulong*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicUMax( (__global ulong*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

// Atomic And


int __attribute__((overloadable)) __spirv_AtomicAnd( __private int *Pointer, int Scope, int Semantics, int Value )
{
    uint orig = *Pointer;
    *Pointer &= Value;
    return orig;
}

int __attribute__((overloadable)) __spirv_AtomicAnd( __global int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_and_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}

int __attribute__((overloadable)) __spirv_AtomicAnd( __local int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_and_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int __attribute__((overloadable)) __spirv_AtomicAnd( __generic int *Pointer, int Scope, int Semantics, int Value )
{
    __builtin_assume((__local int*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_and_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value, false );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicAnd((__private int*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_and_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_extended_atomics)

long __attribute__((overloadable)) __spirv_AtomicAnd( __private long *Pointer, int Scope, int Semantics, long Value )
{
    ulong orig = *Pointer;
    *Pointer &= Value;
    return orig;
}

long __attribute__((overloadable)) __spirv_AtomicAnd( __global long *Pointer, int Scope, int Semantics, long Value )
{
    atomic_operation_1op( __builtin_IB_atomic_and_global_i64, ulong, (global long*)Pointer, Scope, Semantics, Value, true );
}

long __attribute__((overloadable)) __spirv_AtomicAnd( __local long *Pointer, int Scope, int Semantics, long Value )
{
    return __intel_atomic_binary(ATOMIC_AND64, Pointer, Scope, Semantics, Value);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long __attribute__((overloadable)) __spirv_AtomicAnd( __generic long *Pointer, int Scope, int Semantics, long Value )
{
    __builtin_assume((__local long*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicAnd( (__local long*)Pointer, Scope, Semantics, Value );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicAnd((__private long*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicAnd( (__global long*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

// Atomic OR


int __attribute__((overloadable)) __spirv_AtomicOr( __private int *Pointer, int Scope, int Semantics, int Value )
{
    uint orig = *Pointer;
    *Pointer |= Value;
    return orig;
}

int __attribute__((overloadable)) __spirv_AtomicOr( __global int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_or_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}

int __attribute__((overloadable)) __spirv_AtomicOr( __local int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_or_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int __attribute__((overloadable)) __spirv_AtomicOr( __generic int *Pointer, int Scope, int Semantics, int Value )
{
    __builtin_assume((__local int*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_or_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value, false );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicOr((__private int*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_or_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_extended_atomics)

long __attribute__((overloadable)) __spirv_AtomicOr( __private long *Pointer, int Scope, int Semantics, long Value )
{
    ulong orig = *Pointer;
    *Pointer |= Value;
    return orig;
}

long __attribute__((overloadable)) __spirv_AtomicOr( __global long *Pointer, int Scope, int Semantics, long Value )
{
    atomic_operation_1op( __builtin_IB_atomic_or_global_i64, ulong, (global long*)Pointer, Scope, Semantics, Value, true );
}

long __attribute__((overloadable)) __spirv_AtomicOr( __local long *Pointer, int Scope, int Semantics, long Value )
{
    return __intel_atomic_binary(ATOMIC_OR64, Pointer, Scope, Semantics, Value);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long __attribute__((overloadable)) __spirv_AtomicOr( __generic long *Pointer, int Scope, int Semantics, long Value )
{
    __builtin_assume((__local long*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
      return __spirv_AtomicOr( (__local long*)Pointer, Scope, Semantics, Value );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicOr((__private long*)Pointer, Scope, Semantics, Value);
    }
    else
    {
      return __spirv_AtomicOr( (__global long*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

#if defined(cl_intel_bfloat16_atomics)
short __attribute__((overloadable)) __spirv_AtomicOr( __private short *Pointer, int Scope, int Semantics, short Value )
{
    short orig = *Pointer;
    *Pointer |= Value;
    return orig;
}

short __attribute__((overloadable)) __spirv_AtomicOr( __global short *Pointer, int Scope, int Semantics, short Value )
{
    atomic_operation_1op( __builtin_IB_atomic_or_global_i16, short, (global short*)Pointer, Scope, Semantics, Value, true );
}

short __attribute__((overloadable)) __spirv_AtomicOr( __local short *Pointer, int Scope, int Semantics, short Value )
{
    atomic_operation_1op( __builtin_IB_atomic_or_local_i16, short, (local short*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

short __attribute__((overloadable)) __spirv_AtomicOr( __generic short *Pointer, int Scope, int Semantics, short Value )
{
    __builtin_assume((__local short*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_or_local_i16, short, (__local short*)Pointer, Scope, Semantics, Value, false );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicOr((__private short*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_or_global_i16, short, (__global short*)Pointer, Scope, Semantics, Value, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_intel_bfloat16_atomics)

// Atomic Xor


int __attribute__((overloadable)) __spirv_AtomicXor( __private int *Pointer, int Scope, int Semantics, int Value )
{
    uint orig = *Pointer;
    *Pointer ^= Value;
    return orig;
}

int __attribute__((overloadable)) __spirv_AtomicXor( __global int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xor_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}

int __attribute__((overloadable)) __spirv_AtomicXor( __local int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xor_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int __attribute__((overloadable)) __spirv_AtomicXor( __generic int *Pointer, int Scope, int Semantics, int Value )
{
    __builtin_assume((__local int*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_xor_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value, false );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicXor((__private int*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_xor_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_extended_atomics)

long __attribute__((overloadable)) __spirv_AtomicXor( __private long *Pointer, int Scope, int Semantics, long Value )
{
    ulong orig = *Pointer;
    *Pointer ^= Value;
    return orig;
}

long __attribute__((overloadable)) __spirv_AtomicXor( __global long *Pointer, int Scope, int Semantics, long Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xor_global_i64, ulong, (global long*)Pointer, Scope, Semantics, Value, true );
}

long __attribute__((overloadable)) __spirv_AtomicXor( __local long *Pointer, int Scope, int Semantics, long Value )
{
    return __intel_atomic_binary(ATOMIC_XOR64, Pointer, Scope, Semantics, Value);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long __attribute__((overloadable)) __spirv_AtomicXor( __generic long *Pointer, int Scope, int Semantics, long Value )
{
    __builtin_assume((__local long*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicXor( (__local long*)Pointer, Scope, Semantics, Value );
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicXor((__private long*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicXor( (__global long*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

// Atomic FlagTestAndSet


bool __attribute__((overloadable)) __spirv_AtomicFlagTestAndSet( __private int *Pointer, int Scope, int Semantics )
{
    return (bool)__spirv_AtomicExchange( Pointer, Scope, Semantics, ATOMIC_FLAG_TRUE );
}

bool __attribute__((overloadable)) __spirv_AtomicFlagTestAndSet( __global int *Pointer, int Scope, int Semantics )
{
    return (bool)__spirv_AtomicExchange( Pointer, Scope, Semantics, ATOMIC_FLAG_TRUE );
}

bool __attribute__((overloadable)) __spirv_AtomicFlagTestAndSet( __local int *Pointer, int Scope, int Semantics )
{
    return (bool)__spirv_AtomicExchange( Pointer, Scope, Semantics, ATOMIC_FLAG_TRUE );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

bool __attribute__((overloadable)) __spirv_AtomicFlagTestAndSet( __generic int *Pointer, int Scope, int Semantics )
{
    return (bool)__spirv_AtomicExchange( Pointer, Scope, Semantics, ATOMIC_FLAG_TRUE );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


// Atomic FlagClear


void __attribute__((overloadable)) __spirv_AtomicFlagClear( __private int *Pointer, int Scope, int Semantics )
{
    __spirv_AtomicStore( Pointer, Scope, Semantics, ATOMIC_FLAG_FALSE );
}

void __attribute__((overloadable)) __spirv_AtomicFlagClear( __global int *Pointer, int Scope, int Semantics )
{
    __spirv_AtomicStore( Pointer, Scope, Semantics, ATOMIC_FLAG_FALSE );
}

void __attribute__((overloadable)) __spirv_AtomicFlagClear( __local int *Pointer, int Scope, int Semantics )
{
    __spirv_AtomicStore( Pointer, Scope, Semantics, ATOMIC_FLAG_FALSE );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void __attribute__((overloadable)) __spirv_AtomicFlagClear( __generic int *Pointer, int Scope, int Semantics )
{
    __spirv_AtomicStore( Pointer, Scope, Semantics, ATOMIC_FLAG_FALSE );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float __attribute__((overloadable)) __spirv_AtomicFAddEXT( __private float *Pointer, int Scope, int Semantics, float Value)
{
    float orig = *Pointer;
    *Pointer += Value;
    return orig;
}

float __attribute__((overloadable)) __spirv_AtomicFAddEXT( __global float *Pointer, int Scope, int Semantics, float Value)
{
    if(BIF_FLAG_CTRL_GET(UseNativeFP32GlobalAtomicAdd))
    {
        atomic_operation_1op_as_float( __builtin_IB_atomic_add_global_f32, float, Pointer, Scope, Semantics, Value, true );
    }
    // We don't use SPINLOCK_START and SPINLOCK_END emulation here, since do-while loop is more efficient for global atomics.
    float orig;
    float desired;
    do {
        orig = as_float(__spirv_AtomicLoad((__global int*)Pointer, Scope, Semantics));
        desired = orig + Value;
    } while(as_int(orig) != __spirv_AtomicCompareExchange(
                                (__global int*)Pointer, Scope, Semantics, Semantics,
                                as_int(desired), as_int(orig)));
    return orig;
}

float __attribute__((overloadable)) __spirv_AtomicFAddEXT( __local float *Pointer, int Scope, int Semantics, float Value)
{
    if(BIF_FLAG_CTRL_GET(UseNativeFP32LocalAtomicAdd))
    {
        atomic_operation_1op_as_float( __builtin_IB_atomic_add_local_f32, float, Pointer, Scope, Semantics, Value, false )
    }
    // We don't use SPINLOCK_START and SPINLOCK_END emulation here, since do-while loop is more efficient for global atomics.
    float orig;
    float desired;
    do {
        orig = as_float(__spirv_AtomicLoad((__local int*)Pointer, Scope, Semantics));
        desired = orig + Value;
    } while(as_int(orig) != __spirv_AtomicCompareExchange(
                                (__local int*)Pointer, Scope, Semantics, Semantics,
                                as_int(desired), as_int(orig)));
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
//The atomic emulation pattern is used later in AtomicOptPass.
//If you change the pattern, you need to make the appropriate changes to AtomicOptPass.
float __attribute__((overloadable)) __spirv_AtomicFAddEXT( __generic float *Pointer, int Scope, int Semantics, float Value)
{
    __builtin_assume((__local float*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFAddEXT((local float*)Pointer, Scope, Semantics, Value);
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFAddEXT((__private float*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicFAddEXT((global float*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double __attribute__((overloadable)) __spirv_AtomicFAddEXT( __private double *Pointer, int Scope, int Semantics, double Value)
{
    double orig = *Pointer;
    *Pointer += Value;
    return orig;
}

double __attribute__((overloadable)) __spirv_AtomicFAddEXT( __global double *Pointer, int Scope, int Semantics, double Value)
{
    // We don't use __builtin_IB_eu_thread_pause() for platforms which don't support it
    if (BIF_FLAG_CTRL_GET(UseNativeFP64GlobalAtomicAdd) || !BIF_FLAG_CTRL_GET(HasThreadPauseSupport))
    {
        atomic_operation_1op_as_double( __builtin_IB_atomic_add_global_f64, double, Pointer, Scope, Semantics, Value, true );
    }
    // We don't use SPINLOCK_START and SPINLOCK_END emulation here, since do-while loop is more efficient for global atomics.
    // Another important reason of using do-while loop emulation is to avoid HW Bug on XeHP SDV:
    // "NodeDSS works in fixed arbitration mode where writes are always prioritized over reads.
    //  This is causing the IC read request to stall behind other pending write requests.
    //  Since IC read is not progressing, the thread which acquired the lock is not proceeding
    //  further to clear the lock and thus causing hang."
    // do-while loop emulation doesn't expose the HW issue since it reads 'Pointer' value inside a loop.
    double orig;
    double desired;
    do {
        orig = as_double(__spirv_AtomicLoad((__global long*)Pointer, Scope, Semantics));
        desired = orig + Value;
    } while(as_long(orig) != __spirv_AtomicCompareExchange(
                                (__global long*)Pointer, Scope, Semantics, Semantics,
                                as_long(desired), as_long(orig)));
    return orig;
}

double __attribute__((overloadable)) __spirv_AtomicFAddEXT( __local double *Pointer, int Scope, int Semantics, double Value)
{
    if(BIF_FLAG_CTRL_GET(PlatformType) == IGFX_PVC)
    {
        double orig;
        double desired;
        do {
            orig = as_double(__spirv_AtomicLoad((__local long*)Pointer, Scope, Semantics));
            desired = orig + Value;
        } while(as_long(orig) != __spirv_AtomicCompareExchange(
                                (__local long*)Pointer, Scope, Semantics, Semantics,
                                as_long(desired), as_long(orig)));
        return orig;
    }
    else
    {
        double orig;
        FENCE_PRE_OP(Scope, Semantics, false)
        LOCAL_SPINLOCK_START()
        orig = *Pointer;
        *Pointer = orig + Value;
        LOCAL_SPINLOCK_END()
        FENCE_POST_OP(Scope, Semantics, false)
        return orig;
    }
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __attribute__((overloadable)) __spirv_AtomicFAddEXT( __generic double *Pointer, int Scope, int Semantics, double Value)
{
    __builtin_assume((__local double*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFAddEXT((local double*)Pointer, Scope, Semantics, Value);
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFAddEXT((__private double*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicFAddEXT((global double*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

half __attribute__((overloadable)) __spirv_AtomicFMinEXT( private half* Pointer, int Scope, int Semantics, half Value)
{
    half orig = *Pointer;
    *Pointer = (orig < Value) ? orig : Value;
    return orig;
}

half __attribute__((overloadable)) __spirv_AtomicFMinEXT( global half* Pointer, int Scope, int Semantics, half Value)
{
    if(BIF_FLAG_CTRL_GET(UseNativeFP16AtomicMinMax))
    {
        atomic_operation_1op_as_half( __builtin_IB_atomic_min_global_f16, half, Pointer, Scope, Semantics, Value, true );
    }
    half orig;
    FENCE_PRE_OP(Scope, Semantics, true)
    GLOBAL_SPINLOCK_START()
    orig = *Pointer;
    *Pointer = (orig < Value) ? orig : Value;
    GLOBAL_SPINLOCK_END()
    FENCE_POST_OP(Scope, Semantics, true)
    return orig;
}

half __attribute__((overloadable)) __spirv_AtomicFMinEXT( local half* Pointer, int Scope, int Semantics, half Value)
{
    if(BIF_FLAG_CTRL_GET(UseNativeFP16AtomicMinMax))
    {
        atomic_operation_1op_as_half( __builtin_IB_atomic_min_local_f16, half, Pointer, Scope, Semantics, Value, false );
    }
    half orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    LOCAL_SPINLOCK_START()
    orig = *Pointer;
    *Pointer = (orig < Value) ? orig : Value;
    LOCAL_SPINLOCK_END()
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __attribute__((overloadable)) __spirv_AtomicFMinEXT( generic half* Pointer, int Scope, int Semantics, half Value)
{
    __builtin_assume((__local half*)Pointer != 0);
    if (__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFMinEXT((__local half*)Pointer, Scope, Semantics, Value);
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFMinEXT((__private half*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicFMinEXT((__global half*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

half __attribute__((overloadable)) __spirv_AtomicFAddEXT( __private half *Pointer, int Scope, int Semantics, half Value)
{
    half orig = *Pointer;
    *Pointer += Value;
    return orig;
}

half __attribute__((overloadable)) __spirv_AtomicFAddEXT( global half* Pointer, int Scope, int Semantics, half Value)
{
    atomic_operation_1op_as_half( __builtin_IB_atomic_add_global_f16, half, Pointer, Scope, Semantics, Value, true )
}

half __attribute__((overloadable)) __spirv_AtomicFAddEXT( local half* Pointer, int Scope, int Semantics, half Value)
{
    atomic_operation_1op_as_half( __builtin_IB_atomic_add_local_f16, half, Pointer, Scope, Semantics, Value, false )
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __attribute__((overloadable)) __spirv_AtomicFAddEXT( __generic half *Pointer, int Scope, int Semantics, half Value)
{
    __builtin_assume((__local half*)Pointer != 0);
    if(__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFAddEXT((local half*)Pointer, Scope, Semantics, Value);
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFAddEXT((__private half*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicFAddEXT((global half*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float __attribute__((overloadable)) __spirv_AtomicFMinEXT( private float* Pointer, int Scope, int Semantics, float Value)
{
    float orig = *Pointer;
    *Pointer = (orig < Value) ? orig : Value;
    return orig;
}

float __attribute__((overloadable)) __spirv_AtomicFMinEXT( global float* Pointer, int Scope, int Semantics, float Value)
{
    atomic_operation_1op_as_float(__builtin_IB_atomic_min_global_f32, float, Pointer, Scope, Semantics, Value, true);
}

float __attribute__((overloadable)) __spirv_AtomicFMinEXT( local float* Pointer, int Scope, int Semantics, float Value)
{
    atomic_operation_1op_as_float(__builtin_IB_atomic_min_local_f32, float, Pointer, Scope, Semantics, Value, false);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __attribute__((overloadable)) __spirv_AtomicFMinEXT( generic float* Pointer, int Scope, int Semantics, float Value)
{
    __builtin_assume((__local float*)Pointer != 0);
    if (__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFMinEXT((__local float*)Pointer, Scope, Semantics, Value);
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFMinEXT((__private float*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicFMinEXT((__global float*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double __attribute__((overloadable)) __spirv_AtomicFMinEXT( private double* Pointer, int Scope, int Semantics, double Value)
{
    double orig = *Pointer;
    *Pointer = (orig < Value) ? orig : Value;
    return orig;
}

double __attribute__((overloadable)) __spirv_AtomicFMinEXT( global double* Pointer, int Scope, int Semantics, double Value)
{
    // We don't use SPINLOCK_START and SPINLOCK_END emulation here, since do-while loop is more efficient for global atomics.
    // Another important reason of using do-while loop emulation is to avoid HW Bug on XeHP SDV:
    // "NodeDSS works in fixed arbitration mode where writes are always prioritized over reads.
    //  This is causing the IC read request to stall behind other pending write requests.
    //  Since IC read is not progressing, the thread which acquired the lock is not proceeding
    //  further to clear the lock and thus causing hang."
    // do-while loop emulation doesn't expose the HW issue since it reads 'Pointer' value inside a loop.
    double orig;
    double desired;
    do {
        orig = as_double(__spirv_AtomicLoad((__global long*)Pointer, Scope, Semantics));
        desired = ( orig < Value ) ? orig : Value;
    } while(as_long(orig) != __spirv_AtomicCompareExchange(
                                (__global long*)Pointer, Scope, Semantics, Semantics,
                                as_long(desired), as_long(orig)));
    return orig;
}

double __attribute__((overloadable)) __spirv_AtomicFMinEXT( local double* Pointer, int Scope, int Semantics, double Value)
{
    double orig;
    double desired;
    do {
        orig = as_double(__spirv_AtomicLoad((__local long*)Pointer, Scope, Semantics));
        desired = ( orig < Value ) ? orig : Value;
    } while(as_long(orig) != __spirv_AtomicCompareExchange(
                                (__local long*)Pointer, Scope, Semantics, Semantics,
                                as_long(desired), as_long(orig)));
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __attribute__((overloadable)) __spirv_AtomicFMinEXT( generic double* Pointer, int Scope, int Semantics, double Value)
{
    __builtin_assume((__local double*)Pointer != 0);
    if (__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFMinEXT((__local double*)Pointer, Scope, Semantics, Value);
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFMinEXT((__private double*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicFMinEXT((__global double*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

half __attribute__((overloadable)) __spirv_AtomicFMaxEXT( private half* Pointer, int Scope, int Semantics, half Value)
{
    half orig = *Pointer;
    *Pointer = (orig > Value) ? orig : Value;
    return orig;
}

half __attribute__((overloadable)) __spirv_AtomicFMaxEXT( global half* Pointer, int Scope, int Semantics, half Value)
{
    if(BIF_FLAG_CTRL_GET(UseNativeFP16AtomicMinMax))
    {
        atomic_operation_1op_as_half( __builtin_IB_atomic_max_global_f16, half, Pointer, Scope, Semantics, Value, true );
    }
    half orig;
    FENCE_PRE_OP(Scope, Semantics, true)
    GLOBAL_SPINLOCK_START()
    orig = *Pointer;
    *Pointer = (orig > Value) ? orig : Value;
    GLOBAL_SPINLOCK_END()
    FENCE_POST_OP(Scope, Semantics, true)
    return orig;
}

half __attribute__((overloadable)) __spirv_AtomicFMaxEXT( local half* Pointer, int Scope, int Semantics, half Value)
{
    if(BIF_FLAG_CTRL_GET(UseNativeFP16AtomicMinMax))
    {
        atomic_operation_1op_as_half( __builtin_IB_atomic_max_local_f16, half, Pointer, Scope, Semantics, Value, false );
    }
    half orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    LOCAL_SPINLOCK_START()
    orig = *Pointer;
    *Pointer = (orig > Value) ? orig : Value;
    LOCAL_SPINLOCK_END()
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __attribute__((overloadable)) __spirv_AtomicFMaxEXT( generic half* Pointer, int Scope, int Semantics, half Value)
{
    __builtin_assume((__local half*)Pointer != 0);
    if (__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFMaxEXT((__local half*)Pointer, Scope, Semantics, Value);
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFMaxEXT((__private half*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicFMaxEXT((__global half*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float __attribute__((overloadable)) __spirv_AtomicFMaxEXT( private float* Pointer, int Scope, int Semantics, float Value)
{
    float orig = *Pointer;
    *Pointer = (orig > Value) ? orig : Value;
    return orig;
}

float __attribute__((overloadable)) __spirv_AtomicFMaxEXT( global float* Pointer, int Scope, int Semantics, float Value)
{
    atomic_operation_1op_as_float(__builtin_IB_atomic_max_global_f32, float, Pointer, Scope, Semantics, Value, true);
}

float __attribute__((overloadable)) __spirv_AtomicFMaxEXT( local float* Pointer, int Scope, int Semantics, float Value)
{
    atomic_operation_1op_as_float(__builtin_IB_atomic_max_local_f32, float, Pointer, Scope, Semantics, Value, false);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __attribute__((overloadable)) __spirv_AtomicFMaxEXT( generic float* Pointer, int Scope, int Semantics, float Value)
{
    __builtin_assume((__local float*)Pointer != 0);
    if (__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFMaxEXT((__local float*)Pointer, Scope, Semantics, Value);
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFMaxEXT((__private float*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicFMaxEXT((__global float*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double __attribute__((overloadable)) __spirv_AtomicFMaxEXT( private double* Pointer, int Scope, int Semantics, double Value)
{
    double orig = *Pointer;
    *Pointer = (orig > Value) ? orig : Value;
    return orig;
}

double __attribute__((overloadable)) __spirv_AtomicFMaxEXT( global double* Pointer, int Scope, int Semantics, double Value)
{
    // We don't use SPINLOCK_START and SPINLOCK_END emulation here, since do-while loop is more efficient for global atomics.
    // Another important reason of using do-while loop emulation is to avoid HW Bug on XeHP SDV:
    // "NodeDSS works in fixed arbitration mode where writes are always prioritized over reads.
    //  This is causing the IC read request to stall behind other pending write requests.
    //  Since IC read is not progressing, the thread which acquired the lock is not proceeding
    //  further to clear the lock and thus causing hang."
    // do-while loop emulation doesn't expose the HW issue since it reads 'Pointer' value inside a loop.
    double orig;
    double desired;
    do {
        orig = as_double(__spirv_AtomicLoad((__global long*)Pointer, Scope, Semantics));
        desired = ( orig > Value ) ? orig : Value;
    } while(as_long(orig) != __spirv_AtomicCompareExchange(
                                (__global long*)Pointer, Scope, Semantics, Semantics,
                                as_long(desired), as_long(orig)));
    return orig;
}

double __attribute__((overloadable)) __spirv_AtomicFMaxEXT( local double* Pointer, int Scope, int Semantics, double Value)
{
    double orig;
    double desired;
    do {
        orig = as_double(__spirv_AtomicLoad((__local long*)Pointer, Scope, Semantics));
        desired = ( orig > Value ) ? orig : Value;
    } while(as_long(orig) != __spirv_AtomicCompareExchange(
                                (__local long*)Pointer, Scope, Semantics, Semantics,
                                as_long(desired), as_long(orig)));
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __attribute__((overloadable)) __spirv_AtomicFMaxEXT( generic double* Pointer, int Scope, int Semantics, double Value)
{
    __builtin_assume((__local double*)Pointer != 0);
    if (__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFMaxEXT((__local double*)Pointer, Scope, Semantics, Value);
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFMaxEXT((__private double*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicFMaxEXT((__global double*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float __attribute__((overloadable)) __spirv_AtomicFAdd( __global float *Pointer, int Scope, int Semantics, float Value)
{
    atomic_operation_1op_as_float( __builtin_IB_atomic_add_global_f32, float, Pointer, Scope, Semantics, Value, true );
}
float __attribute__((overloadable)) __spirv_AtomicFSub( __global float *Pointer, int Scope, int Semantics, float Value)
{
    atomic_operation_1op_as_float( __builtin_IB_atomic_sub_global_f32, float, Pointer, Scope, Semantics, Value, true );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#undef ATOMIC_FLAG_FALSE
#undef ATOMIC_FLAG_TRUE

#define KMP_LOCK_FREE 0
#define KMP_LOCK_BUSY 1

void __builtin_IB_kmp_acquire_lock(int *lock)
{
  volatile atomic_uint *lck = (volatile atomic_uint *)lock;
  uint expected = KMP_LOCK_FREE;
  while (atomic_load_explicit(lck, memory_order_relaxed) != KMP_LOCK_FREE ||
      !atomic_compare_exchange_strong_explicit(lck, &expected, KMP_LOCK_BUSY,
                                               memory_order_acquire,
                                               memory_order_relaxed)) {
    expected = KMP_LOCK_FREE;
  }
}

void __builtin_IB_kmp_release_lock(int *lock)
{
  volatile atomic_uint *lck = (volatile atomic_uint *)lock;
  atomic_store_explicit(lck, KMP_LOCK_FREE, memory_order_release);
}

#undef KMP_LOCK_FREE
#undef KMP_LOCK_BUSY

#undef SEMANTICS_NEED_FENCE
#undef FENCE_PRE_OP
#undef FENCE_POST_OP
#undef SPINLOCK_START
#undef SPINLOCK_END

#undef atomic_operation_1op
#undef atomic_operation_0op
#undef atomic_cmpxhg
