/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __SPIRV_BFLOAT_H__
#define __SPIRV_BFLOAT_H__

#include "spirv.h"

//
// BFloat16 Type Definitions and Function Declarations
//

// Type definitions
typedef __bf16 bfloat;
typedef bfloat bfloat2 __attribute__((ext_vector_type(2)));
typedef bfloat bfloat3 __attribute__((ext_vector_type(3)));
typedef bfloat bfloat4 __attribute__((ext_vector_type(4)));
typedef bfloat bfloat8 __attribute__((ext_vector_type(8)));
typedef bfloat bfloat16 __attribute__((ext_vector_type(16)));
// Geometric - Dot Product

// Geometric - Dot Product
bfloat __attribute__((overloadable)) __spirv_Dot(bfloat2 Vector1, bfloat2 Vector2);
bfloat __attribute__((overloadable)) __spirv_Dot(bfloat3 Vector1, bfloat3 Vector2);
bfloat __attribute__((overloadable)) __spirv_Dot(bfloat4 Vector1, bfloat4 Vector2);
bfloat __attribute__((overloadable)) __spirv_Dot(bfloat8 Vector1, bfloat8 Vector2);
bfloat __attribute__((overloadable)) __spirv_Dot(bfloat16 Vector1, bfloat16 Vector2);

// Classification - IsNan

// Classification - IsNan
bool __attribute__((overloadable))     __spirv_IsNan(bfloat x);
__bool2 __attribute__((overloadable))  __spirv_IsNan(bfloat2 x);
__bool3 __attribute__((overloadable))  __spirv_IsNan(bfloat3 x);
__bool4 __attribute__((overloadable))  __spirv_IsNan(bfloat4 x);
__bool8 __attribute__((overloadable))  __spirv_IsNan(bfloat8 x);
__bool16 __attribute__((overloadable)) __spirv_IsNan(bfloat16 x);

// Classification - IsInf
bool __attribute__((overloadable))     __spirv_IsInf(bfloat x);
__bool2 __attribute__((overloadable))  __spirv_IsInf(bfloat2 x);
__bool3 __attribute__((overloadable))  __spirv_IsInf(bfloat3 x);
__bool4 __attribute__((overloadable))  __spirv_IsInf(bfloat4 x);
__bool8 __attribute__((overloadable))  __spirv_IsInf(bfloat8 x);
__bool16 __attribute__((overloadable)) __spirv_IsInf(bfloat16 x);

// Classification - IsFinite
bool __attribute__((overloadable))     __spirv_IsFinite(bfloat x);
__bool2 __attribute__((overloadable))  __spirv_IsFinite(bfloat2 x);
__bool3 __attribute__((overloadable))  __spirv_IsFinite(bfloat3 x);
__bool4 __attribute__((overloadable))  __spirv_IsFinite(bfloat4 x);
__bool8 __attribute__((overloadable))  __spirv_IsFinite(bfloat8 x);
__bool16 __attribute__((overloadable)) __spirv_IsFinite(bfloat16 x);

// Classification - IsNormal
bool __attribute__((overloadable))     __spirv_IsNormal(bfloat x);
__bool2 __attribute__((overloadable))  __spirv_IsNormal(bfloat2 x);
__bool3 __attribute__((overloadable))  __spirv_IsNormal(bfloat3 x);
__bool4 __attribute__((overloadable))  __spirv_IsNormal(bfloat4 x);
__bool8 __attribute__((overloadable))  __spirv_IsNormal(bfloat8 x);
__bool16 __attribute__((overloadable)) __spirv_IsNormal(bfloat16 x);

// Classification - SignBitSet
bool __attribute__((overloadable))     __spirv_SignBitSet(bfloat x);
__bool2 __attribute__((overloadable))  __spirv_SignBitSet(bfloat2 x);
__bool3 __attribute__((overloadable))  __spirv_SignBitSet(bfloat3 x);
__bool4 __attribute__((overloadable))  __spirv_SignBitSet(bfloat4 x);
__bool8 __attribute__((overloadable))  __spirv_SignBitSet(bfloat8 x);
__bool16 __attribute__((overloadable)) __spirv_SignBitSet(bfloat16 x);

