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

// Atomic Instructions

#include "../Headers/spirv.h"

#define ATOMIC_FLAG_TRUE 1
#define ATOMIC_FLAG_FALSE 0

#define SEMANTICS_PRE_OP_NEED_FENCE ( Release | AcquireRelease | SequentiallyConsistent)

#define SEMANTICS_POST_OP_NEEDS_FENCE ( Acquire | AcquireRelease | SequentiallyConsistent)

  __local uint* __builtin_IB_get_local_lock();
  void __builtin_IB_eu_thread_pause(uint value);

#define SPINLOCK_START \
  { \
  volatile bool done = false; \
  while(!done) { \
       __builtin_IB_eu_thread_pause(32); \
       if(atomic_cmpxchg(__builtin_IB_get_local_lock(), 0, 1) == 0) {

#define SPINLOCK_END \
            done = true; \
            atomic_store(__builtin_IB_get_local_lock(), 0); \
  }}}

#define FENCE_PRE_OP(Scope, Semantics, isGlobal)                                  \
  if( ( (Semantics) & ( SEMANTICS_PRE_OP_NEED_FENCE ) ) > 0 )                     \
  {                                                                               \
      __builtin_IB_memfence(true, false, false, false, false, isGlobal, false);   \
  }

#define FENCE_POST_OP(Scope, Semantics, isGlobal)                                 \
  if( ( (Semantics) & ( SEMANTICS_POST_OP_NEEDS_FENCE ) ) > 0 )                   \
  {                                                                               \
      __builtin_IB_memfence(true, false, false, false, false, isGlobal, false);   \
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

uint __builtin_spirv_OpAtomicLoad_p0i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics )
{
    return *Pointer;
}

uint __builtin_spirv_OpAtomicLoad_p1i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics )
{
    return __builtin_spirv_OpAtomicOr_p1i32_i32_i32_i32( Pointer, Scope, Semantics, 0 );
}

