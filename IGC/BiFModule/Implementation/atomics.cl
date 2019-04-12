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

#define SEMANTICS_POST_OP_NEEDS_FENCE ( 0 )

// This fencing scheme allows us to obey the memory model when coherency is
// enabled or disabled.  Because the L3$ has 2 pipelines (cohereny&atomics and 
// non-coherant) the fences guarentee the memory model is followed when coherency
// is disabled.  
//
// When coherency is enabled, though, all HDC traffic uses the same L3$ pipe so
// these fences would not be needed.  The compiler is agnostic to coherency 
// being enabled or disbled so we asume the worst case.


#define atomic_operation_1op( INTRINSIC, TYPE, Pointer, Scope, Semantics, Value )   \
{                                                                                   \
    if( ( (Semantics) & ( SEMANTICS_PRE_OP_NEED_FENCE ) ) > 0 )                     \
    {                                                                               \
        __builtin_spirv_OpMemoryBarrier_i32_i32( (Scope), (Semantics) );            \
    }                                                                               \
                                                                                    \
    TYPE result = INTRINSIC( (Pointer), (Value) );                                  \
                                                                                    \
    if( ( (Semantics) & ( SEMANTICS_POST_OP_NEEDS_FENCE ) ) > 0 )                   \
    {                                                                               \
        __builtin_spirv_OpMemoryBarrier_i32_i32( (Scope), (Semantics) );            \
    }                                                                               \
                                                                                    \
    return result;                                                                  \
}

#define atomic_operation_1op_as_float( INTRINSIC, TYPE, Pointer, Scope, Semantics, Value )\
{                                                                                   \
    if( ( (Semantics) & ( SEMANTICS_PRE_OP_NEED_FENCE ) ) > 0 )                     \
    {                                                                               \
        __builtin_spirv_OpMemoryBarrier_i32_i32( (Scope), (Semantics) );            \
    }                                                                               \
                                                                                    \
    TYPE result = as_float(INTRINSIC( (Pointer), (Value) ));                        \
                                                                                    \
    if( ( (Semantics) & ( SEMANTICS_POST_OP_NEEDS_FENCE ) ) > 0 )                   \
    {                                                                               \
        __builtin_spirv_OpMemoryBarrier_i32_i32( (Scope), (Semantics) );            \
    }                                                                               \
                                                                                    \
    return result;                                                                  \
}

#define atomic_operation_0op( INTRINSIC, TYPE, Pointer, Scope, Semantics )          \
{                                                                                   \
    if( ( (Semantics) & ( SEMANTICS_PRE_OP_NEED_FENCE ) ) > 0 )                     \
    {                                                                               \
        __builtin_spirv_OpMemoryBarrier_i32_i32( (Scope), (Semantics) );            \
    }                                                                               \
                                                                                    \
    TYPE result = INTRINSIC( (Pointer) );                                           \
                                                                                    \
    if( ( (Semantics) & ( SEMANTICS_POST_OP_NEEDS_FENCE ) ) > 0 )                   \
    {                                                                               \
        __builtin_spirv_OpMemoryBarrier_i32_i32( (Scope), (Semantics) );            \
    }                                                                               \
                                                                                    \
    return result;                                                                  \
}

#define atomic_cmpxhg( INTRINSIC, TYPE, Pointer, Scope, Semantics, Value, Comp )\
{                                                                               \
    if( ( (Semantics) & ( SEMANTICS_PRE_OP_NEED_FENCE ) ) > 0 )                 \
    {                                                                           \
        __builtin_spirv_OpMemoryBarrier_i32_i32( (Scope), (Semantics) );        \
    }                                                                           \
                                                                                \
    TYPE result = INTRINSIC( (Pointer), (Comp), (Value) );                      \
                                                                                \
    if( ( (Semantics) & ( SEMANTICS_POST_OP_NEEDS_FENCE ) ) > 0 )               \
    {                                                                           \
        __builtin_spirv_OpMemoryBarrier_i32_i32( (Scope), (Semantics) );        \
    }                                                                           \
                                                                                \
    return result;                                                              \
}

