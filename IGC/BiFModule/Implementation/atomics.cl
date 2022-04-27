/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Atomic Instructions

#include "../Headers/spirv.h"

#define ATOMIC_FLAG_TRUE 1
#define ATOMIC_FLAG_FALSE 0

#define SEMANTICS_PRE_OP_NEED_FENCE ( Release | AcquireRelease | SequentiallyConsistent)

#define SEMANTICS_POST_OP_NEEDS_FENCE ( Acquire | AcquireRelease | SequentiallyConsistent)

extern __constant int __UseNativeFP32GlobalAtomicAdd;
extern __constant int __UseNativeFP16AtomicMinMax;

extern __constant int __HasInt64SLMAtomicCAS;
extern __constant int __UseNativeFP64GlobalAtomicAdd;

  __local int* __builtin_IB_get_local_lock();
  __global int* __builtin_IB_get_global_lock();
  void __builtin_IB_eu_thread_pause(uint value);
  void __intel_memfence_handler(bool flushRW, bool isGlobal, bool invalidateL1);

#define LOCAL_SPINLOCK_START() \
  { \
  volatile bool done = false; \
  while(!done) { \
       __builtin_IB_eu_thread_pause(32); \
       if(SPIRV_BUILTIN(AtomicCompareExchange, _p3i32_i32_i32_i32_i32_i32, )(__builtin_IB_get_local_lock(), Device, Relaxed, Relaxed, 1, 0) == 0) {

#define LOCAL_SPINLOCK_END() \
            done = true; \
            SPIRV_BUILTIN(AtomicStore, _p3i32_i32_i32_i32, )(__builtin_IB_get_local_lock(), Device, SequentiallyConsistent | WorkgroupMemory, 0); \
  }}}

#define GLOBAL_SPINLOCK_START() \
  { \
  volatile bool done = false; \
  while(!done) { \
       __builtin_IB_eu_thread_pause(32); \
       if(SPIRV_BUILTIN(AtomicCompareExchange, _p1i32_i32_i32_i32_i32_i32, )(__builtin_IB_get_global_lock(), Device, Relaxed, Relaxed, 1, 0) == 0) {

#define GLOBAL_SPINLOCK_END() \
            done = true; \
            SPIRV_BUILTIN(AtomicStore, _p1i32_i32_i32_i32, )(__builtin_IB_get_global_lock(), Device, SequentiallyConsistent | CrossWorkgroupMemory, 0); \
  }}}

#define FENCE_PRE_OP(Scope, Semantics, isGlobal)                                      \
  if( ( (Semantics) & ( SEMANTICS_PRE_OP_NEED_FENCE ) ) > 0 )                         \
  {                                                                                   \
      bool flushL3 = (isGlobal) && ((Scope) == Device || (Scope) == CrossDevice);     \
      __intel_memfence_handler(flushL3, isGlobal, false);                             \
  }

#define FENCE_POST_OP(Scope, Semantics, isGlobal)                                     \
  if( ( (Semantics) & ( SEMANTICS_POST_OP_NEEDS_FENCE ) ) > 0 )                       \
  {                                                                                   \
      bool flushL3 = (isGlobal) && ((Scope) == Device || (Scope) == CrossDevice);     \
      __intel_memfence_handler(flushL3, isGlobal, isGlobal);                          \
  }

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

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p0i32_i32_i32, )( __private int *Pointer, int Scope, int Semantics )
{
    return *Pointer;
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p1i32_i32_i32, )( __global int *Pointer, int Scope, int Semantics )
{
    return SPIRV_BUILTIN(AtomicOr, _p1i32_i32_i32_i32, )( Pointer, Scope, Semantics, 0 );
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p3i32_i32_i32, )( __local int *Pointer, int Scope, int Semantics )
{
    return SPIRV_BUILTIN(AtomicOr, _p3i32_i32_i32_i32, )( Pointer, Scope, Semantics, 0 );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p4i32_i32_i32, )( __generic int *Pointer, int Scope, int Semantics )
{
    return SPIRV_BUILTIN(AtomicOr, _p4i32_i32_i32_i32, )( Pointer, Scope, Semantics, 0 );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p0i64_i32_i32, )( __private long *Pointer, int Scope, int Semantics )
{
    return *Pointer;
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p1i64_i32_i32, )( __global long *Pointer, int Scope, int Semantics )
{
    return SPIRV_BUILTIN(AtomicOr, _p1i64_i32_i32_i64, )( Pointer, Scope, Semantics, 0 );
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p3i64_i32_i32, )( __local long *Pointer, int Scope, int Semantics )
{
    return SPIRV_BUILTIN(AtomicOr, _p3i64_i32_i32_i64, )( Pointer, Scope, Semantics, 0 );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p4i64_i32_i32, )( __generic long *Pointer, int Scope, int Semantics )
{
    return SPIRV_BUILTIN(AtomicOr, _p4i64_i32_i32_i64, )( Pointer, Scope, Semantics, 0 );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)


float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p0f32_i32_i32, )( __private float *Pointer, int Scope, int Semantics )
{
    return *Pointer;
}


float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p1f32_i32_i32, )( __global float *Pointer, int Scope, int Semantics )
{
    return as_float( SPIRV_BUILTIN(AtomicOr, _p1i32_i32_i32_i32, )( (__global int*)Pointer, Scope, Semantics, 0 ) );
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p3f32_i32_i32, )( __local float *Pointer, int Scope, int Semantics )
{
    return as_float( SPIRV_BUILTIN(AtomicOr, _p3i32_i32_i32_i32, )( (__local int*)Pointer, Scope, Semantics, 0 ) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p4f32_i32_i32, )( __generic float *Pointer, int Scope, int Semantics )
{
    return as_float( SPIRV_BUILTIN(AtomicOr, _p4i32_i32_i32_i32, )( (volatile __generic int*)Pointer, Scope, Semantics, 0 ) );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp64)
#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p0f64_i32_i32, )( __private double *Pointer, int Scope, int Semantics )
{
    return *Pointer;
}


double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p1f64_i32_i32, )( __global double *Pointer, int Scope, int Semantics )
{
    return as_double( SPIRV_BUILTIN(AtomicOr, _p1i64_i32_i32_i64, )( (__global long*)Pointer, Scope, Semantics, 0 ) );
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p3f64_i32_i32, )( __local double *Pointer, int Scope, int Semantics )
{
    return as_double( SPIRV_BUILTIN(AtomicOr, _p3i64_i32_i32_i64, )( (__local long*)Pointer, Scope, Semantics, 0 ) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p4f64_i32_i32, )( __generic double *Pointer, int Scope, int Semantics )
{
    return as_double( SPIRV_BUILTIN(AtomicOr, _p4i64_i32_i32_i64, )( (__generic long*)Pointer, Scope, Semantics, 0 ) );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p0f16_i32_i32, )(__private half* Pointer, int Scope, int Semantics)
{
    return *Pointer;
}

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p1f16_i32_i32, )(__global half* Pointer, int Scope, int Semantics)
{
    atomic_operation_1op_as_half(__builtin_IB_atomic_or_global_i16, half, (__global short*)Pointer, Scope, Semantics, 0, true);
}

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p3f16_i32_i32, )(__local half* Pointer, int Scope, int Semantics)
{
    atomic_operation_1op_as_half(__builtin_IB_atomic_or_local_i16, half, (__local short*)Pointer, Scope, Semantics, 0, false);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p4f16_i32_i32, )(__generic half* Pointer, int Scope, int Semantics)
{
    if (SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicLoad, _p3f16_i32_i32, )((__local half*)Pointer, Scope, Semantics);
    }
    else
    {
        return SPIRV_BUILTIN(AtomicLoad, _p1f16_i32_i32, )((__global half*)Pointer, Scope, Semantics);
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_fp16)

