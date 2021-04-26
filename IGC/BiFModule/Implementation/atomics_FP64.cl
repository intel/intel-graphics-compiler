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


  __local uint* __builtin_IB_get_local_lock();
  __global uint* __builtin_IB_get_global_lock();
   void __intel_memfence_handler(bool flushRW, bool isGlobal, bool invalidateL1);

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
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
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

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p0f64_i32_i32_f64, )( __private double *Pointer, int Scope, int Semantics, double Value)
{
    double orig = *Pointer;
    *Pointer += Value;
    return orig;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p1f64_i32_i32_f64, )( __global double *Pointer, int Scope, int Semantics, double Value)
{
    double orig;
    FENCE_PRE_OP(Scope, Semantics, true)
    SPINLOCK_START(global)
    orig = *Pointer;
    *Pointer = orig + Value;
    SPINLOCK_END(global)
    FENCE_POST_OP(Scope, Semantics, true)
    return orig;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p3f64_i32_i32_f64, )( __local double *Pointer, int Scope, int Semantics, double Value)
{
    double orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    SPINLOCK_START(local)
    orig = *Pointer;
    *Pointer = orig + Value;
    SPINLOCK_END(local)
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p4f64_i32_i32_f64, )( __generic double *Pointer, int Scope, int Semantics, double Value)
{
    if(SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicFAddEXT, _p3f64_i32_i32_f64, )((local double*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return SPIRV_BUILTIN(AtomicFAddEXT, _p1f64_i32_i32_f64, )((global double*)Pointer, Scope, Semantics, Value);
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
    double orig;
    FENCE_PRE_OP(Scope, Semantics, true)
    SPINLOCK_START(global)
    orig = *Pointer;
    *Pointer = (orig < Value) ? orig : Value;
    SPINLOCK_END(global)
    FENCE_POST_OP(Scope, Semantics, true)
    return orig;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p3f64_i32_i32_f64, )( local double* Pointer, int Scope, int Semantics, double Value)
{
    double orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    SPINLOCK_START(local)
    orig = *Pointer;
    *Pointer = (orig < Value) ? orig : Value;
    SPINLOCK_END(local)
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p4f64_i32_i32_f64, )( generic double* Pointer, int Scope, int Semantics, double Value)
{
    if (SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicFMinEXT, _p3f64_i32_i32_f64, )((__local double*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return SPIRV_BUILTIN(AtomicFMinEXT, _p1f64_i32_i32_f64, )((__global double*)Pointer, Scope, Semantics, Value);
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
    double orig;
    FENCE_PRE_OP(Scope, Semantics, true)
    SPINLOCK_START(global)
    orig = *Pointer;
    *Pointer = (orig > Value) ? orig : Value;
    SPINLOCK_END(global)
    FENCE_POST_OP(Scope, Semantics, true)
    return orig;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p3f64_i32_i32_f64, )( local double* Pointer, int Scope, int Semantics, double Value)
{
    double orig;
    FENCE_PRE_OP(Scope, Semantics, false)
    SPINLOCK_START(local)
    orig = *Pointer;
    *Pointer = (orig > Value) ? orig : Value;
    SPINLOCK_END(local)
    FENCE_POST_OP(Scope, Semantics, false)
    return orig;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p4f64_i32_i32_f64, )( generic double* Pointer, int Scope, int Semantics, double Value)
{
    if (SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(__builtin_astype((Pointer), __generic void*), StorageWorkgroup))
    {
        return SPIRV_BUILTIN(AtomicFMaxEXT, _p3f64_i32_i32_f64, )((__local double*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return SPIRV_BUILTIN(AtomicFMaxEXT, _p1f64_i32_i32_f64, )((__global double*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