#define atomic_cmpxhg_as_float( INTRINSIC, TYPE, Pointer, Scope, Semantics, Value, Comp )\
{                                                                               \
    if( ( (Semantics) & ( SEMANTICS_PRE_OP_NEED_FENCE ) ) > 0 )                 \
    {                                                                           \
        __builtin_spirv_OpMemoryBarrier_i32_i32( (Scope), (Semantics) );        \
    }                                                                           \
                                                                                \
    TYPE result = as_float(INTRINSIC( (Pointer), (Comp), (Value) ));                      \
                                                                                \
    if( ( (Semantics) & ( SEMANTICS_POST_OP_NEEDS_FENCE ) ) > 0 )               \
    {                                                                           \
        __builtin_spirv_OpMemoryBarrier_i32_i32( (Scope), (Semantics) );        \
    }                                                                           \
                                                                                \
    return result;                                                              \
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


// Atomic Exchange


uint __builtin_spirv_OpAtomicExchange_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    uint orig = *Pointer;

    *Pointer = Value;

    return orig;
}


uint __builtin_spirv_OpAtomicExchange_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xchg_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value );
}


uint __builtin_spirv_OpAtomicExchange_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xchg_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicExchange_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_xchg_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_xchg_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value );
    }

}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float __builtin_spirv_OpAtomicExchange_p0f32_i32_i32_f32( volatile __private float *Pointer, uint Scope, uint Semantics, float Value)
{
    float orig = *Pointer;

    *Pointer = Value;

    return orig;
}

float __builtin_spirv_OpAtomicExchange_p1f32_i32_i32_f32( volatile __global float *Pointer, uint Scope, uint Semantics, float Value)
{
    atomic_operation_1op_as_float( __builtin_IB_atomic_xchg_global_i32, float, (global int*)Pointer, Scope, Semantics, as_int(Value) );
}


float __builtin_spirv_OpAtomicExchange_p3f32_i32_i32_f32( volatile __local float *Pointer, uint Scope, uint Semantics, float Value)
{
    atomic_operation_1op_as_float( __builtin_IB_atomic_xchg_local_i32, float, (local int*)Pointer, Scope, Semantics, as_int(Value) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float __builtin_spirv_OpAtomicExchange_p4f32_i32_i32_f32( volatile __generic float *Pointer, uint Scope, uint Semantics, float Value)
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        atomic_operation_1op_as_float( __builtin_IB_atomic_xchg_local_i32, float, (local int*)Pointer, Scope, Semantics, as_int(Value) );
    }
    else
    {
        atomic_operation_1op_as_float( __builtin_IB_atomic_xchg_global_i32, float, (global int*)Pointer, Scope, Semantics, as_int(Value) );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


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
    atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_global_i32, uint, (global int*)Pointer, Scope, Equal, Value, Comparator );
}


uint __builtin_spirv_OpAtomicCompareExchange_p3i32_i32_i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Equal, uint Unequal, uint Value, uint Comparator)
{
    atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_local_i32, uint, (local int*)Pointer, Scope, Equal, Value, Comparator );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicCompareExchange_p4i32_i32_i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Equal, uint Unequal, uint Value, uint Comparator)
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_local_i32, uint, (__local int*)Pointer, Scope, Equal, Value, Comparator );
    }
    else
    {
        atomic_cmpxhg( __builtin_IB_atomic_cmpxchg_global_i32, uint, (__global int*)Pointer, Scope, Equal, Value, Comparator );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

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
    atomic_cmpxhg_as_float( __builtin_IB_atomic_cmpxchg_global_i32, float, (global int*)Pointer, Scope, Equal, as_uint(Value), as_uint(Comparator) );
}