// Atomic Stores


void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p0i32_i32_i32_i32, )( __private int *Pointer, int Scope, int Semantics, int Value )
{
    *Pointer = Value;
}


void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p1i32_i32_i32_i32, )( __global int *Pointer, int Scope, int Semantics, int Value )
{
    SPIRV_BUILTIN(AtomicExchange, _p1i32_i32_i32_i32, )( Pointer, Scope, Semantics, Value );
}


void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p3i32_i32_i32_i32, )( __local int *Pointer, int Scope, int Semantics, int Value )
{
    SPIRV_BUILTIN(AtomicExchange, _p3i32_i32_i32_i32, )( Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p4i32_i32_i32_i32, )( __generic int *Pointer, int Scope, int Semantics, int Value )
{
    SPIRV_BUILTIN(AtomicExchange, _p4i32_i32_i32_i32, )( Pointer, Scope, Semantics, Value );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p0i64_i32_i32_i64, )( __private long *Pointer, int Scope, int Semantics, long Value )
{
    *Pointer = Value;
}


void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p1i64_i32_i32_i64, )( __global long *Pointer, int Scope, int Semantics, long Value )
{
    SPIRV_BUILTIN(AtomicExchange, _p1i64_i32_i32_i64, )( Pointer, Scope, Semantics, Value );
}


void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p3i64_i32_i32_i64, )( __local long *Pointer, int Scope, int Semantics, long Value )
{
    SPIRV_BUILTIN(AtomicExchange, _p3i64_i32_i32_i64, )( Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p4i64_i32_i32_i64, )( __generic long *Pointer, int Scope, int Semantics, long Value )
{
    SPIRV_BUILTIN(AtomicExchange, _p4i64_i32_i32_i64, )( Pointer, Scope, Semantics, Value );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)


void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p0f32_i32_i32_f32, )( __private float *Pointer, int Scope, int Semantics, float Value )
{
    SPIRV_BUILTIN(AtomicExchange, _p0f32_i32_i32_f32, )( Pointer, Scope, Semantics, Value );
}


void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p1f32_i32_i32_f32, )( __global float *Pointer, int Scope, int Semantics, float Value )
{
    SPIRV_BUILTIN(AtomicExchange, _p1f32_i32_i32_f32, )( Pointer, Scope, Semantics, Value );
}


void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p3f32_i32_i32_f32, )( __local float *Pointer, int Scope, int Semantics, float Value )
{
    SPIRV_BUILTIN(AtomicExchange, _p3f32_i32_i32_f32, )( Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p4f32_i32_i32_f32, )( __generic float *Pointer, int Scope, int Semantics, float Value )
{
    SPIRV_BUILTIN(AtomicExchange, _p4f32_i32_i32_f32, )( Pointer, Scope, Semantics, Value );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp64)
#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p0f64_i32_i32_f64, )( __private double *Pointer, int Scope, int Semantics, double Value )
{
    SPIRV_BUILTIN(AtomicExchange, _p0f64_i32_i32_f64, )( Pointer, Scope, Semantics, Value );
}


void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p1f64_i32_i32_f64, )( __global double *Pointer, int Scope, int Semantics, double Value )
{
    SPIRV_BUILTIN(AtomicExchange, _p1f64_i32_i32_f64, )( Pointer, Scope, Semantics, Value );
}


void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p3f64_i32_i32_f64, )( __local double *Pointer, int Scope, int Semantics, double Value )
{
    SPIRV_BUILTIN(AtomicExchange, _p3f64_i32_i32_f64, )( Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p4f64_i32_i32_f64, )( __generic double *Pointer, int Scope, int Semantics, double Value )
{
    SPIRV_BUILTIN(AtomicExchange, _p4f64_i32_i32_f64, )( Pointer, Scope, Semantics, Value );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p0f16_i32_i32_f16, )(__private half* Pointer, int Scope, int Semantics, half Value)
{
    SPIRV_BUILTIN(AtomicExchange, _p0f16_i32_i32_f16, )(Pointer, Scope, Semantics, Value);
}

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p1f16_i32_i32_f16, )(__global half* Pointer, int Scope, int Semantics, half Value)
{
    SPIRV_BUILTIN(AtomicExchange, _p1f16_i32_i32_f16, )(Pointer, Scope, Semantics, Value);
}

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p3f16_i32_i32_f16, )(__local half* Pointer, int Scope, int Semantics, half Value)
{
    SPIRV_BUILTIN(AtomicExchange, _p3f16_i32_i32_f16, )(Pointer, Scope, Semantics, Value);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p4f16_i32_i32_f16, )(__generic half* Pointer, int Scope, int Semantics, half Value)
{
    SPIRV_BUILTIN(AtomicExchange, _p4f16_i32_i32_f16, )(Pointer, Scope, Semantics, Value);
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_fp16)

// Atomic Exchange


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p0i32_i32_i32_i32, )( __private int *Pointer, int Scope, int Semantics, int Value )
{
    uint orig = *Pointer;
    *Pointer = Value;
    return orig;
}


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p1i32_i32_i32_i32, )( __global int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xchg_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p3i32_i32_i32_i32, )( __local int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xchg_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p4i32_i32_i32_i32, )( __generic int *Pointer, int Scope, int Semantics, int Value )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_xchg_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value, false );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_xchg_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
    }

}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p0i64_i32_i32_i64, )( __private long *Pointer, int Scope, int Semantics, long Value )
{
    ulong orig = *Pointer;
    *Pointer = Value;
    return orig;
}


long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p1i64_i32_i32_i64, )( __global long *Pointer, int Scope, int Semantics, long Value )
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
    if (__HasInt64SLMAtomicCAS)
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
    if (__HasInt64SLMAtomicCAS)
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
    if (__HasInt64SLMAtomicCAS)
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

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p3i64_i32_i32_i64, )( __local long *Pointer, int Scope, int Semantics, long Value )
{
    return __intel_atomic_binary(ATOMIC_XCHG64, Pointer, Scope, Semantics, Value);
}