uint __builtin_spirv_OpAtomicLoad_p3i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics )
{
    return __builtin_spirv_OpAtomicOr_p3i32_i32_i32_i32( Pointer, Scope, Semantics, 0 );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicLoad_p4i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics )
{
    return __builtin_spirv_OpAtomicOr_p4i32_i32_i32_i32( Pointer, Scope, Semantics, 0 );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

ulong __builtin_spirv_OpAtomicLoad_p0i64_i32_i32( volatile __private ulong *Pointer, uint Scope, uint Semantics )
{
    return *Pointer;
}

ulong __builtin_spirv_OpAtomicLoad_p1i64_i32_i32( volatile __global ulong *Pointer, uint Scope, uint Semantics )
{
    return __builtin_spirv_OpAtomicOr_p1i64_i32_i32_i64( Pointer, Scope, Semantics, 0 );
}

ulong __builtin_spirv_OpAtomicLoad_p3i64_i32_i32( volatile __local ulong *Pointer, uint Scope, uint Semantics )
{
    return __builtin_spirv_OpAtomicOr_p3i64_i32_i32_i64( Pointer, Scope, Semantics, 0 );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

ulong __builtin_spirv_OpAtomicLoad_p4i64_i32_i32( volatile __generic ulong *Pointer, uint Scope, uint Semantics )
{
    return __builtin_spirv_OpAtomicOr_p4i64_i32_i32_i64( Pointer, Scope, Semantics, 0 );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)


float __builtin_spirv_OpAtomicLoad_p0f32_i32_i32( volatile __private float *Pointer, uint Scope, uint Semantics )
{
    return *Pointer;
}


float __builtin_spirv_OpAtomicLoad_p1f32_i32_i32( volatile __global float *Pointer, uint Scope, uint Semantics )
{
    return as_float( __builtin_spirv_OpAtomicOr_p1i32_i32_i32_i32( (volatile __global uint*)Pointer, Scope, Semantics, 0 ) );
}

float __builtin_spirv_OpAtomicLoad_p3f32_i32_i32( volatile __local float *Pointer, uint Scope, uint Semantics )
{
    return as_float( __builtin_spirv_OpAtomicOr_p3i32_i32_i32_i32( (volatile __local uint*)Pointer, Scope, Semantics, 0 ) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float __builtin_spirv_OpAtomicLoad_p4f32_i32_i32( volatile __generic float *Pointer, uint Scope, uint Semantics )
{
    return as_float( __builtin_spirv_OpAtomicOr_p4i32_i32_i32_i32( (volatile __generic uint*)Pointer, Scope, Semantics, 0 ) );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp64)
#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
double __builtin_spirv_OpAtomicLoad_p0f64_i32_i32( volatile __private double *Pointer, uint Scope, uint Semantics )
{
    return *Pointer;
}


double __builtin_spirv_OpAtomicLoad_p1f64_i32_i32( volatile __global double *Pointer, uint Scope, uint Semantics )
{
    return as_double( __builtin_spirv_OpAtomicOr_p1i64_i32_i32_i64( (volatile __global ulong*)Pointer, Scope, Semantics, 0 ) );
}

double __builtin_spirv_OpAtomicLoad_p3f64_i32_i32( volatile __local double *Pointer, uint Scope, uint Semantics )
{
    return as_double( __builtin_spirv_OpAtomicOr_p3i64_i32_i32_i64( (volatile __local ulong*)Pointer, Scope, Semantics, 0 ) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double __builtin_spirv_OpAtomicLoad_p4f64_i32_i32( volatile __generic double *Pointer, uint Scope, uint Semantics )
{
    return as_double( __builtin_spirv_OpAtomicOr_p4i64_i32_i32_i64( (volatile __generic ulong*)Pointer, Scope, Semantics, 0 ) );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
#endif // defined(cl_khr_fp64)


// Atomic Stores


void __builtin_spirv_OpAtomicStore_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    *Pointer = Value;
}


void __builtin_spirv_OpAtomicStore_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    __builtin_spirv_OpAtomicExchange_p1i32_i32_i32_i32( Pointer, Scope, Semantics, Value );
}


void __builtin_spirv_OpAtomicStore_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    __builtin_spirv_OpAtomicExchange_p3i32_i32_i32_i32( Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void __builtin_spirv_OpAtomicStore_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    __builtin_spirv_OpAtomicExchange_p4i32_i32_i32_i32( Pointer, Scope, Semantics, Value );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

void __builtin_spirv_OpAtomicStore_p0i64_i32_i32_i64( volatile __private ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    *Pointer = Value;
}


void __builtin_spirv_OpAtomicStore_p1i64_i32_i32_i64( volatile __global ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    __builtin_spirv_OpAtomicExchange_p1i64_i32_i32_i64( Pointer, Scope, Semantics, Value );
}


void __builtin_spirv_OpAtomicStore_p3i64_i32_i32_i64( volatile __local ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    __builtin_spirv_OpAtomicExchange_p3i64_i32_i32_i64( Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void __builtin_spirv_OpAtomicStore_p4i64_i32_i32_i64( volatile __generic ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    __builtin_spirv_OpAtomicExchange_p4i64_i32_i32_i64( Pointer, Scope, Semantics, Value );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)


void __builtin_spirv_OpAtomicStore_p0f32_i32_i32_f32( volatile __private float *Pointer, uint Scope, uint Semantics, float Value )
{
    __builtin_spirv_OpAtomicExchange_p0f32_i32_i32_f32( Pointer, Scope, Semantics, Value );
}


void __builtin_spirv_OpAtomicStore_p1f32_i32_i32_f32( volatile __global float *Pointer, uint Scope, uint Semantics, float Value )
{
    __builtin_spirv_OpAtomicExchange_p1f32_i32_i32_f32( Pointer, Scope, Semantics, Value );
}


void __builtin_spirv_OpAtomicStore_p3f32_i32_i32_f32( volatile __local float *Pointer, uint Scope, uint Semantics, float Value )
{
    __builtin_spirv_OpAtomicExchange_p3f32_i32_i32_f32( Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void __builtin_spirv_OpAtomicStore_p4f32_i32_i32_f32( volatile __generic float *Pointer, uint Scope, uint Semantics, float Value )
{
    __builtin_spirv_OpAtomicExchange_p4f32_i32_i32_f32( Pointer, Scope, Semantics, Value );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp64)
#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

void __builtin_spirv_OpAtomicStore_p0f64_i32_i32_f64( volatile __private double *Pointer, uint Scope, uint Semantics, double Value )
{
    __builtin_spirv_OpAtomicExchange_p0f64_i32_i32_f64( Pointer, Scope, Semantics, Value );
}


void __builtin_spirv_OpAtomicStore_p1f64_i32_i32_f64( volatile __global double *Pointer, uint Scope, uint Semantics, double Value )
{
    __builtin_spirv_OpAtomicExchange_p1f64_i32_i32_f64( Pointer, Scope, Semantics, Value );
}


void __builtin_spirv_OpAtomicStore_p3f64_i32_i32_f64( volatile __local double *Pointer, uint Scope, uint Semantics, double Value )
{
    __builtin_spirv_OpAtomicExchange_p3f64_i32_i32_f64( Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void __builtin_spirv_OpAtomicStore_p4f64_i32_i32_f64( volatile __generic double *Pointer, uint Scope, uint Semantics, double Value )
{
    __builtin_spirv_OpAtomicExchange_p4f64_i32_i32_f64( Pointer, Scope, Semantics, Value );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
#endif // defined(cl_khr_fp64)


// Atomic Exchange


uint __builtin_spirv_OpAtomicExchange_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    uint orig = *Pointer;
    *Pointer = Value;
    return orig;
}


uint __builtin_spirv_OpAtomicExchange_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xchg_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}


uint __builtin_spirv_OpAtomicExchange_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xchg_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicExchange_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
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
ulong __builtin_spirv_OpAtomicExchange_p0i64_i32_i32_i64( volatile __private ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    ulong orig = *Pointer;
    *Pointer = Value;
    return orig;
}


ulong __builtin_spirv_OpAtomicExchange_p1i64_i32_i32_i64( volatile __global ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xchg_global_i64, ulong, (global long*)Pointer, Scope, Semantics, Value, true );
}


ulong __builtin_spirv_OpAtomicExchange_p3i64_i32_i32_i64( volatile __local ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    ulong orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    SPINLOCK_START
    orig = *Pointer;
    *Pointer = Value;
    SPINLOCK_END
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}


#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

ulong __builtin_spirv_OpAtomicExchange_p4i64_i32_i32_i64( volatile __generic ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        return __builtin_spirv_OpAtomicExchange_p3i64_i32_i32_i64((__local long*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __builtin_spirv_OpAtomicExchange_p1i64_i32_i32_i64((__global long*)Pointer, Scope, Semantics, Value);
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics)

float __builtin_spirv_OpAtomicExchange_p0f32_i32_i32_f32( volatile __private float *Pointer, uint Scope, uint Semantics, float Value)
{
    float orig = *Pointer;

    *Pointer = Value;

    return orig;
}

float __builtin_spirv_OpAtomicExchange_p1f32_i32_i32_f32( volatile __global float *Pointer, uint Scope, uint Semantics, float Value)
{
    atomic_operation_1op_as_float( __builtin_IB_atomic_xchg_global_i32, float, (global int*)Pointer, Scope, Semantics, as_int(Value), true );
}


float __builtin_spirv_OpAtomicExchange_p3f32_i32_i32_f32( volatile __local float *Pointer, uint Scope, uint Semantics, float Value)
{
    atomic_operation_1op_as_float( __builtin_IB_atomic_xchg_local_i32, float, (local int*)Pointer, Scope, Semantics, as_int(Value), false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float __builtin_spirv_OpAtomicExchange_p4f32_i32_i32_f32( volatile __generic float *Pointer, uint Scope, uint Semantics, float Value)
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
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

double __builtin_spirv_OpAtomicExchange_p0f64_i32_i32_f64( volatile __private double *Pointer, uint Scope, uint Semantics, double Value)
{
    return as_double(__builtin_spirv_OpAtomicExchange_p0i64_i32_i32_i64((__private long*) Pointer, Scope, Semantics, as_long(Value)));
}

double __builtin_spirv_OpAtomicExchange_p1f64_i32_i32_f64( volatile __global double *Pointer, uint Scope, uint Semantics, double Value)
{
    return as_double(__builtin_spirv_OpAtomicExchange_p1i64_i32_i32_i64((__global long*) Pointer, Scope, Semantics, as_long(Value)));
}


double __builtin_spirv_OpAtomicExchange_p3f64_i32_i32_f64( volatile __local double *Pointer, uint Scope, uint Semantics, double Value)
{
    return as_double(__builtin_spirv_OpAtomicExchange_p3i64_i32_i32_i64((__local long*) Pointer, Scope, Semantics, as_long(Value)));
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double __builtin_spirv_OpAtomicExchange_p4f64_i32_i32_f64( volatile __generic double *Pointer, uint Scope, uint Semantics, double Value)
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        return __builtin_spirv_OpAtomicExchange_p3f64_i32_i32_f64((__local double*) Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __builtin_spirv_OpAtomicExchange_p1f64_i32_i32_f64((__global double*) Pointer, Scope, Semantics, Value);
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics)
#endif // defined(cl_khr_fp64)


// Atomic Compare Exchange


uint __builtin_spirv_OpAtomicCompareExchange_p0i32_i32_i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Equal, uint Unequal, uint Value, uint Comparator)
{
    uint orig = *Pointer;
    if( orig == Comparator )
    {
        *Pointer = Value;
    }
    return orig;
}


uint __builtin_spirv_OpAtomicCompareExchange_p1i32_i32_i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Equal, uint Unequal, uint Value, uint Comparator)
{
    atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_global_i32, uint, (global int*)Pointer, Scope, Equal, Value, Comparator, true );
}


uint __builtin_spirv_OpAtomicCompareExchange_p3i32_i32_i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Equal, uint Unequal, uint Value, uint Comparator)
{
    atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_local_i32, uint, (local int*)Pointer, Scope, Equal, Value, Comparator, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicCompareExchange_p4i32_i32_i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Equal, uint Unequal, uint Value, uint Comparator)
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
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
ulong __builtin_spirv_OpAtomicCompareExchange_p0i64_i32_i32_i32_i64_i64( volatile __private ulong *Pointer, uint Scope, uint Equal, uint Unequal, ulong Value, ulong Comparator)
{
    ulong orig = *Pointer;
    if( orig == Comparator )
    {
        *Pointer = Value;
    }
    return orig;
}


ulong __builtin_spirv_OpAtomicCompareExchange_p1i64_i32_i32_i32_i64_i64( volatile __global ulong *Pointer, uint Scope, uint Equal, uint Unequal, ulong Value, ulong Comparator)
{
    atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_global_i64, ulong, (global long*)Pointer, Scope, Equal, Value, Comparator, true );
}


ulong __builtin_spirv_OpAtomicCompareExchange_p3i64_i32_i32_i32_i64_i64( volatile __local ulong *Pointer, uint Scope, uint Equal, uint Unequal, ulong Value, ulong Comparator)
{
    ulong orig;
    FENCE_PRE_OP(Scope, Equal, false)
    SPINLOCK_START
    orig = *Pointer;
    if( orig == Comparator )
    {
        *Pointer = Value;
    }
    SPINLOCK_END
    FENCE_POST_OP(Scope, Equal, false)
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

ulong __builtin_spirv_OpAtomicCompareExchange_p4i64_i32_i32_i32_i64_i64( volatile __generic ulong *Pointer, uint Scope, uint Equal, uint Unequal, ulong Value, ulong Comparator)
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        return __builtin_spirv_OpAtomicCompareExchange_p3i64_i32_i32_i32_i64_i64( (__local long*)Pointer, Scope, Equal, Unequal, Value, Comparator );
    }
    else
    {
        return __builtin_spirv_OpAtomicCompareExchange_p1i64_i32_i32_i32_i64_i64( (__global long*)Pointer, Scope, Equal, Unequal, Value, Comparator );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics)

float __builtin_spirv_OpAtomicCompareExchange_p0f32_i32_i32_i32_f32_f32( volatile __private float *Pointer, uint Scope, uint Equal, uint Unequal, float Value, float Comparator)
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

float __builtin_spirv_OpAtomicCompareExchange_p1f32_i32_i32_i32_f32_f32( volatile __global float *Pointer, uint Scope, uint Equal, uint Unequal, float Value, float Comparator)
{
    atomic_cmpxhg_as_float( __builtin_IB_atomic_cmpxchg_global_i32, float, (global int*)Pointer, Scope, Equal, as_uint(Value), as_uint(Comparator), true );
}


float __builtin_spirv_OpAtomicCompareExchange_p3f32_i32_i32_i32_f32_f32( volatile __local float *Pointer, uint Scope, uint Equal, uint Unequal, float Value, float Comparator)
{
    atomic_cmpxhg_as_float( __builtin_IB_atomic_cmpxchg_local_i32, float, (local int*)Pointer, Scope, Equal, as_uint(Value), as_uint(Comparator), false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float __builtin_spirv_OpAtomicCompareExchange_p4f32_i32_i32_i32_f32_f32( volatile __generic float *Pointer, uint Scope, uint Equal, uint Unequal, float Value, float Comparator)
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        atomic_cmpxhg_as_float( __builtin_IB_atomic_cmpxchg_local_i32, float, (__local int*)Pointer, Scope, Equal, as_uint(Value), as_uint(Comparator), false );
    }
    else
    {
        atomic_cmpxhg_as_float( __builtin_IB_atomic_cmpxchg_global_i32, float, (__global int*)Pointer, Scope, Equal, as_uint(Value), as_uint(Comparator), true );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicCompareExchangeWeak_p0i32_i32_i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Equal, uint Unequal, uint Value, uint Comparator)
{
    return __builtin_spirv_OpAtomicCompareExchange_p0i32_i32_i32_i32_i32_i32( Pointer, Scope, Equal, Unequal, Value, Comparator );
}


uint __builtin_spirv_OpAtomicCompareExchangeWeak_p1i32_i32_i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Equal, uint Unequal, uint Value, uint Comparator)
{
    return __builtin_spirv_OpAtomicCompareExchange_p1i32_i32_i32_i32_i32_i32( Pointer, Scope, Equal, Unequal, Value, Comparator );
}


uint __builtin_spirv_OpAtomicCompareExchangeWeak_p3i32_i32_i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Equal, uint Unequal, uint Value, uint Comparator)
{
    return __builtin_spirv_OpAtomicCompareExchange_p3i32_i32_i32_i32_i32_i32( Pointer, Scope, Equal, Unequal, Value, Comparator );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicCompareExchangeWeak_p4i32_i32_i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Equal, uint Unequal, uint Value, uint Comparator)
{
    return __builtin_spirv_OpAtomicCompareExchange_p4i32_i32_i32_i32_i32_i32( Pointer, Scope, Equal, Unequal, Value, Comparator );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_int64_base_atomics)
ulong __builtin_spirv_OpAtomicCompareExchangeWeak_p0i64_i32_i32_i32_i64_i64( volatile __private ulong *Pointer, uint Scope, uint Equal, uint Unequal, ulong Value, ulong Comparator)
{
    return __builtin_spirv_OpAtomicCompareExchange_p0i64_i32_i32_i32_i64_i64( Pointer, Scope, Equal, Unequal, Value, Comparator );
}


ulong __builtin_spirv_OpAtomicCompareExchangeWeak_p1i64_i32_i32_i32_i64_i64( volatile __global ulong *Pointer, uint Scope, uint Equal, uint Unequal, ulong Value, ulong Comparator)
{
    return __builtin_spirv_OpAtomicCompareExchange_p1i64_i32_i32_i32_i64_i64( Pointer, Scope, Equal, Unequal, Value, Comparator );
}


ulong __builtin_spirv_OpAtomicCompareExchangeWeak_p3i64_i32_i32_i32_i64_i64( volatile __local ulong *Pointer, uint Scope, uint Equal, uint Unequal, ulong Value, ulong Comparator)
{
    return __builtin_spirv_OpAtomicCompareExchange_p3i64_i32_i32_i32_i64_i64( Pointer, Scope, Equal, Unequal, Value, Comparator );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

ulong __builtin_spirv_OpAtomicCompareExchangeWeak_p4i64_i32_i32_i32_i64_i64( volatile __generic ulong *Pointer, uint Scope, uint Equal, uint Unequal, ulong Value, ulong Comparator)
{
    return __builtin_spirv_OpAtomicCompareExchange_p4i64_i32_i32_i32_i64_i64( Pointer, Scope, Equal, Unequal, Value, Comparator );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_int64_base_atomics)

// Atomic Increment


uint __builtin_spirv_OpAtomicIIncrement_p0i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics )
{
    uint orig = *Pointer;
    *Pointer += 1;
    return orig;
}


uint __builtin_spirv_OpAtomicIIncrement_p1i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_inc_global_i32, uint, (global int*)Pointer, Scope, Semantics, true );
}


uint __builtin_spirv_OpAtomicIIncrement_p3i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_inc_local_i32, uint, (local int*)Pointer, Scope, Semantics, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicIIncrement_p4i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
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
ulong __builtin_spirv_OpAtomicIIncrement_p0i64_i32_i32( volatile __private ulong *Pointer, uint Scope, uint Semantics )
{
    ulong orig = *Pointer;
    *Pointer += 1;
    return orig;
}


ulong __builtin_spirv_OpAtomicIIncrement_p1i64_i32_i32( volatile __global ulong *Pointer, uint Scope, uint Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_inc_global_i64, ulong, (global int*)Pointer, Scope, Semantics, true );
}


ulong __builtin_spirv_OpAtomicIIncrement_p3i64_i32_i32( volatile __local ulong *Pointer, uint Scope, uint Semantics )
{
    ulong orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    SPINLOCK_START
    orig = *Pointer;
    *Pointer += 1;
    SPINLOCK_END
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

ulong __builtin_spirv_OpAtomicIIncrement_p4i64_i32_i32( volatile __generic ulong *Pointer, uint Scope, uint Semantics )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        return __builtin_spirv_OpAtomicIIncrement_p3i64_i32_i32((__local long*)Pointer, Scope, Semantics );
    }
    else
    {
        return __builtin_spirv_OpAtomicIIncrement_p1i64_i32_i32((__global long*)Pointer, Scope, Semantics );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_int64_base_atomics)

// Atomic Decrement


uint __builtin_spirv_OpAtomicIDecrement_p0i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics )
{
    uint orig = *Pointer;

    *Pointer -= 1;

    return orig;
}

uint __builtin_spirv_OpAtomicIDecrement_p1i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_dec_global_i32, uint, (global int*)Pointer, Scope, Semantics, true );
}

uint __builtin_spirv_OpAtomicIDecrement_p3i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_dec_local_i32, uint, (local int*)Pointer, Scope, Semantics, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicIDecrement_p4i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
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
ulong __builtin_spirv_OpAtomicIDecrement_p0i64_i32_i32( volatile __private ulong *Pointer, uint Scope, uint Semantics )
{
    ulong orig = *Pointer;
    *Pointer -= 1;
    return orig;
}

ulong __builtin_spirv_OpAtomicIDecrement_p1i64_i32_i32( volatile __global ulong *Pointer, uint Scope, uint Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_dec_global_i64, ulong, (global long*)Pointer, Scope, Semantics, true );
}

ulong __builtin_spirv_OpAtomicIDecrement_p3i64_i32_i32( volatile __local ulong *Pointer, uint Scope, uint Semantics )
{
    ulong orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    SPINLOCK_START
    orig = *Pointer;
    *Pointer -= 1;
    SPINLOCK_END
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

ulong __builtin_spirv_OpAtomicIDecrement_p4i64_i32_i32( volatile __generic ulong *Pointer, uint Scope, uint Semantics )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        return __builtin_spirv_OpAtomicIDecrement_p3i64_i32_i32( (__local long*)Pointer, Scope, Semantics );
    }
    else
    {
        return __builtin_spirv_OpAtomicIDecrement_p1i64_i32_i32( (__global long*)Pointer, Scope, Semantics );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_int64_base_atomics)


// Atomic IAdd


uint __builtin_spirv_OpAtomicIAdd_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    uint orig = *Pointer;

    *Pointer += Value;

    return orig;
}


uint __builtin_spirv_OpAtomicIAdd_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_add_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}