// Relational - LessOrGreater
bool __attribute__((overloadable))     __spirv_LessOrGreater(bfloat x, bfloat y);
__bool2 __attribute__((overloadable))  __spirv_LessOrGreater(bfloat2 x, bfloat2 y);
__bool3 __attribute__((overloadable))  __spirv_LessOrGreater(bfloat3 x, bfloat3 y);
__bool4 __attribute__((overloadable))  __spirv_LessOrGreater(bfloat4 x, bfloat4 y);
__bool8 __attribute__((overloadable))  __spirv_LessOrGreater(bfloat8 x, bfloat8 y);
__bool16 __attribute__((overloadable)) __spirv_LessOrGreater(bfloat16 x, bfloat16 y);

#if defined(cl_intel_bfloat16_atomics)

// Atomic Operations - Floating-Point Add
bfloat __attribute__((overloadable))
__spirv_AtomicFAddEXT(private bfloat *Pointer, int Scope, int Semantics, bfloat Value);
bfloat __attribute__((overloadable))
__spirv_AtomicFAddEXT(global bfloat *Pointer, int Scope, int Semantics, bfloat Value);
bfloat __attribute__((overloadable))
__spirv_AtomicFAddEXT(local bfloat *Pointer, int Scope, int Semantics, bfloat Value);

// Atomic Operations - Floating-Point Subtract
bfloat __attribute__((overloadable))
__spirv_AtomicFSubEXT(private bfloat *Pointer, int Scope, int Semantics, bfloat Value);
bfloat __attribute__((overloadable))
__spirv_AtomicFSubEXT(global bfloat *Pointer, int Scope, int Semantics, bfloat Value);
bfloat __attribute__((overloadable))
__spirv_AtomicFSubEXT(local bfloat *Pointer, int Scope, int Semantics, bfloat Value);

// Atomic Operations - Floating-Point Min
bfloat __attribute__((overloadable))
__spirv_AtomicFMinEXT(private bfloat *Pointer, int Scope, int Semantics, bfloat Value);
bfloat __attribute__((overloadable))
__spirv_AtomicFMinEXT(global bfloat *Pointer, int Scope, int Semantics, bfloat Value);
bfloat __attribute__((overloadable))
__spirv_AtomicFMinEXT(local bfloat *Pointer, int Scope, int Semantics, bfloat Value);

// Atomic Operations - Floating-Point Max
bfloat __attribute__((overloadable))
__spirv_AtomicFMaxEXT(private bfloat *Pointer, int Scope, int Semantics, bfloat Value);
bfloat __attribute__((overloadable))
__spirv_AtomicFMaxEXT(global bfloat *Pointer, int Scope, int Semantics, bfloat Value);
bfloat __attribute__((overloadable))
__spirv_AtomicFMaxEXT(local bfloat *Pointer, int Scope, int Semantics, bfloat Value);

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
bfloat __attribute__((overloadable))
__spirv_AtomicFAddEXT(generic bfloat *Pointer, int Scope, int Semantics, bfloat Value);
bfloat __attribute__((overloadable))
__spirv_AtomicFSubEXT(generic bfloat *Pointer, int Scope, int Semantics, bfloat Value);
bfloat __attribute__((overloadable))
__spirv_AtomicFMinEXT(generic bfloat *Pointer, int Scope, int Semantics, bfloat Value);
bfloat __attribute__((overloadable))
__spirv_AtomicFMaxEXT(generic bfloat *Pointer, int Scope, int Semantics, bfloat Value);
#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_intel_bfloat16_atomics)

#endif // __SPIRV_BFLOAT_H__