#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p4i64_i32_i32_i64, )( __generic long *Pointer, int Scope, int Semantics, long Value )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicExchange, _p3i64_i32_i32_i64, )((__local long*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return SPIRV_BUILTIN(AtomicExchange, _p1i64_i32_i32_i64, )((__global long*)Pointer, Scope, Semantics, Value);
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics)

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p0f32_i32_i32_f32, )( __private float *Pointer, int Scope, int Semantics, float Value)
{
    float orig = *Pointer;

    *Pointer = Value;

    return orig;
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p1f32_i32_i32_f32, )( __global float *Pointer, int Scope, int Semantics, float Value)
{
    atomic_operation_1op_as_float( __builtin_IB_atomic_xchg_global_i32, float, (global int*)Pointer, Scope, Semantics, as_int(Value), true );
}


float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p3f32_i32_i32_f32, )( __local float *Pointer, int Scope, int Semantics, float Value)
{
    atomic_operation_1op_as_float( __builtin_IB_atomic_xchg_local_i32, float, (local int*)Pointer, Scope, Semantics, as_int(Value), false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p4f32_i32_i32_f32, )( __generic float *Pointer, int Scope, int Semantics, float Value)
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op_as_float( __builtin_IB_atomic_xchg_local_i32, float, (local int*)Pointer, Scope, Semantics, as_int(Value), false );
    }
    else
    {
        atomic_operation_1op_as_float( __builtin_IB_atomic_xchg_global_i32, float, (global int*)Pointer, Scope, Semantics, as_int(Value), true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp64)
#if defined(cl_khr_int64_base_atomics)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p0f64_i32_i32_f64, )( __private double *Pointer, int Scope, int Semantics, double Value)
{
    return as_double(SPIRV_BUILTIN(AtomicExchange, _p0i64_i32_i32_i64, )((__private long*) Pointer, Scope, Semantics, as_long(Value)));
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p1f64_i32_i32_f64, )( __global double *Pointer, int Scope, int Semantics, double Value)
{
    return as_double(SPIRV_BUILTIN(AtomicExchange, _p1i64_i32_i32_i64, )((__global long*) Pointer, Scope, Semantics, as_long(Value)));
}


double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p3f64_i32_i32_f64, )( __local double *Pointer, int Scope, int Semantics, double Value)
{
    return as_double(SPIRV_BUILTIN(AtomicExchange, _p3i64_i32_i32_i64, )((__local long*) Pointer, Scope, Semantics, as_long(Value)));
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p4f64_i32_i32_f64, )( __generic double *Pointer, int Scope, int Semantics, double Value)
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicExchange, _p3f64_i32_i32_f64, )((__local double*) Pointer, Scope, Semantics, Value);
    }
    else
    {
        return SPIRV_BUILTIN(AtomicExchange, _p1f64_i32_i32_f64, )((__global double*) Pointer, Scope, Semantics, Value);
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics)
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p0f16_i32_i32_f16, )(__private half* Pointer, int Scope, int Semantics, half Value)
{
    half orig = *Pointer;

    *Pointer = Value;

    return orig;
}

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p1f16_i32_i32_f16, )(__global half* Pointer, int Scope, int Semantics, half Value)
{
    atomic_operation_1op_as_half(__builtin_IB_atomic_xchg_global_i16, half, (__global short*)Pointer, Scope, Semantics, as_short(Value), true);
}

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p3f16_i32_i32_f16, )(__local half* Pointer, int Scope, int Semantics, half Value)
{
    atomic_operation_1op_as_half(__builtin_IB_atomic_xchg_local_i16, half, (__local short*)Pointer, Scope, Semantics, as_short(Value), false);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p4f16_i32_i32_f16, )(__generic half* Pointer, int Scope, int Semantics, half Value)
{
    if (SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicExchange, _p3f16_i32_i32_f16, )((__local half*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return SPIRV_BUILTIN(AtomicExchange, _p1f16_i32_i32_f16, )((__global half*)Pointer, Scope, Semantics, Value);
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_fp16)

// Atomic Compare Exchange


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p0i32_i32_i32_i32_i32_i32, )( __private int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator)
{
    uint orig = *Pointer;
    if( orig == Comparator )
    {
        *Pointer = Value;
    }
    return orig;
}


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p1i32_i32_i32_i32_i32_i32, )( __global int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator)
{
    atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_global_i32, uint, (global int*)Pointer, Scope, Equal, Value, Comparator, true );
}


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p3i32_i32_i32_i32_i32_i32, )( __local int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator)
{
    atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_local_i32, uint, (local int*)Pointer, Scope, Equal, Value, Comparator, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p4i32_i32_i32_i32_i32_i32, )( __generic int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator)
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_local_i32, uint, (__local int*)Pointer, Scope, Equal, Value, Comparator, false );
    }
    else
    {
        atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_global_i32, uint, (__global int*)Pointer, Scope, Equal, Value, Comparator, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p0i64_i32_i32_i32_i64_i64, )( __private long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator)
{
    ulong orig = *Pointer;
    if( orig == Comparator )
    {
        *Pointer = Value;
    }
    return orig;
}


long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p1i64_i32_i32_i32_i64_i64, )( __global long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator)
{
    atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_global_i64, ulong, (global long*)Pointer, Scope, Equal, Value, Comparator, true );
}


long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p3i64_i32_i32_i32_i64_i64, )( __local long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator)
{
    if (__HasInt64SLMAtomicCAS)
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

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p4i64_i32_i32_i32_i64_i64, )( __generic long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator)
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicCompareExchange, _p3i64_i32_i32_i32_i64_i64, )( (__local long*)Pointer, Scope, Equal, Unequal, Value, Comparator );
    }
    else
    {
        return SPIRV_BUILTIN(AtomicCompareExchange, _p1i64_i32_i32_i32_i64_i64, )( (__global long*)Pointer, Scope, Equal, Unequal, Value, Comparator );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics)

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p0f32_i32_i32_i32_f32_f32, )( __private float *Pointer, int Scope, int Equal, int Unequal, float Value, float Comparator)
{
    float orig = *Pointer;

    if( orig == Comparator )
    {
        *Pointer = Value;
    }

    return orig;
}

// Float compare-and-exchange builtins are handled as integer builtins, because OpenCL C specification says that the float atomics are
// doing bitwise comparisons, not float comparisons

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p1f32_i32_i32_i32_f32_f32, )( __global float *Pointer, int Scope, int Equal, int Unequal, float Value, float Comparator)
{
    atomic_cmpxhg_as_float( __builtin_IB_atomic_cmpxchg_global_i32, float, (global int*)Pointer, Scope, Equal, as_uint(Value), as_uint(Comparator), true );
}


