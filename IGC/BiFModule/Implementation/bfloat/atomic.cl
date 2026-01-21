/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "spirv_bfloat.h"
#include "include/atomic_fence_impl.h"
#include "../../Languages/OpenCL/IBiF_SPIRV_Utils.cl"

#if defined(cl_intel_bfloat16_atomics)

bfloat __attribute__((overloadable)) __spirv_AtomicLoad( __private bfloat *Pointer, int Scope, int Semantics )
{
    return as_bfloat(__spirv_AtomicLoad( (__private short*)Pointer, Scope, Semantics ));
}

bfloat __attribute__((overloadable)) __spirv_AtomicLoad( __global bfloat *Pointer, int Scope, int Semantics )
{
    return as_bfloat(__spirv_AtomicLoad( (__global short*)Pointer, Scope, Semantics ) );
}

bfloat __attribute__((overloadable)) __spirv_AtomicLoad( __local bfloat *Pointer, int Scope, int Semantics )
{
    return as_bfloat(__spirv_AtomicLoad( (__local short*)Pointer, Scope, Semantics ));
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

bfloat __attribute__((overloadable)) __spirv_AtomicLoad( __generic bfloat *Pointer, int Scope, int Semantics )
{
    return as_bfloat(__spirv_AtomicLoad( (__generic short*)Pointer, Scope, Semantics ));
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void __attribute__((overloadable)) __spirv_AtomicStore( __private bfloat *Pointer, int Scope, int Semantics, bfloat Value )
{
    __spirv_AtomicStore( (__private short*)Pointer, Scope, Semantics, as_short(Value) );
}

void __attribute__((overloadable)) __spirv_AtomicStore( __global bfloat *Pointer, int Scope, int Semantics, bfloat Value )
{
    __spirv_AtomicStore( (__global short*)Pointer, Scope, Semantics, as_short(Value) );
}

void __attribute__((overloadable)) __spirv_AtomicStore( __local bfloat *Pointer, int Scope, int Semantics, bfloat Value )
{
    __spirv_AtomicStore( (__local short*)Pointer, Scope, Semantics, as_short(Value) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

void __attribute__((overloadable)) __spirv_AtomicStore( __generic bfloat *Pointer, int Scope, int Semantics, bfloat Value )
{
    __spirv_AtomicStore( (__generic short*)Pointer, Scope, Semantics, as_short(Value) );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

bfloat __attribute__((overloadable)) __spirv_AtomicExchange( __private bfloat *Pointer, int Scope, int Semantics, bfloat Value )
{
    return as_bfloat(__spirv_AtomicExchange( (__private short*)Pointer, Scope, Semantics, as_short(Value) ));
}

bfloat __attribute__((overloadable)) __spirv_AtomicExchange( __global bfloat *Pointer, int Scope, int Semantics, bfloat Value )
{
    return as_bfloat(__spirv_AtomicExchange( (__global short*)Pointer, Scope, Semantics, as_short(Value) ));
}


bfloat __attribute__((overloadable)) __spirv_AtomicExchange( __local bfloat *Pointer, int Scope, int Semantics, bfloat Value )
{
    return as_bfloat(__spirv_AtomicExchange( (__local short*)Pointer, Scope, Semantics, as_short(Value) ));
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

bfloat __attribute__((overloadable)) __spirv_AtomicExchange( __generic bfloat *Pointer, int Scope, int Semantics, bfloat Value )
{
    return as_bfloat(__spirv_AtomicExchange( (__generic short*)Pointer, Scope, Semantics, as_short(Value) ));
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#define atomic_operation_1op_as_bfloat16( INTRINSIC, TYPE, Pointer, Scope, Semantics, Value, isGlobal )\
{                                                                                             \
    FENCE_PRE_OP((Scope), (Semantics), isGlobal)                                              \
    bfloat result = as_bfloat(INTRINSIC( (Pointer), as_##TYPE(Value) ));                      \
    FENCE_POST_OP((Scope), (Semantics), isGlobal)                                             \
    return result;                                                                            \
}

INLINE bfloat __attribute__((overloadable)) __spirv_AtomicFAddEXT(private bfloat* Pointer, int Scope, int Semantics, bfloat Value)
{
    bfloat orig = *Pointer;
    *Pointer += Value;
    return orig;
}

INLINE bfloat __attribute__((overloadable)) __spirv_AtomicFSubEXT(private bfloat* Pointer, int Scope, int Semantics, bfloat Value)
{
    bfloat orig = *Pointer;
    *Pointer -= Value;
    return orig;
}

INLINE bfloat __attribute__((overloadable)) __spirv_AtomicFMinEXT(private bfloat* Pointer, int Scope, int Semantics, bfloat Value)
{
    bfloat orig = *Pointer;
    *Pointer = (orig < Value) ? orig : Value;
    return orig;
}

INLINE bfloat __attribute__((overloadable)) __spirv_AtomicFMaxEXT(private bfloat* Pointer, int Scope, int Semantics, bfloat Value)
{
    bfloat orig = *Pointer;
    *Pointer = (orig > Value) ? orig : Value;
    return orig;
}

INLINE bfloat __attribute__((overloadable)) __spirv_AtomicFAddEXT( global bfloat* Pointer, int Scope, int Semantics, bfloat Value)
{
    atomic_operation_1op_as_bfloat16( __builtin_IB_atomic_add_global_bf16, short, Pointer, Scope, Semantics, Value, true )
}
INLINE bfloat __attribute__((overloadable)) __spirv_AtomicFSubEXT( global bfloat* Pointer, int Scope, int Semantics, bfloat Value)
{
    atomic_operation_1op_as_bfloat16( __builtin_IB_atomic_sub_global_bf16, short, Pointer, Scope, Semantics, Value, true )
}
INLINE bfloat __attribute__((overloadable)) __spirv_AtomicFMinEXT( global bfloat* Pointer, int Scope, int Semantics, bfloat Value)
{
    atomic_operation_1op_as_bfloat16( __builtin_IB_atomic_min_global_bf16, short, Pointer, Scope, Semantics, Value, true )
}
INLINE bfloat __attribute__((overloadable)) __spirv_AtomicFMaxEXT( global bfloat* Pointer, int Scope, int Semantics, bfloat Value)
{
    atomic_operation_1op_as_bfloat16( __builtin_IB_atomic_max_global_bf16, short, Pointer, Scope, Semantics, Value, true )
}

INLINE bfloat __attribute__((overloadable)) __spirv_AtomicFAddEXT( local bfloat* Pointer, int Scope, int Semantics, bfloat Value)
{
    atomic_operation_1op_as_bfloat16( __builtin_IB_atomic_add_local_bf16, short, Pointer, Scope, Semantics, Value, false )
}
INLINE bfloat __attribute__((overloadable)) __spirv_AtomicFSubEXT( local bfloat* Pointer, int Scope, int Semantics, bfloat Value)
{
    atomic_operation_1op_as_bfloat16( __builtin_IB_atomic_sub_local_bf16, short, Pointer, Scope, Semantics, Value, false )
}
INLINE bfloat __attribute__((overloadable)) __spirv_AtomicFMinEXT( local bfloat* Pointer, int Scope, int Semantics, bfloat Value)
{
    atomic_operation_1op_as_bfloat16( __builtin_IB_atomic_min_local_bf16, short, Pointer, Scope, Semantics, Value, false )
}
INLINE bfloat __attribute__((overloadable)) __spirv_AtomicFMaxEXT( local bfloat* Pointer, int Scope, int Semantics, bfloat Value)
{
    atomic_operation_1op_as_bfloat16( __builtin_IB_atomic_max_local_bf16, short, Pointer, Scope, Semantics, Value, false )
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE bfloat __attribute__((overloadable)) __spirv_AtomicFAddEXT( generic bfloat* Pointer, int Scope, int Semantics, bfloat Value)
{
    __builtin_assume((__local bfloat*)Pointer != 0);
    if (__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFAddEXT((__local bfloat*)Pointer, Scope, Semantics, Value);
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFAddEXT((__private bfloat*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicFAddEXT((__global bfloat*)Pointer, Scope, Semantics, Value);
    }
}

INLINE bfloat __attribute__((overloadable)) __spirv_AtomicFSubEXT( generic bfloat* Pointer, int Scope, int Semantics, bfloat Value)
{
    __builtin_assume((__local bfloat*)Pointer != 0);
    if (__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFSubEXT((__local bfloat*)Pointer, Scope, Semantics, Value);
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFSubEXT((__private bfloat*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicFSubEXT((__global bfloat*)Pointer, Scope, Semantics, Value);
    }
}

INLINE bfloat __attribute__((overloadable)) __spirv_AtomicFMinEXT( generic bfloat* Pointer, int Scope, int Semantics, bfloat Value)
{
    __builtin_assume((__local bfloat*)Pointer != 0);
    if (__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFMinEXT((__local bfloat*)Pointer, Scope, Semantics, Value);
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFMinEXT((__private bfloat*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicFMinEXT((__global bfloat*)Pointer, Scope, Semantics, Value);
    }
}

INLINE bfloat __attribute__((overloadable)) __spirv_AtomicFMaxEXT( generic bfloat* Pointer, int Scope, int Semantics, bfloat Value)
{
    __builtin_assume((__local bfloat*)Pointer != 0);
    if (__spirv_GenericCastToPtrExplicit_ToLocal(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFMaxEXT((__local bfloat*)Pointer, Scope, Semantics, Value);
    }
    else if (__spirv_GenericCastToPtrExplicit_ToPrivate(__builtin_astype((Pointer), __generic char*), StorageWorkgroup))
    {
        return __spirv_AtomicFMaxEXT((__private bfloat*)Pointer, Scope, Semantics, Value);
    }
    else
    {
        return __spirv_AtomicFMaxEXT((__global bfloat*)Pointer, Scope, Semantics, Value);
    }
}
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

typedef ushort atomic_ushort;

#define ATOMIC_FETCH_AS_BFLOAT16_FUNCTION_ADDRSPACE(KEY, OCL_TYPE, OPCODE, ADDRSPACE, ABBR_ADDRSPACE) \
INLINE OCL_TYPE OVERLOADABLE intel_atomic_fetch_##KEY##_as_bfloat16_explicit(volatile ADDRSPACE atomic_##OCL_TYPE *object, OCL_TYPE operand, memory_order order, memory_scope scope) \
{ \
  return as_##OCL_TYPE(__spirv_Atomic##OPCODE( \
            (ADDRSPACE bfloat *)object,                                                  \
            get_spirv_mem_scope(scope),                                                  \
            get_spirv_mem_order(order) |                                                 \
                get_spirv_mem_fence(get_fence((const ADDRSPACE void *)object)),          \
            as_bfloat(operand)));                                                        \
} \
INLINE OCL_TYPE OVERLOADABLE intel_atomic_fetch_##KEY##_as_bfloat16_explicit(volatile ADDRSPACE atomic_##OCL_TYPE *object, OCL_TYPE operand, memory_order order) \
{ \
  return intel_atomic_fetch_##KEY##_as_bfloat16_explicit(object, operand, order, memory_scope_device); \
} \
INLINE OCL_TYPE OVERLOADABLE intel_atomic_fetch_##KEY##_as_bfloat16(volatile ADDRSPACE atomic_##OCL_TYPE *object, OCL_TYPE operand) \
{ \
  return intel_atomic_fetch_##KEY##_as_bfloat16_explicit(object, operand, memory_order_seq_cst); \
}

ATOMIC_FETCH_AS_BFLOAT16_FUNCTION_ADDRSPACE(add, ushort, FAddEXT, global, p1)
ATOMIC_FETCH_AS_BFLOAT16_FUNCTION_ADDRSPACE(sub, ushort, FSubEXT, global, p1)
ATOMIC_FETCH_AS_BFLOAT16_FUNCTION_ADDRSPACE(max, ushort, FMaxEXT, global, p1)
ATOMIC_FETCH_AS_BFLOAT16_FUNCTION_ADDRSPACE(min, ushort, FMinEXT, global, p1)

ATOMIC_FETCH_AS_BFLOAT16_FUNCTION_ADDRSPACE(add, ushort, FAddEXT, local, p3)
ATOMIC_FETCH_AS_BFLOAT16_FUNCTION_ADDRSPACE(sub, ushort, FSubEXT, local, p3)
ATOMIC_FETCH_AS_BFLOAT16_FUNCTION_ADDRSPACE(max, ushort, FMaxEXT, local, p3)
ATOMIC_FETCH_AS_BFLOAT16_FUNCTION_ADDRSPACE(min, ushort, FMinEXT, local, p3)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
ATOMIC_FETCH_AS_BFLOAT16_FUNCTION_ADDRSPACE(add, ushort, FAddEXT, generic, p4)
ATOMIC_FETCH_AS_BFLOAT16_FUNCTION_ADDRSPACE(sub, ushort, FSubEXT, generic, p4)
ATOMIC_FETCH_AS_BFLOAT16_FUNCTION_ADDRSPACE(max, ushort, FMaxEXT, generic, p4)
ATOMIC_FETCH_AS_BFLOAT16_FUNCTION_ADDRSPACE(min, ushort, FMinEXT, generic, p4)
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_intel_bfloat16_atomics)
