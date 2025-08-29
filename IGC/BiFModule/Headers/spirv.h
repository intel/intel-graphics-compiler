/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __SPIRV_H__
#define __SPIRV_H__

#include "spirv_macros.h"
#include "spirv_types.h"

/******
 *
 * Mangling Scheme:
 *
 * Type      ::= Scalar | Vector | Pointer | Struct | Block | Opaque | Array
 *
 * Scalar    ::= Integer | Float | Void
 * Integer   ::= i1 | i8 | i16 | i32 | i64
 * Float     ::= f16 | f32 | f64
 * Void      ::= 'v'
 * Opaque    ::= i64 // All opaque types are currently represented as i64.
 * Array     ::= 'a' + [0-9]+ + Type
 *
 * Vector    ::= 'v' + Len + Scalar
 * Len       ::= 2 | 3 | 4 | 8 | 16
 *
 * Pointer   ::= 'p' + Addrspace + Type
 * Addrspace ::= 0 | 1 | 2 | 3 | 4
 *
 * Struct    ::= 's' // We can make this more specific if necessary
 *
 * Block     ::= 'fp' + Addrspace + Type + Type* + VarArg? // (return type + arg types + possible varargs '...')
 * VarArg    ::= 'x'
 *
 * All function args are mangled and the mangle for the return type is only added if necessary
 * to distinguish between overloads.
 *
 ******/

typedef enum
{
    None        = 0x0,
    CmdExecTime = 0x1
} KernelProfilingInfo_t;

#include "spirv_atomics_common.h"

typedef enum
{
    StorageUniformConstant = 0,
    StorageWorkgroup       = 4,
    StorageCrossWorkgroup  = 5,
    StorageFunction        = 7,
    StorageGeneric         = 8
} StorageClass_t;

typedef enum
{
    GroupOperationReduce          = 0,
    GroupOperationInclusiveScan   = 1,
    GroupOperationExclusiveScan   = 2,
    GroupOperationClusteredReduce = 3
} GroupOperations_t;

typedef enum
{
    rte = 0,
    rtz = 1,
    rtp = 2,
    rtn = 3
};
typedef int RoundingMode_t;

// Note: Unify these defines in a common header that this and
// the cth include.

#if __32bit__ > 0
typedef uint  size_t;
typedef uint3 size_t3;
typedef int   ptrdiff_t;
#else
typedef ulong  size_t;
typedef ulong3 size_t3;
typedef long   ptrdiff_t;
#endif

typedef ptrdiff_t intptr_t;
typedef size_t    uintptr_t;

#define NULL 0

#define HALF_DIG 3
#define HALF_MANT_DIG 11
#define HALF_MAX_10_EXP +4
#define HALF_MAX_EXP +16
#define HALF_MIN_10_EXP -4
#define HALF_MIN_EXP -13
#define HALF_RADIX 2
#define HALF_MAX ((0x1.ffcp15h))
#define HALF_MIN ((0x1.0p-14h))
#define HALF_EPSILON ((0x1.0p-10h))
#define HALF_MAX_SQRT ((0x1.0p+8h))
#define HALF_MIN_SQRT ((0x1.0p-8h))
#define M_E_H 2.71828182845904523536028747135266250h
#define M_LOG2E_H 1.44269504088896340735992468100189214h
#define M_LOG10E_H 0.434294481903251827651128918916605082h
#define M_LN2_H 0.693147180559945309417232121458176568h
#define M_LN10_H 2.30258509299404568401799145468436421h
#define M_PI_H 3.14159265358979323846264338327950288h
#define M_PI_2_H 1.57079632679489661923132169163975144h
#define M_PI_4_H 0.785398163397448309615660845819875721h
#define M_1_PI_H 0.318309886183790671537767526745028724h
#define M_2_PI_H 0.636619772367581343075535053490057448h
#define M_2_SQRTPI_H 1.12837916709551257389615890312154517h
#define M_SQRT2_H 1.41421356237309504880168872420969808h
#define M_SQRT1_2_H 0.707106781186547524400844362104849039h

#define DBL_DIG 15
#define DBL_MANT_DIG 53
#define DBL_MAX_10_EXP +308
#define DBL_MAX_EXP +1024
#define DBL_MIN_10_EXP -307
#define DBL_MIN_EXP -1021
#define DBL_RADIX 2
#define DBL_MAX 0x1.fffffffffffffp1023
#define DBL_MIN 0x1.0p-1022
#define DBL_EPSILON 0x1.0p-52

#define M_E 0x1.5bf0a8b145769p+1
#define M_LOG2E 0x1.71547652b82fep+0
#define M_LOG10E 0x1.bcb7b1526e50ep-2
#define M_LN2 0x1.62e42fefa39efp-1
#define M_LN10 0x1.26bb1bbb55516p+1
#define M_PI 0x1.921fb54442d18p+1
#define M_PI_2 0x1.921fb54442d18p+0
#define M_PI_4 0x1.921fb54442d18p-1
#define M_1_PI 0x1.45f306dc9c883p-2
#define M_2_PI 0x1.45f306dc9c883p-1
#define M_2_SQRTPI 0x1.20dd750429b6dp+0
#define M_SQRT2 0x1.6a09e667f3bcdp+0
#define M_SQRT1_2 0x1.6a09e667f3bcdp-1

#if defined(cl_khr_fp64)
#define cl_fp64_basic_ops
#endif // cl_fp64_basic_ops

typedef char __bool2 __attribute__((ext_vector_type(2)));
typedef char __bool3 __attribute__((ext_vector_type(3)));
typedef char __bool4 __attribute__((ext_vector_type(4)));
typedef char __bool8 __attribute__((ext_vector_type(8)));
typedef char __bool16 __attribute__((ext_vector_type(16)));

#define INTEL_PIPE_RESERVE_ID_VALID_BIT (1U << 30)
#define CLK_NULL_RESERVE_ID \
    (__builtin_astype(((void *)(~INTEL_PIPE_RESERVE_ID_VALID_BIT)), __spirv_ReserveId))

#define IMAGETYPE_SAMPLED_SHIFT 62
#define IMAGETYPE_DIM_SHIFT 59
#define IMAGETYPE_DEPTH_SHIFT 58
#define IMAGETYPE_ARRAYED_SHIFT 57
#define IMAGETYPE_MULTISAMPLED_SHIFT 56
#define IMAGETYPE_ACCESSQUALIFER_SHIFT 54

// Include bif control flag to have posibilty on shape of
// built-in instructions during compilation of user shaders
#include "bif_control.h"

// Keep track of SaturatedConversion

// Work-item functions

size_t __attribute__((overloadable)) __spirv_BuiltInNumWorkgroups(int dimindx);
size_t __attribute__((overloadable)) __spirv_BuiltInWorkgroupSize(int dimindx);
size_t __attribute__((overloadable)) __spirv_BuiltInWorkgroupId(int dimindx);
size_t __attribute__((overloadable)) __spirv_BuiltInLocalInvocationId(int dimindx);
size_t __attribute__((overloadable)) __spirv_BuiltInGlobalInvocationId(int dimindx);
size_t __attribute__((overloadable)) __spirv_BuiltInGlobalSize(int dimindx);
size_t __attribute__((overloadable)) __spirv_BuiltInEnqueuedWorkgroupSize(int dimindx);
size_t __attribute__((overloadable)) __spirv_BuiltInGlobalOffset(int dimindx);

size_t SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInLocalInvocationIndex, , )(void);
size_t SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInGlobalLinearId, , )(void);
uint SPIRV_OVERLOADABLE   SPIRV_BUILTIN_NO_OP(BuiltInWorkDim, , )(void);
uint SPIRV_OVERLOADABLE   SPIRV_BUILTIN_NO_OP(BuiltInSubgroupMaxSize, , )(void);
uint SPIRV_OVERLOADABLE   SPIRV_BUILTIN_NO_OP(BuiltInSubgroupId, , )(void);
uint SPIRV_OVERLOADABLE   SPIRV_BUILTIN_NO_OP(BuiltInNumSubgroups, , )(void);
uint SPIRV_OVERLOADABLE   SPIRV_BUILTIN_NO_OP(BuiltInSubgroupSize, , )(void);
uint SPIRV_OVERLOADABLE   SPIRV_BUILTIN_NO_OP(BuiltInNumEnqueuedSubgroups, , )(void);
uint SPIRV_OVERLOADABLE   SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )(void);

// Image Instructions
//

__spirv_SampledImage_1D SPIRV_OVERLOADABLE SPIRV_BUILTIN(SampledImage, _img1d_ro_i64, )(
    global Img1d_ro *Image, __spirv_Sampler Sampler);
__spirv_SampledImage_2D SPIRV_OVERLOADABLE SPIRV_BUILTIN(SampledImage, _img2d_ro_i64, )(
    global Img2d_ro *Image, __spirv_Sampler Sampler);
__spirv_SampledImage_3D SPIRV_OVERLOADABLE SPIRV_BUILTIN(SampledImage, _img3d_ro_i64, )(
    global Img3d_ro *Image, __spirv_Sampler Sampler);
__spirv_SampledImage_1D_array SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SampledImage,
    _img1d_array_ro_i64, )(global Img1d_array_ro *Image, __spirv_Sampler Sampler);
__spirv_SampledImage_2D_array SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SampledImage,
    _img2d_array_ro_i64, )(global Img2d_array_ro *Image, __spirv_Sampler Sampler);
__spirv_SampledImage_2D_depth SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SampledImage,
    _img2d_depth_ro_i64, )(global Img2d_depth_ro *Image, __spirv_Sampler Sampler);
__spirv_SampledImage_2D_array_depth
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SampledImage, _img2d_array_depth_ro_i64, )(
        global Img2d_array_depth_ro *Image, __spirv_Sampler Sampler);

float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4f32_img2d_ro_v2f32_i32_f32, _Rfloat4)(
        __spirv_SampledImage_2D SampledImage,
        float2                  Coordinate,
        int                     ImageOperands,
        float                   Lod);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4f32_img2d_ro_v2i32_i32_f32, _Rfloat4)(
    __spirv_SampledImage_2D SampledImage, int2 Coordinate, int ImageOperands, float Lod);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4i32_img2d_ro_v2f32_i32_f32, _Rint4)(
        __spirv_SampledImage_2D SampledImage,
        float2                  Coordinate,
        int                     ImageOperands,
        float                   Lod);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4i32_img2d_ro_v2i32_i32_f32, _Rint4)(
    __spirv_SampledImage_2D SampledImage, int2 Coordinate, int ImageOperands, float Lod);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4f32_img3d_ro_v4f32_i32_f32, _Rfloat4)(
        __spirv_SampledImage_3D SampledImage,
        float4                  Coordinate,
        int                     ImageOperands,
        float                   Lod);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4f32_img3d_ro_v4i32_i32_f32, _Rfloat4)(
    __spirv_SampledImage_3D SampledImage, int4 Coordinate, int ImageOperands, float Lod);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4i32_img3d_ro_v4f32_i32_f32, _Rint4)(
        __spirv_SampledImage_3D SampledImage,
        float4                  Coordinate,
        int                     ImageOperands,
        float                   Lod);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4i32_img3d_ro_v4i32_i32_f32, _Rint4)(
    __spirv_SampledImage_3D SampledImage, int4 Coordinate, int ImageOperands, float Lod);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4f32_img2d_array_ro_v4f32_i32_f32, _Rfloat4)(
        __spirv_SampledImage_2D_array SampledImage,
        float4                        Coordinate,
        int                           ImageOperands,
        float                         Lod);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4f32_img2d_array_ro_v4i32_i32_f32, _Rfloat4)(
        __spirv_SampledImage_2D_array SampledImage,
        int4                          Coordinate,
        int                           ImageOperands,
        float                         Lod);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4i32_img2d_array_ro_v4f32_i32_f32, _Rint4)(
        __spirv_SampledImage_2D_array SampledImage,
        float4                        Coordinate,
        int                           ImageOperands,
        float                         Lod);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4i32_img2d_array_ro_v4i32_i32_f32, _Rint4)(
        __spirv_SampledImage_2D_array SampledImage,
        int4                          Coordinate,
        int                           ImageOperands,
        float                         Lod);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4f32_img1d_ro_f32_i32_f32, _Rfloat4)(
    __spirv_SampledImage_1D SampledImage, float Coordinate, int ImageOperands, float Lod);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4f32_img1d_ro_i32_i32_f32, _Rfloat4)(
    __spirv_SampledImage_1D SampledImage, int Coordinate, int ImageOperands, float Lod);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4i32_img1d_ro_f32_i32_f32, _Rint4)(
    __spirv_SampledImage_1D SampledImage, float Coordinate, int ImageOperands, float Lod);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4i32_img1d_ro_i32_i32_f32, _Rint4)(
    __spirv_SampledImage_1D SampledImage, int Coordinate, int ImageOperands, float Lod);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4f32_img1d_array_ro_v2f32_i32_f32, _Rfloat4)(
        __spirv_SampledImage_1D_array SampledImage,
        float2                        Coordinate,
        int                           ImageOperands,
        float                         Lod);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4f32_img1d_array_ro_v2i32_i32_f32, _Rfloat4)(
        __spirv_SampledImage_1D_array SampledImage,
        int2                          Coordinate,
        int                           ImageOperands,
        float                         Lod);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4i32_img1d_array_ro_v2f32_i32_f32, _Rint4)(
        __spirv_SampledImage_1D_array SampledImage,
        float2                        Coordinate,
        int                           ImageOperands,
        float                         Lod);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4i32_img1d_array_ro_v2i32_i32_f32, _Rint4)(
        __spirv_SampledImage_1D_array SampledImage,
        int2                          Coordinate,
        int                           ImageOperands,
        float                         Lod);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4f32_img2d_depth_ro_v2f32_i32_f32, _Rfloat4)(
        __spirv_SampledImage_2D_depth SampledImage,
        float2                        Coordinate,
        int                           ImageOperands,
        float                         Lod);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4f32_img2d_depth_ro_v2i32_i32_f32, _Rfloat4)(
        __spirv_SampledImage_2D_depth SampledImage,
        int2                          Coordinate,
        int                           ImageOperands,
        float                         Lod);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4f32_img2d_array_depth_ro_v4f32_i32_f32, _Rfloat4)(
    __spirv_SampledImage_2D_array_depth SampledImage,
    float4                              Coordinate,
    int                                 ImageOperands,
    float                               Lod);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4f32_img2d_array_depth_ro_v4i32_i32_f32, _Rfloat4)(
    __spirv_SampledImage_2D_array_depth SampledImage,
    int4                                Coordinate,
    int                                 ImageOperands,
    float                               Lod);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4f32_img2d_ro_v2f32_i32_v2f32_v2f32, _Rfloat4)(
    __spirv_SampledImage_2D SampledImage,
    float2                  Coordinate,
    int                     ImageOperands,
    float2                  dx,
    float2                  dy);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4i32_img2d_ro_v2f32_i32_v2f32_v2f32, _Rint4)(
        __spirv_SampledImage_2D SampledImage,
        float2                  Coordinate,
        int                     ImageOperands,
        float2                  dx,
        float2                  dy);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4f32_img3d_ro_v4f32_i32_v4f32_v4f32, _Rfloat4)(
    __spirv_SampledImage_3D SampledImage,
    float4                  Coordinate,
    int                     ImageOperands,
    float4                  dx,
    float4                  dy);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4i32_img3d_ro_v4f32_i32_v4f32_v4f32, _Rint4)(
        __spirv_SampledImage_3D SampledImage,
        float4                  Coordinate,
        int                     ImageOperands,
        float4                  dx,
        float4                  dy);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4f32_img2d_array_ro_v4f32_i32_v2f32_v2f32, _Rfloat4)(
    __spirv_SampledImage_2D SampledImage,
    float4                  Coordinate,
    int                     ImageOperands,
    float2                  dx,
    float2                  dy);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4i32_img2d_array_ro_v4f32_i32_v2f32_v2f32, _Rint4)(
    __spirv_SampledImage_2D SampledImage,
    float4                  Coordinate,
    int                     ImageOperands,
    float2                  dx,
    float2                  dy);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4f32_img1d_ro_f32_i32_f32_f32, _Rfloat4)(
        __spirv_SampledImage_1D SampledImage,
        float                   Coordinate,
        int                     ImageOperands,
        float                   dx,
        float                   dy);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4i32_img1d_ro_f32_i32_f32_f32, _Rint4)(
        __spirv_SampledImage_1D SampledImage,
        float                   Coordinate,
        int                     ImageOperands,
        float                   dx,
        float                   dy);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4f32_img1d_array_ro_v2f32_i32_f32_f32, _Rfloat4)(
    __spirv_SampledImage_1D_array SampledImage,
    float2                        Coordinate,
    int                           ImageOperands,
    float                         dx,
    float                         dy);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4i32_img1d_array_ro_v2f32_i32_f32_f32, _Rint4)(
    __spirv_SampledImage_1D_array SampledImage,
    float2                        Coordinate,
    int                           ImageOperands,
    float                         dx,
    float                         dy);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4f32_img2d_depth_ro_v2f32_i32_v2f32_v2f32, _Rfloat4)(
    __spirv_SampledImage_2D_depth SampledImage,
    float2                        Coordinate,
    int                           ImageOperands,
    float2                        dx,
    float2                        dy);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4f32_img2d_array_depth_ro_v4f32_i32_v2f32_v2f32, _Rfloat4)(
    __spirv_SampledImage_2D_array_depth SampledImage,
    float4                              Coordinate,
    int                                 ImageOperands,
    float2                              dx,
    float2                              dy);

#ifdef cl_khr_fp16
half4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4f16_img2d_ro_v2f32_i32_f32, _Rhalf4)(
        __spirv_SampledImage_2D SampledImage,
        float2                  Coordinate,
        int                     ImageOperands,
        float                   Lod);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4f16_img2d_ro_v2i32_i32_f32, _Rhalf4)(
    __spirv_SampledImage_2D SampledImage, int2 Coordinate, int ImageOperands, float Lod);
half4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4f16_img3d_ro_v4f32_i32_f32, _Rhalf4)(
        __spirv_SampledImage_3D SampledImage,
        float4                  Coordinate,
        int                     ImageOperands,
        float                   Lod);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageSampleExplicitLod, _v4f16_img3d_ro_v4i32_i32_f32, _Rhalf4)(
    __spirv_SampledImage_3D SampledImage, int4 Coordinate, int ImageOperands, float Lod);
half4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4f16_img2d_array_ro_v4f32_i32_f32, _Rhalf4)(
        __spirv_SampledImage_2D_array SampledImage,
        float4                        Coordinate,
        int                           ImageOperands,
        float                         Lod);
half4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4f16_img2d_array_ro_v4i32_i32_f32, _Rhalf4)(
        __spirv_SampledImage_2D_array SampledImage,
        int4                          Coordinate,
        int                           ImageOperands,
        float                         Lod);
half4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4f16_img1d_array_ro_v2f32_i32_f32, _Rhalf4)(
        __spirv_SampledImage_1D_array SampledImage,
        float2                        Coordinate,
        int                           ImageOperands,
        float                         Lod);
half4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageSampleExplicitLod, _v4f16_img1d_array_ro_v2i32_i32_f32, _Rhalf4)(
        __spirv_SampledImage_1D_array SampledImage,
        int2                          Coordinate,
        int                           ImageOperands,
        float                         Lod);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_wo_v2i32_v4f16, )(
    global Img2d_wo *Image, int2 Coordinate, half4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_rw_v2i32_v4f16, )(
    global Img2d_rw *Image, int2 Coordinate, half4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img3d_wo_v4i32_v4f16, )(
    global Img3d_wo *Image, int4 Coordinate, half4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img3d_rw_v4i32_v4f16, )(
    global Img3d_rw *Image, int4 Coordinate, half4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_array_wo_v4i32_v4f16, )(
    global Img2d_array_wo *Image, int4 Coordinate, half4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_array_rw_v4i32_v4f16, )(
    global Img2d_array_rw *Image, int4 Coordinate, half4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_wo_i32_v4f16, )(
    global Img1d_wo *Image, int Coordinate, half4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_rw_i32_v4f16, )(
    global Img1d_rw *Image, int Coordinate, half4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_buffer_wo_i32_v4f16, )(
    global Img1d_buffer_wo *Image, int Coordinate, half4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_buffer_rw_i32_v4f16, )(
    global Img1d_buffer_rw *Image, int Coordinate, half4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_array_wo_v2i32_v4f16, )(
    global Img1d_array_wo *Image, int2 Coordinate, half4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_array_rw_v2i32_v4f16, )(
    global Img1d_array_rw *Image, int2 Coordinate, half4 Texel);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f16_img2d_ro_v2i32, _Rhalf4)(
    global Img2d_ro *Image, int2 Coordinate);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f16_img2d_rw_v2i32, _Rhalf4)(
    global Img2d_rw *Image, int2 Coordinate);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f16_img3d_ro_v4i32, _Rhalf4)(
    global Img3d_ro *Image, int4 Coordinate);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f16_img3d_rw_v4i32, _Rhalf4)(
    global Img3d_rw *Image, int4 Coordinate);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f16_img2d_array_ro_v4i32, _Rhalf4)(
    global Img2d_array_ro *Image, int4 Coordinate);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f16_img2d_array_rw_v4i32, _Rhalf4)(
    global Img2d_array_rw *Image, int4 Coordinate);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f16_img1d_ro_i32, _Rhalf4)(
    global Img1d_ro *Image, int Coordinate);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f16_img1d_rw_i32, _Rhalf4)(
    global Img1d_rw *Image, int Coordinate);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f16_img1d_buffer_ro_i32, _Rhalf4)(
    global Img1d_buffer_ro *Image, int Coordinate);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f16_img1d_buffer_rw_i32, _Rhalf4)(
    global Img1d_buffer_rw *Image, int Coordinate);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f16_img1d_array_ro_v2i32, _Rhalf4)(
    global Img1d_array_ro *Image, int2 Coordinate);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f16_img1d_array_rw_v2i32, _Rhalf4)(
    global Img1d_array_rw *Image, int2 Coordinate);
#endif // cl_khr_fp16

// Pipe in image type information.
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img2d_ro_v2i32, _Rint4)(
    global Img2d_ro *Image, int2 Coordinate);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img2d_ro_v2i32, _Rfloat4)(
    global Img2d_ro *Image, int2 Coordinate);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img2d_rw_v2i32, _Rint4)(
    global Img2d_rw *Image, int2 Coordinate);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img2d_rw_v2i32, _Rfloat4)(
    global Img2d_rw *Image, int2 Coordinate);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img3d_ro_v4i32, _Rint4)(
    global Img3d_ro *Image, int4 Coordinate);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img3d_ro_v4i32, _Rfloat4)(
    global Img3d_ro *Image, int4 Coordinate);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img3d_rw_v4i32, _Rint4)(
    global Img3d_rw *Image, int4 Coordinate);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img3d_rw_v4i32, _Rfloat4)(
    global Img3d_rw *Image, int4 Coordinate);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img2d_array_ro_v4i32, _Rint4)(
    global Img2d_array_ro *Image, int4 Coordinate);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img2d_array_ro_v4i32, _Rfloat4)(
    global Img2d_array_ro *Image, int4 Coordinate);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img2d_array_rw_v4i32, _Rint4)(
    global Img2d_array_rw *Image, int4 Coordinate);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img2d_array_rw_v4i32, _Rfloat4)(
    global Img2d_array_rw *Image, int4 Coordinate);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img1d_ro_i32, _Rint4)(
    global Img1d_ro *Image, int Coordinate);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img1d_ro_i32, _Rfloat4)(
    global Img1d_ro *Image, int Coordinate);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img1d_rw_i32, _Rint4)(
    global Img1d_rw *Image, int Coordinate);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img1d_rw_i32, _Rfloat4)(
    global Img1d_rw *Image, int Coordinate);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img1d_buffer_ro_i32, _Rint4)(
    global Img1d_buffer_ro *Image, int Coordinate);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img1d_buffer_ro_i32, _Rfloat4)(
    global Img1d_buffer_ro *Image, int Coordinate);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img1d_buffer_rw_i32, _Rint4)(
    global Img1d_buffer_rw *Image, int Coordinate);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img1d_buffer_rw_i32, _Rfloat4)(
    global Img1d_buffer_rw *Image, int Coordinate);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img1d_array_ro_v2i32, _Rint4)(
    global Img1d_array_ro *Image, int2 Coordinate);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img1d_array_ro_v2i32, _Rfloat4)(
    global Img1d_array_ro *Image, int2 Coordinate);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img1d_array_rw_v2i32, _Rint4)(
    global Img1d_array_rw *Image, int2 Coordinate);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img1d_array_rw_v2i32, _Rfloat4)(
    global Img1d_array_rw *Image, int2 Coordinate);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _f32_img2d_depth_ro_v2i32, _Rfloat)(
    global Img2d_depth_ro *Image, int2 Coordinate);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _f32_img2d_depth_rw_v2i32, _Rfloat)(
    global Img2d_depth_rw *Image, int2 Coordinate);
float
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _f32_img2d_array_depth_ro_v4i32, _Rfloat)(
        global Img2d_array_depth_ro *Image, int4 Coordinate);
float
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _f32_img2d_array_depth_rw_v4i32, _Rfloat)(
        global Img2d_array_depth_rw *Image, int4 Coordinate);

// Image Read MSAA
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageRead,
    _v4i32_img2d_msaa_ro_v2i32_i32_i32,
    _Rint4)(global Img2d_msaa_ro *Image, int2 Coordinate, int ImageOperands, int Sample);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageRead, _v4f32_img2d_msaa_ro_v2i32_i32_i32, _Rfloat4)(
        global Img2d_msaa_ro *Image, int2 Coordinate, int ImageOperands, int Sample);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageRead, _v4i32_img2d_array_msaa_ro_v4i32_i32_i32, _Rint4)(
    global Img2d_array_msaa_ro *Image, int4 Coordinate, int ImageOperands, int Sample);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageRead, _v4f32_img2d_array_msaa_ro_v4i32_i32_i32, _Rfloat4)(
    global Img2d_array_msaa_ro *Image, int4 Coordinate, int ImageOperands, int Sample);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageRead, _f32_img2d_msaa_depth_ro_v2i32_i32_i32, _Rfloat)(
    global Img2d_msaa_depth_ro *Image, int2 Coordinate, int ImageOperands, int Sample);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageRead, _f32_img2d_array_msaa_depth_ro_v4i32_i32_i32, _Rfloat)(
        global Img2d_array_msaa_depth_ro *Image,
        int4                              Coordinate,
        int                               ImageOperands,
        int                               Sample);

// Image Read with unused ImageOperands
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img2d_ro_v2i32_i32, _Rint4)(
    global Img2d_ro *Image, int2 Coordinate, int ImageOperands);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img2d_ro_v2i32_i32, _Rfloat4)(
    global Img2d_ro *Image, int2 Coordinate, int ImageOperands);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img3d_ro_v4i32_i32, _Rint4)(
    global Img3d_ro *Image, int4 Coordinate, int ImageOperands);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img3d_ro_v4i32_i32, _Rfloat4)(
    global Img3d_ro *Image, int4 Coordinate, int ImageOperands);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img1d_ro_i32_i32, _Rint4)(
    global Img1d_ro *Image, int Coordinate, int ImageOperands);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img1d_ro_i32_i32, _Rfloat4)(
    global Img1d_ro *Image, int Coordinate, int ImageOperands);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img2d_array_ro_v4i32_i32, _Rint4)(
    global Img2d_array_ro *Image, int4 Coordinate, int ImageOperands);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageRead,
    _v4f32_img2d_array_ro_v4i32_i32,
    _Rfloat4)(global Img2d_array_ro *Image, int4 Coordinate, int ImageOperands);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img1d_array_ro_v2i32_i32, _Rint4)(
    global Img1d_array_ro *Image, int2 Coordinate, int ImageOperands);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageRead,
    _v4f32_img1d_array_ro_v2i32_i32,
    _Rfloat4)(global Img1d_array_ro *Image, int2 Coordinate, int ImageOperands);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img1d_buffer_ro_i32_i32, _Rint4)(
    global Img1d_buffer_ro *Image, int Coordinate, int ImageOperands);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img1d_buffer_ro_i32_i32, _Rfloat4)(
        global Img1d_buffer_ro *Image, int Coordinate, int ImageOperands);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img2d_rw_v2i32_i32, _Rint4)(
    global Img2d_rw *Image, int2 Coordinate, int ImageOperands);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img2d_rw_v2i32_i32, _Rfloat4)(
    global Img2d_rw *Image, int2 Coordinate, int ImageOperands);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img3d_rw_v4i32_i32, _Rint4)(
    global Img3d_rw *Image, int4 Coordinate, int ImageOperands);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img3d_rw_v4i32_i32, _Rfloat4)(
    global Img3d_rw *Image, int4 Coordinate, int ImageOperands);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img1d_rw_i32_i32, _Rint4)(
    global Img1d_rw *Image, int Coordinate, int ImageOperands);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img1d_rw_i32_i32, _Rfloat4)(
    global Img1d_rw *Image, int Coordinate, int ImageOperands);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img2d_array_rw_v4i32_i32, _Rint4)(
    global Img2d_array_rw *Image, int4 Coordinate, int ImageOperands);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageRead,
    _v4f32_img2d_array_rw_v4i32_i32,
    _Rfloat4)(global Img2d_array_rw *Image, int4 Coordinate, int ImageOperands);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img1d_array_rw_v2i32_i32, _Rint4)(
    global Img1d_array_rw *Image, int2 Coordinate, int ImageOperands);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageRead,
    _v4f32_img1d_array_rw_v2i32_i32,
    _Rfloat4)(global Img1d_array_rw *Image, int2 Coordinate, int ImageOperands);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4i32_img1d_buffer_rw_i32_i32, _Rint4)(
    global Img1d_buffer_rw *Image, int Coordinate, int ImageOperands);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageRead, _v4f32_img1d_buffer_rw_i32_i32, _Rfloat4)(
        global Img1d_buffer_rw *Image, int Coordinate, int ImageOperands);

// Image Write
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_wo_v2i32_v4i32, )(
    global Img2d_wo *Image, int2 Coordinate, int4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_wo_v2i32_v4f32, )(
    global Img2d_wo *Image, int2 Coordinate, float4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_rw_v2i32_v4i32, )(
    global Img2d_rw *Image, int2 Coordinate, int4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_rw_v2i32_v4f32, )(
    global Img2d_rw *Image, int2 Coordinate, float4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_array_wo_v4i32_v4i32, )(
    global Img2d_array_wo *Image, int4 Coordinate, int4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_array_wo_v4i32_v4f32, )(
    global Img2d_array_wo *Image, int4 Coordinate, float4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_array_rw_v4i32_v4i32, )(
    global Img2d_array_rw *Image, int4 Coordinate, int4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_array_rw_v4i32_v4f32, )(
    global Img2d_array_rw *Image, int4 Coordinate, float4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_wo_i32_v4i32, )(
    global Img1d_wo *Image, int Coordinate, int4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_wo_i32_v4f32, )(
    global Img1d_wo *Image, int Coordinate, float4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_rw_i32_v4i32, )(
    global Img1d_rw *Image, int Coordinate, int4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_rw_i32_v4f32, )(
    global Img1d_rw *Image, int Coordinate, float4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_buffer_wo_i32_v4i32, )(
    global Img1d_buffer_wo *Image, int Coordinate, int4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_buffer_wo_i32_v4f32, )(
    global Img1d_buffer_wo *Image, int Coordinate, float4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_buffer_rw_i32_v4i32, )(
    global Img1d_buffer_rw *Image, int Coordinate, int4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_buffer_rw_i32_v4f32, )(
    global Img1d_buffer_rw *Image, int Coordinate, float4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_array_wo_v2i32_v4i32, )(
    global Img1d_array_wo *Image, int2 Coordinate, int4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_array_wo_v2i32_v4f32, )(
    global Img1d_array_wo *Image, int2 Coordinate, float4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_array_rw_v2i32_v4i32, )(
    global Img1d_array_rw *Image, int2 Coordinate, int4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_array_rw_v2i32_v4f32, )(
    global Img1d_array_rw *Image, int2 Coordinate, float4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_depth_wo_v2i32_f32, )(
    global Img2d_depth_wo *Image, int2 Coordinate, float Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_depth_rw_v2i32_f32, )(
    global Img2d_depth_rw *Image, int2 Coordinate, float Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_array_depth_wo_v4i32_f32, )(
    global Img2d_array_depth_wo *Image, int4 Coordinate, float Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_array_depth_rw_v4i32_f32, )(
    global Img2d_array_depth_rw *Image, int4 Coordinate, float Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img3d_wo_v4i32_v4i32, )(
    global Img3d_wo *Image, int4 Coordinate, int4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img3d_wo_v4i32_v4f32, )(
    global Img3d_wo *Image, int4 Coordinate, float4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img3d_rw_v4i32_v4i32, )(
    global Img3d_rw *Image, int4 Coordinate, int4 Texel);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img3d_rw_v4i32_v4f32, )(
    global Img3d_rw *Image, int4 Coordinate, float4 Texel);

// Image Write with unused ImageOperands
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_wo_v2i32_v4i32_i32, )(
    global Img2d_wo *Image, int2 Coordinate, int4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_wo_v2i32_v4f32_i32, )(
    global Img2d_wo *Image, int2 Coordinate, float4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_depth_wo_v2i32_f32_i32, )(
    global Img2d_depth_wo *Image, int2 Coordinate, float Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_wo_i32_v4i32_i32, )(
    global Img1d_wo *Image, int Coordinate, int4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_wo_i32_v4f32_i32, )(
    global Img1d_wo *Image, int Coordinate, float4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_array_wo_v2i32_v4i32_i32, )(
    global Img1d_array_wo *Image, int2 Coordinate, int4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_array_wo_v2i32_v4f32_i32, )(
    global Img1d_array_wo *Image, int2 Coordinate, float4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_buffer_wo_i32_v4i32_i32, )(
    global Img1d_buffer_wo *Image, int Coordinate, int4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_buffer_wo_i32_v4f32_i32, )(
    global Img1d_buffer_wo *Image, int Coordinate, float4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_array_wo_v4i32_v4i32_i32, )(
    global Img2d_array_wo *Image, int4 Coordinate, int4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_array_wo_v4i32_v4f32_i32, )(
    global Img2d_array_wo *Image, int4 Coordinate, float4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_array_depth_wo_v4i32_f32_i32, )(
    global Img2d_array_depth_wo *Image, int4 Coordinate, float Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img3d_wo_v4i32_v4i32_i32, )(
    global Img3d_wo *Image, int4 Coordinate, int4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img3d_wo_v4i32_v4f32_i32, )(
    global Img3d_wo *Image, int4 Coordinate, float4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_rw_v2i32_v4i32_i32, )(
    global Img2d_rw *Image, int2 Coordinate, int4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_rw_v2i32_v4f32_i32, )(
    global Img2d_rw *Image, int2 Coordinate, float4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_depth_rw_v2i32_f32_i32, )(
    global Img2d_depth_rw *Image, int2 Coordinate, float Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_rw_i32_v4i32_i32, )(
    global Img1d_rw *Image, int Coordinate, int4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_rw_i32_v4f32_i32, )(
    global Img1d_rw *Image, int Coordinate, float4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_array_rw_v2i32_v4i32_i32, )(
    global Img1d_array_rw *Image, int2 Coordinate, int4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_array_rw_v2i32_v4f32_i32, )(
    global Img1d_array_rw *Image, int2 Coordinate, float4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_buffer_rw_i32_v4i32_i32, )(
    global Img1d_buffer_rw *Image, int Coordinate, int4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_buffer_rw_i32_v4f32_i32, )(
    global Img1d_buffer_rw *Image, int Coordinate, float4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_array_rw_v4i32_v4i32_i32, )(
    global Img2d_array_rw *Image, int4 Coordinate, int4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_array_rw_v4i32_v4f32_i32, )(
    global Img2d_array_rw *Image, int4 Coordinate, float4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_array_depth_rw_v4i32_f32_i32, )(
    global Img2d_array_depth_rw *Image, int4 Coordinate, float Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img3d_rw_v4i32_v4i32_i32, )(
    global Img3d_rw *Image, int4 Coordinate, int4 Texel, int ImageOperands);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img3d_rw_v4i32_v4f32_i32, )(
    global Img3d_rw *Image, int4 Coordinate, float4 Texel, int ImageOperands);

// Image Write LoD
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_wo_v2i32_v4i32_i32_i32, )(
    global Img2d_wo *Image, int2 Coordinate, int4 Texel, int ImageOperands, int Lod);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_wo_v2i32_v4f32_i32_i32, )(
    global Img2d_wo *Image, int2 Coordinate, float4 Texel, int ImageOperands, int Lod);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_depth_wo_v2i32_f32_i32_i32, )(
    global Img2d_depth_wo *Image,
    int2                   Coordinate,
    float                  Texel,
    int                    ImageOperands,
    int                    Lod);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_wo_i32_v4i32_i32_i32, )(
    global Img1d_wo *Image, int Coordinate, int4 Texel, int ImageOperands, int Lod);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_wo_i32_v4f32_i32_i32, )(
    global Img1d_wo *Image, int Coordinate, float4 Texel, int ImageOperands, int Lod);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_array_wo_v2i32_v4i32_i32_i32, )(
    global Img1d_array_wo *Image,
    int2                   Coordinate,
    int4                   Texel,
    int                    ImageOperands,
    int                    Lod);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img1d_array_wo_v2i32_v4f32_i32_i32, )(
    global Img1d_array_wo *Image,
    int2                   Coordinate,
    float4                 Texel,
    int                    ImageOperands,
    int                    Lod);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_array_wo_v4i32_v4i32_i32_i32, )(
    global Img2d_array_wo *Image,
    int4                   Coordinate,
    int4                   Texel,
    int                    ImageOperands,
    int                    Lod);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img2d_array_wo_v4i32_v4f32_i32_i32, )(
    global Img2d_array_wo *Image,
    int4                   Coordinate,
    float4                 Texel,
    int                    ImageOperands,
    int                    Lod);
void SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageWrite, _img2d_array_depth_wo_v4i32_f32_i32_i32, )(
        global Img2d_array_depth_wo *Image,
        int4                         Coordinate,
        float                        Texel,
        int                          ImageOperands,
        int                          Lod);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img3d_wo_v4i32_v4i32_i32_i32, )(
    global Img3d_wo *Image, int4 Coordinate, int4 Texel, int ImageOperands, int Lod);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageWrite, _img3d_wo_v4i32_v4f32_i32_i32, )(
    global Img3d_wo *Image, int4 Coordinate, float4 Texel, int ImageOperands, int Lod);

// Image Query Format
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img2d_ro, )(global Img2d_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img2d_wo, )(global Img2d_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img2d_rw, )(global Img2d_rw *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img3d_ro, )(global Img3d_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img3d_wo, )(global Img3d_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img3d_rw, )(global Img3d_rw *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img1d_ro, )(global Img1d_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img1d_wo, )(global Img1d_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img1d_rw, )(global Img1d_rw *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img1d_buffer_ro, )(global Img1d_buffer_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img1d_buffer_wo, )(global Img1d_buffer_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img1d_buffer_rw, )(global Img1d_buffer_rw *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img1d_array_ro, )(global Img1d_array_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img1d_array_wo, )(global Img1d_array_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img1d_array_rw, )(global Img1d_array_rw *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img2d_array_ro, )(global Img2d_array_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img2d_array_wo, )(global Img2d_array_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img2d_array_rw, )(global Img2d_array_rw *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img2d_depth_ro, )(global Img2d_depth_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img2d_depth_wo, )(global Img2d_depth_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img2d_depth_rw, )(global Img2d_depth_rw *Image);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQueryFormat, _img2d_array_depth_ro, )(
    global Img2d_array_depth_ro *Image);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQueryFormat, _img2d_array_depth_wo, )(
    global Img2d_array_depth_wo *Image);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQueryFormat, _img2d_array_depth_rw, )(
    global Img2d_array_depth_rw *Image);

uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryFormat, _img2d_msaa_ro, )(global Img2d_msaa_ro *Image);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQueryFormat, _img2d_array_msaa_ro, )(
    global Img2d_array_msaa_ro *Image);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQueryFormat, _img2d_msaa_depth_ro, )(
    global Img2d_msaa_depth_ro *Image);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQueryFormat, _img2d_array_msaa_depth_ro, )(
    global Img2d_array_msaa_depth_ro *Image);

// Image Query Order
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img2d_ro, )(global Img2d_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img2d_wo, )(global Img2d_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img2d_rw, )(global Img2d_rw *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img3d_ro, )(global Img3d_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img3d_wo, )(global Img3d_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img3d_rw, )(global Img3d_rw *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img1d_ro, )(global Img1d_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img1d_wo, )(global Img1d_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img1d_rw, )(global Img1d_rw *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img1d_buffer_ro, )(global Img1d_buffer_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img1d_buffer_wo, )(global Img1d_buffer_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img1d_buffer_rw, )(global Img1d_buffer_rw *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img1d_array_ro, )(global Img1d_array_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img1d_array_wo, )(global Img1d_array_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img1d_array_rw, )(global Img1d_array_rw *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img2d_array_ro, )(global Img2d_array_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img2d_array_wo, )(global Img2d_array_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img2d_array_rw, )(global Img2d_array_rw *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img2d_depth_ro, )(global Img2d_depth_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img2d_depth_wo, )(global Img2d_depth_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img2d_depth_rw, )(global Img2d_depth_rw *Image);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQueryOrder, _img2d_array_depth_ro, )(
    global Img2d_array_depth_ro *Image);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQueryOrder, _img2d_array_depth_wo, )(
    global Img2d_array_depth_wo *Image);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQueryOrder, _img2d_array_depth_rw, )(
    global Img2d_array_depth_rw *Image);

uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryOrder, _img2d_msaa_ro, )(global Img2d_msaa_ro *Image);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQueryOrder, _img2d_array_msaa_ro, )(
    global Img2d_array_msaa_ro *Image);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQueryOrder, _img2d_msaa_depth_ro, )(
    global Img2d_msaa_depth_ro *Image);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQueryOrder, _img2d_array_msaa_depth_ro, )(
    global Img2d_array_msaa_depth_ro *Image);

// Image Query Size
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySize, _i32_img1d_ro, _Rint)(global Img1d_ro *Image);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySize, _i32_img1d_wo, _Rint)(global Img1d_wo *Image);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySize, _i32_img1d_rw, _Rint)(global Img1d_rw *Image);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _i32_img1d_buffer_ro, _Rint)(
    global Img1d_buffer_ro *Image);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _i32_img1d_buffer_wo, _Rint)(
    global Img1d_buffer_wo *Image);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _i32_img1d_buffer_rw, _Rint)(
    global Img1d_buffer_rw *Image);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v2i32_img1d_array_ro, _Rint2)(
    global Img1d_array_ro *Image);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v2i32_img1d_array_wo, _Rint2)(
    global Img1d_array_wo *Image);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v2i32_img1d_array_rw, _Rint2)(
    global Img1d_array_rw *Image);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySize, _v2i32_img2d_ro, _Rint2)(global Img2d_ro *Image);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySize, _v2i32_img2d_wo, _Rint2)(global Img2d_wo *Image);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySize, _v2i32_img2d_rw, _Rint2)(global Img2d_rw *Image);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v2i32_img2d_depth_ro, _Rint2)(
    global Img2d_depth_ro *Image);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v2i32_img2d_depth_wo, _Rint2)(
    global Img2d_depth_wo *Image);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v2i32_img2d_depth_rw, _Rint2)(
    global Img2d_depth_rw *Image);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v3i32_img2d_array_ro, _Rint3)(
    global Img2d_array_ro *Image);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v3i32_img2d_array_wo, _Rint3)(
    global Img2d_array_wo *Image);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v3i32_img2d_array_rw, _Rint3)(
    global Img2d_array_rw *Image);
int3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v3i32_img2d_array_depth_ro, _Rint3)(
        global Img2d_array_depth_ro *Image);
int3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v3i32_img2d_array_depth_wo, _Rint3)(
        global Img2d_array_depth_wo *Image);
int3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v3i32_img2d_array_depth_rw, _Rint3)(
        global Img2d_array_depth_rw *Image);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySize, _v3i32_img3d_ro, _Rint3)(global Img3d_ro *Image);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySize, _v3i32_img3d_wo, _Rint3)(global Img3d_wo *Image);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySize, _v3i32_img3d_rw, _Rint3)(global Img3d_rw *Image);

int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v2i32_img2d_msaa_ro, _Rint2)(
    global Img2d_msaa_ro *Image);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v2i32_img2d_msaa_depth_ro, _Rint2)(
    global Img2d_msaa_depth_ro *Image);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v3i32_img2d_array_msaa_ro, _Rint3)(
    global Img2d_array_msaa_ro *Image);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySize,
    _v3i32_img2d_array_msaa_depth_ro,
    _Rint3)(global Img2d_array_msaa_depth_ro *Image);

long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySize, _i64_img1d_ro, _Rlong)(global Img1d_ro *Image);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySize, _i64_img1d_wo, _Rlong)(global Img1d_wo *Image);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySize, _i64_img1d_rw, _Rlong)(global Img1d_rw *Image);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _i64_img1d_buffer_ro, _Rlong)(
    global Img1d_buffer_ro *Image);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _i64_img1d_buffer_wo, _Rlong)(
    global Img1d_buffer_wo *Image);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _i64_img1d_buffer_rw, _Rlong)(
    global Img1d_buffer_rw *Image);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v2i64_img1d_array_ro, _Rlong2)(
    global Img1d_array_ro *Image);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v2i64_img1d_array_wo, _Rlong2)(
    global Img1d_array_wo *Image);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v2i64_img1d_array_rw, _Rlong2)(
    global Img1d_array_rw *Image);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySize, _v2i64_img2d_ro, _Rlong2)(global Img2d_ro *Image);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySize, _v2i64_img2d_wo, _Rlong2)(global Img2d_wo *Image);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySize, _v2i64_img2d_rw, _Rlong2)(global Img2d_rw *Image);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v2i64_img2d_depth_ro, _Rlong2)(
    global Img2d_depth_ro *Image);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v2i64_img2d_depth_wo, _Rlong2)(
    global Img2d_depth_wo *Image);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v2i64_img2d_depth_rw, _Rlong2)(
    global Img2d_depth_rw *Image);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v3i64_img2d_array_ro, _Rlong3)(
    global Img2d_array_ro *Image);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v3i64_img2d_array_wo, _Rlong3)(
    global Img2d_array_wo *Image);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v3i64_img2d_array_rw, _Rlong3)(
    global Img2d_array_rw *Image);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySize,
    _v3i64_img2d_array_depth_ro,
    _Rlong3)(global Img2d_array_depth_ro *Image);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySize,
    _v3i64_img2d_array_depth_wo,
    _Rlong3)(global Img2d_array_depth_wo *Image);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySize,
    _v3i64_img2d_array_depth_rw,
    _Rlong3)(global Img2d_array_depth_rw *Image);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySize, _v3i64_img3d_ro, _Rlong3)(global Img3d_ro *Image);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySize, _v3i64_img3d_wo, _Rlong3)(global Img3d_wo *Image);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySize, _v3i64_img3d_rw, _Rlong3)(global Img3d_rw *Image);

long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v2i64_img2d_msaa_ro, _Rlong2)(
    global Img2d_msaa_ro *Image);
long2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v2i64_img2d_msaa_depth_ro, _Rlong2)(
        global Img2d_msaa_depth_ro *Image);
long3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySize, _v3i64_img2d_array_msaa_ro, _Rlong3)(
        global Img2d_array_msaa_ro *Image);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySize,
    _v3i64_img2d_array_msaa_depth_ro,
    _Rlong3)(global Img2d_array_msaa_depth_ro *Image);

// Image Query Size Lod
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _i32_img1d_ro_i32, _Rint)(
    global Img1d_ro *Image, int Lod);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _i32_img1d_wo_i32, _Rint)(
    global Img1d_wo *Image, int Lod);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _i32_img1d_rw_i32, _Rint)(
    global Img1d_rw *Image, int Lod);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _i32_img1d_buffer_ro_i32, _Rint)(
    global Img1d_buffer_ro *Image, int Lod);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _i32_img1d_buffer_wo_i32, _Rint)(
    global Img1d_buffer_wo *Image, int Lod);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _i32_img1d_buffer_rw_i32, _Rint)(
    global Img1d_buffer_rw *Image, int Lod);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v2i32_img1d_array_ro_i32,
    _Rint2)(global Img1d_array_ro *Image, int Lod);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v2i32_img1d_array_wo_i32,
    _Rint2)(global Img1d_array_wo *Image, int Lod);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v2i32_img1d_array_rw_i32,
    _Rint2)(global Img1d_array_rw *Image, int Lod);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _v2i32_img2d_ro_i32, _Rint2)(
    global Img2d_ro *Image, int Lod);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _v2i32_img2d_wo_i32, _Rint2)(
    global Img2d_wo *Image, int Lod);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _v2i32_img2d_rw_i32, _Rint2)(
    global Img2d_rw *Image, int Lod);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v2i32_img2d_depth_ro_i32,
    _Rint2)(global Img2d_depth_ro *Image, int Lod);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v2i32_img2d_depth_wo_i32,
    _Rint2)(global Img2d_depth_wo *Image, int Lod);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v2i32_img2d_depth_rw_i32,
    _Rint2)(global Img2d_depth_rw *Image, int Lod);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v3i32_img2d_array_ro_i32,
    _Rint3)(global Img2d_array_ro *Image, int Lod);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v3i32_img2d_array_wo_i32,
    _Rint3)(global Img2d_array_wo *Image, int Lod);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v3i32_img2d_array_rw_i32,
    _Rint3)(global Img2d_array_rw *Image, int Lod);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v3i32_img2d_array_depth_ro_i32,
    _Rint3)(global Img2d_array_depth_ro *Image, int Lod);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v3i32_img2d_array_depth_wo_i32,
    _Rint3)(global Img2d_array_depth_wo *Image, int Lod);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v3i32_img2d_array_depth_rw_i32,
    _Rint3)(global Img2d_array_depth_rw *Image, int Lod);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _v3i32_img3d_ro_i32, _Rint3)(
    global Img3d_ro *Image, int Lod);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _v3i32_img3d_wo_i32, _Rint3)(
    global Img3d_wo *Image, int Lod);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _v3i32_img3d_rw_i32, _Rint3)(
    global Img3d_rw *Image, int Lod);

int2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _v2i32_img2d_msaa_ro_i32, _Rint2)(
        global Img2d_msaa_ro *Image, int Lod);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v2i32_img2d_msaa_depth_ro_i32,
    _Rint2)(global Img2d_msaa_depth_ro *Image, int Lod);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v3i32_img2d_array_msaa_ro_i32,
    _Rint3)(global Img2d_array_msaa_ro *Image, int Lod);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v3i32_img2d_array_msaa_depth_ro_i32,
    _Rint3)(global Img2d_array_msaa_depth_ro *Image, int Lod);

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _i64_img1d_ro_i32, _Rlong)(
    global Img1d_ro *Image, int Lod);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _i64_img1d_wo_i32, _Rlong)(
    global Img1d_wo *Image, int Lod);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _i64_img1d_rw_i32, _Rlong)(
    global Img1d_rw *Image, int Lod);
long
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _i64_img1d_buffer_ro_i32, _Rlong)(
        global Img1d_buffer_ro *Image, int Lod);
long
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _i64_img1d_buffer_wo_i32, _Rlong)(
        global Img1d_buffer_wo *Image, int Lod);
long
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _i64_img1d_buffer_rw_i32, _Rlong)(
        global Img1d_buffer_rw *Image, int Lod);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v2i64_img1d_array_ro_i32,
    _Rlong2)(global Img1d_array_ro *Image, int Lod);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v2i64_img1d_array_wo_i32,
    _Rlong2)(global Img1d_array_wo *Image, int Lod);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v2i64_img1d_array_rw_i32,
    _Rlong2)(global Img1d_array_rw *Image, int Lod);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _v2i64_img2d_ro_i32, _Rlong2)(
    global Img2d_ro *Image, int Lod);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _v2i64_img2d_wo_i32, _Rlong2)(
    global Img2d_wo *Image, int Lod);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _v2i64_img2d_rw_i32, _Rlong2)(
    global Img2d_rw *Image, int Lod);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v2i64_img2d_depth_ro_i32,
    _Rlong2)(global Img2d_depth_ro *Image, int Lod);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v2i64_img2d_depth_wo_i32,
    _Rlong2)(global Img2d_depth_wo *Image, int Lod);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v2i64_img2d_depth_rw_i32,
    _Rlong2)(global Img2d_depth_rw *Image, int Lod);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v3i64_img2d_array_ro_i32,
    _Rlong3)(global Img2d_array_ro *Image, int Lod);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v3i64_img2d_array_wo_i32,
    _Rlong3)(global Img2d_array_wo *Image, int Lod);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v3i64_img2d_array_rw_i32,
    _Rlong3)(global Img2d_array_rw *Image, int Lod);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v3i64_img2d_array_depth_ro_i32,
    _Rlong3)(global Img2d_array_depth_ro *Image, int Lod);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v3i64_img2d_array_depth_wo_i32,
    _Rlong3)(global Img2d_array_depth_wo *Image, int Lod);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v3i64_img2d_array_depth_rw_i32,
    _Rlong3)(global Img2d_array_depth_rw *Image, int Lod);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _v3i64_img3d_ro_i32, _Rlong3)(
    global Img3d_ro *Image, int Lod);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _v3i64_img3d_wo_i32, _Rlong3)(
    global Img3d_wo *Image, int Lod);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySizeLod, _v3i64_img3d_rw_i32, _Rlong3)(
    global Img3d_rw *Image, int Lod);

long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v2i64_img2d_msaa_ro_i32,
    _Rlong2)(global Img2d_msaa_ro *Image, int Lod);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v2i64_img2d_msaa_depth_ro_i32,
    _Rlong2)(global Img2d_msaa_depth_ro *Image, int Lod);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v3i64_img2d_array_msaa_ro_i32,
    _Rlong3)(global Img2d_array_msaa_ro *Image, int Lod);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    ImageQuerySizeLod,
    _v3i64_img2d_array_msaa_depth_ro_i32,
    _Rlong3)(global Img2d_array_msaa_depth_ro *Image, int Lod);

// Image Query Levels
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryLevels, _img1d_ro, )(global Img1d_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryLevels, _img1d_wo, )(global Img1d_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryLevels, _img1d_rw, )(global Img1d_rw *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryLevels, _img2d_ro, )(global Img2d_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryLevels, _img2d_wo, )(global Img2d_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryLevels, _img2d_rw, )(global Img2d_rw *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryLevels, _img3d_ro, )(global Img3d_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryLevels, _img3d_wo, )(global Img3d_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryLevels, _img3d_rw, )(global Img3d_rw *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryLevels, _img1d_array_ro, )(global Img1d_array_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryLevels, _img1d_array_wo, )(global Img1d_array_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryLevels, _img1d_array_rw, )(global Img1d_array_rw *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryLevels, _img2d_array_ro, )(global Img2d_array_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryLevels, _img2d_array_wo, )(global Img2d_array_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryLevels, _img2d_array_rw, )(global Img2d_array_rw *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryLevels, _img2d_depth_ro, )(global Img2d_depth_ro *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryLevels, _img2d_depth_wo, )(global Img2d_depth_wo *Image);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQueryLevels, _img2d_depth_rw, )(global Img2d_depth_rw *Image);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQueryLevels, _img2d_array_depth_ro, )(
    global Img2d_array_depth_ro *Image);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQueryLevels, _img2d_array_depth_wo, )(
    global Img2d_array_depth_wo *Image);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQueryLevels, _img2d_array_depth_rw, )(
    global Img2d_array_depth_rw *Image);

// Image Query Samples
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ImageQuerySamples, _img2d_msaa_ro, )(global Img2d_msaa_ro *Image);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySamples, _img2d_array_msaa_ro, )(
    global Img2d_array_msaa_ro *Image);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySamples, _img2d_msaa_depth_ro, )(
    global Img2d_msaa_depth_ro *Image);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ImageQuerySamples, _img2d_array_msaa_depth_ro, )(
    global Img2d_array_msaa_depth_ro *Image);

// Conversion Instructions

uchar SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(half FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i16_f16, _Rushort)(half FloatValue);
uint SPIRV_OVERLOADABLE   SPIRV_BUILTIN(ConvertFToU, _i32_f16, _Ruint)(half FloatValue);
ulong SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToU, _i64_f16, _Rulong)(half FloatValue);
uchar SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToU, _i8_f32, _Ruchar)(float FloatValue);
ushort
    SPIRV_OVERLOADABLE   SPIRV_BUILTIN(ConvertFToU, _i16_f32, _Rushort)(float FloatValue);
uint SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToU, _i32_f32, _Ruint)(float FloatValue);
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i64_f32, _Rulong)(float FloatValue);
#if defined(cl_khr_fp64)
uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i8_f64, _Ruchar)(double FloatValue);
ushort
    SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToU, _i16_f64, _Rushort)(double FloatValue);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i32_f64, _Ruint)(double FloatValue);
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i64_f64, _Rulong)(double FloatValue);
#endif // defined(cl_khr_fp64)
char SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(half FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(half FloatValue);
int SPIRV_OVERLOADABLE   SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(half FloatValue);
long SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _i64_f16, _Rlong)(half FloatValue);
char SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _i8_f32, _Rchar)(float FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i16_f32, _Rshort)(float FloatValue);
int SPIRV_OVERLOADABLE   SPIRV_BUILTIN(ConvertFToS, _i32_f32, _Rint)(float FloatValue);
long SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _i64_f32, _Rlong)(float FloatValue);
#if defined(cl_khr_fp64)
char SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _i8_f64, _Rchar)(double FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i16_f64, _Rshort)(double FloatValue);
int SPIRV_OVERLOADABLE   SPIRV_BUILTIN(ConvertFToS, _i32_f64, _Rint)(double FloatValue);
long SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _i64_f64, _Rlong)(double FloatValue);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _f16_i8, _Rhalf)(char SignedValue);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f32_i8, _Rfloat)(char SignedValue);
half SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _f16_i16, _Rhalf)(short SignedValue);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f32_i16, _Rfloat)(short SignedValue);
half SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _f16_i32, _Rhalf)(int SignedValue);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f32_i32, _Rfloat)(int SignedValue);
half SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _f16_i64, _Rhalf)(long SignedValue);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f32_i64, _Rfloat)(long SignedValue);
half SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertUToF, _f16_i8, _Rhalf)(uchar UnsignedValue);
float
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f32_i8, _Rfloat)(uchar UnsignedValue);
half
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f16_i16, _Rhalf)(ushort UnsignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _f32_i16, _Rfloat)(ushort UnsignedValue);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f16_i32, _Rhalf)(uint UnsignedValue);
float
    SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertUToF, _f32_i32, _Rfloat)(uint UnsignedValue);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f16_i64, _Rhalf)(ulong UnsignedValue);
float
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f32_i64, _Rfloat)(ulong UnsignedValue);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f64_i8, _Rdouble)(char SignedValue);
double
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f64_i16, _Rdouble)(short SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f64_i32, _Rdouble)(int SignedValue);
double
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f64_i64, _Rdouble)(long SignedValue);
double
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f64_i8, _Rdouble)(uchar UnsignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _f64_i16, _Rdouble)(ushort UnsignedValue);
double
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f64_i32, _Rdouble)(uint UnsignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _f64_i64, _Rdouble)(ulong UnsignedValue);
#endif // defined(cl_khr_fp64)
uchar SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _i8_i8, _Ruchar)(uchar UnsignedValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i16_i8, _Rushort)(uchar UnsignedValue);
uint SPIRV_OVERLOADABLE   SPIRV_BUILTIN(UConvert, _i32_i8, _Ruint)(uchar UnsignedValue);
ulong SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _i64_i8, _Rulong)(uchar UnsignedValue);
uchar SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _i8_i16, _Ruchar)(ushort UnsignedValue);
ushort
    SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _i16_i16, _Rushort)(ushort UnsignedValue);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i32_i16, _Ruint)(ushort UnsignedValue);
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i64_i16, _Rulong)(ushort UnsignedValue);
uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i8_i32, _Ruchar)(uint UnsignedValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i16_i32, _Rushort)(uint UnsignedValue);
uint SPIRV_OVERLOADABLE   SPIRV_BUILTIN(UConvert, _i32_i32, _Ruint)(uint UnsignedValue);
ulong SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _i64_i32, _Rulong)(uint UnsignedValue);
uchar SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _i8_i64, _Ruchar)(ulong UnsignedValue);
ushort
    SPIRV_OVERLOADABLE   SPIRV_BUILTIN(UConvert, _i16_i64, _Rushort)(ulong UnsignedValue);
uint SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _i32_i64, _Ruint)(ulong UnsignedValue);
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i64_i64, _Rulong)(ulong UnsignedValue);
char SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _i8_i8, _Rchar)(char SignedValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i16_i8, _Rshort)(char SignedValue);
int SPIRV_OVERLOADABLE   SPIRV_BUILTIN(SConvert, _i32_i8, _Rint)(char SignedValue);
long SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _i64_i8, _Rlong)(char SignedValue);
char SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _i8_i16, _Rchar)(short SignedValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i16_i16, _Rshort)(short SignedValue);
int SPIRV_OVERLOADABLE   SPIRV_BUILTIN(SConvert, _i32_i16, _Rint)(short SignedValue);
long SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _i64_i16, _Rlong)(short SignedValue);
char SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _i8_i32, _Rchar)(int SignedValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i16_i32, _Rshort)(int SignedValue);
int SPIRV_OVERLOADABLE   SPIRV_BUILTIN(SConvert, _i32_i32, _Rint)(int SignedValue);
long SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _i64_i32, _Rlong)(int SignedValue);
char SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _i8_i64, _Rchar)(long SignedValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i16_i64, _Rshort)(long SignedValue);
int SPIRV_OVERLOADABLE   SPIRV_BUILTIN(SConvert, _i32_i64, _Rint)(long SignedValue);
long SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _i64_i64, _Rlong)(long SignedValue);
half SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _f16_f16, _Rhalf)(half FloatValue);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f32_f16, _Rfloat)(half FloatValue);
half SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)(float FloatValue);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f32_f32, _Rfloat)(float FloatValue);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f64_f16, _Rdouble)(half FloatValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f64_f32, _Rdouble)(float FloatValue);
half SPIRV_OVERLOADABLE   SPIRV_BUILTIN(FConvert, _f16_f64, _Rhalf)(double FloatValue);
float SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _f32_f64, _Rfloat)(double FloatValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f64_f64, _Rdouble)(double FloatValue);
#endif // defined(cl_khr_fp64)
uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i8_i8, _Ruchar)(char SignedValue);
ushort
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i16_i8, _Rushort)(char SignedValue);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i32_i8, _Ruint)(char SignedValue);
ulong
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i64_i8, _Rulong)(char SignedValue);
uchar
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i8_i16, _Ruchar)(short SignedValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertSToU, _i16_i16, _Rushort)(short SignedValue);
uint
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i32_i16, _Ruint)(short SignedValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertSToU, _i64_i16, _Rulong)(short SignedValue);
uchar SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i8_i32, _Ruchar)(int SignedValue);
ushort
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i16_i32, _Rushort)(int SignedValue);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i32_i32, _Ruint)(int SignedValue);
ulong
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i64_i32, _Rulong)(int SignedValue);
uchar
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i8_i64, _Ruchar)(long SignedValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertSToU, _i16_i64, _Rushort)(long SignedValue);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i32_i64, _Ruint)(long SignedValue);
ulong
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i64_i64, _Rulong)(long SignedValue);
char
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i8_i8, _Rchar)(uchar UnsignedValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertUToS, _i16_i8, _Rshort)(uchar UnsignedValue);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i32_i8, _Rint)(uchar UnsignedValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertUToS, _i64_i8, _Rlong)(uchar UnsignedValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertUToS, _i8_i16, _Rchar)(ushort UnsignedValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertUToS, _i16_i16, _Rshort)(ushort UnsignedValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertUToS, _i32_i16, _Rint)(ushort UnsignedValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertUToS, _i64_i16, _Rlong)(ushort UnsignedValue);
char
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i8_i32, _Rchar)(uint UnsignedValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertUToS, _i16_i32, _Rshort)(uint UnsignedValue);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i32_i32, _Rint)(uint UnsignedValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertUToS, _i64_i32, _Rlong)(uint UnsignedValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertUToS, _i8_i64, _Rchar)(ulong UnsignedValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertUToS, _i16_i64, _Rshort)(ulong UnsignedValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertUToS, _i32_i64, _Rint)(ulong UnsignedValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertUToS, _i64_i64, _Rlong)(ulong UnsignedValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_i8_f16, _Ruchar_rte)(half FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_i8_f16, _Ruchar_rtz)(half FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_i8_f16, _Ruchar_rtp)(half FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_i8_f16, _Ruchar_rtn)(half FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f16, _Ruchar_sat)(half FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i8_f16, _Ruchar_sat_rte)(half FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i8_f16, _Ruchar_sat_rtz)(half FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i8_f16, _Ruchar_sat_rtp)(half FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i8_f16, _Ruchar_sat_rtn)(half FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_i16_f16, _Rushort_rte)(half FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_i16_f16, _Rushort_rtz)(half FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_i16_f16, _Rushort_rtp)(half FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_i16_f16, _Rushort_rtn)(half FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f16, _Rushort_sat)(half FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i16_f16, _Rushort_sat_rte)(half FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i16_f16, _Rushort_sat_rtz)(half FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i16_f16, _Rushort_sat_rtp)(half FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i16_f16, _Rushort_sat_rtn)(half FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_i32_f16, _Ruint_rte)(half FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_i32_f16, _Ruint_rtz)(half FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_i32_f16, _Ruint_rtp)(half FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_i32_f16, _Ruint_rtn)(half FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f16, _Ruint_sat)(half FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i32_f16, _Ruint_sat_rte)(half FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i32_f16, _Ruint_sat_rtz)(half FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i32_f16, _Ruint_sat_rtp)(half FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i32_f16, _Ruint_sat_rtn)(half FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_i64_f16, _Rulong_rte)(half FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_i64_f16, _Rulong_rtz)(half FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_i64_f16, _Rulong_rtp)(half FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_i64_f16, _Rulong_rtn)(half FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f16, _Rulong_sat)(half FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i64_f16, _Rulong_sat_rte)(half FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i64_f16, _Rulong_sat_rtz)(half FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i64_f16, _Rulong_sat_rtp)(half FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i64_f16, _Rulong_sat_rtn)(half FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_i8_f32, _Ruchar_rte)(float FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_i8_f32, _Ruchar_rtz)(float FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_i8_f32, _Ruchar_rtp)(float FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_i8_f32, _Ruchar_rtn)(float FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f32, _Ruchar_sat)(float FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i8_f32, _Ruchar_sat_rte)(float FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i8_f32, _Ruchar_sat_rtz)(float FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i8_f32, _Ruchar_sat_rtp)(float FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i8_f32, _Ruchar_sat_rtn)(float FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_i16_f32, _Rushort_rte)(float FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_i16_f32, _Rushort_rtz)(float FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_i16_f32, _Rushort_rtp)(float FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_i16_f32, _Rushort_rtn)(float FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f32, _Rushort_sat)(float FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i16_f32, _Rushort_sat_rte)(float FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i16_f32, _Rushort_sat_rtz)(float FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i16_f32, _Rushort_sat_rtp)(float FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i16_f32, _Rushort_sat_rtn)(float FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_i32_f32, _Ruint_rte)(float FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_i32_f32, _Ruint_rtz)(float FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_i32_f32, _Ruint_rtp)(float FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_i32_f32, _Ruint_rtn)(float FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f32, _Ruint_sat)(float FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i32_f32, _Ruint_sat_rte)(float FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i32_f32, _Ruint_sat_rtz)(float FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i32_f32, _Ruint_sat_rtp)(float FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i32_f32, _Ruint_sat_rtn)(float FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_i64_f32, _Rulong_rte)(float FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_i64_f32, _Rulong_rtz)(float FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_i64_f32, _Rulong_rtp)(float FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_i64_f32, _Rulong_rtn)(float FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f32, _Rulong_sat)(float FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i64_f32, _Rulong_sat_rte)(float FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i64_f32, _Rulong_sat_rtz)(float FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i64_f32, _Rulong_sat_rtp)(float FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i64_f32, _Rulong_sat_rtn)(float FloatValue);
#if defined(cl_khr_fp64)
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_i8_f64, _Ruchar_rte)(double FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_i8_f64, _Ruchar_rtz)(double FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_i8_f64, _Ruchar_rtp)(double FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_i8_f64, _Ruchar_rtn)(double FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f64, _Ruchar_sat)(double FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i8_f64, _Ruchar_sat_rte)(double FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i8_f64, _Ruchar_sat_rtz)(double FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i8_f64, _Ruchar_sat_rtp)(double FloatValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i8_f64, _Ruchar_sat_rtn)(double FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_i16_f64, _Rushort_rte)(double FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_i16_f64, _Rushort_rtz)(double FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_i16_f64, _Rushort_rtp)(double FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_i16_f64, _Rushort_rtn)(double FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f64, _Rushort_sat)(double FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i16_f64, _Rushort_sat_rte)(double FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i16_f64, _Rushort_sat_rtz)(double FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i16_f64, _Rushort_sat_rtp)(double FloatValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i16_f64, _Rushort_sat_rtn)(double FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_i32_f64, _Ruint_rte)(double FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_i32_f64, _Ruint_rtz)(double FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_i32_f64, _Ruint_rtp)(double FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_i32_f64, _Ruint_rtn)(double FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f64, _Ruint_sat)(double FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i32_f64, _Ruint_sat_rte)(double FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i32_f64, _Ruint_sat_rtz)(double FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i32_f64, _Ruint_sat_rtp)(double FloatValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i32_f64, _Ruint_sat_rtn)(double FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_i64_f64, _Rulong_rte)(double FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_i64_f64, _Rulong_rtz)(double FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_i64_f64, _Rulong_rtp)(double FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_i64_f64, _Rulong_rtn)(double FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f64, _Rulong_sat)(double FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i64_f64, _Rulong_sat_rte)(double FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i64_f64, _Rulong_sat_rtz)(double FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i64_f64, _Rulong_sat_rtp)(double FloatValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i64_f64, _Rulong_sat_rtn)(double FloatValue);
#endif // defined(cl_khr_fp64)
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_i8_f16, _Rchar_rte)(half FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_i8_f16, _Rchar_rtz)(half FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_i8_f16, _Rchar_rtp)(half FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_i8_f16, _Rchar_rtn)(half FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f16, _Rchar_sat)(half FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i8_f16, _Rchar_sat_rte)(half FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i8_f16, _Rchar_sat_rtz)(half FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i8_f16, _Rchar_sat_rtp)(half FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i8_f16, _Rchar_sat_rtn)(half FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_i16_f16, _Rshort_rte)(half FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_i16_f16, _Rshort_rtz)(half FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_i16_f16, _Rshort_rtp)(half FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_i16_f16, _Rshort_rtn)(half FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f16, _Rshort_sat)(half FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i16_f16, _Rshort_sat_rte)(half FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i16_f16, _Rshort_sat_rtz)(half FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i16_f16, _Rshort_sat_rtp)(half FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i16_f16, _Rshort_sat_rtn)(half FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_i32_f16, _Rint_rte)(half FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_i32_f16, _Rint_rtz)(half FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_i32_f16, _Rint_rtp)(half FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_i32_f16, _Rint_rtn)(half FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f16, _Rint_sat)(half FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i32_f16, _Rint_sat_rte)(half FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i32_f16, _Rint_sat_rtz)(half FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i32_f16, _Rint_sat_rtp)(half FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i32_f16, _Rint_sat_rtn)(half FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_i64_f16, _Rlong_rte)(half FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_i64_f16, _Rlong_rtz)(half FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_i64_f16, _Rlong_rtp)(half FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_i64_f16, _Rlong_rtn)(half FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f16, _Rlong_sat)(half FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i64_f16, _Rlong_sat_rte)(half FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i64_f16, _Rlong_sat_rtz)(half FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i64_f16, _Rlong_sat_rtp)(half FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i64_f16, _Rlong_sat_rtn)(half FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_i8_f32, _Rchar_rte)(float FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_i8_f32, _Rchar_rtz)(float FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_i8_f32, _Rchar_rtp)(float FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_i8_f32, _Rchar_rtn)(float FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f32, _Rchar_sat)(float FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i8_f32, _Rchar_sat_rte)(float FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i8_f32, _Rchar_sat_rtz)(float FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i8_f32, _Rchar_sat_rtp)(float FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i8_f32, _Rchar_sat_rtn)(float FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_i16_f32, _Rshort_rte)(float FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_i16_f32, _Rshort_rtz)(float FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_i16_f32, _Rshort_rtp)(float FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_i16_f32, _Rshort_rtn)(float FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f32, _Rshort_sat)(float FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i16_f32, _Rshort_sat_rte)(float FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i16_f32, _Rshort_sat_rtz)(float FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i16_f32, _Rshort_sat_rtp)(float FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i16_f32, _Rshort_sat_rtn)(float FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_i32_f32, _Rint_rte)(float FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_i32_f32, _Rint_rtz)(float FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_i32_f32, _Rint_rtp)(float FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_i32_f32, _Rint_rtn)(float FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f32, _Rint_sat)(float FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i32_f32, _Rint_sat_rte)(float FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i32_f32, _Rint_sat_rtz)(float FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i32_f32, _Rint_sat_rtp)(float FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i32_f32, _Rint_sat_rtn)(float FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_i64_f32, _Rlong_rte)(float FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_i64_f32, _Rlong_rtz)(float FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_i64_f32, _Rlong_rtp)(float FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_i64_f32, _Rlong_rtn)(float FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f32, _Rlong_sat)(float FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i64_f32, _Rlong_sat_rte)(float FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i64_f32, _Rlong_sat_rtz)(float FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i64_f32, _Rlong_sat_rtp)(float FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i64_f32, _Rlong_sat_rtn)(float FloatValue);
#if defined(cl_khr_fp64)
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_i8_f64, _Rchar_rte)(double FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_i8_f64, _Rchar_rtz)(double FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_i8_f64, _Rchar_rtp)(double FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_i8_f64, _Rchar_rtn)(double FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f64, _Rchar_sat)(double FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i8_f64, _Rchar_sat_rte)(double FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i8_f64, _Rchar_sat_rtz)(double FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i8_f64, _Rchar_sat_rtp)(double FloatValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i8_f64, _Rchar_sat_rtn)(double FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_i16_f64, _Rshort_rte)(double FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_i16_f64, _Rshort_rtz)(double FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_i16_f64, _Rshort_rtp)(double FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_i16_f64, _Rshort_rtn)(double FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f64, _Rshort_sat)(double FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i16_f64, _Rshort_sat_rte)(double FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i16_f64, _Rshort_sat_rtz)(double FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i16_f64, _Rshort_sat_rtp)(double FloatValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i16_f64, _Rshort_sat_rtn)(double FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_i32_f64, _Rint_rte)(double FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_i32_f64, _Rint_rtz)(double FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_i32_f64, _Rint_rtp)(double FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_i32_f64, _Rint_rtn)(double FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f64, _Rint_sat)(double FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i32_f64, _Rint_sat_rte)(double FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i32_f64, _Rint_sat_rtz)(double FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i32_f64, _Rint_sat_rtp)(double FloatValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i32_f64, _Rint_sat_rtn)(double FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_i64_f64, _Rlong_rte)(double FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_i64_f64, _Rlong_rtz)(double FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_i64_f64, _Rlong_rtp)(double FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_i64_f64, _Rlong_rtn)(double FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f64, _Rlong_sat)(double FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i64_f64, _Rlong_sat_rte)(double FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i64_f64, _Rlong_sat_rtz)(double FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i64_f64, _Rlong_sat_rtp)(double FloatValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i64_f64, _Rlong_sat_rtn)(double FloatValue);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_f16_i8, _Rhalf_rte)(char SignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_f16_i8, _Rhalf_rtz)(char SignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_f16_i8, _Rhalf_rtp)(char SignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_f16_i8, _Rhalf_rtn)(char SignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_f32_i8, _Rfloat_rte)(char SignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_f32_i8, _Rfloat_rtz)(char SignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_f32_i8, _Rfloat_rtp)(char SignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_f32_i8, _Rfloat_rtn)(char SignedValue);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_f64_i8, _Rdouble_rte)(char SignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_f64_i8, _Rdouble_rtz)(char SignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_f64_i8, _Rdouble_rtp)(char SignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_f64_i8, _Rdouble_rtn)(char SignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_f64_i16, _Rdouble_rte)(short SignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_f64_i16, _Rdouble_rtz)(short SignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_f64_i16, _Rdouble_rtp)(short SignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_f64_i16, _Rdouble_rtn)(short SignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_f64_i32, _Rdouble_rte)(int SignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_f64_i32, _Rdouble_rtz)(int SignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_f64_i32, _Rdouble_rtp)(int SignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_f64_i32, _Rdouble_rtn)(int SignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_f64_i64, _Rdouble_rte)(long SignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_f64_i64, _Rdouble_rtz)(long SignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_f64_i64, _Rdouble_rtp)(long SignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_f64_i64, _Rdouble_rtn)(long SignedValue);
#endif // defined(cl_khr_fp64)
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_f16_i16, _Rhalf_rte)(short SignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_f16_i16, _Rhalf_rtz)(short SignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_f16_i16, _Rhalf_rtp)(short SignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_f16_i16, _Rhalf_rtn)(short SignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_f32_i16, _Rfloat_rte)(short SignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_f32_i16, _Rfloat_rtz)(short SignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_f32_i16, _Rfloat_rtp)(short SignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_f32_i16, _Rfloat_rtn)(short SignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_f16_i32, _Rhalf_rte)(int SignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_f16_i32, _Rhalf_rtz)(int SignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_f16_i32, _Rhalf_rtp)(int SignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_f16_i32, _Rhalf_rtn)(int SignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_f32_i32, _Rfloat_rte)(int SignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_f32_i32, _Rfloat_rtz)(int SignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_f32_i32, _Rfloat_rtp)(int SignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_f32_i32, _Rfloat_rtn)(int SignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_f16_i64, _Rhalf_rte)(long SignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_f16_i64, _Rhalf_rtz)(long SignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_f16_i64, _Rhalf_rtp)(long SignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_f16_i64, _Rhalf_rtn)(long SignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_f32_i64, _Rfloat_rte)(long SignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_f32_i64, _Rfloat_rtz)(long SignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_f32_i64, _Rfloat_rtp)(long SignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_f32_i64, _Rfloat_rtn)(long SignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_f16_i8, _Rhalf_rte)(uchar UnsignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_f16_i8, _Rhalf_rtz)(uchar UnsignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_f16_i8, _Rhalf_rtp)(uchar UnsignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_f16_i8, _Rhalf_rtn)(uchar UnsignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_f32_i8, _Rfloat_rte)(uchar UnsignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i8, _Rfloat_rtz)(uchar UnsignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i8, _Rfloat_rtp)(uchar UnsignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_f32_i8, _Rfloat_rtn)(uchar UnsignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_f16_i16, _Rhalf_rte)(ushort UnsignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_f16_i16, _Rhalf_rtz)(ushort UnsignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_f16_i16, _Rhalf_rtp)(ushort UnsignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_f16_i16, _Rhalf_rtn)(ushort UnsignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_f32_i16, _Rfloat_rte)(ushort UnsignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i16, _Rfloat_rtz)(ushort UnsignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i16, _Rfloat_rtp)(ushort UnsignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_f32_i16, _Rfloat_rtn)(ushort UnsignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_f16_i32, _Rhalf_rte)(uint UnsignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_f16_i32, _Rhalf_rtz)(uint UnsignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_f16_i32, _Rhalf_rtp)(uint UnsignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_f16_i32, _Rhalf_rtn)(uint UnsignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_f32_i32, _Rfloat_rte)(uint UnsignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i32, _Rfloat_rtz)(uint UnsignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i32, _Rfloat_rtp)(uint UnsignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_f32_i32, _Rfloat_rtn)(uint UnsignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_f16_i64, _Rhalf_rte)(ulong UnsignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_f16_i64, _Rhalf_rtz)(ulong UnsignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_f16_i64, _Rhalf_rtp)(ulong UnsignedValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_f16_i64, _Rhalf_rtn)(ulong UnsignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_f32_i64, _Rfloat_rte)(ulong UnsignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i64, _Rfloat_rtz)(ulong UnsignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i64, _Rfloat_rtp)(ulong UnsignedValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_f32_i64, _Rfloat_rtn)(ulong UnsignedValue);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_f64_i8, _Rdouble_rte)(uchar UnsignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_f64_i8, _Rdouble_rtz)(uchar UnsignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_f64_i8, _Rdouble_rtp)(uchar UnsignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_f64_i8, _Rdouble_rtn)(uchar UnsignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_f64_i16, _Rdouble_rte)(ushort UnsignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_f64_i16, _Rdouble_rtz)(ushort UnsignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_f64_i16, _Rdouble_rtp)(ushort UnsignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_f64_i16, _Rdouble_rtn)(ushort UnsignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_f64_i32, _Rdouble_rte)(uint UnsignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_f64_i32, _Rdouble_rtz)(uint UnsignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_f64_i32, _Rdouble_rtp)(uint UnsignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_f64_i32, _Rdouble_rtn)(uint UnsignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_f64_i64, _Rdouble_rte)(ulong UnsignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_f64_i64, _Rdouble_rtz)(ulong UnsignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_f64_i64, _Rdouble_rtp)(ulong UnsignedValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_f64_i64, _Rdouble_rtn)(ulong UnsignedValue);
#endif // defined(cl_khr_fp64)
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_i8_i8, _Ruchar_sat)(uchar UnsignedValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_i16_i8, _Rushort_sat)(uchar UnsignedValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_i32_i8, _Ruint_sat)(uchar UnsignedValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_i64_i8, _Rulong_sat)(uchar UnsignedValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_i8_i16, _Ruchar_sat)(ushort UnsignedValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_i16_i16, _Rushort_sat)(ushort UnsignedValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_i32_i16, _Ruint_sat)(ushort UnsignedValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_i64_i16, _Rulong_sat)(ushort UnsignedValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_i8_i32, _Ruchar_sat)(uint UnsignedValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_i16_i32, _Rushort_sat)(uint UnsignedValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_i32_i32, _Ruint_sat)(uint UnsignedValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_i64_i32, _Rulong_sat)(uint UnsignedValue);
uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_i8_i64, _Ruchar_sat)(ulong UnsignedValue);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_i16_i64, _Rushort_sat)(ulong UnsignedValue);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_i32_i64, _Ruint_sat)(ulong UnsignedValue);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_i64_i64, _Rulong_sat)(ulong UnsignedValue);
char SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i8_i8, _Rchar_sat)(char SignedValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SConvert, _Sat_i16_i8, _Rshort_sat)(char SignedValue);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i32_i8, _Rint_sat)(char SignedValue);
long
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i64_i8, _Rlong_sat)(char SignedValue);
char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SConvert, _Sat_i8_i16, _Rchar_sat)(short SignedValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SConvert, _Sat_i16_i16, _Rshort_sat)(short SignedValue);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SConvert, _Sat_i32_i16, _Rint_sat)(short SignedValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SConvert, _Sat_i64_i16, _Rlong_sat)(short SignedValue);
char SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i8_i32, _Rchar_sat)(int SignedValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SConvert, _Sat_i16_i32, _Rshort_sat)(int SignedValue);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i32_i32, _Rint_sat)(int SignedValue);
long
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i64_i32, _Rlong_sat)(int SignedValue);
char
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i8_i64, _Rchar_sat)(long SignedValue);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SConvert, _Sat_i16_i64, _Rshort_sat)(long SignedValue);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i32_i64, _Rint_sat)(long SignedValue);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SConvert, _Sat_i64_i64, _Rlong_sat)(long SignedValue);
half
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f16_f16, _Rhalf_rte)(half FloatValue);
half
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f16_f16, _Rhalf_rtz)(half FloatValue);
half
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f16_f16, _Rhalf_rtp)(half FloatValue);
half
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f16_f16, _Rhalf_rtn)(half FloatValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_f32_f16, _Rfloat_rte)(half FloatValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_f32_f16, _Rfloat_rtz)(half FloatValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_f32_f16, _Rfloat_rtp)(half FloatValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_f32_f16, _Rfloat_rtn)(half FloatValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_f16_f32, _Rhalf_rte)(float FloatValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _Rhalf_rtz)(float FloatValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _Rhalf_rtp)(float FloatValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _Rhalf_rtn)(float FloatValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_f32_f32, _Rfloat_rte)(float FloatValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_f32_f32, _Rfloat_rtz)(float FloatValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_f32_f32, _Rfloat_rtp)(float FloatValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_f32_f32, _Rfloat_rtn)(float FloatValue);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_f64_f16, _Rdouble_rte)(half FloatValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_f64_f16, _Rdouble_rtz)(half FloatValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_f64_f16, _Rdouble_rtp)(half FloatValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_f64_f16, _Rdouble_rtn)(half FloatValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_f64_f32, _Rdouble_rte)(float FloatValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_f64_f32, _Rdouble_rtz)(float FloatValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_f64_f32, _Rdouble_rtp)(float FloatValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_f64_f32, _Rdouble_rtn)(float FloatValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_f16_f64, _Rhalf_rte)(double FloatValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_f16_f64, _Rhalf_rtz)(double FloatValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_f16_f64, _Rhalf_rtp)(double FloatValue);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_f16_f64, _Rhalf_rtn)(double FloatValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_f32_f64, _Rfloat_rte)(double FloatValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_f32_f64, _Rfloat_rtz)(double FloatValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_f32_f64, _Rfloat_rtp)(double FloatValue);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_f32_f64, _Rfloat_rtn)(double FloatValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_f64_f64, _Rdouble_rte)(double FloatValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_f64_f64, _Rdouble_rtz)(double FloatValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_f64_f64, _Rdouble_rtp)(double FloatValue);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_f64_f64, _Rdouble_rtn)(double FloatValue);
#endif // defined(cl_khr_fp64)
half2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v2f16_v2i8, _Rhalf2)(char2 x);
half3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v3f16_v3i8, _Rhalf3)(char3 x);
half4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v4f16_v4i8, _Rhalf4)(char4 x);
half8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v8f16_v8i8, _Rhalf8)(char8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f16_v16i8, _Rhalf16)(char16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v2f16_v2i8, _Rhalf2_rte)(char2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v3f16_v3i8, _Rhalf3_rte)(char3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v4f16_v4i8, _Rhalf4_rte)(char4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v8f16_v8i8, _Rhalf8_rte)(char8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v16f16_v16i8, _Rhalf16_rte)(char16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f16_v2i8, _Rhalf2_rtz)(char2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f16_v3i8, _Rhalf3_rtz)(char3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f16_v4i8, _Rhalf4_rtz)(char4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f16_v8i8, _Rhalf8_rtz)(char8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f16_v16i8, _Rhalf16_rtz)(char16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v2f16_v2i8, _Rhalf2_rtp)(char2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v3f16_v3i8, _Rhalf3_rtp)(char3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v4f16_v4i8, _Rhalf4_rtp)(char4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v8f16_v8i8, _Rhalf8_rtp)(char8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v16f16_v16i8, _Rhalf16_rtp)(char16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v2f16_v2i8, _Rhalf2_rtn)(char2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v3f16_v3i8, _Rhalf3_rtn)(char3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v4f16_v4i8, _Rhalf4_rtn)(char4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v8f16_v8i8, _Rhalf8_rtn)(char8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v16f16_v16i8, _Rhalf16_rtn)(char16 x);
half2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v2f16_v2i16, _Rhalf2)(short2 x);
half3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v3f16_v3i16, _Rhalf3)(short3 x);
half4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v4f16_v4i16, _Rhalf4)(short4 x);
half8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v8f16_v8i16, _Rhalf8)(short8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f16_v16i16, _Rhalf16)(short16 x);
half2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v2f16_v2i16, _Rhalf2_rte)(short2 x);
half3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v3f16_v3i16, _Rhalf3_rte)(short3 x);
half4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v4f16_v4i16, _Rhalf4_rte)(short4 x);
half8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v8f16_v8i16, _Rhalf8_rte)(short8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v16f16_v16i16, _Rhalf16_rte)(short16 x);
half2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f16_v2i16, _Rhalf2_rtz)(short2 x);
half3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f16_v3i16, _Rhalf3_rtz)(short3 x);
half4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f16_v4i16, _Rhalf4_rtz)(short4 x);
half8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f16_v8i16, _Rhalf8_rtz)(short8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f16_v16i16, _Rhalf16_rtz)(short16 x);
half2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v2f16_v2i16, _Rhalf2_rtp)(short2 x);
half3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v3f16_v3i16, _Rhalf3_rtp)(short3 x);
half4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v4f16_v4i16, _Rhalf4_rtp)(short4 x);
half8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v8f16_v8i16, _Rhalf8_rtp)(short8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v16f16_v16i16, _Rhalf16_rtp)(short16 x);
half2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v2f16_v2i16, _Rhalf2_rtn)(short2 x);
half3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v3f16_v3i16, _Rhalf3_rtn)(short3 x);
half4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v4f16_v4i16, _Rhalf4_rtn)(short4 x);
half8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v8f16_v8i16, _Rhalf8_rtn)(short8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v16f16_v16i16, _Rhalf16_rtn)(short16 x);
half2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v2f16_v2i32, _Rhalf2)(int2 x);
half3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v3f16_v3i32, _Rhalf3)(int3 x);
half4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v4f16_v4i32, _Rhalf4)(int4 x);
half8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v8f16_v8i32, _Rhalf8)(int8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f16_v16i32, _Rhalf16)(int16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v2f16_v2i32, _Rhalf2_rte)(int2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v3f16_v3i32, _Rhalf3_rte)(int3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v4f16_v4i32, _Rhalf4_rte)(int4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v8f16_v8i32, _Rhalf8_rte)(int8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v16f16_v16i32, _Rhalf16_rte)(int16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f16_v2i32, _Rhalf2_rtz)(int2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f16_v3i32, _Rhalf3_rtz)(int3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f16_v4i32, _Rhalf4_rtz)(int4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f16_v8i32, _Rhalf8_rtz)(int8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f16_v16i32, _Rhalf16_rtz)(int16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v2f16_v2i32, _Rhalf2_rtp)(int2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v3f16_v3i32, _Rhalf3_rtp)(int3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v4f16_v4i32, _Rhalf4_rtp)(int4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v8f16_v8i32, _Rhalf8_rtp)(int8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v16f16_v16i32, _Rhalf16_rtp)(int16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v2f16_v2i32, _Rhalf2_rtn)(int2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v3f16_v3i32, _Rhalf3_rtn)(int3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v4f16_v4i32, _Rhalf4_rtn)(int4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v8f16_v8i32, _Rhalf8_rtn)(int8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v16f16_v16i32, _Rhalf16_rtn)(int16 x);
half2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v2f16_v2i64, _Rhalf2)(long2 x);
half3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v3f16_v3i64, _Rhalf3)(long3 x);
half4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v4f16_v4i64, _Rhalf4)(long4 x);
half8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v8f16_v8i64, _Rhalf8)(long8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f16_v16i64, _Rhalf16)(long16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v2f16_v2i64, _Rhalf2_rte)(long2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v3f16_v3i64, _Rhalf3_rte)(long3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v4f16_v4i64, _Rhalf4_rte)(long4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v8f16_v8i64, _Rhalf8_rte)(long8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v16f16_v16i64, _Rhalf16_rte)(long16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f16_v2i64, _Rhalf2_rtz)(long2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f16_v3i64, _Rhalf3_rtz)(long3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f16_v4i64, _Rhalf4_rtz)(long4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f16_v8i64, _Rhalf8_rtz)(long8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f16_v16i64, _Rhalf16_rtz)(long16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v2f16_v2i64, _Rhalf2_rtp)(long2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v3f16_v3i64, _Rhalf3_rtp)(long3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v4f16_v4i64, _Rhalf4_rtp)(long4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v8f16_v8i64, _Rhalf8_rtp)(long8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v16f16_v16i64, _Rhalf16_rtp)(long16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v2f16_v2i64, _Rhalf2_rtn)(long2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v3f16_v3i64, _Rhalf3_rtn)(long3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v4f16_v4i64, _Rhalf4_rtn)(long4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v8f16_v8i64, _Rhalf8_rtn)(long8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v16f16_v16i64, _Rhalf16_rtn)(long16 x);
float2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v2f32_v2i8, _Rfloat2)(char2 x);
float3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v3f32_v3i8, _Rfloat3)(char3 x);
float4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v4f32_v4i8, _Rfloat4)(char4 x);
float8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v8f32_v8i8, _Rfloat8)(char8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f32_v16i8, _Rfloat16)(char16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v2f32_v2i8, _Rfloat2_rte)(char2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v3f32_v3i8, _Rfloat3_rte)(char3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v4f32_v4i8, _Rfloat4_rte)(char4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v8f32_v8i8, _Rfloat8_rte)(char8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v16f32_v16i8, _Rfloat16_rte)(char16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f32_v2i8, _Rfloat2_rtz)(char2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f32_v3i8, _Rfloat3_rtz)(char3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f32_v4i8, _Rfloat4_rtz)(char4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f32_v8i8, _Rfloat8_rtz)(char8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f32_v16i8, _Rfloat16_rtz)(char16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v2f32_v2i8, _Rfloat2_rtp)(char2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v3f32_v3i8, _Rfloat3_rtp)(char3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v4f32_v4i8, _Rfloat4_rtp)(char4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v8f32_v8i8, _Rfloat8_rtp)(char8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v16f32_v16i8, _Rfloat16_rtp)(char16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v2f32_v2i8, _Rfloat2_rtn)(char2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v3f32_v3i8, _Rfloat3_rtn)(char3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v4f32_v4i8, _Rfloat4_rtn)(char4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v8f32_v8i8, _Rfloat8_rtn)(char8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v16f32_v16i8, _Rfloat16_rtn)(char16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v2f32_v2i16, _Rfloat2)(short2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v3f32_v3i16, _Rfloat3)(short3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v4f32_v4i16, _Rfloat4)(short4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v8f32_v8i16, _Rfloat8)(short8 x);
float16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f32_v16i16, _Rfloat16)(short16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v2f32_v2i16, _Rfloat2_rte)(short2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v3f32_v3i16, _Rfloat3_rte)(short3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v4f32_v4i16, _Rfloat4_rte)(short4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v8f32_v8i16, _Rfloat8_rte)(short8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v16f32_v16i16, _Rfloat16_rte)(short16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f32_v2i16, _Rfloat2_rtz)(short2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f32_v3i16, _Rfloat3_rtz)(short3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f32_v4i16, _Rfloat4_rtz)(short4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f32_v8i16, _Rfloat8_rtz)(short8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f32_v16i16, _Rfloat16_rtz)(short16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v2f32_v2i16, _Rfloat2_rtp)(short2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v3f32_v3i16, _Rfloat3_rtp)(short3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v4f32_v4i16, _Rfloat4_rtp)(short4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v8f32_v8i16, _Rfloat8_rtp)(short8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v16f32_v16i16, _Rfloat16_rtp)(short16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v2f32_v2i16, _Rfloat2_rtn)(short2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v3f32_v3i16, _Rfloat3_rtn)(short3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v4f32_v4i16, _Rfloat4_rtn)(short4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v8f32_v8i16, _Rfloat8_rtn)(short8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v16f32_v16i16, _Rfloat16_rtn)(short16 x);
float2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v2f32_v2i32, _Rfloat2)(int2 x);
float3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v3f32_v3i32, _Rfloat3)(int3 x);
float4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v4f32_v4i32, _Rfloat4)(int4 x);
float8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertSToF, _v8f32_v8i32, _Rfloat8)(int8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f32_v16i32, _Rfloat16)(int16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v2f32_v2i32, _Rfloat2_rte)(int2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v3f32_v3i32, _Rfloat3_rte)(int3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v4f32_v4i32, _Rfloat4_rte)(int4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v8f32_v8i32, _Rfloat8_rte)(int8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v16f32_v16i32, _Rfloat16_rte)(int16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f32_v2i32, _Rfloat2_rtz)(int2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f32_v3i32, _Rfloat3_rtz)(int3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f32_v4i32, _Rfloat4_rtz)(int4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f32_v8i32, _Rfloat8_rtz)(int8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f32_v16i32, _Rfloat16_rtz)(int16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v2f32_v2i32, _Rfloat2_rtp)(int2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v3f32_v3i32, _Rfloat3_rtp)(int3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v4f32_v4i32, _Rfloat4_rtp)(int4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v8f32_v8i32, _Rfloat8_rtp)(int8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v16f32_v16i32, _Rfloat16_rtp)(int16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v2f32_v2i32, _Rfloat2_rtn)(int2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v3f32_v3i32, _Rfloat3_rtn)(int3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v4f32_v4i32, _Rfloat4_rtn)(int4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v8f32_v8i32, _Rfloat8_rtn)(int8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v16f32_v16i32, _Rfloat16_rtn)(int16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v2f32_v2i64, _Rfloat2)(long2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v3f32_v3i64, _Rfloat3)(long3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v4f32_v4i64, _Rfloat4)(long4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v8f32_v8i64, _Rfloat8)(long8 x);
float16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f32_v16i64, _Rfloat16)(long16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v2f32_v2i64, _Rfloat2_rte)(long2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v3f32_v3i64, _Rfloat3_rte)(long3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v4f32_v4i64, _Rfloat4_rte)(long4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v8f32_v8i64, _Rfloat8_rte)(long8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v16f32_v16i64, _Rfloat16_rte)(long16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f32_v2i64, _Rfloat2_rtz)(long2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f32_v3i64, _Rfloat3_rtz)(long3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f32_v4i64, _Rfloat4_rtz)(long4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f32_v8i64, _Rfloat8_rtz)(long8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f32_v16i64, _Rfloat16_rtz)(long16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v2f32_v2i64, _Rfloat2_rtp)(long2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v3f32_v3i64, _Rfloat3_rtp)(long3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v4f32_v4i64, _Rfloat4_rtp)(long4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v8f32_v8i64, _Rfloat8_rtp)(long8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v16f32_v16i64, _Rfloat16_rtp)(long16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v2f32_v2i64, _Rfloat2_rtn)(long2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v3f32_v3i64, _Rfloat3_rtn)(long3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v4f32_v4i64, _Rfloat4_rtn)(long4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v8f32_v8i64, _Rfloat8_rtn)(long8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v16f32_v16i64, _Rfloat16_rtn)(long16 x);
#if defined(cl_khr_fp64)
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v2f64_v2i8, _Rdouble2)(char2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v3f64_v3i8, _Rdouble3)(char3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v4f64_v4i8, _Rdouble4)(char4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v8f64_v8i8, _Rdouble8)(char8 x);
double16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f64_v16i8, _Rdouble16)(char16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v2f64_v2i8, _Rdouble2_rte)(char2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v3f64_v3i8, _Rdouble3_rte)(char3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v4f64_v4i8, _Rdouble4_rte)(char4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v8f64_v8i8, _Rdouble8_rte)(char8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v16f64_v16i8, _Rdouble16_rte)(char16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f64_v2i8, _Rdouble2_rtz)(char2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f64_v3i8, _Rdouble3_rtz)(char3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f64_v4i8, _Rdouble4_rtz)(char4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f64_v8i8, _Rdouble8_rtz)(char8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f64_v16i8, _Rdouble16_rtz)(char16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v2f64_v2i8, _Rdouble2_rtp)(char2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v3f64_v3i8, _Rdouble3_rtp)(char3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v4f64_v4i8, _Rdouble4_rtp)(char4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v8f64_v8i8, _Rdouble8_rtp)(char8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v16f64_v16i8, _Rdouble16_rtp)(char16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v2f64_v2i8, _Rdouble2_rtn)(char2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v3f64_v3i8, _Rdouble3_rtn)(char3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v4f64_v4i8, _Rdouble4_rtn)(char4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v8f64_v8i8, _Rdouble8_rtn)(char8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v16f64_v16i8, _Rdouble16_rtn)(char16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v2f64_v2i16, _Rdouble2)(short2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v3f64_v3i16, _Rdouble3)(short3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v4f64_v4i16, _Rdouble4)(short4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v8f64_v8i16, _Rdouble8)(short8 x);
double16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f64_v16i16, _Rdouble16)(short16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v2f64_v2i16, _Rdouble2_rte)(short2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v3f64_v3i16, _Rdouble3_rte)(short3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v4f64_v4i16, _Rdouble4_rte)(short4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v8f64_v8i16, _Rdouble8_rte)(short8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v16f64_v16i16, _Rdouble16_rte)(short16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f64_v2i16, _Rdouble2_rtz)(short2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f64_v3i16, _Rdouble3_rtz)(short3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f64_v4i16, _Rdouble4_rtz)(short4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f64_v8i16, _Rdouble8_rtz)(short8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f64_v16i16, _Rdouble16_rtz)(short16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v2f64_v2i16, _Rdouble2_rtp)(short2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v3f64_v3i16, _Rdouble3_rtp)(short3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v4f64_v4i16, _Rdouble4_rtp)(short4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v8f64_v8i16, _Rdouble8_rtp)(short8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v16f64_v16i16, _Rdouble16_rtp)(short16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v2f64_v2i16, _Rdouble2_rtn)(short2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v3f64_v3i16, _Rdouble3_rtn)(short3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v4f64_v4i16, _Rdouble4_rtn)(short4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v8f64_v8i16, _Rdouble8_rtn)(short8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v16f64_v16i16, _Rdouble16_rtn)(short16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v2f64_v2i32, _Rdouble2)(int2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v3f64_v3i32, _Rdouble3)(int3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v4f64_v4i32, _Rdouble4)(int4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v8f64_v8i32, _Rdouble8)(int8 x);
double16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f64_v16i32, _Rdouble16)(int16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v2f64_v2i32, _Rdouble2_rte)(int2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v3f64_v3i32, _Rdouble3_rte)(int3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v4f64_v4i32, _Rdouble4_rte)(int4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v8f64_v8i32, _Rdouble8_rte)(int8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v16f64_v16i32, _Rdouble16_rte)(int16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f64_v2i32, _Rdouble2_rtz)(int2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f64_v3i32, _Rdouble3_rtz)(int3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f64_v4i32, _Rdouble4_rtz)(int4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f64_v8i32, _Rdouble8_rtz)(int8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f64_v16i32, _Rdouble16_rtz)(int16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v2f64_v2i32, _Rdouble2_rtp)(int2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v3f64_v3i32, _Rdouble3_rtp)(int3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v4f64_v4i32, _Rdouble4_rtp)(int4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v8f64_v8i32, _Rdouble8_rtp)(int8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v16f64_v16i32, _Rdouble16_rtp)(int16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v2f64_v2i32, _Rdouble2_rtn)(int2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v3f64_v3i32, _Rdouble3_rtn)(int3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v4f64_v4i32, _Rdouble4_rtn)(int4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v8f64_v8i32, _Rdouble8_rtn)(int8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v16f64_v16i32, _Rdouble16_rtn)(int16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v2f64_v2i64, _Rdouble2)(long2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v3f64_v3i64, _Rdouble3)(long3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v4f64_v4i64, _Rdouble4)(long4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v8f64_v8i64, _Rdouble8)(long8 x);
double16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f64_v16i64, _Rdouble16)(long16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v2f64_v2i64, _Rdouble2_rte)(long2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v3f64_v3i64, _Rdouble3_rte)(long3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v4f64_v4i64, _Rdouble4_rte)(long4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v8f64_v8i64, _Rdouble8_rte)(long8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTE_v16f64_v16i64, _Rdouble16_rte)(long16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f64_v2i64, _Rdouble2_rtz)(long2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f64_v3i64, _Rdouble3_rtz)(long3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f64_v4i64, _Rdouble4_rtz)(long4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f64_v8i64, _Rdouble8_rtz)(long8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f64_v16i64, _Rdouble16_rtz)(long16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v2f64_v2i64, _Rdouble2_rtp)(long2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v3f64_v3i64, _Rdouble3_rtp)(long3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v4f64_v4i64, _Rdouble4_rtp)(long4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v8f64_v8i64, _Rdouble8_rtp)(long8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTP_v16f64_v16i64, _Rdouble16_rtp)(long16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v2f64_v2i64, _Rdouble2_rtn)(long2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v3f64_v3i64, _Rdouble3_rtn)(long3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v4f64_v4i64, _Rdouble4_rtn)(long4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v8f64_v8i64, _Rdouble8_rtn)(long8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertSToF, _RTN_v16f64_v16i64, _Rdouble16_rtn)(long16 x);
#endif // defined(cl_khr_fp64)
half2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertUToF, _v2f16_v2i8, _Rhalf2)(uchar2 x);
half3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertUToF, _v3f16_v3i8, _Rhalf3)(uchar3 x);
half4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertUToF, _v4f16_v4i8, _Rhalf4)(uchar4 x);
half8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertUToF, _v8f16_v8i8, _Rhalf8)(uchar8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f16_v16i8, _Rhalf16)(uchar16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v2f16_v2i8, _Rhalf2_rte)(uchar2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v3f16_v3i8, _Rhalf3_rte)(uchar3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v4f16_v4i8, _Rhalf4_rte)(uchar4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v8f16_v8i8, _Rhalf8_rte)(uchar8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v16f16_v16i8, _Rhalf16_rte)(uchar16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f16_v2i8, _Rhalf2_rtz)(uchar2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f16_v3i8, _Rhalf3_rtz)(uchar3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f16_v4i8, _Rhalf4_rtz)(uchar4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f16_v8i8, _Rhalf8_rtz)(uchar8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f16_v16i8, _Rhalf16_rtz)(uchar16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v2f16_v2i8, _Rhalf2_rtp)(uchar2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v3f16_v3i8, _Rhalf3_rtp)(uchar3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v4f16_v4i8, _Rhalf4_rtp)(uchar4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v8f16_v8i8, _Rhalf8_rtp)(uchar8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v16f16_v16i8, _Rhalf16_rtp)(uchar16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v2f16_v2i8, _Rhalf2_rtn)(uchar2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v3f16_v3i8, _Rhalf3_rtn)(uchar3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v4f16_v4i8, _Rhalf4_rtn)(uchar4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v8f16_v8i8, _Rhalf8_rtn)(uchar8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v16f16_v16i8, _Rhalf16_rtn)(uchar16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f16_v2i16, _Rhalf2)(ushort2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f16_v3i16, _Rhalf3)(ushort3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f16_v4i16, _Rhalf4)(ushort4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f16_v8i16, _Rhalf8)(ushort8 x);
half16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f16_v16i16, _Rhalf16)(ushort16 x);
half2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v2f16_v2i16, _Rhalf2_rte)(ushort2 x);
half3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v3f16_v3i16, _Rhalf3_rte)(ushort3 x);
half4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v4f16_v4i16, _Rhalf4_rte)(ushort4 x);
half8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v8f16_v8i16, _Rhalf8_rte)(ushort8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v16f16_v16i16, _Rhalf16_rte)(ushort16 x);
half2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f16_v2i16, _Rhalf2_rtz)(ushort2 x);
half3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f16_v3i16, _Rhalf3_rtz)(ushort3 x);
half4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f16_v4i16, _Rhalf4_rtz)(ushort4 x);
half8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f16_v8i16, _Rhalf8_rtz)(ushort8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f16_v16i16, _Rhalf16_rtz)(ushort16 x);
half2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v2f16_v2i16, _Rhalf2_rtp)(ushort2 x);
half3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v3f16_v3i16, _Rhalf3_rtp)(ushort3 x);
half4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v4f16_v4i16, _Rhalf4_rtp)(ushort4 x);
half8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v8f16_v8i16, _Rhalf8_rtp)(ushort8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v16f16_v16i16, _Rhalf16_rtp)(ushort16 x);
half2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v2f16_v2i16, _Rhalf2_rtn)(ushort2 x);
half3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v3f16_v3i16, _Rhalf3_rtn)(ushort3 x);
half4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v4f16_v4i16, _Rhalf4_rtn)(ushort4 x);
half8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v8f16_v8i16, _Rhalf8_rtn)(ushort8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v16f16_v16i16, _Rhalf16_rtn)(ushort16 x);
half2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertUToF, _v2f16_v2i32, _Rhalf2)(uint2 x);
half3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertUToF, _v3f16_v3i32, _Rhalf3)(uint3 x);
half4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertUToF, _v4f16_v4i32, _Rhalf4)(uint4 x);
half8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertUToF, _v8f16_v8i32, _Rhalf8)(uint8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f16_v16i32, _Rhalf16)(uint16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v2f16_v2i32, _Rhalf2_rte)(uint2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v3f16_v3i32, _Rhalf3_rte)(uint3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v4f16_v4i32, _Rhalf4_rte)(uint4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v8f16_v8i32, _Rhalf8_rte)(uint8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v16f16_v16i32, _Rhalf16_rte)(uint16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f16_v2i32, _Rhalf2_rtz)(uint2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f16_v3i32, _Rhalf3_rtz)(uint3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f16_v4i32, _Rhalf4_rtz)(uint4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f16_v8i32, _Rhalf8_rtz)(uint8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f16_v16i32, _Rhalf16_rtz)(uint16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v2f16_v2i32, _Rhalf2_rtp)(uint2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v3f16_v3i32, _Rhalf3_rtp)(uint3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v4f16_v4i32, _Rhalf4_rtp)(uint4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v8f16_v8i32, _Rhalf8_rtp)(uint8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v16f16_v16i32, _Rhalf16_rtp)(uint16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v2f16_v2i32, _Rhalf2_rtn)(uint2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v3f16_v3i32, _Rhalf3_rtn)(uint3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v4f16_v4i32, _Rhalf4_rtn)(uint4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v8f16_v8i32, _Rhalf8_rtn)(uint8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v16f16_v16i32, _Rhalf16_rtn)(uint16 x);
half2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertUToF, _v2f16_v2i64, _Rhalf2)(ulong2 x);
half3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertUToF, _v3f16_v3i64, _Rhalf3)(ulong3 x);
half4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertUToF, _v4f16_v4i64, _Rhalf4)(ulong4 x);
half8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertUToF, _v8f16_v8i64, _Rhalf8)(ulong8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f16_v16i64, _Rhalf16)(ulong16 x);
half2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v2f16_v2i64, _Rhalf2_rte)(ulong2 x);
half3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v3f16_v3i64, _Rhalf3_rte)(ulong3 x);
half4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v4f16_v4i64, _Rhalf4_rte)(ulong4 x);
half8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v8f16_v8i64, _Rhalf8_rte)(ulong8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v16f16_v16i64, _Rhalf16_rte)(ulong16 x);
half2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f16_v2i64, _Rhalf2_rtz)(ulong2 x);
half3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f16_v3i64, _Rhalf3_rtz)(ulong3 x);
half4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f16_v4i64, _Rhalf4_rtz)(ulong4 x);
half8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f16_v8i64, _Rhalf8_rtz)(ulong8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f16_v16i64, _Rhalf16_rtz)(ulong16 x);
half2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v2f16_v2i64, _Rhalf2_rtp)(ulong2 x);
half3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v3f16_v3i64, _Rhalf3_rtp)(ulong3 x);
half4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v4f16_v4i64, _Rhalf4_rtp)(ulong4 x);
half8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v8f16_v8i64, _Rhalf8_rtp)(ulong8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v16f16_v16i64, _Rhalf16_rtp)(ulong16 x);
half2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v2f16_v2i64, _Rhalf2_rtn)(ulong2 x);
half3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v3f16_v3i64, _Rhalf3_rtn)(ulong3 x);
half4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v4f16_v4i64, _Rhalf4_rtn)(ulong4 x);
half8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v8f16_v8i64, _Rhalf8_rtn)(ulong8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v16f16_v16i64, _Rhalf16_rtn)(ulong16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f32_v2i8, _Rfloat2)(uchar2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f32_v3i8, _Rfloat3)(uchar3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f32_v4i8, _Rfloat4)(uchar4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f32_v8i8, _Rfloat8)(uchar8 x);
float16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f32_v16i8, _Rfloat16)(uchar16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v2f32_v2i8, _Rfloat2_rte)(uchar2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v3f32_v3i8, _Rfloat3_rte)(uchar3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v4f32_v4i8, _Rfloat4_rte)(uchar4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v8f32_v8i8, _Rfloat8_rte)(uchar8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v16f32_v16i8, _Rfloat16_rte)(uchar16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f32_v2i8, _Rfloat2_rtz)(uchar2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f32_v3i8, _Rfloat3_rtz)(uchar3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f32_v4i8, _Rfloat4_rtz)(uchar4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f32_v8i8, _Rfloat8_rtz)(uchar8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f32_v16i8, _Rfloat16_rtz)(uchar16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v2f32_v2i8, _Rfloat2_rtp)(uchar2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v3f32_v3i8, _Rfloat3_rtp)(uchar3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v4f32_v4i8, _Rfloat4_rtp)(uchar4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v8f32_v8i8, _Rfloat8_rtp)(uchar8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v16f32_v16i8, _Rfloat16_rtp)(uchar16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v2f32_v2i8, _Rfloat2_rtn)(uchar2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v3f32_v3i8, _Rfloat3_rtn)(uchar3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v4f32_v4i8, _Rfloat4_rtn)(uchar4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v8f32_v8i8, _Rfloat8_rtn)(uchar8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v16f32_v16i8, _Rfloat16_rtn)(uchar16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f32_v2i16, _Rfloat2)(ushort2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f32_v3i16, _Rfloat3)(ushort3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f32_v4i16, _Rfloat4)(ushort4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f32_v8i16, _Rfloat8)(ushort8 x);
float16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f32_v16i16, _Rfloat16)(ushort16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v2f32_v2i16, _Rfloat2_rte)(ushort2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v3f32_v3i16, _Rfloat3_rte)(ushort3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v4f32_v4i16, _Rfloat4_rte)(ushort4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v8f32_v8i16, _Rfloat8_rte)(ushort8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v16f32_v16i16, _Rfloat16_rte)(ushort16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f32_v2i16, _Rfloat2_rtz)(ushort2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f32_v3i16, _Rfloat3_rtz)(ushort3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f32_v4i16, _Rfloat4_rtz)(ushort4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f32_v8i16, _Rfloat8_rtz)(ushort8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f32_v16i16, _Rfloat16_rtz)(ushort16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v2f32_v2i16, _Rfloat2_rtp)(ushort2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v3f32_v3i16, _Rfloat3_rtp)(ushort3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v4f32_v4i16, _Rfloat4_rtp)(ushort4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v8f32_v8i16, _Rfloat8_rtp)(ushort8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v16f32_v16i16, _Rfloat16_rtp)(ushort16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v2f32_v2i16, _Rfloat2_rtn)(ushort2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v3f32_v3i16, _Rfloat3_rtn)(ushort3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v4f32_v4i16, _Rfloat4_rtn)(ushort4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v8f32_v8i16, _Rfloat8_rtn)(ushort8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v16f32_v16i16, _Rfloat16_rtn)(ushort16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f32_v2i32, _Rfloat2)(uint2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f32_v3i32, _Rfloat3)(uint3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f32_v4i32, _Rfloat4)(uint4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f32_v8i32, _Rfloat8)(uint8 x);
float16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f32_v16i32, _Rfloat16)(uint16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v2f32_v2i32, _Rfloat2_rte)(uint2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v3f32_v3i32, _Rfloat3_rte)(uint3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v4f32_v4i32, _Rfloat4_rte)(uint4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v8f32_v8i32, _Rfloat8_rte)(uint8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v16f32_v16i32, _Rfloat16_rte)(uint16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f32_v2i32, _Rfloat2_rtz)(uint2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f32_v3i32, _Rfloat3_rtz)(uint3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f32_v4i32, _Rfloat4_rtz)(uint4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f32_v8i32, _Rfloat8_rtz)(uint8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f32_v16i32, _Rfloat16_rtz)(uint16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v2f32_v2i32, _Rfloat2_rtp)(uint2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v3f32_v3i32, _Rfloat3_rtp)(uint3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v4f32_v4i32, _Rfloat4_rtp)(uint4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v8f32_v8i32, _Rfloat8_rtp)(uint8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v16f32_v16i32, _Rfloat16_rtp)(uint16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v2f32_v2i32, _Rfloat2_rtn)(uint2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v3f32_v3i32, _Rfloat3_rtn)(uint3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v4f32_v4i32, _Rfloat4_rtn)(uint4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v8f32_v8i32, _Rfloat8_rtn)(uint8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v16f32_v16i32, _Rfloat16_rtn)(uint16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f32_v2i64, _Rfloat2)(ulong2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f32_v3i64, _Rfloat3)(ulong3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f32_v4i64, _Rfloat4)(ulong4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f32_v8i64, _Rfloat8)(ulong8 x);
float16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f32_v16i64, _Rfloat16)(ulong16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v2f32_v2i64, _Rfloat2_rte)(ulong2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v3f32_v3i64, _Rfloat3_rte)(ulong3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v4f32_v4i64, _Rfloat4_rte)(ulong4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v8f32_v8i64, _Rfloat8_rte)(ulong8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v16f32_v16i64, _Rfloat16_rte)(ulong16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f32_v2i64, _Rfloat2_rtz)(ulong2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f32_v3i64, _Rfloat3_rtz)(ulong3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f32_v4i64, _Rfloat4_rtz)(ulong4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f32_v8i64, _Rfloat8_rtz)(ulong8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f32_v16i64, _Rfloat16_rtz)(ulong16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v2f32_v2i64, _Rfloat2_rtp)(ulong2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v3f32_v3i64, _Rfloat3_rtp)(ulong3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v4f32_v4i64, _Rfloat4_rtp)(ulong4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v8f32_v8i64, _Rfloat8_rtp)(ulong8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v16f32_v16i64, _Rfloat16_rtp)(ulong16 x);
float2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v2f32_v2i64, _Rfloat2_rtn)(ulong2 x);
float3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v3f32_v3i64, _Rfloat3_rtn)(ulong3 x);
float4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v4f32_v4i64, _Rfloat4_rtn)(ulong4 x);
float8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v8f32_v8i64, _Rfloat8_rtn)(ulong8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v16f32_v16i64, _Rfloat16_rtn)(ulong16 x);
#if defined(cl_khr_fp64)
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f64_v2i8, _Rdouble2)(uchar2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f64_v3i8, _Rdouble3)(uchar3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f64_v4i8, _Rdouble4)(uchar4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f64_v8i8, _Rdouble8)(uchar8 x);
double16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f64_v16i8, _Rdouble16)(uchar16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v2f64_v2i8, _Rdouble2_rte)(uchar2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v3f64_v3i8, _Rdouble3_rte)(uchar3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v4f64_v4i8, _Rdouble4_rte)(uchar4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v8f64_v8i8, _Rdouble8_rte)(uchar8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v16f64_v16i8, _Rdouble16_rte)(uchar16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f64_v2i8, _Rdouble2_rtz)(uchar2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f64_v3i8, _Rdouble3_rtz)(uchar3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f64_v4i8, _Rdouble4_rtz)(uchar4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f64_v8i8, _Rdouble8_rtz)(uchar8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f64_v16i8, _Rdouble16_rtz)(uchar16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v2f64_v2i8, _Rdouble2_rtp)(uchar2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v3f64_v3i8, _Rdouble3_rtp)(uchar3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v4f64_v4i8, _Rdouble4_rtp)(uchar4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v8f64_v8i8, _Rdouble8_rtp)(uchar8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v16f64_v16i8, _Rdouble16_rtp)(uchar16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v2f64_v2i8, _Rdouble2_rtn)(uchar2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v3f64_v3i8, _Rdouble3_rtn)(uchar3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v4f64_v4i8, _Rdouble4_rtn)(uchar4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v8f64_v8i8, _Rdouble8_rtn)(uchar8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v16f64_v16i8, _Rdouble16_rtn)(uchar16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f64_v2i16, _Rdouble2)(ushort2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f64_v3i16, _Rdouble3)(ushort3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f64_v4i16, _Rdouble4)(ushort4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f64_v8i16, _Rdouble8)(ushort8 x);
double16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f64_v16i16, _Rdouble16)(ushort16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v2f64_v2i16, _Rdouble2_rte)(ushort2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v3f64_v3i16, _Rdouble3_rte)(ushort3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v4f64_v4i16, _Rdouble4_rte)(ushort4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v8f64_v8i16, _Rdouble8_rte)(ushort8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v16f64_v16i16, _Rdouble16_rte)(ushort16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f64_v2i16, _Rdouble2_rtz)(ushort2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f64_v3i16, _Rdouble3_rtz)(ushort3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f64_v4i16, _Rdouble4_rtz)(ushort4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f64_v8i16, _Rdouble8_rtz)(ushort8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f64_v16i16, _Rdouble16_rtz)(ushort16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v2f64_v2i16, _Rdouble2_rtp)(ushort2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v3f64_v3i16, _Rdouble3_rtp)(ushort3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v4f64_v4i16, _Rdouble4_rtp)(ushort4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v8f64_v8i16, _Rdouble8_rtp)(ushort8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v16f64_v16i16, _Rdouble16_rtp)(ushort16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v2f64_v2i16, _Rdouble2_rtn)(ushort2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v3f64_v3i16, _Rdouble3_rtn)(ushort3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v4f64_v4i16, _Rdouble4_rtn)(ushort4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v8f64_v8i16, _Rdouble8_rtn)(ushort8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v16f64_v16i16, _Rdouble16_rtn)(ushort16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f64_v2i32, _Rdouble2)(uint2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f64_v3i32, _Rdouble3)(uint3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f64_v4i32, _Rdouble4)(uint4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f64_v8i32, _Rdouble8)(uint8 x);
double16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f64_v16i32, _Rdouble16)(uint16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v2f64_v2i32, _Rdouble2_rte)(uint2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v3f64_v3i32, _Rdouble3_rte)(uint3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v4f64_v4i32, _Rdouble4_rte)(uint4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v8f64_v8i32, _Rdouble8_rte)(uint8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v16f64_v16i32, _Rdouble16_rte)(uint16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f64_v2i32, _Rdouble2_rtz)(uint2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f64_v3i32, _Rdouble3_rtz)(uint3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f64_v4i32, _Rdouble4_rtz)(uint4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f64_v8i32, _Rdouble8_rtz)(uint8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f64_v16i32, _Rdouble16_rtz)(uint16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v2f64_v2i32, _Rdouble2_rtp)(uint2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v3f64_v3i32, _Rdouble3_rtp)(uint3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v4f64_v4i32, _Rdouble4_rtp)(uint4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v8f64_v8i32, _Rdouble8_rtp)(uint8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v16f64_v16i32, _Rdouble16_rtp)(uint16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v2f64_v2i32, _Rdouble2_rtn)(uint2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v3f64_v3i32, _Rdouble3_rtn)(uint3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v4f64_v4i32, _Rdouble4_rtn)(uint4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v8f64_v8i32, _Rdouble8_rtn)(uint8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v16f64_v16i32, _Rdouble16_rtn)(uint16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f64_v2i64, _Rdouble2)(ulong2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f64_v3i64, _Rdouble3)(ulong3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f64_v4i64, _Rdouble4)(ulong4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f64_v8i64, _Rdouble8)(ulong8 x);
double16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f64_v16i64, _Rdouble16)(ulong16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v2f64_v2i64, _Rdouble2_rte)(ulong2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v3f64_v3i64, _Rdouble3_rte)(ulong3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v4f64_v4i64, _Rdouble4_rte)(ulong4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v8f64_v8i64, _Rdouble8_rte)(ulong8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTE_v16f64_v16i64, _Rdouble16_rte)(ulong16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f64_v2i64, _Rdouble2_rtz)(ulong2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f64_v3i64, _Rdouble3_rtz)(ulong3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f64_v4i64, _Rdouble4_rtz)(ulong4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f64_v8i64, _Rdouble8_rtz)(ulong8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f64_v16i64, _Rdouble16_rtz)(ulong16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v2f64_v2i64, _Rdouble2_rtp)(ulong2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v3f64_v3i64, _Rdouble3_rtp)(ulong3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v4f64_v4i64, _Rdouble4_rtp)(ulong4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v8f64_v8i64, _Rdouble8_rtp)(ulong8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTP_v16f64_v16i64, _Rdouble16_rtp)(ulong16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v2f64_v2i64, _Rdouble2_rtn)(ulong2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v3f64_v3i64, _Rdouble3_rtn)(ulong3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v4f64_v4i64, _Rdouble4_rtn)(ulong4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v8f64_v8i64, _Rdouble8_rtn)(ulong8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertUToF, _RTN_v16f64_v16i64, _Rdouble16_rtn)(ulong16 x);
#endif // defined(cl_khr_fp64)
half2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v2f16_v2f32, _Rhalf2)(float2 x);
half3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v3f16_v3f32, _Rhalf3)(float3 x);
half4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v4f16_v4f32, _Rhalf4)(float4 x);
half8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v8f16_v8f32, _Rhalf8)(float8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v16f16_v16f32, _Rhalf16)(float16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v2f16_v2f32, _Rhalf2_rte)(float2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v3f16_v3f32, _Rhalf3_rte)(float3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v4f16_v4f32, _Rhalf4_rte)(float4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v8f16_v8f32, _Rhalf8_rte)(float8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_v16f16_v16f32, _Rhalf16_rte)(float16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v2f16_v2f32, _Rhalf2_rtz)(float2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v3f16_v3f32, _Rhalf3_rtz)(float3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v4f16_v4f32, _Rhalf4_rtz)(float4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v8f16_v8f32, _Rhalf8_rtz)(float8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_v16f16_v16f32, _Rhalf16_rtz)(float16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v2f16_v2f32, _Rhalf2_rtp)(float2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v3f16_v3f32, _Rhalf3_rtp)(float3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v4f16_v4f32, _Rhalf4_rtp)(float4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v8f16_v8f32, _Rhalf8_rtp)(float8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_v16f16_v16f32, _Rhalf16_rtp)(float16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v2f16_v2f32, _Rhalf2_rtn)(float2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v3f16_v3f32, _Rhalf3_rtn)(float3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v4f16_v4f32, _Rhalf4_rtn)(float4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v8f16_v8f32, _Rhalf8_rtn)(float8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_v16f16_v16f32, _Rhalf16_rtn)(float16 x);
half2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v2f16_v2f16, _Rhalf2)(half2 x);
half3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v3f16_v3f16, _Rhalf3)(half3 x);
half4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v4f16_v4f16, _Rhalf4)(half4 x);
half8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v8f16_v8f16, _Rhalf8)(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v16f16_v16f16, _Rhalf16)(half16 x);
half2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _RTE_v2f16_v2f16, _Rhalf2_rte)(half2 x);
half3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _RTE_v3f16_v3f16, _Rhalf3_rte)(half3 x);
half4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _RTE_v4f16_v4f16, _Rhalf4_rte)(half4 x);
half8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _RTE_v8f16_v8f16, _Rhalf8_rte)(half8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_v16f16_v16f16, _Rhalf16_rte)(half16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v2f16_v2f16, _Rhalf2_rtz)(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v3f16_v3f16, _Rhalf3_rtz)(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v4f16_v4f16, _Rhalf4_rtz)(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v8f16_v8f16, _Rhalf8_rtz)(half8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_v16f16_v16f16, _Rhalf16_rtz)(half16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v2f16_v2f16, _Rhalf2_rtp)(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v3f16_v3f16, _Rhalf3_rtp)(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v4f16_v4f16, _Rhalf4_rtp)(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v8f16_v8f16, _Rhalf8_rtp)(half8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_v16f16_v16f16, _Rhalf16_rtp)(half16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v2f16_v2f16, _Rhalf2_rtn)(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v3f16_v3f16, _Rhalf3_rtn)(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v4f16_v4f16, _Rhalf4_rtn)(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v8f16_v8f16, _Rhalf8_rtn)(half8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_v16f16_v16f16, _Rhalf16_rtn)(half16 x);
#if defined(cl_khr_fp64)
half2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v2f16_v2f64, _Rhalf2)(double2 x);
half3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v3f16_v3f64, _Rhalf3)(double3 x);
half4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v4f16_v4f64, _Rhalf4)(double4 x);
half8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v8f16_v8f64, _Rhalf8)(double8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v16f16_v16f64, _Rhalf16)(double16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v2f16_v2f64, _Rhalf2_rte)(double2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v3f16_v3f64, _Rhalf3_rte)(double3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v4f16_v4f64, _Rhalf4_rte)(double4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v8f16_v8f64, _Rhalf8_rte)(double8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_v16f16_v16f64, _Rhalf16_rte)(double16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v2f16_v2f64, _Rhalf2_rtz)(double2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v3f16_v3f64, _Rhalf3_rtz)(double3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v4f16_v4f64, _Rhalf4_rtz)(double4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v8f16_v8f64, _Rhalf8_rtz)(double8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_v16f16_v16f64, _Rhalf16_rtz)(double16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v2f16_v2f64, _Rhalf2_rtp)(double2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v3f16_v3f64, _Rhalf3_rtp)(double3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v4f16_v4f64, _Rhalf4_rtp)(double4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v8f16_v8f64, _Rhalf8_rtp)(double8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_v16f16_v16f64, _Rhalf16_rtp)(double16 x);
half2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v2f16_v2f64, _Rhalf2_rtn)(double2 x);
half3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v3f16_v3f64, _Rhalf3_rtn)(double3 x);
half4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v4f16_v4f64, _Rhalf4_rtn)(double4 x);
half8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v8f16_v8f64, _Rhalf8_rtn)(double8 x);
half16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_v16f16_v16f64, _Rhalf16_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
float2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v2f32_v2f32, _Rfloat2)(float2 x);
float3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v3f32_v3f32, _Rfloat3)(float3 x);
float4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v4f32_v4f32, _Rfloat4)(float4 x);
float8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v8f32_v8f32, _Rfloat8)(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v16f32_v16f32, _Rfloat16)(float16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v2f32_v2f32, _Rfloat2_rte)(float2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v3f32_v3f32, _Rfloat3_rte)(float3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v4f32_v4f32, _Rfloat4_rte)(float4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v8f32_v8f32, _Rfloat8_rte)(float8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_v16f32_v16f32, _Rfloat16_rte)(float16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v2f32_v2f32, _Rfloat2_rtz)(float2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v3f32_v3f32, _Rfloat3_rtz)(float3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v4f32_v4f32, _Rfloat4_rtz)(float4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v8f32_v8f32, _Rfloat8_rtz)(float8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_v16f32_v16f32, _Rfloat16_rtz)(float16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v2f32_v2f32, _Rfloat2_rtp)(float2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v3f32_v3f32, _Rfloat3_rtp)(float3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v4f32_v4f32, _Rfloat4_rtp)(float4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v8f32_v8f32, _Rfloat8_rtp)(float8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_v16f32_v16f32, _Rfloat16_rtp)(float16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v2f32_v2f32, _Rfloat2_rtn)(float2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v3f32_v3f32, _Rfloat3_rtn)(float3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v4f32_v4f32, _Rfloat4_rtn)(float4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v8f32_v8f32, _Rfloat8_rtn)(float8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_v16f32_v16f32, _Rfloat16_rtn)(float16 x);
float2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v2f32_v2f16, _Rfloat2)(half2 x);
float3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v3f32_v3f16, _Rfloat3)(half3 x);
float4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v4f32_v4f16, _Rfloat4)(half4 x);
float8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v8f32_v8f16, _Rfloat8)(half8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v16f32_v16f16, _Rfloat16)(half16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v2f32_v2f16, _Rfloat2_rte)(half2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v3f32_v3f16, _Rfloat3_rte)(half3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v4f32_v4f16, _Rfloat4_rte)(half4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v8f32_v8f16, _Rfloat8_rte)(half8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_v16f32_v16f16, _Rfloat16_rte)(half16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v2f32_v2f16, _Rfloat2_rtz)(half2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v3f32_v3f16, _Rfloat3_rtz)(half3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v4f32_v4f16, _Rfloat4_rtz)(half4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v8f32_v8f16, _Rfloat8_rtz)(half8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_v16f32_v16f16, _Rfloat16_rtz)(half16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v2f32_v2f16, _Rfloat2_rtp)(half2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v3f32_v3f16, _Rfloat3_rtp)(half3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v4f32_v4f16, _Rfloat4_rtp)(half4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v8f32_v8f16, _Rfloat8_rtp)(half8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_v16f32_v16f16, _Rfloat16_rtp)(half16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v2f32_v2f16, _Rfloat2_rtn)(half2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v3f32_v3f16, _Rfloat3_rtn)(half3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v4f32_v4f16, _Rfloat4_rtn)(half4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v8f32_v8f16, _Rfloat8_rtn)(half8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_v16f32_v16f16, _Rfloat16_rtn)(half16 x);
#if defined(cl_khr_fp64)
float2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v2f32_v2f64, _Rfloat2)(double2 x);
float3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v3f32_v3f64, _Rfloat3)(double3 x);
float4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v4f32_v4f64, _Rfloat4)(double4 x);
float8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v8f32_v8f64, _Rfloat8)(double8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v16f32_v16f64, _Rfloat16)(double16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v2f32_v2f64, _Rfloat2_rte)(double2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v3f32_v3f64, _Rfloat3_rte)(double3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v4f32_v4f64, _Rfloat4_rte)(double4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v8f32_v8f64, _Rfloat8_rte)(double8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_v16f32_v16f64, _Rfloat16_rte)(double16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v2f32_v2f64, _Rfloat2_rtz)(double2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v3f32_v3f64, _Rfloat3_rtz)(double3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v4f32_v4f64, _Rfloat4_rtz)(double4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v8f32_v8f64, _Rfloat8_rtz)(double8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_v16f32_v16f64, _Rfloat16_rtz)(double16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v2f32_v2f64, _Rfloat2_rtp)(double2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v3f32_v3f64, _Rfloat3_rtp)(double3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v4f32_v4f64, _Rfloat4_rtp)(double4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v8f32_v8f64, _Rfloat8_rtp)(double8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_v16f32_v16f64, _Rfloat16_rtp)(double16 x);
float2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v2f32_v2f64, _Rfloat2_rtn)(double2 x);
float3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v3f32_v3f64, _Rfloat3_rtn)(double3 x);
float4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v4f32_v4f64, _Rfloat4_rtn)(double4 x);
float8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v8f32_v8f64, _Rfloat8_rtn)(double8 x);
float16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_v16f32_v16f64, _Rfloat16_rtn)(double16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v2f64_v2f32, _Rdouble2)(float2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v3f64_v3f32, _Rdouble3)(float3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v4f64_v4f32, _Rdouble4)(float4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v8f64_v8f32, _Rdouble8)(float8 x);
double16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v16f64_v16f32, _Rdouble16)(float16 x);
double2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v2f64_v2f32, _Rdouble2_rte)(float2 x);
double3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v3f64_v3f32, _Rdouble3_rte)(float3 x);
double4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v4f64_v4f32, _Rdouble4_rte)(float4 x);
double8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v8f64_v8f32, _Rdouble8_rte)(float8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_v16f64_v16f32, _Rdouble16_rte)(float16 x);
double2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v2f64_v2f32, _Rdouble2_rtz)(float2 x);
double3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v3f64_v3f32, _Rdouble3_rtz)(float3 x);
double4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v4f64_v4f32, _Rdouble4_rtz)(float4 x);
double8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v8f64_v8f32, _Rdouble8_rtz)(float8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_v16f64_v16f32, _Rdouble16_rtz)(float16 x);
double2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v2f64_v2f32, _Rdouble2_rtp)(float2 x);
double3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v3f64_v3f32, _Rdouble3_rtp)(float3 x);
double4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v4f64_v4f32, _Rdouble4_rtp)(float4 x);
double8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v8f64_v8f32, _Rdouble8_rtp)(float8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_v16f64_v16f32, _Rdouble16_rtp)(float16 x);
double2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v2f64_v2f32, _Rdouble2_rtn)(float2 x);
double3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v3f64_v3f32, _Rdouble3_rtn)(float3 x);
double4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v4f64_v4f32, _Rdouble4_rtn)(float4 x);
double8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v8f64_v8f32, _Rdouble8_rtn)(float8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_v16f64_v16f32, _Rdouble16_rtn)(float16 x);
double2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v2f64_v2f16, _Rdouble2)(half2 x);
double3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v3f64_v3f16, _Rdouble3)(half3 x);
double4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v4f64_v4f16, _Rdouble4)(half4 x);
double8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(FConvert, _v8f64_v8f16, _Rdouble8)(half8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v16f64_v16f16, _Rdouble16)(half16 x);
double2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v2f64_v2f16, _Rdouble2_rte)(half2 x);
double3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v3f64_v3f16, _Rdouble3_rte)(half3 x);
double4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v4f64_v4f16, _Rdouble4_rte)(half4 x);
double8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v8f64_v8f16, _Rdouble8_rte)(half8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_v16f64_v16f16, _Rdouble16_rte)(half16 x);
double2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v2f64_v2f16, _Rdouble2_rtz)(half2 x);
double3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v3f64_v3f16, _Rdouble3_rtz)(half3 x);
double4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v4f64_v4f16, _Rdouble4_rtz)(half4 x);
double8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v8f64_v8f16, _Rdouble8_rtz)(half8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_v16f64_v16f16, _Rdouble16_rtz)(half16 x);
double2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v2f64_v2f16, _Rdouble2_rtp)(half2 x);
double3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v3f64_v3f16, _Rdouble3_rtp)(half3 x);
double4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v4f64_v4f16, _Rdouble4_rtp)(half4 x);
double8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v8f64_v8f16, _Rdouble8_rtp)(half8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_v16f64_v16f16, _Rdouble16_rtp)(half16 x);
double2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v2f64_v2f16, _Rdouble2_rtn)(half2 x);
double3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v3f64_v3f16, _Rdouble3_rtn)(half3 x);
double4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v4f64_v4f16, _Rdouble4_rtn)(half4 x);
double8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v8f64_v8f16, _Rdouble8_rtn)(half8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_v16f64_v16f16, _Rdouble16_rtn)(half16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v2f64_v2f64, _Rdouble2)(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v3f64_v3f64, _Rdouble3)(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v4f64_v4f64, _Rdouble4)(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v8f64_v8f64, _Rdouble8)(double8 x);
double16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v16f64_v16f64, _Rdouble16)(double16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_v2f64_v2f64, _Rdouble2_rte)(double2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_v3f64_v3f64, _Rdouble3_rte)(double3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_v4f64_v4f64, _Rdouble4_rte)(double4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_v8f64_v8f64, _Rdouble8_rte)(double8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTE_v16f64_v16f64, _Rdouble16_rte)(double16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_v2f64_v2f64, _Rdouble2_rtz)(double2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_v3f64_v3f64, _Rdouble3_rtz)(double3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_v4f64_v4f64, _Rdouble4_rtz)(double4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_v8f64_v8f64, _Rdouble8_rtz)(double8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTZ_v16f64_v16f64, _Rdouble16_rtz)(double16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_v2f64_v2f64, _Rdouble2_rtp)(double2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_v3f64_v3f64, _Rdouble3_rtp)(double3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_v4f64_v4f64, _Rdouble4_rtp)(double4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_v8f64_v8f64, _Rdouble8_rtp)(double8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTP_v16f64_v16f64, _Rdouble16_rtp)(double16 x);
double2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_v2f64_v2f64, _Rdouble2_rtn)(double2 x);
double3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_v3f64_v3f64, _Rdouble3_rtn)(double3 x);
double4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_v4f64_v4f64, _Rdouble4_rtn)(double4 x);
double8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_v8f64_v8f64, _Rdouble8_rtn)(double8 x);
double16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(FConvert, _RTN_v16f64_v16f64, _Rdouble16_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
char2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v2i8_v2i8, _Rchar2)(char2 x);
char3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v3i8_v3i8, _Rchar3)(char3 x);
char4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v4i8_v4i8, _Rchar4)(char4 x);
char8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v8i8_v8i8, _Rchar8)(char8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i8_v16i8, _Rchar16)(char16 x);
char2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v2i8_v2i8, _Rchar2_sat)(char2 x);
char3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v3i8_v3i8, _Rchar3_sat)(char3 x);
char4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v4i8_v4i8, _Rchar4_sat)(char4 x);
char8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v8i8_v8i8, _Rchar8_sat)(char8 x);
char16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i8_v16i8, _Rchar16_sat)(char16 x);
char2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v2i8_v2i16, _Rchar2)(short2 x);
char3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v3i8_v3i16, _Rchar3)(short3 x);
char4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v4i8_v4i16, _Rchar4)(short4 x);
char8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v8i8_v8i16, _Rchar8)(short8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i8_v16i16, _Rchar16)(short16 x);
char2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v2i8_v2i16, _Rchar2_sat)(short2 x);
char3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v3i8_v3i16, _Rchar3_sat)(short3 x);
char4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v4i8_v4i16, _Rchar4_sat)(short4 x);
char8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v8i8_v8i16, _Rchar8_sat)(short8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SConvert, _Sat_v16i8_v16i16, _Rchar16_sat)(short16 x);
char2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v2i8_v2i32, _Rchar2)(int2 x);
char3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v3i8_v3i32, _Rchar3)(int3 x);
char4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v4i8_v4i32, _Rchar4)(int4 x);
char8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v8i8_v8i32, _Rchar8)(int8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i8_v16i32, _Rchar16)(int16 x);
char2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v2i8_v2i32, _Rchar2_sat)(int2 x);
char3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v3i8_v3i32, _Rchar3_sat)(int3 x);
char4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v4i8_v4i32, _Rchar4_sat)(int4 x);
char8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v8i8_v8i32, _Rchar8_sat)(int8 x);
char16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i8_v16i32, _Rchar16_sat)(int16 x);
char2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v2i8_v2i64, _Rchar2)(long2 x);
char3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v3i8_v3i64, _Rchar3)(long3 x);
char4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v4i8_v4i64, _Rchar4)(long4 x);
char8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v8i8_v8i64, _Rchar8)(long8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i8_v16i64, _Rchar16)(long16 x);
char2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v2i8_v2i64, _Rchar2_sat)(long2 x);
char3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v3i8_v3i64, _Rchar3_sat)(long3 x);
char4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v4i8_v4i64, _Rchar4_sat)(long4 x);
char8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v8i8_v8i64, _Rchar8_sat)(long8 x);
char16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i8_v16i64, _Rchar16_sat)(long16 x);
short2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v2i16_v2i8, _Rshort2)(char2 x);
short3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v3i16_v3i8, _Rshort3)(char3 x);
short4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v4i16_v4i8, _Rshort4)(char4 x);
short8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v8i16_v8i8, _Rshort8)(char8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i16_v16i8, _Rshort16)(char16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i16_v2i8, _Rshort2_sat)(char2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i16_v3i8, _Rshort3_sat)(char3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i16_v4i8, _Rshort4_sat)(char4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i16_v8i8, _Rshort8_sat)(char8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SConvert, _Sat_v16i16_v16i8, _Rshort16_sat)(char16 x);
short2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v2i16_v2i16, _Rshort2)(short2 x);
short3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v3i16_v3i16, _Rshort3)(short3 x);
short4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v4i16_v4i16, _Rshort4)(short4 x);
short8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v8i16_v8i16, _Rshort8)(short8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i16_v16i16, _Rshort16)(short16 x);
short2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i16_v2i16, _Rshort2_sat)(short2 x);
short3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i16_v3i16, _Rshort3_sat)(short3 x);
short4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i16_v4i16, _Rshort4_sat)(short4 x);
short8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i16_v8i16, _Rshort8_sat)(short8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SConvert, _Sat_v16i16_v16i16, _Rshort16_sat)(short16 x);
short2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v2i16_v2i32, _Rshort2)(int2 x);
short3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v3i16_v3i32, _Rshort3)(int3 x);
short4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v4i16_v4i32, _Rshort4)(int4 x);
short8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v8i16_v8i32, _Rshort8)(int8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i16_v16i32, _Rshort16)(int16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i16_v2i32, _Rshort2_sat)(int2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i16_v3i32, _Rshort3_sat)(int3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i16_v4i32, _Rshort4_sat)(int4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i16_v8i32, _Rshort8_sat)(int8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SConvert, _Sat_v16i16_v16i32, _Rshort16_sat)(int16 x);
short2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v2i16_v2i64, _Rshort2)(long2 x);
short3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v3i16_v3i64, _Rshort3)(long3 x);
short4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v4i16_v4i64, _Rshort4)(long4 x);
short8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v8i16_v8i64, _Rshort8)(long8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i16_v16i64, _Rshort16)(long16 x);
short2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i16_v2i64, _Rshort2_sat)(long2 x);
short3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i16_v3i64, _Rshort3_sat)(long3 x);
short4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i16_v4i64, _Rshort4_sat)(long4 x);
short8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i16_v8i64, _Rshort8_sat)(long8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SConvert, _Sat_v16i16_v16i64, _Rshort16_sat)(long16 x);
int2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v2i32_v2i8, _Rint2)(char2 x);
int3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v3i32_v3i8, _Rint3)(char3 x);
int4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v4i32_v4i8, _Rint4)(char4 x);
int8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v8i32_v8i8, _Rint8)(char8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i32_v16i8, _Rint16)(char16 x);
int2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v2i32_v2i8, _Rint2_sat)(char2 x);
int3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v3i32_v3i8, _Rint3_sat)(char3 x);
int4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v4i32_v4i8, _Rint4_sat)(char4 x);
int8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v8i32_v8i8, _Rint8_sat)(char8 x);
int16
    SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v16i32_v16i8, _Rint16_sat)(char16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v2i32_v2i16, _Rint2)(short2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v3i32_v3i16, _Rint3)(short3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v4i32_v4i16, _Rint4)(short4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v8i32_v8i16, _Rint8)(short8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i32_v16i16, _Rint16)(short16 x);
int2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v2i32_v2i16, _Rint2_sat)(short2 x);
int3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v3i32_v3i16, _Rint3_sat)(short3 x);
int4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v4i32_v4i16, _Rint4_sat)(short4 x);
int8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v8i32_v8i16, _Rint8_sat)(short8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SConvert, _Sat_v16i32_v16i16, _Rint16_sat)(short16 x);
int2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v2i32_v2i32, _Rint2)(int2 x);
int3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v3i32_v3i32, _Rint3)(int3 x);
int4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v4i32_v4i32, _Rint4)(int4 x);
int8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v8i32_v8i32, _Rint8)(int8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i32_v16i32, _Rint16)(int16 x);
int2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v2i32_v2i32, _Rint2_sat)(int2 x);
int3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v3i32_v3i32, _Rint3_sat)(int3 x);
int4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v4i32_v4i32, _Rint4_sat)(int4 x);
int8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v8i32_v8i32, _Rint8_sat)(int8 x);
int16
    SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v16i32_v16i32, _Rint16_sat)(int16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v2i32_v2i64, _Rint2)(long2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v3i32_v3i64, _Rint3)(long3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v4i32_v4i64, _Rint4)(long4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v8i32_v8i64, _Rint8)(long8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i32_v16i64, _Rint16)(long16 x);
int2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v2i32_v2i64, _Rint2_sat)(long2 x);
int3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v3i32_v3i64, _Rint3_sat)(long3 x);
int4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v4i32_v4i64, _Rint4_sat)(long4 x);
int8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v8i32_v8i64, _Rint8_sat)(long8 x);
int16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i32_v16i64, _Rint16_sat)(long16 x);
long2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v2i64_v2i8, _Rlong2)(char2 x);
long3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v3i64_v3i8, _Rlong3)(char3 x);
long4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v4i64_v4i8, _Rlong4)(char4 x);
long8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v8i64_v8i8, _Rlong8)(char8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i64_v16i8, _Rlong16)(char16 x);
long2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v2i64_v2i8, _Rlong2_sat)(char2 x);
long3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v3i64_v3i8, _Rlong3_sat)(char3 x);
long4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v4i64_v4i8, _Rlong4_sat)(char4 x);
long8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v8i64_v8i8, _Rlong8_sat)(char8 x);
long16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i64_v16i8, _Rlong16_sat)(char16 x);
long2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v2i64_v2i16, _Rlong2)(short2 x);
long3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v3i64_v3i16, _Rlong3)(short3 x);
long4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v4i64_v4i16, _Rlong4)(short4 x);
long8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v8i64_v8i16, _Rlong8)(short8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i64_v16i16, _Rlong16)(short16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i64_v2i16, _Rlong2_sat)(short2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i64_v3i16, _Rlong3_sat)(short3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i64_v4i16, _Rlong4_sat)(short4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i64_v8i16, _Rlong8_sat)(short8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SConvert, _Sat_v16i64_v16i16, _Rlong16_sat)(short16 x);
long2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v2i64_v2i32, _Rlong2)(int2 x);
long3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v3i64_v3i32, _Rlong3)(int3 x);
long4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v4i64_v4i32, _Rlong4)(int4 x);
long8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v8i64_v8i32, _Rlong8)(int8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i64_v16i32, _Rlong16)(int16 x);
long2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v2i64_v2i32, _Rlong2_sat)(int2 x);
long3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v3i64_v3i32, _Rlong3_sat)(int3 x);
long4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v4i64_v4i32, _Rlong4_sat)(int4 x);
long8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v8i64_v8i32, _Rlong8_sat)(int8 x);
long16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i64_v16i32, _Rlong16_sat)(int16 x);
long2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v2i64_v2i64, _Rlong2)(long2 x);
long3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v3i64_v3i64, _Rlong3)(long3 x);
long4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v4i64_v4i64, _Rlong4)(long4 x);
long8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _v8i64_v8i64, _Rlong8)(long8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i64_v16i64, _Rlong16)(long16 x);
long2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v2i64_v2i64, _Rlong2_sat)(long2 x);
long3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v3i64_v3i64, _Rlong3_sat)(long3 x);
long4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v4i64_v4i64, _Rlong4_sat)(long4 x);
long8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SConvert, _Sat_v8i64_v8i64, _Rlong8_sat)(long8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SConvert, _Sat_v16i64_v16i64, _Rlong16_sat)(long16 x);
uchar2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v2i8_v2i8, _Ruchar2)(uchar2 x);
uchar3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v3i8_v3i8, _Ruchar3)(uchar3 x);
uchar4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v4i8_v4i8, _Ruchar4)(uchar4 x);
uchar8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v8i8_v8i8, _Ruchar8)(uchar8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i8_v16i8, _Ruchar16)(uchar16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i8_v2i8, _Ruchar2_sat)(uchar2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i8_v3i8, _Ruchar3_sat)(uchar3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i8_v4i8, _Ruchar4_sat)(uchar4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i8_v8i8, _Ruchar8_sat)(uchar8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v16i8_v16i8, _Ruchar16_sat)(uchar16 x);
uchar2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v2i8_v2i16, _Ruchar2)(ushort2 x);
uchar3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v3i8_v3i16, _Ruchar3)(ushort3 x);
uchar4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v4i8_v4i16, _Ruchar4)(ushort4 x);
uchar8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v8i8_v8i16, _Ruchar8)(ushort8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i8_v16i16, _Ruchar16)(ushort16 x);
uchar2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i8_v2i16, _Ruchar2_sat)(ushort2 x);
uchar3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i8_v3i16, _Ruchar3_sat)(ushort3 x);
uchar4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i8_v4i16, _Ruchar4_sat)(ushort4 x);
uchar8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i8_v8i16, _Ruchar8_sat)(ushort8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v16i8_v16i16, _Ruchar16_sat)(ushort16 x);
uchar2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v2i8_v2i32, _Ruchar2)(uint2 x);
uchar3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v3i8_v3i32, _Ruchar3)(uint3 x);
uchar4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v4i8_v4i32, _Ruchar4)(uint4 x);
uchar8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v8i8_v8i32, _Ruchar8)(uint8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i8_v16i32, _Ruchar16)(uint16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i8_v2i32, _Ruchar2_sat)(uint2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i8_v3i32, _Ruchar3_sat)(uint3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i8_v4i32, _Ruchar4_sat)(uint4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i8_v8i32, _Ruchar8_sat)(uint8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v16i8_v16i32, _Ruchar16_sat)(uint16 x);
uchar2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v2i8_v2i64, _Ruchar2)(ulong2 x);
uchar3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v3i8_v3i64, _Ruchar3)(ulong3 x);
uchar4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v4i8_v4i64, _Ruchar4)(ulong4 x);
uchar8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v8i8_v8i64, _Ruchar8)(ulong8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i8_v16i64, _Ruchar16)(ulong16 x);
uchar2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i8_v2i64, _Ruchar2_sat)(ulong2 x);
uchar3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i8_v3i64, _Ruchar3_sat)(ulong3 x);
uchar4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i8_v4i64, _Ruchar4_sat)(ulong4 x);
uchar8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i8_v8i64, _Ruchar8_sat)(ulong8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v16i8_v16i64, _Ruchar16_sat)(ulong16 x);
ushort2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v2i16_v2i8, _Rushort2)(uchar2 x);
ushort3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v3i16_v3i8, _Rushort3)(uchar3 x);
ushort4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v4i16_v4i8, _Rushort4)(uchar4 x);
ushort8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v8i16_v8i8, _Rushort8)(uchar8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i16_v16i8, _Rushort16)(uchar16 x);
ushort2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i16_v2i8, _Rushort2_sat)(uchar2 x);
ushort3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i16_v3i8, _Rushort3_sat)(uchar3 x);
ushort4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i16_v4i8, _Rushort4_sat)(uchar4 x);
ushort8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i16_v8i8, _Rushort8_sat)(uchar8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v16i16_v16i8, _Rushort16_sat)(uchar16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v2i16_v2i16, _Rushort2)(ushort2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v3i16_v3i16, _Rushort3)(ushort3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v4i16_v4i16, _Rushort4)(ushort4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v8i16_v8i16, _Rushort8)(ushort8 x);
ushort16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i16_v16i16, _Rushort16)(ushort16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v2i16_v2i16, _Rushort2_sat)(ushort2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v3i16_v3i16, _Rushort3_sat)(ushort3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v4i16_v4i16, _Rushort4_sat)(ushort4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v8i16_v8i16, _Rushort8_sat)(ushort8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v16i16_v16i16, _Rushort16_sat)(ushort16 x);
ushort2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v2i16_v2i32, _Rushort2)(uint2 x);
ushort3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v3i16_v3i32, _Rushort3)(uint3 x);
ushort4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v4i16_v4i32, _Rushort4)(uint4 x);
ushort8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v8i16_v8i32, _Rushort8)(uint8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i16_v16i32, _Rushort16)(uint16 x);
ushort2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i16_v2i32, _Rushort2_sat)(uint2 x);
ushort3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i16_v3i32, _Rushort3_sat)(uint3 x);
ushort4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i16_v4i32, _Rushort4_sat)(uint4 x);
ushort8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i16_v8i32, _Rushort8_sat)(uint8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v16i16_v16i32, _Rushort16_sat)(uint16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v2i16_v2i64, _Rushort2)(ulong2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v3i16_v3i64, _Rushort3)(ulong3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v4i16_v4i64, _Rushort4)(ulong4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v8i16_v8i64, _Rushort8)(ulong8 x);
ushort16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i16_v16i64, _Rushort16)(ulong16 x);
ushort2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i16_v2i64, _Rushort2_sat)(ulong2 x);
ushort3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i16_v3i64, _Rushort3_sat)(ulong3 x);
ushort4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i16_v4i64, _Rushort4_sat)(ulong4 x);
ushort8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i16_v8i64, _Rushort8_sat)(ulong8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v16i16_v16i64, _Rushort16_sat)(ulong16 x);
uint2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v2i32_v2i8, _Ruint2)(uchar2 x);
uint3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v3i32_v3i8, _Ruint3)(uchar3 x);
uint4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v4i32_v4i8, _Ruint4)(uchar4 x);
uint8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v8i32_v8i8, _Ruint8)(uchar8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i32_v16i8, _Ruint16)(uchar16 x);
uint2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _Sat_v2i32_v2i8, _Ruint2_sat)(uchar2 x);
uint3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _Sat_v3i32_v3i8, _Ruint3_sat)(uchar3 x);
uint4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _Sat_v4i32_v4i8, _Ruint4_sat)(uchar4 x);
uint8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _Sat_v8i32_v8i8, _Ruint8_sat)(uchar8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v16i32_v16i8, _Ruint16_sat)(uchar16 x);
uint2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v2i32_v2i16, _Ruint2)(ushort2 x);
uint3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v3i32_v3i16, _Ruint3)(ushort3 x);
uint4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v4i32_v4i16, _Ruint4)(ushort4 x);
uint8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v8i32_v8i16, _Ruint8)(ushort8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i32_v16i16, _Ruint16)(ushort16 x);
uint2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i32_v2i16, _Ruint2_sat)(ushort2 x);
uint3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i32_v3i16, _Ruint3_sat)(ushort3 x);
uint4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i32_v4i16, _Ruint4_sat)(ushort4 x);
uint8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i32_v8i16, _Ruint8_sat)(ushort8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v16i32_v16i16, _Ruint16_sat)(ushort16 x);
uint2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v2i32_v2i32, _Ruint2)(uint2 x);
uint3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v3i32_v3i32, _Ruint3)(uint3 x);
uint4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v4i32_v4i32, _Ruint4)(uint4 x);
uint8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v8i32_v8i32, _Ruint8)(uint8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i32_v16i32, _Ruint16)(uint16 x);
uint2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _Sat_v2i32_v2i32, _Ruint2_sat)(uint2 x);
uint3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _Sat_v3i32_v3i32, _Ruint3_sat)(uint3 x);
uint4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _Sat_v4i32_v4i32, _Ruint4_sat)(uint4 x);
uint8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _Sat_v8i32_v8i32, _Ruint8_sat)(uint8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v16i32_v16i32, _Ruint16_sat)(uint16 x);
uint2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v2i32_v2i64, _Ruint2)(ulong2 x);
uint3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v3i32_v3i64, _Ruint3)(ulong3 x);
uint4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v4i32_v4i64, _Ruint4)(ulong4 x);
uint8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v8i32_v8i64, _Ruint8)(ulong8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i32_v16i64, _Ruint16)(ulong16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i32_v2i64, _Ruint2_sat)(ulong2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i32_v3i64, _Ruint3_sat)(ulong3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i32_v4i64, _Ruint4_sat)(ulong4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i32_v8i64, _Ruint8_sat)(ulong8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v16i32_v16i64, _Ruint16_sat)(ulong16 x);
ulong2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v2i64_v2i8, _Rulong2)(uchar2 x);
ulong3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v3i64_v3i8, _Rulong3)(uchar3 x);
ulong4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v4i64_v4i8, _Rulong4)(uchar4 x);
ulong8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v8i64_v8i8, _Rulong8)(uchar8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i64_v16i8, _Rulong16)(uchar16 x);
ulong2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i64_v2i8, _Rulong2_sat)(uchar2 x);
ulong3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i64_v3i8, _Rulong3_sat)(uchar3 x);
ulong4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i64_v4i8, _Rulong4_sat)(uchar4 x);
ulong8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i64_v8i8, _Rulong8_sat)(uchar8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v16i64_v16i8, _Rulong16_sat)(uchar16 x);
ulong2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v2i64_v2i16, _Rulong2)(ushort2 x);
ulong3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v3i64_v3i16, _Rulong3)(ushort3 x);
ulong4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v4i64_v4i16, _Rulong4)(ushort4 x);
ulong8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v8i64_v8i16, _Rulong8)(ushort8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i64_v16i16, _Rulong16)(ushort16 x);
ulong2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i64_v2i16, _Rulong2_sat)(ushort2 x);
ulong3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i64_v3i16, _Rulong3_sat)(ushort3 x);
ulong4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i64_v4i16, _Rulong4_sat)(ushort4 x);
ulong8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i64_v8i16, _Rulong8_sat)(ushort8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v16i64_v16i16, _Rulong16_sat)(ushort16 x);
ulong2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v2i64_v2i32, _Rulong2)(uint2 x);
ulong3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v3i64_v3i32, _Rulong3)(uint3 x);
ulong4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v4i64_v4i32, _Rulong4)(uint4 x);
ulong8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v8i64_v8i32, _Rulong8)(uint8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i64_v16i32, _Rulong16)(uint16 x);
ulong2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i64_v2i32, _Rulong2_sat)(uint2 x);
ulong3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i64_v3i32, _Rulong3_sat)(uint3 x);
ulong4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i64_v4i32, _Rulong4_sat)(uint4 x);
ulong8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i64_v8i32, _Rulong8_sat)(uint8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v16i64_v16i32, _Rulong16_sat)(uint16 x);
ulong2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v2i64_v2i64, _Rulong2)(ulong2 x);
ulong3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v3i64_v3i64, _Rulong3)(ulong3 x);
ulong4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v4i64_v4i64, _Rulong4)(ulong4 x);
ulong8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(UConvert, _v8i64_v8i64, _Rulong8)(ulong8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i64_v16i64, _Rulong16)(ulong16 x);
ulong2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i64_v2i64, _Rulong2_sat)(ulong2 x);
ulong3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i64_v3i64, _Rulong3_sat)(ulong3 x);
ulong4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i64_v4i64, _Rulong4_sat)(ulong4 x);
ulong8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i64_v8i64, _Rulong8_sat)(ulong8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UConvert, _Sat_v16i64_v16i64, _Rulong16_sat)(ulong16 x);
char2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v2i8_v2f32, _Rchar2)(float2 x);
char3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v3i8_v3f32, _Rchar3)(float3 x);
char4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v4i8_v4f32, _Rchar4)(float4 x);
char8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v8i8_v8f32, _Rchar8)(float8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i8_v16f32, _Rchar16)(float16 x);
char2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v2i8_v2f32, _Rchar2_sat)(float2 x);
char3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v3i8_v3f32, _Rchar3_sat)(float3 x);
char4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v4i8_v4f32, _Rchar4_sat)(float4 x);
char8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v8i8_v8f32, _Rchar8_sat)(float8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v16i8_v16f32, _Rchar16_sat)(float16 x);
char2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v2i8_v2f32, _Rchar2_rte)(float2 x);
char3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v3i8_v3f32, _Rchar3_rte)(float3 x);
char4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v4i8_v4f32, _Rchar4_rte)(float4 x);
char8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v8i8_v8f32, _Rchar8_rte)(float8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v16i8_v16f32, _Rchar16_rte)(float16 x);
char2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i8_v2f32, _Rchar2_rtz)(float2 x);
char3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i8_v3f32, _Rchar3_rtz)(float3 x);
char4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i8_v4f32, _Rchar4_rtz)(float4 x);
char8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i8_v8f32, _Rchar8_rtz)(float8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i8_v16f32, _Rchar16_rtz)(float16 x);
char2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v2i8_v2f32, _Rchar2_rtp)(float2 x);
char3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v3i8_v3f32, _Rchar3_rtp)(float3 x);
char4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v4i8_v4f32, _Rchar4_rtp)(float4 x);
char8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v8i8_v8f32, _Rchar8_rtp)(float8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v16i8_v16f32, _Rchar16_rtp)(float16 x);
char2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v2i8_v2f32, _Rchar2_rtn)(float2 x);
char3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v3i8_v3f32, _Rchar3_rtn)(float3 x);
char4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v4i8_v4f32, _Rchar4_rtn)(float4 x);
char8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v8i8_v8f32, _Rchar8_rtn)(float8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v16i8_v16f32, _Rchar16_rtn)(float16 x);
char2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i8_v2f32, _Rchar2_sat_rte)(float2 x);
char3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i8_v3f32, _Rchar3_sat_rte)(float3 x);
char4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i8_v4f32, _Rchar4_sat_rte)(float4 x);
char8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i8_v8f32, _Rchar8_sat_rte)(float8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i8_v16f32, _Rchar16_sat_rte)(float16 x);
char2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i8_v2f32, _Rchar2_sat_rtz)(float2 x);
char3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i8_v3f32, _Rchar3_sat_rtz)(float3 x);
char4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i8_v4f32, _Rchar4_sat_rtz)(float4 x);
char8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i8_v8f32, _Rchar8_sat_rtz)(float8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i8_v16f32, _Rchar16_sat_rtz)(float16 x);
char2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i8_v2f32, _Rchar2_sat_rtp)(float2 x);
char3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i8_v3f32, _Rchar3_sat_rtp)(float3 x);
char4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i8_v4f32, _Rchar4_sat_rtp)(float4 x);
char8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i8_v8f32, _Rchar8_sat_rtp)(float8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i8_v16f32, _Rchar16_sat_rtp)(float16 x);
char2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i8_v2f32, _Rchar2_sat_rtn)(float2 x);
char3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i8_v3f32, _Rchar3_sat_rtn)(float3 x);
char4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i8_v4f32, _Rchar4_sat_rtn)(float4 x);
char8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i8_v8f32, _Rchar8_sat_rtn)(float8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i8_v16f32, _Rchar16_sat_rtn)(float16 x);
char2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v2i8_v2f16, _Rchar2)(half2 x);
char3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v3i8_v3f16, _Rchar3)(half3 x);
char4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v4i8_v4f16, _Rchar4)(half4 x);
char8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v8i8_v8f16, _Rchar8)(half8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i8_v16f16, _Rchar16)(half16 x);
char2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v2i8_v2f16, _Rchar2_sat)(half2 x);
char3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v3i8_v3f16, _Rchar3_sat)(half3 x);
char4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v4i8_v4f16, _Rchar4_sat)(half4 x);
char8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v8i8_v8f16, _Rchar8_sat)(half8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v16i8_v16f16, _Rchar16_sat)(half16 x);
char2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v2i8_v2f16, _Rchar2_rte)(half2 x);
char3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v3i8_v3f16, _Rchar3_rte)(half3 x);
char4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v4i8_v4f16, _Rchar4_rte)(half4 x);
char8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v8i8_v8f16, _Rchar8_rte)(half8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v16i8_v16f16, _Rchar16_rte)(half16 x);
char2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i8_v2f16, _Rchar2_rtz)(half2 x);
char3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i8_v3f16, _Rchar3_rtz)(half3 x);
char4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i8_v4f16, _Rchar4_rtz)(half4 x);
char8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i8_v8f16, _Rchar8_rtz)(half8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i8_v16f16, _Rchar16_rtz)(half16 x);
char2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v2i8_v2f16, _Rchar2_rtp)(half2 x);
char3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v3i8_v3f16, _Rchar3_rtp)(half3 x);
char4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v4i8_v4f16, _Rchar4_rtp)(half4 x);
char8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v8i8_v8f16, _Rchar8_rtp)(half8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v16i8_v16f16, _Rchar16_rtp)(half16 x);
char2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v2i8_v2f16, _Rchar2_rtn)(half2 x);
char3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v3i8_v3f16, _Rchar3_rtn)(half3 x);
char4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v4i8_v4f16, _Rchar4_rtn)(half4 x);
char8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v8i8_v8f16, _Rchar8_rtn)(half8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v16i8_v16f16, _Rchar16_rtn)(half16 x);
char2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i8_v2f16, _Rchar2_sat_rte)(half2 x);
char3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i8_v3f16, _Rchar3_sat_rte)(half3 x);
char4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i8_v4f16, _Rchar4_sat_rte)(half4 x);
char8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i8_v8f16, _Rchar8_sat_rte)(half8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i8_v16f16, _Rchar16_sat_rte)(half16 x);
char2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i8_v2f16, _Rchar2_sat_rtz)(half2 x);
char3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i8_v3f16, _Rchar3_sat_rtz)(half3 x);
char4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i8_v4f16, _Rchar4_sat_rtz)(half4 x);
char8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i8_v8f16, _Rchar8_sat_rtz)(half8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i8_v16f16, _Rchar16_sat_rtz)(half16 x);
char2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i8_v2f16, _Rchar2_sat_rtp)(half2 x);
char3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i8_v3f16, _Rchar3_sat_rtp)(half3 x);
char4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i8_v4f16, _Rchar4_sat_rtp)(half4 x);
char8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i8_v8f16, _Rchar8_sat_rtp)(half8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i8_v16f16, _Rchar16_sat_rtp)(half16 x);
char2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i8_v2f16, _Rchar2_sat_rtn)(half2 x);
char3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i8_v3f16, _Rchar3_sat_rtn)(half3 x);
char4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i8_v4f16, _Rchar4_sat_rtn)(half4 x);
char8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i8_v8f16, _Rchar8_sat_rtn)(half8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i8_v16f16, _Rchar16_sat_rtn)(half16 x);
#if defined(cl_khr_fp64)
char2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v2i8_v2f64, _Rchar2)(double2 x);
char3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v3i8_v3f64, _Rchar3)(double3 x);
char4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v4i8_v4f64, _Rchar4)(double4 x);
char8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v8i8_v8f64, _Rchar8)(double8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i8_v16f64, _Rchar16)(double16 x);
char2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v2i8_v2f64, _Rchar2_sat)(double2 x);
char3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v3i8_v3f64, _Rchar3_sat)(double3 x);
char4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v4i8_v4f64, _Rchar4_sat)(double4 x);
char8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v8i8_v8f64, _Rchar8_sat)(double8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v16i8_v16f64, _Rchar16_sat)(double16 x);
char2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v2i8_v2f64, _Rchar2_rte)(double2 x);
char3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v3i8_v3f64, _Rchar3_rte)(double3 x);
char4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v4i8_v4f64, _Rchar4_rte)(double4 x);
char8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v8i8_v8f64, _Rchar8_rte)(double8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v16i8_v16f64, _Rchar16_rte)(double16 x);
char2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i8_v2f64, _Rchar2_rtz)(double2 x);
char3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i8_v3f64, _Rchar3_rtz)(double3 x);
char4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i8_v4f64, _Rchar4_rtz)(double4 x);
char8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i8_v8f64, _Rchar8_rtz)(double8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i8_v16f64, _Rchar16_rtz)(double16 x);
char2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v2i8_v2f64, _Rchar2_rtp)(double2 x);
char3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v3i8_v3f64, _Rchar3_rtp)(double3 x);
char4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v4i8_v4f64, _Rchar4_rtp)(double4 x);
char8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v8i8_v8f64, _Rchar8_rtp)(double8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v16i8_v16f64, _Rchar16_rtp)(double16 x);
char2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v2i8_v2f64, _Rchar2_rtn)(double2 x);
char3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v3i8_v3f64, _Rchar3_rtn)(double3 x);
char4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v4i8_v4f64, _Rchar4_rtn)(double4 x);
char8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v8i8_v8f64, _Rchar8_rtn)(double8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v16i8_v16f64, _Rchar16_rtn)(double16 x);
char2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i8_v2f64, _Rchar2_sat_rte)(double2 x);
char3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i8_v3f64, _Rchar3_sat_rte)(double3 x);
char4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i8_v4f64, _Rchar4_sat_rte)(double4 x);
char8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i8_v8f64, _Rchar8_sat_rte)(double8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i8_v16f64, _Rchar16_sat_rte)(double16 x);
char2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i8_v2f64, _Rchar2_sat_rtz)(double2 x);
char3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i8_v3f64, _Rchar3_sat_rtz)(double3 x);
char4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i8_v4f64, _Rchar4_sat_rtz)(double4 x);
char8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i8_v8f64, _Rchar8_sat_rtz)(double8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i8_v16f64, _Rchar16_sat_rtz)(double16 x);
char2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i8_v2f64, _Rchar2_sat_rtp)(double2 x);
char3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i8_v3f64, _Rchar3_sat_rtp)(double3 x);
char4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i8_v4f64, _Rchar4_sat_rtp)(double4 x);
char8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i8_v8f64, _Rchar8_sat_rtp)(double8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i8_v16f64, _Rchar16_sat_rtp)(double16 x);
char2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i8_v2f64, _Rchar2_sat_rtn)(double2 x);
char3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i8_v3f64, _Rchar3_sat_rtn)(double3 x);
char4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i8_v4f64, _Rchar4_sat_rtn)(double4 x);
char8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i8_v8f64, _Rchar8_sat_rtn)(double8 x);
char16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i8_v16f64, _Rchar16_sat_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v2i16_v2f32, _Rshort2)(float2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v3i16_v3f32, _Rshort3)(float3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v4i16_v4f32, _Rshort4)(float4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v8i16_v8f32, _Rshort8)(float8 x);
short16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i16_v16f32, _Rshort16)(float16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v2i16_v2f32, _Rshort2_sat)(float2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v3i16_v3f32, _Rshort3_sat)(float3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v4i16_v4f32, _Rshort4_sat)(float4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v8i16_v8f32, _Rshort8_sat)(float8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v16i16_v16f32, _Rshort16_sat)(float16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v2i16_v2f32, _Rshort2_rte)(float2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v3i16_v3f32, _Rshort3_rte)(float3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v4i16_v4f32, _Rshort4_rte)(float4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v8i16_v8f32, _Rshort8_rte)(float8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v16i16_v16f32, _Rshort16_rte)(float16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i16_v2f32, _Rshort2_rtz)(float2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i16_v3f32, _Rshort3_rtz)(float3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i16_v4f32, _Rshort4_rtz)(float4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i16_v8f32, _Rshort8_rtz)(float8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i16_v16f32, _Rshort16_rtz)(float16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v2i16_v2f32, _Rshort2_rtp)(float2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v3i16_v3f32, _Rshort3_rtp)(float3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v4i16_v4f32, _Rshort4_rtp)(float4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v8i16_v8f32, _Rshort8_rtp)(float8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v16i16_v16f32, _Rshort16_rtp)(float16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v2i16_v2f32, _Rshort2_rtn)(float2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v3i16_v3f32, _Rshort3_rtn)(float3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v4i16_v4f32, _Rshort4_rtn)(float4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v8i16_v8f32, _Rshort8_rtn)(float8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v16i16_v16f32, _Rshort16_rtn)(float16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i16_v2f32, _Rshort2_sat_rte)(float2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i16_v3f32, _Rshort3_sat_rte)(float3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i16_v4f32, _Rshort4_sat_rte)(float4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i16_v8f32, _Rshort8_sat_rte)(float8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i16_v16f32, _Rshort16_sat_rte)(float16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i16_v2f32, _Rshort2_sat_rtz)(float2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i16_v3f32, _Rshort3_sat_rtz)(float3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i16_v4f32, _Rshort4_sat_rtz)(float4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i16_v8f32, _Rshort8_sat_rtz)(float8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i16_v16f32, _Rshort16_sat_rtz)(float16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i16_v2f32, _Rshort2_sat_rtp)(float2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i16_v3f32, _Rshort3_sat_rtp)(float3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i16_v4f32, _Rshort4_sat_rtp)(float4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i16_v8f32, _Rshort8_sat_rtp)(float8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i16_v16f32, _Rshort16_sat_rtp)(float16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i16_v2f32, _Rshort2_sat_rtn)(float2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i16_v3f32, _Rshort3_sat_rtn)(float3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i16_v4f32, _Rshort4_sat_rtn)(float4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i16_v8f32, _Rshort8_sat_rtn)(float8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i16_v16f32, _Rshort16_sat_rtn)(float16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v2i16_v2f16, _Rshort2)(half2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v3i16_v3f16, _Rshort3)(half3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v4i16_v4f16, _Rshort4)(half4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v8i16_v8f16, _Rshort8)(half8 x);
short16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i16_v16f16, _Rshort16)(half16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v2i16_v2f16, _Rshort2_sat)(half2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v3i16_v3f16, _Rshort3_sat)(half3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v4i16_v4f16, _Rshort4_sat)(half4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v8i16_v8f16, _Rshort8_sat)(half8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v16i16_v16f16, _Rshort16_sat)(half16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v2i16_v2f16, _Rshort2_rte)(half2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v3i16_v3f16, _Rshort3_rte)(half3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v4i16_v4f16, _Rshort4_rte)(half4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v8i16_v8f16, _Rshort8_rte)(half8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v16i16_v16f16, _Rshort16_rte)(half16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i16_v2f16, _Rshort2_rtz)(half2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i16_v3f16, _Rshort3_rtz)(half3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i16_v4f16, _Rshort4_rtz)(half4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i16_v8f16, _Rshort8_rtz)(half8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i16_v16f16, _Rshort16_rtz)(half16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v2i16_v2f16, _Rshort2_rtp)(half2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v3i16_v3f16, _Rshort3_rtp)(half3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v4i16_v4f16, _Rshort4_rtp)(half4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v8i16_v8f16, _Rshort8_rtp)(half8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v16i16_v16f16, _Rshort16_rtp)(half16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v2i16_v2f16, _Rshort2_rtn)(half2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v3i16_v3f16, _Rshort3_rtn)(half3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v4i16_v4f16, _Rshort4_rtn)(half4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v8i16_v8f16, _Rshort8_rtn)(half8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v16i16_v16f16, _Rshort16_rtn)(half16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i16_v2f16, _Rshort2_sat_rte)(half2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i16_v3f16, _Rshort3_sat_rte)(half3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i16_v4f16, _Rshort4_sat_rte)(half4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i16_v8f16, _Rshort8_sat_rte)(half8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i16_v16f16, _Rshort16_sat_rte)(half16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i16_v2f16, _Rshort2_sat_rtz)(half2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i16_v3f16, _Rshort3_sat_rtz)(half3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i16_v4f16, _Rshort4_sat_rtz)(half4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i16_v8f16, _Rshort8_sat_rtz)(half8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i16_v16f16, _Rshort16_sat_rtz)(half16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i16_v2f16, _Rshort2_sat_rtp)(half2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i16_v3f16, _Rshort3_sat_rtp)(half3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i16_v4f16, _Rshort4_sat_rtp)(half4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i16_v8f16, _Rshort8_sat_rtp)(half8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i16_v16f16, _Rshort16_sat_rtp)(half16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i16_v2f16, _Rshort2_sat_rtn)(half2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i16_v3f16, _Rshort3_sat_rtn)(half3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i16_v4f16, _Rshort4_sat_rtn)(half4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i16_v8f16, _Rshort8_sat_rtn)(half8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i16_v16f16, _Rshort16_sat_rtn)(half16 x);
#if defined(cl_khr_fp64)
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v2i16_v2f64, _Rshort2)(double2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v3i16_v3f64, _Rshort3)(double3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v4i16_v4f64, _Rshort4)(double4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v8i16_v8f64, _Rshort8)(double8 x);
short16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i16_v16f64, _Rshort16)(double16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v2i16_v2f64, _Rshort2_sat)(double2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v3i16_v3f64, _Rshort3_sat)(double3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v4i16_v4f64, _Rshort4_sat)(double4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v8i16_v8f64, _Rshort8_sat)(double8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v16i16_v16f64, _Rshort16_sat)(double16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v2i16_v2f64, _Rshort2_rte)(double2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v3i16_v3f64, _Rshort3_rte)(double3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v4i16_v4f64, _Rshort4_rte)(double4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v8i16_v8f64, _Rshort8_rte)(double8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v16i16_v16f64, _Rshort16_rte)(double16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i16_v2f64, _Rshort2_rtz)(double2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i16_v3f64, _Rshort3_rtz)(double3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i16_v4f64, _Rshort4_rtz)(double4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i16_v8f64, _Rshort8_rtz)(double8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i16_v16f64, _Rshort16_rtz)(double16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v2i16_v2f64, _Rshort2_rtp)(double2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v3i16_v3f64, _Rshort3_rtp)(double3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v4i16_v4f64, _Rshort4_rtp)(double4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v8i16_v8f64, _Rshort8_rtp)(double8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v16i16_v16f64, _Rshort16_rtp)(double16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v2i16_v2f64, _Rshort2_rtn)(double2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v3i16_v3f64, _Rshort3_rtn)(double3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v4i16_v4f64, _Rshort4_rtn)(double4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v8i16_v8f64, _Rshort8_rtn)(double8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v16i16_v16f64, _Rshort16_rtn)(double16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i16_v2f64, _Rshort2_sat_rte)(double2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i16_v3f64, _Rshort3_sat_rte)(double3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i16_v4f64, _Rshort4_sat_rte)(double4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i16_v8f64, _Rshort8_sat_rte)(double8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i16_v16f64, _Rshort16_sat_rte)(double16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i16_v2f64, _Rshort2_sat_rtz)(double2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i16_v3f64, _Rshort3_sat_rtz)(double3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i16_v4f64, _Rshort4_sat_rtz)(double4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i16_v8f64, _Rshort8_sat_rtz)(double8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i16_v16f64, _Rshort16_sat_rtz)(double16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i16_v2f64, _Rshort2_sat_rtp)(double2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i16_v3f64, _Rshort3_sat_rtp)(double3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i16_v4f64, _Rshort4_sat_rtp)(double4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i16_v8f64, _Rshort8_sat_rtp)(double8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i16_v16f64, _Rshort16_sat_rtp)(double16 x);
short2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i16_v2f64, _Rshort2_sat_rtn)(double2 x);
short3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i16_v3f64, _Rshort3_sat_rtn)(double3 x);
short4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i16_v4f64, _Rshort4_sat_rtn)(double4 x);
short8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i16_v8f64, _Rshort8_sat_rtn)(double8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i16_v16f64, _Rshort16_sat_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
int2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v2i32_v2f32, _Rint2)(float2 x);
int3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v3i32_v3f32, _Rint3)(float3 x);
int4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v4i32_v4f32, _Rint4)(float4 x);
int8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v8i32_v8f32, _Rint8)(float8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i32_v16f32, _Rint16)(float16 x);
int2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v2i32_v2f32, _Rint2_sat)(float2 x);
int3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v3i32_v3f32, _Rint3_sat)(float3 x);
int4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v4i32_v4f32, _Rint4_sat)(float4 x);
int8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v8i32_v8f32, _Rint8_sat)(float8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v16i32_v16f32, _Rint16_sat)(float16 x);
int2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v2i32_v2f32, _Rint2_rte)(float2 x);
int3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v3i32_v3f32, _Rint3_rte)(float3 x);
int4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v4i32_v4f32, _Rint4_rte)(float4 x);
int8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v8i32_v8f32, _Rint8_rte)(float8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v16i32_v16f32, _Rint16_rte)(float16 x);
int2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i32_v2f32, _Rint2_rtz)(float2 x);
int3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i32_v3f32, _Rint3_rtz)(float3 x);
int4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i32_v4f32, _Rint4_rtz)(float4 x);
int8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i32_v8f32, _Rint8_rtz)(float8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i32_v16f32, _Rint16_rtz)(float16 x);
int2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v2i32_v2f32, _Rint2_rtp)(float2 x);
int3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v3i32_v3f32, _Rint3_rtp)(float3 x);
int4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v4i32_v4f32, _Rint4_rtp)(float4 x);
int8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v8i32_v8f32, _Rint8_rtp)(float8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v16i32_v16f32, _Rint16_rtp)(float16 x);
int2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v2i32_v2f32, _Rint2_rtn)(float2 x);
int3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v3i32_v3f32, _Rint3_rtn)(float3 x);
int4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v4i32_v4f32, _Rint4_rtn)(float4 x);
int8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v8i32_v8f32, _Rint8_rtn)(float8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v16i32_v16f32, _Rint16_rtn)(float16 x);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i32_v2f32, _Rint2_sat_rte)(float2 x);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i32_v3f32, _Rint3_sat_rte)(float3 x);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i32_v4f32, _Rint4_sat_rte)(float4 x);
int8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i32_v8f32, _Rint8_sat_rte)(float8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i32_v16f32, _Rint16_sat_rte)(float16 x);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i32_v2f32, _Rint2_sat_rtz)(float2 x);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i32_v3f32, _Rint3_sat_rtz)(float3 x);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i32_v4f32, _Rint4_sat_rtz)(float4 x);
int8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i32_v8f32, _Rint8_sat_rtz)(float8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i32_v16f32, _Rint16_sat_rtz)(float16 x);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i32_v2f32, _Rint2_sat_rtp)(float2 x);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i32_v3f32, _Rint3_sat_rtp)(float3 x);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i32_v4f32, _Rint4_sat_rtp)(float4 x);
int8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i32_v8f32, _Rint8_sat_rtp)(float8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i32_v16f32, _Rint16_sat_rtp)(float16 x);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i32_v2f32, _Rint2_sat_rtn)(float2 x);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i32_v3f32, _Rint3_sat_rtn)(float3 x);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i32_v4f32, _Rint4_sat_rtn)(float4 x);
int8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i32_v8f32, _Rint8_sat_rtn)(float8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i32_v16f32, _Rint16_sat_rtn)(float16 x);
int2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v2i32_v2f16, _Rint2)(half2 x);
int3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v3i32_v3f16, _Rint3)(half3 x);
int4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v4i32_v4f16, _Rint4)(half4 x);
int8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v8i32_v8f16, _Rint8)(half8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i32_v16f16, _Rint16)(half16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v2i32_v2f16, _Rint2_sat)(half2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v3i32_v3f16, _Rint3_sat)(half3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v4i32_v4f16, _Rint4_sat)(half4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v8i32_v8f16, _Rint8_sat)(half8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v16i32_v16f16, _Rint16_sat)(half16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v2i32_v2f16, _Rint2_rte)(half2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v3i32_v3f16, _Rint3_rte)(half3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v4i32_v4f16, _Rint4_rte)(half4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v8i32_v8f16, _Rint8_rte)(half8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v16i32_v16f16, _Rint16_rte)(half16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i32_v2f16, _Rint2_rtz)(half2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i32_v3f16, _Rint3_rtz)(half3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i32_v4f16, _Rint4_rtz)(half4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i32_v8f16, _Rint8_rtz)(half8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i32_v16f16, _Rint16_rtz)(half16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v2i32_v2f16, _Rint2_rtp)(half2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v3i32_v3f16, _Rint3_rtp)(half3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v4i32_v4f16, _Rint4_rtp)(half4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v8i32_v8f16, _Rint8_rtp)(half8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v16i32_v16f16, _Rint16_rtp)(half16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v2i32_v2f16, _Rint2_rtn)(half2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v3i32_v3f16, _Rint3_rtn)(half3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v4i32_v4f16, _Rint4_rtn)(half4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v8i32_v8f16, _Rint8_rtn)(half8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v16i32_v16f16, _Rint16_rtn)(half16 x);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i32_v2f16, _Rint2_sat_rte)(half2 x);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i32_v3f16, _Rint3_sat_rte)(half3 x);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i32_v4f16, _Rint4_sat_rte)(half4 x);
int8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i32_v8f16, _Rint8_sat_rte)(half8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i32_v16f16, _Rint16_sat_rte)(half16 x);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i32_v2f16, _Rint2_sat_rtz)(half2 x);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i32_v3f16, _Rint3_sat_rtz)(half3 x);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i32_v4f16, _Rint4_sat_rtz)(half4 x);
int8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i32_v8f16, _Rint8_sat_rtz)(half8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i32_v16f16, _Rint16_sat_rtz)(half16 x);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i32_v2f16, _Rint2_sat_rtp)(half2 x);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i32_v3f16, _Rint3_sat_rtp)(half3 x);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i32_v4f16, _Rint4_sat_rtp)(half4 x);
int8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i32_v8f16, _Rint8_sat_rtp)(half8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i32_v16f16, _Rint16_sat_rtp)(half16 x);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i32_v2f16, _Rint2_sat_rtn)(half2 x);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i32_v3f16, _Rint3_sat_rtn)(half3 x);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i32_v4f16, _Rint4_sat_rtn)(half4 x);
int8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i32_v8f16, _Rint8_sat_rtn)(half8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i32_v16f16, _Rint16_sat_rtn)(half16 x);
#if defined(cl_khr_fp64)
int2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v2i32_v2f64, _Rint2)(double2 x);
int3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v3i32_v3f64, _Rint3)(double3 x);
int4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v4i32_v4f64, _Rint4)(double4 x);
int8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v8i32_v8f64, _Rint8)(double8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i32_v16f64, _Rint16)(double16 x);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v2i32_v2f64, _Rint2_sat)(double2 x);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v3i32_v3f64, _Rint3_sat)(double3 x);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v4i32_v4f64, _Rint4_sat)(double4 x);
int8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v8i32_v8f64, _Rint8_sat)(double8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v16i32_v16f64, _Rint16_sat)(double16 x);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v2i32_v2f64, _Rint2_rte)(double2 x);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v3i32_v3f64, _Rint3_rte)(double3 x);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v4i32_v4f64, _Rint4_rte)(double4 x);
int8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v8i32_v8f64, _Rint8_rte)(double8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v16i32_v16f64, _Rint16_rte)(double16 x);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i32_v2f64, _Rint2_rtz)(double2 x);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i32_v3f64, _Rint3_rtz)(double3 x);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i32_v4f64, _Rint4_rtz)(double4 x);
int8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i32_v8f64, _Rint8_rtz)(double8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i32_v16f64, _Rint16_rtz)(double16 x);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v2i32_v2f64, _Rint2_rtp)(double2 x);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v3i32_v3f64, _Rint3_rtp)(double3 x);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v4i32_v4f64, _Rint4_rtp)(double4 x);
int8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v8i32_v8f64, _Rint8_rtp)(double8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v16i32_v16f64, _Rint16_rtp)(double16 x);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v2i32_v2f64, _Rint2_rtn)(double2 x);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v3i32_v3f64, _Rint3_rtn)(double3 x);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v4i32_v4f64, _Rint4_rtn)(double4 x);
int8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v8i32_v8f64, _Rint8_rtn)(double8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v16i32_v16f64, _Rint16_rtn)(double16 x);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i32_v2f64, _Rint2_sat_rte)(double2 x);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i32_v3f64, _Rint3_sat_rte)(double3 x);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i32_v4f64, _Rint4_sat_rte)(double4 x);
int8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i32_v8f64, _Rint8_sat_rte)(double8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i32_v16f64, _Rint16_sat_rte)(double16 x);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i32_v2f64, _Rint2_sat_rtz)(double2 x);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i32_v3f64, _Rint3_sat_rtz)(double3 x);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i32_v4f64, _Rint4_sat_rtz)(double4 x);
int8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i32_v8f64, _Rint8_sat_rtz)(double8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i32_v16f64, _Rint16_sat_rtz)(double16 x);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i32_v2f64, _Rint2_sat_rtp)(double2 x);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i32_v3f64, _Rint3_sat_rtp)(double3 x);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i32_v4f64, _Rint4_sat_rtp)(double4 x);
int8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i32_v8f64, _Rint8_sat_rtp)(double8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i32_v16f64, _Rint16_sat_rtp)(double16 x);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i32_v2f64, _Rint2_sat_rtn)(double2 x);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i32_v3f64, _Rint3_sat_rtn)(double3 x);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i32_v4f64, _Rint4_sat_rtn)(double4 x);
int8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i32_v8f64, _Rint8_sat_rtn)(double8 x);
int16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i32_v16f64, _Rint16_sat_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
long2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v2i64_v2f32, _Rlong2)(float2 x);
long3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v3i64_v3f32, _Rlong3)(float3 x);
long4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v4i64_v4f32, _Rlong4)(float4 x);
long8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v8i64_v8f32, _Rlong8)(float8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i64_v16f32, _Rlong16)(float16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v2i64_v2f32, _Rlong2_sat)(float2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v3i64_v3f32, _Rlong3_sat)(float3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v4i64_v4f32, _Rlong4_sat)(float4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v8i64_v8f32, _Rlong8_sat)(float8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v16i64_v16f32, _Rlong16_sat)(float16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v2i64_v2f32, _Rlong2_rte)(float2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v3i64_v3f32, _Rlong3_rte)(float3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v4i64_v4f32, _Rlong4_rte)(float4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v8i64_v8f32, _Rlong8_rte)(float8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v16i64_v16f32, _Rlong16_rte)(float16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i64_v2f32, _Rlong2_rtz)(float2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i64_v3f32, _Rlong3_rtz)(float3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i64_v4f32, _Rlong4_rtz)(float4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i64_v8f32, _Rlong8_rtz)(float8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i64_v16f32, _Rlong16_rtz)(float16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v2i64_v2f32, _Rlong2_rtp)(float2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v3i64_v3f32, _Rlong3_rtp)(float3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v4i64_v4f32, _Rlong4_rtp)(float4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v8i64_v8f32, _Rlong8_rtp)(float8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v16i64_v16f32, _Rlong16_rtp)(float16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v2i64_v2f32, _Rlong2_rtn)(float2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v3i64_v3f32, _Rlong3_rtn)(float3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v4i64_v4f32, _Rlong4_rtn)(float4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v8i64_v8f32, _Rlong8_rtn)(float8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v16i64_v16f32, _Rlong16_rtn)(float16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i64_v2f32, _Rlong2_sat_rte)(float2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i64_v3f32, _Rlong3_sat_rte)(float3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i64_v4f32, _Rlong4_sat_rte)(float4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i64_v8f32, _Rlong8_sat_rte)(float8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i64_v16f32, _Rlong16_sat_rte)(float16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i64_v2f32, _Rlong2_sat_rtz)(float2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i64_v3f32, _Rlong3_sat_rtz)(float3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i64_v4f32, _Rlong4_sat_rtz)(float4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i64_v8f32, _Rlong8_sat_rtz)(float8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i64_v16f32, _Rlong16_sat_rtz)(float16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i64_v2f32, _Rlong2_sat_rtp)(float2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i64_v3f32, _Rlong3_sat_rtp)(float3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i64_v4f32, _Rlong4_sat_rtp)(float4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i64_v8f32, _Rlong8_sat_rtp)(float8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i64_v16f32, _Rlong16_sat_rtp)(float16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i64_v2f32, _Rlong2_sat_rtn)(float2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i64_v3f32, _Rlong3_sat_rtn)(float3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i64_v4f32, _Rlong4_sat_rtn)(float4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i64_v8f32, _Rlong8_sat_rtn)(float8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i64_v16f32, _Rlong16_sat_rtn)(float16 x);
long2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v2i64_v2f16, _Rlong2)(half2 x);
long3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v3i64_v3f16, _Rlong3)(half3 x);
long4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v4i64_v4f16, _Rlong4)(half4 x);
long8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToS, _v8i64_v8f16, _Rlong8)(half8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i64_v16f16, _Rlong16)(half16 x);
long2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v2i64_v2f16, _Rlong2_sat)(half2 x);
long3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v3i64_v3f16, _Rlong3_sat)(half3 x);
long4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v4i64_v4f16, _Rlong4_sat)(half4 x);
long8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v8i64_v8f16, _Rlong8_sat)(half8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v16i64_v16f16, _Rlong16_sat)(half16 x);
long2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v2i64_v2f16, _Rlong2_rte)(half2 x);
long3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v3i64_v3f16, _Rlong3_rte)(half3 x);
long4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v4i64_v4f16, _Rlong4_rte)(half4 x);
long8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v8i64_v8f16, _Rlong8_rte)(half8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v16i64_v16f16, _Rlong16_rte)(half16 x);
long2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i64_v2f16, _Rlong2_rtz)(half2 x);
long3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i64_v3f16, _Rlong3_rtz)(half3 x);
long4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i64_v4f16, _Rlong4_rtz)(half4 x);
long8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i64_v8f16, _Rlong8_rtz)(half8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i64_v16f16, _Rlong16_rtz)(half16 x);
long2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v2i64_v2f16, _Rlong2_rtp)(half2 x);
long3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v3i64_v3f16, _Rlong3_rtp)(half3 x);
long4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v4i64_v4f16, _Rlong4_rtp)(half4 x);
long8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v8i64_v8f16, _Rlong8_rtp)(half8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v16i64_v16f16, _Rlong16_rtp)(half16 x);
long2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v2i64_v2f16, _Rlong2_rtn)(half2 x);
long3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v3i64_v3f16, _Rlong3_rtn)(half3 x);
long4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v4i64_v4f16, _Rlong4_rtn)(half4 x);
long8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v8i64_v8f16, _Rlong8_rtn)(half8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v16i64_v16f16, _Rlong16_rtn)(half16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i64_v2f16, _Rlong2_sat_rte)(half2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i64_v3f16, _Rlong3_sat_rte)(half3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i64_v4f16, _Rlong4_sat_rte)(half4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i64_v8f16, _Rlong8_sat_rte)(half8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i64_v16f16, _Rlong16_sat_rte)(half16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i64_v2f16, _Rlong2_sat_rtz)(half2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i64_v3f16, _Rlong3_sat_rtz)(half3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i64_v4f16, _Rlong4_sat_rtz)(half4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i64_v8f16, _Rlong8_sat_rtz)(half8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i64_v16f16, _Rlong16_sat_rtz)(half16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i64_v2f16, _Rlong2_sat_rtp)(half2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i64_v3f16, _Rlong3_sat_rtp)(half3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i64_v4f16, _Rlong4_sat_rtp)(half4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i64_v8f16, _Rlong8_sat_rtp)(half8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i64_v16f16, _Rlong16_sat_rtp)(half16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i64_v2f16, _Rlong2_sat_rtn)(half2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i64_v3f16, _Rlong3_sat_rtn)(half3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i64_v4f16, _Rlong4_sat_rtn)(half4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i64_v8f16, _Rlong8_sat_rtn)(half8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i64_v16f16, _Rlong16_sat_rtn)(half16 x);
#if defined(cl_khr_fp64)
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v2i64_v2f64, _Rlong2)(double2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v3i64_v3f64, _Rlong3)(double3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v4i64_v4f64, _Rlong4)(double4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v8i64_v8f64, _Rlong8)(double8 x);
long16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i64_v16f64, _Rlong16)(double16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v2i64_v2f64, _Rlong2_sat)(double2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v3i64_v3f64, _Rlong3_sat)(double3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v4i64_v4f64, _Rlong4_sat)(double4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v8i64_v8f64, _Rlong8_sat)(double8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_v16i64_v16f64, _Rlong16_sat)(double16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v2i64_v2f64, _Rlong2_rte)(double2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v3i64_v3f64, _Rlong3_rte)(double3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v4i64_v4f64, _Rlong4_rte)(double4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v8i64_v8f64, _Rlong8_rte)(double8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTE_v16i64_v16f64, _Rlong16_rte)(double16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i64_v2f64, _Rlong2_rtz)(double2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i64_v3f64, _Rlong3_rtz)(double3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i64_v4f64, _Rlong4_rtz)(double4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i64_v8f64, _Rlong8_rtz)(double8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i64_v16f64, _Rlong16_rtz)(double16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v2i64_v2f64, _Rlong2_rtp)(double2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v3i64_v3f64, _Rlong3_rtp)(double3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v4i64_v4f64, _Rlong4_rtp)(double4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v8i64_v8f64, _Rlong8_rtp)(double8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTP_v16i64_v16f64, _Rlong16_rtp)(double16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v2i64_v2f64, _Rlong2_rtn)(double2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v3i64_v3f64, _Rlong3_rtn)(double3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v4i64_v4f64, _Rlong4_rtn)(double4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v8i64_v8f64, _Rlong8_rtn)(double8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _RTN_v16i64_v16f64, _Rlong16_rtn)(double16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i64_v2f64, _Rlong2_sat_rte)(double2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i64_v3f64, _Rlong3_sat_rte)(double3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i64_v4f64, _Rlong4_sat_rte)(double4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i64_v8f64, _Rlong8_sat_rte)(double8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i64_v16f64, _Rlong16_sat_rte)(double16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i64_v2f64, _Rlong2_sat_rtz)(double2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i64_v3f64, _Rlong3_sat_rtz)(double3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i64_v4f64, _Rlong4_sat_rtz)(double4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i64_v8f64, _Rlong8_sat_rtz)(double8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i64_v16f64, _Rlong16_sat_rtz)(double16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i64_v2f64, _Rlong2_sat_rtp)(double2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i64_v3f64, _Rlong3_sat_rtp)(double3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i64_v4f64, _Rlong4_sat_rtp)(double4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i64_v8f64, _Rlong8_sat_rtp)(double8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i64_v16f64, _Rlong16_sat_rtp)(double16 x);
long2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i64_v2f64, _Rlong2_sat_rtn)(double2 x);
long3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i64_v3f64, _Rlong3_sat_rtn)(double3 x);
long4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i64_v4f64, _Rlong4_sat_rtn)(double4 x);
long8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i64_v8f64, _Rlong8_sat_rtn)(double8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i64_v16f64, _Rlong16_sat_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i8_v2f32, _Ruchar2)(float2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i8_v3f32, _Ruchar3)(float3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i8_v4f32, _Ruchar4)(float4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i8_v8f32, _Ruchar8)(float8 x);
uchar16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i8_v16f32, _Ruchar16)(float16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v2i8_v2f32, _Ruchar2_sat)(float2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v3i8_v3f32, _Ruchar3_sat)(float3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v4i8_v4f32, _Ruchar4_sat)(float4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v8i8_v8f32, _Ruchar8_sat)(float8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v16i8_v16f32, _Ruchar16_sat)(float16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v2i8_v2f32, _Ruchar2_rte)(float2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v3i8_v3f32, _Ruchar3_rte)(float3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v4i8_v4f32, _Ruchar4_rte)(float4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v8i8_v8f32, _Ruchar8_rte)(float8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v16i8_v16f32, _Ruchar16_rte)(float16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i8_v2f32, _Ruchar2_rtz)(float2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i8_v3f32, _Ruchar3_rtz)(float3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i8_v4f32, _Ruchar4_rtz)(float4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i8_v8f32, _Ruchar8_rtz)(float8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i8_v16f32, _Ruchar16_rtz)(float16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v2i8_v2f32, _Ruchar2_rtp)(float2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v3i8_v3f32, _Ruchar3_rtp)(float3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v4i8_v4f32, _Ruchar4_rtp)(float4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v8i8_v8f32, _Ruchar8_rtp)(float8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v16i8_v16f32, _Ruchar16_rtp)(float16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v2i8_v2f32, _Ruchar2_rtn)(float2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v3i8_v3f32, _Ruchar3_rtn)(float3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v4i8_v4f32, _Ruchar4_rtn)(float4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v8i8_v8f32, _Ruchar8_rtn)(float8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v16i8_v16f32, _Ruchar16_rtn)(float16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i8_v2f32, _Ruchar2_sat_rte)(float2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i8_v3f32, _Ruchar3_sat_rte)(float3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i8_v4f32, _Ruchar4_sat_rte)(float4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i8_v8f32, _Ruchar8_sat_rte)(float8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i8_v16f32, _Ruchar16_sat_rte)(float16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i8_v2f32, _Ruchar2_sat_rtz)(float2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i8_v3f32, _Ruchar3_sat_rtz)(float3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i8_v4f32, _Ruchar4_sat_rtz)(float4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i8_v8f32, _Ruchar8_sat_rtz)(float8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i8_v16f32, _Ruchar16_sat_rtz)(float16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i8_v2f32, _Ruchar2_sat_rtp)(float2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i8_v3f32, _Ruchar3_sat_rtp)(float3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i8_v4f32, _Ruchar4_sat_rtp)(float4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i8_v8f32, _Ruchar8_sat_rtp)(float8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i8_v16f32, _Ruchar16_sat_rtp)(float16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i8_v2f32, _Ruchar2_sat_rtn)(float2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i8_v3f32, _Ruchar3_sat_rtn)(float3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i8_v4f32, _Ruchar4_sat_rtn)(float4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i8_v8f32, _Ruchar8_sat_rtn)(float8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i8_v16f32, _Ruchar16_sat_rtn)(float16 x);
uchar2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToU, _v2i8_v2f16, _Ruchar2)(half2 x);
uchar3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToU, _v3i8_v3f16, _Ruchar3)(half3 x);
uchar4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToU, _v4i8_v4f16, _Ruchar4)(half4 x);
uchar8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToU, _v8i8_v8f16, _Ruchar8)(half8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i8_v16f16, _Ruchar16)(half16 x);
uchar2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v2i8_v2f16, _Ruchar2_sat)(half2 x);
uchar3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v3i8_v3f16, _Ruchar3_sat)(half3 x);
uchar4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v4i8_v4f16, _Ruchar4_sat)(half4 x);
uchar8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v8i8_v8f16, _Ruchar8_sat)(half8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v16i8_v16f16, _Ruchar16_sat)(half16 x);
uchar2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v2i8_v2f16, _Ruchar2_rte)(half2 x);
uchar3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v3i8_v3f16, _Ruchar3_rte)(half3 x);
uchar4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v4i8_v4f16, _Ruchar4_rte)(half4 x);
uchar8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v8i8_v8f16, _Ruchar8_rte)(half8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v16i8_v16f16, _Ruchar16_rte)(half16 x);
uchar2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i8_v2f16, _Ruchar2_rtz)(half2 x);
uchar3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i8_v3f16, _Ruchar3_rtz)(half3 x);
uchar4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i8_v4f16, _Ruchar4_rtz)(half4 x);
uchar8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i8_v8f16, _Ruchar8_rtz)(half8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i8_v16f16, _Ruchar16_rtz)(half16 x);
uchar2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v2i8_v2f16, _Ruchar2_rtp)(half2 x);
uchar3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v3i8_v3f16, _Ruchar3_rtp)(half3 x);
uchar4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v4i8_v4f16, _Ruchar4_rtp)(half4 x);
uchar8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v8i8_v8f16, _Ruchar8_rtp)(half8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v16i8_v16f16, _Ruchar16_rtp)(half16 x);
uchar2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v2i8_v2f16, _Ruchar2_rtn)(half2 x);
uchar3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v3i8_v3f16, _Ruchar3_rtn)(half3 x);
uchar4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v4i8_v4f16, _Ruchar4_rtn)(half4 x);
uchar8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v8i8_v8f16, _Ruchar8_rtn)(half8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v16i8_v16f16, _Ruchar16_rtn)(half16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i8_v2f16, _Ruchar2_sat_rte)(half2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i8_v3f16, _Ruchar3_sat_rte)(half3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i8_v4f16, _Ruchar4_sat_rte)(half4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i8_v8f16, _Ruchar8_sat_rte)(half8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i8_v16f16, _Ruchar16_sat_rte)(half16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i8_v2f16, _Ruchar2_sat_rtz)(half2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i8_v3f16, _Ruchar3_sat_rtz)(half3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i8_v4f16, _Ruchar4_sat_rtz)(half4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i8_v8f16, _Ruchar8_sat_rtz)(half8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i8_v16f16, _Ruchar16_sat_rtz)(half16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i8_v2f16, _Ruchar2_sat_rtp)(half2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i8_v3f16, _Ruchar3_sat_rtp)(half3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i8_v4f16, _Ruchar4_sat_rtp)(half4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i8_v8f16, _Ruchar8_sat_rtp)(half8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i8_v16f16, _Ruchar16_sat_rtp)(half16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i8_v2f16, _Ruchar2_sat_rtn)(half2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i8_v3f16, _Ruchar3_sat_rtn)(half3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i8_v4f16, _Ruchar4_sat_rtn)(half4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i8_v8f16, _Ruchar8_sat_rtn)(half8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i8_v16f16, _Ruchar16_sat_rtn)(half16 x);
#if defined(cl_khr_fp64)
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i8_v2f64, _Ruchar2)(double2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i8_v3f64, _Ruchar3)(double3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i8_v4f64, _Ruchar4)(double4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i8_v8f64, _Ruchar8)(double8 x);
uchar16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i8_v16f64, _Ruchar16)(double16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v2i8_v2f64, _Ruchar2_sat)(double2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v3i8_v3f64, _Ruchar3_sat)(double3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v4i8_v4f64, _Ruchar4_sat)(double4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v8i8_v8f64, _Ruchar8_sat)(double8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v16i8_v16f64, _Ruchar16_sat)(double16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v2i8_v2f64, _Ruchar2_rte)(double2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v3i8_v3f64, _Ruchar3_rte)(double3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v4i8_v4f64, _Ruchar4_rte)(double4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v8i8_v8f64, _Ruchar8_rte)(double8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v16i8_v16f64, _Ruchar16_rte)(double16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i8_v2f64, _Ruchar2_rtz)(double2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i8_v3f64, _Ruchar3_rtz)(double3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i8_v4f64, _Ruchar4_rtz)(double4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i8_v8f64, _Ruchar8_rtz)(double8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i8_v16f64, _Ruchar16_rtz)(double16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v2i8_v2f64, _Ruchar2_rtp)(double2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v3i8_v3f64, _Ruchar3_rtp)(double3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v4i8_v4f64, _Ruchar4_rtp)(double4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v8i8_v8f64, _Ruchar8_rtp)(double8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v16i8_v16f64, _Ruchar16_rtp)(double16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v2i8_v2f64, _Ruchar2_rtn)(double2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v3i8_v3f64, _Ruchar3_rtn)(double3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v4i8_v4f64, _Ruchar4_rtn)(double4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v8i8_v8f64, _Ruchar8_rtn)(double8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v16i8_v16f64, _Ruchar16_rtn)(double16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i8_v2f64, _Ruchar2_sat_rte)(double2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i8_v3f64, _Ruchar3_sat_rte)(double3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i8_v4f64, _Ruchar4_sat_rte)(double4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i8_v8f64, _Ruchar8_sat_rte)(double8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i8_v16f64, _Ruchar16_sat_rte)(double16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i8_v2f64, _Ruchar2_sat_rtz)(double2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i8_v3f64, _Ruchar3_sat_rtz)(double3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i8_v4f64, _Ruchar4_sat_rtz)(double4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i8_v8f64, _Ruchar8_sat_rtz)(double8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i8_v16f64, _Ruchar16_sat_rtz)(double16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i8_v2f64, _Ruchar2_sat_rtp)(double2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i8_v3f64, _Ruchar3_sat_rtp)(double3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i8_v4f64, _Ruchar4_sat_rtp)(double4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i8_v8f64, _Ruchar8_sat_rtp)(double8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i8_v16f64, _Ruchar16_sat_rtp)(double16 x);
uchar2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i8_v2f64, _Ruchar2_sat_rtn)(double2 x);
uchar3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i8_v3f64, _Ruchar3_sat_rtn)(double3 x);
uchar4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i8_v4f64, _Ruchar4_sat_rtn)(double4 x);
uchar8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i8_v8f64, _Ruchar8_sat_rtn)(double8 x);
uchar16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i8_v16f64, _Ruchar16_sat_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i16_v2f32, _Rushort2)(float2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i16_v3f32, _Rushort3)(float3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i16_v4f32, _Rushort4)(float4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i16_v8f32, _Rushort8)(float8 x);
ushort16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i16_v16f32, _Rushort16)(float16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v2i16_v2f32, _Rushort2_sat)(float2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v3i16_v3f32, _Rushort3_sat)(float3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v4i16_v4f32, _Rushort4_sat)(float4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v8i16_v8f32, _Rushort8_sat)(float8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v16i16_v16f32, _Rushort16_sat)(float16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v2i16_v2f32, _Rushort2_rte)(float2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v3i16_v3f32, _Rushort3_rte)(float3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v4i16_v4f32, _Rushort4_rte)(float4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v8i16_v8f32, _Rushort8_rte)(float8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v16i16_v16f32, _Rushort16_rte)(float16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i16_v2f32, _Rushort2_rtz)(float2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i16_v3f32, _Rushort3_rtz)(float3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i16_v4f32, _Rushort4_rtz)(float4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i16_v8f32, _Rushort8_rtz)(float8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i16_v16f32, _Rushort16_rtz)(float16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v2i16_v2f32, _Rushort2_rtp)(float2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v3i16_v3f32, _Rushort3_rtp)(float3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v4i16_v4f32, _Rushort4_rtp)(float4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v8i16_v8f32, _Rushort8_rtp)(float8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v16i16_v16f32, _Rushort16_rtp)(float16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v2i16_v2f32, _Rushort2_rtn)(float2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v3i16_v3f32, _Rushort3_rtn)(float3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v4i16_v4f32, _Rushort4_rtn)(float4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v8i16_v8f32, _Rushort8_rtn)(float8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v16i16_v16f32, _Rushort16_rtn)(float16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i16_v2f32, _Rushort2_sat_rte)(float2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i16_v3f32, _Rushort3_sat_rte)(float3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i16_v4f32, _Rushort4_sat_rte)(float4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i16_v8f32, _Rushort8_sat_rte)(float8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i16_v16f32, _Rushort16_sat_rte)(float16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i16_v2f32, _Rushort2_sat_rtz)(float2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i16_v3f32, _Rushort3_sat_rtz)(float3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i16_v4f32, _Rushort4_sat_rtz)(float4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i16_v8f32, _Rushort8_sat_rtz)(float8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i16_v16f32, _Rushort16_sat_rtz)(float16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i16_v2f32, _Rushort2_sat_rtp)(float2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i16_v3f32, _Rushort3_sat_rtp)(float3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i16_v4f32, _Rushort4_sat_rtp)(float4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i16_v8f32, _Rushort8_sat_rtp)(float8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i16_v16f32, _Rushort16_sat_rtp)(float16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i16_v2f32, _Rushort2_sat_rtn)(float2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i16_v3f32, _Rushort3_sat_rtn)(float3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i16_v4f32, _Rushort4_sat_rtn)(float4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i16_v8f32, _Rushort8_sat_rtn)(float8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i16_v16f32, _Rushort16_sat_rtn)(float16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i16_v2f16, _Rushort2)(half2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i16_v3f16, _Rushort3)(half3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i16_v4f16, _Rushort4)(half4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i16_v8f16, _Rushort8)(half8 x);
ushort16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i16_v16f16, _Rushort16)(half16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v2i16_v2f16, _Rushort2_sat)(half2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v3i16_v3f16, _Rushort3_sat)(half3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v4i16_v4f16, _Rushort4_sat)(half4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v8i16_v8f16, _Rushort8_sat)(half8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v16i16_v16f16, _Rushort16_sat)(half16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v2i16_v2f16, _Rushort2_rte)(half2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v3i16_v3f16, _Rushort3_rte)(half3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v4i16_v4f16, _Rushort4_rte)(half4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v8i16_v8f16, _Rushort8_rte)(half8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v16i16_v16f16, _Rushort16_rte)(half16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i16_v2f16, _Rushort2_rtz)(half2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i16_v3f16, _Rushort3_rtz)(half3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i16_v4f16, _Rushort4_rtz)(half4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i16_v8f16, _Rushort8_rtz)(half8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i16_v16f16, _Rushort16_rtz)(half16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v2i16_v2f16, _Rushort2_rtp)(half2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v3i16_v3f16, _Rushort3_rtp)(half3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v4i16_v4f16, _Rushort4_rtp)(half4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v8i16_v8f16, _Rushort8_rtp)(half8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v16i16_v16f16, _Rushort16_rtp)(half16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v2i16_v2f16, _Rushort2_rtn)(half2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v3i16_v3f16, _Rushort3_rtn)(half3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v4i16_v4f16, _Rushort4_rtn)(half4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v8i16_v8f16, _Rushort8_rtn)(half8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v16i16_v16f16, _Rushort16_rtn)(half16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i16_v2f16, _Rushort2_sat_rte)(half2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i16_v3f16, _Rushort3_sat_rte)(half3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i16_v4f16, _Rushort4_sat_rte)(half4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i16_v8f16, _Rushort8_sat_rte)(half8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i16_v16f16, _Rushort16_sat_rte)(half16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i16_v2f16, _Rushort2_sat_rtz)(half2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i16_v3f16, _Rushort3_sat_rtz)(half3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i16_v4f16, _Rushort4_sat_rtz)(half4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i16_v8f16, _Rushort8_sat_rtz)(half8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i16_v16f16, _Rushort16_sat_rtz)(half16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i16_v2f16, _Rushort2_sat_rtp)(half2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i16_v3f16, _Rushort3_sat_rtp)(half3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i16_v4f16, _Rushort4_sat_rtp)(half4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i16_v8f16, _Rushort8_sat_rtp)(half8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i16_v16f16, _Rushort16_sat_rtp)(half16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i16_v2f16, _Rushort2_sat_rtn)(half2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i16_v3f16, _Rushort3_sat_rtn)(half3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i16_v4f16, _Rushort4_sat_rtn)(half4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i16_v8f16, _Rushort8_sat_rtn)(half8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i16_v16f16, _Rushort16_sat_rtn)(half16 x);
#if defined(cl_khr_fp64)
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i16_v2f64, _Rushort2)(double2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i16_v3f64, _Rushort3)(double3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i16_v4f64, _Rushort4)(double4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i16_v8f64, _Rushort8)(double8 x);
ushort16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i16_v16f64, _Rushort16)(double16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v2i16_v2f64, _Rushort2_sat)(double2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v3i16_v3f64, _Rushort3_sat)(double3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v4i16_v4f64, _Rushort4_sat)(double4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v8i16_v8f64, _Rushort8_sat)(double8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v16i16_v16f64, _Rushort16_sat)(double16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v2i16_v2f64, _Rushort2_rte)(double2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v3i16_v3f64, _Rushort3_rte)(double3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v4i16_v4f64, _Rushort4_rte)(double4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v8i16_v8f64, _Rushort8_rte)(double8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v16i16_v16f64, _Rushort16_rte)(double16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i16_v2f64, _Rushort2_rtz)(double2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i16_v3f64, _Rushort3_rtz)(double3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i16_v4f64, _Rushort4_rtz)(double4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i16_v8f64, _Rushort8_rtz)(double8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i16_v16f64, _Rushort16_rtz)(double16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v2i16_v2f64, _Rushort2_rtp)(double2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v3i16_v3f64, _Rushort3_rtp)(double3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v4i16_v4f64, _Rushort4_rtp)(double4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v8i16_v8f64, _Rushort8_rtp)(double8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v16i16_v16f64, _Rushort16_rtp)(double16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v2i16_v2f64, _Rushort2_rtn)(double2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v3i16_v3f64, _Rushort3_rtn)(double3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v4i16_v4f64, _Rushort4_rtn)(double4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v8i16_v8f64, _Rushort8_rtn)(double8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v16i16_v16f64, _Rushort16_rtn)(double16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i16_v2f64, _Rushort2_sat_rte)(double2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i16_v3f64, _Rushort3_sat_rte)(double3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i16_v4f64, _Rushort4_sat_rte)(double4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i16_v8f64, _Rushort8_sat_rte)(double8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i16_v16f64, _Rushort16_sat_rte)(double16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i16_v2f64, _Rushort2_sat_rtz)(double2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i16_v3f64, _Rushort3_sat_rtz)(double3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i16_v4f64, _Rushort4_sat_rtz)(double4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i16_v8f64, _Rushort8_sat_rtz)(double8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i16_v16f64, _Rushort16_sat_rtz)(double16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i16_v2f64, _Rushort2_sat_rtp)(double2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i16_v3f64, _Rushort3_sat_rtp)(double3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i16_v4f64, _Rushort4_sat_rtp)(double4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i16_v8f64, _Rushort8_sat_rtp)(double8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i16_v16f64, _Rushort16_sat_rtp)(double16 x);
ushort2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i16_v2f64, _Rushort2_sat_rtn)(double2 x);
ushort3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i16_v3f64, _Rushort3_sat_rtn)(double3 x);
ushort4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i16_v4f64, _Rushort4_sat_rtn)(double4 x);
ushort8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i16_v8f64, _Rushort8_sat_rtn)(double8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i16_v16f64, _Rushort16_sat_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
uint2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToU, _v2i32_v2f32, _Ruint2)(float2 x);
uint3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToU, _v3i32_v3f32, _Ruint3)(float3 x);
uint4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToU, _v4i32_v4f32, _Ruint4)(float4 x);
uint8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToU, _v8i32_v8f32, _Ruint8)(float8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i32_v16f32, _Ruint16)(float16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v2i32_v2f32, _Ruint2_sat)(float2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v3i32_v3f32, _Ruint3_sat)(float3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v4i32_v4f32, _Ruint4_sat)(float4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v8i32_v8f32, _Ruint8_sat)(float8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v16i32_v16f32, _Ruint16_sat)(float16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v2i32_v2f32, _Ruint2_rte)(float2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v3i32_v3f32, _Ruint3_rte)(float3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v4i32_v4f32, _Ruint4_rte)(float4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v8i32_v8f32, _Ruint8_rte)(float8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v16i32_v16f32, _Ruint16_rte)(float16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i32_v2f32, _Ruint2_rtz)(float2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i32_v3f32, _Ruint3_rtz)(float3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i32_v4f32, _Ruint4_rtz)(float4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i32_v8f32, _Ruint8_rtz)(float8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i32_v16f32, _Ruint16_rtz)(float16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v2i32_v2f32, _Ruint2_rtp)(float2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v3i32_v3f32, _Ruint3_rtp)(float3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v4i32_v4f32, _Ruint4_rtp)(float4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v8i32_v8f32, _Ruint8_rtp)(float8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v16i32_v16f32, _Ruint16_rtp)(float16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v2i32_v2f32, _Ruint2_rtn)(float2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v3i32_v3f32, _Ruint3_rtn)(float3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v4i32_v4f32, _Ruint4_rtn)(float4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v8i32_v8f32, _Ruint8_rtn)(float8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v16i32_v16f32, _Ruint16_rtn)(float16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i32_v2f32, _Ruint2_sat_rte)(float2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i32_v3f32, _Ruint3_sat_rte)(float3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i32_v4f32, _Ruint4_sat_rte)(float4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i32_v8f32, _Ruint8_sat_rte)(float8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i32_v16f32, _Ruint16_sat_rte)(float16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i32_v2f32, _Ruint2_sat_rtz)(float2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i32_v3f32, _Ruint3_sat_rtz)(float3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i32_v4f32, _Ruint4_sat_rtz)(float4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i32_v8f32, _Ruint8_sat_rtz)(float8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i32_v16f32, _Ruint16_sat_rtz)(float16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i32_v2f32, _Ruint2_sat_rtp)(float2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i32_v3f32, _Ruint3_sat_rtp)(float3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i32_v4f32, _Ruint4_sat_rtp)(float4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i32_v8f32, _Ruint8_sat_rtp)(float8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i32_v16f32, _Ruint16_sat_rtp)(float16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i32_v2f32, _Ruint2_sat_rtn)(float2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i32_v3f32, _Ruint3_sat_rtn)(float3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i32_v4f32, _Ruint4_sat_rtn)(float4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i32_v8f32, _Ruint8_sat_rtn)(float8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i32_v16f32, _Ruint16_sat_rtn)(float16 x);
uint2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToU, _v2i32_v2f16, _Ruint2)(half2 x);
uint3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToU, _v3i32_v3f16, _Ruint3)(half3 x);
uint4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToU, _v4i32_v4f16, _Ruint4)(half4 x);
uint8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToU, _v8i32_v8f16, _Ruint8)(half8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i32_v16f16, _Ruint16)(half16 x);
uint2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v2i32_v2f16, _Ruint2_sat)(half2 x);
uint3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v3i32_v3f16, _Ruint3_sat)(half3 x);
uint4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v4i32_v4f16, _Ruint4_sat)(half4 x);
uint8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v8i32_v8f16, _Ruint8_sat)(half8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v16i32_v16f16, _Ruint16_sat)(half16 x);
uint2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v2i32_v2f16, _Ruint2_rte)(half2 x);
uint3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v3i32_v3f16, _Ruint3_rte)(half3 x);
uint4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v4i32_v4f16, _Ruint4_rte)(half4 x);
uint8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v8i32_v8f16, _Ruint8_rte)(half8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v16i32_v16f16, _Ruint16_rte)(half16 x);
uint2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i32_v2f16, _Ruint2_rtz)(half2 x);
uint3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i32_v3f16, _Ruint3_rtz)(half3 x);
uint4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i32_v4f16, _Ruint4_rtz)(half4 x);
uint8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i32_v8f16, _Ruint8_rtz)(half8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i32_v16f16, _Ruint16_rtz)(half16 x);
uint2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v2i32_v2f16, _Ruint2_rtp)(half2 x);
uint3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v3i32_v3f16, _Ruint3_rtp)(half3 x);
uint4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v4i32_v4f16, _Ruint4_rtp)(half4 x);
uint8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v8i32_v8f16, _Ruint8_rtp)(half8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v16i32_v16f16, _Ruint16_rtp)(half16 x);
uint2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v2i32_v2f16, _Ruint2_rtn)(half2 x);
uint3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v3i32_v3f16, _Ruint3_rtn)(half3 x);
uint4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v4i32_v4f16, _Ruint4_rtn)(half4 x);
uint8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v8i32_v8f16, _Ruint8_rtn)(half8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v16i32_v16f16, _Ruint16_rtn)(half16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i32_v2f16, _Ruint2_sat_rte)(half2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i32_v3f16, _Ruint3_sat_rte)(half3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i32_v4f16, _Ruint4_sat_rte)(half4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i32_v8f16, _Ruint8_sat_rte)(half8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i32_v16f16, _Ruint16_sat_rte)(half16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i32_v2f16, _Ruint2_sat_rtz)(half2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i32_v3f16, _Ruint3_sat_rtz)(half3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i32_v4f16, _Ruint4_sat_rtz)(half4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i32_v8f16, _Ruint8_sat_rtz)(half8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i32_v16f16, _Ruint16_sat_rtz)(half16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i32_v2f16, _Ruint2_sat_rtp)(half2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i32_v3f16, _Ruint3_sat_rtp)(half3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i32_v4f16, _Ruint4_sat_rtp)(half4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i32_v8f16, _Ruint8_sat_rtp)(half8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i32_v16f16, _Ruint16_sat_rtp)(half16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i32_v2f16, _Ruint2_sat_rtn)(half2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i32_v3f16, _Ruint3_sat_rtn)(half3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i32_v4f16, _Ruint4_sat_rtn)(half4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i32_v8f16, _Ruint8_sat_rtn)(half8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i32_v16f16, _Ruint16_sat_rtn)(half16 x);
#if defined(cl_khr_fp64)
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i32_v2f64, _Ruint2)(double2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i32_v3f64, _Ruint3)(double3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i32_v4f64, _Ruint4)(double4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i32_v8f64, _Ruint8)(double8 x);
uint16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i32_v16f64, _Ruint16)(double16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v2i32_v2f64, _Ruint2_sat)(double2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v3i32_v3f64, _Ruint3_sat)(double3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v4i32_v4f64, _Ruint4_sat)(double4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v8i32_v8f64, _Ruint8_sat)(double8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v16i32_v16f64, _Ruint16_sat)(double16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v2i32_v2f64, _Ruint2_rte)(double2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v3i32_v3f64, _Ruint3_rte)(double3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v4i32_v4f64, _Ruint4_rte)(double4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v8i32_v8f64, _Ruint8_rte)(double8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v16i32_v16f64, _Ruint16_rte)(double16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i32_v2f64, _Ruint2_rtz)(double2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i32_v3f64, _Ruint3_rtz)(double3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i32_v4f64, _Ruint4_rtz)(double4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i32_v8f64, _Ruint8_rtz)(double8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i32_v16f64, _Ruint16_rtz)(double16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v2i32_v2f64, _Ruint2_rtp)(double2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v3i32_v3f64, _Ruint3_rtp)(double3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v4i32_v4f64, _Ruint4_rtp)(double4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v8i32_v8f64, _Ruint8_rtp)(double8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v16i32_v16f64, _Ruint16_rtp)(double16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v2i32_v2f64, _Ruint2_rtn)(double2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v3i32_v3f64, _Ruint3_rtn)(double3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v4i32_v4f64, _Ruint4_rtn)(double4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v8i32_v8f64, _Ruint8_rtn)(double8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v16i32_v16f64, _Ruint16_rtn)(double16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i32_v2f64, _Ruint2_sat_rte)(double2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i32_v3f64, _Ruint3_sat_rte)(double3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i32_v4f64, _Ruint4_sat_rte)(double4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i32_v8f64, _Ruint8_sat_rte)(double8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i32_v16f64, _Ruint16_sat_rte)(double16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i32_v2f64, _Ruint2_sat_rtz)(double2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i32_v3f64, _Ruint3_sat_rtz)(double3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i32_v4f64, _Ruint4_sat_rtz)(double4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i32_v8f64, _Ruint8_sat_rtz)(double8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i32_v16f64, _Ruint16_sat_rtz)(double16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i32_v2f64, _Ruint2_sat_rtp)(double2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i32_v3f64, _Ruint3_sat_rtp)(double3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i32_v4f64, _Ruint4_sat_rtp)(double4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i32_v8f64, _Ruint8_sat_rtp)(double8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i32_v16f64, _Ruint16_sat_rtp)(double16 x);
uint2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i32_v2f64, _Ruint2_sat_rtn)(double2 x);
uint3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i32_v3f64, _Ruint3_sat_rtn)(double3 x);
uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i32_v4f64, _Ruint4_sat_rtn)(double4 x);
uint8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i32_v8f64, _Ruint8_sat_rtn)(double8 x);
uint16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i32_v16f64, _Ruint16_sat_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i64_v2f32, _Rulong2)(float2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i64_v3f32, _Rulong3)(float3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i64_v4f32, _Rulong4)(float4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i64_v8f32, _Rulong8)(float8 x);
ulong16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i64_v16f32, _Rulong16)(float16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v2i64_v2f32, _Rulong2_sat)(float2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v3i64_v3f32, _Rulong3_sat)(float3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v4i64_v4f32, _Rulong4_sat)(float4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v8i64_v8f32, _Rulong8_sat)(float8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v16i64_v16f32, _Rulong16_sat)(float16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v2i64_v2f32, _Rulong2_rte)(float2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v3i64_v3f32, _Rulong3_rte)(float3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v4i64_v4f32, _Rulong4_rte)(float4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v8i64_v8f32, _Rulong8_rte)(float8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v16i64_v16f32, _Rulong16_rte)(float16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i64_v2f32, _Rulong2_rtz)(float2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i64_v3f32, _Rulong3_rtz)(float3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i64_v4f32, _Rulong4_rtz)(float4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i64_v8f32, _Rulong8_rtz)(float8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i64_v16f32, _Rulong16_rtz)(float16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v2i64_v2f32, _Rulong2_rtp)(float2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v3i64_v3f32, _Rulong3_rtp)(float3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v4i64_v4f32, _Rulong4_rtp)(float4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v8i64_v8f32, _Rulong8_rtp)(float8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v16i64_v16f32, _Rulong16_rtp)(float16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v2i64_v2f32, _Rulong2_rtn)(float2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v3i64_v3f32, _Rulong3_rtn)(float3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v4i64_v4f32, _Rulong4_rtn)(float4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v8i64_v8f32, _Rulong8_rtn)(float8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v16i64_v16f32, _Rulong16_rtn)(float16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i64_v2f32, _Rulong2_sat_rte)(float2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i64_v3f32, _Rulong3_sat_rte)(float3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i64_v4f32, _Rulong4_sat_rte)(float4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i64_v8f32, _Rulong8_sat_rte)(float8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i64_v16f32, _Rulong16_sat_rte)(float16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i64_v2f32, _Rulong2_sat_rtz)(float2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i64_v3f32, _Rulong3_sat_rtz)(float3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i64_v4f32, _Rulong4_sat_rtz)(float4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i64_v8f32, _Rulong8_sat_rtz)(float8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i64_v16f32, _Rulong16_sat_rtz)(float16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i64_v2f32, _Rulong2_sat_rtp)(float2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i64_v3f32, _Rulong3_sat_rtp)(float3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i64_v4f32, _Rulong4_sat_rtp)(float4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i64_v8f32, _Rulong8_sat_rtp)(float8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i64_v16f32, _Rulong16_sat_rtp)(float16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i64_v2f32, _Rulong2_sat_rtn)(float2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i64_v3f32, _Rulong3_sat_rtn)(float3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i64_v4f32, _Rulong4_sat_rtn)(float4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i64_v8f32, _Rulong8_sat_rtn)(float8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i64_v16f32, _Rulong16_sat_rtn)(float16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i64_v2f16, _Rulong2)(half2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i64_v3f16, _Rulong3)(half3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i64_v4f16, _Rulong4)(half4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i64_v8f16, _Rulong8)(half8 x);
ulong16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i64_v16f16, _Rulong16)(half16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v2i64_v2f16, _Rulong2_sat)(half2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v3i64_v3f16, _Rulong3_sat)(half3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v4i64_v4f16, _Rulong4_sat)(half4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v8i64_v8f16, _Rulong8_sat)(half8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v16i64_v16f16, _Rulong16_sat)(half16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v2i64_v2f16, _Rulong2_rte)(half2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v3i64_v3f16, _Rulong3_rte)(half3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v4i64_v4f16, _Rulong4_rte)(half4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v8i64_v8f16, _Rulong8_rte)(half8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v16i64_v16f16, _Rulong16_rte)(half16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i64_v2f16, _Rulong2_rtz)(half2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i64_v3f16, _Rulong3_rtz)(half3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i64_v4f16, _Rulong4_rtz)(half4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i64_v8f16, _Rulong8_rtz)(half8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i64_v16f16, _Rulong16_rtz)(half16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v2i64_v2f16, _Rulong2_rtp)(half2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v3i64_v3f16, _Rulong3_rtp)(half3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v4i64_v4f16, _Rulong4_rtp)(half4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v8i64_v8f16, _Rulong8_rtp)(half8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v16i64_v16f16, _Rulong16_rtp)(half16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v2i64_v2f16, _Rulong2_rtn)(half2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v3i64_v3f16, _Rulong3_rtn)(half3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v4i64_v4f16, _Rulong4_rtn)(half4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v8i64_v8f16, _Rulong8_rtn)(half8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v16i64_v16f16, _Rulong16_rtn)(half16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i64_v2f16, _Rulong2_sat_rte)(half2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i64_v3f16, _Rulong3_sat_rte)(half3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i64_v4f16, _Rulong4_sat_rte)(half4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i64_v8f16, _Rulong8_sat_rte)(half8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i64_v16f16, _Rulong16_sat_rte)(half16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i64_v2f16, _Rulong2_sat_rtz)(half2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i64_v3f16, _Rulong3_sat_rtz)(half3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i64_v4f16, _Rulong4_sat_rtz)(half4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i64_v8f16, _Rulong8_sat_rtz)(half8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i64_v16f16, _Rulong16_sat_rtz)(half16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i64_v2f16, _Rulong2_sat_rtp)(half2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i64_v3f16, _Rulong3_sat_rtp)(half3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i64_v4f16, _Rulong4_sat_rtp)(half4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i64_v8f16, _Rulong8_sat_rtp)(half8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i64_v16f16, _Rulong16_sat_rtp)(half16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i64_v2f16, _Rulong2_sat_rtn)(half2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i64_v3f16, _Rulong3_sat_rtn)(half3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i64_v4f16, _Rulong4_sat_rtn)(half4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i64_v8f16, _Rulong8_sat_rtn)(half8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i64_v16f16, _Rulong16_sat_rtn)(half16 x);
#if defined(cl_khr_fp64)
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i64_v2f64, _Rulong2)(double2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i64_v3f64, _Rulong3)(double3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i64_v4f64, _Rulong4)(double4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i64_v8f64, _Rulong8)(double8 x);
ulong16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i64_v16f64, _Rulong16)(double16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v2i64_v2f64, _Rulong2_sat)(double2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v3i64_v3f64, _Rulong3_sat)(double3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v4i64_v4f64, _Rulong4_sat)(double4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v8i64_v8f64, _Rulong8_sat)(double8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_v16i64_v16f64, _Rulong16_sat)(double16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v2i64_v2f64, _Rulong2_rte)(double2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v3i64_v3f64, _Rulong3_rte)(double3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v4i64_v4f64, _Rulong4_rte)(double4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v8i64_v8f64, _Rulong8_rte)(double8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTE_v16i64_v16f64, _Rulong16_rte)(double16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i64_v2f64, _Rulong2_rtz)(double2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i64_v3f64, _Rulong3_rtz)(double3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i64_v4f64, _Rulong4_rtz)(double4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i64_v8f64, _Rulong8_rtz)(double8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i64_v16f64, _Rulong16_rtz)(double16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v2i64_v2f64, _Rulong2_rtp)(double2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v3i64_v3f64, _Rulong3_rtp)(double3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v4i64_v4f64, _Rulong4_rtp)(double4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v8i64_v8f64, _Rulong8_rtp)(double8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTP_v16i64_v16f64, _Rulong16_rtp)(double16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v2i64_v2f64, _Rulong2_rtn)(double2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v3i64_v3f64, _Rulong3_rtn)(double3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v4i64_v4f64, _Rulong4_rtn)(double4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v8i64_v8f64, _Rulong8_rtn)(double8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _RTN_v16i64_v16f64, _Rulong16_rtn)(double16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i64_v2f64, _Rulong2_sat_rte)(double2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i64_v3f64, _Rulong3_sat_rte)(double3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i64_v4f64, _Rulong4_sat_rte)(double4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i64_v8f64, _Rulong8_sat_rte)(double8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i64_v16f64, _Rulong16_sat_rte)(double16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i64_v2f64, _Rulong2_sat_rtz)(double2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i64_v3f64, _Rulong3_sat_rtz)(double3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i64_v4f64, _Rulong4_sat_rtz)(double4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i64_v8f64, _Rulong8_sat_rtz)(double8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i64_v16f64, _Rulong16_sat_rtz)(double16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i64_v2f64, _Rulong2_sat_rtp)(double2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i64_v3f64, _Rulong3_sat_rtp)(double3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i64_v4f64, _Rulong4_sat_rtp)(double4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i64_v8f64, _Rulong8_sat_rtp)(double8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i64_v16f64, _Rulong16_sat_rtp)(double16 x);
ulong2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i64_v2f64, _Rulong2_sat_rtn)(double2 x);
ulong3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i64_v3f64, _Rulong3_sat_rtn)(double3 x);
ulong4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i64_v4f64, _Rulong4_sat_rtn)(double4 x);
ulong8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i64_v8f64, _Rulong8_sat_rtn)(double8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i64_v16f64, _Rulong16_sat_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i8_v2i8, _Ruchar2)(char2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i8_v3i8, _Ruchar3)(char3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i8_v4i8, _Ruchar4)(char4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i8_v8i8, _Ruchar8)(char8 x);
uchar16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i8_v16i8, _Ruchar16)(char16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i8_v2i16, _Ruchar2)(short2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i8_v3i16, _Ruchar3)(short3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i8_v4i16, _Ruchar4)(short4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i8_v8i16, _Ruchar8)(short8 x);
uchar16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i8_v16i16, _Ruchar16)(short16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i8_v2i32, _Ruchar2)(int2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i8_v3i32, _Ruchar3)(int3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i8_v4i32, _Ruchar4)(int4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i8_v8i32, _Ruchar8)(int8 x);
uchar16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i8_v16i32, _Ruchar16)(int16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i8_v2i64, _Ruchar2)(long2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i8_v3i64, _Ruchar3)(long3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i8_v4i64, _Ruchar4)(long4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i8_v8i64, _Ruchar8)(long8 x);
uchar16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i8_v16i64, _Ruchar16)(long16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i16_v2i8, _Rushort2)(char2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i16_v3i8, _Rushort3)(char3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i16_v4i8, _Rushort4)(char4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i16_v8i8, _Rushort8)(char8 x);
ushort16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i16_v16i8, _Rushort16)(char16 x);
ushort2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i16_v2i16, _Rushort2)(short2 x);
ushort3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i16_v3i16, _Rushort3)(short3 x);
ushort4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i16_v4i16, _Rushort4)(short4 x);
ushort8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i16_v8i16, _Rushort8)(short8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertSToU, _v16i16_v16i16, _Rushort16)(short16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i16_v2i32, _Rushort2)(int2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i16_v3i32, _Rushort3)(int3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i16_v4i32, _Rushort4)(int4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i16_v8i32, _Rushort8)(int8 x);
ushort16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i16_v16i32, _Rushort16)(int16 x);
ushort2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i16_v2i64, _Rushort2)(long2 x);
ushort3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i16_v3i64, _Rushort3)(long3 x);
ushort4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i16_v4i64, _Rushort4)(long4 x);
ushort8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i16_v8i64, _Rushort8)(long8 x);
ushort16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertSToU, _v16i16_v16i64, _Rushort16)(long16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i32_v2i8, _Ruint2)(char2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i32_v3i8, _Ruint3)(char3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i32_v4i8, _Ruint4)(char4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i32_v8i8, _Ruint8)(char8 x);
uint16
    SPIRV_OVERLOADABLE   SPIRV_BUILTIN(SatConvertSToU, _v16i32_v16i8, _Ruint16)(char16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i32_v2i16, _Ruint2)(short2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i32_v3i16, _Ruint3)(short3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i32_v4i16, _Ruint4)(short4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i32_v8i16, _Ruint8)(short8 x);
uint16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i32_v16i16, _Ruint16)(short16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i32_v2i32, _Ruint2)(int2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i32_v3i32, _Ruint3)(int3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i32_v4i32, _Ruint4)(int4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i32_v8i32, _Ruint8)(int8 x);
uint16
    SPIRV_OVERLOADABLE   SPIRV_BUILTIN(SatConvertSToU, _v16i32_v16i32, _Ruint16)(int16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i32_v2i64, _Ruint2)(long2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i32_v3i64, _Ruint3)(long3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i32_v4i64, _Ruint4)(long4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i32_v8i64, _Ruint8)(long8 x);
uint16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i32_v16i64, _Ruint16)(long16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i64_v2i8, _Rulong2)(char2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i64_v3i8, _Rulong3)(char3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i64_v4i8, _Rulong4)(char4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i64_v8i8, _Rulong8)(char8 x);
ulong16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i64_v16i8, _Rulong16)(char16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i64_v2i16, _Rulong2)(short2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i64_v3i16, _Rulong3)(short3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i64_v4i16, _Rulong4)(short4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i64_v8i16, _Rulong8)(short8 x);
ulong16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertSToU, _v16i64_v16i16, _Rulong16)(short16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i64_v2i32, _Rulong2)(int2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i64_v3i32, _Rulong3)(int3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i64_v4i32, _Rulong4)(int4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i64_v8i32, _Rulong8)(int8 x);
ulong16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i64_v16i32, _Rulong16)(int16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i64_v2i64, _Rulong2)(long2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i64_v3i64, _Rulong3)(long3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i64_v4i64, _Rulong4)(long4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i64_v8i64, _Rulong8)(long8 x);
ulong16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i64_v16i64, _Rulong16)(long16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i8_v2i8, _Rchar2)(uchar2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i8_v3i8, _Rchar3)(uchar3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i8_v4i8, _Rchar4)(uchar4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i8_v8i8, _Rchar8)(uchar8 x);
char16
    SPIRV_OVERLOADABLE   SPIRV_BUILTIN(SatConvertUToS, _v16i8_v16i8, _Rchar16)(uchar16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i8_v2i16, _Rchar2)(ushort2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i8_v3i16, _Rchar3)(ushort3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i8_v4i16, _Rchar4)(ushort4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i8_v8i16, _Rchar8)(ushort8 x);
char16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i8_v16i16, _Rchar16)(ushort16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i8_v2i32, _Rchar2)(uint2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i8_v3i32, _Rchar3)(uint3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i8_v4i32, _Rchar4)(uint4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i8_v8i32, _Rchar8)(uint8 x);
char16
    SPIRV_OVERLOADABLE   SPIRV_BUILTIN(SatConvertUToS, _v16i8_v16i32, _Rchar16)(uint16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i8_v2i64, _Rchar2)(ulong2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i8_v3i64, _Rchar3)(ulong3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i8_v4i64, _Rchar4)(ulong4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i8_v8i64, _Rchar8)(ulong8 x);
char16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i8_v16i64, _Rchar16)(ulong16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i16_v2i8, _Rshort2)(uchar2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i16_v3i8, _Rshort3)(uchar3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i16_v4i8, _Rshort4)(uchar4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i16_v8i8, _Rshort8)(uchar8 x);
short16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i16_v16i8, _Rshort16)(uchar16 x);
short2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i16_v2i16, _Rshort2)(ushort2 x);
short3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i16_v3i16, _Rshort3)(ushort3 x);
short4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i16_v4i16, _Rshort4)(ushort4 x);
short8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i16_v8i16, _Rshort8)(ushort8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertUToS, _v16i16_v16i16, _Rshort16)(ushort16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i16_v2i32, _Rshort2)(uint2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i16_v3i32, _Rshort3)(uint3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i16_v4i32, _Rshort4)(uint4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i16_v8i32, _Rshort8)(uint8 x);
short16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i16_v16i32, _Rshort16)(uint16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i16_v2i64, _Rshort2)(ulong2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i16_v3i64, _Rshort3)(ulong3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i16_v4i64, _Rshort4)(ulong4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i16_v8i64, _Rshort8)(ulong8 x);
short16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertUToS, _v16i16_v16i64, _Rshort16)(ulong16 x);
int2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SatConvertUToS, _v2i32_v2i8, _Rint2)(uchar2 x);
int3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SatConvertUToS, _v3i32_v3i8, _Rint3)(uchar3 x);
int4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SatConvertUToS, _v4i32_v4i8, _Rint4)(uchar4 x);
int8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SatConvertUToS, _v8i32_v8i8, _Rint8)(uchar8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i32_v16i8, _Rint16)(uchar16 x);
int2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SatConvertUToS, _v2i32_v2i16, _Rint2)(ushort2 x);
int3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SatConvertUToS, _v3i32_v3i16, _Rint3)(ushort3 x);
int4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SatConvertUToS, _v4i32_v4i16, _Rint4)(ushort4 x);
int8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SatConvertUToS, _v8i32_v8i16, _Rint8)(ushort8 x);
int16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i32_v16i16, _Rint16)(ushort16 x);
int2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SatConvertUToS, _v2i32_v2i32, _Rint2)(uint2 x);
int3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SatConvertUToS, _v3i32_v3i32, _Rint3)(uint3 x);
int4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SatConvertUToS, _v4i32_v4i32, _Rint4)(uint4 x);
int8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SatConvertUToS, _v8i32_v8i32, _Rint8)(uint8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i32_v16i32, _Rint16)(uint16 x);
int2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SatConvertUToS, _v2i32_v2i64, _Rint2)(ulong2 x);
int3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SatConvertUToS, _v3i32_v3i64, _Rint3)(ulong3 x);
int4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SatConvertUToS, _v4i32_v4i64, _Rint4)(ulong4 x);
int8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SatConvertUToS, _v8i32_v8i64, _Rint8)(ulong8 x);
int16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i32_v16i64, _Rint16)(ulong16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i64_v2i8, _Rlong2)(uchar2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i64_v3i8, _Rlong3)(uchar3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i64_v4i8, _Rlong4)(uchar4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i64_v8i8, _Rlong8)(uchar8 x);
long16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i64_v16i8, _Rlong16)(uchar16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i64_v2i16, _Rlong2)(ushort2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i64_v3i16, _Rlong3)(ushort3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i64_v4i16, _Rlong4)(ushort4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i64_v8i16, _Rlong8)(ushort8 x);
long16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SatConvertUToS, _v16i64_v16i16, _Rlong16)(ushort16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i64_v2i32, _Rlong2)(uint2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i64_v3i32, _Rlong3)(uint3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i64_v4i32, _Rlong4)(uint4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i64_v8i32, _Rlong8)(uint8 x);
long16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i64_v16i32, _Rlong16)(uint16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i64_v2i64, _Rlong2)(ulong2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i64_v3i64, _Rlong3)(ulong3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i64_v4i64, _Rlong4)(ulong4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i64_v8i64, _Rlong8)(ulong8 x);
long16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i64_v16i64, _Rlong16)(ulong16 x);
short SPIRV_OVERLOADABLE   SPIRV_BUILTIN(ConvertFToBF16INTEL, _f32, )(float x);
short2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToBF16INTEL, _v2f32, )(float2 x);
short3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToBF16INTEL, _v3f32, )(float3 x);
short4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToBF16INTEL, _v4f32, )(float4 x);
short8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertFToBF16INTEL, _v8f32, )(float8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToBF16INTEL, _v16f32, )(float16 x);
float SPIRV_OVERLOADABLE   SPIRV_BUILTIN(ConvertBF16ToFINTEL, _i16, )(short x);
float2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertBF16ToFINTEL, _v2i16, )(short2 x);
float3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertBF16ToFINTEL, _v3i16, )(short3 x);
float4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertBF16ToFINTEL, _v4i16, )(short4 x);
float8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(ConvertBF16ToFINTEL, _v8i16, )(short8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertBF16ToFINTEL, _v16i16, )(short16 x);

float SPIRV_OVERLOADABLE   SPIRV_BUILTIN(RoundFToTF32INTEL, _f32, )(float x);
float2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(RoundFToTF32INTEL, _v2f32, )(float2 x);
float3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(RoundFToTF32INTEL, _v3f32, )(float3 x);
float4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(RoundFToTF32INTEL, _v4f32, )(float4 x);
float8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(RoundFToTF32INTEL, _v8f32, )(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(RoundFToTF32INTEL, _v16f32, )(float16 x);

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
private
void *SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    GenericCastToPtrExplicit,
    _p0i8_p4i8_i32,
    _ToPrivate)(generic char *Pointer, int Storage);
local void *
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(
        generic char *Pointer, int Storage);
global void *
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GenericCastToPtrExplicit, _p1i8_p4i8_i32, _ToGlobal)(
        generic char *Pointer, int Storage);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GenericPtrMemSemantics, _p4i8, )(generic char *Pointer);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

// Arithmetic Instructions

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v2f16_v2f16, )(half2 Vector1, half2 Vector2);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v3f16_v3f16, )(half3 Vector1, half3 Vector2);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v4f16_v4f16, )(half4 Vector1, half4 Vector2);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v8f16_v8f16, )(half8 Vector1, half8 Vector2);
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(Dot, _v16f16_v16f16, )(half16 Vector1, half16 Vector2);
float
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v2f32_v2f32, )(float2 Vector1, float2 Vector2);
float
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v3f32_v3f32, )(float3 Vector1, float3 Vector2);
float
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v4f32_v4f32, )(float4 Vector1, float4 Vector2);
float
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v8f32_v8f32, )(float8 Vector1, float8 Vector2);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(Dot, _v16f32_v16f32, )(float16 Vector1, float16 Vector2);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(Dot, _v2f64_v2f64, )(double2 Vector1, double2 Vector2);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(Dot, _v3f64_v3f64, )(double3 Vector1, double3 Vector2);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(Dot, _v4f64_v4f64, )(double4 Vector1, double4 Vector2);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(Dot, _v8f64_v8f64, )(double8 Vector1, double8 Vector2);
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(Dot, _v16f64_v16f64, )(double16 Vector1, double16 Vector2);
#endif // defined(cl_khr_fp64)
typedef struct
{
    uchar a;
    uchar b;
} TwoOp_i8;
typedef struct
{
    uchar2 a;
    uchar2 b;
} TwoOp_v2i8;
typedef struct
{
    uchar3 a;
    uchar3 b;
} TwoOp_v3i8;
typedef struct
{
    uchar4 a;
    uchar4 b;
} TwoOp_v4i8;
typedef struct
{
    uchar8 a;
    uchar8 b;
} TwoOp_v8i8;
typedef struct
{
    uchar16 a;
    uchar16 b;
} TwoOp_v16i8;
typedef struct
{
    ushort a;
    ushort b;
} TwoOp_i16;
typedef struct
{
    ushort2 a;
    ushort2 b;
} TwoOp_v2i16;
typedef struct
{
    ushort3 a;
    ushort3 b;
} TwoOp_v3i16;
typedef struct
{
    ushort4 a;
    ushort4 b;
} TwoOp_v4i16;
typedef struct
{
    ushort8 a;
    ushort8 b;
} TwoOp_v8i16;
typedef struct
{
    ushort16 a;
    ushort16 b;
} TwoOp_v16i16;
typedef struct
{
    uint a;
    uint b;
} TwoOp_i32;
typedef struct
{
    uint2 a;
    uint2 b;
} TwoOp_v2i32;
typedef struct
{
    uint3 a;
    uint3 b;
} TwoOp_v3i32;
typedef struct
{
    uint4 a;
    uint4 b;
} TwoOp_v4i32;
typedef struct
{
    uint8 a;
    uint8 b;
} TwoOp_v8i32;
typedef struct
{
    uint16 a;
    uint16 b;
} TwoOp_v16i32;
typedef struct
{
    ulong a;
    ulong b;
} TwoOp_i64;
typedef struct
{
    ulong2 a;
    ulong2 b;
} TwoOp_v2i64;
typedef struct
{
    ulong3 a;
    ulong3 b;
} TwoOp_v3i64;
typedef struct
{
    ulong4 a;
    ulong4 b;
} TwoOp_v4i64;
typedef struct
{
    ulong8 a;
    ulong8 b;
} TwoOp_v8i64;
typedef struct
{
    ulong16 a;
    ulong16 b;
} TwoOp_v16i64;

TwoOp_i8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _i8_i8, )(uchar Operand1, uchar Operand2);
TwoOp_v2i8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v2i8_v2i8, )(uchar2 Operand1, uchar2 Operand2);
TwoOp_v3i8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v3i8_v3i8, )(uchar3 Operand1, uchar3 Operand2);
TwoOp_v4i8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v4i8_v4i8, )(uchar4 Operand1, uchar4 Operand2);
TwoOp_v8i8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v8i8_v8i8, )(uchar8 Operand1, uchar8 Operand2);
TwoOp_v16i8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v16i8_v16i8, )(uchar16 Operand1, uchar16 Operand2);
TwoOp_i16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _i16_i16, )(ushort Operand1, ushort Operand2);
TwoOp_v2i16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v2i16_v2i16, )(ushort2 Operand1, ushort2 Operand2);
TwoOp_v3i16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v3i16_v3i16, )(ushort3 Operand1, ushort3 Operand2);
TwoOp_v4i16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v4i16_v4i16, )(ushort4 Operand1, ushort4 Operand2);
TwoOp_v8i16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v8i16_v8i16, )(ushort8 Operand1, ushort8 Operand2);
TwoOp_v16i16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v16i16_v16i16, )(ushort16 Operand1, ushort16 Operand2);
TwoOp_i32 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _i32_i32, )(uint Operand1, uint Operand2);
TwoOp_v2i32 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v2i32_v2i32, )(uint2 Operand1, uint2 Operand2);
TwoOp_v3i32 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v3i32_v3i32, )(uint3 Operand1, uint3 Operand2);
TwoOp_v4i32 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v4i32_v4i32, )(uint4 Operand1, uint4 Operand2);
TwoOp_v8i32 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v8i32_v8i32, )(uint8 Operand1, uint8 Operand2);
TwoOp_v16i32 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v16i32_v16i32, )(uint16 Operand1, uint16 Operand2);
TwoOp_i64 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _i64_i64, )(ulong Operand1, ulong Operand2);
TwoOp_v2i64 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v2i64_v2i64, )(ulong2 Operand1, ulong2 Operand2);
TwoOp_v3i64 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v3i64_v3i64, )(ulong3 Operand1, ulong3 Operand2);
TwoOp_v4i64 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v4i64_v4i64, )(ulong4 Operand1, ulong4 Operand2);
TwoOp_v8i64 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v8i64_v8i64, )(ulong8 Operand1, ulong8 Operand2);
TwoOp_v16i64 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(UMulExtended, _v16i64_v16i64, )(ulong16 Operand1, ulong16 Operand2);

TwoOp_i8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _i8_i8, )(char Operand1, char Operand2);
TwoOp_v2i8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v2i8_v2i8, )(char2 Operand1, char2 Operand2);
TwoOp_v3i8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v3i8_v3i8, )(char3 Operand1, char3 Operand2);
TwoOp_v4i8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v4i8_v4i8, )(char4 Operand1, char4 Operand2);
TwoOp_v8i8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v8i8_v8i8, )(char8 Operand1, char8 Operand2);
TwoOp_v16i8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v16i8_v16i8, )(char16 Operand1, char16 Operand2);
TwoOp_i16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _i16_i16, )(short Operand1, short Operand2);
TwoOp_v2i16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v2i16_v2i16, )(short2 Operand1, short2 Operand2);
TwoOp_v3i16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v3i16_v3i16, )(short3 Operand1, short3 Operand2);
TwoOp_v4i16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v4i16_v4i16, )(short4 Operand1, short4 Operand2);
TwoOp_v8i16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v8i16_v8i16, )(short8 Operand1, short8 Operand2);
TwoOp_v16i16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v16i16_v16i16, )(short16 Operand1, short16 Operand2);
TwoOp_i32 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _i32_i32, )(int Operand1, int Operand2);
TwoOp_v2i32 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v2i32_v2i32, )(int2 Operand1, int2 Operand2);
TwoOp_v3i32 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v3i32_v3i32, )(int3 Operand1, int3 Operand2);
TwoOp_v4i32 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v4i32_v4i32, )(int4 Operand1, int4 Operand2);
TwoOp_v8i32 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v8i32_v8i32, )(int8 Operand1, int8 Operand2);
TwoOp_v16i32 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v16i32_v16i32, )(int16 Operand1, int16 Operand2);
TwoOp_i64 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _i64_i64, )(long Operand1, long Operand2);
TwoOp_v2i64 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v2i64_v2i64, )(long2 Operand1, long2 Operand2);
TwoOp_v3i64 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v3i64_v3i64, )(long3 Operand1, long3 Operand2);
TwoOp_v4i64 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v4i64_v4i64, )(long4 Operand1, long4 Operand2);
TwoOp_v8i64 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v8i64_v8i64, )(long8 Operand1, long8 Operand2);
TwoOp_v16i64 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SMulExtended, _v16i64_v16i64, )(long16 Operand1, long16 Operand2);

// Bit Instructions

char SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _i8_i8_i32_i32, )(
    char Base, char Insert, uint Offset, uint Count);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v2i8_v2i8_i32_i32, )(
    char2 Base, char2 Insert, uint Offset, uint Count);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v3i8_v3i8_i32_i32, )(
    char3 Base, char3 Insert, uint Offset, uint Count);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v4i8_v4i8_i32_i32, )(
    char4 Base, char4 Insert, uint Offset, uint Count);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v8i8_v8i8_i32_i32, )(
    char8 Base, char8 Insert, uint Offset, uint Count);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v16i8_v16i8_i32_i32, )(
    char16 Base, char16 Insert, uint Offset, uint Count);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _i16_i16_i32_i32, )(
    short Base, short Insert, uint Offset, uint Count);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v2i16_v2i16_i32_i32, )(
    short2 Base, short2 Insert, uint Offset, uint Count);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v3i16_v3i16_i32_i32, )(
    short3 Base, short3 Insert, uint Offset, uint Count);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v4i16_v4i16_i32_i32, )(
    short4 Base, short4 Insert, uint Offset, uint Count);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v8i16_v8i16_i32_i32, )(
    short8 Base, short8 Insert, uint Offset, uint Count);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v16i16_v16i16_i32_i32, )(
    short16 Base, short16 Insert, uint Offset, uint Count);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _i32_i32_i32_i32, )(
    int Base, int Insert, uint Offset, uint Count);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v2i32_v2i32_i32_i32, )(
    int2 Base, int2 Insert, uint Offset, uint Count);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v3i32_v3i32_i32_i32, )(
    int3 Base, int3 Insert, uint Offset, uint Count);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v4i32_v4i32_i32_i32, )(
    int4 Base, int4 Insert, uint Offset, uint Count);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v8i32_v8i32_i32_i32, )(
    int8 Base, int8 Insert, uint Offset, uint Count);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v16i32_v16i32_i32_i32, )(
    int16 Base, int16 Insert, uint Offset, uint Count);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _i64_i64_i32_i32, )(
    long Base, long Insert, uint Offset, uint Count);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v2i64_v2i64_i32_i32, )(
    long2 Base, long2 Insert, uint Offset, uint Count);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v3i64_v3i64_i32_i32, )(
    long3 Base, long3 Insert, uint Offset, uint Count);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v4i64_v4i64_i32_i32, )(
    long4 Base, long4 Insert, uint Offset, uint Count);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v8i64_v8i64_i32_i32, )(
    long8 Base, long8 Insert, uint Offset, uint Count);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v16i64_v16i64_i32_i32, )(
    long16 Base, long16 Insert, uint Offset, uint Count);

char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(BitFieldSExtract, _i8_i32_i32, )(char Base, uint Offset, uint Count);
char2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(BitFieldSExtract, _v2i8_i32_i32, )(char2 Base, uint Offset, uint Count);
char3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(BitFieldSExtract, _v3i8_i32_i32, )(char3 Base, uint Offset, uint Count);
char4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(BitFieldSExtract, _v4i8_i32_i32, )(char4 Base, uint Offset, uint Count);
char8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(BitFieldSExtract, _v8i8_i32_i32, )(char8 Base, uint Offset, uint Count);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldSExtract, _v16i8_i32_i32, )(
    char16 Base, uint Offset, uint Count);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(BitFieldSExtract, _i16_i32_i32, )(short Base, uint Offset, uint Count);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldSExtract, _v2i16_i32_i32, )(
    short2 Base, uint Offset, uint Count);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldSExtract, _v3i16_i32_i32, )(
    short3 Base, uint Offset, uint Count);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldSExtract, _v4i16_i32_i32, )(
    short4 Base, uint Offset, uint Count);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldSExtract, _v8i16_i32_i32, )(
    short8 Base, uint Offset, uint Count);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldSExtract, _v16i16_i32_i32, )(
    short16 Base, uint Offset, uint Count);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(BitFieldSExtract, _i32_i32_i32, )(int Base, uint Offset, uint Count);
int2 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(BitFieldSExtract, _v2i32_i32_i32, )(int2 Base, uint Offset, uint Count);
int3 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(BitFieldSExtract, _v3i32_i32_i32, )(int3 Base, uint Offset, uint Count);
int4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(BitFieldSExtract, _v4i32_i32_i32, )(int4 Base, uint Offset, uint Count);
int8 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(BitFieldSExtract, _v8i32_i32_i32, )(int8 Base, uint Offset, uint Count);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldSExtract, _v16i32_i32_i32, )(
    int16 Base, uint Offset, uint Count);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(BitFieldSExtract, _i64_i32_i32, )(long Base, uint Offset, uint Count);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldSExtract, _v2i64_i32_i32, )(
    long2 Base, uint Offset, uint Count);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldSExtract, _v3i64_i32_i32, )(
    long3 Base, uint Offset, uint Count);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldSExtract, _v4i64_i32_i32, )(
    long4 Base, uint Offset, uint Count);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldSExtract, _v8i64_i32_i32, )(
    long8 Base, uint Offset, uint Count);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldSExtract, _v16i64_i32_i32, )(
    long16 Base, uint Offset, uint Count);

uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(BitFieldUExtract, _i8_i32_i32, )(uchar Base, uint Offset, uint Count);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v2i8_i32_i32, )(
    uchar2 Base, uint Offset, uint Count);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v3i8_i32_i32, )(
    uchar3 Base, uint Offset, uint Count);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v4i8_i32_i32, )(
    uchar4 Base, uint Offset, uint Count);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v8i8_i32_i32, )(
    uchar8 Base, uint Offset, uint Count);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v16i8_i32_i32, )(
    uchar16 Base, uint Offset, uint Count);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(BitFieldUExtract, _i16_i32_i32, )(ushort Base, uint Offset, uint Count);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v2i16_i32_i32, )(
    ushort2 Base, uint Offset, uint Count);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v3i16_i32_i32, )(
    ushort3 Base, uint Offset, uint Count);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v4i16_i32_i32, )(
    ushort4 Base, uint Offset, uint Count);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v8i16_i32_i32, )(
    ushort8 Base, uint Offset, uint Count);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v16i16_i32_i32, )(
    ushort16 Base, uint Offset, uint Count);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(BitFieldUExtract, _i32_i32_i32, )(uint Base, uint Offset, uint Count);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v2i32_i32_i32, )(
    uint2 Base, uint Offset, uint Count);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v3i32_i32_i32, )(
    uint3 Base, uint Offset, uint Count);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v4i32_i32_i32, )(
    uint4 Base, uint Offset, uint Count);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v8i32_i32_i32, )(
    uint8 Base, uint Offset, uint Count);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v16i32_i32_i32, )(
    uint16 Base, uint Offset, uint Count);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(BitFieldUExtract, _i64_i32_i32, )(ulong Base, uint Offset, uint Count);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v2i64_i32_i32, )(
    ulong2 Base, uint Offset, uint Count);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v3i64_i32_i32, )(
    ulong3 Base, uint Offset, uint Count);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v4i64_i32_i32, )(
    ulong4 Base, uint Offset, uint Count);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v8i64_i32_i32, )(
    ulong8 Base, uint Offset, uint Count);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldUExtract, _v16i64_i32_i32, )(
    ulong16 Base, uint Offset, uint Count);

char SPIRV_OVERLOADABLE    SPIRV_BUILTIN(BitReverse, _i8, )(char Base);
char2 SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitReverse, _v2i8, )(char2 Base);
char3 SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitReverse, _v3i8, )(char3 Base);
char4 SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitReverse, _v4i8, )(char4 Base);
char8 SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitReverse, _v8i8, )(char8 Base);
char16 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(BitReverse, _v16i8, )(char16 Base);
short SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitReverse, _i16, )(short Base);
short2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(BitReverse, _v2i16, )(short2 Base);
short3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(BitReverse, _v3i16, )(short3 Base);
short4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(BitReverse, _v4i16, )(short4 Base);
short8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(BitReverse, _v8i16, )(short8 Base);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitReverse, _v16i16, )(short16 Base);
int SPIRV_OVERLOADABLE     SPIRV_BUILTIN(BitReverse, _i32, )(int Base);
int2 SPIRV_OVERLOADABLE    SPIRV_BUILTIN(BitReverse, _v2i32, )(int2 Base);
int3 SPIRV_OVERLOADABLE    SPIRV_BUILTIN(BitReverse, _v3i32, )(int3 Base);
int4 SPIRV_OVERLOADABLE    SPIRV_BUILTIN(BitReverse, _v4i32, )(int4 Base);
int8 SPIRV_OVERLOADABLE    SPIRV_BUILTIN(BitReverse, _v8i32, )(int8 Base);
int16 SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitReverse, _v16i32, )(int16 Base);
long SPIRV_OVERLOADABLE    SPIRV_BUILTIN(BitReverse, _i64, )(long Base);
long2 SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitReverse, _v2i64, )(long2 Base);
long3 SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitReverse, _v3i64, )(long3 Base);
long4 SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitReverse, _v4i64, )(long4 Base);
long8 SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitReverse, _v8i64, )(long8 Base);
long16 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(BitReverse, _v16i64, )(long16 Base);

uchar SPIRV_OVERLOADABLE    SPIRV_BUILTIN(BitCount, _i8, )(char Base);
uchar2 SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitCount, _v2i8, )(char2 Base);
uchar3 SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitCount, _v3i8, )(char3 Base);
uchar4 SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitCount, _v4i8, )(char4 Base);
uchar8 SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitCount, _v8i8, )(char8 Base);
uchar16 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(BitCount, _v16i8, )(char16 Base);
ushort SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitCount, _i16, )(short Base);
ushort2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(BitCount, _v2i16, )(short2 Base);
ushort3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(BitCount, _v3i16, )(short3 Base);
ushort4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(BitCount, _v4i16, )(short4 Base);
ushort8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(BitCount, _v8i16, )(short8 Base);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitCount, _v16i16, )(short16 Base);
uint SPIRV_OVERLOADABLE     SPIRV_BUILTIN(BitCount, _i32, )(int Base);
uint2 SPIRV_OVERLOADABLE    SPIRV_BUILTIN(BitCount, _v2i32, )(int2 Base);
uint3 SPIRV_OVERLOADABLE    SPIRV_BUILTIN(BitCount, _v3i32, )(int3 Base);
uint4 SPIRV_OVERLOADABLE    SPIRV_BUILTIN(BitCount, _v4i32, )(int4 Base);
uint8 SPIRV_OVERLOADABLE    SPIRV_BUILTIN(BitCount, _v8i32, )(int8 Base);
uint16 SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitCount, _v16i32, )(int16 Base);
ulong SPIRV_OVERLOADABLE    SPIRV_BUILTIN(BitCount, _i64, )(long Base);
ulong2 SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitCount, _v2i64, )(long2 Base);
ulong3 SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitCount, _v3i64, )(long3 Base);
ulong4 SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitCount, _v4i64, )(long4 Base);
ulong8 SPIRV_OVERLOADABLE   SPIRV_BUILTIN(BitCount, _v8i64, )(long8 Base);
ulong16 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(BitCount, _v16i64, )(long16 Base);

// Relational and Logical Instructions

// Our OpenCL C builtins currently do not call this
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Any, _v2i1, )(__bool2 Vector);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Any, _v3i1, )(__bool3 Vector);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Any, _v4i1, )(__bool4 Vector);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Any, _v8i1, )(__bool8 Vector);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Any, _v16i1, )(__bool16 Vector);

// Our OpenCL C builtins currently do not call this
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(All, _v2i1, )(__bool2 Vector);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(All, _v3i1, )(__bool3 Vector);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(All, _v4i1, )(__bool4 Vector);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(All, _v8i1, )(__bool8 Vector);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(All, _v16i1, )(__bool16 Vector);

bool SPIRV_OVERLOADABLE     SPIRV_BUILTIN(IsNan, _f16, )(half x);
bool SPIRV_OVERLOADABLE     SPIRV_BUILTIN(IsNan, _f32, )(float x);
__bool2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNan, _v2f16, )(half2 x);
__bool2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNan, _v2f32, )(float2 x);
__bool3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNan, _v3f16, )(half3 x);
__bool3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNan, _v3f32, )(float3 x);
__bool4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNan, _v4f16, )(half4 x);
__bool4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNan, _v4f32, )(float4 x);
__bool8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNan, _v8f16, )(half8 x);
__bool8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNan, _v8f32, )(float8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _v16f16, )(half16 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE     SPIRV_BUILTIN(IsNan, _f64, )(double x);
__bool2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNan, _v2f64, )(double2 x);
__bool3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNan, _v3f64, )(double3 x);
__bool4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNan, _v4f64, )(double4 x);
__bool8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNan, _v8f64, )(double8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)

bool SPIRV_OVERLOADABLE     SPIRV_BUILTIN(IsInf, _f16, )(half x);
bool SPIRV_OVERLOADABLE     SPIRV_BUILTIN(IsInf, _f32, )(float x);
__bool2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsInf, _v2f16, )(half2 x);
__bool2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsInf, _v2f32, )(float2 x);
__bool3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsInf, _v3f16, )(half3 x);
__bool3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsInf, _v3f32, )(float3 x);
__bool4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsInf, _v4f16, )(half4 x);
__bool4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsInf, _v4f32, )(float4 x);
__bool8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsInf, _v8f16, )(half8 x);
__bool8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsInf, _v8f32, )(float8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _v16f16, )(half16 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE     SPIRV_BUILTIN(IsInf, _f64, )(double x);
__bool2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsInf, _v2f64, )(double2 x);
__bool3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsInf, _v3f64, )(double3 x);
__bool4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsInf, _v4f64, )(double4 x);
__bool8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsInf, _v8f64, )(double8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)

bool SPIRV_OVERLOADABLE     SPIRV_BUILTIN(IsFinite, _f16, )(half x);
bool SPIRV_OVERLOADABLE     SPIRV_BUILTIN(IsFinite, _f32, )(float x);
__bool2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsFinite, _v2f16, )(half2 x);
__bool2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsFinite, _v2f32, )(float2 x);
__bool3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsFinite, _v3f16, )(half3 x);
__bool3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsFinite, _v3f32, )(float3 x);
__bool4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsFinite, _v4f16, )(half4 x);
__bool4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsFinite, _v4f32, )(float4 x);
__bool8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsFinite, _v8f16, )(half8 x);
__bool8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsFinite, _v8f32, )(float8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _v16f16, )(half16 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE     SPIRV_BUILTIN(IsFinite, _f64, )(double x);
__bool2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsFinite, _v2f64, )(double2 x);
__bool3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsFinite, _v3f64, )(double3 x);
__bool4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsFinite, _v4f64, )(double4 x);
__bool8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsFinite, _v8f64, )(double8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)

bool SPIRV_OVERLOADABLE     SPIRV_BUILTIN(IsNormal, _f16, )(half x);
bool SPIRV_OVERLOADABLE     SPIRV_BUILTIN(IsNormal, _f32, )(float x);
__bool2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNormal, _v2f16, )(half2 x);
__bool2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNormal, _v2f32, )(float2 x);
__bool3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNormal, _v3f16, )(half3 x);
__bool3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNormal, _v3f32, )(float3 x);
__bool4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNormal, _v4f16, )(half4 x);
__bool4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNormal, _v4f32, )(float4 x);
__bool8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNormal, _v8f16, )(half8 x);
__bool8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNormal, _v8f32, )(float8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _v16f16, )(half16 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE     SPIRV_BUILTIN(IsNormal, _f64, )(double x);
__bool2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNormal, _v2f64, )(double2 x);
__bool3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNormal, _v3f64, )(double3 x);
__bool4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNormal, _v4f64, )(double4 x);
__bool8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(IsNormal, _v8f64, )(double8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)

bool SPIRV_OVERLOADABLE     SPIRV_BUILTIN(SignBitSet, _f16, )(half x);
bool SPIRV_OVERLOADABLE     SPIRV_BUILTIN(SignBitSet, _f32, )(float x);
__bool2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SignBitSet, _v2f16, )(half2 x);
__bool2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SignBitSet, _v2f32, )(float2 x);
__bool3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SignBitSet, _v3f16, )(half3 x);
__bool3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SignBitSet, _v3f32, )(float3 x);
__bool4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SignBitSet, _v4f16, )(half4 x);
__bool4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SignBitSet, _v4f32, )(float4 x);
__bool8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SignBitSet, _v8f16, )(half8 x);
__bool8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SignBitSet, _v8f32, )(float8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _v16f16, )(half16 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE     SPIRV_BUILTIN(SignBitSet, _f64, )(double x);
__bool2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SignBitSet, _v2f64, )(double2 x);
__bool3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SignBitSet, _v3f64, )(double3 x);
__bool4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SignBitSet, _v4f64, )(double4 x);
__bool8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(SignBitSet, _v8f64, )(double8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)

bool SPIRV_OVERLOADABLE    SPIRV_BUILTIN(LessOrGreater, _f16_f16, )(half x, half y);
bool SPIRV_OVERLOADABLE    SPIRV_BUILTIN(LessOrGreater, _f32_f32, )(float x, float y);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v2f16_v2f16, )(half2 x, half2 y);
__bool2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v2f32_v2f32, )(float2 x, float2 y);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v3f16_v3f16, )(half3 x, half3 y);
__bool3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v3f32_v3f32, )(float3 x, float3 y);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v4f16_v4f16, )(half4 x, half4 y);
__bool4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v4f32_v4f32, )(float4 x, float4 y);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v8f16_v8f16, )(half8 x, half8 y);
__bool8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v8f32_v8f32, )(float8 x, float8 y);
__bool16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v16f16_v16f16, )(half16 x, half16 y);
__bool16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(LessOrGreater, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _f64_f64, )(double x, double y);
__bool2
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v2f64_v2f64, )(double2 x, double2 y);
__bool3
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v3f64_v3f64, )(double3 x, double3 y);
__bool4
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v4f64_v4f64, )(double4 x, double4 y);
__bool8
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v8f64_v8f64, )(double8 x, double8 y);
__bool16 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(LessOrGreater, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)

bool SPIRV_OVERLOADABLE     SPIRV_BUILTIN(Ordered, _f16_f16, )(half x, half y);
bool SPIRV_OVERLOADABLE     SPIRV_BUILTIN(Ordered, _f32_f32, )(float x, float y);
__bool2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(Ordered, _v2f16_v2f16, )(half2 x, half2 y);
__bool2 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(Ordered, _v2f32_v2f32, )(float2 x, float2 y);
__bool3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(Ordered, _v3f16_v3f16, )(half3 x, half3 y);
__bool3 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(Ordered, _v3f32_v3f32, )(float3 x, float3 y);
__bool4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(Ordered, _v4f16_v4f16, )(half4 x, half4 y);
__bool4 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(Ordered, _v4f32_v4f32, )(float4 x, float4 y);
__bool8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(Ordered, _v8f16_v8f16, )(half8 x, half8 y);
__bool8 SPIRV_OVERLOADABLE  SPIRV_BUILTIN(Ordered, _v8f32_v8f32, )(float8 x, float8 y);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v16f16_v16f16, )(half16 x, half16 y);
__bool16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE    SPIRV_BUILTIN(Ordered, _f64_f64, )(double x, double y);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v2f64_v2f64, )(double2 x, double2 y);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v3f64_v3f64, )(double3 x, double3 y);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v4f64_v4f64, )(double4 x, double4 y);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v8f64_v8f64, )(double8 x, double8 y);
__bool16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)

bool SPIRV_OVERLOADABLE    SPIRV_BUILTIN(Unordered, _f16_f16, )(half x, half y);
bool SPIRV_OVERLOADABLE    SPIRV_BUILTIN(Unordered, _f32_f32, )(float x, float y);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v2f16_v2f16, )(half2 x, half2 y);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v2f32_v2f32, )(float2 x, float2 y);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v3f16_v3f16, )(half3 x, half3 y);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v3f32_v3f32, )(float3 x, float3 y);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v4f16_v4f16, )(half4 x, half4 y);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v4f32_v4f32, )(float4 x, float4 y);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v8f16_v8f16, )(half8 x, half8 y);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v8f32_v8f32, )(float8 x, float8 y);
__bool16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v16f16_v16f16, )(half16 x, half16 y);
__bool16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE    SPIRV_BUILTIN(Unordered, _f64_f64, )(double x, double y);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v2f64_v2f64, )(double2 x, double2 y);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v3f64_v3f64, )(double3 x, double3 y);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v4f64_v4f64, )(double4 x, double4 y);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v8f64_v8f64, )(double8 x, double8 y);
__bool16
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)

// Atomic Instructions

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p0i32_i32_i32, )(
    private int *Pointer, int Scope, int Semantics);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p1i32_i32_i32, )(
    global int *Pointer, int Scope, int Semantics);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p3i32_i32_i32, )(
    local int *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p4i32_i32_i32, )(
    generic int *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p0i64_i32_i32, )(
    private long *Pointer, int Scope, int Semantics);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p1i64_i32_i32, )(
    global long *Pointer, int Scope, int Semantics);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p3i64_i32_i32, )(
    local long *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p4i64_i32_i32, )(
    generic long *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p0f32_i32_i32, )(
    private float *Pointer, int Scope, int Semantics);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p1f32_i32_i32, )(
    global float *Pointer, int Scope, int Semantics);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p3f32_i32_i32, )(
    local float *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p4f32_i32_i32, )(
    generic float *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
#if defined(cl_khr_int64_base_atomics)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p0f64_i32_i32, )(
    private double *Pointer, int Scope, int Semantics);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p1f64_i32_i32, )(
    global double *Pointer, int Scope, int Semantics);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p3f64_i32_i32, )(
    local double *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p4f64_i32_i32, )(
    generic double *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p0f16_i32_i32, )(
    private half *Pointer, int Scope, int Semantics);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p1f16_i32_i32, )(
    global half *Pointer, int Scope, int Semantics);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p3f16_i32_i32, )(
    local half *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p4f16_i32_i32, )(
    generic half *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp16)

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p0i32_i32_i32_i32, )(
    private int *Pointer, int Scope, int Semantics, int Value);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p1i32_i32_i32_i32, )(
    global int *Pointer, int Scope, int Semantics, int Value);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p3i32_i32_i32_i32, )(
    local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p4i32_i32_i32_i32, )(
    generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p0i64_i32_i32_i64, )(
    private long *Pointer, int Scope, int Semantics, long Value);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p1i64_i32_i32_i64, )(
    global long *Pointer, int Scope, int Semantics, long Value);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p3i64_i32_i32_i64, )(
    local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p4i64_i32_i32_i64, )(
    generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p0f32_i32_i32_f32, )(
    private float *Pointer, int Scope, int Semantics, float Value);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p1f32_i32_i32_f32, )(
    global float *Pointer, int Scope, int Semantics, float Value);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p3f32_i32_i32_f32, )(
    local float *Pointer, int Scope, int Semantics, float Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p4f32_i32_i32_f32, )(
    generic float *Pointer, int Scope, int Semantics, float Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p0f64_i32_i32_f64, )(
    private double *Pointer, int Scope, int Semantics, double Value);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p1f64_i32_i32_f64, )(
    global double *Pointer, int Scope, int Semantics, double Value);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p3f64_i32_i32_f64, )(
    local double *Pointer, int Scope, int Semantics, double Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p4f64_i32_i32_f64, )(
    generic double *Pointer, int Scope, int Semantics, double Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p0f16_i32_i32_f16, )(
    private half *Pointer, int Scope, int Semantics, half Value);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p1f16_i32_i32_f16, )(
    global half *Pointer, int Scope, int Semantics, half Value);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p3f16_i32_i32_f16, )(
    local half *Pointer, int Scope, int Semantics, half Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p4f16_i32_i32_f16, )(
    generic half *Pointer, int Scope, int Semantics, half Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp16)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p0i32_i32_i32_i32, )(
    private int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p1i32_i32_i32_i32, )(
    global int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p3i32_i32_i32_i32, )(
    local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p4i32_i32_i32_i32, )(
    generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p0i64_i32_i32_i64, )(
    private long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p1i64_i32_i32_i64, )(
    global long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p3i64_i32_i32_i64, )(
    local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p4i64_i32_i32_i64, )(
    generic long *Pointer, int Scope, int Semantics, long Value);
#endif                   // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif                   // defined(cl_khr_int64_base_atomics)

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p0f32_i32_i32_f32, )(
    private float *Pointer, int Scope, int Semantics, float Value);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p1f32_i32_i32_f32, )(
    global float *Pointer, int Scope, int Semantics, float Value);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p3f32_i32_i32_f32, )(
    local float *Pointer, int Scope, int Semantics, float Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p4f32_i32_i32_f32, )(
    generic float *Pointer, int Scope, int Semantics, float Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#if defined(cl_khr_fp64)
#if defined(cl_khr_int64_base_atomics)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p0f64_i32_i32_f64, )(
    private double *Pointer, int Scope, int Semantics, double Value);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p1f64_i32_i32_f64, )(
    global double *Pointer, int Scope, int Semantics, double Value);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p3f64_i32_i32_f64, )(
    local double *Pointer, int Scope, int Semantics, double Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p4f64_i32_i32_f64, )(
    generic double *Pointer, int Scope, int Semantics, double Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p0f16_i32_i32_f16, )(
    __private half *Pointer, int Scope, int Semantics, half Value);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p1f16_i32_i32_f16, )(
    __global half *Pointer, int Scope, int Semantics, half Value);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p3f16_i32_i32_f16, )(
    __local half *Pointer, int Scope, int Semantics, half Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p4f16_i32_i32_f16, )(
    __generic half *Pointer, int Scope, int Semantics, half Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp16)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p0i32_i32_i32_i32_i32_i32, )(
    private int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p1i32_i32_i32_i32_i32_i32, )(
    global int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p3i32_i32_i32_i32_i32_i32, )(
    local int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p4i32_i32_i32_i32_i32_i32, )(
    generic int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p0i64_i32_i32_i32_i64_i64, )(
        private long *Pointer,
        int           Scope,
        int           Equal,
        int           Unequal,
        long          Value,
        long          Comparator);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    AtomicCompareExchange, _p1i64_i32_i32_i32_i64_i64, )(
    global long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    AtomicCompareExchange, _p3i64_i32_i32_i32_i64_i64, )(
    local long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p4i64_i32_i32_i32_i64_i64, )(
        generic long *Pointer,
        int           Scope,
        int           Equal,
        int           Unequal,
        long          Value,
        long          Comparator);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

float
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p0f32_i32_i32_i32_f32_f32, )(
        private float *Pointer,
        int            Scope,
        int            Equal,
        int            Unequal,
        float          Value,
        float          Comparator);
float
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p1f32_i32_i32_i32_f32_f32, )(
        global float *Pointer,
        int           Scope,
        int           Equal,
        int           Unequal,
        float         Value,
        float         Comparator);
float
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p3f32_i32_i32_i32_f32_f32, )(
        local float *Pointer,
        int          Scope,
        int          Equal,
        int          Unequal,
        float        Value,
        float        Comparator);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p4f32_i32_i32_i32_f32_f32, )(
        generic float *Pointer,
        int            Scope,
        int            Equal,
        int            Unequal,
        float          Value,
        float          Comparator);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    AtomicCompareExchangeWeak, _p0i32_i32_i32_i32_i32_i32, )(
    private int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    AtomicCompareExchangeWeak, _p1i32_i32_i32_i32_i32_i32, )(
    global int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p3i32_i32_i32_i32_i32_i32, )(
        local int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    AtomicCompareExchangeWeak, _p4i32_i32_i32_i32_i32_i32, )(
    generic int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p0i64_i32_i32_i32_i64_i64, )(
        private long *Pointer,
        int           Scope,
        int           Equal,
        int           Unequal,
        long          Value,
        long          Comparator);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    AtomicCompareExchangeWeak, _p1i64_i32_i32_i32_i64_i64, )(
    global long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    AtomicCompareExchangeWeak, _p3i64_i32_i32_i32_i64_i64, )(
    local long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p4i64_i32_i32_i32_i64_i64, )(
        generic long *Pointer,
        int           Scope,
        int           Equal,
        int           Unequal,
        long          Value,
        long          Comparator);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p0i32_i32_i32, )(
    private int *Pointer, int Scope, int Semantics);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p1i32_i32_i32, )(
    global int *Pointer, int Scope, int Semantics);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p3i32_i32_i32, )(
    local int *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p4i32_i32_i32, )(
    generic int *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p0i64_i32_i32, )(
    private long *Pointer, int Scope, int Semantics);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p1i64_i32_i32, )(
    global long *Pointer, int Scope, int Semantics);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p3i64_i32_i32, )(
    local long *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p4i64_i32_i32, )(
    generic long *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p0i32_i32_i32, )(
    private int *Pointer, int Scope, int Semantics);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p1i32_i32_i32, )(
    global int *Pointer, int Scope, int Semantics);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p3i32_i32_i32, )(
    local int *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p4i32_i32_i32, )(
    generic int *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p0i64_i32_i32, )(
    private long *Pointer, int Scope, int Semantics);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p1i64_i32_i32, )(
    global long *Pointer, int Scope, int Semantics);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p3i64_i32_i32, )(
    local long *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p4i64_i32_i32, )(
    generic long *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p0i32_i32_i32_i32, )(
    private int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p1i32_i32_i32_i32, )(
    global int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p3i32_i32_i32_i32, )(
    local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p4i32_i32_i32_i32, )(
    generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p0i64_i32_i32_i64, )(
    private long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p1i64_i32_i32_i64, )(
    global long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p3i64_i32_i32_i64, )(
    local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p4i64_i32_i32_i64, )(
    generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p0i32_i32_i32_i32, )(
    private int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p1i32_i32_i32_i32, )(
    global int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p3i32_i32_i32_i32, )(
    local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p4i32_i32_i32_i32, )(
    generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p0i64_i32_i32_i64, )(
    private long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p1i64_i32_i32_i64, )(
    global long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p3i64_i32_i32_i64, )(
    local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p4i64_i32_i32_i64, )(
    generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p0i32_i32_i32_i32, )(
    private int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p1i32_i32_i32_i32, )(
    global int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p3i32_i32_i32_i32, )(
    local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p4i32_i32_i32_i32, )(
    generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p0i64_i32_i32_i64, )(
    private long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p1i64_i32_i32_i64, )(
    global long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p3i64_i32_i32_i64, )(
    local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p4i64_i32_i32_i64, )(
    generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p0i32_i32_i32_i32, )(
    private uint *Pointer, int Scope, int Semantics, uint Value);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p1i32_i32_i32_i32, )(
    global uint *Pointer, int Scope, int Semantics, uint Value);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p3i32_i32_i32_i32, )(
    local uint *Pointer, int Scope, int Semantics, uint Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p4i32_i32_i32_i32, )(
    generic uint *Pointer, int Scope, int Semantics, uint Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p0i64_i32_i32_i64, )(
    private ulong *Pointer, int Scope, int Semantics, ulong Value);
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p1i64_i32_i32_i64, )(
    global ulong *Pointer, int Scope, int Semantics, ulong Value);
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p3i64_i32_i32_i64, )(
    local ulong *Pointer, int Scope, int Semantics, ulong Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p4i64_i32_i32_i64, )(
    generic ulong *Pointer, int Scope, int Semantics, ulong Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p0i32_i32_i32_i32, )(
    private int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p1i32_i32_i32_i32, )(
    global int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p3i32_i32_i32_i32, )(
    local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p4i32_i32_i32_i32, )(
    generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p0i64_i32_i32_i64, )(
    private long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p1i64_i32_i32_i64, )(
    global long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p3i64_i32_i32_i64, )(
    local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p4i64_i32_i32_i64, )(
    generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p0i32_i32_i32_i32, )(
    private uint *Pointer, int Scope, int Semantics, uint Value);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p1i32_i32_i32_i32, )(
    global uint *Pointer, int Scope, int Semantics, uint Value);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p3i32_i32_i32_i32, )(
    local uint *Pointer, int Scope, int Semantics, uint Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p4i32_i32_i32_i32, )(
    generic uint *Pointer, int Scope, int Semantics, uint Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p0i64_i32_i32_i64, )(
    private ulong *Pointer, int Scope, int Semantics, ulong Value);
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p1i64_i32_i32_i64, )(
    global ulong *Pointer, int Scope, int Semantics, ulong Value);
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p3i64_i32_i32_i64, )(
    local ulong *Pointer, int Scope, int Semantics, ulong Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p4i64_i32_i32_i64, )(
    generic ulong *Pointer, int Scope, int Semantics, ulong Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p0i32_i32_i32_i32, )(
    private int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p1i32_i32_i32_i32, )(
    global int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p3i32_i32_i32_i32, )(
    local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p4i32_i32_i32_i32, )(
    generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p0i64_i32_i32_i64, )(
    private long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p1i64_i32_i32_i64, )(
    global long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p3i64_i32_i32_i64, )(
    local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p4i64_i32_i32_i64, )(
    generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p0i32_i32_i32_i32, )(
    private int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p1i32_i32_i32_i32, )(
    global int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p3i32_i32_i32_i32, )(
    local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p4i32_i32_i32_i32, )(
    generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p0i64_i32_i32_i64, )(
    private long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p1i64_i32_i32_i64, )(
    global long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p3i64_i32_i32_i64, )(
    local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p4i64_i32_i32_i64, )(
    generic long *Pointer, int Scope, int Semantics, long Value);
#endif                   // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif                   // defined(cl_khr_int64_extended_atomics)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p0i32_i32_i32_i32, )(
    private int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p1i32_i32_i32_i32, )(
    global int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p3i32_i32_i32_i32, )(
    local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p4i32_i32_i32_i32, )(
    generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p0i64_i32_i32_i64, )(
    private long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p1i64_i32_i32_i64, )(
    global long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p3i64_i32_i32_i64, )(
    local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p4i64_i32_i32_i64, )(
    generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagTestAndSet, _p0i32_i32_i32, )(
    private int *Pointer, int Scope, int Semantics);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagTestAndSet, _p1i32_i32_i32, )(
    global int *Pointer, int Scope, int Semantics);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagTestAndSet, _p3i32_i32_i32, )(
    local int *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagTestAndSet, _p4i32_i32_i32, )(
    generic int *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagClear, _p0i32_i32_i32, )(
    private int *Pointer, int Scope, int Semantics);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagClear, _p1i32_i32_i32, )(
    global int *Pointer, int Scope, int Semantics);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagClear, _p3i32_i32_i32, )(
    local int *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagClear, _p4i32_i32_i32, )(
    generic int *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAdd, _p1f32_i32_i32_f32, )(
    global float *Pointer, int Scope, int Semantics, float Value);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFSub, _p1f32_i32_i32_f32, )(
    global float *Pointer, int Scope, int Semantics, float Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

// SPV_EXT_shader_atomic_float_add
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p0f32_i32_i32_f32, )(
    private float *Pointer, int Scope, int Semantics, float Value);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p1f32_i32_i32_f32, )(
    global float *Pointer, int Scope, int Semantics, float Value);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p3f32_i32_i32_f32, )(
    local float *Pointer, int Scope, int Semantics, float Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p4f32_i32_i32_f32, )(
    generic float *Pointer, int Scope, int Semantics, float Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p0f64_i32_i32_f64, )(
    private double *Pointer, int Scope, int Semantics, double Value);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p1f64_i32_i32_f64, )(
    global double *Pointer, int Scope, int Semantics, double Value);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p3f64_i32_i32_f64, )(
    local double *Pointer, int Scope, int Semantics, double Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p4f64_i32_i32_f64, )(
    generic double *Pointer, int Scope, int Semantics, double Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

// SPV_EXT_shader_atomic_float_min_max
#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p0f16_i32_i32_f16, )(
    private half *Pointer, int Scope, int Semantics, half Value);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p1f16_i32_i32_f16, )(
    global half *Pointer, int Scope, int Semantics, half Value);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p3f16_i32_i32_f16, )(
    local half *Pointer, int Scope, int Semantics, half Value);
#endif // defined(cl_khr_fp16)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p0f32_i32_i32_f32, )(
    private float *Pointer, int Scope, int Semantics, float Value);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p1f32_i32_i32_f32, )(
    global float *Pointer, int Scope, int Semantics, float Value);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p3f32_i32_i32_f32, )(
    local float *Pointer, int Scope, int Semantics, float Value);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p0f64_i32_i32_f64, )(
    private double *Pointer, int Scope, int Semantics, double Value);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p1f64_i32_i32_f64, )(
    global double *Pointer, int Scope, int Semantics, double Value);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p3f64_i32_i32_f64, )(
    local double *Pointer, int Scope, int Semantics, double Value);
#endif // defined(cl_khr_fp64)
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p4f16_i32_i32_f16, )(
    generic half *Pointer, int Scope, int Semantics, half Value);
#endif // defined(cl_khr_fp16)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p4f32_i32_i32_f32, )(
    generic float *Pointer, int Scope, int Semantics, float Value);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p4f64_i32_i32_f64, )(
    generic double *Pointer, int Scope, int Semantics, double Value);
#endif // defined(cl_khr_fp64)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p0f16_i32_i32_f16, )(
    private half *Pointer, int Scope, int Semantics, half Value);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p1f16_i32_i32_f16, )(
    global half *Pointer, int Scope, int Semantics, half Value);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p3f16_i32_i32_f16, )(
    local half *Pointer, int Scope, int Semantics, half Value);
#endif // defined(cl_khr_fp16)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p0f32_i32_i32_f32, )(
    private float *Pointer, int Scope, int Semantics, float Value);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p1f32_i32_i32_f32, )(
    global float *Pointer, int Scope, int Semantics, float Value);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p3f32_i32_i32_f32, )(
    local float *Pointer, int Scope, int Semantics, float Value);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p0f64_i32_i32_f64, )(
    private double *Pointer, int Scope, int Semantics, double Value);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p1f64_i32_i32_f64, )(
    global double *Pointer, int Scope, int Semantics, double Value);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p3f64_i32_i32_f64, )(
    local double *Pointer, int Scope, int Semantics, double Value);
#endif // defined(cl_khr_fp64)
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p4f16_i32_i32_f16, )(
    generic half *Pointer, int Scope, int Semantics, half Value);
#endif // defined(cl_khr_fp16)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p4f32_i32_i32_f32, )(
    generic float *Pointer, int Scope, int Semantics, float Value);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p4f64_i32_i32_f64, )(
    generic double *Pointer, int Scope, int Semantics, double Value);
#endif                   // defined(cl_khr_fp64)
#endif                   // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

// Barrier Instructions

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(
    int Execution, int Memory, int Semantics);
void SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(MemoryBarrier, _i32_i32, )(int Memory, int Semantics);

#ifndef NAMED_BARRIER_STRUCT_TYPE
#define NAMED_BARRIER_STRUCT_TYPE
typedef struct
{
    int count;
    int orig_count;
    int inc;
} __namedBarrier;
#endif

local __namedBarrier *__builtin_spirv_OpNamedBarrierInitialize_i32_p3__namedBarrier_p3i32(
    int Count, local __namedBarrier *nb_array, local uint *id);
void __builtin_spirv_OpMemoryNamedBarrier_p3__namedBarrier_i32_i32(
    local __namedBarrier *NB, Scope_t Memory, uint Semantics);
void __builtin_spirv_OpMemoryNamedBarrierWrapperOCL_p3__namedBarrier_i32(
    local __namedBarrier *barrier, cl_mem_fence_flags flags);
void __builtin_spirv_OpMemoryNamedBarrierWrapperOCL_p3__namedBarrier_i32_i32(
    local __namedBarrier *barrier, cl_mem_fence_flags flags, memory_scope scope);
// Group Instructions
// TODO: Do we want to split out size_t into i64 and i32 as we've done here?

__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i8_p3i8_i64_i64_i64, )(
        int           Execution,
        global char  *Destination,
        local char   *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i8_p3i8_i32_i32_i64, )(
        int           Execution,
        global char  *Destination,
        local char   *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i16_p3i16_i64_i64_i64, )(
        int           Execution,
        global short *Destination,
        local short  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i16_p3i16_i32_i32_i64, )(
        int           Execution,
        global short *Destination,
        local short  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i32_p3i32_i64_i64_i64, )(
        int           Execution,
        global int   *Destination,
        local int    *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i32_p3i32_i32_i32_i64, )(
        int           Execution,
        global int   *Destination,
        local int    *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i64_p3i64_i64_i64_i64, )(
        int           Execution,
        global long  *Destination,
        local long   *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i64_p3i64_i32_i32_i64, )(
        int           Execution,
        global long  *Destination,
        local long   *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1f16_p3f16_i64_i64_i64, )(
        int           Execution,
        global half  *Destination,
        local half   *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1f16_p3f16_i32_i32_i64, )(
        int           Execution,
        global half  *Destination,
        local half   *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1f32_p3f32_i64_i64_i64, )(
        int           Execution,
        global float *Destination,
        local float  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1f32_p3f32_i32_i32_i64, )(
        int           Execution,
        global float *Destination,
        local float  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i8_p3v2i8_i64_i64_i64, )(
        int           Execution,
        global char2 *Destination,
        local char2  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i8_p3v2i8_i32_i32_i64, )(
        int           Execution,
        global char2 *Destination,
        local char2  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i8_p3v3i8_i64_i64_i64, )(
        int           Execution,
        global char3 *Destination,
        local char3  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i8_p3v3i8_i32_i32_i64, )(
        int           Execution,
        global char3 *Destination,
        local char3  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i8_p3v4i8_i64_i64_i64, )(
        int           Execution,
        global char4 *Destination,
        local char4  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i8_p3v4i8_i32_i32_i64, )(
        int           Execution,
        global char4 *Destination,
        local char4  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i8_p3v8i8_i64_i64_i64, )(
        int           Execution,
        global char8 *Destination,
        local char8  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i8_p3v8i8_i32_i32_i64, )(
        int           Execution,
        global char8 *Destination,
        local char8  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i8_p3v16i8_i64_i64_i64, )(
        int            Execution,
        global char16 *Destination,
        local char16  *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i8_p3v16i8_i32_i32_i64, )(
        int            Execution,
        global char16 *Destination,
        local char16  *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i16_p3v2i16_i64_i64_i64, )(
        int            Execution,
        global short2 *Destination,
        local short2  *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i16_p3v2i16_i32_i32_i64, )(
        int            Execution,
        global short2 *Destination,
        local short2  *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i16_p3v3i16_i64_i64_i64, )(
        int            Execution,
        global short3 *Destination,
        local short3  *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i16_p3v3i16_i32_i32_i64, )(
        int            Execution,
        global short3 *Destination,
        local short3  *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i16_p3v4i16_i64_i64_i64, )(
        int            Execution,
        global short4 *Destination,
        local short4  *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i16_p3v4i16_i32_i32_i64, )(
        int            Execution,
        global short4 *Destination,
        local short4  *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i16_p3v8i16_i64_i64_i64, )(
        int            Execution,
        global short8 *Destination,
        local short8  *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i16_p3v8i16_i32_i32_i64, )(
        int            Execution,
        global short8 *Destination,
        local short8  *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i16_p3v16i16_i64_i64_i64, )(
        int             Execution,
        global short16 *Destination,
        local short16  *Source,
        long            NumElements,
        long            Stride,
        __spirv_Event   Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i16_p3v16i16_i32_i32_i64, )(
        int             Execution,
        global short16 *Destination,
        local short16  *Source,
        int             NumElements,
        int             Stride,
        __spirv_Event   Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i32_p3v2i32_i64_i64_i64, )(
        int           Execution,
        global int2  *Destination,
        local int2   *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i32_p3v2i32_i32_i32_i64, )(
        int           Execution,
        global int2  *Destination,
        local int2   *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i32_p3v3i32_i64_i64_i64, )(
        int           Execution,
        global int3  *Destination,
        local int3   *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i32_p3v3i32_i32_i32_i64, )(
        int           Execution,
        global int3  *Destination,
        local int3   *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i32_p3v4i32_i64_i64_i64, )(
        int           Execution,
        global int4  *Destination,
        local int4   *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i32_p3v4i32_i32_i32_i64, )(
        int           Execution,
        global int4  *Destination,
        local int4   *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i32_p3v8i32_i64_i64_i64, )(
        int           Execution,
        global int8  *Destination,
        local int8   *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i32_p3v8i32_i32_i32_i64, )(
        int           Execution,
        global int8  *Destination,
        local int8   *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i32_p3v16i32_i64_i64_i64, )(
        int           Execution,
        global int16 *Destination,
        local int16  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i32_p3v16i32_i32_i32_i64, )(
        int           Execution,
        global int16 *Destination,
        local int16  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i64_p3v2i64_i64_i64_i64, )(
        int           Execution,
        global long2 *Destination,
        local long2  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i64_p3v2i64_i32_i32_i64, )(
        int           Execution,
        global long2 *Destination,
        local long2  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i64_p3v3i64_i64_i64_i64, )(
        int           Execution,
        global long3 *Destination,
        local long3  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i64_p3v3i64_i32_i32_i64, )(
        int           Execution,
        global long3 *Destination,
        local long3  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i64_p3v4i64_i64_i64_i64, )(
        int           Execution,
        global long4 *Destination,
        local long4  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i64_p3v4i64_i32_i32_i64, )(
        int           Execution,
        global long4 *Destination,
        local long4  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i64_p3v8i64_i64_i64_i64, )(
        int           Execution,
        global long8 *Destination,
        local long8  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i64_p3v8i64_i32_i32_i64, )(
        int           Execution,
        global long8 *Destination,
        local long8  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i64_p3v16i64_i64_i64_i64, )(
        int            Execution,
        global long16 *Destination,
        local long16  *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i64_p3v16i64_i32_i32_i64, )(
        int            Execution,
        global long16 *Destination,
        local long16  *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2f16_p3v2f16_i64_i64_i64, )(
        int           Execution,
        global half2 *Destination,
        local half2  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2f16_p3v2f16_i32_i32_i64, )(
        int           Execution,
        global half2 *Destination,
        local half2  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3f16_p3v3f16_i64_i64_i64, )(
        int           Execution,
        global half3 *Destination,
        local half3  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3f16_p3v3f16_i32_i32_i64, )(
        int           Execution,
        global half3 *Destination,
        local half3  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4f16_p3v4f16_i64_i64_i64, )(
        int           Execution,
        global half4 *Destination,
        local half4  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4f16_p3v4f16_i32_i32_i64, )(
        int           Execution,
        global half4 *Destination,
        local half4  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8f16_p3v8f16_i64_i64_i64, )(
        int           Execution,
        global half8 *Destination,
        local half8  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8f16_p3v8f16_i32_i32_i64, )(
        int           Execution,
        global half8 *Destination,
        local half8  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16f16_p3v16f16_i64_i64_i64, )(
        int            Execution,
        global half16 *Destination,
        local half16  *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16f16_p3v16f16_i32_i32_i64, )(
        int            Execution,
        global half16 *Destination,
        local half16  *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2f32_p3v2f32_i64_i64_i64, )(
        int            Execution,
        global float2 *Destination,
        local float2  *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2f32_p3v2f32_i32_i32_i64, )(
        int            Execution,
        global float2 *Destination,
        local float2  *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3f32_p3v3f32_i64_i64_i64, )(
        int            Execution,
        global float3 *Destination,
        local float3  *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3f32_p3v3f32_i32_i32_i64, )(
        int            Execution,
        global float3 *Destination,
        local float3  *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4f32_p3v4f32_i64_i64_i64, )(
        int            Execution,
        global float4 *Destination,
        local float4  *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4f32_p3v4f32_i32_i32_i64, )(
        int            Execution,
        global float4 *Destination,
        local float4  *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8f32_p3v8f32_i64_i64_i64, )(
        int            Execution,
        global float8 *Destination,
        local float8  *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8f32_p3v8f32_i32_i32_i64, )(
        int            Execution,
        global float8 *Destination,
        local float8  *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16f32_p3v16f32_i64_i64_i64, )(
        int             Execution,
        global float16 *Destination,
        local float16  *Source,
        long            NumElements,
        long            Stride,
        __spirv_Event   Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16f32_p3v16f32_i32_i32_i64, )(
        int             Execution,
        global float16 *Destination,
        local float16  *Source,
        int             NumElements,
        int             Stride,
        __spirv_Event   Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i8_p1i8_i64_i64_i64, )(
        int           Execution,
        local char   *Destination,
        global char  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i8_p1i8_i32_i32_i64, )(
        int           Execution,
        local char   *Destination,
        global char  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i16_p1i16_i64_i64_i64, )(
        int           Execution,
        local short  *Destination,
        global short *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i16_p1i16_i32_i32_i64, )(
        int           Execution,
        local short  *Destination,
        global short *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i32_p1i32_i64_i64_i64, )(
        int           Execution,
        local int    *Destination,
        global int   *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i32_p1i32_i32_i32_i64, )(
        int           Execution,
        local int    *Destination,
        global int   *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i64_p1i64_i64_i64_i64, )(
        int           Execution,
        local long   *Destination,
        global long  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i64_p1i64_i32_i32_i64, )(
        int           Execution,
        local long   *Destination,
        global long  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3f16_p1f16_i64_i64_i64, )(
        int           Execution,
        local half   *Destination,
        global half  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3f16_p1f16_i32_i32_i64, )(
        int           Execution,
        local half   *Destination,
        global half  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3f32_p1f32_i64_i64_i64, )(
        int           Execution,
        local float  *Destination,
        global float *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i8_p1v2i8_i64_i64_i64, )(
        int           Execution,
        local char2  *Destination,
        global char2 *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i8_p1v2i8_i32_i32_i64, )(
        int           Execution,
        local char2  *Destination,
        global char2 *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i8_p1v3i8_i64_i64_i64, )(
        int           Execution,
        local char3  *Destination,
        global char3 *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i8_p1v3i8_i32_i32_i64, )(
        int           Execution,
        local char3  *Destination,
        global char3 *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i8_p1v4i8_i64_i64_i64, )(
        int           Execution,
        local char4  *Destination,
        global char4 *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i8_p1v4i8_i32_i32_i64, )(
        int           Execution,
        local char4  *Destination,
        global char4 *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i8_p1v8i8_i64_i64_i64, )(
        int           Execution,
        local char8  *Destination,
        global char8 *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i8_p1v8i8_i32_i32_i64, )(
        int           Execution,
        local char8  *Destination,
        global char8 *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i8_p1v16i8_i64_i64_i64, )(
        int            Execution,
        local char16  *Destination,
        global char16 *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i8_p1v16i8_i32_i32_i64, )(
        int            Execution,
        local char16  *Destination,
        global char16 *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i16_p1v2i16_i64_i64_i64, )(
        int            Execution,
        local short2  *Destination,
        global short2 *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i16_p1v2i16_i32_i32_i64, )(
        int            Execution,
        local short2  *Destination,
        global short2 *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i16_p1v3i16_i64_i64_i64, )(
        int            Execution,
        local short3  *Destination,
        global short3 *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i16_p1v3i16_i32_i32_i64, )(
        int            Execution,
        local short3  *Destination,
        global short3 *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i16_p1v4i16_i64_i64_i64, )(
        int            Execution,
        local short4  *Destination,
        global short4 *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i16_p1v4i16_i32_i32_i64, )(
        int            Execution,
        local short4  *Destination,
        global short4 *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i16_p1v8i16_i64_i64_i64, )(
        int            Execution,
        local short8  *Destination,
        global short8 *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i16_p1v8i16_i32_i32_i64, )(
        int            Execution,
        local short8  *Destination,
        global short8 *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i16_p1v16i16_i64_i64_i64, )(
        int             Execution,
        local short16  *Destination,
        global short16 *Source,
        long            NumElements,
        long            Stride,
        __spirv_Event   Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i16_p1v16i16_i32_i32_i64, )(
        int             Execution,
        local short16  *Destination,
        global short16 *Source,
        int             NumElements,
        int             Stride,
        __spirv_Event   Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i32_p1v2i32_i64_i64_i64, )(
        int           Execution,
        local int2   *Destination,
        global int2  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i32_p1v2i32_i32_i32_i64, )(
        int           Execution,
        local int2   *Destination,
        global int2  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i32_p1v3i32_i64_i64_i64, )(
        int           Execution,
        local int3   *Destination,
        global int3  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i32_p1v3i32_i32_i32_i64, )(
        int           Execution,
        local int3   *Destination,
        global int3  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i32_p1v4i32_i64_i64_i64, )(
        int           Execution,
        local int4   *Destination,
        global int4  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i32_p1v4i32_i32_i32_i64, )(
        int           Execution,
        local int4   *Destination,
        global int4  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i32_p1v8i32_i64_i64_i64, )(
        int           Execution,
        local int8   *Destination,
        global int8  *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i32_p1v8i32_i32_i32_i64, )(
        int           Execution,
        local int8   *Destination,
        global int8  *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i32_p1v16i32_i64_i64_i64, )(
        int           Execution,
        local int16  *Destination,
        global int16 *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i32_p1v16i32_i32_i32_i64, )(
        int           Execution,
        local int16  *Destination,
        global int16 *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i64_p1v2i64_i64_i64_i64, )(
        int           Execution,
        local long2  *Destination,
        global long2 *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i64_p1v2i64_i32_i32_i64, )(
        int           Execution,
        local long2  *Destination,
        global long2 *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i64_p1v3i64_i64_i64_i64, )(
        int           Execution,
        local long3  *Destination,
        global long3 *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i64_p1v3i64_i32_i32_i64, )(
        int           Execution,
        local long3  *Destination,
        global long3 *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i64_p1v4i64_i64_i64_i64, )(
        int           Execution,
        local long4  *Destination,
        global long4 *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i64_p1v4i64_i32_i32_i64, )(
        int           Execution,
        local long4  *Destination,
        global long4 *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i64_p1v8i64_i64_i64_i64, )(
        int           Execution,
        local long8  *Destination,
        global long8 *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i64_p1v8i64_i32_i32_i64, )(
        int           Execution,
        local long8  *Destination,
        global long8 *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i64_p1v16i64_i64_i64_i64, )(
        int            Execution,
        local long16  *Destination,
        global long16 *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i64_p1v16i64_i32_i32_i64, )(
        int            Execution,
        local long16  *Destination,
        global long16 *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2f16_p1v2f16_i64_i64_i64, )(
        int           Execution,
        local half2  *Destination,
        global half2 *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2f16_p1v2f16_i32_i32_i64, )(
        int           Execution,
        local half2  *Destination,
        global half2 *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3f16_p1v3f16_i64_i64_i64, )(
        int           Execution,
        local half3  *Destination,
        global half3 *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3f16_p1v3f16_i32_i32_i64, )(
        int           Execution,
        local half3  *Destination,
        global half3 *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4f16_p1v4f16_i64_i64_i64, )(
        int           Execution,
        local half4  *Destination,
        global half4 *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4f16_p1v4f16_i32_i32_i64, )(
        int           Execution,
        local half4  *Destination,
        global half4 *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8f16_p1v8f16_i64_i64_i64, )(
        int           Execution,
        local half8  *Destination,
        global half8 *Source,
        long          NumElements,
        long          Stride,
        __spirv_Event Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8f16_p1v8f16_i32_i32_i64, )(
        int           Execution,
        local half8  *Destination,
        global half8 *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16f16_p1v16f16_i64_i64_i64, )(
        int            Execution,
        local half16  *Destination,
        global half16 *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16f16_p1v16f16_i32_i32_i64, )(
        int            Execution,
        local half16  *Destination,
        global half16 *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2f32_p1v2f32_i64_i64_i64, )(
        int            Execution,
        local float2  *Destination,
        global float2 *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2f32_p1v2f32_i32_i32_i64, )(
        int            Execution,
        local float2  *Destination,
        global float2 *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3f32_p1v3f32_i64_i64_i64, )(
        int            Execution,
        local float3  *Destination,
        global float3 *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3f32_p1v3f32_i32_i32_i64, )(
        int            Execution,
        local float3  *Destination,
        global float3 *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4f32_p1v4f32_i64_i64_i64, )(
        int            Execution,
        local float4  *Destination,
        global float4 *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4f32_p1v4f32_i32_i32_i64, )(
        int            Execution,
        local float4  *Destination,
        global float4 *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8f32_p1v8f32_i64_i64_i64, )(
        int            Execution,
        local float8  *Destination,
        global float8 *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8f32_p1v8f32_i32_i32_i64, )(
        int            Execution,
        local float8  *Destination,
        global float8 *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16f32_p1v16f32_i64_i64_i64, )(
        int             Execution,
        local float16  *Destination,
        global float16 *Source,
        long            NumElements,
        long            Stride,
        __spirv_Event   Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16f32_p1v16f32_i32_i32_i64, )(
        int             Execution,
        local float16  *Destination,
        global float16 *Source,
        int             NumElements,
        int             Stride,
        __spirv_Event   Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3f32_p1f32_i32_i32_i64, )(
        int           Execution,
        local float  *Destination,
        global float *Source,
        int           NumElements,
        int           Stride,
        __spirv_Event Event);
#if defined(cl_khr_fp64)
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1f64_p3f64_i64_i64_i64, )(
        int            Execution,
        global double *Destination,
        local double  *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1f64_p3f64_i32_i32_i64, )(
        int            Execution,
        global double *Destination,
        local double  *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2f64_p3v2f64_i64_i64_i64, )(
        int             Execution,
        global double2 *Destination,
        local double2  *Source,
        long            NumElements,
        long            Stride,
        __spirv_Event   Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2f64_p3v2f64_i32_i32_i64, )(
        int             Execution,
        global double2 *Destination,
        local double2  *Source,
        int             NumElements,
        int             Stride,
        __spirv_Event   Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3f64_p3v3f64_i64_i64_i64, )(
        int             Execution,
        global double3 *Destination,
        local double3  *Source,
        long            NumElements,
        long            Stride,
        __spirv_Event   Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3f64_p3v3f64_i32_i32_i64, )(
        int             Execution,
        global double3 *Destination,
        local double3  *Source,
        int             NumElements,
        int             Stride,
        __spirv_Event   Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4f64_p3v4f64_i64_i64_i64, )(
        int             Execution,
        global double4 *Destination,
        local double4  *Source,
        long            NumElements,
        long            Stride,
        __spirv_Event   Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4f64_p3v4f64_i32_i32_i64, )(
        int             Execution,
        global double4 *Destination,
        local double4  *Source,
        int             NumElements,
        int             Stride,
        __spirv_Event   Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8f64_p3v8f64_i64_i64_i64, )(
        int             Execution,
        global double8 *Destination,
        local double8  *Source,
        long            NumElements,
        long            Stride,
        __spirv_Event   Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8f64_p3v8f64_i32_i32_i64, )(
        int             Execution,
        global double8 *Destination,
        local double8  *Source,
        int             NumElements,
        int             Stride,
        __spirv_Event   Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16f64_p3v16f64_i64_i64_i64, )(
        int              Execution,
        global double16 *Destination,
        local double16  *Source,
        long             NumElements,
        long             Stride,
        __spirv_Event    Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16f64_p3v16f64_i32_i32_i64, )(
        int              Execution,
        global double16 *Destination,
        local double16  *Source,
        int              NumElements,
        int              Stride,
        __spirv_Event    Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3f64_p1f64_i64_i64_i64, )(
        int            Execution,
        local double  *Destination,
        global double *Source,
        long           NumElements,
        long           Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3f64_p1f64_i32_i32_i64, )(
        int            Execution,
        local double  *Destination,
        global double *Source,
        int            NumElements,
        int            Stride,
        __spirv_Event  Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2f64_p1v2f64_i64_i64_i64, )(
        int             Execution,
        local double2  *Destination,
        global double2 *Source,
        long            NumElements,
        long            Stride,
        __spirv_Event   Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2f64_p1v2f64_i32_i32_i64, )(
        int             Execution,
        local double2  *Destination,
        global double2 *Source,
        int             NumElements,
        int             Stride,
        __spirv_Event   Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3f64_p1v3f64_i64_i64_i64, )(
        int             Execution,
        local double3  *Destination,
        global double3 *Source,
        long            NumElements,
        long            Stride,
        __spirv_Event   Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3f64_p1v3f64_i32_i32_i64, )(
        int             Execution,
        local double3  *Destination,
        global double3 *Source,
        int             NumElements,
        int             Stride,
        __spirv_Event   Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4f64_p1v4f64_i64_i64_i64, )(
        int             Execution,
        local double4  *Destination,
        global double4 *Source,
        long            NumElements,
        long            Stride,
        __spirv_Event   Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4f64_p1v4f64_i32_i32_i64, )(
        int             Execution,
        local double4  *Destination,
        global double4 *Source,
        int             NumElements,
        int             Stride,
        __spirv_Event   Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8f64_p1v8f64_i64_i64_i64, )(
        int             Execution,
        local double8  *Destination,
        global double8 *Source,
        long            NumElements,
        long            Stride,
        __spirv_Event   Event);
__spirv_Event
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8f64_p1v8f64_i32_i32_i64, )(
        int             Execution,
        local double8  *Destination,
        global double8 *Source,
        int             NumElements,
        int             Stride,
        __spirv_Event   Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16f64_p1v16f64_i64_i64_i64, )(
        int              Execution,
        local double16  *Destination,
        global double16 *Source,
        long             NumElements,
        long             Stride,
        __spirv_Event    Event);
__spirv_Event SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16f64_p1v16f64_i32_i32_i64, )(
        int              Execution,
        local double16  *Destination,
        global double16 *Source,
        int              NumElements,
        int              Stride,
        __spirv_Event    Event);
#endif // defined(cl_khr_fp64)

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupWaitEvents, _i32_i32_p0i64, )(
    int Execution, int NumEvents, private __spirv_Event *EventsList);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupWaitEvents, _i32_i32_p4i64, )(
    int Execution, int NumEvents, generic __spirv_Event *EventsList);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#if defined(cl_khr_subgroup_non_uniform_vote)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformElect, _i32, )(int Execution);
bool SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupNonUniformAll, _i32_i1, )(int Execution, bool Predicate);
bool SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupNonUniformAny, _i32_i1, )(int Execution, bool Predicate);
bool SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupNonUniformAllEqual, _i32_i8, )(int Execution, char Value);
bool SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupNonUniformAllEqual, _i32_i16, )(int Execution, short Value);
bool SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupNonUniformAllEqual, _i32_i32, )(int Execution, int Value);
bool SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupNonUniformAllEqual, _i32_i64, )(int Execution, long Value);
bool SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupNonUniformAllEqual, _i32_f32, )(int Execution, float Value);
#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupNonUniformAllEqual, _i32_f64, )(int Execution, double Value);
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
bool SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupNonUniformAllEqual, _i32_f16, )(int Execution, half Value);
#endif // defined(cl_khr_fp16)
#endif // defined(cl_khr_subgroup_non_uniform_vote)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAll, _i32_i1, )(int Execution, bool Predicate);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAny, _i32_i1, )(int Execution, bool Predicate);
#define DECL_SUB_GROUP_BROADCAST_BASE(TYPE, TYPE_ABBR)                                 \
    TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_##TYPE_ABBR##_v3i32, )( \
        int Execution, TYPE Value, int3 LocalId);                                      \
    TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_##TYPE_ABBR##_v3i64, )( \
        int Execution, TYPE Value, long3 LocalId);                                     \
    TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_##TYPE_ABBR##_v2i32, )( \
        int Execution, TYPE Value, int2 LocalId);                                      \
    TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_##TYPE_ABBR##_v2i64, )( \
        int Execution, TYPE Value, long2 LocalId);                                     \
    TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_##TYPE_ABBR##_i32, )(   \
        int Execution, TYPE Value, int LocalId);                                       \
    TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_##TYPE_ABBR##_i64, )(   \
        int Execution, TYPE Value, long LocalId);

char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SubgroupShuffleINTEL, _i8_i32, )(char Data, uint InvocationId);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SubgroupShuffleINTEL, _i16_i32, )(short Data, uint InvocationId);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SubgroupShuffleINTEL, _i32_i32, )(int Data, uint InvocationId);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SubgroupShuffleINTEL, _i64_i32, )(long Data, uint InvocationId);
#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SubgroupShuffleINTEL, _f16_i32, )(half Data, uint InvocationId);
#endif // defined(cl_khr_fp16)
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SubgroupShuffleINTEL, _f32_i32, )(float Data, uint InvocationId);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SubgroupShuffleINTEL, _f64_i32, )(double Data, uint InvocationId);
#endif // defined(cl_khr_fp64)

char SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _i8_i8_i32, )(
    char Current, char Next, uint Delta);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _i16_i16_i32, )(
    short Current, short Next, uint Delta);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _i32_i32_i32, )(
    int Current, int Next, uint Delta);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _i64_i64_i32, )(
    long Current, long Next, uint Delta);
#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _f16_f16_i32, )(
    half Current, half Next, uint Delta);
#endif // defined(cl_khr_fp16)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _f32_f32_i32, )(
    float Current, float Next, uint Delta);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _f64_f64_i32, )(
    double Current, double Next, uint Delta);
#endif // defined(cl_khr_fp64)

char SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleUpINTEL, _i8_i8_i32, )(
    char Previous, char Current, uint Delta);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleUpINTEL, _i16_i16_i32, )(
    short Previous, short Current, uint Delta);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleUpINTEL, _i32_i32_i32, )(
    int Previous, int Current, uint Delta);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleUpINTEL, _i64_i64_i32, )(
    long Previous, long Current, uint Delta);
#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleUpINTEL, _f16_f16_i32, )(
    half Previous, half Current, uint Delta);
#endif // defined(cl_khr_fp16)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleUpINTEL, _f32_f32_i32, )(
    float Previous, float Current, uint Delta);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleUpINTEL, _f64_f64_i32, )(
    double Previous, double Current, uint Delta);
#endif // defined(cl_khr_fp64)

char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SubgroupShuffleXorINTEL, _i8_i32, )(char Data, uint Value);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SubgroupShuffleXorINTEL, _i16_i32, )(short Data, uint Value);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SubgroupShuffleXorINTEL, _i32_i32, )(int Data, uint Value);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SubgroupShuffleXorINTEL, _i64_i32, )(long Data, uint Value);
#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SubgroupShuffleXorINTEL, _f16_i32, )(half Data, uint Value);
#endif // defined(cl_khr_fp16)
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SubgroupShuffleXorINTEL, _f32_i32, )(float Data, uint Value);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SubgroupShuffleXorINTEL, _f64_i32, )(double Data, uint Value);
#endif // defined(cl_khr_fp64)

#ifdef cl_intel_subgroups_char
char SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SubgroupImageBlockReadINTEL,
    _i8_img2d_ro_v2i32,
    _Rchar)(global Img2d_ro *image, int2 coord);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SubgroupImageBlockReadINTEL,
    _v2i8_img2d_ro_v2i32,
    _Rchar2)(global Img2d_ro *image, int2 coord);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SubgroupImageBlockReadINTEL,
    _v4i8_img2d_ro_v2i32,
    _Rchar4)(global Img2d_ro *image, int2 coord);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SubgroupImageBlockReadINTEL,
    _v8i8_img2d_ro_v2i32,
    _Rchar8)(global Img2d_ro *image, int2 coord);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SubgroupImageBlockReadINTEL,
    _v16i8_img2d_ro_v2i32,
    _Rchar16)(global Img2d_ro *image, int2 coord);
#endif // cl_intel_subgroups_char

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SubgroupImageBlockReadINTEL,
    _i16_img2d_ro_v2i32,
    _Rshort)(global Img2d_ro *image, int2 coord);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SubgroupImageBlockReadINTEL,
    _v2i16_img2d_ro_v2i32,
    _Rshort2)(global Img2d_ro *image, int2 coord);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SubgroupImageBlockReadINTEL,
    _v4i16_img2d_ro_v2i32,
    _Rshort4)(global Img2d_ro *image, int2 coord);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SubgroupImageBlockReadINTEL,
    _v8i16_img2d_ro_v2i32,
    _Rshort8)(global Img2d_ro *image, int2 coord);

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SubgroupImageBlockReadINTEL,
    _i32_img2d_ro_v2i32,
    _Rint)(global Img2d_ro *image, int2 coord);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SubgroupImageBlockReadINTEL,
    _v2i32_img2d_ro_v2i32,
    _Rint2)(global Img2d_ro *image, int2 coord);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SubgroupImageBlockReadINTEL,
    _v4i32_img2d_ro_v2i32,
    _Rint4)(global Img2d_ro *image, int2 coord);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SubgroupImageBlockReadINTEL,
    _v8i32_img2d_ro_v2i32,
    _Rint8)(global Img2d_ro *image, int2 coord);

#ifdef cl_intel_subgroups_long
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SubgroupImageBlockReadINTEL,
    _i64_img2d_ro_v2i32,
    _Rlong)(global Img2d_ro *image, int2 coord);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SubgroupImageBlockReadINTEL,
    _v2i64_img2d_ro_v2i32,
    _Rlong2)(global Img2d_ro *image, int2 coord);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SubgroupImageBlockReadINTEL,
    _v4i64_img2d_ro_v2i32,
    _Rlong4)(global Img2d_ro *image, int2 coord);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(
    SubgroupImageBlockReadINTEL,
    _v8i64_img2d_ro_v2i32,
    _Rlong8)(global Img2d_ro *image, int2 coord);
#endif // cl_intel_subgroups_long

#ifdef cl_intel_subgroup_buffer_prefetch
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupBlockPrefetchINTEL, _p1i8, )(
    const global uchar *ptr, uint num_bytes);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupBlockPrefetchINTEL, _p1i16, )(
    const global ushort *ptr, uint num_bytes);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupBlockPrefetchINTEL, _p1i32, )(
    const global uint *ptr, uint num_bytes);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupBlockPrefetchINTEL, _p1i64, )(
    const global ulong *ptr, uint num_bytes);
#endif // cl_intel_subgroup_buffer_prefetch

#define DECL_SUB_GROUP_BROADCAST(TYPE, TYPE_ABBR)         \
    DECL_SUB_GROUP_BROADCAST_BASE(TYPE, TYPE_ABBR)        \
    DECL_SUB_GROUP_BROADCAST_BASE(TYPE##2, v2##TYPE_ABBR) \
    DECL_SUB_GROUP_BROADCAST_BASE(TYPE##3, v3##TYPE_ABBR) \
    DECL_SUB_GROUP_BROADCAST_BASE(TYPE##4, v4##TYPE_ABBR) \
    DECL_SUB_GROUP_BROADCAST_BASE(TYPE##8, v8##TYPE_ABBR) \
    DECL_SUB_GROUP_BROADCAST_BASE(TYPE##16, v16##TYPE_ABBR)

DECL_SUB_GROUP_BROADCAST(char, i8)
DECL_SUB_GROUP_BROADCAST(short, i16)
DECL_SUB_GROUP_BROADCAST(int, i32)
DECL_SUB_GROUP_BROADCAST(long, i64)
DECL_SUB_GROUP_BROADCAST(float, f32)
#if defined(cl_khr_fp16)
DECL_SUB_GROUP_BROADCAST(half, f16)
#endif // defined(cl_khr_fp16)
#if defined(cl_khr_fp64)
DECL_SUB_GROUP_BROADCAST(double, f64)
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_subgroup_ballot)
#define DECL_NON_UNIFORM_BROADCAST_BASE(TYPE, TYPE_ABBR)               \
    TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(                             \
        GroupNonUniformBroadcast,                                      \
        _i32_##TYPE_ABBR##_i32, )(int Execution, TYPE Value, uint Id); \
    TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(                             \
        GroupNonUniformBroadcastFirst, _i32_##TYPE_ABBR, )(int Execution, TYPE Value);

#define DECL_NON_UNIFORM_BROADCAST(TYPE, TYPE_ABBR)         \
    DECL_NON_UNIFORM_BROADCAST_BASE(TYPE, TYPE_ABBR)        \
    DECL_NON_UNIFORM_BROADCAST_BASE(TYPE##2, v2##TYPE_ABBR) \
    DECL_NON_UNIFORM_BROADCAST_BASE(TYPE##3, v3##TYPE_ABBR) \
    DECL_NON_UNIFORM_BROADCAST_BASE(TYPE##4, v4##TYPE_ABBR) \
    DECL_NON_UNIFORM_BROADCAST_BASE(TYPE##8, v8##TYPE_ABBR) \
    DECL_NON_UNIFORM_BROADCAST_BASE(TYPE##16, v16##TYPE_ABBR)

DECL_NON_UNIFORM_BROADCAST(char, i8)
DECL_NON_UNIFORM_BROADCAST(short, i16)
DECL_NON_UNIFORM_BROADCAST(int, i32)
DECL_NON_UNIFORM_BROADCAST(long, i64)
DECL_NON_UNIFORM_BROADCAST(float, f32)
#if defined(cl_khr_fp64)
DECL_NON_UNIFORM_BROADCAST(double, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DECL_NON_UNIFORM_BROADCAST(half, f16)
#endif // defined(cl_khr_fp16)

uint4 SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupNonUniformBallot, _i32_i1, )(int Execution, bool Predicate);
bool SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupNonUniformInverseBallot, _i32_v4i32, )(int Execution, uint4 Value);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBallotBitExtract, _i32_v4i32_i32, )(
    int Execution, uint4 Value, uint Index);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBallotBitCount, _i32_i32_v4i32, )(
    int Execution, int Operation, uint4 Value);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupNonUniformBallotFindLSB, _i32_v4i32, )(int Execution, uint4 Value);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupNonUniformBallotFindMSB, _i32_v4i32, )(int Execution, uint4 Value);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupEqMask, , )(void);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupGeMask, , )(void);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupGtMask, , )(void);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLeMask, , )(void);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLtMask, , )(void);
#endif // defined(cl_khr_subgroup_ballot)

#if defined(cl_khr_subgroup_shuffle)
#define DECL_NON_UNIFORM_SHUFFLE(TYPE, TYPE_ABBR)                      \
    TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(                             \
        GroupNonUniformShuffle,                                        \
        _i32_##TYPE_ABBR##_i32, )(int Execution, TYPE Value, uint Id); \
    TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(                             \
        GroupNonUniformShuffleXor,                                     \
        _i32_##TYPE_ABBR##_i32, )(int Execution, TYPE Value, uint Mask);

DECL_NON_UNIFORM_SHUFFLE(char, i8)
DECL_NON_UNIFORM_SHUFFLE(short, i16)
DECL_NON_UNIFORM_SHUFFLE(int, i32)
DECL_NON_UNIFORM_SHUFFLE(long, i64)
DECL_NON_UNIFORM_SHUFFLE(float, f32)
#if defined(cl_khr_fp64)
DECL_NON_UNIFORM_SHUFFLE(double, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DECL_NON_UNIFORM_SHUFFLE(half, f16)
#endif // defined(cl_khr_fp16)
#endif // defined(cl_khr_subgroup_shuffle)

#if defined(cl_khr_subgroup_shuffle_relative)
#define DECL_NON_UNIFORM_SHUFFLE_RELATIVE(TYPE, TYPE_ABBR)                \
    TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(                                \
        GroupNonUniformShuffleUp,                                         \
        _i32_##TYPE_ABBR##_i32, )(int Execution, TYPE Value, uint Delta); \
    TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(                                \
        GroupNonUniformShuffleDown,                                       \
        _i32_##TYPE_ABBR##_i32, )(int Execution, TYPE Value, uint Delta);

DECL_NON_UNIFORM_SHUFFLE_RELATIVE(char, i8)
DECL_NON_UNIFORM_SHUFFLE_RELATIVE(short, i16)
DECL_NON_UNIFORM_SHUFFLE_RELATIVE(int, i32)
DECL_NON_UNIFORM_SHUFFLE_RELATIVE(long, i64)
DECL_NON_UNIFORM_SHUFFLE_RELATIVE(float, f32)
#if defined(cl_khr_fp64)
DECL_NON_UNIFORM_SHUFFLE_RELATIVE(double, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DECL_NON_UNIFORM_SHUFFLE_RELATIVE(half, f16)
#endif // defined(cl_khr_fp16)
#endif // defined(cl_khr_subgroup_shuffle_relative)

#if defined(cl_khr_subgroup_non_uniform_arithmetic) || \
    defined(cl_khr_subgroup_clustered_reduce)
#define DEFN_NON_UNIFORM_OPERATION_BASE(TYPE, OPERATION, TYPE_ABBR)
#define DEFN_NON_UNIFORM_CLUSTERED_OPERATION(TYPE, OPERATION, TYPE_ABBR)

#if defined(cl_khr_subgroup_non_uniform_arithmetic)
#define DEFN_NON_UNIFORM_OPERATION_BASE(TYPE, OPERATION, TYPE_ABBR) \
    TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(                          \
        GroupNonUniform##OPERATION,                                 \
        _i32_i32_##TYPE_ABBR, )(int Execution, int Operation, TYPE X);
#endif // defined(cl_khr_subgroup_non_uniform_arithmetic)

#if defined(cl_khr_subgroup_clustered_reduce)
#define DEFN_NON_UNIFORM_CLUSTERED_OPERATION(TYPE, OPERATION, TYPE_ABBR) \
    TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(                               \
        GroupNonUniform##OPERATION, _i32_i32_##TYPE_ABBR##_i32, )(       \
        int Execution, int Operation, TYPE X, uint ClusterSize);
#endif // defined(cl_khr_subgroup_clustered_reduce)

#define DEFN_NON_UNIFORM_OPERATION(TYPE, OPERATION, TYPE_ABBR)  \
    DEFN_NON_UNIFORM_OPERATION_BASE(TYPE, OPERATION, TYPE_ABBR) \
    DEFN_NON_UNIFORM_CLUSTERED_OPERATION(TYPE, OPERATION, TYPE_ABBR)

// ARITHMETIC OPERATIONS

// - OpGroupNonUniformIAdd, OpGroupNonUniformFAdd
// - OpGroupNonUniformIMul, OpGroupNonUniformFMul
#define DEFN_NON_UNIFORM_ADD_MUL(TYPE, TYPE_SIGN, TYPE_ABBR)    \
    DEFN_NON_UNIFORM_OPERATION(TYPE, TYPE_SIGN##Add, TYPE_ABBR) \
    DEFN_NON_UNIFORM_OPERATION(TYPE, TYPE_SIGN##Mul, TYPE_ABBR)

DEFN_NON_UNIFORM_ADD_MUL(char, I, i8)
DEFN_NON_UNIFORM_ADD_MUL(short, I, i16)
DEFN_NON_UNIFORM_ADD_MUL(int, I, i32)
DEFN_NON_UNIFORM_ADD_MUL(long, I, i64)
DEFN_NON_UNIFORM_ADD_MUL(float, F, f32)
#if defined(cl_khr_fp64)
DEFN_NON_UNIFORM_ADD_MUL(double, F, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_ADD_MUL(half, F, f16)
#endif // defined(cl_khr_fp16)

// - OpGroupNonUniformSMin, OpGroupNonUniformUMin, OpGroupNonUniformFMin
// - OpGroupNonUniformSMax, OpGroupNonUniformUMax, OpGroupNonUniformFMax
#define DEFN_NON_UNIFORM_MIN_MAX(TYPE, TYPE_SIGN, TYPE_ABBR)    \
    DEFN_NON_UNIFORM_OPERATION(TYPE, TYPE_SIGN##Min, TYPE_ABBR) \
    DEFN_NON_UNIFORM_OPERATION(TYPE, TYPE_SIGN##Max, TYPE_ABBR)

DEFN_NON_UNIFORM_MIN_MAX(char, S, i8)
DEFN_NON_UNIFORM_MIN_MAX(uchar, U, i8)
DEFN_NON_UNIFORM_MIN_MAX(short, S, i16)
DEFN_NON_UNIFORM_MIN_MAX(ushort, U, i16)
DEFN_NON_UNIFORM_MIN_MAX(int, S, i32)
DEFN_NON_UNIFORM_MIN_MAX(uint, U, i32)
DEFN_NON_UNIFORM_MIN_MAX(long, S, i64)
DEFN_NON_UNIFORM_MIN_MAX(ulong, U, i64)
DEFN_NON_UNIFORM_MIN_MAX(float, F, f32)
#if defined(cl_khr_fp64)
DEFN_NON_UNIFORM_MIN_MAX(double, F, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_MIN_MAX(half, F, f16)
#endif // defined(cl_khr_fp16)

// BITWISE OPERATIONS

// - OpGroupNonUniformBitwiseAnd
// - OpGroupNonUniformBitwiseOr
// - OpGroupNonUniformBitwiseXor
#define DEFN_NON_UNIFORM_BITWISE_OPERATIONS(TYPE, TYPE_ABBR) \
    DEFN_NON_UNIFORM_OPERATION(TYPE, BitwiseAnd, TYPE_ABBR)  \
    DEFN_NON_UNIFORM_OPERATION(TYPE, BitwiseOr, TYPE_ABBR)   \
    DEFN_NON_UNIFORM_OPERATION(TYPE, BitwiseXor, TYPE_ABBR)

DEFN_NON_UNIFORM_BITWISE_OPERATIONS(char, i8)
DEFN_NON_UNIFORM_BITWISE_OPERATIONS(short, i16)
DEFN_NON_UNIFORM_BITWISE_OPERATIONS(int, i32)
DEFN_NON_UNIFORM_BITWISE_OPERATIONS(long, i64)

// LOGICAL OPERATIONS

// - OpGroupNonUniformLogicalAnd
// - OpGroupNonUniformLogicalOr
// - OpGroupNonUniformLogicalXor
#define DEFN_NON_UNIFORM_LOGICAL_OPERATIONS(TYPE, TYPE_ABBR) \
    DEFN_NON_UNIFORM_OPERATION(TYPE, LogicalAnd, TYPE_ABBR)  \
    DEFN_NON_UNIFORM_OPERATION(TYPE, LogicalOr, TYPE_ABBR)   \
    DEFN_NON_UNIFORM_OPERATION(TYPE, LogicalXor, TYPE_ABBR)

DEFN_NON_UNIFORM_LOGICAL_OPERATIONS(bool, i1)
#endif // defined(cl_khr_subgroup_non_uniform_arithmetic) || defined(cl_khr_subgroup_clustered_reduce)

char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupIAdd, _i32_i32_i8, )(int Execution, int Operation, char X);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupIAdd, _i32_i32_i16, )(int Execution, int Operation, short X);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupIAdd, _i32_i32_i32, )(int Execution, int Operation, int X);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupIAdd, _i32_i32_i64, )(int Execution, int Operation, long X);

half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupFAdd, _i32_i32_f16, )(int Execution, int Operation, half X);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupFAdd, _i32_i32_f32, )(int Execution, int Operation, float X);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupFAdd, _i32_i32_f64, )(int Execution, int Operation, double X);
#endif // defined(cl_khr_fp64)

uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupUMin, _i32_i32_i8, )(int Execution, int Operation, uchar X);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupUMin, _i32_i32_i16, )(int Execution, int Operation, ushort X);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupUMin, _i32_i32_i32, )(int Execution, int Operation, uint X);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupUMin, _i32_i32_i64, )(int Execution, int Operation, ulong X);

half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupFMin, _i32_i32_f16, )(int Execution, int Operation, half X);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupFMin, _i32_i32_f32, )(int Execution, int Operation, float X);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupFMin, _i32_i32_f64, )(int Execution, int Operation, double X);
#endif // defined(cl_khr_fp64)

char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupSMin, _i32_i32_i8, )(int Execution, int Operation, char X);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupSMin, _i32_i32_i16, )(int Execution, int Operation, short X);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupSMin, _i32_i32_i32, )(int Execution, int Operation, int X);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupSMin, _i32_i32_i64, )(int Execution, int Operation, long X);

half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupFMax, _i32_i32_f16, )(int Execution, int Operation, half X);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupFMax, _i32_i32_f32, )(int Execution, int Operation, float X);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupFMax, _i32_i32_f64, )(int Execution, int Operation, double X);
#endif // defined(cl_khr_fp64)

uchar SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupUMax, _i32_i32_i8, )(int Execution, int Operation, uchar X);
ushort SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupUMax, _i32_i32_i16, )(int Execution, int Operation, ushort X);
uint SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupUMax, _i32_i32_i32, )(int Execution, int Operation, uint X);
ulong SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupUMax, _i32_i32_i64, )(int Execution, int Operation, ulong X);

char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupSMax, _i32_i32_i8, )(int Execution, int Operation, char X);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupSMax, _i32_i32_i16, )(int Execution, int Operation, short X);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupSMax, _i32_i32_i32, )(int Execution, int Operation, int X);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupSMax, _i32_i32_i64, )(int Execution, int Operation, long X);

char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupIMulKHR, _i32_i32_i8, )(int Execution, int Operation, char X);
short SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupIMulKHR, _i32_i32_i16, )(int Execution, int Operation, short X);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupIMulKHR, _i32_i32_i32, )(int Execution, int Operation, int X);
long SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupIMulKHR, _i32_i32_i64, )(int Execution, int Operation, long X);

half SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupFMulKHR, _i32_i32_f16, )(int Execution, int Operation, half X);
float SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupFMulKHR, _i32_i32_f32, )(int Execution, int Operation, float X);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupFMulKHR, _i32_i32_f64, )(int Execution, int Operation, double X);
#endif // defined(cl_khr_fp64)

char SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBitwiseAndKHR, _i32_i32_i8, )(
    int Execution, int Operation, char X);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBitwiseAndKHR, _i32_i32_i16, )(
    int Execution, int Operation, short X);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBitwiseAndKHR, _i32_i32_i32, )(
    int Execution, int Operation, int X);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBitwiseAndKHR, _i32_i32_i64, )(
    int Execution, int Operation, long X);

char SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupBitwiseOrKHR, _i32_i32_i8, )(int Execution, int Operation, char X);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBitwiseOrKHR, _i32_i32_i16, )(
    int Execution, int Operation, short X);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupBitwiseOrKHR, _i32_i32_i32, )(int Execution, int Operation, int X);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBitwiseOrKHR, _i32_i32_i64, )(
    int Execution, int Operation, long X);

char SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBitwiseXorKHR, _i32_i32_i8, )(
    int Execution, int Operation, char X);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBitwiseXorKHR, _i32_i32_i16, )(
    int Execution, int Operation, short X);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBitwiseXorKHR, _i32_i32_i32, )(
    int Execution, int Operation, int X);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBitwiseXorKHR, _i32_i32_i64, )(
    int Execution, int Operation, long X);

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupLogicalAndKHR, _i32_i32_i1, )(
    int Execution, int Operation, bool X);
bool SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupLogicalOrKHR, _i32_i32_i1, )(int Execution, int Operation, bool X);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupLogicalXorKHR, _i32_i32_i1, )(
    int Execution, int Operation, bool X);

// Device-Side Enqueue Instructions
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(EnqueueMarker, _i64_i32_p0i64_p0i64, )(
    __spirv_Queue                Queue,
    uint                         NumEvents,
    __spirv_DeviceEvent private *WaitEvents,
    __spirv_DeviceEvent private *RetEvent);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(EnqueueMarker, _i64_i32_p3i64_p3i64, )(
    __spirv_Queue              Queue,
    uint                       NumEvents,
    __spirv_DeviceEvent local *WaitEvents,
    __spirv_DeviceEvent local *RetEvent);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(EnqueueMarker, _i64_i32_p4i64_p4i64, )(
    __spirv_Queue                Queue,
    uint                         NumEvents,
    __spirv_DeviceEvent generic *WaitEvents,
    __spirv_DeviceEvent generic *RetEvent);

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

typedef struct
{
    uint   dim;
    size_t globalWorkOffset[3];
    size_t globalWorkSize[3];
    size_t localWorkSize[3];
} Ndrange_t;

// OpEnqueueKernel will be custom lowered?
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
uint __builtin_spirv_OpGetKernelWorkGroupSize_fp0i32_p0i8_i32_i32(
    uchar *Invoke, private uchar *Param, uint ParamSize, uint ParamAlign);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(RetainEvent, _i64, )(__spirv_DeviceEvent Event);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ReleaseEvent, _i64, )(__spirv_DeviceEvent Event);
__spirv_DeviceEvent SPIRV_OVERLOADABLE SPIRV_BUILTIN(CreateUserEvent, , )(void);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsValidEvent, _i64, )(__spirv_DeviceEvent Event);
void SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(SetUserEventStatus, _i64_i32, )(__spirv_DeviceEvent Event, int Status);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(CaptureEventProfilingInfo, _i64_i32_p1i8, )(
    __spirv_DeviceEvent Event, int ProfilingInfo, global char *Value);
__spirv_Queue SPIRV_OVERLOADABLE SPIRV_BUILTIN(GetDefaultQueue, , )(void);

#define DECL_BUILD_NDRANGE(TYPE, TYPE_MANGLING)                                         \
    Ndrange_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(                                         \
        BuildNDRange, _##TYPE_MANGLING##_##TYPE_MANGLING##_##TYPE_MANGLING, _1D)(       \
        TYPE GlobalWorkSize, TYPE LocalWorkSize, TYPE GlobalWorkOffset);                \
    Ndrange_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(                                         \
        BuildNDRange, _a2##TYPE_MANGLING##_a2##TYPE_MANGLING##_a2##TYPE_MANGLING, _2D)( \
        TYPE GlobalWorkSize[2], TYPE LocalWorkSize[2], TYPE GlobalWorkOffset[2]);       \
    Ndrange_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(                                         \
        BuildNDRange, _a3##TYPE_MANGLING##_a3##TYPE_MANGLING##_a3##TYPE_MANGLING, _3D)( \
        TYPE GlobalWorkSize[3], TYPE LocalWorkSize[3], TYPE GlobalWorkOffset[3]);

#if __32bit__ > 0
DECL_BUILD_NDRANGE(int, i32)
#else
DECL_BUILD_NDRANGE(long, i64)
#endif

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

// Pipe Instructions
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(ReadPipe, _Pipe_ro_p4i8_i32, )(
    __spirv_Pipe_ro Pipe,
    generic char   *Pointer,
    int             PacketSize /*, int PacketAlignment*/);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(WritePipe, _Pipe_wo_p4i8_i32, )(
    __spirv_Pipe_wo Pipe,
    generic char   *Pointer,
    int             PacketSize /*, int PacketAlignment*/);

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(ReservedReadPipe, _Pipe_ro_ReserveId_i32_p4i8_i32, )(
    __spirv_Pipe_ro   Pipe,
    __spirv_ReserveId ReserveId,
    int               Index,
    generic char     *Pointer,
    int               PacketSize /*, int PacketAlignment*/);
int SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(ReservedWritePipe, _Pipe_wo_ReserveId_i32_p4i8_i32, )(
        __spirv_Pipe_wo   Pipe,
        __spirv_ReserveId ReserveId,
        int               Index,
        generic char     *Pointer,
        int               PacketSize /*, int PacketAlignment*/);

__spirv_ReserveId
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ReserveReadPipePackets, _Pipe_ro_i32_i32, )(
        __spirv_Pipe_ro Pipe, int NumPackets, int PacketSize /*, int PacketAlignment*/);
__spirv_ReserveId
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(ReserveWritePipePackets, _Pipe_wo_i32_i32, )(
        __spirv_Pipe_wo Pipe, int NumPackets, int PacketSize /*, int PacketAlignment*/);

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(CommitReadPipe, _Pipe_ro_ReserveId_i32, )(
    __spirv_Pipe_ro   Pipe,
    __spirv_ReserveId ReserveId,
    int               PacketSize /*, int PacketAlignment*/);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(CommitWritePipe, _Pipe_wo_ReserveId_i32, )(
    __spirv_Pipe_wo   Pipe,
    __spirv_ReserveId ReserveId,
    int               PacketSize /*, int PacketAlignment*/);

bool SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(IsValidReserveId, _ReserveId, )(__spirv_ReserveId ReserveId);

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(GetNumPipePackets, _Pipe_ro_i32, )(
    __spirv_Pipe_ro Pipe, int PacketSize /*, int PacketAlignment*/);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(GetNumPipePackets, _Pipe_wo_i32, )(
    __spirv_Pipe_wo Pipe, int PacketSize /*, int PacketAlignment*/);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(GetMaxPipePackets, _Pipe_ro_i32, )(
    __spirv_Pipe_ro Pipe, int PacketSize /*, int PacketAlignment*/);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(GetMaxPipePackets, _Pipe_wo_i32, )(
    __spirv_Pipe_wo Pipe, int PacketSize /*, int PacketAlignment*/);

__spirv_ReserveId
    SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupReserveReadPipePackets, _i32_Pipe_ro_i32_i32, )(
        int             Execution,
        __spirv_Pipe_ro Pipe,
        int             NumPackets,
        int             PacketSize /*, int PacketAlignment*/);
__spirv_ReserveId SPIRV_OVERLOADABLE
    SPIRV_BUILTIN(GroupReserveWritePipePackets, _i32_Pipe_wo_i32_i32, )(
        int             Execution,
        __spirv_Pipe_wo Pipe,
        int             NumPackets,
        int             PacketSize /*, int PacketAlignment*/);

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupCommitReadPipe, _i32_Pipe_ro_ReserveId_i32, )(
    int               Execution,
    __spirv_Pipe_ro   Pipe,
    __spirv_ReserveId ReserveId,
    int               PacketSize /*, int PacketAlignment*/);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupCommitWritePipe, _i32_Pipe_wo_ReserveId_i32, )(
    int               Execution,
    __spirv_Pipe_wo   Pipe,
    __spirv_ReserveId ReserveId,
    int               PacketSize /*, int PacketAlignment*/);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#include "spirv_math.h"

char2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v2i8_v2i8, )(char2 v, char2 m);
char2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v4i8_v2i8, )(char4 v, char2 m);
char2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v8i8_v2i8, )(char8 v, char2 m);
char2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v16i8_v2i8, )(char16 v, char2 m);
char4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v2i8_v4i8, )(char2 v, char4 m);
char4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v4i8_v4i8, )(char4 v, char4 m);
char4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v8i8_v4i8, )(char8 v, char4 m);
char4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v16i8_v4i8, )(char16 v, char4 m);
char8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v2i8_v8i8, )(char2 v, char8 m);
char8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v4i8_v8i8, )(char4 v, char8 m);
char8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v8i8_v8i8, )(char8 v, char8 m);
char8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v16i8_v8i8, )(char16 v, char8 m);
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v2i8_v16i8, )(char2 v, char16 m);
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v4i8_v16i8, )(char4 v, char16 m);
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v8i8_v16i8, )(char8 v, char16 m);
char16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v16i8_v16i8, )(char16 v, char16 m);
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v2i16_v2i16, )(short2 v, short2 m);
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v4i16_v2i16, )(short4 v, short2 m);
short2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v8i16_v2i16, )(short8 v, short2 m);
short2
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v16i16_v2i16, )(short16 v, short2 m);
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v2i16_v4i16, )(short2 v, short4 m);
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v4i16_v4i16, )(short4 v, short4 m);
short4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v8i16_v4i16, )(short8 v, short4 m);
short4
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v16i16_v4i16, )(short16 v, short4 m);
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v2i16_v8i16, )(short2 v, short8 m);
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v4i16_v8i16, )(short4 v, short8 m);
short8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v8i16_v8i16, )(short8 v, short8 m);
short8
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v16i16_v8i16, )(short16 v, short8 m);
short16
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v2i16_v16i16, )(short2 v, short16 m);
short16
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v4i16_v16i16, )(short4 v, short16 m);
short16
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v8i16_v16i16, )(short8 v, short16 m);
short16
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v16i16_v16i16, )(short16 v, short16 m);
int2 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(shuffle, _v2i32_v2i32, )(int2 v, int2 m);
int2 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(shuffle, _v4i32_v2i32, )(int4 v, int2 m);
int2 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(shuffle, _v8i32_v2i32, )(int8 v, int2 m);
int2 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(shuffle, _v16i32_v2i32, )(int16 v, int2 m);
int4 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(shuffle, _v2i32_v4i32, )(int2 v, int4 m);
int4 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(shuffle, _v4i32_v4i32, )(int4 v, int4 m);
int4 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(shuffle, _v8i32_v4i32, )(int8 v, int4 m);
int4 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(shuffle, _v16i32_v4i32, )(int16 v, int4 m);
int8 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(shuffle, _v2i32_v8i32, )(int2 v, int8 m);
int8 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(shuffle, _v4i32_v8i32, )(int4 v, int8 m);
int8 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(shuffle, _v8i32_v8i32, )(int8 v, int8 m);
int8 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(shuffle, _v16i32_v8i32, )(int16 v, int8 m);
int16 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v2i32_v16i32, )(int2 v, int16 m);
int16 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v4i32_v16i32, )(int4 v, int16 m);
int16 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v8i32_v16i32, )(int8 v, int16 m);
int16 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v16i32_v16i32, )(int16 v, int16 m);
long2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v2i64_v2i64, )(long2 v, long2 m);
long2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v4i64_v2i64, )(long4 v, long2 m);
long2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v8i64_v2i64, )(long8 v, long2 m);
long2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v16i64_v2i64, )(long16 v, long2 m);
long4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v2i64_v4i64, )(long2 v, long4 m);
long4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v4i64_v4i64, )(long4 v, long4 m);
long4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v8i64_v4i64, )(long8 v, long4 m);
long4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v16i64_v4i64, )(long16 v, long4 m);
long8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v2i64_v8i64, )(long2 v, long8 m);
long8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v4i64_v8i64, )(long4 v, long8 m);
long8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v8i64_v8i64, )(long8 v, long8 m);
long8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v16i64_v8i64, )(long16 v, long8 m);
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v2i64_v16i64, )(long2 v, long16 m);
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v4i64_v16i64, )(long4 v, long16 m);
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v8i64_v16i64, )(long8 v, long16 m);
long16
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v16i64_v16i64, )(long16 v, long16 m);
float2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v2f32_v2i32, )(float2 v, int2 m);
float2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v4f32_v2i32, )(float4 v, int2 m);
float2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v8f32_v2i32, )(float8 v, int2 m);
float2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v16f32_v2i32, )(float16 v, int2 m);
float4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v2f32_v4i32, )(float2 v, int4 m);
float4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v4f32_v4i32, )(float4 v, int4 m);
float4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v8f32_v4i32, )(float8 v, int4 m);
float4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v16f32_v4i32, )(float16 v, int4 m);
float8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v2f32_v8i32, )(float2 v, int8 m);
float8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v4f32_v8i32, )(float4 v, int8 m);
float8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v8f32_v8i32, )(float8 v, int8 m);
float8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v16f32_v8i32, )(float16 v, int8 m);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v2f32_v16i32, )(float2 v, int16 m);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v4f32_v16i32, )(float4 v, int16 m);
float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v8f32_v16i32, )(float8 v, int16 m);
float16
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v16f32_v16i32, )(float16 v, int16 m);

char2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2i8_v2i8_v2i8, )(char2 v0, char2 v1, char2 m);
char2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4i8_v4i8_v2i8, )(char4 v0, char4 v1, char2 m);
char2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8i8_v8i8_v2i8, )(char8 v0, char8 v1, char2 m);
char2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16i8_v16i8_v2i8, )(char16 v0, char16 v1, char2 m);
char4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2i8_v2i8_v4i8, )(char2 v0, char2 v1, char4 m);
char4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4i8_v4i8_v4i8, )(char4 v0, char4 v1, char4 m);
char4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8i8_v8i8_v4i8, )(char8 v0, char8 v1, char4 m);
char4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16i8_v16i8_v4i8, )(char16 v0, char16 v1, char4 m);
char8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2i8_v2i8_v8i8, )(char2 v0, char2 v1, char8 m);
char8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4i8_v4i8_v8i8, )(char4 v0, char4 v1, char8 m);
char8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8i8_v8i8_v8i8, )(char8 v0, char8 v1, char8 m);
char8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16i8_v16i8_v8i8, )(char16 v0, char16 v1, char8 m);
char16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2i8_v2i8_v16i8, )(char2 v0, char2 v1, char16 m);
char16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4i8_v4i8_v16i8, )(char4 v0, char4 v1, char16 m);
char16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8i8_v8i8_v16i8, )(char8 v0, char8 v1, char16 m);
char16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16i8_v16i8_v16i8, )(char16 v0, char16 v1, char16 m);
short2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2i16_v2i16_v2i16, )(short2 v0, short2 v1, short2 m);
short2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4i16_v4i16_v2i16, )(short4 v0, short4 v1, short2 m);
short2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8i16_v8i16_v2i16, )(short8 v0, short8 v1, short2 m);
short2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16i16_v16i16_v2i16, )(short16 v0, short16 v1, short2 m);
short4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2i16_v2i16_v4i16, )(short2 v0, short2 v1, short4 m);
short4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4i16_v4i16_v4i16, )(short4 v0, short4 v1, short4 m);
short4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8i16_v8i16_v4i16, )(short8 v0, short8 v1, short4 m);
short4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16i16_v16i16_v4i16, )(short16 v0, short16 v1, short4 m);
short8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2i16_v2i16_v8i16, )(short2 v0, short2 v1, short8 m);
short8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4i16_v4i16_v8i16, )(short4 v0, short4 v1, short8 m);
short8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8i16_v8i16_v8i16, )(short8 v0, short8 v1, short8 m);
short8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16i16_v16i16_v8i16, )(short16 v0, short16 v1, short8 m);
short16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2i16_v2i16_v16i16, )(short2 v0, short2 v1, short16 m);
short16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4i16_v4i16_v16i16, )(short4 v0, short4 v1, short16 m);
short16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8i16_v8i16_v16i16, )(short8 v0, short8 v1, short16 m);
short16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle2, _v16i16_v16i16_v16i16, )(
    short16 v0, short16 v1, short16 m);
int2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2i32_v2i32_v2i32, )(int2 v0, int2 v1, int2 m);
int2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4i32_v4i32_v2i32, )(int4 v0, int4 v1, int2 m);
int2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8i32_v8i32_v2i32, )(int8 v0, int8 v1, int2 m);
int2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16i32_v16i32_v2i32, )(int16 v0, int16 v1, int2 m);
int4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2i32_v2i32_v4i32, )(int2 v0, int2 v1, int4 m);
int4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4i32_v4i32_v4i32, )(int4 v0, int4 v1, int4 m);
int4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8i32_v8i32_v4i32, )(int8 v0, int8 v1, int4 m);
int4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16i32_v16i32_v4i32, )(int16 v0, int16 v1, int4 m);
int8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2i32_v2i32_v8i32, )(int2 v0, int2 v1, int8 m);
int8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4i32_v4i32_v8i32, )(int4 v0, int4 v1, int8 m);
int8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8i32_v8i32_v8i32, )(int8 v0, int8 v1, int8 m);
int8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16i32_v16i32_v8i32, )(int16 v0, int16 v1, int8 m);
int16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2i32_v2i32_v16i32, )(int2 v0, int2 v1, int16 m);
int16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4i32_v4i32_v16i32, )(int4 v0, int4 v1, int16 m);
int16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8i32_v8i32_v16i32, )(int8 v0, int8 v1, int16 m);
int16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16i32_v16i32_v16i32, )(int16 v0, int16 v1, int16 m);
long2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2i64_v2i64_v2i64, )(long2 v0, long2 v1, long2 m);
long2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4i64_v4i64_v2i64, )(long4 v0, long4 v1, long2 m);
long2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8i64_v8i64_v2i64, )(long8 v0, long8 v1, long2 m);
long2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16i64_v16i64_v2i64, )(long16 v0, long16 v1, long2 m);
long4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2i64_v2i64_v4i64, )(long2 v0, long2 v1, long4 m);
long4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4i64_v4i64_v4i64, )(long4 v0, long4 v1, long4 m);
long4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8i64_v8i64_v4i64, )(long8 v0, long8 v1, long4 m);
long4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16i64_v16i64_v4i64, )(long16 v0, long16 v1, long4 m);
long8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2i64_v2i64_v8i64, )(long2 v0, long2 v1, long8 m);
long8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4i64_v4i64_v8i64, )(long4 v0, long4 v1, long8 m);
long8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8i64_v8i64_v8i64, )(long8 v0, long8 v1, long8 m);
long8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16i64_v16i64_v8i64, )(long16 v0, long16 v1, long8 m);
long16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2i64_v2i64_v16i64, )(long2 v0, long2 v1, long16 m);
long16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4i64_v4i64_v16i64, )(long4 v0, long4 v1, long16 m);
long16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8i64_v8i64_v16i64, )(long8 v0, long8 v1, long16 m);
long16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16i64_v16i64_v16i64, )(long16 v0, long16 v1, long16 m);
float2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2f32_v2f32_v2i32, )(float2 v0, float2 v1, int2 m);
float2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4f32_v4f32_v2i32, )(float4 v0, float4 v1, int2 m);
float2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8f32_v8f32_v2i32, )(float8 v0, float8 v1, int2 m);
float2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16f32_v16f32_v2i32, )(float16 v0, float16 v1, int2 m);
float4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2f32_v2f32_v4i32, )(float2 v0, float2 v1, int4 m);
float4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4f32_v4f32_v4i32, )(float4 v0, float4 v1, int4 m);
float4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8f32_v8f32_v4i32, )(float8 v0, float8 v1, int4 m);
float4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16f32_v16f32_v4i32, )(float16 v0, float16 v1, int4 m);
float8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2f32_v2f32_v8i32, )(float2 v0, float2 v1, int8 m);
float8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4f32_v4f32_v8i32, )(float4 v0, float4 v1, int8 m);
float8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8f32_v8f32_v8i32, )(float8 v0, float8 v1, int8 m);
float8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16f32_v16f32_v8i32, )(float16 v0, float16 v1, int8 m);
float16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2f32_v2f32_v16i32, )(float2 v0, float2 v1, int16 m);
float16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4f32_v4f32_v16i32, )(float4 v0, float4 v1, int16 m);
float16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8f32_v8f32_v16i32, )(float8 v0, float8 v1, int16 m);
float16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16f32_v16f32_v16i32, )(float16 v0, float16 v1, int16 m);

half2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v2f16_v2i16, )(half2 v, short2 m);
half2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v4f16_v2i16, )(half4 v, short2 m);
half2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v8f16_v2i16, )(half8 v, short2 m);
half2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v16f16_v2i16, )(half16 v, short2 m);
half4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v2f16_v4i16, )(half2 v, short4 m);
half4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v4f16_v4i16, )(half4 v, short4 m);
half4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v8f16_v4i16, )(half8 v, short4 m);
half4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v16f16_v4i16, )(half16 v, short4 m);
half8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v2f16_v8i16, )(half2 v, short8 m);
half8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v4f16_v8i16, )(half4 v, short8 m);
half8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v8f16_v8i16, )(half8 v, short8 m);
half8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(shuffle, _v16f16_v8i16, )(half16 v, short8 m);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v2f16_v16i16, )(half2 v, short16 m);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v4f16_v16i16, )(half4 v, short16 m);
half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v8f16_v16i16, )(half8 v, short16 m);
half16
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v16f16_v16i16, )(half16 v, short16 m);

half2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2f16_v2f16_v2i16, )(half2 v0, half2 v1, short2 m);
half2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4f16_v4f16_v2i16, )(half4 v0, half4 v1, short2 m);
half2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8f16_v8f16_v2i16, )(half8 v0, half8 v1, short2 m);
half2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16f16_v16f16_v2i16, )(half16 v0, half16 v1, short2 m);
half4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2f16_v2f16_v4i16, )(half2 v0, half2 v1, short4 m);
half4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4f16_v4f16_v4i16, )(half4 v0, half4 v1, short4 m);
half4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8f16_v8f16_v4i16, )(half8 v0, half8 v1, short4 m);
half4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16f16_v16f16_v4i16, )(half16 v0, half16 v1, short4 m);
half8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2f16_v2f16_v8i16, )(half2 v0, half2 v1, short8 m);
half8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4f16_v4f16_v8i16, )(half4 v0, half4 v1, short8 m);
half8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8f16_v8f16_v8i16, )(half8 v0, half8 v1, short8 m);
half8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16f16_v16f16_v8i16, )(half16 v0, half16 v1, short8 m);
half16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2f16_v2f16_v16i16, )(half2 v0, half2 v1, short16 m);
half16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4f16_v4f16_v16i16, )(half4 v0, half4 v1, short16 m);
half16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8f16_v8f16_v16i16, )(half8 v0, half8 v1, short16 m);
half16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v16f16_v16f16_v16i16, )(half16 v0, half16 v1, short16 m);

#if defined(cl_khr_fp64)
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v2f64_v2i64, )(double2 v, long2 m);
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v4f64_v2i64, )(double4 v, long2 m);
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v8f64_v2i64, )(double8 v, long2 m);
double2
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v16f64_v2i64, )(double16 v, long2 m);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v2f64_v4i64, )(double2 v, long4 m);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v4f64_v4i64, )(double4 v, long4 m);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v8f64_v4i64, )(double8 v, long4 m);
double4
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v16f64_v4i64, )(double16 v, long4 m);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v2f64_v8i64, )(double2 v, long8 m);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v4f64_v8i64, )(double4 v, long8 m);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v8f64_v8i64, )(double8 v, long8 m);
double8
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v16f64_v8i64, )(double16 v, long8 m);
double16
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v2f64_v16i64, )(double2 v, long16 m);
double16
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v4f64_v16i64, )(double4 v, long16 m);
double16
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v8f64_v16i64, )(double8 v, long16 m);
double16
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle, _v16f64_v16i64, )(double16 v, long16 m);

double2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2f64_v2f64_v2i64, )(double2 v0, double2 v1, long2 m);
double2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4f64_v4f64_v2i64, )(double4 v0, double4 v1, long2 m);
double2 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8f64_v8f64_v2i64, )(double8 v0, double8 v1, long2 m);
double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle2, _v16f64_v16f64_v2i64, )(
    double16 v0, double16 v1, long2 m);
double4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2f64_v2f64_v4i64, )(double2 v0, double2 v1, long4 m);
double4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4f64_v4f64_v4i64, )(double4 v0, double4 v1, long4 m);
double4 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8f64_v8f64_v4i64, )(double8 v0, double8 v1, long4 m);
double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle2, _v16f64_v16f64_v4i64, )(
    double16 v0, double16 v1, long4 m);
double8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2f64_v2f64_v8i64, )(double2 v0, double2 v1, long8 m);
double8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4f64_v4f64_v8i64, )(double4 v0, double4 v1, long8 m);
double8 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8f64_v8f64_v8i64, )(double8 v0, double8 v1, long8 m);
double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle2, _v16f64_v16f64_v8i64, )(
    double16 v0, double16 v1, long8 m);
double16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v2f64_v2f64_v16i64, )(double2 v0, double2 v1, long16 m);
double16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v4f64_v4f64_v16i64, )(double4 v0, double4 v1, long16 m);
double16 SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(shuffle2, _v8f64_v8f64_v16i64, )(double8 v0, double8 v1, long16 m);
double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(shuffle2, _v16f64_v16f64_v16i64, )(
    double16 v0, double16 v1, long16 m);
#endif // defined(cl_khr_fp64)

char SPIRV_OVERLOADABLE    SPIRV_OCL_BUILTIN(s_min, _i8_i8, )(char x, char y);
char2 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(s_min, _v2i8_v2i8, )(char2 x, char2 y);
char3 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(s_min, _v3i8_v3i8, )(char3 x, char3 y);
char4 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(s_min, _v4i8_v4i8, )(char4 x, char4 y);
char8 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(s_min, _v8i8_v8i8, )(char8 x, char8 y);
char16 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(s_min, _v16i8_v16i8, )(char16 x, char16 y);
uchar SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(u_min, _i8_i8, )(uchar x, uchar y);
uchar2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(u_min, _v2i8_v2i8, )(uchar2 x, uchar2 y);
uchar3 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(u_min, _v3i8_v3i8, )(uchar3 x, uchar3 y);
uchar4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(u_min, _v4i8_v4i8, )(uchar4 x, uchar4 y);
uchar8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(u_min, _v8i8_v8i8, )(uchar8 x, uchar8 y);
uchar16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_min, _v16i8_v16i8, )(uchar16 x, uchar16 y);
short SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(s_min, _i16_i16, )(short x, short y);
short2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(s_min, _v2i16_v2i16, )(short2 x, short2 y);
short3 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(s_min, _v3i16_v3i16, )(short3 x, short3 y);
short4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(s_min, _v4i16_v4i16, )(short4 x, short4 y);
short8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(s_min, _v8i16_v8i16, )(short8 x, short8 y);
short16
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_min, _v16i16_v16i16, )(short16 x, short16 y);
ushort SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(u_min, _i16_i16, )(ushort x, ushort y);
ushort2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_min, _v2i16_v2i16, )(ushort2 x, ushort2 y);
ushort3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_min, _v3i16_v3i16, )(ushort3 x, ushort3 y);
ushort4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_min, _v4i16_v4i16, )(ushort4 x, ushort4 y);
ushort8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_min, _v8i16_v8i16, )(ushort8 x, ushort8 y);
ushort16
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_min, _v16i16_v16i16, )(ushort16 x, ushort16 y);
int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_min, _i32_i32, )(int x, int y);
int2 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(s_min, _v2i32_v2i32, )(int2 x, int2 y);
int3 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(s_min, _v3i32_v3i32, )(int3 x, int3 y);
int4 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(s_min, _v4i32_v4i32, )(int4 x, int4 y);
int8 SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(s_min, _v8i32_v8i32, )(int8 x, int8 y);
int16 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(s_min, _v16i32_v16i32, )(int16 x, int16 y);
uint SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(u_min, _i32_i32, )(uint x, uint y);
uint2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(u_min, _v2i32_v2i32, )(uint2 x, uint2 y);
uint3 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(u_min, _v3i32_v3i32, )(uint3 x, uint3 y);
uint4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(u_min, _v4i32_v4i32, )(uint4 x, uint4 y);
uint8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(u_min, _v8i32_v8i32, )(uint8 x, uint8 y);
uint16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_min, _v16i32_v16i32, )(uint16 x, uint16 y);
long SPIRV_OVERLOADABLE   SPIRV_OCL_BUILTIN(s_min, _i64_i64, )(long x, long y);
long2 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(s_min, _v2i64_v2i64, )(long2 x, long2 y);
long3 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(s_min, _v3i64_v3i64, )(long3 x, long3 y);
long4 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(s_min, _v4i64_v4i64, )(long4 x, long4 y);
long8 SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(s_min, _v8i64_v8i64, )(long8 x, long8 y);
long16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(s_min, _v16i64_v16i64, )(long16 x, long16 y);
ulong SPIRV_OVERLOADABLE  SPIRV_OCL_BUILTIN(u_min, _i64_i64, )(ulong x, ulong y);
ulong2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_min, _v2i64_v2i64, )(ulong2 x, ulong2 y);
ulong3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_min, _v3i64_v3i64, )(ulong3 x, ulong3 y);
ulong4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_min, _v4i64_v4i64, )(ulong4 x, ulong4 y);
ulong8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_min, _v8i64_v8i64, )(ulong8 x, ulong8 y);
ulong16
    SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(u_min, _v16i64_v16i64, )(ulong16 x, ulong16 y);

// Misc Instructions

void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1i8_i32, )(global char *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v2i8_i32, )(global char2 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v3i8_i32, )(global char3 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v4i8_i32, )(global char4 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v8i8_i32, )(global char8 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v16i8_i32, )(global char16 *p, int num_elements);

void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1i16_i32, )(global short *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v2i16_i32, )(global short2 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v3i16_i32, )(global short3 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v4i16_i32, )(global short4 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v8i16_i32, )(global short8 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v16i16_i32, )(global short16 *p, int num_elements);

void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1i32_i32, )(global int *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v2i32_i32, )(global int2 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v3i32_i32, )(global int3 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v4i32_i32, )(global int4 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v8i32_i32, )(global int8 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v16i32_i32, )(global int16 *p, int num_elements);

void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1i64_i32, )(global long *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v2i64_i32, )(global long2 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v3i64_i32, )(global long3 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v4i64_i32, )(global long4 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v8i64_i32, )(global long8 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v16i64_i32, )(global long16 *p, int num_elements);

void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1f32_i32, )(global float *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v2f32_i32, )(global float2 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v3f32_i32, )(global float3 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v4f32_i32, )(global float4 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v8f32_i32, )(global float8 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v16f32_i32, )(global float16 *p, int num_elements);

void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1f16_i32, )(global half *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v2f16_i32, )(global half2 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v3f16_i32, )(global half3 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v4f16_i32, )(global half4 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v8f16_i32, )(global half8 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v16f16_i32, )(global half16 *p, int num_elements);

#if defined(cl_khr_fp64)
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1f64_i32, )(global double *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v2f64_i32, )(global double2 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v3f64_i32, )(global double3 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v4f64_i32, )(global double4 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v8f64_i32, )(global double8 *p, int num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v16f64_i32, )(global double16 *p, int num_elements);
#endif // defined(cl_khr_fp64)

void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1i8_i64, )(global char *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v2i8_i64, )(global char2 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v3i8_i64, )(global char3 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v4i8_i64, )(global char4 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v8i8_i64, )(global char8 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v16i8_i64, )(global char16 *p, long num_elements);

void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1i16_i64, )(global short *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v2i16_i64, )(global short2 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v3i16_i64, )(global short3 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v4i16_i64, )(global short4 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v8i16_i64, )(global short8 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v16i16_i64, )(global short16 *p, long num_elements);

void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1i32_i64, )(global int *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v2i32_i64, )(global int2 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v3i32_i64, )(global int3 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v4i32_i64, )(global int4 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v8i32_i64, )(global int8 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v16i32_i64, )(global int16 *p, long num_elements);

void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1i64_i64, )(global long *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v2i64_i64, )(global long2 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v3i64_i64, )(global long3 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v4i64_i64, )(global long4 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v8i64_i64, )(global long8 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v16i64_i64, )(global long16 *p, long num_elements);

void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1f32_i64, )(global float *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v2f32_i64, )(global float2 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v3f32_i64, )(global float3 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v4f32_i64, )(global float4 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v8f32_i64, )(global float8 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v16f32_i64, )(global float16 *p, long num_elements);

void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1f16_i64, )(global half *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v2f16_i64, )(global half2 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v3f16_i64, )(global half3 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v4f16_i64, )(global half4 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v8f16_i64, )(global half8 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v16f16_i64, )(global half16 *p, long num_elements);

#if defined(cl_khr_fp64)
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1f64_i64, )(global double *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v2f64_i64, )(global double2 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v3f64_i64, )(global double3 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v4f64_i64, )(global double4 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v8f64_i64, )(global double8 *p, long num_elements);
void SPIRV_OVERLOADABLE
    SPIRV_OCL_BUILTIN(prefetch, _p1v16f64_i64, )(global double16 *p, long num_elements);
#endif // defined(cl_khr_fp64)

ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(ReadClockKHR, _i64_i32, _Rulong)(int scope);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ReadClockKHR, _v2i32_i32, _Ruint2)(int scope);

int SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubDeviceIDINTEL, , )(void);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(GlobalHWThreadIDINTEL, , )(void);

#endif // __SPIRV_H__