float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p3f32_i32_i32_i32_f32_f32, )( __local float *Pointer, int Scope, int Equal, int Unequal, float Value, float Comparator)
{
    atomic_cmpxhg_as_float( __builtin_IB_atomic_cmpxchg_local_i32, float, (local int*)Pointer, Scope, Equal, as_uint(Value), as_uint(Comparator), false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p4f32_i32_i32_i32_f32_f32, )( __generic float *Pointer, int Scope, int Equal, int Unequal, float Value, float Comparator)
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_cmpxhg_as_float( __builtin_IB_atomic_cmpxchg_local_i32, float, (__local int*)Pointer, Scope, Equal, as_uint(Value), as_uint(Comparator), false );
    }
    else
    {
        atomic_cmpxhg_as_float( __builtin_IB_atomic_cmpxchg_global_i32, float, (__global int*)Pointer, Scope, Equal, as_uint(Value), as_uint(Comparator), true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p0i32_i32_i32_i32_i32_i32, )( __private int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator)
{
    return SPIRV_BUILTIN(AtomicCompareExchange, _p0i32_i32_i32_i32_i32_i32, )( Pointer, Scope, Equal, Unequal, Value, Comparator );
}


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p1i32_i32_i32_i32_i32_i32, )( __global int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator)
{
    return SPIRV_BUILTIN(AtomicCompareExchange, _p1i32_i32_i32_i32_i32_i32, )( Pointer, Scope, Equal, Unequal, Value, Comparator );
}


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p3i32_i32_i32_i32_i32_i32, )( __local int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator)
{
    return SPIRV_BUILTIN(AtomicCompareExchange, _p3i32_i32_i32_i32_i32_i32, )( Pointer, Scope, Equal, Unequal, Value, Comparator );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p4i32_i32_i32_i32_i32_i32, )( __generic int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator)
{
    return SPIRV_BUILTIN(AtomicCompareExchange, _p4i32_i32_i32_i32_i32_i32, )( Pointer, Scope, Equal, Unequal, Value, Comparator );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p0i64_i32_i32_i32_i64_i64, )( __private long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator)
{
    return SPIRV_BUILTIN(AtomicCompareExchange, _p0i64_i32_i32_i32_i64_i64, )( Pointer, Scope, Equal, Unequal, Value, Comparator );
}


long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p1i64_i32_i32_i32_i64_i64, )( __global long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator)
{
    return SPIRV_BUILTIN(AtomicCompareExchange, _p1i64_i32_i32_i32_i64_i64, )( Pointer, Scope, Equal, Unequal, Value, Comparator );
}


long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p3i64_i32_i32_i32_i64_i64, )( __local long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator)
{
    return SPIRV_BUILTIN(AtomicCompareExchange, _p3i64_i32_i32_i32_i64_i64, )( Pointer, Scope, Equal, Unequal, Value, Comparator );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p4i64_i32_i32_i32_i64_i64, )( __generic long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator)
{
    return SPIRV_BUILTIN(AtomicCompareExchange, _p4i64_i32_i32_i32_i64_i64, )( Pointer, Scope, Equal, Unequal, Value, Comparator );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_int64_base_atomics)

// Atomic Increment


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p0i32_i32_i32, )( __private int *Pointer, int Scope, int Semantics )
{
    uint orig = *Pointer;
    *Pointer += 1;
    return orig;
}


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p1i32_i32_i32, )( __global int *Pointer, int Scope, int Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_inc_global_i32, uint, (global int*)Pointer, Scope, Semantics, true );
}


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p3i32_i32_i32, )( __local int *Pointer, int Scope, int Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_inc_local_i32, uint, (local int*)Pointer, Scope, Semantics, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p4i32_i32_i32, )( __generic int *Pointer, int Scope, int Semantics )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_0op( __builtin_IB_atomic_inc_local_i32, uint, (__local int*)Pointer, Scope, Semantics, false );
    }
    else
    {
        atomic_operation_0op( __builtin_IB_atomic_inc_global_i32, uint, (__global int*)Pointer, Scope, Semantics, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p0i64_i32_i32, )( __private long *Pointer, int Scope, int Semantics )
{
    ulong orig = *Pointer;
    *Pointer += 1;
    return orig;
}


long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p1i64_i32_i32, )( __global long *Pointer, int Scope, int Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_inc_global_i64, ulong, (global int*)Pointer, Scope, Semantics, true );
}


long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p3i64_i32_i32, )( __local long *Pointer, int Scope, int Semantics )
{
    return __intel_atomic_unary(true, Pointer, Scope, Semantics);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p4i64_i32_i32, )( __generic long *Pointer, int Scope, int Semantics )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicIIncrement, _p3i64_i32_i32, )((__local long*)Pointer, Scope, Semantics );
    }
    else
    {
        return SPIRV_BUILTIN(AtomicIIncrement, _p1i64_i32_i32, )((__global long*)Pointer, Scope, Semantics );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_int64_base_atomics)

// Atomic Decrement


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p0i32_i32_i32, )( __private int *Pointer, int Scope, int Semantics )
{
    uint orig = *Pointer;

    *Pointer -= 1;

    return orig;
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p1i32_i32_i32, )( __global int *Pointer, int Scope, int Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_dec_global_i32, uint, (global int*)Pointer, Scope, Semantics, true );
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p3i32_i32_i32, )( __local int *Pointer, int Scope, int Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_dec_local_i32, uint, (local int*)Pointer, Scope, Semantics, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p4i32_i32_i32, )( __generic int *Pointer, int Scope, int Semantics )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_0op( __builtin_IB_atomic_dec_local_i32, uint, (__local int*)Pointer, Scope, Semantics, false );
    }
    else
    {
        atomic_operation_0op( __builtin_IB_atomic_dec_global_i32, uint, (__global int*)Pointer, Scope, Semantics, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p0i64_i32_i32, )( __private long *Pointer, int Scope, int Semantics )
{
    ulong orig = *Pointer;
    *Pointer -= 1;
    return orig;
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p1i64_i32_i32, )( __global long *Pointer, int Scope, int Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_dec_global_i64, ulong, (global long*)Pointer, Scope, Semantics, true );
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p3i64_i32_i32, )( __local long *Pointer, int Scope, int Semantics )
{
    return __intel_atomic_unary(false, Pointer, Scope, Semantics);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p4i64_i32_i32, )( __generic long *Pointer, int Scope, int Semantics )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicIDecrement, _p3i64_i32_i32, )( (__local long*)Pointer, Scope, Semantics );
    }
    else
    {
        return SPIRV_BUILTIN(AtomicIDecrement, _p1i64_i32_i32, )( (__global long*)Pointer, Scope, Semantics );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_int64_base_atomics)


// Atomic IAdd


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p0i32_i32_i32_i32, )( __private int *Pointer, int Scope, int Semantics, int Value )
{
    uint orig = *Pointer;

    *Pointer += Value;

    return orig;
}


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p1i32_i32_i32_i32, )( __global int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_add_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p3i32_i32_i32_i32, )( __local int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_add_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p4i32_i32_i32_i32, )( __generic int *Pointer, int Scope, int Semantics, int Value )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_add_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value, false );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_add_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
    }
}