uint __builtin_spirv_OpAtomicIAdd_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_add_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicIAdd_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_add_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value, false );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_add_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
    }
}

#if defined(cl_khr_int64_base_atomics)
ulong __builtin_spirv_OpAtomicIAdd_p0i64_i32_i32_i64( volatile __private ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    ulong orig = *Pointer;
    *Pointer += Value;
    return orig;
}

ulong __builtin_spirv_OpAtomicIAdd_p1i64_i32_i32_i64( volatile __global ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    atomic_operation_1op( __builtin_IB_atomic_add_global_i64, ulong, (__global ulong*)Pointer, Scope, Semantics, Value, true );
}

ulong __builtin_spirv_OpAtomicIAdd_p3i64_i32_i32_i64( volatile __local ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    ulong orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    SPINLOCK_START
    orig = *Pointer;
    *Pointer += Value;
    SPINLOCK_END
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}


ulong __builtin_spirv_OpAtomicIAdd_p4i64_i32_i32_i64( volatile __generic ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        return __builtin_spirv_OpAtomicIAdd_p3i64_i32_i32_i64((__local ulong*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __builtin_spirv_OpAtomicIAdd_p1i64_i32_i32_i64((__global ulong*)Pointer, Scope, Semantics, Value);
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#endif // defined(cl_khr_int64_base_atomics)

// Atomic ISub

uint __builtin_spirv_OpAtomicISub_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    uint orig = *Pointer;

    *Pointer -= Value;

    return orig;
}


uint __builtin_spirv_OpAtomicISub_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_sub_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}


uint __builtin_spirv_OpAtomicISub_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_sub_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicISub_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
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
ulong __builtin_spirv_OpAtomicISub_p0i64_i32_i32_i64( volatile __private ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    ulong orig = *Pointer;
    *Pointer -= Value;
    return orig;
}


ulong __builtin_spirv_OpAtomicISub_p1i64_i32_i32_i64( volatile __global ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    atomic_operation_1op( __builtin_IB_atomic_sub_global_i64, ulong, (global long*)Pointer, Scope, Semantics, Value, true );
}


ulong __builtin_spirv_OpAtomicISub_p3i64_i32_i32_i64( volatile __local ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    ulong orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    SPINLOCK_START
    orig = *Pointer;
    *Pointer -= Value;
    SPINLOCK_END
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

ulong __builtin_spirv_OpAtomicISub_p4i64_i32_i32_i64( volatile __generic ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        return __builtin_spirv_OpAtomicISub_p3i64_i32_i32_i64((__local long*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        return __builtin_spirv_OpAtomicISub_p1i64_i32_i32_i64((__global long*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_base_atomics)


// Atomic SMin


int __builtin_spirv_OpAtomicSMin_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, int Value)
{
    int orig = *Pointer;
    *Pointer = ( orig < Value ) ? orig : Value;
    return orig;
}

int __builtin_spirv_OpAtomicSMin_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, int Value)
{
    atomic_operation_1op( __builtin_IB_atomic_min_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value, true );
}

int __builtin_spirv_OpAtomicSMin_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, int Value)
{
    atomic_operation_1op( __builtin_IB_atomic_min_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int __builtin_spirv_OpAtomicSMin_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, int Value)
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
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

long __builtin_spirv_OpAtomicSMin_p0i64_i32_i32_i64( volatile __private ulong *Pointer, uint Scope, uint Semantics, long Value)
{
    long orig = *Pointer;
    *Pointer = ( orig < Value ) ? orig : Value;
    return orig;
}

long __builtin_spirv_OpAtomicSMin_p1i64_i32_i32_i64( volatile __global ulong *Pointer, uint Scope, uint Semantics, long Value)
{
    atomic_operation_1op( __builtin_IB_atomic_min_global_i64, ulong, (__global long*)Pointer, Scope, Semantics, Value, true );
}

long __builtin_spirv_OpAtomicSMin_p3i64_i32_i32_i64( volatile __local ulong *Pointer, uint Scope, uint Semantics, long Value)
{
    long orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    SPINLOCK_START
    orig = *Pointer;
    *Pointer = ( orig < Value ) ? orig : Value;
    SPINLOCK_END
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long __builtin_spirv_OpAtomicSMin_p4i64_i32_i32_i64( volatile __generic ulong *Pointer, uint Scope, uint Semantics, long Value)
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        return __builtin_spirv_OpAtomicSMin_p3i64_i32_i32_i64((__local int*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        return __builtin_spirv_OpAtomicSMin_p1i64_i32_i32_i64((__global int*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

uint __builtin_spirv_OpAtomicUMin_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    uint orig = *Pointer;

    *Pointer = ( orig < Value ) ? orig : Value;

    return orig;
}

uint __builtin_spirv_OpAtomicUMin_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_min_global_u32, uint, Pointer, Scope, Semantics, Value, true );
}

uint __builtin_spirv_OpAtomicUMin_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_min_local_u32, uint, Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicUMin_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
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

ulong __builtin_spirv_OpAtomicUMin_p0i64_i32_i32_i64( volatile __private ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    ulong orig = *Pointer;
    *Pointer = ( orig < Value ) ? orig : Value;
    return orig;
}

ulong __builtin_spirv_OpAtomicUMin_p1i64_i32_i32_i64( volatile __global ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    atomic_operation_1op( __builtin_IB_atomic_min_global_u64, ulong, Pointer, Scope, Semantics, Value, true );
}

ulong __builtin_spirv_OpAtomicUMin_p3i64_i32_i32_i64( volatile __local ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    ulong orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    SPINLOCK_START
    orig = *Pointer;
    *Pointer = ( orig < Value ) ? orig : Value;
    SPINLOCK_END
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

ulong __builtin_spirv_OpAtomicUMin_p4i64_i32_i32_i64( volatile __generic ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        return __builtin_spirv_OpAtomicUMin_p3i64_i32_i32_i64( (__local ulong*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        return __builtin_spirv_OpAtomicUMin_p1i64_i32_i32_i64( (__global ulong*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

// Atomic SMax


int __builtin_spirv_OpAtomicSMax_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, int Value)
{
    int orig = *Pointer;
    *Pointer = ( orig > Value ) ? orig : Value;
    return orig;
}

int __builtin_spirv_OpAtomicSMax_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, int Value)
{
    atomic_operation_1op( __builtin_IB_atomic_max_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}

int __builtin_spirv_OpAtomicSMax_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, int Value)
{
    atomic_operation_1op( __builtin_IB_atomic_max_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int __builtin_spirv_OpAtomicSMax_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, int Value)
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
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

long __builtin_spirv_OpAtomicSMax_p0i64_i32_i32_i64( volatile __private ulong *Pointer, uint Scope, uint Semantics, long Value)
{
    long orig = *Pointer;
    *Pointer = ( orig > Value ) ? orig : Value;
    return orig;
}

long __builtin_spirv_OpAtomicSMax_p1i64_i32_i32_i64( volatile __global ulong *Pointer, uint Scope, uint Semantics, long Value)
{
    atomic_operation_1op( __builtin_IB_atomic_max_global_i64, ulong, (global long*)Pointer, Scope, Semantics, Value, true );
}

long __builtin_spirv_OpAtomicSMax_p3i64_i32_i32_i64( volatile __local ulong *Pointer, uint Scope, uint Semantics, long Value)
{
    long orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    SPINLOCK_START
    orig = *Pointer;
    *Pointer = ( orig > Value ) ? orig : Value;
    SPINLOCK_END
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

long __builtin_spirv_OpAtomicSMax_p4i64_i32_i32_i64( volatile __generic ulong *Pointer, uint Scope, uint Semantics, long Value)
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        return __builtin_spirv_OpAtomicSMax_p3i64_i32_i32_i64( (__local ulong*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        return __builtin_spirv_OpAtomicSMax_p1i64_i32_i32_i64( (__global ulong*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

// Atomic UMax


uint __builtin_spirv_OpAtomicUMax_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    uint orig = *Pointer;

    *Pointer = ( orig > Value ) ? orig : Value;

    return orig;
}

uint __builtin_spirv_OpAtomicUMax_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_max_global_u32, uint, Pointer, Scope, Semantics, Value, true );
}

uint __builtin_spirv_OpAtomicUMax_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_max_local_u32, uint, Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicUMax_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
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

ulong __builtin_spirv_OpAtomicUMax_p0i64_i32_i32_i64( volatile __private ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    ulong orig = *Pointer;
    *Pointer = ( orig > Value ) ? orig : Value;
    return orig;
}

ulong __builtin_spirv_OpAtomicUMax_p1i64_i32_i32_i64( volatile __global ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    atomic_operation_1op( __builtin_IB_atomic_max_global_u64, ulong, Pointer, Scope, Semantics, Value, true );
}

ulong __builtin_spirv_OpAtomicUMax_p3i64_i32_i32_i64( volatile __local ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    ulong orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    SPINLOCK_START
    orig = *Pointer;
    *Pointer = ( orig > Value ) ? orig : Value;
    SPINLOCK_END
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

ulong __builtin_spirv_OpAtomicUMax_p4i64_i32_i32_i64( volatile __generic ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        return __builtin_spirv_OpAtomicUMax_p3i64_i32_i32_i64( (__local ulong*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        return __builtin_spirv_OpAtomicUMax_p1i64_i32_i32_i64( (__global ulong*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

// Atomic And


uint __builtin_spirv_OpAtomicAnd_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    uint orig = *Pointer;
    *Pointer &= Value;
    return orig;
}

uint __builtin_spirv_OpAtomicAnd_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_and_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}

uint __builtin_spirv_OpAtomicAnd_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_and_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicAnd_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
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

ulong __builtin_spirv_OpAtomicAnd_p0i64_i32_i32_i64( volatile __private ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    ulong orig = *Pointer;
    *Pointer &= Value;
    return orig;
}

ulong __builtin_spirv_OpAtomicAnd_p1i64_i32_i32_i64( volatile __global ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    atomic_operation_1op( __builtin_IB_atomic_and_global_i64, ulong, (global long*)Pointer, Scope, Semantics, Value, true );
}

ulong __builtin_spirv_OpAtomicAnd_p3i64_i32_i32_i64( volatile __local ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    ulong orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    SPINLOCK_START
    orig = *Pointer;
    *Pointer &= Value;
    SPINLOCK_END
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

ulong __builtin_spirv_OpAtomicAnd_p4i64_i32_i32_i64( volatile __generic ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        return __builtin_spirv_OpAtomicAnd_p3i64_i32_i32_i64( (__local ulong*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        return __builtin_spirv_OpAtomicAnd_p1i64_i32_i32_i64( (__global ulong*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

// Atomic OR


uint __builtin_spirv_OpAtomicOr_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    uint orig = *Pointer;
    *Pointer |= Value;
    return orig;
}

uint __builtin_spirv_OpAtomicOr_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_or_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}

uint __builtin_spirv_OpAtomicOr_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_or_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicOr_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
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

ulong __builtin_spirv_OpAtomicOr_p0i64_i32_i32_i64( volatile __private ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    ulong orig = *Pointer;
    *Pointer |= Value;
    return orig;
}

ulong __builtin_spirv_OpAtomicOr_p1i64_i32_i32_i64( volatile __global ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    atomic_operation_1op( __builtin_IB_atomic_or_global_i64, ulong, (global long*)Pointer, Scope, Semantics, Value, true );
}

ulong __builtin_spirv_OpAtomicOr_p3i64_i32_i32_i64( volatile __local ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
      ulong orig;
      FENCE_PRE_OP(Scope, Semantics, false)
      SPINLOCK_START
      orig = *Pointer;
      *Pointer |= Value;
      SPINLOCK_END
      FENCE_POST_OP(Scope, Semantics, false)
      return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

ulong __builtin_spirv_OpAtomicOr_p4i64_i32_i32_i64( volatile __generic ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
      return __builtin_spirv_OpAtomicOr_p3i64_i32_i32_i64( (__local long*)Pointer, Scope, Semantics, Value );
    }
    else
    {
      return __builtin_spirv_OpAtomicOr_p1i64_i32_i32_i64( (__global long*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)


// Atomic Xor


uint __builtin_spirv_OpAtomicXor_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    uint orig = *Pointer;
    *Pointer ^= Value;
    return orig;
}

uint __builtin_spirv_OpAtomicXor_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xor_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value, true );
}

uint __builtin_spirv_OpAtomicXor_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xor_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value, false );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicXor_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
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

ulong __builtin_spirv_OpAtomicXor_p0i64_i32_i32_i64( volatile __private ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    ulong orig = *Pointer;
    *Pointer ^= Value;
    return orig;
}

ulong __builtin_spirv_OpAtomicXor_p1i64_i32_i32_i64( volatile __global ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xor_global_i64, ulong, (global long*)Pointer, Scope, Semantics, Value, true );
}

ulong __builtin_spirv_OpAtomicXor_p3i64_i32_i32_i64( volatile __local ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    ulong orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    SPINLOCK_START
    orig = *Pointer;
    *Pointer ^= Value;
    SPINLOCK_END
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

ulong __builtin_spirv_OpAtomicXor_p4i64_i32_i32_i64( volatile __generic ulong *Pointer, uint Scope, uint Semantics, ulong Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        return __builtin_spirv_OpAtomicXor_p3i64_i32_i32_i64( (__local long*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        return __builtin_spirv_OpAtomicXor_p1i64_i32_i32_i64( (__global long*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_int64_extended_atomics)

// Atomic FlagTestAndSet


bool __builtin_spirv_OpAtomicFlagTestAndSet_p0i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics )
{
    return (bool)__builtin_spirv_OpAtomicExchange_p0i32_i32_i32_i32( Pointer, Scope, Semantics, ATOMIC_FLAG_TRUE );
}

bool __builtin_spirv_OpAtomicFlagTestAndSet_p1i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics )
{
    return (bool)__builtin_spirv_OpAtomicExchange_p1i32_i32_i32_i32( Pointer, Scope, Semantics, ATOMIC_FLAG_TRUE );
}

bool __builtin_spirv_OpAtomicFlagTestAndSet_p3i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics )
{
    return (bool)__builtin_spirv_OpAtomicExchange_p3i32_i32_i32_i32( Pointer, Scope, Semantics, ATOMIC_FLAG_TRUE );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

bool __builtin_spirv_OpAtomicFlagTestAndSet_p4i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics )
{
    return (bool)__builtin_spirv_OpAtomicExchange_p4i32_i32_i32_i32( Pointer, Scope, Semantics, ATOMIC_FLAG_TRUE );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


// Atomic FlagClear


void __builtin_spirv_OpAtomicFlagClear_p0i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics )
{
    __builtin_spirv_OpAtomicStore_p0i32_i32_i32_i32( Pointer, Scope, Semantics, ATOMIC_FLAG_FALSE );
}

void __builtin_spirv_OpAtomicFlagClear_p1i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics )
{
    __builtin_spirv_OpAtomicStore_p1i32_i32_i32_i32( Pointer, Scope, Semantics, ATOMIC_FLAG_FALSE );
}

void __builtin_spirv_OpAtomicFlagClear_p3i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics )
{
    __builtin_spirv_OpAtomicStore_p3i32_i32_i32_i32( Pointer, Scope, Semantics, ATOMIC_FLAG_FALSE );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void __builtin_spirv_OpAtomicFlagClear_p4i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics )
{
    __builtin_spirv_OpAtomicStore_p4i32_i32_i32_i32( Pointer, Scope, Semantics, ATOMIC_FLAG_FALSE );
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