float __builtin_spirv_OpAtomicCompareExchange_p3f32_i32_i32_i32_f32_f32( volatile __local float *Pointer, uint Scope, uint Equal, uint Unequal, float Value, float Comparator)
{
    atomic_cmpxhg_as_float( __builtin_IB_atomic_cmpxchg_local_i32, float, (local int*)Pointer, Scope, Equal, as_uint(Value), as_uint(Comparator) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float __builtin_spirv_OpAtomicCompareExchange_p4f32_i32_i32_i32_f32_f32( volatile __generic float *Pointer, uint Scope, uint Equal, uint Unequal, float Value, float Comparator)
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        atomic_cmpxhg_as_float( __builtin_IB_atomic_cmpxchg_local_i32, float, (__local int*)Pointer, Scope, Equal, as_uint(Value), as_uint(Comparator) );
    }
    else
    {
        atomic_cmpxhg_as_float( __builtin_IB_atomic_cmpxchg_global_i32, float, (__global int*)Pointer, Scope, Equal, as_uint(Value), as_uint(Comparator) );
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


// Atomic Increment


uint __builtin_spirv_OpAtomicIIncrement_p0i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics )
{
    uint orig = *Pointer;

    *Pointer += 1;

    return orig;
}


uint __builtin_spirv_OpAtomicIIncrement_p1i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_inc_global_i32, uint, (global int*)Pointer, Scope, Semantics );
}


uint __builtin_spirv_OpAtomicIIncrement_p3i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_inc_local_i32, uint, (local int*)Pointer, Scope, Semantics );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicIIncrement_p4i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        atomic_operation_0op( __builtin_IB_atomic_inc_local_i32, uint, (__local int*)Pointer, Scope, Semantics );
    }
    else
    {
        atomic_operation_0op( __builtin_IB_atomic_inc_global_i32, uint, (__global int*)Pointer, Scope, Semantics );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


// Atomic Decrement


uint __builtin_spirv_OpAtomicIDecrement_p0i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics )
{
    uint orig = *Pointer;

    *Pointer -= 1;

    return orig;
}

uint __builtin_spirv_OpAtomicIDecrement_p1i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_dec_global_i32, uint, (global int*)Pointer, Scope, Semantics );
}

uint __builtin_spirv_OpAtomicIDecrement_p3i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics )
{
    atomic_operation_0op( __builtin_IB_atomic_dec_local_i32, uint, (local int*)Pointer, Scope, Semantics );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicIDecrement_p4i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        atomic_operation_0op( __builtin_IB_atomic_dec_local_i32, uint, (__local int*)Pointer, Scope, Semantics );
    }
    else
    {
        atomic_operation_0op( __builtin_IB_atomic_dec_global_i32, uint, (__global int*)Pointer, Scope, Semantics );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


// Atomic IAdd


uint __builtin_spirv_OpAtomicIAdd_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    uint orig = *Pointer;

    *Pointer += Value;

    return orig;
}


uint __builtin_spirv_OpAtomicIAdd_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_add_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value );
}

uint __builtin_spirv_OpAtomicIAdd_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_add_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicIAdd_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_add_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_add_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


// Atomic ISub


uint __builtin_spirv_OpAtomicISub_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    uint orig = *Pointer;

    *Pointer -= Value;

    return orig;
}


uint __builtin_spirv_OpAtomicISub_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_add_global_i32, uint, (global int*)Pointer, Scope, Semantics, -as_int(Value) );
}


uint __builtin_spirv_OpAtomicISub_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_add_local_i32, uint, (local int*)Pointer, Scope, Semantics, -as_int(Value) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicISub_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_add_local_i32, uint, (__local int*)Pointer, Scope, Semantics, -as_int(Value) );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_add_global_i32, uint, (__global int*)Pointer, Scope, Semantics, -as_int(Value) );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


// Atomic SMin


int __builtin_spirv_OpAtomicSMin_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, int Value)
{
    int orig = *Pointer;

    *Pointer = ( orig < Value ) ? orig : Value;

    return orig;
}