#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p0i64_i32_i32_i64, )( __private long *Pointer, int Scope, int Semantics, long Value )
{
    ulong orig = *Pointer;
    *Pointer += Value;
    return orig;
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p1i64_i32_i32_i64, )( __global long *Pointer, int Scope, int Semantics, long Value )
{
    atomic_operation_1op( __builtin_IB_atomic_add_global_i64, ulong, (__global ulong*)Pointer, Scope, Semantics, Value, true );
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p3i64_i32_i32_i64, )( __local long *Pointer, int Scope, int Semantics, long Value )
{
    return __intel_atomic_binary(ATOMIC_IADD64, Pointer, Scope, Semantics, Value);
}


long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p4i64_i32_i32_i64, )( __generic long *Pointer, int Scope, int Semantics, long Value )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicIAdd, _p3i64_i32_i32_i64, )((__local long*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return SPIRV_BUILTIN(AtomicIAdd, _p1i64_i32_i32_i64, )((__global long*)Pointer, Scope, Semantics, Value);
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_int64_base_atomics)

// Atomic ISub

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p0i32_i32_i32_i32, )( __private int *Pointer, int Scope, int Semantics, int Value )
{
    uint orig = *Pointer;

    *Pointer -= Value;

    return orig;
}


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p1i32_i32_i32_i32, )( __global int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_sub_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p3i32_i32_i32_i32, )( __local int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_sub_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p4i32_i32_i32_i32, )( __generic int *Pointer, int Scope, int Semantics, int Value )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_sub_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value, false );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_sub_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p0i64_i32_i32_i64, )( __private long *Pointer, int Scope, int Semantics, long Value )
{
    ulong orig = *Pointer;
    *Pointer -= Value;
    return orig;
}


long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p1i64_i32_i32_i64, )( __global long *Pointer, int Scope, int Semantics, long Value )
{
    atomic_operation_1op( __builtin_IB_atomic_sub_global_i64, ulong, (global long*)Pointer, Scope, Semantics, Value, true );
}