int __builtin_spirv_OpAtomicSMin_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, int Value)
{
    atomic_operation_1op( __builtin_IB_atomic_min_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value );
}

int __builtin_spirv_OpAtomicSMin_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, int Value)
{
    atomic_operation_1op( __builtin_IB_atomic_min_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int __builtin_spirv_OpAtomicSMin_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, int Value)
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_min_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_min_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicUMin_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    uint orig = *Pointer;

    *Pointer = ( orig < Value ) ? orig : Value;

    return orig;
}

uint __builtin_spirv_OpAtomicUMin_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_min_global_u32, uint, Pointer, Scope, Semantics, Value );
}

uint __builtin_spirv_OpAtomicUMin_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_min_local_u32, uint, Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicUMin_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_min_local_u32, uint, (__local uint*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_min_global_u32, uint, (__global uint*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


// Atomic SMax


int __builtin_spirv_OpAtomicSMax_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, int Value)
{
    int orig = *Pointer;

    *Pointer = ( orig > Value ) ? orig : Value;

    return orig;
}

int __builtin_spirv_OpAtomicSMax_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, int Value)
{
    atomic_operation_1op( __builtin_IB_atomic_max_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value );
}

int __builtin_spirv_OpAtomicSMax_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, int Value)
{
    atomic_operation_1op( __builtin_IB_atomic_max_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int __builtin_spirv_OpAtomicSMax_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, int Value)
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_max_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_max_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


// Atomic UMax


uint __builtin_spirv_OpAtomicUMax_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    uint orig = *Pointer;

    *Pointer = ( orig > Value ) ? orig : Value;

    return orig;
}

uint __builtin_spirv_OpAtomicUMax_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_max_global_u32, uint, Pointer, Scope, Semantics, Value );
}

uint __builtin_spirv_OpAtomicUMax_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_max_local_u32, uint, Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicUMax_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_max_local_u32, uint, (__local uint*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_max_global_u32, uint, (__global uint*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


// Atomic And


uint __builtin_spirv_OpAtomicAnd_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    uint orig = *Pointer;

    *Pointer &= Value;

    return orig;
}

uint __builtin_spirv_OpAtomicAnd_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_and_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value );
}

uint __builtin_spirv_OpAtomicAnd_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_and_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicAnd_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_and_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_and_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


// Atomic OR


uint __builtin_spirv_OpAtomicOr_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    uint orig = *Pointer;

    *Pointer |= Value;

    return orig;
}

uint __builtin_spirv_OpAtomicOr_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_or_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value );
}

uint __builtin_spirv_OpAtomicOr_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_or_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicOr_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_or_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_or_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


// Atomic Xor


uint __builtin_spirv_OpAtomicXor_p0i32_i32_i32_i32( volatile __private uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    uint orig = *Pointer;

    *Pointer ^= Value;

    return orig;
}

uint __builtin_spirv_OpAtomicXor_p1i32_i32_i32_i32( volatile __global uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xor_global_i32, uint, (global int*)Pointer, Scope, Semantics, Value );
}

uint __builtin_spirv_OpAtomicXor_p3i32_i32_i32_i32( volatile __local uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    atomic_operation_1op( __builtin_IB_atomic_xor_local_i32, uint, (local int*)Pointer, Scope, Semantics, Value );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

uint __builtin_spirv_OpAtomicXor_p4i32_i32_i32_i32( volatile __generic uint *Pointer, uint Scope, uint Semantics, uint Value )
{
    if(__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        atomic_operation_1op( __builtin_IB_atomic_xor_local_i32, uint, (__local int*)Pointer, Scope, Semantics, Value );
    }
    else
    {
        atomic_operation_1op( __builtin_IB_atomic_xor_global_i32, uint, (__global int*)Pointer, Scope, Semantics, Value );
    }
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


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

#undef SEMANTICS_NEED_FENCE

#undef atomic_operation_1op
#undef atomic_operation_0op
#undef atomic_cmpxhg