long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p3i64_i32_i32_i64, )( __local long *Pointer, int Scope, int Semantics, long Value )
{
    return __intel_atomic_binary(ATOMIC_SUB64, Pointer, Scope, Semantics, Value);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p4i64_i32_i32_i64, )( __generic long *Pointer, int Scope, int Semantics, long Value )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicISub, _p3i64_i32_i32_i64, )((__local long*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        return SPIRV_BUILTIN(AtomicISub, _p1i64_i32_i32_i64, )((__global long*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics)


// Atomic SMin


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p0i32_i32_i32_i32, )( __private int *Pointer, int Scope, int Semantics, int Value)
{
    int orig = *Pointer;
    *Pointer = ( orig < Value ) ? orig : Value;
    return orig;
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p1i32_i32_i32_i32, )( __global int *Pointer, int Scope, int Semantics, int Value)
{
    atomic_operation_1op( __builtin_IB_atomic_min_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p3i32_i32_i32_i32, )( __local int *Pointer, int Scope, int Semantics, int Value)
{
    atomic_operation_1op( __builtin_IB_atomic_min_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p4i32_i32_i32_i32, )( __generic int *Pointer, int Scope, int Semantics, int Value)
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_min_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value, false );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_min_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_extended_atomics)

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p0i64_i32_i32_i64, )( __private long *Pointer, int Scope, int Semantics, long Value)
{
    long orig = *Pointer;
    *Pointer = ( orig < Value ) ? orig : Value;
    return orig;
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p1i64_i32_i32_i64, )( __global long *Pointer, int Scope, int Semantics, long Value)
{
    atomic_operation_1op( __builtin_IB_atomic_min_global_i64, ulong, (__global long*)Pointer, Scope, Semantics, Value, true );
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p3i64_i32_i32_i64, )( __local long *Pointer, int Scope, int Semantics, long Value)
{
    return __intel_atomic_binary(ATOMIC_IMIN64, (volatile __local long *)Pointer, Scope, Semantics, Value);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p4i64_i32_i32_i64, )( __generic long *Pointer, int Scope, int Semantics, long Value)
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicSMin, _p3i64_i32_i32_i64, )((__local int*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        return SPIRV_BUILTIN(AtomicSMin, _p1i64_i32_i32_i64, )((__global int*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p0i32_i32_i32_i32, )( __private uint *Pointer, int Scope, int Semantics, uint Value )
{
    uint orig = *Pointer;

    *Pointer = ( orig < Value ) ? orig : Value;

    return orig;
}

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p1i32_i32_i32_i32, )( __global uint *Pointer, int Scope, int Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_min_global_u32, uint, Pointer, Scope, Semantics, Value, true );
}

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p3i32_i32_i32_i32, )( __local uint *Pointer, int Scope, int Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_min_local_u32, uint, Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p4i32_i32_i32_i32, )( __generic uint *Pointer, int Scope, int Semantics, uint Value )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_min_local_u32, uint, (__local uint*)Pointer, Scope, Semantics, Value, false );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_min_global_u32, uint, (__global uint*)Pointer, Scope, Semantics, Value, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_extended_atomics)

ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p0i64_i32_i32_i64, )( __private ulong *Pointer, int Scope, int Semantics, ulong Value )
{
    ulong orig = *Pointer;
    *Pointer = ( orig < Value ) ? orig : Value;
    return orig;
}

ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p1i64_i32_i32_i64, )( __global ulong *Pointer, int Scope, int Semantics, ulong Value )
{
    atomic_operation_1op( __builtin_IB_atomic_min_global_u64, ulong, Pointer, Scope, Semantics, Value, true );
}

ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p3i64_i32_i32_i64, )( __local ulong *Pointer, int Scope, int Semantics, ulong Value )
{
    return __intel_atomic_binary(ATOMIC_UMIN64, Pointer, Scope, Semantics, Value);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p4i64_i32_i32_i64, )( __generic ulong *Pointer, int Scope, int Semantics, ulong Value )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicUMin, _p3i64_i32_i32_i64, )( (__local ulong*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        return SPIRV_BUILTIN(AtomicUMin, _p1i64_i32_i32_i64, )( (__global ulong*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

// Atomic SMax


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p0i32_i32_i32_i32, )( __private int *Pointer, int Scope, int Semantics, int Value)
{
    int orig = *Pointer;
    *Pointer = ( orig > Value ) ? orig : Value;
    return orig;
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p1i32_i32_i32_i32, )( __global int *Pointer, int Scope, int Semantics, int Value)
{
    atomic_operation_1op( __builtin_IB_atomic_max_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p3i32_i32_i32_i32, )( __local int *Pointer, int Scope, int Semantics, int Value)
{
    atomic_operation_1op( __builtin_IB_atomic_max_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p4i32_i32_i32_i32, )( __generic int *Pointer, int Scope, int Semantics, int Value)
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_max_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value, false );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_max_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_extended_atomics)

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p0i64_i32_i32_i64, )( __private long *Pointer, int Scope, int Semantics, long Value)
{
    long orig = *Pointer;
    *Pointer = ( orig > Value ) ? orig : Value;
    return orig;
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p1i64_i32_i32_i64, )( __global long *Pointer, int Scope, int Semantics, long Value)
{
    atomic_operation_1op( __builtin_IB_atomic_max_global_i64, ulong, (global long*)Pointer, Scope, Semantics, Value, true );
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p3i64_i32_i32_i64, )( __local long *Pointer, int Scope, int Semantics, long Value)
{
    return __intel_atomic_binary(ATOMIC_IMAX64, (volatile __local long *)Pointer, Scope, Semantics, Value);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p4i64_i32_i32_i64, )( __generic long *Pointer, int Scope, int Semantics, long Value)
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicSMax, _p3i64_i32_i32_i64, )( (__local long*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        return SPIRV_BUILTIN(AtomicSMax, _p1i64_i32_i32_i64, )( (__global long*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

// Atomic UMax


uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p0i32_i32_i32_i32, )( __private uint *Pointer, int Scope, int Semantics, uint Value )
{
    uint orig = *Pointer;

    *Pointer = ( orig > Value ) ? orig : Value;

    return orig;
}

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p1i32_i32_i32_i32, )( __global uint *Pointer, int Scope, int Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_max_global_u32, uint, Pointer, Scope, Semantics, Value, true );
}

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p3i32_i32_i32_i32, )( __local uint *Pointer, int Scope, int Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_max_local_u32, uint, Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p4i32_i32_i32_i32, )( __generic uint *Pointer, int Scope, int Semantics, uint Value )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_max_local_u32, uint, (__local uint*)Pointer, Scope, Semantics, Value, false );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_max_global_u32, uint, (__global uint*)Pointer, Scope, Semantics, Value, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_extended_atomics)

ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p0i64_i32_i32_i64, )( __private ulong *Pointer, int Scope, int Semantics, ulong Value )
{
    ulong orig = *Pointer;
    *Pointer = ( orig > Value ) ? orig : Value;
    return orig;
}

ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p1i64_i32_i32_i64, )( __global ulong *Pointer, int Scope, int Semantics, ulong Value )
{
    atomic_operation_1op( __builtin_IB_atomic_max_global_u64, ulong, Pointer, Scope, Semantics, Value, true );
}

ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p3i64_i32_i32_i64, )( __local ulong *Pointer, int Scope, int Semantics, ulong Value )
{
    return __intel_atomic_binary(ATOMIC_UMAX64, Pointer, Scope, Semantics, Value);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p4i64_i32_i32_i64, )( __generic ulong *Pointer, int Scope, int Semantics, ulong Value )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicUMax, _p3i64_i32_i32_i64, )( (__local ulong*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        return SPIRV_BUILTIN(AtomicUMax, _p1i64_i32_i32_i64, )( (__global ulong*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

// Atomic And


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p0i32_i32_i32_i32, )( __private int *Pointer, int Scope, int Semantics, int Value )
{
    uint orig = *Pointer;
    *Pointer &= Value;
    return orig;
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p1i32_i32_i32_i32, )( __global int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_and_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p3i32_i32_i32_i32, )( __local int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_and_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p4i32_i32_i32_i32, )( __generic int *Pointer, int Scope, int Semantics, int Value )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_and_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value, false );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_and_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_extended_atomics)

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p0i64_i32_i32_i64, )( __private long *Pointer, int Scope, int Semantics, long Value )
{
    ulong orig = *Pointer;
    *Pointer &= Value;
    return orig;
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p1i64_i32_i32_i64, )( __global long *Pointer, int Scope, int Semantics, long Value )
{
    atomic_operation_1op( __builtin_IB_atomic_and_global_i64, ulong, (global long*)Pointer, Scope, Semantics, Value, true );
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p3i64_i32_i32_i64, )( __local long *Pointer, int Scope, int Semantics, long Value )
{
    return __intel_atomic_binary(ATOMIC_AND64, Pointer, Scope, Semantics, Value);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p4i64_i32_i32_i64, )( __generic long *Pointer, int Scope, int Semantics, long Value )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicAnd, _p3i64_i32_i32_i64, )( (__local long*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        return SPIRV_BUILTIN(AtomicAnd, _p1i64_i32_i32_i64, )( (__global long*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

// Atomic OR


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p0i32_i32_i32_i32, )( __private int *Pointer, int Scope, int Semantics, int Value )
{
    uint orig = *Pointer;
    *Pointer |= Value;
    return orig;
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p1i32_i32_i32_i32, )( __global int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_or_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p3i32_i32_i32_i32, )( __local int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_or_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p4i32_i32_i32_i32, )( __generic int *Pointer, int Scope, int Semantics, int Value )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_or_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value, false );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_or_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_extended_atomics)

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p0i64_i32_i32_i64, )( __private long *Pointer, int Scope, int Semantics, long Value )
{
    ulong orig = *Pointer;
    *Pointer |= Value;
    return orig;
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p1i64_i32_i32_i64, )( __global long *Pointer, int Scope, int Semantics, long Value )
{
    atomic_operation_1op( __builtin_IB_atomic_or_global_i64, ulong, (global long*)Pointer, Scope, Semantics, Value, true );
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p3i64_i32_i32_i64, )( __local long *Pointer, int Scope, int Semantics, long Value )
{
    return __intel_atomic_binary(ATOMIC_OR64, Pointer, Scope, Semantics, Value);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p4i64_i32_i32_i64, )( __generic long *Pointer, int Scope, int Semantics, long Value )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
      return SPIRV_BUILTIN(AtomicOr, _p3i64_i32_i32_i64, )( (__local long*)Pointer, Scope, Semantics, Value );
    }
    else
    {
      return SPIRV_BUILTIN(AtomicOr, _p1i64_i32_i32_i64, )( (__global long*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)


// Atomic Xor


int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p0i32_i32_i32_i32, )( __private int *Pointer, int Scope, int Semantics, int Value )
{
    uint orig = *Pointer;
    *Pointer ^= Value;
    return orig;
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p1i32_i32_i32_i32, )( __global int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xor_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p3i32_i32_i32_i32, )( __local int *Pointer, int Scope, int Semantics, int Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xor_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p4i32_i32_i32_i32, )( __generic int *Pointer, int Scope, int Semantics, int Value )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_xor_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value, false );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_xor_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_extended_atomics)

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p0i64_i32_i32_i64, )( __private long *Pointer, int Scope, int Semantics, long Value )
{
    ulong orig = *Pointer;
    *Pointer ^= Value;
    return orig;
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p1i64_i32_i32_i64, )( __global long *Pointer, int Scope, int Semantics, long Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xor_global_i64, ulong, (global long*)Pointer, Scope, Semantics, Value, true );
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p3i64_i32_i32_i64, )( __local long *Pointer, int Scope, int Semantics, long Value )
{
    return __intel_atomic_binary(ATOMIC_XOR64, Pointer, Scope, Semantics, Value);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p4i64_i32_i32_i64, )( __generic long *Pointer, int Scope, int Semantics, long Value )
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicXor, _p3i64_i32_i32_i64, )( (__local long*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        return SPIRV_BUILTIN(AtomicXor, _p1i64_i32_i32_i64, )( (__global long*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

// Atomic FlagTestAndSet


bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagTestAndSet, _p0i32_i32_i32, )( __private int *Pointer, int Scope, int Semantics )
{
    return (bool)SPIRV_BUILTIN(AtomicExchange, _p0i32_i32_i32_i32, )( Pointer, Scope, Semantics, ATOMIC_FLAG_TRUE );
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagTestAndSet, _p1i32_i32_i32, )( __global int *Pointer, int Scope, int Semantics )
{
    return (bool)SPIRV_BUILTIN(AtomicExchange, _p1i32_i32_i32_i32, )( Pointer, Scope, Semantics, ATOMIC_FLAG_TRUE );
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagTestAndSet, _p3i32_i32_i32, )( __local int *Pointer, int Scope, int Semantics )
{
    return (bool)SPIRV_BUILTIN(AtomicExchange, _p3i32_i32_i32_i32, )( Pointer, Scope, Semantics, ATOMIC_FLAG_TRUE );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagTestAndSet, _p4i32_i32_i32, )( __generic int *Pointer, int Scope, int Semantics )
{
    return (bool)SPIRV_BUILTIN(AtomicExchange, _p4i32_i32_i32_i32, )( Pointer, Scope, Semantics, ATOMIC_FLAG_TRUE );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


// Atomic FlagClear


void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagClear, _p0i32_i32_i32, )( __private int *Pointer, int Scope, int Semantics )
{
    SPIRV_BUILTIN(AtomicStore, _p0i32_i32_i32_i32, )( Pointer, Scope, Semantics, ATOMIC_FLAG_FALSE );
}

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagClear, _p1i32_i32_i32, )( __global int *Pointer, int Scope, int Semantics )
{
    SPIRV_BUILTIN(AtomicStore, _p1i32_i32_i32_i32, )( Pointer, Scope, Semantics, ATOMIC_FLAG_FALSE );
}

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagClear, _p3i32_i32_i32, )( __local int *Pointer, int Scope, int Semantics )
{
    SPIRV_BUILTIN(AtomicStore, _p3i32_i32_i32_i32, )( Pointer, Scope, Semantics, ATOMIC_FLAG_FALSE );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagClear, _p4i32_i32_i32, )( __generic int *Pointer, int Scope, int Semantics )
{
    SPIRV_BUILTIN(AtomicStore, _p4i32_i32_i32_i32, )( Pointer, Scope, Semantics, ATOMIC_FLAG_FALSE );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p0f32_i32_i32_f32, )( __private float *Pointer, int Scope, int Semantics, float Value)
{
    float orig = *Pointer;
    *Pointer += Value;
    return orig;
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p1f32_i32_i32_f32, )( __global float *Pointer, int Scope, int Semantics, float Value)
{
    if(__UseNativeFP32GlobalAtomicAdd)
    {
        atomic_operation_1op_as_float( __builtin_IB_atomic_add_global_f32, float, Pointer, Scope, Semantics, Value, true );
    }
    // We don't use SPINLOCK_START and SPINLOCK_END emulation here, since do-while loop is more efficient for global atomics.
    float orig;
    float desired;
    do {
        orig = as_float(SPIRV_BUILTIN(AtomicLoad, _p1i32_i32_i32, )((__global int*)Pointer, Scope, Semantics));
        desired = orig + Value;
    } while(as_int(orig) != SPIRV_BUILTIN(AtomicCompareExchange, _p1i32_i32_i32_i32_i32_i32, )(
                                (__global int*)Pointer, Scope, Semantics, Semantics,
                                as_int(desired), as_int(orig)));
    return orig;
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p3f32_i32_i32_f32, )( __local float *Pointer, int Scope, int Semantics, float Value)
{
    // We don't use SPINLOCK_START and SPINLOCK_END emulation here, since do-while loop is more efficient for global atomics.
    float orig;
    float desired;
    do {
        orig = as_float(SPIRV_BUILTIN(AtomicLoad, _p3i32_i32_i32, )((__local int*)Pointer, Scope, Semantics));
        desired = orig + Value;
    } while(as_int(orig) != SPIRV_BUILTIN(AtomicCompareExchange, _p3i32_i32_i32_i32_i32_i32, )(
                                (__local int*)Pointer, Scope, Semantics, Semantics,
                                as_int(desired), as_int(orig)));
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p4f32_i32_i32_f32, )( __generic float *Pointer, int Scope, int Semantics, float Value)
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicFAddEXT, _p3f32_i32_i32_f32, )((local float*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return SPIRV_BUILTIN(AtomicFAddEXT, _p1f32_i32_i32_f32, )((global float*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p0f64_i32_i32_f64, )( __private double *Pointer, int Scope, int Semantics, double Value)
{
    double orig = *Pointer;
    *Pointer += Value;
    return orig;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p1f64_i32_i32_f64, )( __global double *Pointer, int Scope, int Semantics, double Value)
{
    if (__UseNativeFP64GlobalAtomicAdd)
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
        orig = as_double(SPIRV_BUILTIN(AtomicLoad, _p1i64_i32_i32, )((__global long*)Pointer, Scope, Semantics));
        desired = orig + Value;
    } while(as_long(orig) != SPIRV_BUILTIN(AtomicCompareExchange, _p1i64_i32_i32_i32_i64_i64, )(
                                (__global long*)Pointer, Scope, Semantics, Semantics,
                                as_long(desired), as_long(orig)));
    return orig;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p3f64_i32_i32_f64, )( __local double *Pointer, int Scope, int Semantics, double Value)
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

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p4f64_i32_i32_f64, )( __generic double *Pointer, int Scope, int Semantics, double Value)
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicFAddEXT, _p3f64_i32_i32_f64, )((local double*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return SPIRV_BUILTIN(AtomicFAddEXT, _p1f64_i32_i32_f64, )((global double*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p0f16_i32_i32_f16, )( private half* Pointer, int Scope, int Semantics, half Value)
{
    half orig = *Pointer;
    *Pointer = (orig < Value) ? orig : Value;
    return orig;
}

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p1f16_i32_i32_f16, )( global half* Pointer, int Scope, int Semantics, half Value)
{
    if(__UseNativeFP16AtomicMinMax)
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

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p3f16_i32_i32_f16, )( local half* Pointer, int Scope, int Semantics, half Value)
{
    if(__UseNativeFP16AtomicMinMax)
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
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p4f16_i32_i32_f16, )( generic half* Pointer, int Scope, int Semantics, half Value)
{
    if (SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicFMinEXT, _p3f16_i32_i32_f16, )((__local half*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return SPIRV_BUILTIN(AtomicFMinEXT, _p1f16_i32_i32_f16, )((__global half*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p0f32_i32_i32_f32, )( private float* Pointer, int Scope, int Semantics, float Value)
{
    float orig = *Pointer;
    *Pointer = (orig < Value) ? orig : Value;
    return orig;
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p1f32_i32_i32_f32, )( global float* Pointer, int Scope, int Semantics, float Value)
{
    atomic_operation_1op_as_float(__builtin_IB_atomic_min_global_f32, float, Pointer, Scope, Semantics, Value, true);
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p3f32_i32_i32_f32, )( local float* Pointer, int Scope, int Semantics, float Value)
{
    atomic_operation_1op_as_float(__builtin_IB_atomic_min_local_f32, float, Pointer, Scope, Semantics, Value, false);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p4f32_i32_i32_f32, )( generic float* Pointer, int Scope, int Semantics, float Value)
{
    if (SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicFMinEXT, _p3f32_i32_i32_f32, )((__local float*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return SPIRV_BUILTIN(AtomicFMinEXT, _p1f32_i32_i32_f32, )((__global float*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p0f64_i32_i32_f64, )( private double* Pointer, int Scope, int Semantics, double Value)
{
    double orig = *Pointer;
    *Pointer = (orig < Value) ? orig : Value;
    return orig;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p1f64_i32_i32_f64, )( global double* Pointer, int Scope, int Semantics, double Value)
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
        orig = as_double(SPIRV_BUILTIN(AtomicLoad, _p1i64_i32_i32, )((__global long*)Pointer, Scope, Semantics));
        desired = ( orig < Value ) ? orig : Value;
    } while(as_long(orig) != SPIRV_BUILTIN(AtomicCompareExchange, _p1i64_i32_i32_i32_i64_i64, )(
                                (__global long*)Pointer, Scope, Semantics, Semantics,
                                as_long(desired), as_long(orig)));
    return orig;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p3f64_i32_i32_f64, )( local double* Pointer, int Scope, int Semantics, double Value)
{
    double orig;
    double desired;
    do {
        orig = as_double(SPIRV_BUILTIN(AtomicLoad, _p3i64_i32_i32, )((__local long*)Pointer, Scope, Semantics));
        desired = ( orig < Value ) ? orig : Value;
    } while(as_long(orig) != SPIRV_BUILTIN(AtomicCompareExchange, _p3i64_i32_i32_i32_i64_i64, )(
                                (__local long*)Pointer, Scope, Semantics, Semantics,
                                as_long(desired), as_long(orig)));
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p4f64_i32_i32_f64, )( generic double* Pointer, int Scope, int Semantics, double Value)
{
    if (SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicFMinEXT, _p3f64_i32_i32_f64, )((__local double*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return SPIRV_BUILTIN(AtomicFMinEXT, _p1f64_i32_i32_f64, )((__global double*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p0f16_i32_i32_f16, )( private half* Pointer, int Scope, int Semantics, half Value)
{
    half orig = *Pointer;
    *Pointer = (orig > Value) ? orig : Value;
    return orig;
}

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p1f16_i32_i32_f16, )( global half* Pointer, int Scope, int Semantics, half Value)
{
    if(__UseNativeFP16AtomicMinMax)
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

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p3f16_i32_i32_f16, )( local half* Pointer, int Scope, int Semantics, half Value)
{
    if(__UseNativeFP16AtomicMinMax)
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
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p4f16_i32_i32_f16, )( generic half* Pointer, int Scope, int Semantics, half Value)
{
    if (SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicFMaxEXT, _p3f16_i32_i32_f16, )((__local half*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return SPIRV_BUILTIN(AtomicFMaxEXT, _p1f16_i32_i32_f16, )((__global half*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p0f32_i32_i32_f32, )( private float* Pointer, int Scope, int Semantics, float Value)
{
    float orig = *Pointer;
    *Pointer = (orig > Value) ? orig : Value;
    return orig;
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p1f32_i32_i32_f32, )( global float* Pointer, int Scope, int Semantics, float Value)
{
    atomic_operation_1op_as_float(__builtin_IB_atomic_max_global_f32, float, Pointer, Scope, Semantics, Value, true);
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p3f32_i32_i32_f32, )( local float* Pointer, int Scope, int Semantics, float Value)
{
    atomic_operation_1op_as_float(__builtin_IB_atomic_max_local_f32, float, Pointer, Scope, Semantics, Value, false);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p4f32_i32_i32_f32, )( generic float* Pointer, int Scope, int Semantics, float Value)
{
    if (SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicFMaxEXT, _p3f32_i32_i32_f32, )((__local float*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return SPIRV_BUILTIN(AtomicFMaxEXT, _p1f32_i32_i32_f32, )((__global float*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p0f64_i32_i32_f64, )( private double* Pointer, int Scope, int Semantics, double Value)
{
    double orig = *Pointer;
    *Pointer = (orig > Value) ? orig : Value;
    return orig;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p1f64_i32_i32_f64, )( global double* Pointer, int Scope, int Semantics, double Value)
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
        orig = as_double(SPIRV_BUILTIN(AtomicLoad, _p1i64_i32_i32, )((__global long*)Pointer, Scope, Semantics));
        desired = ( orig > Value ) ? orig : Value;
    } while(as_long(orig) != SPIRV_BUILTIN(AtomicCompareExchange, _p1i64_i32_i32_i32_i64_i64, )(
                                (__global long*)Pointer, Scope, Semantics, Semantics,
                                as_long(desired), as_long(orig)));
    return orig;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p3f64_i32_i32_f64, )( local double* Pointer, int Scope, int Semantics, double Value)
{
    double orig;
    double desired;
    do {
        orig = as_double(SPIRV_BUILTIN(AtomicLoad, _p3i64_i32_i32, )((__local long*)Pointer, Scope, Semantics));
        desired = ( orig > Value ) ? orig : Value;
    } while(as_long(orig) != SPIRV_BUILTIN(AtomicCompareExchange, _p3i64_i32_i32_i32_i64_i64, )(
                                (__local long*)Pointer, Scope, Semantics, Semantics,
                                as_long(desired), as_long(orig)));
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p4f64_i32_i32_f64, )( generic double* Pointer, int Scope, int Semantics, double Value)
{
    if (SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicFMaxEXT, _p3f64_i32_i32_f64, )((__local double*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return SPIRV_BUILTIN(AtomicFMaxEXT, _p1f64_i32_i32_f64, )((__global double*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAdd, _p1f32_i32_i32_f32, )( __global float *Pointer, int Scope, int Semantics, float Value)
{
    atomic_operation_1op_as_float( __builtin_IB_atomic_add_global_f32, float, Pointer, Scope, Semantics, Value, true );
}
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFSub, _p1f32_i32_i32_f32, )( __global float *Pointer, int Scope, int Semantics, float Value)
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
