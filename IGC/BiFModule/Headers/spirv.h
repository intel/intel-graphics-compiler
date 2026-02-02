/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __SPIRV_H__
#define __SPIRV_H__

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
#define CLK_NULL_RESERVE_ID                               \
    (__builtin_IB_convert_object_type_to_spirv_reserveid( \
        ((void *)(~INTEL_PIPE_RESERVE_ID_VALID_BIT))))

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

size_t __attribute__((overloadable)) __spirv_BuiltInLocalInvocationIndex(void);
size_t __attribute__((overloadable)) __spirv_BuiltInGlobalLinearId(void);
uint __attribute__((overloadable))   __spirv_BuiltInWorkDim(void);
uint __attribute__((overloadable))   __spirv_BuiltInSubgroupMaxSize(void);
uint __attribute__((overloadable))   __spirv_BuiltInSubgroupId(void);
uint __attribute__((overloadable))   __spirv_BuiltInNumSubgroups(void);
uint __attribute__((overloadable))   __spirv_BuiltInSubgroupSize(void);
uint __attribute__((overloadable))   __spirv_BuiltInNumEnqueuedSubgroups(void);
uint __attribute__((overloadable))   __spirv_BuiltInSubgroupLocalInvocationId(void);

// Image Instructions
//

__spirv_SampledImage_1D __attribute__((overloadable))
__spirv_SampledImage(global Img1d_ro *Image, __spirv_Sampler Sampler);
__spirv_SampledImage_2D __attribute__((overloadable))
__spirv_SampledImage(global Img2d_ro *Image, __spirv_Sampler Sampler);
__spirv_SampledImage_3D __attribute__((overloadable))
__spirv_SampledImage(global Img3d_ro *Image, __spirv_Sampler Sampler);
__spirv_SampledImage_1D_array __attribute__((overloadable))
__spirv_SampledImage(global Img1d_array_ro *Image, __spirv_Sampler Sampler);
__spirv_SampledImage_2D_array __attribute__((overloadable))
__spirv_SampledImage(global Img2d_array_ro *Image, __spirv_Sampler Sampler);
__spirv_SampledImage_2D_depth __attribute__((overloadable))
__spirv_SampledImage(global Img2d_depth_ro *Image, __spirv_Sampler Sampler);
__spirv_SampledImage_2D_array_depth __attribute__((overloadable))
__spirv_SampledImage(global Img2d_array_depth_ro *Image, __spirv_Sampler Sampler);

float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_2D SampledImage,
    float2                  Coordinate,
    int                     ImageOperands,
    float                   Lod);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_2D SampledImage, int2 Coordinate, int ImageOperands, float Lod);
int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_2D SampledImage,
    float2                  Coordinate,
    int                     ImageOperands,
    float                   Lod);
int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_2D SampledImage, int2 Coordinate, int ImageOperands, float Lod);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_3D SampledImage,
    float4                  Coordinate,
    int                     ImageOperands,
    float                   Lod);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_3D SampledImage, int4 Coordinate, int ImageOperands, float Lod);
int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_3D SampledImage,
    float4                  Coordinate,
    int                     ImageOperands,
    float                   Lod);
int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_3D SampledImage, int4 Coordinate, int ImageOperands, float Lod);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_2D_array SampledImage,
    float4                        Coordinate,
    int                           ImageOperands,
    float                         Lod);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_2D_array SampledImage,
    int4                          Coordinate,
    int                           ImageOperands,
    float                         Lod);
int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_2D_array SampledImage,
    float4                        Coordinate,
    int                           ImageOperands,
    float                         Lod);
int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_2D_array SampledImage,
    int4                          Coordinate,
    int                           ImageOperands,
    float                         Lod);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_1D SampledImage, float Coordinate, int ImageOperands, float Lod);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_1D SampledImage, int Coordinate, int ImageOperands, float Lod);
int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_1D SampledImage, float Coordinate, int ImageOperands, float Lod);
int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_1D SampledImage, int Coordinate, int ImageOperands, float Lod);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_1D_array SampledImage,
    float2                        Coordinate,
    int                           ImageOperands,
    float                         Lod);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_1D_array SampledImage,
    int2                          Coordinate,
    int                           ImageOperands,
    float                         Lod);
int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_1D_array SampledImage,
    float2                        Coordinate,
    int                           ImageOperands,
    float                         Lod);
int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_1D_array SampledImage,
    int2                          Coordinate,
    int                           ImageOperands,
    float                         Lod);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_2D_depth SampledImage,
    float2                        Coordinate,
    int                           ImageOperands,
    float                         Lod);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_2D_depth SampledImage,
    int2                          Coordinate,
    int                           ImageOperands,
    float                         Lod);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_2D_array_depth SampledImage,
    float4                              Coordinate,
    int                                 ImageOperands,
    float                               Lod);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_2D_array_depth SampledImage,
    int4                                Coordinate,
    int                                 ImageOperands,
    float                               Lod);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_2D SampledImage,
    float2                  Coordinate,
    int                     ImageOperands,
    float2                  dx,
    float2                  dy);
int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_2D SampledImage,
    float2                  Coordinate,
    int                     ImageOperands,
    float2                  dx,
    float2                  dy);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_3D SampledImage,
    float4                  Coordinate,
    int                     ImageOperands,
    float4                  dx,
    float4                  dy);
int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_3D SampledImage,
    float4                  Coordinate,
    int                     ImageOperands,
    float4                  dx,
    float4                  dy);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_2D SampledImage,
    float4                  Coordinate,
    int                     ImageOperands,
    float2                  dx,
    float2                  dy);
int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_2D SampledImage,
    float4                  Coordinate,
    int                     ImageOperands,
    float2                  dx,
    float2                  dy);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_1D SampledImage,
    float                   Coordinate,
    int                     ImageOperands,
    float                   dx,
    float                   dy);
int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_1D SampledImage,
    float                   Coordinate,
    int                     ImageOperands,
    float                   dx,
    float                   dy);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_1D_array SampledImage,
    float2                        Coordinate,
    int                           ImageOperands,
    float                         dx,
    float                         dy);
int4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rint4(
    __spirv_SampledImage_1D_array SampledImage,
    float2                        Coordinate,
    int                           ImageOperands,
    float                         dx,
    float                         dy);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_2D_depth SampledImage,
    float2                        Coordinate,
    int                           ImageOperands,
    float2                        dx,
    float2                        dy);
float4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rfloat4(
    __spirv_SampledImage_2D_array_depth SampledImage,
    float4                              Coordinate,
    int                                 ImageOperands,
    float2                              dx,
    float2                              dy);

#ifdef cl_khr_fp16
half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(
    __spirv_SampledImage_2D SampledImage,
    float2                  Coordinate,
    int                     ImageOperands,
    float                   Lod);
half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(
    __spirv_SampledImage_2D SampledImage, int2 Coordinate, int ImageOperands, float Lod);
half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(
    __spirv_SampledImage_3D SampledImage,
    float4                  Coordinate,
    int                     ImageOperands,
    float                   Lod);
half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(
    __spirv_SampledImage_3D SampledImage, int4 Coordinate, int ImageOperands, float Lod);
half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(
    __spirv_SampledImage_2D_array SampledImage,
    float4                        Coordinate,
    int                           ImageOperands,
    float                         Lod);
half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(
    __spirv_SampledImage_2D_array SampledImage,
    int4                          Coordinate,
    int                           ImageOperands,
    float                         Lod);
half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(
    __spirv_SampledImage_1D_array SampledImage,
    float2                        Coordinate,
    int                           ImageOperands,
    float                         Lod);
half4 __attribute__((overloadable)) __spirv_ImageSampleExplicitLod_Rhalf4(
    __spirv_SampledImage_1D_array SampledImage,
    int2                          Coordinate,
    int                           ImageOperands,
    float                         Lod);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img2d_wo *Image, int2 Coordinate, half4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img2d_rw *Image, int2 Coordinate, half4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img3d_wo *Image, int4 Coordinate, half4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img3d_rw *Image, int4 Coordinate, half4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img2d_array_wo *Image, int4 Coordinate, half4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img2d_array_rw *Image, int4 Coordinate, half4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_wo *Image, int Coordinate, half4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_rw *Image, int Coordinate, half4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_buffer_wo *Image, int Coordinate, half4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_buffer_rw *Image, int Coordinate, half4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_array_wo *Image, int2 Coordinate, half4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_array_rw *Image, int2 Coordinate, half4 Texel);
half4 __attribute__((overloadable))
__spirv_ImageRead_Rhalf4(global Img2d_ro *Image, int2 Coordinate);
half4 __attribute__((overloadable))
__spirv_ImageRead_Rhalf4(global Img2d_rw *Image, int2 Coordinate);
half4 __attribute__((overloadable))
__spirv_ImageRead_Rhalf4(global Img3d_ro *Image, int4 Coordinate);
half4 __attribute__((overloadable))
__spirv_ImageRead_Rhalf4(global Img3d_rw *Image, int4 Coordinate);
half4 __attribute__((overloadable))
__spirv_ImageRead_Rhalf4(global Img2d_array_ro *Image, int4 Coordinate);
half4 __attribute__((overloadable))
__spirv_ImageRead_Rhalf4(global Img2d_array_rw *Image, int4 Coordinate);
half4 __attribute__((overloadable))
__spirv_ImageRead_Rhalf4(global Img1d_ro *Image, int Coordinate);
half4 __attribute__((overloadable))
__spirv_ImageRead_Rhalf4(global Img1d_rw *Image, int Coordinate);
half4 __attribute__((overloadable))
__spirv_ImageRead_Rhalf4(global Img1d_buffer_ro *Image, int Coordinate);
half4 __attribute__((overloadable))
__spirv_ImageRead_Rhalf4(global Img1d_buffer_rw *Image, int Coordinate);
half4 __attribute__((overloadable))
__spirv_ImageRead_Rhalf4(global Img1d_array_ro *Image, int2 Coordinate);
half4 __attribute__((overloadable))
__spirv_ImageRead_Rhalf4(global Img1d_array_rw *Image, int2 Coordinate);
#endif // cl_khr_fp16

// Pipe in image type information.
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img2d_ro *Image, int2 Coordinate);
float4 __attribute__((overloadable))
__spirv_ImageRead_Rfloat4(global Img2d_ro *Image, int2 Coordinate);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img2d_rw *Image, int2 Coordinate);
float4 __attribute__((overloadable))
__spirv_ImageRead_Rfloat4(global Img2d_rw *Image, int2 Coordinate);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img3d_ro *Image, int4 Coordinate);
float4 __attribute__((overloadable))
__spirv_ImageRead_Rfloat4(global Img3d_ro *Image, int4 Coordinate);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img3d_rw *Image, int4 Coordinate);
float4 __attribute__((overloadable))
__spirv_ImageRead_Rfloat4(global Img3d_rw *Image, int4 Coordinate);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img2d_array_ro *Image, int4 Coordinate);
float4 __attribute__((overloadable))
__spirv_ImageRead_Rfloat4(global Img2d_array_ro *Image, int4 Coordinate);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img2d_array_rw *Image, int4 Coordinate);
float4 __attribute__((overloadable))
__spirv_ImageRead_Rfloat4(global Img2d_array_rw *Image, int4 Coordinate);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img1d_ro *Image, int Coordinate);
float4 __attribute__((overloadable))
__spirv_ImageRead_Rfloat4(global Img1d_ro *Image, int Coordinate);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img1d_rw *Image, int Coordinate);
float4 __attribute__((overloadable))
__spirv_ImageRead_Rfloat4(global Img1d_rw *Image, int Coordinate);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img1d_buffer_ro *Image, int Coordinate);
float4 __attribute__((overloadable))
__spirv_ImageRead_Rfloat4(global Img1d_buffer_ro *Image, int Coordinate);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img1d_buffer_rw *Image, int Coordinate);
float4 __attribute__((overloadable))
__spirv_ImageRead_Rfloat4(global Img1d_buffer_rw *Image, int Coordinate);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img1d_array_ro *Image, int2 Coordinate);
float4 __attribute__((overloadable))
__spirv_ImageRead_Rfloat4(global Img1d_array_ro *Image, int2 Coordinate);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img1d_array_rw *Image, int2 Coordinate);
float4 __attribute__((overloadable))
__spirv_ImageRead_Rfloat4(global Img1d_array_rw *Image, int2 Coordinate);
float __attribute__((overloadable))
__spirv_ImageRead_Rfloat(global Img2d_depth_ro *Image, int2 Coordinate);
float __attribute__((overloadable))
__spirv_ImageRead_Rfloat(global Img2d_depth_rw *Image, int2 Coordinate);
float __attribute__((overloadable))
__spirv_ImageRead_Rfloat(global Img2d_array_depth_ro *Image, int4 Coordinate);
float __attribute__((overloadable))
__spirv_ImageRead_Rfloat(global Img2d_array_depth_rw *Image, int4 Coordinate);

// Image Read MSAA
int4 __attribute__((overloadable)) __spirv_ImageRead_Rint4(
    global Img2d_msaa_ro *Image, int2 Coordinate, int ImageOperands, int Sample);
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(
    global Img2d_msaa_ro *Image, int2 Coordinate, int ImageOperands, int Sample);
int4 __attribute__((overloadable)) __spirv_ImageRead_Rint4(
    global Img2d_array_msaa_ro *Image, int4 Coordinate, int ImageOperands, int Sample);
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(
    global Img2d_array_msaa_ro *Image, int4 Coordinate, int ImageOperands, int Sample);
float __attribute__((overloadable)) __spirv_ImageRead_Rfloat(
    global Img2d_msaa_depth_ro *Image, int2 Coordinate, int ImageOperands, int Sample);
float __attribute__((overloadable)) __spirv_ImageRead_Rfloat(
    global Img2d_array_msaa_depth_ro *Image,
    int4                              Coordinate,
    int                               ImageOperands,
    int                               Sample);

// Image Read with unused ImageOperands
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img2d_ro *Image, int2 Coordinate, int ImageOperands);
float4 __attribute__((overloadable))
__spirv_ImageRead_Rfloat4(global Img2d_ro *Image, int2 Coordinate, int ImageOperands);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img3d_ro *Image, int4 Coordinate, int ImageOperands);
float4 __attribute__((overloadable))
__spirv_ImageRead_Rfloat4(global Img3d_ro *Image, int4 Coordinate, int ImageOperands);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img1d_ro *Image, int Coordinate, int ImageOperands);
float4 __attribute__((overloadable))
__spirv_ImageRead_Rfloat4(global Img1d_ro *Image, int Coordinate, int ImageOperands);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img2d_array_ro *Image, int4 Coordinate, int ImageOperands);
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(
    global Img2d_array_ro *Image, int4 Coordinate, int ImageOperands);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img1d_array_ro *Image, int2 Coordinate, int ImageOperands);
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(
    global Img1d_array_ro *Image, int2 Coordinate, int ImageOperands);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img1d_buffer_ro *Image, int Coordinate, int ImageOperands);
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(
    global Img1d_buffer_ro *Image, int Coordinate, int ImageOperands);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img2d_rw *Image, int2 Coordinate, int ImageOperands);
float4 __attribute__((overloadable))
__spirv_ImageRead_Rfloat4(global Img2d_rw *Image, int2 Coordinate, int ImageOperands);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img3d_rw *Image, int4 Coordinate, int ImageOperands);
float4 __attribute__((overloadable))
__spirv_ImageRead_Rfloat4(global Img3d_rw *Image, int4 Coordinate, int ImageOperands);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img1d_rw *Image, int Coordinate, int ImageOperands);
float4 __attribute__((overloadable))
__spirv_ImageRead_Rfloat4(global Img1d_rw *Image, int Coordinate, int ImageOperands);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img2d_array_rw *Image, int4 Coordinate, int ImageOperands);
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(
    global Img2d_array_rw *Image, int4 Coordinate, int ImageOperands);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img1d_array_rw *Image, int2 Coordinate, int ImageOperands);
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(
    global Img1d_array_rw *Image, int2 Coordinate, int ImageOperands);
int4 __attribute__((overloadable))
__spirv_ImageRead_Rint4(global Img1d_buffer_rw *Image, int Coordinate, int ImageOperands);
float4 __attribute__((overloadable)) __spirv_ImageRead_Rfloat4(
    global Img1d_buffer_rw *Image, int Coordinate, int ImageOperands);

// Image Write
void __attribute__((overloadable))
__spirv_ImageWrite(global Img2d_wo *Image, int2 Coordinate, int4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img2d_wo *Image, int2 Coordinate, float4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img2d_rw *Image, int2 Coordinate, int4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img2d_rw *Image, int2 Coordinate, float4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img2d_array_wo *Image, int4 Coordinate, int4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img2d_array_wo *Image, int4 Coordinate, float4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img2d_array_rw *Image, int4 Coordinate, int4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img2d_array_rw *Image, int4 Coordinate, float4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_wo *Image, int Coordinate, int4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_wo *Image, int Coordinate, float4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_rw *Image, int Coordinate, int4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_rw *Image, int Coordinate, float4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_buffer_wo *Image, int Coordinate, int4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_buffer_wo *Image, int Coordinate, float4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_buffer_rw *Image, int Coordinate, int4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_buffer_rw *Image, int Coordinate, float4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_array_wo *Image, int2 Coordinate, int4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_array_wo *Image, int2 Coordinate, float4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_array_rw *Image, int2 Coordinate, int4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_array_rw *Image, int2 Coordinate, float4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img2d_depth_wo *Image, int2 Coordinate, float Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img2d_depth_rw *Image, int2 Coordinate, float Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img2d_array_depth_wo *Image, int4 Coordinate, float Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img2d_array_depth_rw *Image, int4 Coordinate, float Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img3d_wo *Image, int4 Coordinate, int4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img3d_wo *Image, int4 Coordinate, float4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img3d_rw *Image, int4 Coordinate, int4 Texel);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img3d_rw *Image, int4 Coordinate, float4 Texel);

// Image Write with unused ImageOperands
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img2d_wo *Image, int2 Coordinate, int4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img2d_wo *Image, int2 Coordinate, float4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img2d_depth_wo *Image, int2 Coordinate, float Texel, int ImageOperands);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_wo *Image, int Coordinate, int4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img1d_wo *Image, int Coordinate, float4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img1d_array_wo *Image, int2 Coordinate, int4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img1d_array_wo *Image, int2 Coordinate, float4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img1d_buffer_wo *Image, int Coordinate, int4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img1d_buffer_wo *Image, int Coordinate, float4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img2d_array_wo *Image, int4 Coordinate, int4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img2d_array_wo *Image, int4 Coordinate, float4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img2d_array_depth_wo *Image, int4 Coordinate, float Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img3d_wo *Image, int4 Coordinate, int4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img3d_wo *Image, int4 Coordinate, float4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img2d_rw *Image, int2 Coordinate, int4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img2d_rw *Image, int2 Coordinate, float4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img2d_depth_rw *Image, int2 Coordinate, float Texel, int ImageOperands);
void __attribute__((overloadable))
__spirv_ImageWrite(global Img1d_rw *Image, int Coordinate, int4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img1d_rw *Image, int Coordinate, float4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img1d_array_rw *Image, int2 Coordinate, int4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img1d_array_rw *Image, int2 Coordinate, float4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img1d_buffer_rw *Image, int Coordinate, int4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img1d_buffer_rw *Image, int Coordinate, float4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img2d_array_rw *Image, int4 Coordinate, int4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img2d_array_rw *Image, int4 Coordinate, float4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img2d_array_depth_rw *Image, int4 Coordinate, float Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img3d_rw *Image, int4 Coordinate, int4 Texel, int ImageOperands);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img3d_rw *Image, int4 Coordinate, float4 Texel, int ImageOperands);

// Image Write LoD
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img2d_wo *Image, int2 Coordinate, int4 Texel, int ImageOperands, int Lod);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img2d_wo *Image, int2 Coordinate, float4 Texel, int ImageOperands, int Lod);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img2d_depth_wo *Image,
    int2                   Coordinate,
    float                  Texel,
    int                    ImageOperands,
    int                    Lod);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img1d_wo *Image, int Coordinate, int4 Texel, int ImageOperands, int Lod);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img1d_wo *Image, int Coordinate, float4 Texel, int ImageOperands, int Lod);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img1d_array_wo *Image,
    int2                   Coordinate,
    int4                   Texel,
    int                    ImageOperands,
    int                    Lod);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img1d_array_wo *Image,
    int2                   Coordinate,
    float4                 Texel,
    int                    ImageOperands,
    int                    Lod);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img2d_array_wo *Image,
    int4                   Coordinate,
    int4                   Texel,
    int                    ImageOperands,
    int                    Lod);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img2d_array_wo *Image,
    int4                   Coordinate,
    float4                 Texel,
    int                    ImageOperands,
    int                    Lod);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img2d_array_depth_wo *Image,
    int4                         Coordinate,
    float                        Texel,
    int                          ImageOperands,
    int                          Lod);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img3d_wo *Image, int4 Coordinate, int4 Texel, int ImageOperands, int Lod);
void __attribute__((overloadable)) __spirv_ImageWrite(
    global Img3d_wo *Image, int4 Coordinate, float4 Texel, int ImageOperands, int Lod);

// Image Query Format
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img2d_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img2d_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img2d_rw *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img3d_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img3d_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img3d_rw *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img1d_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img1d_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img1d_rw *Image);
uint __attribute__((overloadable))
__spirv_ImageQueryFormat(global Img1d_buffer_ro *Image);
uint __attribute__((overloadable))
__spirv_ImageQueryFormat(global Img1d_buffer_wo *Image);
uint __attribute__((overloadable))
__spirv_ImageQueryFormat(global Img1d_buffer_rw *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img1d_array_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img1d_array_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img1d_array_rw *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img2d_array_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img2d_array_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img2d_array_rw *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img2d_depth_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img2d_depth_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img2d_depth_rw *Image);
uint __attribute__((overloadable))
__spirv_ImageQueryFormat(global Img2d_array_depth_ro *Image);
uint __attribute__((overloadable))
__spirv_ImageQueryFormat(global Img2d_array_depth_wo *Image);
uint __attribute__((overloadable))
__spirv_ImageQueryFormat(global Img2d_array_depth_rw *Image);

uint __attribute__((overloadable)) __spirv_ImageQueryFormat(global Img2d_msaa_ro *Image);
uint __attribute__((overloadable))
__spirv_ImageQueryFormat(global Img2d_array_msaa_ro *Image);
uint __attribute__((overloadable))
__spirv_ImageQueryFormat(global Img2d_msaa_depth_ro *Image);
uint __attribute__((overloadable))
__spirv_ImageQueryFormat(global Img2d_array_msaa_depth_ro *Image);

// Image Query Order
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img2d_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img2d_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img2d_rw *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img3d_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img3d_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img3d_rw *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img1d_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img1d_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img1d_rw *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img1d_buffer_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img1d_buffer_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img1d_buffer_rw *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img1d_array_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img1d_array_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img1d_array_rw *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img2d_array_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img2d_array_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img2d_array_rw *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img2d_depth_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img2d_depth_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img2d_depth_rw *Image);
uint __attribute__((overloadable))
__spirv_ImageQueryOrder(global Img2d_array_depth_ro *Image);
uint __attribute__((overloadable))
__spirv_ImageQueryOrder(global Img2d_array_depth_wo *Image);
uint __attribute__((overloadable))
__spirv_ImageQueryOrder(global Img2d_array_depth_rw *Image);

uint __attribute__((overloadable)) __spirv_ImageQueryOrder(global Img2d_msaa_ro *Image);
uint __attribute__((overloadable))
__spirv_ImageQueryOrder(global Img2d_array_msaa_ro *Image);
uint __attribute__((overloadable))
__spirv_ImageQueryOrder(global Img2d_msaa_depth_ro *Image);
uint __attribute__((overloadable))
__spirv_ImageQueryOrder(global Img2d_array_msaa_depth_ro *Image);

// Image Query Size
int __attribute__((overloadable)) __spirv_ImageQuerySize_Rint(global Img1d_ro *Image);
int __attribute__((overloadable)) __spirv_ImageQuerySize_Rint(global Img1d_wo *Image);
int __attribute__((overloadable)) __spirv_ImageQuerySize_Rint(global Img1d_rw *Image);
int __attribute__((overloadable))
__spirv_ImageQuerySize_Rint(global Img1d_buffer_ro *Image);
int __attribute__((overloadable))
__spirv_ImageQuerySize_Rint(global Img1d_buffer_wo *Image);
int __attribute__((overloadable))
__spirv_ImageQuerySize_Rint(global Img1d_buffer_rw *Image);
int2 __attribute__((overloadable))
__spirv_ImageQuerySize_Rint2(global Img1d_array_ro *Image);
int2 __attribute__((overloadable))
__spirv_ImageQuerySize_Rint2(global Img1d_array_wo *Image);
int2 __attribute__((overloadable))
__spirv_ImageQuerySize_Rint2(global Img1d_array_rw *Image);
int2 __attribute__((overloadable)) __spirv_ImageQuerySize_Rint2(global Img2d_ro *Image);
int2 __attribute__((overloadable)) __spirv_ImageQuerySize_Rint2(global Img2d_wo *Image);
int2 __attribute__((overloadable)) __spirv_ImageQuerySize_Rint2(global Img2d_rw *Image);
int2 __attribute__((overloadable))
__spirv_ImageQuerySize_Rint2(global Img2d_depth_ro *Image);
int2 __attribute__((overloadable))
__spirv_ImageQuerySize_Rint2(global Img2d_depth_wo *Image);
int2 __attribute__((overloadable))
__spirv_ImageQuerySize_Rint2(global Img2d_depth_rw *Image);
int3 __attribute__((overloadable))
__spirv_ImageQuerySize_Rint3(global Img2d_array_ro *Image);
int3 __attribute__((overloadable))
__spirv_ImageQuerySize_Rint3(global Img2d_array_wo *Image);
int3 __attribute__((overloadable))
__spirv_ImageQuerySize_Rint3(global Img2d_array_rw *Image);
int3 __attribute__((overloadable))
__spirv_ImageQuerySize_Rint3(global Img2d_array_depth_ro *Image);
int3 __attribute__((overloadable))
__spirv_ImageQuerySize_Rint3(global Img2d_array_depth_wo *Image);
int3 __attribute__((overloadable))
__spirv_ImageQuerySize_Rint3(global Img2d_array_depth_rw *Image);
int3 __attribute__((overloadable)) __spirv_ImageQuerySize_Rint3(global Img3d_ro *Image);
int3 __attribute__((overloadable)) __spirv_ImageQuerySize_Rint3(global Img3d_wo *Image);
int3 __attribute__((overloadable)) __spirv_ImageQuerySize_Rint3(global Img3d_rw *Image);

int2 __attribute__((overloadable))
__spirv_ImageQuerySize_Rint2(global Img2d_msaa_ro *Image);
int2 __attribute__((overloadable))
__spirv_ImageQuerySize_Rint2(global Img2d_msaa_depth_ro *Image);
int3 __attribute__((overloadable))
__spirv_ImageQuerySize_Rint3(global Img2d_array_msaa_ro *Image);
int3 __attribute__((overloadable))
__spirv_ImageQuerySize_Rint3(global Img2d_array_msaa_depth_ro *Image);

long __attribute__((overloadable)) __spirv_ImageQuerySize_Rlong(global Img1d_ro *Image);
long __attribute__((overloadable)) __spirv_ImageQuerySize_Rlong(global Img1d_wo *Image);
long __attribute__((overloadable)) __spirv_ImageQuerySize_Rlong(global Img1d_rw *Image);
long __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong(global Img1d_buffer_ro *Image);
long __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong(global Img1d_buffer_wo *Image);
long __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong(global Img1d_buffer_rw *Image);
long2 __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong2(global Img1d_array_ro *Image);
long2 __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong2(global Img1d_array_wo *Image);
long2 __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong2(global Img1d_array_rw *Image);
long2 __attribute__((overloadable)) __spirv_ImageQuerySize_Rlong2(global Img2d_ro *Image);
long2 __attribute__((overloadable)) __spirv_ImageQuerySize_Rlong2(global Img2d_wo *Image);
long2 __attribute__((overloadable)) __spirv_ImageQuerySize_Rlong2(global Img2d_rw *Image);
long2 __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong2(global Img2d_depth_ro *Image);
long2 __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong2(global Img2d_depth_wo *Image);
long2 __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong2(global Img2d_depth_rw *Image);
long3 __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong3(global Img2d_array_ro *Image);
long3 __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong3(global Img2d_array_wo *Image);
long3 __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong3(global Img2d_array_rw *Image);
long3 __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong3(global Img2d_array_depth_ro *Image);
long3 __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong3(global Img2d_array_depth_wo *Image);
long3 __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong3(global Img2d_array_depth_rw *Image);
long3 __attribute__((overloadable)) __spirv_ImageQuerySize_Rlong3(global Img3d_ro *Image);
long3 __attribute__((overloadable)) __spirv_ImageQuerySize_Rlong3(global Img3d_wo *Image);
long3 __attribute__((overloadable)) __spirv_ImageQuerySize_Rlong3(global Img3d_rw *Image);

long2 __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong2(global Img2d_msaa_ro *Image);
long2 __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong2(global Img2d_msaa_depth_ro *Image);
long3 __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong3(global Img2d_array_msaa_ro *Image);
long3 __attribute__((overloadable))
__spirv_ImageQuerySize_Rlong3(global Img2d_array_msaa_depth_ro *Image);

// Image Query Size Lod
int __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint(global Img1d_ro *Image, int Lod);
int __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint(global Img1d_wo *Image, int Lod);
int __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint(global Img1d_rw *Image, int Lod);
int __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint(global Img1d_buffer_ro *Image, int Lod);
int __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint(global Img1d_buffer_wo *Image, int Lod);
int __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint(global Img1d_buffer_rw *Image, int Lod);
int2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint2(global Img1d_array_ro *Image, int Lod);
int2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint2(global Img1d_array_wo *Image, int Lod);
int2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint2(global Img1d_array_rw *Image, int Lod);
int2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint2(global Img2d_ro *Image, int Lod);
int2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint2(global Img2d_wo *Image, int Lod);
int2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint2(global Img2d_rw *Image, int Lod);
int2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint2(global Img2d_depth_ro *Image, int Lod);
int2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint2(global Img2d_depth_wo *Image, int Lod);
int2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint2(global Img2d_depth_rw *Image, int Lod);
int3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint3(global Img2d_array_ro *Image, int Lod);
int3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint3(global Img2d_array_wo *Image, int Lod);
int3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint3(global Img2d_array_rw *Image, int Lod);
int3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint3(global Img2d_array_depth_ro *Image, int Lod);
int3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint3(global Img2d_array_depth_wo *Image, int Lod);
int3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint3(global Img2d_array_depth_rw *Image, int Lod);
int3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint3(global Img3d_ro *Image, int Lod);
int3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint3(global Img3d_wo *Image, int Lod);
int3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint3(global Img3d_rw *Image, int Lod);

int2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint2(global Img2d_msaa_ro *Image, int Lod);
int2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint2(global Img2d_msaa_depth_ro *Image, int Lod);
int3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint3(global Img2d_array_msaa_ro *Image, int Lod);
int3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rint3(global Img2d_array_msaa_depth_ro *Image, int Lod);

long __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong(global Img1d_ro *Image, int Lod);
long __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong(global Img1d_wo *Image, int Lod);
long __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong(global Img1d_rw *Image, int Lod);
long __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong(global Img1d_buffer_ro *Image, int Lod);
long __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong(global Img1d_buffer_wo *Image, int Lod);
long __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong(global Img1d_buffer_rw *Image, int Lod);
long2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong2(global Img1d_array_ro *Image, int Lod);
long2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong2(global Img1d_array_wo *Image, int Lod);
long2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong2(global Img1d_array_rw *Image, int Lod);
long2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong2(global Img2d_ro *Image, int Lod);
long2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong2(global Img2d_wo *Image, int Lod);
long2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong2(global Img2d_rw *Image, int Lod);
long2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong2(global Img2d_depth_ro *Image, int Lod);
long2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong2(global Img2d_depth_wo *Image, int Lod);
long2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong2(global Img2d_depth_rw *Image, int Lod);
long3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong3(global Img2d_array_ro *Image, int Lod);
long3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong3(global Img2d_array_wo *Image, int Lod);
long3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong3(global Img2d_array_rw *Image, int Lod);
long3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong3(global Img2d_array_depth_ro *Image, int Lod);
long3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong3(global Img2d_array_depth_wo *Image, int Lod);
long3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong3(global Img2d_array_depth_rw *Image, int Lod);
long3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong3(global Img3d_ro *Image, int Lod);
long3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong3(global Img3d_wo *Image, int Lod);
long3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong3(global Img3d_rw *Image, int Lod);

long2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong2(global Img2d_msaa_ro *Image, int Lod);
long2 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong2(global Img2d_msaa_depth_ro *Image, int Lod);
long3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong3(global Img2d_array_msaa_ro *Image, int Lod);
long3 __attribute__((overloadable))
__spirv_ImageQuerySizeLod_Rlong3(global Img2d_array_msaa_depth_ro *Image, int Lod);

// Image Query Levels
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img1d_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img1d_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img1d_rw *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img2d_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img2d_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img2d_rw *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img3d_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img3d_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img3d_rw *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img1d_array_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img1d_array_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img1d_array_rw *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img2d_array_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img2d_array_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img2d_array_rw *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img2d_depth_ro *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img2d_depth_wo *Image);
uint __attribute__((overloadable)) __spirv_ImageQueryLevels(global Img2d_depth_rw *Image);
uint __attribute__((overloadable))
__spirv_ImageQueryLevels(global Img2d_array_depth_ro *Image);
uint __attribute__((overloadable))
__spirv_ImageQueryLevels(global Img2d_array_depth_wo *Image);
uint __attribute__((overloadable))
__spirv_ImageQueryLevels(global Img2d_array_depth_rw *Image);

// Image Query Samples
uint __attribute__((overloadable)) __spirv_ImageQuerySamples(global Img2d_msaa_ro *Image);
uint __attribute__((overloadable))
__spirv_ImageQuerySamples(global Img2d_array_msaa_ro *Image);
uint __attribute__((overloadable))
__spirv_ImageQuerySamples(global Img2d_msaa_depth_ro *Image);
uint __attribute__((overloadable))
__spirv_ImageQuerySamples(global Img2d_array_msaa_depth_ro *Image);

// Conversion Instructions

uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar(half FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort(half FloatValue);
uint __attribute__((overloadable))   __spirv_ConvertFToU_Ruint(half FloatValue);
ulong __attribute__((overloadable))  __spirv_ConvertFToU_Rulong(half FloatValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar(float FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort(float FloatValue);
uint __attribute__((overloadable))   __spirv_ConvertFToU_Ruint(float FloatValue);
ulong __attribute__((overloadable))  __spirv_ConvertFToU_Rulong(float FloatValue);
#if defined(cl_khr_fp64)
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar(double FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort(double FloatValue);
uint __attribute__((overloadable))   __spirv_ConvertFToU_Ruint(double FloatValue);
ulong __attribute__((overloadable))  __spirv_ConvertFToU_Rulong(double FloatValue);
#endif // defined(cl_khr_fp64)
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar(half FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort(half FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint(half FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong(half FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar(float FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort(float FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint(float FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong(float FloatValue);
#if defined(cl_khr_fp64)
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar(double FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort(double FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint(double FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong(double FloatValue);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf(char SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat(char SignedValue);
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf(short SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat(short SignedValue);
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf(int SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat(int SignedValue);
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf(long SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat(long SignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf(uchar UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat(uchar UnsignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf(ushort UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat(ushort UnsignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf(uint UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat(uint UnsignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf(ulong UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat(ulong UnsignedValue);
#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble(char SignedValue);
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble(short SignedValue);
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble(int SignedValue);
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble(long SignedValue);
double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble(uchar UnsignedValue);
double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble(ushort UnsignedValue);
double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble(uint UnsignedValue);
double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble(ulong UnsignedValue);
#endif // defined(cl_khr_fp64)
uchar __attribute__((overloadable))  __spirv_UConvert_Ruchar(uchar UnsignedValue);
ushort __attribute__((overloadable)) __spirv_UConvert_Rushort(uchar UnsignedValue);
uint __attribute__((overloadable))   __spirv_UConvert_Ruint(uchar UnsignedValue);
ulong __attribute__((overloadable))  __spirv_UConvert_Rulong(uchar UnsignedValue);
uchar __attribute__((overloadable))  __spirv_UConvert_Ruchar(ushort UnsignedValue);
ushort __attribute__((overloadable)) __spirv_UConvert_Rushort(ushort UnsignedValue);
uint __attribute__((overloadable))   __spirv_UConvert_Ruint(ushort UnsignedValue);
ulong __attribute__((overloadable))  __spirv_UConvert_Rulong(ushort UnsignedValue);
uchar __attribute__((overloadable))  __spirv_UConvert_Ruchar(uint UnsignedValue);
ushort __attribute__((overloadable)) __spirv_UConvert_Rushort(uint UnsignedValue);
uint __attribute__((overloadable))   __spirv_UConvert_Ruint(uint UnsignedValue);
ulong __attribute__((overloadable))  __spirv_UConvert_Rulong(uint UnsignedValue);
uchar __attribute__((overloadable))  __spirv_UConvert_Ruchar(ulong UnsignedValue);
ushort __attribute__((overloadable)) __spirv_UConvert_Rushort(ulong UnsignedValue);
uint __attribute__((overloadable))   __spirv_UConvert_Ruint(ulong UnsignedValue);
ulong __attribute__((overloadable))  __spirv_UConvert_Rulong(ulong UnsignedValue);
char __attribute__((overloadable))   __spirv_SConvert_Rchar(char SignedValue);
short __attribute__((overloadable))  __spirv_SConvert_Rshort(char SignedValue);
int __attribute__((overloadable))    __spirv_SConvert_Rint(char SignedValue);
long __attribute__((overloadable))   __spirv_SConvert_Rlong(char SignedValue);
char __attribute__((overloadable))   __spirv_SConvert_Rchar(short SignedValue);
short __attribute__((overloadable))  __spirv_SConvert_Rshort(short SignedValue);
int __attribute__((overloadable))    __spirv_SConvert_Rint(short SignedValue);
long __attribute__((overloadable))   __spirv_SConvert_Rlong(short SignedValue);
char __attribute__((overloadable))   __spirv_SConvert_Rchar(int SignedValue);
short __attribute__((overloadable))  __spirv_SConvert_Rshort(int SignedValue);
int __attribute__((overloadable))    __spirv_SConvert_Rint(int SignedValue);
long __attribute__((overloadable))   __spirv_SConvert_Rlong(int SignedValue);
char __attribute__((overloadable))   __spirv_SConvert_Rchar(long SignedValue);
short __attribute__((overloadable))  __spirv_SConvert_Rshort(long SignedValue);
int __attribute__((overloadable))    __spirv_SConvert_Rint(long SignedValue);
long __attribute__((overloadable))   __spirv_SConvert_Rlong(long SignedValue);
half __attribute__((overloadable))   __spirv_FConvert_Rhalf(half FloatValue);
float __attribute__((overloadable))  __spirv_FConvert_Rfloat(half FloatValue);
half __attribute__((overloadable))   __spirv_FConvert_Rhalf(float FloatValue);
float __attribute__((overloadable))  __spirv_FConvert_Rfloat(float FloatValue);
#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_FConvert_Rdouble(half FloatValue);
double __attribute__((overloadable)) __spirv_FConvert_Rdouble(float FloatValue);
half __attribute__((overloadable))   __spirv_FConvert_Rhalf(double FloatValue);
float __attribute__((overloadable))  __spirv_FConvert_Rfloat(double FloatValue);
double __attribute__((overloadable)) __spirv_FConvert_Rdouble(double FloatValue);
#endif // defined(cl_khr_fp64)
uchar __attribute__((overloadable))  __spirv_SatConvertSToU_Ruchar(char SignedValue);
ushort __attribute__((overloadable)) __spirv_SatConvertSToU_Rushort(char SignedValue);
uint __attribute__((overloadable))   __spirv_SatConvertSToU_Ruint(char SignedValue);
ulong __attribute__((overloadable))  __spirv_SatConvertSToU_Rulong(char SignedValue);
uchar __attribute__((overloadable))  __spirv_SatConvertSToU_Ruchar(short SignedValue);
ushort __attribute__((overloadable)) __spirv_SatConvertSToU_Rushort(short SignedValue);
uint __attribute__((overloadable))   __spirv_SatConvertSToU_Ruint(short SignedValue);
ulong __attribute__((overloadable))  __spirv_SatConvertSToU_Rulong(short SignedValue);
uchar __attribute__((overloadable))  __spirv_SatConvertSToU_Ruchar(int SignedValue);
ushort __attribute__((overloadable)) __spirv_SatConvertSToU_Rushort(int SignedValue);
uint __attribute__((overloadable))   __spirv_SatConvertSToU_Ruint(int SignedValue);
ulong __attribute__((overloadable))  __spirv_SatConvertSToU_Rulong(int SignedValue);
uchar __attribute__((overloadable))  __spirv_SatConvertSToU_Ruchar(long SignedValue);
ushort __attribute__((overloadable)) __spirv_SatConvertSToU_Rushort(long SignedValue);
uint __attribute__((overloadable))   __spirv_SatConvertSToU_Ruint(long SignedValue);
ulong __attribute__((overloadable))  __spirv_SatConvertSToU_Rulong(long SignedValue);
char __attribute__((overloadable))   __spirv_SatConvertUToS_Rchar(uchar UnsignedValue);
short __attribute__((overloadable))  __spirv_SatConvertUToS_Rshort(uchar UnsignedValue);
int __attribute__((overloadable))    __spirv_SatConvertUToS_Rint(uchar UnsignedValue);
long __attribute__((overloadable))   __spirv_SatConvertUToS_Rlong(uchar UnsignedValue);
char __attribute__((overloadable))   __spirv_SatConvertUToS_Rchar(ushort UnsignedValue);
short __attribute__((overloadable))  __spirv_SatConvertUToS_Rshort(ushort UnsignedValue);
int __attribute__((overloadable))    __spirv_SatConvertUToS_Rint(ushort UnsignedValue);
long __attribute__((overloadable))   __spirv_SatConvertUToS_Rlong(ushort UnsignedValue);
char __attribute__((overloadable))   __spirv_SatConvertUToS_Rchar(uint UnsignedValue);
short __attribute__((overloadable))  __spirv_SatConvertUToS_Rshort(uint UnsignedValue);
int __attribute__((overloadable))    __spirv_SatConvertUToS_Rint(uint UnsignedValue);
long __attribute__((overloadable))   __spirv_SatConvertUToS_Rlong(uint UnsignedValue);
char __attribute__((overloadable))   __spirv_SatConvertUToS_Rchar(ulong UnsignedValue);
short __attribute__((overloadable))  __spirv_SatConvertUToS_Rshort(ulong UnsignedValue);
int __attribute__((overloadable))    __spirv_SatConvertUToS_Rint(ulong UnsignedValue);
long __attribute__((overloadable))   __spirv_SatConvertUToS_Rlong(ulong UnsignedValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar_rte(half FloatValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar_rtz(half FloatValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar_rtp(half FloatValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar_rtn(half FloatValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar_sat(half FloatValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar_sat_rte(half FloatValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar_sat_rtz(half FloatValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar_sat_rtp(half FloatValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar_sat_rtn(half FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rte(half FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtz(half FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtp(half FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtn(half FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat(half FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rte(half FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtz(half FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtp(half FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtn(half FloatValue);
uint __attribute__((overloadable))   __spirv_ConvertFToU_Ruint_rte(half FloatValue);
uint __attribute__((overloadable))   __spirv_ConvertFToU_Ruint_rtz(half FloatValue);
uint __attribute__((overloadable))   __spirv_ConvertFToU_Ruint_rtp(half FloatValue);
uint __attribute__((overloadable))   __spirv_ConvertFToU_Ruint_rtn(half FloatValue);
uint __attribute__((overloadable))   __spirv_ConvertFToU_Ruint_sat(half FloatValue);
uint __attribute__((overloadable))   __spirv_ConvertFToU_Ruint_sat_rte(half FloatValue);
uint __attribute__((overloadable))   __spirv_ConvertFToU_Ruint_sat_rtz(half FloatValue);
uint __attribute__((overloadable))   __spirv_ConvertFToU_Ruint_sat_rtp(half FloatValue);
uint __attribute__((overloadable))   __spirv_ConvertFToU_Ruint_sat_rtn(half FloatValue);
ulong __attribute__((overloadable))  __spirv_ConvertFToU_Rulong_rte(half FloatValue);
ulong __attribute__((overloadable))  __spirv_ConvertFToU_Rulong_rtz(half FloatValue);
ulong __attribute__((overloadable))  __spirv_ConvertFToU_Rulong_rtp(half FloatValue);
ulong __attribute__((overloadable))  __spirv_ConvertFToU_Rulong_rtn(half FloatValue);
ulong __attribute__((overloadable))  __spirv_ConvertFToU_Rulong_sat(half FloatValue);
ulong __attribute__((overloadable))  __spirv_ConvertFToU_Rulong_sat_rte(half FloatValue);
ulong __attribute__((overloadable))  __spirv_ConvertFToU_Rulong_sat_rtz(half FloatValue);
ulong __attribute__((overloadable))  __spirv_ConvertFToU_Rulong_sat_rtp(half FloatValue);
ulong __attribute__((overloadable))  __spirv_ConvertFToU_Rulong_sat_rtn(half FloatValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar_rte(float FloatValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar_rtz(float FloatValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar_rtp(float FloatValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar_rtn(float FloatValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar_sat(float FloatValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar_sat_rte(float FloatValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar_sat_rtz(float FloatValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar_sat_rtp(float FloatValue);
uchar __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar_sat_rtn(float FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rte(float FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtz(float FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtp(float FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtn(float FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat(float FloatValue);
ushort __attribute__((overloadable))
__spirv_ConvertFToU_Rushort_sat_rte(float FloatValue);
ushort __attribute__((overloadable))
__spirv_ConvertFToU_Rushort_sat_rtz(float FloatValue);
ushort __attribute__((overloadable))
__spirv_ConvertFToU_Rushort_sat_rtp(float FloatValue);
ushort __attribute__((overloadable))
                                    __spirv_ConvertFToU_Rushort_sat_rtn(float FloatValue);
uint __attribute__((overloadable))  __spirv_ConvertFToU_Ruint_rte(float FloatValue);
uint __attribute__((overloadable))  __spirv_ConvertFToU_Ruint_rtz(float FloatValue);
uint __attribute__((overloadable))  __spirv_ConvertFToU_Ruint_rtp(float FloatValue);
uint __attribute__((overloadable))  __spirv_ConvertFToU_Ruint_rtn(float FloatValue);
uint __attribute__((overloadable))  __spirv_ConvertFToU_Ruint_sat(float FloatValue);
uint __attribute__((overloadable))  __spirv_ConvertFToU_Ruint_sat_rte(float FloatValue);
uint __attribute__((overloadable))  __spirv_ConvertFToU_Ruint_sat_rtz(float FloatValue);
uint __attribute__((overloadable))  __spirv_ConvertFToU_Ruint_sat_rtp(float FloatValue);
uint __attribute__((overloadable))  __spirv_ConvertFToU_Ruint_sat_rtn(float FloatValue);
ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rte(float FloatValue);
ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtz(float FloatValue);
ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtp(float FloatValue);
ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtn(float FloatValue);
ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat(float FloatValue);
ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rte(float FloatValue);
ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtz(float FloatValue);
ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtp(float FloatValue);
ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtn(float FloatValue);
#if defined(cl_khr_fp64)
uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rte(double FloatValue);
uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtz(double FloatValue);
uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtp(double FloatValue);
uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtn(double FloatValue);
uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat(double FloatValue);
uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rte(double FloatValue);
uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtz(double FloatValue);
uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtp(double FloatValue);
uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtn(double FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rte(double FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtz(double FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtp(double FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtn(double FloatValue);
ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat(double FloatValue);
ushort __attribute__((overloadable))
__spirv_ConvertFToU_Rushort_sat_rte(double FloatValue);
ushort __attribute__((overloadable))
__spirv_ConvertFToU_Rushort_sat_rtz(double FloatValue);
ushort __attribute__((overloadable))
__spirv_ConvertFToU_Rushort_sat_rtp(double FloatValue);
ushort __attribute__((overloadable))
                                   __spirv_ConvertFToU_Rushort_sat_rtn(double FloatValue);
uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rte(double FloatValue);
uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtz(double FloatValue);
uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtp(double FloatValue);
uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtn(double FloatValue);
uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat(double FloatValue);
uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rte(double FloatValue);
uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtz(double FloatValue);
uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtp(double FloatValue);
uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtn(double FloatValue);
ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rte(double FloatValue);
ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtz(double FloatValue);
ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtp(double FloatValue);
ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtn(double FloatValue);
ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat(double FloatValue);
ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rte(double FloatValue);
ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtz(double FloatValue);
ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtp(double FloatValue);
ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtn(double FloatValue);
#endif // defined(cl_khr_fp64)
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_rte(half FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_rtz(half FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_rtp(half FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_rtn(half FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_sat(half FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_sat_rte(half FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_sat_rtz(half FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_sat_rtp(half FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_sat_rtn(half FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rte(half FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtz(half FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtp(half FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtn(half FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat(half FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rte(half FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtz(half FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtp(half FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtn(half FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_rte(half FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_rtz(half FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_rtp(half FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_rtn(half FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_sat(half FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_sat_rte(half FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_sat_rtz(half FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_sat_rtp(half FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_sat_rtn(half FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_rte(half FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_rtz(half FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_rtp(half FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_rtn(half FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_sat(half FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_sat_rte(half FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_sat_rtz(half FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_sat_rtp(half FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_sat_rtn(half FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_rte(float FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_rtz(float FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_rtp(float FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_rtn(float FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_sat(float FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_sat_rte(float FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_sat_rtz(float FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_sat_rtp(float FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_sat_rtn(float FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rte(float FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtz(float FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtp(float FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtn(float FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat(float FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rte(float FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtz(float FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtp(float FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtn(float FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_rte(float FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_rtz(float FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_rtp(float FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_rtn(float FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_sat(float FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_sat_rte(float FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_sat_rtz(float FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_sat_rtp(float FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_sat_rtn(float FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_rte(float FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_rtz(float FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_rtp(float FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_rtn(float FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_sat(float FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_sat_rte(float FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_sat_rtz(float FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_sat_rtp(float FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_sat_rtn(float FloatValue);
#if defined(cl_khr_fp64)
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_rte(double FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_rtz(double FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_rtp(double FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_rtn(double FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_sat(double FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_sat_rte(double FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_sat_rtz(double FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_sat_rtp(double FloatValue);
char __attribute__((overloadable))  __spirv_ConvertFToS_Rchar_sat_rtn(double FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rte(double FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtz(double FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtp(double FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtn(double FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat(double FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rte(double FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtz(double FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtp(double FloatValue);
short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtn(double FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_rte(double FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_rtz(double FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_rtp(double FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_rtn(double FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_sat(double FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_sat_rte(double FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_sat_rtz(double FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_sat_rtp(double FloatValue);
int __attribute__((overloadable))   __spirv_ConvertFToS_Rint_sat_rtn(double FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_rte(double FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_rtz(double FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_rtp(double FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_rtn(double FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_sat(double FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_sat_rte(double FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_sat_rtz(double FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_sat_rtp(double FloatValue);
long __attribute__((overloadable))  __spirv_ConvertFToS_Rlong_sat_rtn(double FloatValue);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf_rte(char SignedValue);
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf_rtz(char SignedValue);
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf_rtp(char SignedValue);
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf_rtn(char SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rte(char SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtz(char SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtp(char SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtn(char SignedValue);
#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rte(char SignedValue);
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtz(char SignedValue);
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtp(char SignedValue);
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtn(char SignedValue);
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rte(short SignedValue);
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtz(short SignedValue);
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtp(short SignedValue);
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtn(short SignedValue);
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rte(int SignedValue);
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtz(int SignedValue);
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtp(int SignedValue);
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtn(int SignedValue);
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rte(long SignedValue);
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtz(long SignedValue);
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtp(long SignedValue);
double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtn(long SignedValue);
#endif // defined(cl_khr_fp64)
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf_rte(short SignedValue);
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf_rtz(short SignedValue);
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf_rtp(short SignedValue);
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf_rtn(short SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rte(short SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtz(short SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtp(short SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtn(short SignedValue);
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf_rte(int SignedValue);
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf_rtz(int SignedValue);
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf_rtp(int SignedValue);
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf_rtn(int SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rte(int SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtz(int SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtp(int SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtn(int SignedValue);
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf_rte(long SignedValue);
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf_rtz(long SignedValue);
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf_rtp(long SignedValue);
half __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf_rtn(long SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rte(long SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtz(long SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtp(long SignedValue);
float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtn(long SignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf_rte(uchar UnsignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf_rtz(uchar UnsignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf_rtp(uchar UnsignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf_rtn(uchar UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rte(uchar UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtz(uchar UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtp(uchar UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtn(uchar UnsignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf_rte(ushort UnsignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf_rtz(ushort UnsignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf_rtp(ushort UnsignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf_rtn(ushort UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rte(ushort UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtz(ushort UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtp(ushort UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtn(ushort UnsignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf_rte(uint UnsignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf_rtz(uint UnsignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf_rtp(uint UnsignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf_rtn(uint UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rte(uint UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtz(uint UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtp(uint UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtn(uint UnsignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf_rte(ulong UnsignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf_rtz(ulong UnsignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf_rtp(ulong UnsignedValue);
half __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf_rtn(ulong UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rte(ulong UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtz(ulong UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtp(ulong UnsignedValue);
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtn(ulong UnsignedValue);
#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rte(uchar UnsignedValue);
double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtz(uchar UnsignedValue);
double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtp(uchar UnsignedValue);
double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtn(uchar UnsignedValue);
double __attribute__((overloadable))
__spirv_ConvertUToF_Rdouble_rte(ushort UnsignedValue);
double __attribute__((overloadable))
__spirv_ConvertUToF_Rdouble_rtz(ushort UnsignedValue);
double __attribute__((overloadable))
__spirv_ConvertUToF_Rdouble_rtp(ushort UnsignedValue);
double __attribute__((overloadable))
__spirv_ConvertUToF_Rdouble_rtn(ushort UnsignedValue);
double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rte(uint UnsignedValue);
double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtz(uint UnsignedValue);
double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtp(uint UnsignedValue);
double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtn(uint UnsignedValue);
double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rte(ulong UnsignedValue);
double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtz(ulong UnsignedValue);
double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtp(ulong UnsignedValue);
double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtn(ulong UnsignedValue);
#endif // defined(cl_khr_fp64)
uchar __attribute__((overloadable))  __spirv_UConvert_Ruchar_sat(uchar UnsignedValue);
ushort __attribute__((overloadable)) __spirv_UConvert_Rushort_sat(uchar UnsignedValue);
uint __attribute__((overloadable))   __spirv_UConvert_Ruint_sat(uchar UnsignedValue);
ulong __attribute__((overloadable))  __spirv_UConvert_Rulong_sat(uchar UnsignedValue);
uchar __attribute__((overloadable))  __spirv_UConvert_Ruchar_sat(ushort UnsignedValue);
ushort __attribute__((overloadable)) __spirv_UConvert_Rushort_sat(ushort UnsignedValue);
uint __attribute__((overloadable))   __spirv_UConvert_Ruint_sat(ushort UnsignedValue);
ulong __attribute__((overloadable))  __spirv_UConvert_Rulong_sat(ushort UnsignedValue);
uchar __attribute__((overloadable))  __spirv_UConvert_Ruchar_sat(uint UnsignedValue);
ushort __attribute__((overloadable)) __spirv_UConvert_Rushort_sat(uint UnsignedValue);
uint __attribute__((overloadable))   __spirv_UConvert_Ruint_sat(uint UnsignedValue);
ulong __attribute__((overloadable))  __spirv_UConvert_Rulong_sat(uint UnsignedValue);
uchar __attribute__((overloadable))  __spirv_UConvert_Ruchar_sat(ulong UnsignedValue);
ushort __attribute__((overloadable)) __spirv_UConvert_Rushort_sat(ulong UnsignedValue);
uint __attribute__((overloadable))   __spirv_UConvert_Ruint_sat(ulong UnsignedValue);
ulong __attribute__((overloadable))  __spirv_UConvert_Rulong_sat(ulong UnsignedValue);
char __attribute__((overloadable))   __spirv_SConvert_Rchar_sat(char SignedValue);
short __attribute__((overloadable))  __spirv_SConvert_Rshort_sat(char SignedValue);
int __attribute__((overloadable))    __spirv_SConvert_Rint_sat(char SignedValue);
long __attribute__((overloadable))   __spirv_SConvert_Rlong_sat(char SignedValue);
char __attribute__((overloadable))   __spirv_SConvert_Rchar_sat(short SignedValue);
short __attribute__((overloadable))  __spirv_SConvert_Rshort_sat(short SignedValue);
int __attribute__((overloadable))    __spirv_SConvert_Rint_sat(short SignedValue);
long __attribute__((overloadable))   __spirv_SConvert_Rlong_sat(short SignedValue);
char __attribute__((overloadable))   __spirv_SConvert_Rchar_sat(int SignedValue);
short __attribute__((overloadable))  __spirv_SConvert_Rshort_sat(int SignedValue);
int __attribute__((overloadable))    __spirv_SConvert_Rint_sat(int SignedValue);
long __attribute__((overloadable))   __spirv_SConvert_Rlong_sat(int SignedValue);
char __attribute__((overloadable))   __spirv_SConvert_Rchar_sat(long SignedValue);
short __attribute__((overloadable))  __spirv_SConvert_Rshort_sat(long SignedValue);
int __attribute__((overloadable))    __spirv_SConvert_Rint_sat(long SignedValue);
long __attribute__((overloadable))   __spirv_SConvert_Rlong_sat(long SignedValue);
half __attribute__((overloadable))   __spirv_FConvert_Rhalf_rte(half FloatValue);
half __attribute__((overloadable))   __spirv_FConvert_Rhalf_rtz(half FloatValue);
half __attribute__((overloadable))   __spirv_FConvert_Rhalf_rtp(half FloatValue);
half __attribute__((overloadable))   __spirv_FConvert_Rhalf_rtn(half FloatValue);
float __attribute__((overloadable))  __spirv_FConvert_Rfloat_rte(half FloatValue);
float __attribute__((overloadable))  __spirv_FConvert_Rfloat_rtz(half FloatValue);
float __attribute__((overloadable))  __spirv_FConvert_Rfloat_rtp(half FloatValue);
float __attribute__((overloadable))  __spirv_FConvert_Rfloat_rtn(half FloatValue);
half __attribute__((overloadable))   __spirv_FConvert_Rhalf_rte(float FloatValue);
half __attribute__((overloadable))   __spirv_FConvert_Rhalf_rtz(float FloatValue);
half __attribute__((overloadable))   __spirv_FConvert_Rhalf_rtp(float FloatValue);
half __attribute__((overloadable))   __spirv_FConvert_Rhalf_rtn(float FloatValue);
float __attribute__((overloadable))  __spirv_FConvert_Rfloat_rte(float FloatValue);
float __attribute__((overloadable))  __spirv_FConvert_Rfloat_rtz(float FloatValue);
float __attribute__((overloadable))  __spirv_FConvert_Rfloat_rtp(float FloatValue);
float __attribute__((overloadable))  __spirv_FConvert_Rfloat_rtn(float FloatValue);
#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rte(half FloatValue);
double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtz(half FloatValue);
double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtp(half FloatValue);
double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtn(half FloatValue);
double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rte(float FloatValue);
double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtz(float FloatValue);
double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtp(float FloatValue);
double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtn(float FloatValue);
half __attribute__((overloadable))   __spirv_FConvert_Rhalf_rte(double FloatValue);
half __attribute__((overloadable))   __spirv_FConvert_Rhalf_rtz(double FloatValue);
half __attribute__((overloadable))   __spirv_FConvert_Rhalf_rtp(double FloatValue);
half __attribute__((overloadable))   __spirv_FConvert_Rhalf_rtn(double FloatValue);
float __attribute__((overloadable))  __spirv_FConvert_Rfloat_rte(double FloatValue);
float __attribute__((overloadable))  __spirv_FConvert_Rfloat_rtz(double FloatValue);
float __attribute__((overloadable))  __spirv_FConvert_Rfloat_rtp(double FloatValue);
float __attribute__((overloadable))  __spirv_FConvert_Rfloat_rtn(double FloatValue);
double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rte(double FloatValue);
double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtz(double FloatValue);
double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtp(double FloatValue);
double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtn(double FloatValue);
#endif // defined(cl_khr_fp64)
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2(char2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3(char3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4(char4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8(char8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16(char16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2_rte(char2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3_rte(char3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4_rte(char4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8_rte(char8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16_rte(char16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2_rtz(char2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3_rtz(char3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4_rtz(char4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8_rtz(char8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16_rtz(char16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2_rtp(char2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3_rtp(char3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4_rtp(char4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8_rtp(char8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16_rtp(char16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2_rtn(char2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3_rtn(char3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4_rtn(char4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8_rtn(char8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16_rtn(char16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2(short2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3(short3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4(short4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8(short8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16(short16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2_rte(short2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3_rte(short3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4_rte(short4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8_rte(short8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16_rte(short16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2_rtz(short2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3_rtz(short3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4_rtz(short4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8_rtz(short8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16_rtz(short16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2_rtp(short2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3_rtp(short3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4_rtp(short4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8_rtp(short8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16_rtp(short16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2_rtn(short2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3_rtn(short3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4_rtn(short4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8_rtn(short8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16_rtn(short16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2(int2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3(int3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4(int4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8(int8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16(int16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2_rte(int2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3_rte(int3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4_rte(int4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8_rte(int8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16_rte(int16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2_rtz(int2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3_rtz(int3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4_rtz(int4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8_rtz(int8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16_rtz(int16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2_rtp(int2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3_rtp(int3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4_rtp(int4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8_rtp(int8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16_rtp(int16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2_rtn(int2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3_rtn(int3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4_rtn(int4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8_rtn(int8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16_rtn(int16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2(long2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3(long3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4(long4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8(long8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16(long16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2_rte(long2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3_rte(long3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4_rte(long4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8_rte(long8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16_rte(long16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2_rtz(long2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3_rtz(long3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4_rtz(long4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8_rtz(long8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16_rtz(long16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2_rtp(long2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3_rtp(long3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4_rtp(long4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8_rtp(long8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16_rtp(long16 x);
half2 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf2_rtn(long2 x);
half3 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf3_rtn(long3 x);
half4 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf4_rtn(long4 x);
half8 __attribute__((overloadable))   __spirv_ConvertSToF_Rhalf8_rtn(long8 x);
half16 __attribute__((overloadable))  __spirv_ConvertSToF_Rhalf16_rtn(long16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2(char2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3(char3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4(char4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8(char8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16(char16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2_rte(char2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3_rte(char3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4_rte(char4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8_rte(char8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16_rte(char16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2_rtz(char2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3_rtz(char3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4_rtz(char4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8_rtz(char8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16_rtz(char16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2_rtp(char2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3_rtp(char3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4_rtp(char4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8_rtp(char8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16_rtp(char16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2_rtn(char2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3_rtn(char3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4_rtn(char4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8_rtn(char8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16_rtn(char16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2(short2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3(short3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4(short4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8(short8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16(short16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2_rte(short2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3_rte(short3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4_rte(short4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8_rte(short8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16_rte(short16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2_rtz(short2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3_rtz(short3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4_rtz(short4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8_rtz(short8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16_rtz(short16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2_rtp(short2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3_rtp(short3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4_rtp(short4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8_rtp(short8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16_rtp(short16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2_rtn(short2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3_rtn(short3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4_rtn(short4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8_rtn(short8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16_rtn(short16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2(int2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3(int3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4(int4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8(int8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16(int16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2_rte(int2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3_rte(int3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4_rte(int4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8_rte(int8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16_rte(int16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2_rtz(int2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3_rtz(int3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4_rtz(int4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8_rtz(int8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16_rtz(int16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2_rtp(int2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3_rtp(int3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4_rtp(int4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8_rtp(int8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16_rtp(int16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2_rtn(int2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3_rtn(int3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4_rtn(int4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8_rtn(int8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16_rtn(int16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2(long2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3(long3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4(long4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8(long8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16(long16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2_rte(long2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3_rte(long3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4_rte(long4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8_rte(long8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16_rte(long16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2_rtz(long2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3_rtz(long3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4_rtz(long4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8_rtz(long8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16_rtz(long16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2_rtp(long2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3_rtp(long3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4_rtp(long4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8_rtp(long8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16_rtp(long16 x);
float2 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat2_rtn(long2 x);
float3 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat3_rtn(long3 x);
float4 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat4_rtn(long4 x);
float8 __attribute__((overloadable))  __spirv_ConvertSToF_Rfloat8_rtn(long8 x);
float16 __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat16_rtn(long16 x);
#if defined(cl_khr_fp64)
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2(char2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3(char3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4(char4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8(char8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16(char16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2_rte(char2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3_rte(char3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4_rte(char4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8_rte(char8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16_rte(char16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2_rtz(char2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3_rtz(char3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4_rtz(char4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8_rtz(char8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16_rtz(char16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2_rtp(char2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3_rtp(char3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4_rtp(char4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8_rtp(char8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16_rtp(char16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2_rtn(char2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3_rtn(char3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4_rtn(char4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8_rtn(char8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16_rtn(char16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2(short2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3(short3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4(short4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8(short8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16(short16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2_rte(short2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3_rte(short3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4_rte(short4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8_rte(short8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16_rte(short16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2_rtz(short2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3_rtz(short3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4_rtz(short4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8_rtz(short8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16_rtz(short16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2_rtp(short2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3_rtp(short3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4_rtp(short4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8_rtp(short8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16_rtp(short16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2_rtn(short2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3_rtn(short3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4_rtn(short4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8_rtn(short8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16_rtn(short16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2(int2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3(int3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4(int4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8(int8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16(int16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2_rte(int2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3_rte(int3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4_rte(int4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8_rte(int8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16_rte(int16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2_rtz(int2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3_rtz(int3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4_rtz(int4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8_rtz(int8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16_rtz(int16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2_rtp(int2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3_rtp(int3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4_rtp(int4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8_rtp(int8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16_rtp(int16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2_rtn(int2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3_rtn(int3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4_rtn(int4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8_rtn(int8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16_rtn(int16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2(long2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3(long3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4(long4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8(long8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16(long16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2_rte(long2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3_rte(long3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4_rte(long4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8_rte(long8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16_rte(long16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2_rtz(long2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3_rtz(long3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4_rtz(long4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8_rtz(long8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16_rtz(long16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2_rtp(long2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3_rtp(long3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4_rtp(long4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8_rtp(long8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16_rtp(long16 x);
double2 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble2_rtn(long2 x);
double3 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble3_rtn(long3 x);
double4 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble4_rtn(long4 x);
double8 __attribute__((overloadable))  __spirv_ConvertSToF_Rdouble8_rtn(long8 x);
double16 __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble16_rtn(long16 x);
#endif // defined(cl_khr_fp64)
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2(uchar2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3(uchar3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4(uchar4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8(uchar8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16(uchar16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2_rte(uchar2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3_rte(uchar3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4_rte(uchar4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8_rte(uchar8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16_rte(uchar16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2_rtz(uchar2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3_rtz(uchar3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4_rtz(uchar4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8_rtz(uchar8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16_rtz(uchar16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2_rtp(uchar2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3_rtp(uchar3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4_rtp(uchar4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8_rtp(uchar8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16_rtp(uchar16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2_rtn(uchar2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3_rtn(uchar3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4_rtn(uchar4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8_rtn(uchar8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16_rtn(uchar16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2(ushort2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3(ushort3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4(ushort4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8(ushort8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16(ushort16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2_rte(ushort2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3_rte(ushort3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4_rte(ushort4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8_rte(ushort8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16_rte(ushort16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2_rtz(ushort2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3_rtz(ushort3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4_rtz(ushort4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8_rtz(ushort8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16_rtz(ushort16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2_rtp(ushort2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3_rtp(ushort3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4_rtp(ushort4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8_rtp(ushort8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16_rtp(ushort16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2_rtn(ushort2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3_rtn(ushort3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4_rtn(ushort4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8_rtn(ushort8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16_rtn(ushort16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2(uint2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3(uint3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4(uint4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8(uint8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16(uint16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2_rte(uint2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3_rte(uint3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4_rte(uint4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8_rte(uint8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16_rte(uint16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2_rtz(uint2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3_rtz(uint3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4_rtz(uint4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8_rtz(uint8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16_rtz(uint16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2_rtp(uint2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3_rtp(uint3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4_rtp(uint4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8_rtp(uint8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16_rtp(uint16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2_rtn(uint2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3_rtn(uint3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4_rtn(uint4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8_rtn(uint8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16_rtn(uint16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2(ulong2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3(ulong3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4(ulong4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8(ulong8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16(ulong16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2_rte(ulong2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3_rte(ulong3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4_rte(ulong4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8_rte(ulong8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16_rte(ulong16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2_rtz(ulong2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3_rtz(ulong3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4_rtz(ulong4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8_rtz(ulong8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16_rtz(ulong16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2_rtp(ulong2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3_rtp(ulong3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4_rtp(ulong4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8_rtp(ulong8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16_rtp(ulong16 x);
half2 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf2_rtn(ulong2 x);
half3 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf3_rtn(ulong3 x);
half4 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf4_rtn(ulong4 x);
half8 __attribute__((overloadable))   __spirv_ConvertUToF_Rhalf8_rtn(ulong8 x);
half16 __attribute__((overloadable))  __spirv_ConvertUToF_Rhalf16_rtn(ulong16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2(uchar2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3(uchar3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4(uchar4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8(uchar8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16(uchar16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2_rte(uchar2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3_rte(uchar3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4_rte(uchar4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8_rte(uchar8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16_rte(uchar16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2_rtz(uchar2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3_rtz(uchar3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4_rtz(uchar4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8_rtz(uchar8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16_rtz(uchar16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2_rtp(uchar2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3_rtp(uchar3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4_rtp(uchar4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8_rtp(uchar8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16_rtp(uchar16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2_rtn(uchar2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3_rtn(uchar3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4_rtn(uchar4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8_rtn(uchar8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16_rtn(uchar16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2(ushort2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3(ushort3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4(ushort4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8(ushort8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16(ushort16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2_rte(ushort2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3_rte(ushort3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4_rte(ushort4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8_rte(ushort8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16_rte(ushort16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2_rtz(ushort2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3_rtz(ushort3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4_rtz(ushort4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8_rtz(ushort8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16_rtz(ushort16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2_rtp(ushort2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3_rtp(ushort3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4_rtp(ushort4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8_rtp(ushort8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16_rtp(ushort16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2_rtn(ushort2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3_rtn(ushort3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4_rtn(ushort4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8_rtn(ushort8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16_rtn(ushort16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2(uint2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3(uint3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4(uint4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8(uint8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16(uint16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2_rte(uint2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3_rte(uint3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4_rte(uint4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8_rte(uint8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16_rte(uint16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2_rtz(uint2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3_rtz(uint3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4_rtz(uint4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8_rtz(uint8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16_rtz(uint16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2_rtp(uint2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3_rtp(uint3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4_rtp(uint4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8_rtp(uint8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16_rtp(uint16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2_rtn(uint2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3_rtn(uint3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4_rtn(uint4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8_rtn(uint8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16_rtn(uint16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2(ulong2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3(ulong3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4(ulong4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8(ulong8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16(ulong16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2_rte(ulong2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3_rte(ulong3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4_rte(ulong4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8_rte(ulong8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16_rte(ulong16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2_rtz(ulong2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3_rtz(ulong3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4_rtz(ulong4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8_rtz(ulong8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16_rtz(ulong16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2_rtp(ulong2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3_rtp(ulong3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4_rtp(ulong4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8_rtp(ulong8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16_rtp(ulong16 x);
float2 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat2_rtn(ulong2 x);
float3 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat3_rtn(ulong3 x);
float4 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat4_rtn(ulong4 x);
float8 __attribute__((overloadable))  __spirv_ConvertUToF_Rfloat8_rtn(ulong8 x);
float16 __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat16_rtn(ulong16 x);
#if defined(cl_khr_fp64)
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2(uchar2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3(uchar3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4(uchar4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8(uchar8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16(uchar16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2_rte(uchar2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3_rte(uchar3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4_rte(uchar4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8_rte(uchar8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16_rte(uchar16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2_rtz(uchar2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3_rtz(uchar3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4_rtz(uchar4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8_rtz(uchar8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16_rtz(uchar16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2_rtp(uchar2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3_rtp(uchar3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4_rtp(uchar4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8_rtp(uchar8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16_rtp(uchar16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2_rtn(uchar2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3_rtn(uchar3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4_rtn(uchar4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8_rtn(uchar8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16_rtn(uchar16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2(ushort2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3(ushort3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4(ushort4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8(ushort8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16(ushort16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2_rte(ushort2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3_rte(ushort3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4_rte(ushort4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8_rte(ushort8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16_rte(ushort16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2_rtz(ushort2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3_rtz(ushort3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4_rtz(ushort4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8_rtz(ushort8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16_rtz(ushort16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2_rtp(ushort2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3_rtp(ushort3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4_rtp(ushort4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8_rtp(ushort8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16_rtp(ushort16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2_rtn(ushort2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3_rtn(ushort3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4_rtn(ushort4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8_rtn(ushort8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16_rtn(ushort16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2(uint2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3(uint3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4(uint4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8(uint8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16(uint16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2_rte(uint2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3_rte(uint3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4_rte(uint4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8_rte(uint8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16_rte(uint16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2_rtz(uint2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3_rtz(uint3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4_rtz(uint4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8_rtz(uint8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16_rtz(uint16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2_rtp(uint2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3_rtp(uint3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4_rtp(uint4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8_rtp(uint8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16_rtp(uint16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2_rtn(uint2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3_rtn(uint3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4_rtn(uint4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8_rtn(uint8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16_rtn(uint16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2(ulong2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3(ulong3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4(ulong4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8(ulong8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16(ulong16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2_rte(ulong2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3_rte(ulong3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4_rte(ulong4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8_rte(ulong8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16_rte(ulong16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2_rtz(ulong2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3_rtz(ulong3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4_rtz(ulong4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8_rtz(ulong8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16_rtz(ulong16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2_rtp(ulong2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3_rtp(ulong3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4_rtp(ulong4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8_rtp(ulong8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16_rtp(ulong16 x);
double2 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble2_rtn(ulong2 x);
double3 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble3_rtn(ulong3 x);
double4 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble4_rtn(ulong4 x);
double8 __attribute__((overloadable))  __spirv_ConvertUToF_Rdouble8_rtn(ulong8 x);
double16 __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble16_rtn(ulong16 x);
#endif // defined(cl_khr_fp64)
half2 __attribute__((overloadable))  __spirv_FConvert_Rhalf2(float2 x);
half3 __attribute__((overloadable))  __spirv_FConvert_Rhalf3(float3 x);
half4 __attribute__((overloadable))  __spirv_FConvert_Rhalf4(float4 x);
half8 __attribute__((overloadable))  __spirv_FConvert_Rhalf8(float8 x);
half16 __attribute__((overloadable)) __spirv_FConvert_Rhalf16(float16 x);
half2 __attribute__((overloadable))  __spirv_FConvert_Rhalf2_rte(float2 x);
half3 __attribute__((overloadable))  __spirv_FConvert_Rhalf3_rte(float3 x);
half4 __attribute__((overloadable))  __spirv_FConvert_Rhalf4_rte(float4 x);
half8 __attribute__((overloadable))  __spirv_FConvert_Rhalf8_rte(float8 x);
half16 __attribute__((overloadable)) __spirv_FConvert_Rhalf16_rte(float16 x);
half2 __attribute__((overloadable))  __spirv_FConvert_Rhalf2_rtz(float2 x);
half3 __attribute__((overloadable))  __spirv_FConvert_Rhalf3_rtz(float3 x);
half4 __attribute__((overloadable))  __spirv_FConvert_Rhalf4_rtz(float4 x);
half8 __attribute__((overloadable))  __spirv_FConvert_Rhalf8_rtz(float8 x);
half16 __attribute__((overloadable)) __spirv_FConvert_Rhalf16_rtz(float16 x);
half2 __attribute__((overloadable))  __spirv_FConvert_Rhalf2_rtp(float2 x);
half3 __attribute__((overloadable))  __spirv_FConvert_Rhalf3_rtp(float3 x);
half4 __attribute__((overloadable))  __spirv_FConvert_Rhalf4_rtp(float4 x);
half8 __attribute__((overloadable))  __spirv_FConvert_Rhalf8_rtp(float8 x);
half16 __attribute__((overloadable)) __spirv_FConvert_Rhalf16_rtp(float16 x);
half2 __attribute__((overloadable))  __spirv_FConvert_Rhalf2_rtn(float2 x);
half3 __attribute__((overloadable))  __spirv_FConvert_Rhalf3_rtn(float3 x);
half4 __attribute__((overloadable))  __spirv_FConvert_Rhalf4_rtn(float4 x);
half8 __attribute__((overloadable))  __spirv_FConvert_Rhalf8_rtn(float8 x);
half16 __attribute__((overloadable)) __spirv_FConvert_Rhalf16_rtn(float16 x);
half2 __attribute__((overloadable))  __spirv_FConvert_Rhalf2(half2 x);
half3 __attribute__((overloadable))  __spirv_FConvert_Rhalf3(half3 x);
half4 __attribute__((overloadable))  __spirv_FConvert_Rhalf4(half4 x);
half8 __attribute__((overloadable))  __spirv_FConvert_Rhalf8(half8 x);
half16 __attribute__((overloadable)) __spirv_FConvert_Rhalf16(half16 x);
half2 __attribute__((overloadable))  __spirv_FConvert_Rhalf2_rte(half2 x);
half3 __attribute__((overloadable))  __spirv_FConvert_Rhalf3_rte(half3 x);
half4 __attribute__((overloadable))  __spirv_FConvert_Rhalf4_rte(half4 x);
half8 __attribute__((overloadable))  __spirv_FConvert_Rhalf8_rte(half8 x);
half16 __attribute__((overloadable)) __spirv_FConvert_Rhalf16_rte(half16 x);
half2 __attribute__((overloadable))  __spirv_FConvert_Rhalf2_rtz(half2 x);
half3 __attribute__((overloadable))  __spirv_FConvert_Rhalf3_rtz(half3 x);
half4 __attribute__((overloadable))  __spirv_FConvert_Rhalf4_rtz(half4 x);
half8 __attribute__((overloadable))  __spirv_FConvert_Rhalf8_rtz(half8 x);
half16 __attribute__((overloadable)) __spirv_FConvert_Rhalf16_rtz(half16 x);
half2 __attribute__((overloadable))  __spirv_FConvert_Rhalf2_rtp(half2 x);
half3 __attribute__((overloadable))  __spirv_FConvert_Rhalf3_rtp(half3 x);
half4 __attribute__((overloadable))  __spirv_FConvert_Rhalf4_rtp(half4 x);
half8 __attribute__((overloadable))  __spirv_FConvert_Rhalf8_rtp(half8 x);
half16 __attribute__((overloadable)) __spirv_FConvert_Rhalf16_rtp(half16 x);
half2 __attribute__((overloadable))  __spirv_FConvert_Rhalf2_rtn(half2 x);
half3 __attribute__((overloadable))  __spirv_FConvert_Rhalf3_rtn(half3 x);
half4 __attribute__((overloadable))  __spirv_FConvert_Rhalf4_rtn(half4 x);
half8 __attribute__((overloadable))  __spirv_FConvert_Rhalf8_rtn(half8 x);
half16 __attribute__((overloadable)) __spirv_FConvert_Rhalf16_rtn(half16 x);
#if defined(cl_khr_fp64)
half2 __attribute__((overloadable))  __spirv_FConvert_Rhalf2(double2 x);
half3 __attribute__((overloadable))  __spirv_FConvert_Rhalf3(double3 x);
half4 __attribute__((overloadable))  __spirv_FConvert_Rhalf4(double4 x);
half8 __attribute__((overloadable))  __spirv_FConvert_Rhalf8(double8 x);
half16 __attribute__((overloadable)) __spirv_FConvert_Rhalf16(double16 x);
half2 __attribute__((overloadable))  __spirv_FConvert_Rhalf2_rte(double2 x);
half3 __attribute__((overloadable))  __spirv_FConvert_Rhalf3_rte(double3 x);
half4 __attribute__((overloadable))  __spirv_FConvert_Rhalf4_rte(double4 x);
half8 __attribute__((overloadable))  __spirv_FConvert_Rhalf8_rte(double8 x);
half16 __attribute__((overloadable)) __spirv_FConvert_Rhalf16_rte(double16 x);
half2 __attribute__((overloadable))  __spirv_FConvert_Rhalf2_rtz(double2 x);
half3 __attribute__((overloadable))  __spirv_FConvert_Rhalf3_rtz(double3 x);
half4 __attribute__((overloadable))  __spirv_FConvert_Rhalf4_rtz(double4 x);
half8 __attribute__((overloadable))  __spirv_FConvert_Rhalf8_rtz(double8 x);
half16 __attribute__((overloadable)) __spirv_FConvert_Rhalf16_rtz(double16 x);
half2 __attribute__((overloadable))  __spirv_FConvert_Rhalf2_rtp(double2 x);
half3 __attribute__((overloadable))  __spirv_FConvert_Rhalf3_rtp(double3 x);
half4 __attribute__((overloadable))  __spirv_FConvert_Rhalf4_rtp(double4 x);
half8 __attribute__((overloadable))  __spirv_FConvert_Rhalf8_rtp(double8 x);
half16 __attribute__((overloadable)) __spirv_FConvert_Rhalf16_rtp(double16 x);
half2 __attribute__((overloadable))  __spirv_FConvert_Rhalf2_rtn(double2 x);
half3 __attribute__((overloadable))  __spirv_FConvert_Rhalf3_rtn(double3 x);
half4 __attribute__((overloadable))  __spirv_FConvert_Rhalf4_rtn(double4 x);
half8 __attribute__((overloadable))  __spirv_FConvert_Rhalf8_rtn(double8 x);
half16 __attribute__((overloadable)) __spirv_FConvert_Rhalf16_rtn(double16 x);
#endif // defined(cl_khr_fp64)
float2 __attribute__((overloadable))  __spirv_FConvert_Rfloat2(float2 x);
float3 __attribute__((overloadable))  __spirv_FConvert_Rfloat3(float3 x);
float4 __attribute__((overloadable))  __spirv_FConvert_Rfloat4(float4 x);
float8 __attribute__((overloadable))  __spirv_FConvert_Rfloat8(float8 x);
float16 __attribute__((overloadable)) __spirv_FConvert_Rfloat16(float16 x);
float2 __attribute__((overloadable))  __spirv_FConvert_Rfloat2_rte(float2 x);
float3 __attribute__((overloadable))  __spirv_FConvert_Rfloat3_rte(float3 x);
float4 __attribute__((overloadable))  __spirv_FConvert_Rfloat4_rte(float4 x);
float8 __attribute__((overloadable))  __spirv_FConvert_Rfloat8_rte(float8 x);
float16 __attribute__((overloadable)) __spirv_FConvert_Rfloat16_rte(float16 x);
float2 __attribute__((overloadable))  __spirv_FConvert_Rfloat2_rtz(float2 x);
float3 __attribute__((overloadable))  __spirv_FConvert_Rfloat3_rtz(float3 x);
float4 __attribute__((overloadable))  __spirv_FConvert_Rfloat4_rtz(float4 x);
float8 __attribute__((overloadable))  __spirv_FConvert_Rfloat8_rtz(float8 x);
float16 __attribute__((overloadable)) __spirv_FConvert_Rfloat16_rtz(float16 x);
float2 __attribute__((overloadable))  __spirv_FConvert_Rfloat2_rtp(float2 x);
float3 __attribute__((overloadable))  __spirv_FConvert_Rfloat3_rtp(float3 x);
float4 __attribute__((overloadable))  __spirv_FConvert_Rfloat4_rtp(float4 x);
float8 __attribute__((overloadable))  __spirv_FConvert_Rfloat8_rtp(float8 x);
float16 __attribute__((overloadable)) __spirv_FConvert_Rfloat16_rtp(float16 x);
float2 __attribute__((overloadable))  __spirv_FConvert_Rfloat2_rtn(float2 x);
float3 __attribute__((overloadable))  __spirv_FConvert_Rfloat3_rtn(float3 x);
float4 __attribute__((overloadable))  __spirv_FConvert_Rfloat4_rtn(float4 x);
float8 __attribute__((overloadable))  __spirv_FConvert_Rfloat8_rtn(float8 x);
float16 __attribute__((overloadable)) __spirv_FConvert_Rfloat16_rtn(float16 x);
float2 __attribute__((overloadable))  __spirv_FConvert_Rfloat2(half2 x);
float3 __attribute__((overloadable))  __spirv_FConvert_Rfloat3(half3 x);
float4 __attribute__((overloadable))  __spirv_FConvert_Rfloat4(half4 x);
float8 __attribute__((overloadable))  __spirv_FConvert_Rfloat8(half8 x);
float16 __attribute__((overloadable)) __spirv_FConvert_Rfloat16(half16 x);
float2 __attribute__((overloadable))  __spirv_FConvert_Rfloat2_rte(half2 x);
float3 __attribute__((overloadable))  __spirv_FConvert_Rfloat3_rte(half3 x);
float4 __attribute__((overloadable))  __spirv_FConvert_Rfloat4_rte(half4 x);
float8 __attribute__((overloadable))  __spirv_FConvert_Rfloat8_rte(half8 x);
float16 __attribute__((overloadable)) __spirv_FConvert_Rfloat16_rte(half16 x);
float2 __attribute__((overloadable))  __spirv_FConvert_Rfloat2_rtz(half2 x);
float3 __attribute__((overloadable))  __spirv_FConvert_Rfloat3_rtz(half3 x);
float4 __attribute__((overloadable))  __spirv_FConvert_Rfloat4_rtz(half4 x);
float8 __attribute__((overloadable))  __spirv_FConvert_Rfloat8_rtz(half8 x);
float16 __attribute__((overloadable)) __spirv_FConvert_Rfloat16_rtz(half16 x);
float2 __attribute__((overloadable))  __spirv_FConvert_Rfloat2_rtp(half2 x);
float3 __attribute__((overloadable))  __spirv_FConvert_Rfloat3_rtp(half3 x);
float4 __attribute__((overloadable))  __spirv_FConvert_Rfloat4_rtp(half4 x);
float8 __attribute__((overloadable))  __spirv_FConvert_Rfloat8_rtp(half8 x);
float16 __attribute__((overloadable)) __spirv_FConvert_Rfloat16_rtp(half16 x);
float2 __attribute__((overloadable))  __spirv_FConvert_Rfloat2_rtn(half2 x);
float3 __attribute__((overloadable))  __spirv_FConvert_Rfloat3_rtn(half3 x);
float4 __attribute__((overloadable))  __spirv_FConvert_Rfloat4_rtn(half4 x);
float8 __attribute__((overloadable))  __spirv_FConvert_Rfloat8_rtn(half8 x);
float16 __attribute__((overloadable)) __spirv_FConvert_Rfloat16_rtn(half16 x);
#if defined(cl_khr_fp64)
float2 __attribute__((overloadable))   __spirv_FConvert_Rfloat2(double2 x);
float3 __attribute__((overloadable))   __spirv_FConvert_Rfloat3(double3 x);
float4 __attribute__((overloadable))   __spirv_FConvert_Rfloat4(double4 x);
float8 __attribute__((overloadable))   __spirv_FConvert_Rfloat8(double8 x);
float16 __attribute__((overloadable))  __spirv_FConvert_Rfloat16(double16 x);
float2 __attribute__((overloadable))   __spirv_FConvert_Rfloat2_rte(double2 x);
float3 __attribute__((overloadable))   __spirv_FConvert_Rfloat3_rte(double3 x);
float4 __attribute__((overloadable))   __spirv_FConvert_Rfloat4_rte(double4 x);
float8 __attribute__((overloadable))   __spirv_FConvert_Rfloat8_rte(double8 x);
float16 __attribute__((overloadable))  __spirv_FConvert_Rfloat16_rte(double16 x);
float2 __attribute__((overloadable))   __spirv_FConvert_Rfloat2_rtz(double2 x);
float3 __attribute__((overloadable))   __spirv_FConvert_Rfloat3_rtz(double3 x);
float4 __attribute__((overloadable))   __spirv_FConvert_Rfloat4_rtz(double4 x);
float8 __attribute__((overloadable))   __spirv_FConvert_Rfloat8_rtz(double8 x);
float16 __attribute__((overloadable))  __spirv_FConvert_Rfloat16_rtz(double16 x);
float2 __attribute__((overloadable))   __spirv_FConvert_Rfloat2_rtp(double2 x);
float3 __attribute__((overloadable))   __spirv_FConvert_Rfloat3_rtp(double3 x);
float4 __attribute__((overloadable))   __spirv_FConvert_Rfloat4_rtp(double4 x);
float8 __attribute__((overloadable))   __spirv_FConvert_Rfloat8_rtp(double8 x);
float16 __attribute__((overloadable))  __spirv_FConvert_Rfloat16_rtp(double16 x);
float2 __attribute__((overloadable))   __spirv_FConvert_Rfloat2_rtn(double2 x);
float3 __attribute__((overloadable))   __spirv_FConvert_Rfloat3_rtn(double3 x);
float4 __attribute__((overloadable))   __spirv_FConvert_Rfloat4_rtn(double4 x);
float8 __attribute__((overloadable))   __spirv_FConvert_Rfloat8_rtn(double8 x);
float16 __attribute__((overloadable))  __spirv_FConvert_Rfloat16_rtn(double16 x);
double2 __attribute__((overloadable))  __spirv_FConvert_Rdouble2(float2 x);
double3 __attribute__((overloadable))  __spirv_FConvert_Rdouble3(float3 x);
double4 __attribute__((overloadable))  __spirv_FConvert_Rdouble4(float4 x);
double8 __attribute__((overloadable))  __spirv_FConvert_Rdouble8(float8 x);
double16 __attribute__((overloadable)) __spirv_FConvert_Rdouble16(float16 x);
double2 __attribute__((overloadable))  __spirv_FConvert_Rdouble2_rte(float2 x);
double3 __attribute__((overloadable))  __spirv_FConvert_Rdouble3_rte(float3 x);
double4 __attribute__((overloadable))  __spirv_FConvert_Rdouble4_rte(float4 x);
double8 __attribute__((overloadable))  __spirv_FConvert_Rdouble8_rte(float8 x);
double16 __attribute__((overloadable)) __spirv_FConvert_Rdouble16_rte(float16 x);
double2 __attribute__((overloadable))  __spirv_FConvert_Rdouble2_rtz(float2 x);
double3 __attribute__((overloadable))  __spirv_FConvert_Rdouble3_rtz(float3 x);
double4 __attribute__((overloadable))  __spirv_FConvert_Rdouble4_rtz(float4 x);
double8 __attribute__((overloadable))  __spirv_FConvert_Rdouble8_rtz(float8 x);
double16 __attribute__((overloadable)) __spirv_FConvert_Rdouble16_rtz(float16 x);
double2 __attribute__((overloadable))  __spirv_FConvert_Rdouble2_rtp(float2 x);
double3 __attribute__((overloadable))  __spirv_FConvert_Rdouble3_rtp(float3 x);
double4 __attribute__((overloadable))  __spirv_FConvert_Rdouble4_rtp(float4 x);
double8 __attribute__((overloadable))  __spirv_FConvert_Rdouble8_rtp(float8 x);
double16 __attribute__((overloadable)) __spirv_FConvert_Rdouble16_rtp(float16 x);
double2 __attribute__((overloadable))  __spirv_FConvert_Rdouble2_rtn(float2 x);
double3 __attribute__((overloadable))  __spirv_FConvert_Rdouble3_rtn(float3 x);
double4 __attribute__((overloadable))  __spirv_FConvert_Rdouble4_rtn(float4 x);
double8 __attribute__((overloadable))  __spirv_FConvert_Rdouble8_rtn(float8 x);
double16 __attribute__((overloadable)) __spirv_FConvert_Rdouble16_rtn(float16 x);
double2 __attribute__((overloadable))  __spirv_FConvert_Rdouble2(half2 x);
double3 __attribute__((overloadable))  __spirv_FConvert_Rdouble3(half3 x);
double4 __attribute__((overloadable))  __spirv_FConvert_Rdouble4(half4 x);
double8 __attribute__((overloadable))  __spirv_FConvert_Rdouble8(half8 x);
double16 __attribute__((overloadable)) __spirv_FConvert_Rdouble16(half16 x);
double2 __attribute__((overloadable))  __spirv_FConvert_Rdouble2_rte(half2 x);
double3 __attribute__((overloadable))  __spirv_FConvert_Rdouble3_rte(half3 x);
double4 __attribute__((overloadable))  __spirv_FConvert_Rdouble4_rte(half4 x);
double8 __attribute__((overloadable))  __spirv_FConvert_Rdouble8_rte(half8 x);
double16 __attribute__((overloadable)) __spirv_FConvert_Rdouble16_rte(half16 x);
double2 __attribute__((overloadable))  __spirv_FConvert_Rdouble2_rtz(half2 x);
double3 __attribute__((overloadable))  __spirv_FConvert_Rdouble3_rtz(half3 x);
double4 __attribute__((overloadable))  __spirv_FConvert_Rdouble4_rtz(half4 x);
double8 __attribute__((overloadable))  __spirv_FConvert_Rdouble8_rtz(half8 x);
double16 __attribute__((overloadable)) __spirv_FConvert_Rdouble16_rtz(half16 x);
double2 __attribute__((overloadable))  __spirv_FConvert_Rdouble2_rtp(half2 x);
double3 __attribute__((overloadable))  __spirv_FConvert_Rdouble3_rtp(half3 x);
double4 __attribute__((overloadable))  __spirv_FConvert_Rdouble4_rtp(half4 x);
double8 __attribute__((overloadable))  __spirv_FConvert_Rdouble8_rtp(half8 x);
double16 __attribute__((overloadable)) __spirv_FConvert_Rdouble16_rtp(half16 x);
double2 __attribute__((overloadable))  __spirv_FConvert_Rdouble2_rtn(half2 x);
double3 __attribute__((overloadable))  __spirv_FConvert_Rdouble3_rtn(half3 x);
double4 __attribute__((overloadable))  __spirv_FConvert_Rdouble4_rtn(half4 x);
double8 __attribute__((overloadable))  __spirv_FConvert_Rdouble8_rtn(half8 x);
double16 __attribute__((overloadable)) __spirv_FConvert_Rdouble16_rtn(half16 x);
double2 __attribute__((overloadable))  __spirv_FConvert_Rdouble2(double2 x);
double3 __attribute__((overloadable))  __spirv_FConvert_Rdouble3(double3 x);
double4 __attribute__((overloadable))  __spirv_FConvert_Rdouble4(double4 x);
double8 __attribute__((overloadable))  __spirv_FConvert_Rdouble8(double8 x);
double16 __attribute__((overloadable)) __spirv_FConvert_Rdouble16(double16 x);
double2 __attribute__((overloadable))  __spirv_FConvert_Rdouble2_rte(double2 x);
double3 __attribute__((overloadable))  __spirv_FConvert_Rdouble3_rte(double3 x);
double4 __attribute__((overloadable))  __spirv_FConvert_Rdouble4_rte(double4 x);
double8 __attribute__((overloadable))  __spirv_FConvert_Rdouble8_rte(double8 x);
double16 __attribute__((overloadable)) __spirv_FConvert_Rdouble16_rte(double16 x);
double2 __attribute__((overloadable))  __spirv_FConvert_Rdouble2_rtz(double2 x);
double3 __attribute__((overloadable))  __spirv_FConvert_Rdouble3_rtz(double3 x);
double4 __attribute__((overloadable))  __spirv_FConvert_Rdouble4_rtz(double4 x);
double8 __attribute__((overloadable))  __spirv_FConvert_Rdouble8_rtz(double8 x);
double16 __attribute__((overloadable)) __spirv_FConvert_Rdouble16_rtz(double16 x);
double2 __attribute__((overloadable))  __spirv_FConvert_Rdouble2_rtp(double2 x);
double3 __attribute__((overloadable))  __spirv_FConvert_Rdouble3_rtp(double3 x);
double4 __attribute__((overloadable))  __spirv_FConvert_Rdouble4_rtp(double4 x);
double8 __attribute__((overloadable))  __spirv_FConvert_Rdouble8_rtp(double8 x);
double16 __attribute__((overloadable)) __spirv_FConvert_Rdouble16_rtp(double16 x);
double2 __attribute__((overloadable))  __spirv_FConvert_Rdouble2_rtn(double2 x);
double3 __attribute__((overloadable))  __spirv_FConvert_Rdouble3_rtn(double3 x);
double4 __attribute__((overloadable))  __spirv_FConvert_Rdouble4_rtn(double4 x);
double8 __attribute__((overloadable))  __spirv_FConvert_Rdouble8_rtn(double8 x);
double16 __attribute__((overloadable)) __spirv_FConvert_Rdouble16_rtn(double16 x);
#endif // defined(cl_khr_fp64)
char2 __attribute__((overloadable))    __spirv_SConvert_Rchar2(char2 x);
char3 __attribute__((overloadable))    __spirv_SConvert_Rchar3(char3 x);
char4 __attribute__((overloadable))    __spirv_SConvert_Rchar4(char4 x);
char8 __attribute__((overloadable))    __spirv_SConvert_Rchar8(char8 x);
char16 __attribute__((overloadable))   __spirv_SConvert_Rchar16(char16 x);
char2 __attribute__((overloadable))    __spirv_SConvert_Rchar2_sat(char2 x);
char3 __attribute__((overloadable))    __spirv_SConvert_Rchar3_sat(char3 x);
char4 __attribute__((overloadable))    __spirv_SConvert_Rchar4_sat(char4 x);
char8 __attribute__((overloadable))    __spirv_SConvert_Rchar8_sat(char8 x);
char16 __attribute__((overloadable))   __spirv_SConvert_Rchar16_sat(char16 x);
char2 __attribute__((overloadable))    __spirv_SConvert_Rchar2(short2 x);
char3 __attribute__((overloadable))    __spirv_SConvert_Rchar3(short3 x);
char4 __attribute__((overloadable))    __spirv_SConvert_Rchar4(short4 x);
char8 __attribute__((overloadable))    __spirv_SConvert_Rchar8(short8 x);
char16 __attribute__((overloadable))   __spirv_SConvert_Rchar16(short16 x);
char2 __attribute__((overloadable))    __spirv_SConvert_Rchar2_sat(short2 x);
char3 __attribute__((overloadable))    __spirv_SConvert_Rchar3_sat(short3 x);
char4 __attribute__((overloadable))    __spirv_SConvert_Rchar4_sat(short4 x);
char8 __attribute__((overloadable))    __spirv_SConvert_Rchar8_sat(short8 x);
char16 __attribute__((overloadable))   __spirv_SConvert_Rchar16_sat(short16 x);
char2 __attribute__((overloadable))    __spirv_SConvert_Rchar2(int2 x);
char3 __attribute__((overloadable))    __spirv_SConvert_Rchar3(int3 x);
char4 __attribute__((overloadable))    __spirv_SConvert_Rchar4(int4 x);
char8 __attribute__((overloadable))    __spirv_SConvert_Rchar8(int8 x);
char16 __attribute__((overloadable))   __spirv_SConvert_Rchar16(int16 x);
char2 __attribute__((overloadable))    __spirv_SConvert_Rchar2_sat(int2 x);
char3 __attribute__((overloadable))    __spirv_SConvert_Rchar3_sat(int3 x);
char4 __attribute__((overloadable))    __spirv_SConvert_Rchar4_sat(int4 x);
char8 __attribute__((overloadable))    __spirv_SConvert_Rchar8_sat(int8 x);
char16 __attribute__((overloadable))   __spirv_SConvert_Rchar16_sat(int16 x);
char2 __attribute__((overloadable))    __spirv_SConvert_Rchar2(long2 x);
char3 __attribute__((overloadable))    __spirv_SConvert_Rchar3(long3 x);
char4 __attribute__((overloadable))    __spirv_SConvert_Rchar4(long4 x);
char8 __attribute__((overloadable))    __spirv_SConvert_Rchar8(long8 x);
char16 __attribute__((overloadable))   __spirv_SConvert_Rchar16(long16 x);
char2 __attribute__((overloadable))    __spirv_SConvert_Rchar2_sat(long2 x);
char3 __attribute__((overloadable))    __spirv_SConvert_Rchar3_sat(long3 x);
char4 __attribute__((overloadable))    __spirv_SConvert_Rchar4_sat(long4 x);
char8 __attribute__((overloadable))    __spirv_SConvert_Rchar8_sat(long8 x);
char16 __attribute__((overloadable))   __spirv_SConvert_Rchar16_sat(long16 x);
short2 __attribute__((overloadable))   __spirv_SConvert_Rshort2(char2 x);
short3 __attribute__((overloadable))   __spirv_SConvert_Rshort3(char3 x);
short4 __attribute__((overloadable))   __spirv_SConvert_Rshort4(char4 x);
short8 __attribute__((overloadable))   __spirv_SConvert_Rshort8(char8 x);
short16 __attribute__((overloadable))  __spirv_SConvert_Rshort16(char16 x);
short2 __attribute__((overloadable))   __spirv_SConvert_Rshort2_sat(char2 x);
short3 __attribute__((overloadable))   __spirv_SConvert_Rshort3_sat(char3 x);
short4 __attribute__((overloadable))   __spirv_SConvert_Rshort4_sat(char4 x);
short8 __attribute__((overloadable))   __spirv_SConvert_Rshort8_sat(char8 x);
short16 __attribute__((overloadable))  __spirv_SConvert_Rshort16_sat(char16 x);
short2 __attribute__((overloadable))   __spirv_SConvert_Rshort2(short2 x);
short3 __attribute__((overloadable))   __spirv_SConvert_Rshort3(short3 x);
short4 __attribute__((overloadable))   __spirv_SConvert_Rshort4(short4 x);
short8 __attribute__((overloadable))   __spirv_SConvert_Rshort8(short8 x);
short16 __attribute__((overloadable))  __spirv_SConvert_Rshort16(short16 x);
short2 __attribute__((overloadable))   __spirv_SConvert_Rshort2_sat(short2 x);
short3 __attribute__((overloadable))   __spirv_SConvert_Rshort3_sat(short3 x);
short4 __attribute__((overloadable))   __spirv_SConvert_Rshort4_sat(short4 x);
short8 __attribute__((overloadable))   __spirv_SConvert_Rshort8_sat(short8 x);
short16 __attribute__((overloadable))  __spirv_SConvert_Rshort16_sat(short16 x);
short2 __attribute__((overloadable))   __spirv_SConvert_Rshort2(int2 x);
short3 __attribute__((overloadable))   __spirv_SConvert_Rshort3(int3 x);
short4 __attribute__((overloadable))   __spirv_SConvert_Rshort4(int4 x);
short8 __attribute__((overloadable))   __spirv_SConvert_Rshort8(int8 x);
short16 __attribute__((overloadable))  __spirv_SConvert_Rshort16(int16 x);
short2 __attribute__((overloadable))   __spirv_SConvert_Rshort2_sat(int2 x);
short3 __attribute__((overloadable))   __spirv_SConvert_Rshort3_sat(int3 x);
short4 __attribute__((overloadable))   __spirv_SConvert_Rshort4_sat(int4 x);
short8 __attribute__((overloadable))   __spirv_SConvert_Rshort8_sat(int8 x);
short16 __attribute__((overloadable))  __spirv_SConvert_Rshort16_sat(int16 x);
short2 __attribute__((overloadable))   __spirv_SConvert_Rshort2(long2 x);
short3 __attribute__((overloadable))   __spirv_SConvert_Rshort3(long3 x);
short4 __attribute__((overloadable))   __spirv_SConvert_Rshort4(long4 x);
short8 __attribute__((overloadable))   __spirv_SConvert_Rshort8(long8 x);
short16 __attribute__((overloadable))  __spirv_SConvert_Rshort16(long16 x);
short2 __attribute__((overloadable))   __spirv_SConvert_Rshort2_sat(long2 x);
short3 __attribute__((overloadable))   __spirv_SConvert_Rshort3_sat(long3 x);
short4 __attribute__((overloadable))   __spirv_SConvert_Rshort4_sat(long4 x);
short8 __attribute__((overloadable))   __spirv_SConvert_Rshort8_sat(long8 x);
short16 __attribute__((overloadable))  __spirv_SConvert_Rshort16_sat(long16 x);
int2 __attribute__((overloadable))     __spirv_SConvert_Rint2(char2 x);
int3 __attribute__((overloadable))     __spirv_SConvert_Rint3(char3 x);
int4 __attribute__((overloadable))     __spirv_SConvert_Rint4(char4 x);
int8 __attribute__((overloadable))     __spirv_SConvert_Rint8(char8 x);
int16 __attribute__((overloadable))    __spirv_SConvert_Rint16(char16 x);
int2 __attribute__((overloadable))     __spirv_SConvert_Rint2_sat(char2 x);
int3 __attribute__((overloadable))     __spirv_SConvert_Rint3_sat(char3 x);
int4 __attribute__((overloadable))     __spirv_SConvert_Rint4_sat(char4 x);
int8 __attribute__((overloadable))     __spirv_SConvert_Rint8_sat(char8 x);
int16 __attribute__((overloadable))    __spirv_SConvert_Rint16_sat(char16 x);
int2 __attribute__((overloadable))     __spirv_SConvert_Rint2(short2 x);
int3 __attribute__((overloadable))     __spirv_SConvert_Rint3(short3 x);
int4 __attribute__((overloadable))     __spirv_SConvert_Rint4(short4 x);
int8 __attribute__((overloadable))     __spirv_SConvert_Rint8(short8 x);
int16 __attribute__((overloadable))    __spirv_SConvert_Rint16(short16 x);
int2 __attribute__((overloadable))     __spirv_SConvert_Rint2_sat(short2 x);
int3 __attribute__((overloadable))     __spirv_SConvert_Rint3_sat(short3 x);
int4 __attribute__((overloadable))     __spirv_SConvert_Rint4_sat(short4 x);
int8 __attribute__((overloadable))     __spirv_SConvert_Rint8_sat(short8 x);
int16 __attribute__((overloadable))    __spirv_SConvert_Rint16_sat(short16 x);
int2 __attribute__((overloadable))     __spirv_SConvert_Rint2(int2 x);
int3 __attribute__((overloadable))     __spirv_SConvert_Rint3(int3 x);
int4 __attribute__((overloadable))     __spirv_SConvert_Rint4(int4 x);
int8 __attribute__((overloadable))     __spirv_SConvert_Rint8(int8 x);
int16 __attribute__((overloadable))    __spirv_SConvert_Rint16(int16 x);
int2 __attribute__((overloadable))     __spirv_SConvert_Rint2_sat(int2 x);
int3 __attribute__((overloadable))     __spirv_SConvert_Rint3_sat(int3 x);
int4 __attribute__((overloadable))     __spirv_SConvert_Rint4_sat(int4 x);
int8 __attribute__((overloadable))     __spirv_SConvert_Rint8_sat(int8 x);
int16 __attribute__((overloadable))    __spirv_SConvert_Rint16_sat(int16 x);
int2 __attribute__((overloadable))     __spirv_SConvert_Rint2(long2 x);
int3 __attribute__((overloadable))     __spirv_SConvert_Rint3(long3 x);
int4 __attribute__((overloadable))     __spirv_SConvert_Rint4(long4 x);
int8 __attribute__((overloadable))     __spirv_SConvert_Rint8(long8 x);
int16 __attribute__((overloadable))    __spirv_SConvert_Rint16(long16 x);
int2 __attribute__((overloadable))     __spirv_SConvert_Rint2_sat(long2 x);
int3 __attribute__((overloadable))     __spirv_SConvert_Rint3_sat(long3 x);
int4 __attribute__((overloadable))     __spirv_SConvert_Rint4_sat(long4 x);
int8 __attribute__((overloadable))     __spirv_SConvert_Rint8_sat(long8 x);
int16 __attribute__((overloadable))    __spirv_SConvert_Rint16_sat(long16 x);
long2 __attribute__((overloadable))    __spirv_SConvert_Rlong2(char2 x);
long3 __attribute__((overloadable))    __spirv_SConvert_Rlong3(char3 x);
long4 __attribute__((overloadable))    __spirv_SConvert_Rlong4(char4 x);
long8 __attribute__((overloadable))    __spirv_SConvert_Rlong8(char8 x);
long16 __attribute__((overloadable))   __spirv_SConvert_Rlong16(char16 x);
long2 __attribute__((overloadable))    __spirv_SConvert_Rlong2_sat(char2 x);
long3 __attribute__((overloadable))    __spirv_SConvert_Rlong3_sat(char3 x);
long4 __attribute__((overloadable))    __spirv_SConvert_Rlong4_sat(char4 x);
long8 __attribute__((overloadable))    __spirv_SConvert_Rlong8_sat(char8 x);
long16 __attribute__((overloadable))   __spirv_SConvert_Rlong16_sat(char16 x);
long2 __attribute__((overloadable))    __spirv_SConvert_Rlong2(short2 x);
long3 __attribute__((overloadable))    __spirv_SConvert_Rlong3(short3 x);
long4 __attribute__((overloadable))    __spirv_SConvert_Rlong4(short4 x);
long8 __attribute__((overloadable))    __spirv_SConvert_Rlong8(short8 x);
long16 __attribute__((overloadable))   __spirv_SConvert_Rlong16(short16 x);
long2 __attribute__((overloadable))    __spirv_SConvert_Rlong2_sat(short2 x);
long3 __attribute__((overloadable))    __spirv_SConvert_Rlong3_sat(short3 x);
long4 __attribute__((overloadable))    __spirv_SConvert_Rlong4_sat(short4 x);
long8 __attribute__((overloadable))    __spirv_SConvert_Rlong8_sat(short8 x);
long16 __attribute__((overloadable))   __spirv_SConvert_Rlong16_sat(short16 x);
long2 __attribute__((overloadable))    __spirv_SConvert_Rlong2(int2 x);
long3 __attribute__((overloadable))    __spirv_SConvert_Rlong3(int3 x);
long4 __attribute__((overloadable))    __spirv_SConvert_Rlong4(int4 x);
long8 __attribute__((overloadable))    __spirv_SConvert_Rlong8(int8 x);
long16 __attribute__((overloadable))   __spirv_SConvert_Rlong16(int16 x);
long2 __attribute__((overloadable))    __spirv_SConvert_Rlong2_sat(int2 x);
long3 __attribute__((overloadable))    __spirv_SConvert_Rlong3_sat(int3 x);
long4 __attribute__((overloadable))    __spirv_SConvert_Rlong4_sat(int4 x);
long8 __attribute__((overloadable))    __spirv_SConvert_Rlong8_sat(int8 x);
long16 __attribute__((overloadable))   __spirv_SConvert_Rlong16_sat(int16 x);
long2 __attribute__((overloadable))    __spirv_SConvert_Rlong2(long2 x);
long3 __attribute__((overloadable))    __spirv_SConvert_Rlong3(long3 x);
long4 __attribute__((overloadable))    __spirv_SConvert_Rlong4(long4 x);
long8 __attribute__((overloadable))    __spirv_SConvert_Rlong8(long8 x);
long16 __attribute__((overloadable))   __spirv_SConvert_Rlong16(long16 x);
long2 __attribute__((overloadable))    __spirv_SConvert_Rlong2_sat(long2 x);
long3 __attribute__((overloadable))    __spirv_SConvert_Rlong3_sat(long3 x);
long4 __attribute__((overloadable))    __spirv_SConvert_Rlong4_sat(long4 x);
long8 __attribute__((overloadable))    __spirv_SConvert_Rlong8_sat(long8 x);
long16 __attribute__((overloadable))   __spirv_SConvert_Rlong16_sat(long16 x);
uchar2 __attribute__((overloadable))   __spirv_UConvert_Ruchar2(uchar2 x);
uchar3 __attribute__((overloadable))   __spirv_UConvert_Ruchar3(uchar3 x);
uchar4 __attribute__((overloadable))   __spirv_UConvert_Ruchar4(uchar4 x);
uchar8 __attribute__((overloadable))   __spirv_UConvert_Ruchar8(uchar8 x);
uchar16 __attribute__((overloadable))  __spirv_UConvert_Ruchar16(uchar16 x);
uchar2 __attribute__((overloadable))   __spirv_UConvert_Ruchar2_sat(uchar2 x);
uchar3 __attribute__((overloadable))   __spirv_UConvert_Ruchar3_sat(uchar3 x);
uchar4 __attribute__((overloadable))   __spirv_UConvert_Ruchar4_sat(uchar4 x);
uchar8 __attribute__((overloadable))   __spirv_UConvert_Ruchar8_sat(uchar8 x);
uchar16 __attribute__((overloadable))  __spirv_UConvert_Ruchar16_sat(uchar16 x);
uchar2 __attribute__((overloadable))   __spirv_UConvert_Ruchar2(ushort2 x);
uchar3 __attribute__((overloadable))   __spirv_UConvert_Ruchar3(ushort3 x);
uchar4 __attribute__((overloadable))   __spirv_UConvert_Ruchar4(ushort4 x);
uchar8 __attribute__((overloadable))   __spirv_UConvert_Ruchar8(ushort8 x);
uchar16 __attribute__((overloadable))  __spirv_UConvert_Ruchar16(ushort16 x);
uchar2 __attribute__((overloadable))   __spirv_UConvert_Ruchar2_sat(ushort2 x);
uchar3 __attribute__((overloadable))   __spirv_UConvert_Ruchar3_sat(ushort3 x);
uchar4 __attribute__((overloadable))   __spirv_UConvert_Ruchar4_sat(ushort4 x);
uchar8 __attribute__((overloadable))   __spirv_UConvert_Ruchar8_sat(ushort8 x);
uchar16 __attribute__((overloadable))  __spirv_UConvert_Ruchar16_sat(ushort16 x);
uchar2 __attribute__((overloadable))   __spirv_UConvert_Ruchar2(uint2 x);
uchar3 __attribute__((overloadable))   __spirv_UConvert_Ruchar3(uint3 x);
uchar4 __attribute__((overloadable))   __spirv_UConvert_Ruchar4(uint4 x);
uchar8 __attribute__((overloadable))   __spirv_UConvert_Ruchar8(uint8 x);
uchar16 __attribute__((overloadable))  __spirv_UConvert_Ruchar16(uint16 x);
uchar2 __attribute__((overloadable))   __spirv_UConvert_Ruchar2_sat(uint2 x);
uchar3 __attribute__((overloadable))   __spirv_UConvert_Ruchar3_sat(uint3 x);
uchar4 __attribute__((overloadable))   __spirv_UConvert_Ruchar4_sat(uint4 x);
uchar8 __attribute__((overloadable))   __spirv_UConvert_Ruchar8_sat(uint8 x);
uchar16 __attribute__((overloadable))  __spirv_UConvert_Ruchar16_sat(uint16 x);
uchar2 __attribute__((overloadable))   __spirv_UConvert_Ruchar2(ulong2 x);
uchar3 __attribute__((overloadable))   __spirv_UConvert_Ruchar3(ulong3 x);
uchar4 __attribute__((overloadable))   __spirv_UConvert_Ruchar4(ulong4 x);
uchar8 __attribute__((overloadable))   __spirv_UConvert_Ruchar8(ulong8 x);
uchar16 __attribute__((overloadable))  __spirv_UConvert_Ruchar16(ulong16 x);
uchar2 __attribute__((overloadable))   __spirv_UConvert_Ruchar2_sat(ulong2 x);
uchar3 __attribute__((overloadable))   __spirv_UConvert_Ruchar3_sat(ulong3 x);
uchar4 __attribute__((overloadable))   __spirv_UConvert_Ruchar4_sat(ulong4 x);
uchar8 __attribute__((overloadable))   __spirv_UConvert_Ruchar8_sat(ulong8 x);
uchar16 __attribute__((overloadable))  __spirv_UConvert_Ruchar16_sat(ulong16 x);
ushort2 __attribute__((overloadable))  __spirv_UConvert_Rushort2(uchar2 x);
ushort3 __attribute__((overloadable))  __spirv_UConvert_Rushort3(uchar3 x);
ushort4 __attribute__((overloadable))  __spirv_UConvert_Rushort4(uchar4 x);
ushort8 __attribute__((overloadable))  __spirv_UConvert_Rushort8(uchar8 x);
ushort16 __attribute__((overloadable)) __spirv_UConvert_Rushort16(uchar16 x);
ushort2 __attribute__((overloadable))  __spirv_UConvert_Rushort2_sat(uchar2 x);
ushort3 __attribute__((overloadable))  __spirv_UConvert_Rushort3_sat(uchar3 x);
ushort4 __attribute__((overloadable))  __spirv_UConvert_Rushort4_sat(uchar4 x);
ushort8 __attribute__((overloadable))  __spirv_UConvert_Rushort8_sat(uchar8 x);
ushort16 __attribute__((overloadable)) __spirv_UConvert_Rushort16_sat(uchar16 x);
ushort2 __attribute__((overloadable))  __spirv_UConvert_Rushort2(ushort2 x);
ushort3 __attribute__((overloadable))  __spirv_UConvert_Rushort3(ushort3 x);
ushort4 __attribute__((overloadable))  __spirv_UConvert_Rushort4(ushort4 x);
ushort8 __attribute__((overloadable))  __spirv_UConvert_Rushort8(ushort8 x);
ushort16 __attribute__((overloadable)) __spirv_UConvert_Rushort16(ushort16 x);
ushort2 __attribute__((overloadable))  __spirv_UConvert_Rushort2_sat(ushort2 x);
ushort3 __attribute__((overloadable))  __spirv_UConvert_Rushort3_sat(ushort3 x);
ushort4 __attribute__((overloadable))  __spirv_UConvert_Rushort4_sat(ushort4 x);
ushort8 __attribute__((overloadable))  __spirv_UConvert_Rushort8_sat(ushort8 x);
ushort16 __attribute__((overloadable)) __spirv_UConvert_Rushort16_sat(ushort16 x);
ushort2 __attribute__((overloadable))  __spirv_UConvert_Rushort2(uint2 x);
ushort3 __attribute__((overloadable))  __spirv_UConvert_Rushort3(uint3 x);
ushort4 __attribute__((overloadable))  __spirv_UConvert_Rushort4(uint4 x);
ushort8 __attribute__((overloadable))  __spirv_UConvert_Rushort8(uint8 x);
ushort16 __attribute__((overloadable)) __spirv_UConvert_Rushort16(uint16 x);
ushort2 __attribute__((overloadable))  __spirv_UConvert_Rushort2_sat(uint2 x);
ushort3 __attribute__((overloadable))  __spirv_UConvert_Rushort3_sat(uint3 x);
ushort4 __attribute__((overloadable))  __spirv_UConvert_Rushort4_sat(uint4 x);
ushort8 __attribute__((overloadable))  __spirv_UConvert_Rushort8_sat(uint8 x);
ushort16 __attribute__((overloadable)) __spirv_UConvert_Rushort16_sat(uint16 x);
ushort2 __attribute__((overloadable))  __spirv_UConvert_Rushort2(ulong2 x);
ushort3 __attribute__((overloadable))  __spirv_UConvert_Rushort3(ulong3 x);
ushort4 __attribute__((overloadable))  __spirv_UConvert_Rushort4(ulong4 x);
ushort8 __attribute__((overloadable))  __spirv_UConvert_Rushort8(ulong8 x);
ushort16 __attribute__((overloadable)) __spirv_UConvert_Rushort16(ulong16 x);
ushort2 __attribute__((overloadable))  __spirv_UConvert_Rushort2_sat(ulong2 x);
ushort3 __attribute__((overloadable))  __spirv_UConvert_Rushort3_sat(ulong3 x);
ushort4 __attribute__((overloadable))  __spirv_UConvert_Rushort4_sat(ulong4 x);
ushort8 __attribute__((overloadable))  __spirv_UConvert_Rushort8_sat(ulong8 x);
ushort16 __attribute__((overloadable)) __spirv_UConvert_Rushort16_sat(ulong16 x);
uint2 __attribute__((overloadable))    __spirv_UConvert_Ruint2(uchar2 x);
uint3 __attribute__((overloadable))    __spirv_UConvert_Ruint3(uchar3 x);
uint4 __attribute__((overloadable))    __spirv_UConvert_Ruint4(uchar4 x);
uint8 __attribute__((overloadable))    __spirv_UConvert_Ruint8(uchar8 x);
uint16 __attribute__((overloadable))   __spirv_UConvert_Ruint16(uchar16 x);
uint2 __attribute__((overloadable))    __spirv_UConvert_Ruint2_sat(uchar2 x);
uint3 __attribute__((overloadable))    __spirv_UConvert_Ruint3_sat(uchar3 x);
uint4 __attribute__((overloadable))    __spirv_UConvert_Ruint4_sat(uchar4 x);
uint8 __attribute__((overloadable))    __spirv_UConvert_Ruint8_sat(uchar8 x);
uint16 __attribute__((overloadable))   __spirv_UConvert_Ruint16_sat(uchar16 x);
uint2 __attribute__((overloadable))    __spirv_UConvert_Ruint2(ushort2 x);
uint3 __attribute__((overloadable))    __spirv_UConvert_Ruint3(ushort3 x);
uint4 __attribute__((overloadable))    __spirv_UConvert_Ruint4(ushort4 x);
uint8 __attribute__((overloadable))    __spirv_UConvert_Ruint8(ushort8 x);
uint16 __attribute__((overloadable))   __spirv_UConvert_Ruint16(ushort16 x);
uint2 __attribute__((overloadable))    __spirv_UConvert_Ruint2_sat(ushort2 x);
uint3 __attribute__((overloadable))    __spirv_UConvert_Ruint3_sat(ushort3 x);
uint4 __attribute__((overloadable))    __spirv_UConvert_Ruint4_sat(ushort4 x);
uint8 __attribute__((overloadable))    __spirv_UConvert_Ruint8_sat(ushort8 x);
uint16 __attribute__((overloadable))   __spirv_UConvert_Ruint16_sat(ushort16 x);
uint2 __attribute__((overloadable))    __spirv_UConvert_Ruint2(uint2 x);
uint3 __attribute__((overloadable))    __spirv_UConvert_Ruint3(uint3 x);
uint4 __attribute__((overloadable))    __spirv_UConvert_Ruint4(uint4 x);
uint8 __attribute__((overloadable))    __spirv_UConvert_Ruint8(uint8 x);
uint16 __attribute__((overloadable))   __spirv_UConvert_Ruint16(uint16 x);
uint2 __attribute__((overloadable))    __spirv_UConvert_Ruint2_sat(uint2 x);
uint3 __attribute__((overloadable))    __spirv_UConvert_Ruint3_sat(uint3 x);
uint4 __attribute__((overloadable))    __spirv_UConvert_Ruint4_sat(uint4 x);
uint8 __attribute__((overloadable))    __spirv_UConvert_Ruint8_sat(uint8 x);
uint16 __attribute__((overloadable))   __spirv_UConvert_Ruint16_sat(uint16 x);
uint2 __attribute__((overloadable))    __spirv_UConvert_Ruint2(ulong2 x);
uint3 __attribute__((overloadable))    __spirv_UConvert_Ruint3(ulong3 x);
uint4 __attribute__((overloadable))    __spirv_UConvert_Ruint4(ulong4 x);
uint8 __attribute__((overloadable))    __spirv_UConvert_Ruint8(ulong8 x);
uint16 __attribute__((overloadable))   __spirv_UConvert_Ruint16(ulong16 x);
uint2 __attribute__((overloadable))    __spirv_UConvert_Ruint2_sat(ulong2 x);
uint3 __attribute__((overloadable))    __spirv_UConvert_Ruint3_sat(ulong3 x);
uint4 __attribute__((overloadable))    __spirv_UConvert_Ruint4_sat(ulong4 x);
uint8 __attribute__((overloadable))    __spirv_UConvert_Ruint8_sat(ulong8 x);
uint16 __attribute__((overloadable))   __spirv_UConvert_Ruint16_sat(ulong16 x);
ulong2 __attribute__((overloadable))   __spirv_UConvert_Rulong2(uchar2 x);
ulong3 __attribute__((overloadable))   __spirv_UConvert_Rulong3(uchar3 x);
ulong4 __attribute__((overloadable))   __spirv_UConvert_Rulong4(uchar4 x);
ulong8 __attribute__((overloadable))   __spirv_UConvert_Rulong8(uchar8 x);
ulong16 __attribute__((overloadable))  __spirv_UConvert_Rulong16(uchar16 x);
ulong2 __attribute__((overloadable))   __spirv_UConvert_Rulong2_sat(uchar2 x);
ulong3 __attribute__((overloadable))   __spirv_UConvert_Rulong3_sat(uchar3 x);
ulong4 __attribute__((overloadable))   __spirv_UConvert_Rulong4_sat(uchar4 x);
ulong8 __attribute__((overloadable))   __spirv_UConvert_Rulong8_sat(uchar8 x);
ulong16 __attribute__((overloadable))  __spirv_UConvert_Rulong16_sat(uchar16 x);
ulong2 __attribute__((overloadable))   __spirv_UConvert_Rulong2(ushort2 x);
ulong3 __attribute__((overloadable))   __spirv_UConvert_Rulong3(ushort3 x);
ulong4 __attribute__((overloadable))   __spirv_UConvert_Rulong4(ushort4 x);
ulong8 __attribute__((overloadable))   __spirv_UConvert_Rulong8(ushort8 x);
ulong16 __attribute__((overloadable))  __spirv_UConvert_Rulong16(ushort16 x);
ulong2 __attribute__((overloadable))   __spirv_UConvert_Rulong2_sat(ushort2 x);
ulong3 __attribute__((overloadable))   __spirv_UConvert_Rulong3_sat(ushort3 x);
ulong4 __attribute__((overloadable))   __spirv_UConvert_Rulong4_sat(ushort4 x);
ulong8 __attribute__((overloadable))   __spirv_UConvert_Rulong8_sat(ushort8 x);
ulong16 __attribute__((overloadable))  __spirv_UConvert_Rulong16_sat(ushort16 x);
ulong2 __attribute__((overloadable))   __spirv_UConvert_Rulong2(uint2 x);
ulong3 __attribute__((overloadable))   __spirv_UConvert_Rulong3(uint3 x);
ulong4 __attribute__((overloadable))   __spirv_UConvert_Rulong4(uint4 x);
ulong8 __attribute__((overloadable))   __spirv_UConvert_Rulong8(uint8 x);
ulong16 __attribute__((overloadable))  __spirv_UConvert_Rulong16(uint16 x);
ulong2 __attribute__((overloadable))   __spirv_UConvert_Rulong2_sat(uint2 x);
ulong3 __attribute__((overloadable))   __spirv_UConvert_Rulong3_sat(uint3 x);
ulong4 __attribute__((overloadable))   __spirv_UConvert_Rulong4_sat(uint4 x);
ulong8 __attribute__((overloadable))   __spirv_UConvert_Rulong8_sat(uint8 x);
ulong16 __attribute__((overloadable))  __spirv_UConvert_Rulong16_sat(uint16 x);
ulong2 __attribute__((overloadable))   __spirv_UConvert_Rulong2(ulong2 x);
ulong3 __attribute__((overloadable))   __spirv_UConvert_Rulong3(ulong3 x);
ulong4 __attribute__((overloadable))   __spirv_UConvert_Rulong4(ulong4 x);
ulong8 __attribute__((overloadable))   __spirv_UConvert_Rulong8(ulong8 x);
ulong16 __attribute__((overloadable))  __spirv_UConvert_Rulong16(ulong16 x);
ulong2 __attribute__((overloadable))   __spirv_UConvert_Rulong2_sat(ulong2 x);
ulong3 __attribute__((overloadable))   __spirv_UConvert_Rulong3_sat(ulong3 x);
ulong4 __attribute__((overloadable))   __spirv_UConvert_Rulong4_sat(ulong4 x);
ulong8 __attribute__((overloadable))   __spirv_UConvert_Rulong8_sat(ulong8 x);
ulong16 __attribute__((overloadable))  __spirv_UConvert_Rulong16_sat(ulong16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2(float2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3(float3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4(float4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8(float8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16(float16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2_sat(float2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3_sat(float3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4_sat(float4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8_sat(float8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16_sat(float16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2_rte(float2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3_rte(float3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4_rte(float4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8_rte(float8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16_rte(float16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2_rtz(float2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3_rtz(float3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4_rtz(float4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8_rtz(float8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16_rtz(float16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2_rtp(float2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3_rtp(float3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4_rtp(float4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8_rtp(float8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16_rtp(float16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2_rtn(float2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3_rtn(float3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4_rtn(float4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8_rtn(float8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16_rtn(float16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2_sat_rte(float2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3_sat_rte(float3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4_sat_rte(float4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8_sat_rte(float8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16_sat_rte(float16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2_sat_rtz(float2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3_sat_rtz(float3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4_sat_rtz(float4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8_sat_rtz(float8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16_sat_rtz(float16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2_sat_rtp(float2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3_sat_rtp(float3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4_sat_rtp(float4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8_sat_rtp(float8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16_sat_rtp(float16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2_sat_rtn(float2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3_sat_rtn(float3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4_sat_rtn(float4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8_sat_rtn(float8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16_sat_rtn(float16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2(half2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3(half3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4(half4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8(half8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16(half16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2_sat(half2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3_sat(half3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4_sat(half4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8_sat(half8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16_sat(half16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2_rte(half2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3_rte(half3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4_rte(half4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8_rte(half8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16_rte(half16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2_rtz(half2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3_rtz(half3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4_rtz(half4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8_rtz(half8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16_rtz(half16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2_rtp(half2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3_rtp(half3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4_rtp(half4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8_rtp(half8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16_rtp(half16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2_rtn(half2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3_rtn(half3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4_rtn(half4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8_rtn(half8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16_rtn(half16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2_sat_rte(half2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3_sat_rte(half3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4_sat_rte(half4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8_sat_rte(half8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16_sat_rte(half16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2_sat_rtz(half2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3_sat_rtz(half3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4_sat_rtz(half4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8_sat_rtz(half8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16_sat_rtz(half16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2_sat_rtp(half2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3_sat_rtp(half3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4_sat_rtp(half4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8_sat_rtp(half8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16_sat_rtp(half16 x);
char2 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar2_sat_rtn(half2 x);
char3 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar3_sat_rtn(half3 x);
char4 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar4_sat_rtn(half4 x);
char8 __attribute__((overloadable))    __spirv_ConvertFToS_Rchar8_sat_rtn(half8 x);
char16 __attribute__((overloadable))   __spirv_ConvertFToS_Rchar16_sat_rtn(half16 x);
#if defined(cl_khr_fp64)
char2 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar2(double2 x);
char3 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar3(double3 x);
char4 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar4(double4 x);
char8 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar8(double8 x);
char16 __attribute__((overloadable)) __spirv_ConvertFToS_Rchar16(double16 x);
char2 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar2_sat(double2 x);
char3 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar3_sat(double3 x);
char4 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar4_sat(double4 x);
char8 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar8_sat(double8 x);
char16 __attribute__((overloadable)) __spirv_ConvertFToS_Rchar16_sat(double16 x);
char2 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar2_rte(double2 x);
char3 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar3_rte(double3 x);
char4 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar4_rte(double4 x);
char8 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar8_rte(double8 x);
char16 __attribute__((overloadable)) __spirv_ConvertFToS_Rchar16_rte(double16 x);
char2 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar2_rtz(double2 x);
char3 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar3_rtz(double3 x);
char4 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar4_rtz(double4 x);
char8 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar8_rtz(double8 x);
char16 __attribute__((overloadable)) __spirv_ConvertFToS_Rchar16_rtz(double16 x);
char2 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar2_rtp(double2 x);
char3 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar3_rtp(double3 x);
char4 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar4_rtp(double4 x);
char8 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar8_rtp(double8 x);
char16 __attribute__((overloadable)) __spirv_ConvertFToS_Rchar16_rtp(double16 x);
char2 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar2_rtn(double2 x);
char3 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar3_rtn(double3 x);
char4 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar4_rtn(double4 x);
char8 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar8_rtn(double8 x);
char16 __attribute__((overloadable)) __spirv_ConvertFToS_Rchar16_rtn(double16 x);
char2 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar2_sat_rte(double2 x);
char3 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar3_sat_rte(double3 x);
char4 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar4_sat_rte(double4 x);
char8 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar8_sat_rte(double8 x);
char16 __attribute__((overloadable)) __spirv_ConvertFToS_Rchar16_sat_rte(double16 x);
char2 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar2_sat_rtz(double2 x);
char3 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar3_sat_rtz(double3 x);
char4 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar4_sat_rtz(double4 x);
char8 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar8_sat_rtz(double8 x);
char16 __attribute__((overloadable)) __spirv_ConvertFToS_Rchar16_sat_rtz(double16 x);
char2 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar2_sat_rtp(double2 x);
char3 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar3_sat_rtp(double3 x);
char4 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar4_sat_rtp(double4 x);
char8 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar8_sat_rtp(double8 x);
char16 __attribute__((overloadable)) __spirv_ConvertFToS_Rchar16_sat_rtp(double16 x);
char2 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar2_sat_rtn(double2 x);
char3 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar3_sat_rtn(double3 x);
char4 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar4_sat_rtn(double4 x);
char8 __attribute__((overloadable))  __spirv_ConvertFToS_Rchar8_sat_rtn(double8 x);
char16 __attribute__((overloadable)) __spirv_ConvertFToS_Rchar16_sat_rtn(double16 x);
#endif // defined(cl_khr_fp64)
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2(float2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3(float3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4(float4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8(float8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16(float16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_sat(float2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_sat(float3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_sat(float4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_sat(float8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_sat(float16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_rte(float2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_rte(float3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_rte(float4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_rte(float8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_rte(float16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_rtz(float2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_rtz(float3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_rtz(float4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_rtz(float8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_rtz(float16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_rtp(float2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_rtp(float3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_rtp(float4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_rtp(float8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_rtp(float16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_rtn(float2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_rtn(float3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_rtn(float4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_rtn(float8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_rtn(float16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_sat_rte(float2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_sat_rte(float3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_sat_rte(float4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_sat_rte(float8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_sat_rte(float16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_sat_rtz(float2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_sat_rtz(float3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_sat_rtz(float4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_sat_rtz(float8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_sat_rtz(float16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_sat_rtp(float2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_sat_rtp(float3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_sat_rtp(float4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_sat_rtp(float8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_sat_rtp(float16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_sat_rtn(float2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_sat_rtn(float3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_sat_rtn(float4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_sat_rtn(float8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_sat_rtn(float16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2(half2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3(half3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4(half4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8(half8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16(half16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_sat(half2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_sat(half3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_sat(half4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_sat(half8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_sat(half16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_rte(half2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_rte(half3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_rte(half4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_rte(half8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_rte(half16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_rtz(half2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_rtz(half3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_rtz(half4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_rtz(half8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_rtz(half16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_rtp(half2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_rtp(half3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_rtp(half4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_rtp(half8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_rtp(half16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_rtn(half2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_rtn(half3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_rtn(half4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_rtn(half8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_rtn(half16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_sat_rte(half2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_sat_rte(half3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_sat_rte(half4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_sat_rte(half8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_sat_rte(half16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_sat_rtz(half2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_sat_rtz(half3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_sat_rtz(half4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_sat_rtz(half8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_sat_rtz(half16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_sat_rtp(half2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_sat_rtp(half3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_sat_rtp(half4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_sat_rtp(half8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_sat_rtp(half16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_sat_rtn(half2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_sat_rtn(half3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_sat_rtn(half4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_sat_rtn(half8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_sat_rtn(half16 x);
#if defined(cl_khr_fp64)
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2(double2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3(double3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4(double4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8(double8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16(double16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_sat(double2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_sat(double3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_sat(double4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_sat(double8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_sat(double16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_rte(double2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_rte(double3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_rte(double4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_rte(double8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_rte(double16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_rtz(double2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_rtz(double3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_rtz(double4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_rtz(double8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_rtz(double16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_rtp(double2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_rtp(double3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_rtp(double4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_rtp(double8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_rtp(double16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_rtn(double2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_rtn(double3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_rtn(double4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_rtn(double8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_rtn(double16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_sat_rte(double2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_sat_rte(double3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_sat_rte(double4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_sat_rte(double8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_sat_rte(double16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_sat_rtz(double2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_sat_rtz(double3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_sat_rtz(double4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_sat_rtz(double8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_sat_rtz(double16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_sat_rtp(double2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_sat_rtp(double3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_sat_rtp(double4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_sat_rtp(double8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_sat_rtp(double16 x);
short2 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort2_sat_rtn(double2 x);
short3 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort3_sat_rtn(double3 x);
short4 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort4_sat_rtn(double4 x);
short8 __attribute__((overloadable))  __spirv_ConvertFToS_Rshort8_sat_rtn(double8 x);
short16 __attribute__((overloadable)) __spirv_ConvertFToS_Rshort16_sat_rtn(double16 x);
#endif // defined(cl_khr_fp64)
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2(float2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3(float3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4(float4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8(float8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16(float16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_sat(float2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_sat(float3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_sat(float4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_sat(float8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_sat(float16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_rte(float2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_rte(float3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_rte(float4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_rte(float8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_rte(float16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_rtz(float2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_rtz(float3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_rtz(float4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_rtz(float8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_rtz(float16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_rtp(float2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_rtp(float3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_rtp(float4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_rtp(float8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_rtp(float16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_rtn(float2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_rtn(float3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_rtn(float4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_rtn(float8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_rtn(float16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_sat_rte(float2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_sat_rte(float3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_sat_rte(float4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_sat_rte(float8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_sat_rte(float16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_sat_rtz(float2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_sat_rtz(float3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_sat_rtz(float4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_sat_rtz(float8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_sat_rtz(float16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_sat_rtp(float2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_sat_rtp(float3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_sat_rtp(float4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_sat_rtp(float8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_sat_rtp(float16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_sat_rtn(float2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_sat_rtn(float3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_sat_rtn(float4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_sat_rtn(float8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_sat_rtn(float16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2(half2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3(half3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4(half4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8(half8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16(half16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_sat(half2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_sat(half3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_sat(half4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_sat(half8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_sat(half16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_rte(half2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_rte(half3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_rte(half4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_rte(half8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_rte(half16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_rtz(half2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_rtz(half3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_rtz(half4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_rtz(half8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_rtz(half16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_rtp(half2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_rtp(half3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_rtp(half4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_rtp(half8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_rtp(half16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_rtn(half2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_rtn(half3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_rtn(half4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_rtn(half8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_rtn(half16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_sat_rte(half2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_sat_rte(half3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_sat_rte(half4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_sat_rte(half8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_sat_rte(half16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_sat_rtz(half2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_sat_rtz(half3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_sat_rtz(half4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_sat_rtz(half8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_sat_rtz(half16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_sat_rtp(half2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_sat_rtp(half3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_sat_rtp(half4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_sat_rtp(half8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_sat_rtp(half16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_sat_rtn(half2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_sat_rtn(half3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_sat_rtn(half4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_sat_rtn(half8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_sat_rtn(half16 x);
#if defined(cl_khr_fp64)
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2(double2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3(double3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4(double4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8(double8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16(double16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_sat(double2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_sat(double3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_sat(double4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_sat(double8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_sat(double16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_rte(double2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_rte(double3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_rte(double4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_rte(double8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_rte(double16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_rtz(double2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_rtz(double3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_rtz(double4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_rtz(double8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_rtz(double16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_rtp(double2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_rtp(double3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_rtp(double4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_rtp(double8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_rtp(double16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_rtn(double2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_rtn(double3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_rtn(double4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_rtn(double8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_rtn(double16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_sat_rte(double2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_sat_rte(double3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_sat_rte(double4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_sat_rte(double8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_sat_rte(double16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_sat_rtz(double2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_sat_rtz(double3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_sat_rtz(double4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_sat_rtz(double8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_sat_rtz(double16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_sat_rtp(double2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_sat_rtp(double3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_sat_rtp(double4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_sat_rtp(double8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_sat_rtp(double16 x);
int2 __attribute__((overloadable))  __spirv_ConvertFToS_Rint2_sat_rtn(double2 x);
int3 __attribute__((overloadable))  __spirv_ConvertFToS_Rint3_sat_rtn(double3 x);
int4 __attribute__((overloadable))  __spirv_ConvertFToS_Rint4_sat_rtn(double4 x);
int8 __attribute__((overloadable))  __spirv_ConvertFToS_Rint8_sat_rtn(double8 x);
int16 __attribute__((overloadable)) __spirv_ConvertFToS_Rint16_sat_rtn(double16 x);
#endif // defined(cl_khr_fp64)
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2(float2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3(float3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4(float4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8(float8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16(float16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_sat(float2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_sat(float3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_sat(float4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_sat(float8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_sat(float16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_rte(float2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_rte(float3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_rte(float4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_rte(float8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_rte(float16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_rtz(float2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_rtz(float3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_rtz(float4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_rtz(float8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_rtz(float16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_rtp(float2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_rtp(float3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_rtp(float4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_rtp(float8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_rtp(float16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_rtn(float2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_rtn(float3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_rtn(float4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_rtn(float8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_rtn(float16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_sat_rte(float2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_sat_rte(float3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_sat_rte(float4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_sat_rte(float8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_sat_rte(float16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_sat_rtz(float2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_sat_rtz(float3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_sat_rtz(float4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_sat_rtz(float8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_sat_rtz(float16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_sat_rtp(float2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_sat_rtp(float3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_sat_rtp(float4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_sat_rtp(float8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_sat_rtp(float16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_sat_rtn(float2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_sat_rtn(float3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_sat_rtn(float4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_sat_rtn(float8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_sat_rtn(float16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2(half2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3(half3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4(half4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8(half8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16(half16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_sat(half2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_sat(half3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_sat(half4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_sat(half8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_sat(half16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_rte(half2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_rte(half3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_rte(half4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_rte(half8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_rte(half16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_rtz(half2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_rtz(half3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_rtz(half4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_rtz(half8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_rtz(half16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_rtp(half2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_rtp(half3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_rtp(half4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_rtp(half8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_rtp(half16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_rtn(half2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_rtn(half3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_rtn(half4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_rtn(half8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_rtn(half16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_sat_rte(half2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_sat_rte(half3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_sat_rte(half4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_sat_rte(half8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_sat_rte(half16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_sat_rtz(half2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_sat_rtz(half3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_sat_rtz(half4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_sat_rtz(half8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_sat_rtz(half16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_sat_rtp(half2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_sat_rtp(half3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_sat_rtp(half4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_sat_rtp(half8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_sat_rtp(half16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_sat_rtn(half2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_sat_rtn(half3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_sat_rtn(half4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_sat_rtn(half8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_sat_rtn(half16 x);
#if defined(cl_khr_fp64)
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2(double2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3(double3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4(double4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8(double8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16(double16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_sat(double2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_sat(double3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_sat(double4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_sat(double8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_sat(double16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_rte(double2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_rte(double3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_rte(double4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_rte(double8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_rte(double16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_rtz(double2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_rtz(double3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_rtz(double4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_rtz(double8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_rtz(double16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_rtp(double2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_rtp(double3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_rtp(double4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_rtp(double8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_rtp(double16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_rtn(double2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_rtn(double3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_rtn(double4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_rtn(double8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_rtn(double16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_sat_rte(double2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_sat_rte(double3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_sat_rte(double4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_sat_rte(double8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_sat_rte(double16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_sat_rtz(double2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_sat_rtz(double3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_sat_rtz(double4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_sat_rtz(double8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_sat_rtz(double16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_sat_rtp(double2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_sat_rtp(double3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_sat_rtp(double4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_sat_rtp(double8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_sat_rtp(double16 x);
long2 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong2_sat_rtn(double2 x);
long3 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong3_sat_rtn(double3 x);
long4 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong4_sat_rtn(double4 x);
long8 __attribute__((overloadable))  __spirv_ConvertFToS_Rlong8_sat_rtn(double8 x);
long16 __attribute__((overloadable)) __spirv_ConvertFToS_Rlong16_sat_rtn(double16 x);
#endif // defined(cl_khr_fp64)
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2(float2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3(float3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4(float4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8(float8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16(float16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_sat(float2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_sat(float3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_sat(float4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_sat(float8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_sat(float16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_rte(float2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_rte(float3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_rte(float4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_rte(float8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_rte(float16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_rtz(float2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_rtz(float3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_rtz(float4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_rtz(float8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_rtz(float16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_rtp(float2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_rtp(float3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_rtp(float4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_rtp(float8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_rtp(float16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_rtn(float2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_rtn(float3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_rtn(float4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_rtn(float8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_rtn(float16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_sat_rte(float2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_sat_rte(float3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_sat_rte(float4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_sat_rte(float8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_sat_rte(float16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_sat_rtz(float2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_sat_rtz(float3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_sat_rtz(float4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_sat_rtz(float8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_sat_rtz(float16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_sat_rtp(float2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_sat_rtp(float3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_sat_rtp(float4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_sat_rtp(float8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_sat_rtp(float16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_sat_rtn(float2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_sat_rtn(float3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_sat_rtn(float4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_sat_rtn(float8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_sat_rtn(float16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2(half2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3(half3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4(half4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8(half8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16(half16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_sat(half2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_sat(half3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_sat(half4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_sat(half8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_sat(half16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_rte(half2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_rte(half3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_rte(half4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_rte(half8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_rte(half16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_rtz(half2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_rtz(half3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_rtz(half4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_rtz(half8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_rtz(half16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_rtp(half2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_rtp(half3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_rtp(half4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_rtp(half8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_rtp(half16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_rtn(half2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_rtn(half3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_rtn(half4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_rtn(half8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_rtn(half16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_sat_rte(half2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_sat_rte(half3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_sat_rte(half4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_sat_rte(half8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_sat_rte(half16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_sat_rtz(half2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_sat_rtz(half3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_sat_rtz(half4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_sat_rtz(half8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_sat_rtz(half16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_sat_rtp(half2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_sat_rtp(half3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_sat_rtp(half4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_sat_rtp(half8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_sat_rtp(half16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_sat_rtn(half2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_sat_rtn(half3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_sat_rtn(half4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_sat_rtn(half8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_sat_rtn(half16 x);
#if defined(cl_khr_fp64)
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2(double2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3(double3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4(double4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8(double8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16(double16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_sat(double2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_sat(double3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_sat(double4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_sat(double8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_sat(double16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_rte(double2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_rte(double3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_rte(double4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_rte(double8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_rte(double16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_rtz(double2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_rtz(double3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_rtz(double4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_rtz(double8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_rtz(double16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_rtp(double2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_rtp(double3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_rtp(double4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_rtp(double8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_rtp(double16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_rtn(double2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_rtn(double3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_rtn(double4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_rtn(double8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_rtn(double16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_sat_rte(double2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_sat_rte(double3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_sat_rte(double4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_sat_rte(double8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_sat_rte(double16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_sat_rtz(double2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_sat_rtz(double3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_sat_rtz(double4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_sat_rtz(double8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_sat_rtz(double16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_sat_rtp(double2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_sat_rtp(double3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_sat_rtp(double4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_sat_rtp(double8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_sat_rtp(double16 x);
uchar2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar2_sat_rtn(double2 x);
uchar3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar3_sat_rtn(double3 x);
uchar4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar4_sat_rtn(double4 x);
uchar8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruchar8_sat_rtn(double8 x);
uchar16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar16_sat_rtn(double16 x);
#endif // defined(cl_khr_fp64)
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2(float2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3(float3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4(float4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8(float8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16(float16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_sat(float2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_sat(float3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_sat(float4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_sat(float8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_sat(float16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_rte(float2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_rte(float3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_rte(float4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_rte(float8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_rte(float16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_rtz(float2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_rtz(float3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_rtz(float4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_rtz(float8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_rtz(float16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_rtp(float2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_rtp(float3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_rtp(float4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_rtp(float8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_rtp(float16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_rtn(float2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_rtn(float3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_rtn(float4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_rtn(float8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_rtn(float16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_sat_rte(float2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_sat_rte(float3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_sat_rte(float4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_sat_rte(float8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_sat_rte(float16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_sat_rtz(float2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_sat_rtz(float3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_sat_rtz(float4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_sat_rtz(float8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_sat_rtz(float16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_sat_rtp(float2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_sat_rtp(float3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_sat_rtp(float4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_sat_rtp(float8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_sat_rtp(float16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_sat_rtn(float2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_sat_rtn(float3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_sat_rtn(float4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_sat_rtn(float8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_sat_rtn(float16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2(half2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3(half3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4(half4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8(half8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16(half16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_sat(half2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_sat(half3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_sat(half4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_sat(half8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_sat(half16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_rte(half2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_rte(half3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_rte(half4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_rte(half8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_rte(half16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_rtz(half2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_rtz(half3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_rtz(half4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_rtz(half8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_rtz(half16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_rtp(half2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_rtp(half3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_rtp(half4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_rtp(half8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_rtp(half16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_rtn(half2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_rtn(half3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_rtn(half4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_rtn(half8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_rtn(half16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_sat_rte(half2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_sat_rte(half3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_sat_rte(half4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_sat_rte(half8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_sat_rte(half16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_sat_rtz(half2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_sat_rtz(half3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_sat_rtz(half4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_sat_rtz(half8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_sat_rtz(half16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_sat_rtp(half2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_sat_rtp(half3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_sat_rtp(half4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_sat_rtp(half8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_sat_rtp(half16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_sat_rtn(half2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_sat_rtn(half3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_sat_rtn(half4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_sat_rtn(half8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_sat_rtn(half16 x);
#if defined(cl_khr_fp64)
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2(double2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3(double3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4(double4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8(double8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16(double16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_sat(double2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_sat(double3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_sat(double4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_sat(double8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_sat(double16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_rte(double2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_rte(double3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_rte(double4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_rte(double8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_rte(double16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_rtz(double2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_rtz(double3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_rtz(double4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_rtz(double8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_rtz(double16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_rtp(double2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_rtp(double3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_rtp(double4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_rtp(double8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_rtp(double16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_rtn(double2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_rtn(double3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_rtn(double4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_rtn(double8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_rtn(double16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_sat_rte(double2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_sat_rte(double3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_sat_rte(double4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_sat_rte(double8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_sat_rte(double16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_sat_rtz(double2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_sat_rtz(double3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_sat_rtz(double4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_sat_rtz(double8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_sat_rtz(double16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_sat_rtp(double2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_sat_rtp(double3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_sat_rtp(double4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_sat_rtp(double8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_sat_rtp(double16 x);
ushort2 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort2_sat_rtn(double2 x);
ushort3 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort3_sat_rtn(double3 x);
ushort4 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort4_sat_rtn(double4 x);
ushort8 __attribute__((overloadable))  __spirv_ConvertFToU_Rushort8_sat_rtn(double8 x);
ushort16 __attribute__((overloadable)) __spirv_ConvertFToU_Rushort16_sat_rtn(double16 x);
#endif // defined(cl_khr_fp64)
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2(float2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3(float3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4(float4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8(float8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16(float16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_sat(float2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_sat(float3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_sat(float4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_sat(float8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_sat(float16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_rte(float2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_rte(float3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_rte(float4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_rte(float8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_rte(float16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_rtz(float2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_rtz(float3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_rtz(float4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_rtz(float8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_rtz(float16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_rtp(float2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_rtp(float3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_rtp(float4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_rtp(float8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_rtp(float16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_rtn(float2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_rtn(float3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_rtn(float4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_rtn(float8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_rtn(float16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_sat_rte(float2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_sat_rte(float3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_sat_rte(float4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_sat_rte(float8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_sat_rte(float16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_sat_rtz(float2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_sat_rtz(float3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_sat_rtz(float4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_sat_rtz(float8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_sat_rtz(float16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_sat_rtp(float2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_sat_rtp(float3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_sat_rtp(float4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_sat_rtp(float8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_sat_rtp(float16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_sat_rtn(float2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_sat_rtn(float3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_sat_rtn(float4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_sat_rtn(float8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_sat_rtn(float16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2(half2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3(half3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4(half4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8(half8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16(half16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_sat(half2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_sat(half3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_sat(half4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_sat(half8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_sat(half16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_rte(half2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_rte(half3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_rte(half4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_rte(half8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_rte(half16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_rtz(half2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_rtz(half3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_rtz(half4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_rtz(half8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_rtz(half16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_rtp(half2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_rtp(half3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_rtp(half4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_rtp(half8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_rtp(half16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_rtn(half2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_rtn(half3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_rtn(half4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_rtn(half8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_rtn(half16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_sat_rte(half2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_sat_rte(half3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_sat_rte(half4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_sat_rte(half8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_sat_rte(half16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_sat_rtz(half2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_sat_rtz(half3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_sat_rtz(half4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_sat_rtz(half8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_sat_rtz(half16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_sat_rtp(half2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_sat_rtp(half3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_sat_rtp(half4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_sat_rtp(half8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_sat_rtp(half16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_sat_rtn(half2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_sat_rtn(half3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_sat_rtn(half4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_sat_rtn(half8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_sat_rtn(half16 x);
#if defined(cl_khr_fp64)
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2(double2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3(double3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4(double4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8(double8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16(double16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_sat(double2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_sat(double3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_sat(double4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_sat(double8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_sat(double16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_rte(double2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_rte(double3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_rte(double4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_rte(double8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_rte(double16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_rtz(double2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_rtz(double3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_rtz(double4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_rtz(double8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_rtz(double16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_rtp(double2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_rtp(double3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_rtp(double4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_rtp(double8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_rtp(double16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_rtn(double2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_rtn(double3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_rtn(double4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_rtn(double8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_rtn(double16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_sat_rte(double2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_sat_rte(double3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_sat_rte(double4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_sat_rte(double8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_sat_rte(double16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_sat_rtz(double2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_sat_rtz(double3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_sat_rtz(double4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_sat_rtz(double8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_sat_rtz(double16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_sat_rtp(double2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_sat_rtp(double3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_sat_rtp(double4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_sat_rtp(double8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_sat_rtp(double16 x);
uint2 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint2_sat_rtn(double2 x);
uint3 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint3_sat_rtn(double3 x);
uint4 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint4_sat_rtn(double4 x);
uint8 __attribute__((overloadable))  __spirv_ConvertFToU_Ruint8_sat_rtn(double8 x);
uint16 __attribute__((overloadable)) __spirv_ConvertFToU_Ruint16_sat_rtn(double16 x);
#endif // defined(cl_khr_fp64)
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2(float2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3(float3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4(float4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8(float8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16(float16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_sat(float2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_sat(float3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_sat(float4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_sat(float8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_sat(float16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_rte(float2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_rte(float3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_rte(float4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_rte(float8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_rte(float16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_rtz(float2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_rtz(float3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_rtz(float4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_rtz(float8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_rtz(float16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_rtp(float2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_rtp(float3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_rtp(float4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_rtp(float8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_rtp(float16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_rtn(float2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_rtn(float3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_rtn(float4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_rtn(float8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_rtn(float16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_sat_rte(float2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_sat_rte(float3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_sat_rte(float4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_sat_rte(float8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_sat_rte(float16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_sat_rtz(float2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_sat_rtz(float3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_sat_rtz(float4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_sat_rtz(float8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_sat_rtz(float16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_sat_rtp(float2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_sat_rtp(float3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_sat_rtp(float4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_sat_rtp(float8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_sat_rtp(float16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_sat_rtn(float2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_sat_rtn(float3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_sat_rtn(float4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_sat_rtn(float8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_sat_rtn(float16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2(half2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3(half3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4(half4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8(half8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16(half16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_sat(half2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_sat(half3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_sat(half4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_sat(half8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_sat(half16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_rte(half2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_rte(half3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_rte(half4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_rte(half8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_rte(half16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_rtz(half2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_rtz(half3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_rtz(half4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_rtz(half8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_rtz(half16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_rtp(half2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_rtp(half3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_rtp(half4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_rtp(half8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_rtp(half16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_rtn(half2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_rtn(half3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_rtn(half4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_rtn(half8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_rtn(half16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_sat_rte(half2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_sat_rte(half3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_sat_rte(half4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_sat_rte(half8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_sat_rte(half16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_sat_rtz(half2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_sat_rtz(half3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_sat_rtz(half4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_sat_rtz(half8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_sat_rtz(half16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_sat_rtp(half2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_sat_rtp(half3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_sat_rtp(half4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_sat_rtp(half8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_sat_rtp(half16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_sat_rtn(half2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_sat_rtn(half3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_sat_rtn(half4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_sat_rtn(half8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_sat_rtn(half16 x);
#if defined(cl_khr_fp64)
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2(double2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3(double3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4(double4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8(double8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16(double16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_sat(double2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_sat(double3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_sat(double4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_sat(double8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_sat(double16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_rte(double2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_rte(double3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_rte(double4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_rte(double8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_rte(double16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_rtz(double2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_rtz(double3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_rtz(double4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_rtz(double8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_rtz(double16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_rtp(double2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_rtp(double3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_rtp(double4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_rtp(double8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_rtp(double16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_rtn(double2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_rtn(double3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_rtn(double4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_rtn(double8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_rtn(double16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_sat_rte(double2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_sat_rte(double3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_sat_rte(double4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_sat_rte(double8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_sat_rte(double16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_sat_rtz(double2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_sat_rtz(double3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_sat_rtz(double4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_sat_rtz(double8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_sat_rtz(double16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_sat_rtp(double2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_sat_rtp(double3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_sat_rtp(double4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_sat_rtp(double8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_sat_rtp(double16 x);
ulong2 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong2_sat_rtn(double2 x);
ulong3 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong3_sat_rtn(double3 x);
ulong4 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong4_sat_rtn(double4 x);
ulong8 __attribute__((overloadable))  __spirv_ConvertFToU_Rulong8_sat_rtn(double8 x);
ulong16 __attribute__((overloadable)) __spirv_ConvertFToU_Rulong16_sat_rtn(double16 x);
#endif // defined(cl_khr_fp64)
uchar2 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruchar2(char2 x);
uchar3 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruchar3(char3 x);
uchar4 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruchar4(char4 x);
uchar8 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruchar8(char8 x);
uchar16 __attribute__((overloadable))  __spirv_SatConvertSToU_Ruchar16(char16 x);
uchar2 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruchar2(short2 x);
uchar3 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruchar3(short3 x);
uchar4 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruchar4(short4 x);
uchar8 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruchar8(short8 x);
uchar16 __attribute__((overloadable))  __spirv_SatConvertSToU_Ruchar16(short16 x);
uchar2 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruchar2(int2 x);
uchar3 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruchar3(int3 x);
uchar4 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruchar4(int4 x);
uchar8 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruchar8(int8 x);
uchar16 __attribute__((overloadable))  __spirv_SatConvertSToU_Ruchar16(int16 x);
uchar2 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruchar2(long2 x);
uchar3 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruchar3(long3 x);
uchar4 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruchar4(long4 x);
uchar8 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruchar8(long8 x);
uchar16 __attribute__((overloadable))  __spirv_SatConvertSToU_Ruchar16(long16 x);
ushort2 __attribute__((overloadable))  __spirv_SatConvertSToU_Rushort2(char2 x);
ushort3 __attribute__((overloadable))  __spirv_SatConvertSToU_Rushort3(char3 x);
ushort4 __attribute__((overloadable))  __spirv_SatConvertSToU_Rushort4(char4 x);
ushort8 __attribute__((overloadable))  __spirv_SatConvertSToU_Rushort8(char8 x);
ushort16 __attribute__((overloadable)) __spirv_SatConvertSToU_Rushort16(char16 x);
ushort2 __attribute__((overloadable))  __spirv_SatConvertSToU_Rushort2(short2 x);
ushort3 __attribute__((overloadable))  __spirv_SatConvertSToU_Rushort3(short3 x);
ushort4 __attribute__((overloadable))  __spirv_SatConvertSToU_Rushort4(short4 x);
ushort8 __attribute__((overloadable))  __spirv_SatConvertSToU_Rushort8(short8 x);
ushort16 __attribute__((overloadable)) __spirv_SatConvertSToU_Rushort16(short16 x);
ushort2 __attribute__((overloadable))  __spirv_SatConvertSToU_Rushort2(int2 x);
ushort3 __attribute__((overloadable))  __spirv_SatConvertSToU_Rushort3(int3 x);
ushort4 __attribute__((overloadable))  __spirv_SatConvertSToU_Rushort4(int4 x);
ushort8 __attribute__((overloadable))  __spirv_SatConvertSToU_Rushort8(int8 x);
ushort16 __attribute__((overloadable)) __spirv_SatConvertSToU_Rushort16(int16 x);
ushort2 __attribute__((overloadable))  __spirv_SatConvertSToU_Rushort2(long2 x);
ushort3 __attribute__((overloadable))  __spirv_SatConvertSToU_Rushort3(long3 x);
ushort4 __attribute__((overloadable))  __spirv_SatConvertSToU_Rushort4(long4 x);
ushort8 __attribute__((overloadable))  __spirv_SatConvertSToU_Rushort8(long8 x);
ushort16 __attribute__((overloadable)) __spirv_SatConvertSToU_Rushort16(long16 x);
uint2 __attribute__((overloadable))    __spirv_SatConvertSToU_Ruint2(char2 x);
uint3 __attribute__((overloadable))    __spirv_SatConvertSToU_Ruint3(char3 x);
uint4 __attribute__((overloadable))    __spirv_SatConvertSToU_Ruint4(char4 x);
uint8 __attribute__((overloadable))    __spirv_SatConvertSToU_Ruint8(char8 x);
uint16 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruint16(char16 x);
uint2 __attribute__((overloadable))    __spirv_SatConvertSToU_Ruint2(short2 x);
uint3 __attribute__((overloadable))    __spirv_SatConvertSToU_Ruint3(short3 x);
uint4 __attribute__((overloadable))    __spirv_SatConvertSToU_Ruint4(short4 x);
uint8 __attribute__((overloadable))    __spirv_SatConvertSToU_Ruint8(short8 x);
uint16 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruint16(short16 x);
uint2 __attribute__((overloadable))    __spirv_SatConvertSToU_Ruint2(int2 x);
uint3 __attribute__((overloadable))    __spirv_SatConvertSToU_Ruint3(int3 x);
uint4 __attribute__((overloadable))    __spirv_SatConvertSToU_Ruint4(int4 x);
uint8 __attribute__((overloadable))    __spirv_SatConvertSToU_Ruint8(int8 x);
uint16 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruint16(int16 x);
uint2 __attribute__((overloadable))    __spirv_SatConvertSToU_Ruint2(long2 x);
uint3 __attribute__((overloadable))    __spirv_SatConvertSToU_Ruint3(long3 x);
uint4 __attribute__((overloadable))    __spirv_SatConvertSToU_Ruint4(long4 x);
uint8 __attribute__((overloadable))    __spirv_SatConvertSToU_Ruint8(long8 x);
uint16 __attribute__((overloadable))   __spirv_SatConvertSToU_Ruint16(long16 x);
ulong2 __attribute__((overloadable))   __spirv_SatConvertSToU_Rulong2(char2 x);
ulong3 __attribute__((overloadable))   __spirv_SatConvertSToU_Rulong3(char3 x);
ulong4 __attribute__((overloadable))   __spirv_SatConvertSToU_Rulong4(char4 x);
ulong8 __attribute__((overloadable))   __spirv_SatConvertSToU_Rulong8(char8 x);
ulong16 __attribute__((overloadable))  __spirv_SatConvertSToU_Rulong16(char16 x);
ulong2 __attribute__((overloadable))   __spirv_SatConvertSToU_Rulong2(short2 x);
ulong3 __attribute__((overloadable))   __spirv_SatConvertSToU_Rulong3(short3 x);
ulong4 __attribute__((overloadable))   __spirv_SatConvertSToU_Rulong4(short4 x);
ulong8 __attribute__((overloadable))   __spirv_SatConvertSToU_Rulong8(short8 x);
ulong16 __attribute__((overloadable))  __spirv_SatConvertSToU_Rulong16(short16 x);
ulong2 __attribute__((overloadable))   __spirv_SatConvertSToU_Rulong2(int2 x);
ulong3 __attribute__((overloadable))   __spirv_SatConvertSToU_Rulong3(int3 x);
ulong4 __attribute__((overloadable))   __spirv_SatConvertSToU_Rulong4(int4 x);
ulong8 __attribute__((overloadable))   __spirv_SatConvertSToU_Rulong8(int8 x);
ulong16 __attribute__((overloadable))  __spirv_SatConvertSToU_Rulong16(int16 x);
ulong2 __attribute__((overloadable))   __spirv_SatConvertSToU_Rulong2(long2 x);
ulong3 __attribute__((overloadable))   __spirv_SatConvertSToU_Rulong3(long3 x);
ulong4 __attribute__((overloadable))   __spirv_SatConvertSToU_Rulong4(long4 x);
ulong8 __attribute__((overloadable))   __spirv_SatConvertSToU_Rulong8(long8 x);
ulong16 __attribute__((overloadable))  __spirv_SatConvertSToU_Rulong16(long16 x);
char2 __attribute__((overloadable))    __spirv_SatConvertUToS_Rchar2(uchar2 x);
char3 __attribute__((overloadable))    __spirv_SatConvertUToS_Rchar3(uchar3 x);
char4 __attribute__((overloadable))    __spirv_SatConvertUToS_Rchar4(uchar4 x);
char8 __attribute__((overloadable))    __spirv_SatConvertUToS_Rchar8(uchar8 x);
char16 __attribute__((overloadable))   __spirv_SatConvertUToS_Rchar16(uchar16 x);
char2 __attribute__((overloadable))    __spirv_SatConvertUToS_Rchar2(ushort2 x);
char3 __attribute__((overloadable))    __spirv_SatConvertUToS_Rchar3(ushort3 x);
char4 __attribute__((overloadable))    __spirv_SatConvertUToS_Rchar4(ushort4 x);
char8 __attribute__((overloadable))    __spirv_SatConvertUToS_Rchar8(ushort8 x);
char16 __attribute__((overloadable))   __spirv_SatConvertUToS_Rchar16(ushort16 x);
char2 __attribute__((overloadable))    __spirv_SatConvertUToS_Rchar2(uint2 x);
char3 __attribute__((overloadable))    __spirv_SatConvertUToS_Rchar3(uint3 x);
char4 __attribute__((overloadable))    __spirv_SatConvertUToS_Rchar4(uint4 x);
char8 __attribute__((overloadable))    __spirv_SatConvertUToS_Rchar8(uint8 x);
char16 __attribute__((overloadable))   __spirv_SatConvertUToS_Rchar16(uint16 x);
char2 __attribute__((overloadable))    __spirv_SatConvertUToS_Rchar2(ulong2 x);
char3 __attribute__((overloadable))    __spirv_SatConvertUToS_Rchar3(ulong3 x);
char4 __attribute__((overloadable))    __spirv_SatConvertUToS_Rchar4(ulong4 x);
char8 __attribute__((overloadable))    __spirv_SatConvertUToS_Rchar8(ulong8 x);
char16 __attribute__((overloadable))   __spirv_SatConvertUToS_Rchar16(ulong16 x);
short2 __attribute__((overloadable))   __spirv_SatConvertUToS_Rshort2(uchar2 x);
short3 __attribute__((overloadable))   __spirv_SatConvertUToS_Rshort3(uchar3 x);
short4 __attribute__((overloadable))   __spirv_SatConvertUToS_Rshort4(uchar4 x);
short8 __attribute__((overloadable))   __spirv_SatConvertUToS_Rshort8(uchar8 x);
short16 __attribute__((overloadable))  __spirv_SatConvertUToS_Rshort16(uchar16 x);
short2 __attribute__((overloadable))   __spirv_SatConvertUToS_Rshort2(ushort2 x);
short3 __attribute__((overloadable))   __spirv_SatConvertUToS_Rshort3(ushort3 x);
short4 __attribute__((overloadable))   __spirv_SatConvertUToS_Rshort4(ushort4 x);
short8 __attribute__((overloadable))   __spirv_SatConvertUToS_Rshort8(ushort8 x);
short16 __attribute__((overloadable))  __spirv_SatConvertUToS_Rshort16(ushort16 x);
short2 __attribute__((overloadable))   __spirv_SatConvertUToS_Rshort2(uint2 x);
short3 __attribute__((overloadable))   __spirv_SatConvertUToS_Rshort3(uint3 x);
short4 __attribute__((overloadable))   __spirv_SatConvertUToS_Rshort4(uint4 x);
short8 __attribute__((overloadable))   __spirv_SatConvertUToS_Rshort8(uint8 x);
short16 __attribute__((overloadable))  __spirv_SatConvertUToS_Rshort16(uint16 x);
short2 __attribute__((overloadable))   __spirv_SatConvertUToS_Rshort2(ulong2 x);
short3 __attribute__((overloadable))   __spirv_SatConvertUToS_Rshort3(ulong3 x);
short4 __attribute__((overloadable))   __spirv_SatConvertUToS_Rshort4(ulong4 x);
short8 __attribute__((overloadable))   __spirv_SatConvertUToS_Rshort8(ulong8 x);
short16 __attribute__((overloadable))  __spirv_SatConvertUToS_Rshort16(ulong16 x);
int2 __attribute__((overloadable))     __spirv_SatConvertUToS_Rint2(uchar2 x);
int3 __attribute__((overloadable))     __spirv_SatConvertUToS_Rint3(uchar3 x);
int4 __attribute__((overloadable))     __spirv_SatConvertUToS_Rint4(uchar4 x);
int8 __attribute__((overloadable))     __spirv_SatConvertUToS_Rint8(uchar8 x);
int16 __attribute__((overloadable))    __spirv_SatConvertUToS_Rint16(uchar16 x);
int2 __attribute__((overloadable))     __spirv_SatConvertUToS_Rint2(ushort2 x);
int3 __attribute__((overloadable))     __spirv_SatConvertUToS_Rint3(ushort3 x);
int4 __attribute__((overloadable))     __spirv_SatConvertUToS_Rint4(ushort4 x);
int8 __attribute__((overloadable))     __spirv_SatConvertUToS_Rint8(ushort8 x);
int16 __attribute__((overloadable))    __spirv_SatConvertUToS_Rint16(ushort16 x);
int2 __attribute__((overloadable))     __spirv_SatConvertUToS_Rint2(uint2 x);
int3 __attribute__((overloadable))     __spirv_SatConvertUToS_Rint3(uint3 x);
int4 __attribute__((overloadable))     __spirv_SatConvertUToS_Rint4(uint4 x);
int8 __attribute__((overloadable))     __spirv_SatConvertUToS_Rint8(uint8 x);
int16 __attribute__((overloadable))    __spirv_SatConvertUToS_Rint16(uint16 x);
int2 __attribute__((overloadable))     __spirv_SatConvertUToS_Rint2(ulong2 x);
int3 __attribute__((overloadable))     __spirv_SatConvertUToS_Rint3(ulong3 x);
int4 __attribute__((overloadable))     __spirv_SatConvertUToS_Rint4(ulong4 x);
int8 __attribute__((overloadable))     __spirv_SatConvertUToS_Rint8(ulong8 x);
int16 __attribute__((overloadable))    __spirv_SatConvertUToS_Rint16(ulong16 x);
long2 __attribute__((overloadable))    __spirv_SatConvertUToS_Rlong2(uchar2 x);
long3 __attribute__((overloadable))    __spirv_SatConvertUToS_Rlong3(uchar3 x);
long4 __attribute__((overloadable))    __spirv_SatConvertUToS_Rlong4(uchar4 x);
long8 __attribute__((overloadable))    __spirv_SatConvertUToS_Rlong8(uchar8 x);
long16 __attribute__((overloadable))   __spirv_SatConvertUToS_Rlong16(uchar16 x);
long2 __attribute__((overloadable))    __spirv_SatConvertUToS_Rlong2(ushort2 x);
long3 __attribute__((overloadable))    __spirv_SatConvertUToS_Rlong3(ushort3 x);
long4 __attribute__((overloadable))    __spirv_SatConvertUToS_Rlong4(ushort4 x);
long8 __attribute__((overloadable))    __spirv_SatConvertUToS_Rlong8(ushort8 x);
long16 __attribute__((overloadable))   __spirv_SatConvertUToS_Rlong16(ushort16 x);
long2 __attribute__((overloadable))    __spirv_SatConvertUToS_Rlong2(uint2 x);
long3 __attribute__((overloadable))    __spirv_SatConvertUToS_Rlong3(uint3 x);
long4 __attribute__((overloadable))    __spirv_SatConvertUToS_Rlong4(uint4 x);
long8 __attribute__((overloadable))    __spirv_SatConvertUToS_Rlong8(uint8 x);
long16 __attribute__((overloadable))   __spirv_SatConvertUToS_Rlong16(uint16 x);
long2 __attribute__((overloadable))    __spirv_SatConvertUToS_Rlong2(ulong2 x);
long3 __attribute__((overloadable))    __spirv_SatConvertUToS_Rlong3(ulong3 x);
long4 __attribute__((overloadable))    __spirv_SatConvertUToS_Rlong4(ulong4 x);
long8 __attribute__((overloadable))    __spirv_SatConvertUToS_Rlong8(ulong8 x);
long16 __attribute__((overloadable))   __spirv_SatConvertUToS_Rlong16(ulong16 x);
short __attribute__((overloadable))    __spirv_ConvertFToBF16INTEL(float x);
short2 __attribute__((overloadable))   __spirv_ConvertFToBF16INTEL(float2 x);
short3 __attribute__((overloadable))   __spirv_ConvertFToBF16INTEL(float3 x);
short4 __attribute__((overloadable))   __spirv_ConvertFToBF16INTEL(float4 x);
short8 __attribute__((overloadable))   __spirv_ConvertFToBF16INTEL(float8 x);
short16 __attribute__((overloadable))  __spirv_ConvertFToBF16INTEL(float16 x);
float __attribute__((overloadable))    __spirv_ConvertBF16ToFINTEL(short x);
float2 __attribute__((overloadable))   __spirv_ConvertBF16ToFINTEL(short2 x);
float3 __attribute__((overloadable))   __spirv_ConvertBF16ToFINTEL(short3 x);
float4 __attribute__((overloadable))   __spirv_ConvertBF16ToFINTEL(short4 x);
float8 __attribute__((overloadable))   __spirv_ConvertBF16ToFINTEL(short8 x);
float16 __attribute__((overloadable))  __spirv_ConvertBF16ToFINTEL(short16 x);

float __attribute__((overloadable))   __spirv_RoundFToTF32INTEL(float x);
float2 __attribute__((overloadable))  __spirv_RoundFToTF32INTEL(float2 x);
float3 __attribute__((overloadable))  __spirv_RoundFToTF32INTEL(float3 x);
float4 __attribute__((overloadable))  __spirv_RoundFToTF32INTEL(float4 x);
float8 __attribute__((overloadable))  __spirv_RoundFToTF32INTEL(float8 x);
float16 __attribute__((overloadable)) __spirv_RoundFToTF32INTEL(float16 x);

// lfsr scalar
char __attribute__((overloadable))  __spirv_GaloisLFSRINTEL(char seed, char polynomial);
short __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(short seed, short polynomial);
int __attribute__((overloadable))   __spirv_GaloisLFSRINTEL(int seed, int polynomial);
long __attribute__((overloadable))  __spirv_GaloisLFSRINTEL(long seed, long polynomial);
// lfsr vec2
char2 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(char2 seed, char polynomial);
short2 __attribute__((overloadable))
                                   __spirv_GaloisLFSRINTEL(short2 seed, short polynomial);
int2 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(int2 seed, int polynomial);
// lfsr vec3
char3 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(char3 seed, char polynomial);
short3 __attribute__((overloadable))
                                   __spirv_GaloisLFSRINTEL(short3 seed, short polynomial);
int3 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(int3 seed, int polynomial);
// lfsr vec4
char4 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(char4 seed, char polynomial);
short4 __attribute__((overloadable))
                                   __spirv_GaloisLFSRINTEL(short4 seed, short polynomial);
int4 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(int4 seed, int polynomial);
long4 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(long4 seed, long polynomial);
// lfsr vec8
char8 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(char8 seed, char polynomial);
short8 __attribute__((overloadable))
                                   __spirv_GaloisLFSRINTEL(short8 seed, short polynomial);
int8 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(int8 seed, int polynomial);
long8 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(long8 seed, long polynomial);

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
private
void *__attribute__((overloadable))
__spirv_GenericCastToPtrExplicit_ToPrivate(generic char *Pointer, int Storage);
local void *__attribute__((overloadable))
__spirv_GenericCastToPtrExplicit_ToLocal(generic char *Pointer, int Storage);
global void *__attribute__((overloadable))
__spirv_GenericCastToPtrExplicit_ToGlobal(generic char *Pointer, int Storage);
uint __attribute__((overloadable)) __spirv_GenericPtrMemSemantics(generic char *Pointer);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

// Arithmetic Instructions

half __attribute__((overloadable))  __spirv_Dot(half2 Vector1, half2 Vector2);
half __attribute__((overloadable))  __spirv_Dot(half3 Vector1, half3 Vector2);
half __attribute__((overloadable))  __spirv_Dot(half4 Vector1, half4 Vector2);
half __attribute__((overloadable))  __spirv_Dot(half8 Vector1, half8 Vector2);
half __attribute__((overloadable))  __spirv_Dot(half16 Vector1, half16 Vector2);
float __attribute__((overloadable)) __spirv_Dot(float2 Vector1, float2 Vector2);
float __attribute__((overloadable)) __spirv_Dot(float3 Vector1, float3 Vector2);
float __attribute__((overloadable)) __spirv_Dot(float4 Vector1, float4 Vector2);
float __attribute__((overloadable)) __spirv_Dot(float8 Vector1, float8 Vector2);
float __attribute__((overloadable)) __spirv_Dot(float16 Vector1, float16 Vector2);
#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_Dot(double2 Vector1, double2 Vector2);
double __attribute__((overloadable)) __spirv_Dot(double3 Vector1, double3 Vector2);
double __attribute__((overloadable)) __spirv_Dot(double4 Vector1, double4 Vector2);
double __attribute__((overloadable)) __spirv_Dot(double8 Vector1, double8 Vector2);
double __attribute__((overloadable)) __spirv_Dot(double16 Vector1, double16 Vector2);
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

TwoOp_i8 __attribute__((overloadable))
__spirv_UMulExtended(uchar Operand1, uchar Operand2);
TwoOp_v2i8 __attribute__((overloadable))
__spirv_UMulExtended(uchar2 Operand1, uchar2 Operand2);
TwoOp_v3i8 __attribute__((overloadable))
__spirv_UMulExtended(uchar3 Operand1, uchar3 Operand2);
TwoOp_v4i8 __attribute__((overloadable))
__spirv_UMulExtended(uchar4 Operand1, uchar4 Operand2);
TwoOp_v8i8 __attribute__((overloadable))
__spirv_UMulExtended(uchar8 Operand1, uchar8 Operand2);
TwoOp_v16i8 __attribute__((overloadable))
__spirv_UMulExtended(uchar16 Operand1, uchar16 Operand2);
TwoOp_i16 __attribute__((overloadable))
__spirv_UMulExtended(ushort Operand1, ushort Operand2);
TwoOp_v2i16 __attribute__((overloadable))
__spirv_UMulExtended(ushort2 Operand1, ushort2 Operand2);
TwoOp_v3i16 __attribute__((overloadable))
__spirv_UMulExtended(ushort3 Operand1, ushort3 Operand2);
TwoOp_v4i16 __attribute__((overloadable))
__spirv_UMulExtended(ushort4 Operand1, ushort4 Operand2);
TwoOp_v8i16 __attribute__((overloadable))
__spirv_UMulExtended(ushort8 Operand1, ushort8 Operand2);
TwoOp_v16i16 __attribute__((overloadable))
__spirv_UMulExtended(ushort16 Operand1, ushort16 Operand2);
TwoOp_i32 __attribute__((overloadable))
__spirv_UMulExtended(uint Operand1, uint Operand2);
TwoOp_v2i32 __attribute__((overloadable))
__spirv_UMulExtended(uint2 Operand1, uint2 Operand2);
TwoOp_v3i32 __attribute__((overloadable))
__spirv_UMulExtended(uint3 Operand1, uint3 Operand2);
TwoOp_v4i32 __attribute__((overloadable))
__spirv_UMulExtended(uint4 Operand1, uint4 Operand2);
TwoOp_v8i32 __attribute__((overloadable))
__spirv_UMulExtended(uint8 Operand1, uint8 Operand2);
TwoOp_v16i32 __attribute__((overloadable))
__spirv_UMulExtended(uint16 Operand1, uint16 Operand2);
TwoOp_i64 __attribute__((overloadable))
__spirv_UMulExtended(ulong Operand1, ulong Operand2);
TwoOp_v2i64 __attribute__((overloadable))
__spirv_UMulExtended(ulong2 Operand1, ulong2 Operand2);
TwoOp_v3i64 __attribute__((overloadable))
__spirv_UMulExtended(ulong3 Operand1, ulong3 Operand2);
TwoOp_v4i64 __attribute__((overloadable))
__spirv_UMulExtended(ulong4 Operand1, ulong4 Operand2);
TwoOp_v8i64 __attribute__((overloadable))
__spirv_UMulExtended(ulong8 Operand1, ulong8 Operand2);
TwoOp_v16i64 __attribute__((overloadable))
__spirv_UMulExtended(ulong16 Operand1, ulong16 Operand2);

TwoOp_i8 __attribute__((overloadable)) __spirv_SMulExtended(char Operand1, char Operand2);
TwoOp_v2i8 __attribute__((overloadable))
__spirv_SMulExtended(char2 Operand1, char2 Operand2);
TwoOp_v3i8 __attribute__((overloadable))
__spirv_SMulExtended(char3 Operand1, char3 Operand2);
TwoOp_v4i8 __attribute__((overloadable))
__spirv_SMulExtended(char4 Operand1, char4 Operand2);
TwoOp_v8i8 __attribute__((overloadable))
__spirv_SMulExtended(char8 Operand1, char8 Operand2);
TwoOp_v16i8 __attribute__((overloadable))
__spirv_SMulExtended(char16 Operand1, char16 Operand2);
TwoOp_i16 __attribute__((overloadable))
__spirv_SMulExtended(short Operand1, short Operand2);
TwoOp_v2i16 __attribute__((overloadable))
__spirv_SMulExtended(short2 Operand1, short2 Operand2);
TwoOp_v3i16 __attribute__((overloadable))
__spirv_SMulExtended(short3 Operand1, short3 Operand2);
TwoOp_v4i16 __attribute__((overloadable))
__spirv_SMulExtended(short4 Operand1, short4 Operand2);
TwoOp_v8i16 __attribute__((overloadable))
__spirv_SMulExtended(short8 Operand1, short8 Operand2);
TwoOp_v16i16 __attribute__((overloadable))
__spirv_SMulExtended(short16 Operand1, short16 Operand2);
TwoOp_i32 __attribute__((overloadable)) __spirv_SMulExtended(int Operand1, int Operand2);
TwoOp_v2i32 __attribute__((overloadable))
__spirv_SMulExtended(int2 Operand1, int2 Operand2);
TwoOp_v3i32 __attribute__((overloadable))
__spirv_SMulExtended(int3 Operand1, int3 Operand2);
TwoOp_v4i32 __attribute__((overloadable))
__spirv_SMulExtended(int4 Operand1, int4 Operand2);
TwoOp_v8i32 __attribute__((overloadable))
__spirv_SMulExtended(int8 Operand1, int8 Operand2);
TwoOp_v16i32 __attribute__((overloadable))
__spirv_SMulExtended(int16 Operand1, int16 Operand2);
TwoOp_i64 __attribute__((overloadable))
__spirv_SMulExtended(long Operand1, long Operand2);
TwoOp_v2i64 __attribute__((overloadable))
__spirv_SMulExtended(long2 Operand1, long2 Operand2);
TwoOp_v3i64 __attribute__((overloadable))
__spirv_SMulExtended(long3 Operand1, long3 Operand2);
TwoOp_v4i64 __attribute__((overloadable))
__spirv_SMulExtended(long4 Operand1, long4 Operand2);
TwoOp_v8i64 __attribute__((overloadable))
__spirv_SMulExtended(long8 Operand1, long8 Operand2);
TwoOp_v16i64 __attribute__((overloadable))
__spirv_SMulExtended(long16 Operand1, long16 Operand2);

// Bit Instructions

char __attribute__((overloadable))
__spirv_BitFieldInsert(char Base, char Insert, uint Offset, uint Count);
char2 __attribute__((overloadable))
__spirv_BitFieldInsert(char2 Base, char2 Insert, uint Offset, uint Count);
char3 __attribute__((overloadable))
__spirv_BitFieldInsert(char3 Base, char3 Insert, uint Offset, uint Count);
char4 __attribute__((overloadable))
__spirv_BitFieldInsert(char4 Base, char4 Insert, uint Offset, uint Count);
char8 __attribute__((overloadable))
__spirv_BitFieldInsert(char8 Base, char8 Insert, uint Offset, uint Count);
char16 __attribute__((overloadable))
__spirv_BitFieldInsert(char16 Base, char16 Insert, uint Offset, uint Count);
short __attribute__((overloadable))
__spirv_BitFieldInsert(short Base, short Insert, uint Offset, uint Count);
short2 __attribute__((overloadable))
__spirv_BitFieldInsert(short2 Base, short2 Insert, uint Offset, uint Count);
short3 __attribute__((overloadable))
__spirv_BitFieldInsert(short3 Base, short3 Insert, uint Offset, uint Count);
short4 __attribute__((overloadable))
__spirv_BitFieldInsert(short4 Base, short4 Insert, uint Offset, uint Count);
short8 __attribute__((overloadable))
__spirv_BitFieldInsert(short8 Base, short8 Insert, uint Offset, uint Count);
short16 __attribute__((overloadable))
__spirv_BitFieldInsert(short16 Base, short16 Insert, uint Offset, uint Count);
int __attribute__((overloadable))
__spirv_BitFieldInsert(int Base, int Insert, uint Offset, uint Count);
int2 __attribute__((overloadable))
__spirv_BitFieldInsert(int2 Base, int2 Insert, uint Offset, uint Count);
int3 __attribute__((overloadable))
__spirv_BitFieldInsert(int3 Base, int3 Insert, uint Offset, uint Count);
int4 __attribute__((overloadable))
__spirv_BitFieldInsert(int4 Base, int4 Insert, uint Offset, uint Count);
int8 __attribute__((overloadable))
__spirv_BitFieldInsert(int8 Base, int8 Insert, uint Offset, uint Count);
int16 __attribute__((overloadable))
__spirv_BitFieldInsert(int16 Base, int16 Insert, uint Offset, uint Count);
long __attribute__((overloadable))
__spirv_BitFieldInsert(long Base, long Insert, uint Offset, uint Count);
long2 __attribute__((overloadable))
__spirv_BitFieldInsert(long2 Base, long2 Insert, uint Offset, uint Count);
long3 __attribute__((overloadable))
__spirv_BitFieldInsert(long3 Base, long3 Insert, uint Offset, uint Count);
long4 __attribute__((overloadable))
__spirv_BitFieldInsert(long4 Base, long4 Insert, uint Offset, uint Count);
long8 __attribute__((overloadable))
__spirv_BitFieldInsert(long8 Base, long8 Insert, uint Offset, uint Count);
long16 __attribute__((overloadable))
__spirv_BitFieldInsert(long16 Base, long16 Insert, uint Offset, uint Count);

char __attribute__((overloadable))
__spirv_BitFieldSExtract(char Base, uint Offset, uint Count);
char2 __attribute__((overloadable))
__spirv_BitFieldSExtract(char2 Base, uint Offset, uint Count);
char3 __attribute__((overloadable))
__spirv_BitFieldSExtract(char3 Base, uint Offset, uint Count);
char4 __attribute__((overloadable))
__spirv_BitFieldSExtract(char4 Base, uint Offset, uint Count);
char8 __attribute__((overloadable))
__spirv_BitFieldSExtract(char8 Base, uint Offset, uint Count);
char16 __attribute__((overloadable))
__spirv_BitFieldSExtract(char16 Base, uint Offset, uint Count);
short __attribute__((overloadable))
__spirv_BitFieldSExtract(short Base, uint Offset, uint Count);
short2 __attribute__((overloadable))
__spirv_BitFieldSExtract(short2 Base, uint Offset, uint Count);
short3 __attribute__((overloadable))
__spirv_BitFieldSExtract(short3 Base, uint Offset, uint Count);
short4 __attribute__((overloadable))
__spirv_BitFieldSExtract(short4 Base, uint Offset, uint Count);
short8 __attribute__((overloadable))
__spirv_BitFieldSExtract(short8 Base, uint Offset, uint Count);
short16 __attribute__((overloadable))
__spirv_BitFieldSExtract(short16 Base, uint Offset, uint Count);
int __attribute__((overloadable))
__spirv_BitFieldSExtract(int Base, uint Offset, uint Count);
int2 __attribute__((overloadable))
__spirv_BitFieldSExtract(int2 Base, uint Offset, uint Count);
int3 __attribute__((overloadable))
__spirv_BitFieldSExtract(int3 Base, uint Offset, uint Count);
int4 __attribute__((overloadable))
__spirv_BitFieldSExtract(int4 Base, uint Offset, uint Count);
int8 __attribute__((overloadable))
__spirv_BitFieldSExtract(int8 Base, uint Offset, uint Count);
int16 __attribute__((overloadable))
__spirv_BitFieldSExtract(int16 Base, uint Offset, uint Count);
long __attribute__((overloadable))
__spirv_BitFieldSExtract(long Base, uint Offset, uint Count);
long2 __attribute__((overloadable))
__spirv_BitFieldSExtract(long2 Base, uint Offset, uint Count);
long3 __attribute__((overloadable))
__spirv_BitFieldSExtract(long3 Base, uint Offset, uint Count);
long4 __attribute__((overloadable))
__spirv_BitFieldSExtract(long4 Base, uint Offset, uint Count);
long8 __attribute__((overloadable))
__spirv_BitFieldSExtract(long8 Base, uint Offset, uint Count);
long16 __attribute__((overloadable))
__spirv_BitFieldSExtract(long16 Base, uint Offset, uint Count);

uchar __attribute__((overloadable))
__spirv_BitFieldUExtract(uchar Base, uint Offset, uint Count);
uchar2 __attribute__((overloadable))
__spirv_BitFieldUExtract(uchar2 Base, uint Offset, uint Count);
uchar3 __attribute__((overloadable))
__spirv_BitFieldUExtract(uchar3 Base, uint Offset, uint Count);
uchar4 __attribute__((overloadable))
__spirv_BitFieldUExtract(uchar4 Base, uint Offset, uint Count);
uchar8 __attribute__((overloadable))
__spirv_BitFieldUExtract(uchar8 Base, uint Offset, uint Count);
uchar16 __attribute__((overloadable))
__spirv_BitFieldUExtract(uchar16 Base, uint Offset, uint Count);
ushort __attribute__((overloadable))
__spirv_BitFieldUExtract(ushort Base, uint Offset, uint Count);
ushort2 __attribute__((overloadable))
__spirv_BitFieldUExtract(ushort2 Base, uint Offset, uint Count);
ushort3 __attribute__((overloadable))
__spirv_BitFieldUExtract(ushort3 Base, uint Offset, uint Count);
ushort4 __attribute__((overloadable))
__spirv_BitFieldUExtract(ushort4 Base, uint Offset, uint Count);
ushort8 __attribute__((overloadable))
__spirv_BitFieldUExtract(ushort8 Base, uint Offset, uint Count);
ushort16 __attribute__((overloadable))
__spirv_BitFieldUExtract(ushort16 Base, uint Offset, uint Count);
uint __attribute__((overloadable))
__spirv_BitFieldUExtract(uint Base, uint Offset, uint Count);
uint2 __attribute__((overloadable))
__spirv_BitFieldUExtract(uint2 Base, uint Offset, uint Count);
uint3 __attribute__((overloadable))
__spirv_BitFieldUExtract(uint3 Base, uint Offset, uint Count);
uint4 __attribute__((overloadable))
__spirv_BitFieldUExtract(uint4 Base, uint Offset, uint Count);
uint8 __attribute__((overloadable))
__spirv_BitFieldUExtract(uint8 Base, uint Offset, uint Count);
uint16 __attribute__((overloadable))
__spirv_BitFieldUExtract(uint16 Base, uint Offset, uint Count);
ulong __attribute__((overloadable))
__spirv_BitFieldUExtract(ulong Base, uint Offset, uint Count);
ulong2 __attribute__((overloadable))
__spirv_BitFieldUExtract(ulong2 Base, uint Offset, uint Count);
ulong3 __attribute__((overloadable))
__spirv_BitFieldUExtract(ulong3 Base, uint Offset, uint Count);
ulong4 __attribute__((overloadable))
__spirv_BitFieldUExtract(ulong4 Base, uint Offset, uint Count);
ulong8 __attribute__((overloadable))
__spirv_BitFieldUExtract(ulong8 Base, uint Offset, uint Count);
ulong16 __attribute__((overloadable))
__spirv_BitFieldUExtract(ulong16 Base, uint Offset, uint Count);

char __attribute__((overloadable))    __spirv_BitReverse(char Base);
char2 __attribute__((overloadable))   __spirv_BitReverse(char2 Base);
char3 __attribute__((overloadable))   __spirv_BitReverse(char3 Base);
char4 __attribute__((overloadable))   __spirv_BitReverse(char4 Base);
char8 __attribute__((overloadable))   __spirv_BitReverse(char8 Base);
char16 __attribute__((overloadable))  __spirv_BitReverse(char16 Base);
short __attribute__((overloadable))   __spirv_BitReverse(short Base);
short2 __attribute__((overloadable))  __spirv_BitReverse(short2 Base);
short3 __attribute__((overloadable))  __spirv_BitReverse(short3 Base);
short4 __attribute__((overloadable))  __spirv_BitReverse(short4 Base);
short8 __attribute__((overloadable))  __spirv_BitReverse(short8 Base);
short16 __attribute__((overloadable)) __spirv_BitReverse(short16 Base);
int __attribute__((overloadable))     __spirv_BitReverse(int Base);
int2 __attribute__((overloadable))    __spirv_BitReverse(int2 Base);
int3 __attribute__((overloadable))    __spirv_BitReverse(int3 Base);
int4 __attribute__((overloadable))    __spirv_BitReverse(int4 Base);
int8 __attribute__((overloadable))    __spirv_BitReverse(int8 Base);
int16 __attribute__((overloadable))   __spirv_BitReverse(int16 Base);
long __attribute__((overloadable))    __spirv_BitReverse(long Base);
long2 __attribute__((overloadable))   __spirv_BitReverse(long2 Base);
long3 __attribute__((overloadable))   __spirv_BitReverse(long3 Base);
long4 __attribute__((overloadable))   __spirv_BitReverse(long4 Base);
long8 __attribute__((overloadable))   __spirv_BitReverse(long8 Base);
long16 __attribute__((overloadable))  __spirv_BitReverse(long16 Base);

uchar __attribute__((overloadable))    __spirv_BitCount(char Base);
uchar2 __attribute__((overloadable))   __spirv_BitCount(char2 Base);
uchar3 __attribute__((overloadable))   __spirv_BitCount(char3 Base);
uchar4 __attribute__((overloadable))   __spirv_BitCount(char4 Base);
uchar8 __attribute__((overloadable))   __spirv_BitCount(char8 Base);
uchar16 __attribute__((overloadable))  __spirv_BitCount(char16 Base);
ushort __attribute__((overloadable))   __spirv_BitCount(short Base);
ushort2 __attribute__((overloadable))  __spirv_BitCount(short2 Base);
ushort3 __attribute__((overloadable))  __spirv_BitCount(short3 Base);
ushort4 __attribute__((overloadable))  __spirv_BitCount(short4 Base);
ushort8 __attribute__((overloadable))  __spirv_BitCount(short8 Base);
ushort16 __attribute__((overloadable)) __spirv_BitCount(short16 Base);
uint __attribute__((overloadable))     __spirv_BitCount(int Base);
uint2 __attribute__((overloadable))    __spirv_BitCount(int2 Base);
uint3 __attribute__((overloadable))    __spirv_BitCount(int3 Base);
uint4 __attribute__((overloadable))    __spirv_BitCount(int4 Base);
uint8 __attribute__((overloadable))    __spirv_BitCount(int8 Base);
uint16 __attribute__((overloadable))   __spirv_BitCount(int16 Base);
ulong __attribute__((overloadable))    __spirv_BitCount(long Base);
ulong2 __attribute__((overloadable))   __spirv_BitCount(long2 Base);
ulong3 __attribute__((overloadable))   __spirv_BitCount(long3 Base);
ulong4 __attribute__((overloadable))   __spirv_BitCount(long4 Base);
ulong8 __attribute__((overloadable))   __spirv_BitCount(long8 Base);
ulong16 __attribute__((overloadable))  __spirv_BitCount(long16 Base);

// Relational and Logical Instructions

// Our OpenCL C builtins currently do not call this
bool __attribute__((overloadable)) __spirv_Any(__bool2 Vector);
bool __attribute__((overloadable)) __spirv_Any(__bool3 Vector);
bool __attribute__((overloadable)) __spirv_Any(__bool4 Vector);
bool __attribute__((overloadable)) __spirv_Any(__bool8 Vector);
bool __attribute__((overloadable)) __spirv_Any(__bool16 Vector);

// Our OpenCL C builtins currently do not call this
bool __attribute__((overloadable)) __spirv_All(__bool2 Vector);
bool __attribute__((overloadable)) __spirv_All(__bool3 Vector);
bool __attribute__((overloadable)) __spirv_All(__bool4 Vector);
bool __attribute__((overloadable)) __spirv_All(__bool8 Vector);
bool __attribute__((overloadable)) __spirv_All(__bool16 Vector);

bool __attribute__((overloadable))     __spirv_IsNan(half x);
bool __attribute__((overloadable))     __spirv_IsNan(float x);
__bool2 __attribute__((overloadable))  __spirv_IsNan(half2 x);
__bool2 __attribute__((overloadable))  __spirv_IsNan(float2 x);
__bool3 __attribute__((overloadable))  __spirv_IsNan(half3 x);
__bool3 __attribute__((overloadable))  __spirv_IsNan(float3 x);
__bool4 __attribute__((overloadable))  __spirv_IsNan(half4 x);
__bool4 __attribute__((overloadable))  __spirv_IsNan(float4 x);
__bool8 __attribute__((overloadable))  __spirv_IsNan(half8 x);
__bool8 __attribute__((overloadable))  __spirv_IsNan(float8 x);
__bool16 __attribute__((overloadable)) __spirv_IsNan(half16 x);
__bool16 __attribute__((overloadable)) __spirv_IsNan(float16 x);
#if defined(cl_khr_fp64)
bool __attribute__((overloadable))     __spirv_IsNan(double x);
__bool2 __attribute__((overloadable))  __spirv_IsNan(double2 x);
__bool3 __attribute__((overloadable))  __spirv_IsNan(double3 x);
__bool4 __attribute__((overloadable))  __spirv_IsNan(double4 x);
__bool8 __attribute__((overloadable))  __spirv_IsNan(double8 x);
__bool16 __attribute__((overloadable)) __spirv_IsNan(double16 x);
#endif // defined(cl_khr_fp64)

bool __attribute__((overloadable))     __spirv_IsInf(half x);
bool __attribute__((overloadable))     __spirv_IsInf(float x);
__bool2 __attribute__((overloadable))  __spirv_IsInf(half2 x);
__bool2 __attribute__((overloadable))  __spirv_IsInf(float2 x);
__bool3 __attribute__((overloadable))  __spirv_IsInf(half3 x);
__bool3 __attribute__((overloadable))  __spirv_IsInf(float3 x);
__bool4 __attribute__((overloadable))  __spirv_IsInf(half4 x);
__bool4 __attribute__((overloadable))  __spirv_IsInf(float4 x);
__bool8 __attribute__((overloadable))  __spirv_IsInf(half8 x);
__bool8 __attribute__((overloadable))  __spirv_IsInf(float8 x);
__bool16 __attribute__((overloadable)) __spirv_IsInf(half16 x);
__bool16 __attribute__((overloadable)) __spirv_IsInf(float16 x);
#if defined(cl_khr_fp64)
bool __attribute__((overloadable))     __spirv_IsInf(double x);
__bool2 __attribute__((overloadable))  __spirv_IsInf(double2 x);
__bool3 __attribute__((overloadable))  __spirv_IsInf(double3 x);
__bool4 __attribute__((overloadable))  __spirv_IsInf(double4 x);
__bool8 __attribute__((overloadable))  __spirv_IsInf(double8 x);
__bool16 __attribute__((overloadable)) __spirv_IsInf(double16 x);
#endif // defined(cl_khr_fp64)

bool __attribute__((overloadable))     __spirv_IsFinite(half x);
bool __attribute__((overloadable))     __spirv_IsFinite(float x);
__bool2 __attribute__((overloadable))  __spirv_IsFinite(half2 x);
__bool2 __attribute__((overloadable))  __spirv_IsFinite(float2 x);
__bool3 __attribute__((overloadable))  __spirv_IsFinite(half3 x);
__bool3 __attribute__((overloadable))  __spirv_IsFinite(float3 x);
__bool4 __attribute__((overloadable))  __spirv_IsFinite(half4 x);
__bool4 __attribute__((overloadable))  __spirv_IsFinite(float4 x);
__bool8 __attribute__((overloadable))  __spirv_IsFinite(half8 x);
__bool8 __attribute__((overloadable))  __spirv_IsFinite(float8 x);
__bool16 __attribute__((overloadable)) __spirv_IsFinite(half16 x);
__bool16 __attribute__((overloadable)) __spirv_IsFinite(float16 x);
#if defined(cl_khr_fp64)
bool __attribute__((overloadable))     __spirv_IsFinite(double x);
__bool2 __attribute__((overloadable))  __spirv_IsFinite(double2 x);
__bool3 __attribute__((overloadable))  __spirv_IsFinite(double3 x);
__bool4 __attribute__((overloadable))  __spirv_IsFinite(double4 x);
__bool8 __attribute__((overloadable))  __spirv_IsFinite(double8 x);
__bool16 __attribute__((overloadable)) __spirv_IsFinite(double16 x);
#endif // defined(cl_khr_fp64)

bool __attribute__((overloadable))     __spirv_IsNormal(half x);
bool __attribute__((overloadable))     __spirv_IsNormal(float x);
__bool2 __attribute__((overloadable))  __spirv_IsNormal(half2 x);
__bool2 __attribute__((overloadable))  __spirv_IsNormal(float2 x);
__bool3 __attribute__((overloadable))  __spirv_IsNormal(half3 x);
__bool3 __attribute__((overloadable))  __spirv_IsNormal(float3 x);
__bool4 __attribute__((overloadable))  __spirv_IsNormal(half4 x);
__bool4 __attribute__((overloadable))  __spirv_IsNormal(float4 x);
__bool8 __attribute__((overloadable))  __spirv_IsNormal(half8 x);
__bool8 __attribute__((overloadable))  __spirv_IsNormal(float8 x);
__bool16 __attribute__((overloadable)) __spirv_IsNormal(half16 x);
__bool16 __attribute__((overloadable)) __spirv_IsNormal(float16 x);
#if defined(cl_khr_fp64)
bool __attribute__((overloadable))     __spirv_IsNormal(double x);
__bool2 __attribute__((overloadable))  __spirv_IsNormal(double2 x);
__bool3 __attribute__((overloadable))  __spirv_IsNormal(double3 x);
__bool4 __attribute__((overloadable))  __spirv_IsNormal(double4 x);
__bool8 __attribute__((overloadable))  __spirv_IsNormal(double8 x);
__bool16 __attribute__((overloadable)) __spirv_IsNormal(double16 x);
#endif // defined(cl_khr_fp64)

bool __attribute__((overloadable))     __spirv_SignBitSet(half x);
bool __attribute__((overloadable))     __spirv_SignBitSet(float x);
__bool2 __attribute__((overloadable))  __spirv_SignBitSet(half2 x);
__bool2 __attribute__((overloadable))  __spirv_SignBitSet(float2 x);
__bool3 __attribute__((overloadable))  __spirv_SignBitSet(half3 x);
__bool3 __attribute__((overloadable))  __spirv_SignBitSet(float3 x);
__bool4 __attribute__((overloadable))  __spirv_SignBitSet(half4 x);
__bool4 __attribute__((overloadable))  __spirv_SignBitSet(float4 x);
__bool8 __attribute__((overloadable))  __spirv_SignBitSet(half8 x);
__bool8 __attribute__((overloadable))  __spirv_SignBitSet(float8 x);
__bool16 __attribute__((overloadable)) __spirv_SignBitSet(half16 x);
__bool16 __attribute__((overloadable)) __spirv_SignBitSet(float16 x);
#if defined(cl_khr_fp64)
bool __attribute__((overloadable))     __spirv_SignBitSet(double x);
__bool2 __attribute__((overloadable))  __spirv_SignBitSet(double2 x);
__bool3 __attribute__((overloadable))  __spirv_SignBitSet(double3 x);
__bool4 __attribute__((overloadable))  __spirv_SignBitSet(double4 x);
__bool8 __attribute__((overloadable))  __spirv_SignBitSet(double8 x);
__bool16 __attribute__((overloadable)) __spirv_SignBitSet(double16 x);
#endif // defined(cl_khr_fp64)

bool __attribute__((overloadable))     __spirv_LessOrGreater(half x, half y);
bool __attribute__((overloadable))     __spirv_LessOrGreater(float x, float y);
__bool2 __attribute__((overloadable))  __spirv_LessOrGreater(half2 x, half2 y);
__bool2 __attribute__((overloadable))  __spirv_LessOrGreater(float2 x, float2 y);
__bool3 __attribute__((overloadable))  __spirv_LessOrGreater(half3 x, half3 y);
__bool3 __attribute__((overloadable))  __spirv_LessOrGreater(float3 x, float3 y);
__bool4 __attribute__((overloadable))  __spirv_LessOrGreater(half4 x, half4 y);
__bool4 __attribute__((overloadable))  __spirv_LessOrGreater(float4 x, float4 y);
__bool8 __attribute__((overloadable))  __spirv_LessOrGreater(half8 x, half8 y);
__bool8 __attribute__((overloadable))  __spirv_LessOrGreater(float8 x, float8 y);
__bool16 __attribute__((overloadable)) __spirv_LessOrGreater(half16 x, half16 y);
__bool16 __attribute__((overloadable)) __spirv_LessOrGreater(float16 x, float16 y);
#if defined(cl_khr_fp64)
bool __attribute__((overloadable))     __spirv_LessOrGreater(double x, double y);
__bool2 __attribute__((overloadable))  __spirv_LessOrGreater(double2 x, double2 y);
__bool3 __attribute__((overloadable))  __spirv_LessOrGreater(double3 x, double3 y);
__bool4 __attribute__((overloadable))  __spirv_LessOrGreater(double4 x, double4 y);
__bool8 __attribute__((overloadable))  __spirv_LessOrGreater(double8 x, double8 y);
__bool16 __attribute__((overloadable)) __spirv_LessOrGreater(double16 x, double16 y);
#endif // defined(cl_khr_fp64)

bool __attribute__((overloadable))     __spirv_Ordered(half x, half y);
bool __attribute__((overloadable))     __spirv_Ordered(float x, float y);
__bool2 __attribute__((overloadable))  __spirv_Ordered(half2 x, half2 y);
__bool2 __attribute__((overloadable))  __spirv_Ordered(float2 x, float2 y);
__bool3 __attribute__((overloadable))  __spirv_Ordered(half3 x, half3 y);
__bool3 __attribute__((overloadable))  __spirv_Ordered(float3 x, float3 y);
__bool4 __attribute__((overloadable))  __spirv_Ordered(half4 x, half4 y);
__bool4 __attribute__((overloadable))  __spirv_Ordered(float4 x, float4 y);
__bool8 __attribute__((overloadable))  __spirv_Ordered(half8 x, half8 y);
__bool8 __attribute__((overloadable))  __spirv_Ordered(float8 x, float8 y);
__bool16 __attribute__((overloadable)) __spirv_Ordered(half16 x, half16 y);
__bool16 __attribute__((overloadable)) __spirv_Ordered(float16 x, float16 y);
#if defined(cl_khr_fp64)
bool __attribute__((overloadable))     __spirv_Ordered(double x, double y);
__bool2 __attribute__((overloadable))  __spirv_Ordered(double2 x, double2 y);
__bool3 __attribute__((overloadable))  __spirv_Ordered(double3 x, double3 y);
__bool4 __attribute__((overloadable))  __spirv_Ordered(double4 x, double4 y);
__bool8 __attribute__((overloadable))  __spirv_Ordered(double8 x, double8 y);
__bool16 __attribute__((overloadable)) __spirv_Ordered(double16 x, double16 y);
#endif // defined(cl_khr_fp64)

bool __attribute__((overloadable))     __spirv_Unordered(half x, half y);
bool __attribute__((overloadable))     __spirv_Unordered(float x, float y);
__bool2 __attribute__((overloadable))  __spirv_Unordered(half2 x, half2 y);
__bool2 __attribute__((overloadable))  __spirv_Unordered(float2 x, float2 y);
__bool3 __attribute__((overloadable))  __spirv_Unordered(half3 x, half3 y);
__bool3 __attribute__((overloadable))  __spirv_Unordered(float3 x, float3 y);
__bool4 __attribute__((overloadable))  __spirv_Unordered(half4 x, half4 y);
__bool4 __attribute__((overloadable))  __spirv_Unordered(float4 x, float4 y);
__bool8 __attribute__((overloadable))  __spirv_Unordered(half8 x, half8 y);
__bool8 __attribute__((overloadable))  __spirv_Unordered(float8 x, float8 y);
__bool16 __attribute__((overloadable)) __spirv_Unordered(half16 x, half16 y);
__bool16 __attribute__((overloadable)) __spirv_Unordered(float16 x, float16 y);
#if defined(cl_khr_fp64)
bool __attribute__((overloadable))     __spirv_Unordered(double x, double y);
__bool2 __attribute__((overloadable))  __spirv_Unordered(double2 x, double2 y);
__bool3 __attribute__((overloadable))  __spirv_Unordered(double3 x, double3 y);
__bool4 __attribute__((overloadable))  __spirv_Unordered(double4 x, double4 y);
__bool8 __attribute__((overloadable))  __spirv_Unordered(double8 x, double8 y);
__bool16 __attribute__((overloadable)) __spirv_Unordered(double16 x, double16 y);
#endif // defined(cl_khr_fp64)

// Atomic Instructions

int __attribute__((overloadable))
__spirv_AtomicLoad(private int *Pointer, int Scope, int Semantics);
int __attribute__((overloadable))
__spirv_AtomicLoad(global int *Pointer, int Scope, int Semantics);
int __attribute__((overloadable))
__spirv_AtomicLoad(local int *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int __attribute__((overloadable))
__spirv_AtomicLoad(generic int *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
long __attribute__((overloadable))
__spirv_AtomicLoad(private long *Pointer, int Scope, int Semantics);
long __attribute__((overloadable))
__spirv_AtomicLoad(global long *Pointer, int Scope, int Semantics);
long __attribute__((overloadable))
__spirv_AtomicLoad(local long *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long __attribute__((overloadable))
__spirv_AtomicLoad(generic long *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

#if defined(cl_intel_bfloat16_atomics)
short __attribute__((overloadable))
__spirv_AtomicLoad(private short *Pointer, int Scope, int Semantics);
short __attribute__((overloadable))
__spirv_AtomicLoad(global short *Pointer, int Scope, int Semantics);
short __attribute__((overloadable))
__spirv_AtomicLoad(local short *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
short __attribute__((overloadable))
__spirv_AtomicLoad(generic short *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_intel_bfloat16_atomics)

float __attribute__((overloadable))
__spirv_AtomicLoad(private float *Pointer, int Scope, int Semantics);
float __attribute__((overloadable))
__spirv_AtomicLoad(global float *Pointer, int Scope, int Semantics);
float __attribute__((overloadable))
__spirv_AtomicLoad(local float *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __attribute__((overloadable))
__spirv_AtomicLoad(generic float *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
#if defined(cl_khr_int64_base_atomics)
double __attribute__((overloadable))
__spirv_AtomicLoad(private double *Pointer, int Scope, int Semantics);
double __attribute__((overloadable))
__spirv_AtomicLoad(global double *Pointer, int Scope, int Semantics);
double __attribute__((overloadable))
__spirv_AtomicLoad(local double *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __attribute__((overloadable))
__spirv_AtomicLoad(generic double *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
half __attribute__((overloadable))
__spirv_AtomicLoad(private half *Pointer, int Scope, int Semantics);
half __attribute__((overloadable))
__spirv_AtomicLoad(global half *Pointer, int Scope, int Semantics);
half __attribute__((overloadable))
__spirv_AtomicLoad(local half *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __attribute__((overloadable))
__spirv_AtomicLoad(generic half *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp16)

void __attribute__((overloadable))
__spirv_AtomicStore(private int *Pointer, int Scope, int Semantics, int Value);
void __attribute__((overloadable))
__spirv_AtomicStore(global int *Pointer, int Scope, int Semantics, int Value);
void __attribute__((overloadable))
__spirv_AtomicStore(local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void __attribute__((overloadable))
__spirv_AtomicStore(generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
void __attribute__((overloadable))
__spirv_AtomicStore(private long *Pointer, int Scope, int Semantics, long Value);
void __attribute__((overloadable))
__spirv_AtomicStore(global long *Pointer, int Scope, int Semantics, long Value);
void __attribute__((overloadable))
__spirv_AtomicStore(local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void __attribute__((overloadable))
__spirv_AtomicStore(generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

#if defined(cl_intel_bfloat16_atomics)
void __attribute__((overloadable))
__spirv_AtomicStore(private short *Pointer, int Scope, int Semantics, short Value);
void __attribute__((overloadable))
__spirv_AtomicStore(global short *Pointer, int Scope, int Semantics, short Value);
void __attribute__((overloadable))
__spirv_AtomicStore(local short *Pointer, int Scope, int Semantics, short Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void __attribute__((overloadable))
__spirv_AtomicStore(generic short *Pointer, int Scope, int Semantics, short Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_intel_bfloat16_atomics)

void __attribute__((overloadable))
__spirv_AtomicStore(private float *Pointer, int Scope, int Semantics, float Value);
void __attribute__((overloadable))
__spirv_AtomicStore(global float *Pointer, int Scope, int Semantics, float Value);
void __attribute__((overloadable))
__spirv_AtomicStore(local float *Pointer, int Scope, int Semantics, float Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void __attribute__((overloadable))
__spirv_AtomicStore(generic float *Pointer, int Scope, int Semantics, float Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
void __attribute__((overloadable))
__spirv_AtomicStore(private double *Pointer, int Scope, int Semantics, double Value);
void __attribute__((overloadable))
__spirv_AtomicStore(global double *Pointer, int Scope, int Semantics, double Value);
void __attribute__((overloadable))
__spirv_AtomicStore(local double *Pointer, int Scope, int Semantics, double Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void __attribute__((overloadable))
__spirv_AtomicStore(generic double *Pointer, int Scope, int Semantics, double Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
void __attribute__((overloadable))
__spirv_AtomicStore(private half *Pointer, int Scope, int Semantics, half Value);
void __attribute__((overloadable))
__spirv_AtomicStore(global half *Pointer, int Scope, int Semantics, half Value);
void __attribute__((overloadable))
__spirv_AtomicStore(local half *Pointer, int Scope, int Semantics, half Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void __attribute__((overloadable))
__spirv_AtomicStore(generic half *Pointer, int Scope, int Semantics, half Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp16)

int __attribute__((overloadable))
__spirv_AtomicExchange(private int *Pointer, int Scope, int Semantics, int Value);
int __attribute__((overloadable))
__spirv_AtomicExchange(global int *Pointer, int Scope, int Semantics, int Value);
int __attribute__((overloadable))
__spirv_AtomicExchange(local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int __attribute__((overloadable))
__spirv_AtomicExchange(generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long __attribute__((overloadable))
__spirv_AtomicExchange(private long *Pointer, int Scope, int Semantics, long Value);
long __attribute__((overloadable))
__spirv_AtomicExchange(global long *Pointer, int Scope, int Semantics, long Value);
long __attribute__((overloadable))
__spirv_AtomicExchange(local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long __attribute__((overloadable))
__spirv_AtomicExchange(generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

#if defined(cl_intel_bfloat16_atomics)
short __attribute__((overloadable))
__spirv_AtomicExchange(private short *Pointer, int Scope, int Semantics, short Value);
short __attribute__((overloadable))
__spirv_AtomicExchange(global short *Pointer, int Scope, int Semantics, short Value);
short __attribute__((overloadable))
__spirv_AtomicExchange(local short *Pointer, int Scope, int Semantics, short Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
short __attribute__((overloadable))
__spirv_AtomicExchange(generic short *Pointer, int Scope, int Semantics, short Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_intel_bfloat16_atomics)

float __attribute__((overloadable))
__spirv_AtomicExchange(private float *Pointer, int Scope, int Semantics, float Value);
float __attribute__((overloadable))
__spirv_AtomicExchange(global float *Pointer, int Scope, int Semantics, float Value);
float __attribute__((overloadable))
__spirv_AtomicExchange(local float *Pointer, int Scope, int Semantics, float Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __attribute__((overloadable))
__spirv_AtomicExchange(generic float *Pointer, int Scope, int Semantics, float Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#if defined(cl_khr_fp64)
#if defined(cl_khr_int64_base_atomics)
double __attribute__((overloadable))
__spirv_AtomicExchange(private double *Pointer, int Scope, int Semantics, double Value);
double __attribute__((overloadable))
__spirv_AtomicExchange(global double *Pointer, int Scope, int Semantics, double Value);
double __attribute__((overloadable))
__spirv_AtomicExchange(local double *Pointer, int Scope, int Semantics, double Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __attribute__((overloadable))
__spirv_AtomicExchange(generic double *Pointer, int Scope, int Semantics, double Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
half __attribute__((overloadable))
__spirv_AtomicExchange(__private half *Pointer, int Scope, int Semantics, half Value);
half __attribute__((overloadable))
__spirv_AtomicExchange(__global half *Pointer, int Scope, int Semantics, half Value);
half __attribute__((overloadable))
__spirv_AtomicExchange(__local half *Pointer, int Scope, int Semantics, half Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __attribute__((overloadable))
__spirv_AtomicExchange(__generic half *Pointer, int Scope, int Semantics, half Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp16)

int __attribute__((overloadable)) __spirv_AtomicCompareExchange(
    private int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
int __attribute__((overloadable)) __spirv_AtomicCompareExchange(
    global int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
int __attribute__((overloadable)) __spirv_AtomicCompareExchange(
    local int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int __attribute__((overloadable)) __spirv_AtomicCompareExchange(
    generic int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long __attribute__((overloadable)) __spirv_AtomicCompareExchange(
    private long *Pointer,
    int           Scope,
    int           Equal,
    int           Unequal,
    long          Value,
    long          Comparator);
long __attribute__((overloadable)) __spirv_AtomicCompareExchange(
    global long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator);
long __attribute__((overloadable)) __spirv_AtomicCompareExchange(
    local long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long __attribute__((overloadable)) __spirv_AtomicCompareExchange(
    generic long *Pointer,
    int           Scope,
    int           Equal,
    int           Unequal,
    long          Value,
    long          Comparator);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

float __attribute__((overloadable)) __spirv_AtomicCompareExchange(
    private float *Pointer,
    int            Scope,
    int            Equal,
    int            Unequal,
    float          Value,
    float          Comparator);
float __attribute__((overloadable)) __spirv_AtomicCompareExchange(
    global float *Pointer,
    int           Scope,
    int           Equal,
    int           Unequal,
    float         Value,
    float         Comparator);
float __attribute__((overloadable)) __spirv_AtomicCompareExchange(
    local float *Pointer,
    int          Scope,
    int          Equal,
    int          Unequal,
    float        Value,
    float        Comparator);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __attribute__((overloadable)) __spirv_AtomicCompareExchange(
    generic float *Pointer,
    int            Scope,
    int            Equal,
    int            Unequal,
    float          Value,
    float          Comparator);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

int __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak(
    private int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
int __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak(
    global int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
int __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak(
    local int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak(
    generic int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak(
    private long *Pointer,
    int           Scope,
    int           Equal,
    int           Unequal,
    long          Value,
    long          Comparator);
long __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak(
    global long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator);
long __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak(
    local long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long __attribute__((overloadable)) __spirv_AtomicCompareExchangeWeak(
    generic long *Pointer,
    int           Scope,
    int           Equal,
    int           Unequal,
    long          Value,
    long          Comparator);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

int __attribute__((overloadable))
__spirv_AtomicIIncrement(private int *Pointer, int Scope, int Semantics);
int __attribute__((overloadable))
__spirv_AtomicIIncrement(global int *Pointer, int Scope, int Semantics);
int __attribute__((overloadable))
__spirv_AtomicIIncrement(local int *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int __attribute__((overloadable))
__spirv_AtomicIIncrement(generic int *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long __attribute__((overloadable))
__spirv_AtomicIIncrement(private long *Pointer, int Scope, int Semantics);
long __attribute__((overloadable))
__spirv_AtomicIIncrement(global long *Pointer, int Scope, int Semantics);
long __attribute__((overloadable))
__spirv_AtomicIIncrement(local long *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long __attribute__((overloadable))
__spirv_AtomicIIncrement(generic long *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

int __attribute__((overloadable))
__spirv_AtomicIDecrement(private int *Pointer, int Scope, int Semantics);
int __attribute__((overloadable))
__spirv_AtomicIDecrement(global int *Pointer, int Scope, int Semantics);
int __attribute__((overloadable))
__spirv_AtomicIDecrement(local int *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int __attribute__((overloadable))
__spirv_AtomicIDecrement(generic int *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long __attribute__((overloadable))
__spirv_AtomicIDecrement(private long *Pointer, int Scope, int Semantics);
long __attribute__((overloadable))
__spirv_AtomicIDecrement(global long *Pointer, int Scope, int Semantics);
long __attribute__((overloadable))
__spirv_AtomicIDecrement(local long *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long __attribute__((overloadable))
__spirv_AtomicIDecrement(generic long *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

int __attribute__((overloadable))
__spirv_AtomicIAdd(private int *Pointer, int Scope, int Semantics, int Value);
int __attribute__((overloadable))
__spirv_AtomicIAdd(global int *Pointer, int Scope, int Semantics, int Value);
int __attribute__((overloadable))
__spirv_AtomicIAdd(local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int __attribute__((overloadable))
__spirv_AtomicIAdd(generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long __attribute__((overloadable))
__spirv_AtomicIAdd(private long *Pointer, int Scope, int Semantics, long Value);
long __attribute__((overloadable))
__spirv_AtomicIAdd(global long *Pointer, int Scope, int Semantics, long Value);
long __attribute__((overloadable))
__spirv_AtomicIAdd(local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long __attribute__((overloadable))
__spirv_AtomicIAdd(generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

int __attribute__((overloadable))
__spirv_AtomicISub(private int *Pointer, int Scope, int Semantics, int Value);
int __attribute__((overloadable))
__spirv_AtomicISub(global int *Pointer, int Scope, int Semantics, int Value);
int __attribute__((overloadable))
__spirv_AtomicISub(local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int __attribute__((overloadable))
__spirv_AtomicISub(generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long __attribute__((overloadable))
__spirv_AtomicISub(private long *Pointer, int Scope, int Semantics, long Value);
long __attribute__((overloadable))
__spirv_AtomicISub(global long *Pointer, int Scope, int Semantics, long Value);
long __attribute__((overloadable))
__spirv_AtomicISub(local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long __attribute__((overloadable))
__spirv_AtomicISub(generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

int __attribute__((overloadable))
__spirv_AtomicSMin(private int *Pointer, int Scope, int Semantics, int Value);
int __attribute__((overloadable))
__spirv_AtomicSMin(global int *Pointer, int Scope, int Semantics, int Value);
int __attribute__((overloadable))
__spirv_AtomicSMin(local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int __attribute__((overloadable))
__spirv_AtomicSMin(generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
long __attribute__((overloadable))
__spirv_AtomicSMin(private long *Pointer, int Scope, int Semantics, long Value);
long __attribute__((overloadable))
__spirv_AtomicSMin(global long *Pointer, int Scope, int Semantics, long Value);
long __attribute__((overloadable))
__spirv_AtomicSMin(local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long __attribute__((overloadable))
__spirv_AtomicSMin(generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

uint __attribute__((overloadable))
__spirv_AtomicUMin(private uint *Pointer, int Scope, int Semantics, uint Value);
uint __attribute__((overloadable))
__spirv_AtomicUMin(global uint *Pointer, int Scope, int Semantics, uint Value);
uint __attribute__((overloadable))
__spirv_AtomicUMin(local uint *Pointer, int Scope, int Semantics, uint Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
uint __attribute__((overloadable))
__spirv_AtomicUMin(generic uint *Pointer, int Scope, int Semantics, uint Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
ulong __attribute__((overloadable))
__spirv_AtomicUMin(private ulong *Pointer, int Scope, int Semantics, ulong Value);
ulong __attribute__((overloadable))
__spirv_AtomicUMin(global ulong *Pointer, int Scope, int Semantics, ulong Value);
ulong __attribute__((overloadable))
__spirv_AtomicUMin(local ulong *Pointer, int Scope, int Semantics, ulong Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
ulong __attribute__((overloadable))
__spirv_AtomicUMin(generic ulong *Pointer, int Scope, int Semantics, ulong Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

int __attribute__((overloadable))
__spirv_AtomicSMax(private int *Pointer, int Scope, int Semantics, int Value);
int __attribute__((overloadable))
__spirv_AtomicSMax(global int *Pointer, int Scope, int Semantics, int Value);
int __attribute__((overloadable))
__spirv_AtomicSMax(local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int __attribute__((overloadable))
__spirv_AtomicSMax(generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
long __attribute__((overloadable))
__spirv_AtomicSMax(private long *Pointer, int Scope, int Semantics, long Value);
long __attribute__((overloadable))
__spirv_AtomicSMax(global long *Pointer, int Scope, int Semantics, long Value);
long __attribute__((overloadable))
__spirv_AtomicSMax(local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long __attribute__((overloadable))
__spirv_AtomicSMax(generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

uint __attribute__((overloadable))
__spirv_AtomicUMax(private uint *Pointer, int Scope, int Semantics, uint Value);
uint __attribute__((overloadable))
__spirv_AtomicUMax(global uint *Pointer, int Scope, int Semantics, uint Value);
uint __attribute__((overloadable))
__spirv_AtomicUMax(local uint *Pointer, int Scope, int Semantics, uint Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
uint __attribute__((overloadable))
__spirv_AtomicUMax(generic uint *Pointer, int Scope, int Semantics, uint Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
ulong __attribute__((overloadable))
__spirv_AtomicUMax(private ulong *Pointer, int Scope, int Semantics, ulong Value);
ulong __attribute__((overloadable))
__spirv_AtomicUMax(global ulong *Pointer, int Scope, int Semantics, ulong Value);
ulong __attribute__((overloadable))
__spirv_AtomicUMax(local ulong *Pointer, int Scope, int Semantics, ulong Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
ulong __attribute__((overloadable))
__spirv_AtomicUMax(generic ulong *Pointer, int Scope, int Semantics, ulong Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

int __attribute__((overloadable))
__spirv_AtomicAnd(private int *Pointer, int Scope, int Semantics, int Value);
int __attribute__((overloadable))
__spirv_AtomicAnd(global int *Pointer, int Scope, int Semantics, int Value);
int __attribute__((overloadable))
__spirv_AtomicAnd(local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int __attribute__((overloadable))
__spirv_AtomicAnd(generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
long __attribute__((overloadable))
__spirv_AtomicAnd(private long *Pointer, int Scope, int Semantics, long Value);
long __attribute__((overloadable))
__spirv_AtomicAnd(global long *Pointer, int Scope, int Semantics, long Value);
long __attribute__((overloadable))
__spirv_AtomicAnd(local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long __attribute__((overloadable))
__spirv_AtomicAnd(generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

int __attribute__((overloadable))
__spirv_AtomicOr(private int *Pointer, int Scope, int Semantics, int Value);
int __attribute__((overloadable))
__spirv_AtomicOr(global int *Pointer, int Scope, int Semantics, int Value);
int __attribute__((overloadable))
__spirv_AtomicOr(local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int __attribute__((overloadable))
__spirv_AtomicOr(generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
long __attribute__((overloadable))
__spirv_AtomicOr(private long *Pointer, int Scope, int Semantics, long Value);
long __attribute__((overloadable))
__spirv_AtomicOr(global long *Pointer, int Scope, int Semantics, long Value);
long __attribute__((overloadable))
__spirv_AtomicOr(local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long __attribute__((overloadable))
__spirv_AtomicOr(generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

#if defined(cl_intel_bfloat16_atomics)
short __attribute__((overloadable))
__spirv_AtomicOr(private short *Pointer, int Scope, int Semantics, short Value);
short __attribute__((overloadable))
__spirv_AtomicOr(global short *Pointer, int Scope, int Semantics, short Value);
short __attribute__((overloadable))
__spirv_AtomicOr(local short *Pointer, int Scope, int Semantics, short Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
short __attribute__((overloadable))
__spirv_AtomicOr(generic short *Pointer, int Scope, int Semantics, short Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_intel_bfloat16_atomics)

int __attribute__((overloadable))
__spirv_AtomicXor(private int *Pointer, int Scope, int Semantics, int Value);
int __attribute__((overloadable))
__spirv_AtomicXor(global int *Pointer, int Scope, int Semantics, int Value);
int __attribute__((overloadable))
__spirv_AtomicXor(local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int __attribute__((overloadable))
__spirv_AtomicXor(generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
long __attribute__((overloadable))
__spirv_AtomicXor(private long *Pointer, int Scope, int Semantics, long Value);
long __attribute__((overloadable))
__spirv_AtomicXor(global long *Pointer, int Scope, int Semantics, long Value);
long __attribute__((overloadable))
__spirv_AtomicXor(local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long __attribute__((overloadable))
__spirv_AtomicXor(generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

bool __attribute__((overloadable))
__spirv_AtomicFlagTestAndSet(private int *Pointer, int Scope, int Semantics);
bool __attribute__((overloadable))
__spirv_AtomicFlagTestAndSet(global int *Pointer, int Scope, int Semantics);
bool __attribute__((overloadable))
__spirv_AtomicFlagTestAndSet(local int *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
bool __attribute__((overloadable))
__spirv_AtomicFlagTestAndSet(generic int *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

void __attribute__((overloadable))
__spirv_AtomicFlagClear(private int *Pointer, int Scope, int Semantics);
void __attribute__((overloadable))
__spirv_AtomicFlagClear(global int *Pointer, int Scope, int Semantics);
void __attribute__((overloadable))
__spirv_AtomicFlagClear(local int *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void __attribute__((overloadable))
__spirv_AtomicFlagClear(generic int *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __attribute__((overloadable))
__spirv_AtomicFAdd(global float *Pointer, int Scope, int Semantics, float Value);
float __attribute__((overloadable))
__spirv_AtomicFSub(global float *Pointer, int Scope, int Semantics, float Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

// SPV_EXT_shader_atomic_float_add
float __attribute__((overloadable))
__spirv_AtomicFAddEXT(private float *Pointer, int Scope, int Semantics, float Value);
float __attribute__((overloadable))
__spirv_AtomicFAddEXT(global float *Pointer, int Scope, int Semantics, float Value);
float __attribute__((overloadable))
__spirv_AtomicFAddEXT(local float *Pointer, int Scope, int Semantics, float Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __attribute__((overloadable))
__spirv_AtomicFAddEXT(generic float *Pointer, int Scope, int Semantics, float Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double __attribute__((overloadable))
__spirv_AtomicFAddEXT(private double *Pointer, int Scope, int Semantics, double Value);
double __attribute__((overloadable))
__spirv_AtomicFAddEXT(global double *Pointer, int Scope, int Semantics, double Value);
double __attribute__((overloadable))
__spirv_AtomicFAddEXT(local double *Pointer, int Scope, int Semantics, double Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __attribute__((overloadable))
__spirv_AtomicFAddEXT(generic double *Pointer, int Scope, int Semantics, double Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

// SPV_EXT_shader_atomic_float_min_max
#if defined(cl_khr_fp16)
half __attribute__((overloadable))
__spirv_AtomicFMinEXT(private half *Pointer, int Scope, int Semantics, half Value);
half __attribute__((overloadable))
__spirv_AtomicFMinEXT(global half *Pointer, int Scope, int Semantics, half Value);
half __attribute__((overloadable))
__spirv_AtomicFMinEXT(local half *Pointer, int Scope, int Semantics, half Value);
#endif // defined(cl_khr_fp16)
float __attribute__((overloadable))
__spirv_AtomicFMinEXT(private float *Pointer, int Scope, int Semantics, float Value);
float __attribute__((overloadable))
__spirv_AtomicFMinEXT(global float *Pointer, int Scope, int Semantics, float Value);
float __attribute__((overloadable))
__spirv_AtomicFMinEXT(local float *Pointer, int Scope, int Semantics, float Value);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))
__spirv_AtomicFMinEXT(private double *Pointer, int Scope, int Semantics, double Value);
double __attribute__((overloadable))
__spirv_AtomicFMinEXT(global double *Pointer, int Scope, int Semantics, double Value);
double __attribute__((overloadable))
__spirv_AtomicFMinEXT(local double *Pointer, int Scope, int Semantics, double Value);
#endif // defined(cl_khr_fp64)
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#if defined(cl_khr_fp16)
half __attribute__((overloadable))
__spirv_AtomicFMinEXT(generic half *Pointer, int Scope, int Semantics, half Value);
#endif // defined(cl_khr_fp16)
float __attribute__((overloadable))
__spirv_AtomicFMinEXT(generic float *Pointer, int Scope, int Semantics, float Value);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))
__spirv_AtomicFMinEXT(generic double *Pointer, int Scope, int Semantics, double Value);
#endif // defined(cl_khr_fp64)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp16)
half __attribute__((overloadable))
__spirv_AtomicFMaxEXT(private half *Pointer, int Scope, int Semantics, half Value);
half __attribute__((overloadable))
__spirv_AtomicFMaxEXT(global half *Pointer, int Scope, int Semantics, half Value);
half __attribute__((overloadable))
__spirv_AtomicFMaxEXT(local half *Pointer, int Scope, int Semantics, half Value);
#endif // defined(cl_khr_fp16)
float __attribute__((overloadable))
__spirv_AtomicFMaxEXT(private float *Pointer, int Scope, int Semantics, float Value);
float __attribute__((overloadable))
__spirv_AtomicFMaxEXT(global float *Pointer, int Scope, int Semantics, float Value);
float __attribute__((overloadable))
__spirv_AtomicFMaxEXT(local float *Pointer, int Scope, int Semantics, float Value);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))
__spirv_AtomicFMaxEXT(private double *Pointer, int Scope, int Semantics, double Value);
double __attribute__((overloadable))
__spirv_AtomicFMaxEXT(global double *Pointer, int Scope, int Semantics, double Value);
double __attribute__((overloadable))
__spirv_AtomicFMaxEXT(local double *Pointer, int Scope, int Semantics, double Value);
#endif // defined(cl_khr_fp64)
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#if defined(cl_khr_fp16)
half __attribute__((overloadable))
__spirv_AtomicFMaxEXT(generic half *Pointer, int Scope, int Semantics, half Value);
#endif // defined(cl_khr_fp16)
float __attribute__((overloadable))
__spirv_AtomicFMaxEXT(generic float *Pointer, int Scope, int Semantics, float Value);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))
__spirv_AtomicFMaxEXT(generic double *Pointer, int Scope, int Semantics, double Value);
#endif // defined(cl_khr_fp64)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

half __attribute__((overloadable))
__spirv_AtomicFAddEXT(private half *Pointer, int Scope, int Semantics, half Value);
half __attribute__((overloadable))
__spirv_AtomicFAddEXT(global half *Pointer, int Scope, int Semantics, half Value);
half __attribute__((overloadable))
__spirv_AtomicFAddEXT(local half *Pointer, int Scope, int Semantics, half Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __attribute__((overloadable))
__spirv_AtomicFAddEXT(generic half *Pointer, int Scope, int Semantics, half Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

// Barrier Instructions

void __attribute__((overloadable))
__spirv_ControlBarrier(int Execution, int Memory, int Semantics);
void __attribute__((overloadable)) __spirv_MemoryBarrier(int Memory, int Semantics);

void __attribute__((overloadable))
__spirv_SubRegionControlBarrierINTEL(int Execution, int Memory, int Semantics);

#if defined(cl_intel_concurrent_dispatch)

bool __attribute__((overloadable)) __attribute__((const))
__spirv_BuiltInDeviceBarrierValidINTEL();

#endif // defined(cl_intel_concurrent_dispatch)

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

__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global char  *Destination,
    local char   *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global char  *Destination,
    local char   *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global short *Destination,
    local short  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global short *Destination,
    local short  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global int   *Destination,
    local int    *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global int   *Destination,
    local int    *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global long  *Destination,
    local long   *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global long  *Destination,
    local long   *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global half  *Destination,
    local half   *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global half  *Destination,
    local half   *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global float *Destination,
    local float  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global float *Destination,
    local float  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global char2 *Destination,
    local char2  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global char2 *Destination,
    local char2  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global char3 *Destination,
    local char3  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global char3 *Destination,
    local char3  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global char4 *Destination,
    local char4  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global char4 *Destination,
    local char4  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global char8 *Destination,
    local char8  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global char8 *Destination,
    local char8  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global char16 *Destination,
    local char16  *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global char16 *Destination,
    local char16  *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global short2 *Destination,
    local short2  *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global short2 *Destination,
    local short2  *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global short3 *Destination,
    local short3  *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global short3 *Destination,
    local short3  *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global short4 *Destination,
    local short4  *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global short4 *Destination,
    local short4  *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global short8 *Destination,
    local short8  *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global short8 *Destination,
    local short8  *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    global short16 *Destination,
    local short16  *Source,
    long            NumElements,
    long            Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    global short16 *Destination,
    local short16  *Source,
    int             NumElements,
    int             Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global int2  *Destination,
    local int2   *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global int2  *Destination,
    local int2   *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global int3  *Destination,
    local int3   *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global int3  *Destination,
    local int3   *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global int4  *Destination,
    local int4   *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global int4  *Destination,
    local int4   *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global int8  *Destination,
    local int8   *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global int8  *Destination,
    local int8   *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global int16 *Destination,
    local int16  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global int16 *Destination,
    local int16  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global long2 *Destination,
    local long2  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global long2 *Destination,
    local long2  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global long3 *Destination,
    local long3  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global long3 *Destination,
    local long3  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global long4 *Destination,
    local long4  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global long4 *Destination,
    local long4  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global long8 *Destination,
    local long8  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global long8 *Destination,
    local long8  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global long16 *Destination,
    local long16  *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global long16 *Destination,
    local long16  *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global half2 *Destination,
    local half2  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global half2 *Destination,
    local half2  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global half3 *Destination,
    local half3  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global half3 *Destination,
    local half3  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global half4 *Destination,
    local half4  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global half4 *Destination,
    local half4  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global half8 *Destination,
    local half8  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    global half8 *Destination,
    local half8  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global half16 *Destination,
    local half16  *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global half16 *Destination,
    local half16  *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global float2 *Destination,
    local float2  *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global float2 *Destination,
    local float2  *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global float3 *Destination,
    local float3  *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global float3 *Destination,
    local float3  *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global float4 *Destination,
    local float4  *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global float4 *Destination,
    local float4  *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global float8 *Destination,
    local float8  *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global float8 *Destination,
    local float8  *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    global float16 *Destination,
    local float16  *Source,
    long            NumElements,
    long            Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    global float16 *Destination,
    local float16  *Source,
    int             NumElements,
    int             Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local char   *Destination,
    global char  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local char   *Destination,
    global char  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local short  *Destination,
    global short *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local short  *Destination,
    global short *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local int    *Destination,
    global int   *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local int    *Destination,
    global int   *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local long   *Destination,
    global long  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local long   *Destination,
    global long  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local half   *Destination,
    global half  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local half   *Destination,
    global half  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local float  *Destination,
    global float *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local char2  *Destination,
    global char2 *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local char2  *Destination,
    global char2 *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local char3  *Destination,
    global char3 *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local char3  *Destination,
    global char3 *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local char4  *Destination,
    global char4 *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local char4  *Destination,
    global char4 *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local char8  *Destination,
    global char8 *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local char8  *Destination,
    global char8 *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local char16  *Destination,
    global char16 *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local char16  *Destination,
    global char16 *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local short2  *Destination,
    global short2 *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local short2  *Destination,
    global short2 *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local short3  *Destination,
    global short3 *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local short3  *Destination,
    global short3 *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local short4  *Destination,
    global short4 *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local short4  *Destination,
    global short4 *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local short8  *Destination,
    global short8 *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local short8  *Destination,
    global short8 *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    local short16  *Destination,
    global short16 *Source,
    long            NumElements,
    long            Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    local short16  *Destination,
    global short16 *Source,
    int             NumElements,
    int             Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local int2   *Destination,
    global int2  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local int2   *Destination,
    global int2  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local int3   *Destination,
    global int3  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local int3   *Destination,
    global int3  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local int4   *Destination,
    global int4  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local int4   *Destination,
    global int4  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local int8   *Destination,
    global int8  *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local int8   *Destination,
    global int8  *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local int16  *Destination,
    global int16 *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local int16  *Destination,
    global int16 *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local long2  *Destination,
    global long2 *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local long2  *Destination,
    global long2 *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local long3  *Destination,
    global long3 *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local long3  *Destination,
    global long3 *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local long4  *Destination,
    global long4 *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local long4  *Destination,
    global long4 *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local long8  *Destination,
    global long8 *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local long8  *Destination,
    global long8 *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local long16  *Destination,
    global long16 *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local long16  *Destination,
    global long16 *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local half2  *Destination,
    global half2 *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local half2  *Destination,
    global half2 *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local half3  *Destination,
    global half3 *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local half3  *Destination,
    global half3 *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local half4  *Destination,
    global half4 *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local half4  *Destination,
    global half4 *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local half8  *Destination,
    global half8 *Source,
    long          NumElements,
    long          Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local half8  *Destination,
    global half8 *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local half16  *Destination,
    global half16 *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local half16  *Destination,
    global half16 *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local float2  *Destination,
    global float2 *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local float2  *Destination,
    global float2 *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local float3  *Destination,
    global float3 *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local float3  *Destination,
    global float3 *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local float4  *Destination,
    global float4 *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local float4  *Destination,
    global float4 *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local float8  *Destination,
    global float8 *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local float8  *Destination,
    global float8 *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    local float16  *Destination,
    global float16 *Source,
    long            NumElements,
    long            Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    local float16  *Destination,
    global float16 *Source,
    int             NumElements,
    int             Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int           Execution,
    local float  *Destination,
    global float *Source,
    int           NumElements,
    int           Stride,
    __spirv_Event Event);
#if defined(cl_khr_fp64)
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global double *Destination,
    local double  *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    global double *Destination,
    local double  *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    global double2 *Destination,
    local double2  *Source,
    long            NumElements,
    long            Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    global double2 *Destination,
    local double2  *Source,
    int             NumElements,
    int             Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    global double3 *Destination,
    local double3  *Source,
    long            NumElements,
    long            Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    global double3 *Destination,
    local double3  *Source,
    int             NumElements,
    int             Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    global double4 *Destination,
    local double4  *Source,
    long            NumElements,
    long            Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    global double4 *Destination,
    local double4  *Source,
    int             NumElements,
    int             Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    global double8 *Destination,
    local double8  *Source,
    long            NumElements,
    long            Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    global double8 *Destination,
    local double8  *Source,
    int             NumElements,
    int             Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int              Execution,
    global double16 *Destination,
    local double16  *Source,
    long             NumElements,
    long             Stride,
    __spirv_Event    Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int              Execution,
    global double16 *Destination,
    local double16  *Source,
    int              NumElements,
    int              Stride,
    __spirv_Event    Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local double  *Destination,
    global double *Source,
    long           NumElements,
    long           Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int            Execution,
    local double  *Destination,
    global double *Source,
    int            NumElements,
    int            Stride,
    __spirv_Event  Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    local double2  *Destination,
    global double2 *Source,
    long            NumElements,
    long            Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    local double2  *Destination,
    global double2 *Source,
    int             NumElements,
    int             Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    local double3  *Destination,
    global double3 *Source,
    long            NumElements,
    long            Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    local double3  *Destination,
    global double3 *Source,
    int             NumElements,
    int             Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    local double4  *Destination,
    global double4 *Source,
    long            NumElements,
    long            Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    local double4  *Destination,
    global double4 *Source,
    int             NumElements,
    int             Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    local double8  *Destination,
    global double8 *Source,
    long            NumElements,
    long            Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int             Execution,
    local double8  *Destination,
    global double8 *Source,
    int             NumElements,
    int             Stride,
    __spirv_Event   Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int              Execution,
    local double16  *Destination,
    global double16 *Source,
    long             NumElements,
    long             Stride,
    __spirv_Event    Event);
__spirv_Event __attribute__((overloadable)) __spirv_GroupAsyncCopy(
    int              Execution,
    local double16  *Destination,
    global double16 *Source,
    int              NumElements,
    int              Stride,
    __spirv_Event    Event);
#endif // defined(cl_khr_fp64)

void __attribute__((overloadable))
__spirv_GroupWaitEvents(int Execution, int NumEvents, private __spirv_Event *EventsList);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void __attribute__((overloadable))
__spirv_GroupWaitEvents(int Execution, int NumEvents, generic __spirv_Event *EventsList);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#if defined(cl_khr_subgroup_non_uniform_vote)
bool __attribute__((overloadable)) __spirv_GroupNonUniformElect(int Execution);
bool __attribute__((overloadable))
__spirv_GroupNonUniformAll(int Execution, bool Predicate);
bool __attribute__((overloadable))
__spirv_GroupNonUniformAny(int Execution, bool Predicate);
bool __attribute__((overloadable))
__spirv_GroupNonUniformAllEqual(int Execution, char Value);
bool __attribute__((overloadable))
__spirv_GroupNonUniformAllEqual(int Execution, short Value);
bool __attribute__((overloadable))
__spirv_GroupNonUniformAllEqual(int Execution, int Value);
bool __attribute__((overloadable))
__spirv_GroupNonUniformAllEqual(int Execution, long Value);
bool __attribute__((overloadable))
__spirv_GroupNonUniformAllEqual(int Execution, float Value);
#if defined(cl_khr_fp64)
bool __attribute__((overloadable))
__spirv_GroupNonUniformAllEqual(int Execution, double Value);
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
bool __attribute__((overloadable))
__spirv_GroupNonUniformAllEqual(int Execution, half Value);
#endif // defined(cl_khr_fp16)
#endif // defined(cl_khr_subgroup_non_uniform_vote)

bool __attribute__((overloadable)) __spirv_GroupAll(int Execution, bool Predicate);
bool __attribute__((overloadable)) __spirv_GroupAny(int Execution, bool Predicate);
#define DECL_SUB_GROUP_BROADCAST_BASE(TYPE, TYPE_ABBR)         \
    TYPE __attribute__((overloadable)) __spirv_GroupBroadcast( \
        int Execution, TYPE Value, int3 LocalId);              \
    TYPE __attribute__((overloadable)) __spirv_GroupBroadcast( \
        int Execution, TYPE Value, long3 LocalId);             \
    TYPE __attribute__((overloadable)) __spirv_GroupBroadcast( \
        int Execution, TYPE Value, int2 LocalId);              \
    TYPE __attribute__((overloadable)) __spirv_GroupBroadcast( \
        int Execution, TYPE Value, long2 LocalId);             \
    TYPE __attribute__((overloadable)) __spirv_GroupBroadcast( \
        int Execution, TYPE Value, int LocalId);               \
    TYPE __attribute__((overloadable)) __spirv_GroupBroadcast( \
        int Execution, TYPE Value, long LocalId);

char __attribute__((overloadable))
__spirv_SubgroupShuffleINTEL(char Data, uint InvocationId);
short __attribute__((overloadable))
__spirv_SubgroupShuffleINTEL(short Data, uint InvocationId);
int __attribute__((overloadable))
__spirv_SubgroupShuffleINTEL(int Data, uint InvocationId);
long __attribute__((overloadable))
__spirv_SubgroupShuffleINTEL(long Data, uint InvocationId);
#if defined(cl_khr_fp16)
half __attribute__((overloadable))
__spirv_SubgroupShuffleINTEL(half Data, uint InvocationId);
#endif // defined(cl_khr_fp16)
float __attribute__((overloadable))
__spirv_SubgroupShuffleINTEL(float Data, uint InvocationId);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))
__spirv_SubgroupShuffleINTEL(double Data, uint InvocationId);
#endif // defined(cl_khr_fp64)

char __attribute__((overloadable))
__spirv_SubgroupShuffleDownINTEL(char Current, char Next, uint Delta);
short __attribute__((overloadable))
__spirv_SubgroupShuffleDownINTEL(short Current, short Next, uint Delta);
int __attribute__((overloadable))
__spirv_SubgroupShuffleDownINTEL(int Current, int Next, uint Delta);
long __attribute__((overloadable))
__spirv_SubgroupShuffleDownINTEL(long Current, long Next, uint Delta);
#if defined(cl_khr_fp16)
half __attribute__((overloadable))
__spirv_SubgroupShuffleDownINTEL(half Current, half Next, uint Delta);
#endif // defined(cl_khr_fp16)
float __attribute__((overloadable))
__spirv_SubgroupShuffleDownINTEL(float Current, float Next, uint Delta);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))
__spirv_SubgroupShuffleDownINTEL(double Current, double Next, uint Delta);
#endif // defined(cl_khr_fp64)

char __attribute__((overloadable))
__spirv_SubgroupShuffleUpINTEL(char Previous, char Current, uint Delta);
short __attribute__((overloadable))
__spirv_SubgroupShuffleUpINTEL(short Previous, short Current, uint Delta);
int __attribute__((overloadable))
__spirv_SubgroupShuffleUpINTEL(int Previous, int Current, uint Delta);
long __attribute__((overloadable))
__spirv_SubgroupShuffleUpINTEL(long Previous, long Current, uint Delta);
#if defined(cl_khr_fp16)
half __attribute__((overloadable))
__spirv_SubgroupShuffleUpINTEL(half Previous, half Current, uint Delta);
#endif // defined(cl_khr_fp16)
float __attribute__((overloadable))
__spirv_SubgroupShuffleUpINTEL(float Previous, float Current, uint Delta);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))
__spirv_SubgroupShuffleUpINTEL(double Previous, double Current, uint Delta);
#endif // defined(cl_khr_fp64)

char __attribute__((overloadable)) __spirv_SubgroupShuffleXorINTEL(char Data, uint Value);
short __attribute__((overloadable))
                                  __spirv_SubgroupShuffleXorINTEL(short Data, uint Value);
int __attribute__((overloadable)) __spirv_SubgroupShuffleXorINTEL(int Data, uint Value);
long __attribute__((overloadable)) __spirv_SubgroupShuffleXorINTEL(long Data, uint Value);
#if defined(cl_khr_fp16)
half __attribute__((overloadable)) __spirv_SubgroupShuffleXorINTEL(half Data, uint Value);
#endif // defined(cl_khr_fp16)
float
    __attribute__((overloadable)) __spirv_SubgroupShuffleXorINTEL(float Data, uint Value);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))
__spirv_SubgroupShuffleXorINTEL(double Data, uint Value);
#endif // defined(cl_khr_fp64)

#ifdef cl_intel_subgroups_char
char __attribute__((overloadable))
__spirv_SubgroupImageBlockReadINTEL_Rchar(global Img2d_ro *image, int2 coord);
char2 __attribute__((overloadable))
__spirv_SubgroupImageBlockReadINTEL_Rchar2(global Img2d_ro *image, int2 coord);
char4 __attribute__((overloadable))
__spirv_SubgroupImageBlockReadINTEL_Rchar4(global Img2d_ro *image, int2 coord);
char8 __attribute__((overloadable))
__spirv_SubgroupImageBlockReadINTEL_Rchar8(global Img2d_ro *image, int2 coord);
char16 __attribute__((overloadable))
__spirv_SubgroupImageBlockReadINTEL_Rchar16(global Img2d_ro *image, int2 coord);
#endif // cl_intel_subgroups_char

short __attribute__((overloadable))
__spirv_SubgroupImageBlockReadINTEL_Rshort(global Img2d_ro *image, int2 coord);
short2 __attribute__((overloadable))
__spirv_SubgroupImageBlockReadINTEL_Rshort2(global Img2d_ro *image, int2 coord);
short4 __attribute__((overloadable))
__spirv_SubgroupImageBlockReadINTEL_Rshort4(global Img2d_ro *image, int2 coord);
short8 __attribute__((overloadable))
__spirv_SubgroupImageBlockReadINTEL_Rshort8(global Img2d_ro *image, int2 coord);

int __attribute__((overloadable))
__spirv_SubgroupImageBlockReadINTEL_Rint(global Img2d_ro *image, int2 coord);
int2 __attribute__((overloadable))
__spirv_SubgroupImageBlockReadINTEL_Rint2(global Img2d_ro *image, int2 coord);
int4 __attribute__((overloadable))
__spirv_SubgroupImageBlockReadINTEL_Rint4(global Img2d_ro *image, int2 coord);
int8 __attribute__((overloadable))
__spirv_SubgroupImageBlockReadINTEL_Rint8(global Img2d_ro *image, int2 coord);

#ifdef cl_intel_subgroups_long
long __attribute__((overloadable))
__spirv_SubgroupImageBlockReadINTEL_Rlong(global Img2d_ro *image, int2 coord);
long2 __attribute__((overloadable))
__spirv_SubgroupImageBlockReadINTEL_Rlong2(global Img2d_ro *image, int2 coord);
long4 __attribute__((overloadable))
__spirv_SubgroupImageBlockReadINTEL_Rlong4(global Img2d_ro *image, int2 coord);
long8 __attribute__((overloadable))
__spirv_SubgroupImageBlockReadINTEL_Rlong8(global Img2d_ro *image, int2 coord);
#endif // cl_intel_subgroups_long

#ifdef cl_intel_subgroup_buffer_prefetch
void __attribute__((overloadable))
__spirv_SubgroupBlockPrefetchINTEL(const global uchar *ptr, uint num_bytes);
void __attribute__((overloadable))
__spirv_SubgroupBlockPrefetchINTEL(const global ushort *ptr, uint num_bytes);
void __attribute__((overloadable))
__spirv_SubgroupBlockPrefetchINTEL(const global uint *ptr, uint num_bytes);
void __attribute__((overloadable))
__spirv_SubgroupBlockPrefetchINTEL(const global ulong *ptr, uint num_bytes);
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
#define DECL_NON_UNIFORM_BROADCAST_BASE(TYPE, TYPE_ABBR)                      \
    TYPE __attribute__((overloadable)) __spirv_GroupNonUniformBroadcast(      \
        int Execution, TYPE Value, uint Id);                                  \
    TYPE __attribute__((overloadable)) __spirv_GroupNonUniformBroadcastFirst( \
        int Execution, TYPE Value);

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

uint4 __attribute__((overloadable))
__spirv_GroupNonUniformBallot(int Execution, bool Predicate);
bool __attribute__((overloadable))
__spirv_GroupNonUniformInverseBallot(int Execution, uint4 Value);
bool __attribute__((overloadable))
__spirv_GroupNonUniformBallotBitExtract(int Execution, uint4 Value, uint Index);
uint __attribute__((overloadable))
__spirv_GroupNonUniformBallotBitCount(int Execution, int Operation, uint4 Value);
uint __attribute__((overloadable))
__spirv_GroupNonUniformBallotFindLSB(int Execution, uint4 Value);
uint __attribute__((overloadable))
__spirv_GroupNonUniformBallotFindMSB(int Execution, uint4 Value);
uint4 __attribute__((overloadable)) __spirv_BuiltInSubgroupEqMask(void);
uint4 __attribute__((overloadable)) __spirv_BuiltInSubgroupGeMask(void);
uint4 __attribute__((overloadable)) __spirv_BuiltInSubgroupGtMask(void);
uint4 __attribute__((overloadable)) __spirv_BuiltInSubgroupLeMask(void);
uint4 __attribute__((overloadable)) __spirv_BuiltInSubgroupLtMask(void);
#endif // defined(cl_khr_subgroup_ballot)

#if defined(cl_khr_subgroup_shuffle)
#define DECL_NON_UNIFORM_SHUFFLE(TYPE, TYPE_ABBR)                         \
    TYPE __attribute__((overloadable)) __spirv_GroupNonUniformShuffle(    \
        int Execution, TYPE Value, uint Id);                              \
    TYPE __attribute__((overloadable)) __spirv_GroupNonUniformShuffleXor( \
        int Execution, TYPE Value, uint Mask);

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
#define DECL_NON_UNIFORM_SHUFFLE_RELATIVE(TYPE, TYPE_ABBR)                 \
    TYPE __attribute__((overloadable)) __spirv_GroupNonUniformShuffleUp(   \
        int Execution, TYPE Value, uint Delta);                            \
    TYPE __attribute__((overloadable)) __spirv_GroupNonUniformShuffleDown( \
        int Execution, TYPE Value, uint Delta);

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
#define DEFN_NON_UNIFORM_OPERATION_BASE(TYPE, OPERATION, TYPE_ABBR)        \
    TYPE __attribute__((overloadable)) __spirv_GroupNonUniform##OPERATION( \
        int Execution, int Operation, TYPE X);
#endif // defined(cl_khr_subgroup_non_uniform_arithmetic)

#if defined(cl_khr_subgroup_clustered_reduce)
#define DEFN_NON_UNIFORM_CLUSTERED_OPERATION(TYPE, OPERATION, TYPE_ABBR)   \
    TYPE __attribute__((overloadable)) __spirv_GroupNonUniform##OPERATION( \
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

char
    __attribute__((overloadable)) __spirv_GroupIAdd(int Execution, int Operation, char X);
short __attribute__((overloadable))
__spirv_GroupIAdd(int Execution, int Operation, short X);
int __attribute__((overloadable)) __spirv_GroupIAdd(int Execution, int Operation, int X);
long __attribute__((overloadable))
__spirv_GroupIAdd(int Execution, int Operation, long X);

half __attribute__((overloadable))
__spirv_GroupFAdd(int Execution, int Operation, half X);
float __attribute__((overloadable))
__spirv_GroupFAdd(int Execution, int Operation, float X);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))
__spirv_GroupFAdd(int Execution, int Operation, double X);
#endif // defined(cl_khr_fp64)

uchar __attribute__((overloadable))
__spirv_GroupUMin(int Execution, int Operation, uchar X);
ushort __attribute__((overloadable))
__spirv_GroupUMin(int Execution, int Operation, ushort X);
uint __attribute__((overloadable))
__spirv_GroupUMin(int Execution, int Operation, uint X);
ulong __attribute__((overloadable))
__spirv_GroupUMin(int Execution, int Operation, ulong X);

half __attribute__((overloadable))
__spirv_GroupFMin(int Execution, int Operation, half X);
float __attribute__((overloadable))
__spirv_GroupFMin(int Execution, int Operation, float X);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))
__spirv_GroupFMin(int Execution, int Operation, double X);
#endif // defined(cl_khr_fp64)

char
    __attribute__((overloadable)) __spirv_GroupSMin(int Execution, int Operation, char X);
short __attribute__((overloadable))
__spirv_GroupSMin(int Execution, int Operation, short X);
int __attribute__((overloadable)) __spirv_GroupSMin(int Execution, int Operation, int X);
long __attribute__((overloadable))
__spirv_GroupSMin(int Execution, int Operation, long X);

half __attribute__((overloadable))
__spirv_GroupFMax(int Execution, int Operation, half X);
float __attribute__((overloadable))
__spirv_GroupFMax(int Execution, int Operation, float X);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))
__spirv_GroupFMax(int Execution, int Operation, double X);
#endif // defined(cl_khr_fp64)

uchar __attribute__((overloadable))
__spirv_GroupUMax(int Execution, int Operation, uchar X);
ushort __attribute__((overloadable))
__spirv_GroupUMax(int Execution, int Operation, ushort X);
uint __attribute__((overloadable))
__spirv_GroupUMax(int Execution, int Operation, uint X);
ulong __attribute__((overloadable))
__spirv_GroupUMax(int Execution, int Operation, ulong X);

char __attribute__((overloadable))
__spirv_GroupSMax(int Execution, int Operation, char X);
short __attribute__((overloadable))
__spirv_GroupSMax(int Execution, int Operation, short X);
int __attribute__((overloadable)) __spirv_GroupSMax(int Execution, int Operation, int X);
long __attribute__((overloadable))
__spirv_GroupSMax(int Execution, int Operation, long X);

char __attribute__((overloadable))
__spirv_GroupIMulKHR(int Execution, int Operation, char X);
short __attribute__((overloadable))
__spirv_GroupIMulKHR(int Execution, int Operation, short X);
int __attribute__((overloadable))
__spirv_GroupIMulKHR(int Execution, int Operation, int X);
long __attribute__((overloadable))
__spirv_GroupIMulKHR(int Execution, int Operation, long X);

half __attribute__((overloadable))
__spirv_GroupFMulKHR(int Execution, int Operation, half X);
float __attribute__((overloadable))
__spirv_GroupFMulKHR(int Execution, int Operation, float X);
#if defined(cl_khr_fp64)
double __attribute__((overloadable))
__spirv_GroupFMulKHR(int Execution, int Operation, double X);
#endif // defined(cl_khr_fp64)

char __attribute__((overloadable))
__spirv_GroupBitwiseAndKHR(int Execution, int Operation, char X);
short __attribute__((overloadable))
__spirv_GroupBitwiseAndKHR(int Execution, int Operation, short X);
int __attribute__((overloadable))
__spirv_GroupBitwiseAndKHR(int Execution, int Operation, int X);
long __attribute__((overloadable))
__spirv_GroupBitwiseAndKHR(int Execution, int Operation, long X);

char __attribute__((overloadable))
__spirv_GroupBitwiseOrKHR(int Execution, int Operation, char X);
short __attribute__((overloadable))
__spirv_GroupBitwiseOrKHR(int Execution, int Operation, short X);
int __attribute__((overloadable))
__spirv_GroupBitwiseOrKHR(int Execution, int Operation, int X);
long __attribute__((overloadable))
__spirv_GroupBitwiseOrKHR(int Execution, int Operation, long X);

char __attribute__((overloadable))
__spirv_GroupBitwiseXorKHR(int Execution, int Operation, char X);
short __attribute__((overloadable))
__spirv_GroupBitwiseXorKHR(int Execution, int Operation, short X);
int __attribute__((overloadable))
__spirv_GroupBitwiseXorKHR(int Execution, int Operation, int X);
long __attribute__((overloadable))
__spirv_GroupBitwiseXorKHR(int Execution, int Operation, long X);

bool __attribute__((overloadable))
__spirv_GroupLogicalAndKHR(int Execution, int Operation, bool X);
bool __attribute__((overloadable))
__spirv_GroupLogicalOrKHR(int Execution, int Operation, bool X);
bool __attribute__((overloadable))
__spirv_GroupLogicalXorKHR(int Execution, int Operation, bool X);

// Device-Side Enqueue Instructions
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
uint __attribute__((overloadable)) __spirv_EnqueueMarker(
    __spirv_Queue                Queue,
    uint                         NumEvents,
    __spirv_DeviceEvent private *WaitEvents,
    __spirv_DeviceEvent private *RetEvent);
uint __attribute__((overloadable)) __spirv_EnqueueMarker(
    __spirv_Queue              Queue,
    uint                       NumEvents,
    __spirv_DeviceEvent local *WaitEvents,
    __spirv_DeviceEvent local *RetEvent);
uint __attribute__((overloadable)) __spirv_EnqueueMarker(
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
void __attribute__((overloadable)) __spirv_RetainEvent(__spirv_DeviceEvent Event);
void __attribute__((overloadable)) __spirv_ReleaseEvent(__spirv_DeviceEvent Event);
__spirv_DeviceEvent __attribute__((overloadable)) __spirv_CreateUserEvent(void);
bool __attribute__((overloadable)) __spirv_IsValidEvent(__spirv_DeviceEvent Event);
void __attribute__((overloadable))
__spirv_SetUserEventStatus(__spirv_DeviceEvent Event, int Status);
void __attribute__((overloadable)) __spirv_CaptureEventProfilingInfo(
    __spirv_DeviceEvent Event, int ProfilingInfo, global char *Value);
__spirv_Queue __attribute__((overloadable)) __spirv_GetDefaultQueue(void);

#define DECL_BUILD_NDRANGE(TYPE, TYPE_MANGLING)                                   \
    Ndrange_t __attribute__((overloadable)) __spirv_BuildNDRange_1D(              \
        TYPE GlobalWorkSize, TYPE LocalWorkSize, TYPE GlobalWorkOffset);          \
    Ndrange_t __attribute__((overloadable)) __spirv_BuildNDRange_2D(              \
        TYPE GlobalWorkSize[2], TYPE LocalWorkSize[2], TYPE GlobalWorkOffset[2]); \
    Ndrange_t __attribute__((overloadable)) _spirv_BuildNDRange_3D(               \
        TYPE GlobalWorkSize[3], TYPE LocalWorkSize[3], TYPE GlobalWorkOffset[3]);

#if __32bit__ > 0
DECL_BUILD_NDRANGE(int, i32)
#else
DECL_BUILD_NDRANGE(long, i64)
#endif

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

// Pipe Instructions
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int __attribute__((overloadable)) __spirv_ReadPipe(
    __spirv_Pipe_ro Pipe,
    generic char   *Pointer,
    int             PacketSize /*, int PacketAlignment*/);
int __attribute__((overloadable)) __spirv_WritePipe(
    __spirv_Pipe_wo Pipe,
    generic char   *Pointer,
    int             PacketSize /*, int PacketAlignment*/);

int __attribute__((overloadable)) __spirv_ReservedReadPipe(
    __spirv_Pipe_ro   Pipe,
    __spirv_ReserveId ReserveId,
    int               Index,
    generic char     *Pointer,
    int               PacketSize /*, int PacketAlignment*/);
int __attribute__((overloadable)) __spirv_ReservedWritePipe(
    __spirv_Pipe_wo   Pipe,
    __spirv_ReserveId ReserveId,
    int               Index,
    generic char     *Pointer,
    int               PacketSize /*, int PacketAlignment*/);

__spirv_ReserveId __attribute__((overloadable)) __spirv_ReserveReadPipePackets(
    __spirv_Pipe_ro Pipe, int NumPackets, int PacketSize /*, int PacketAlignment*/);
__spirv_ReserveId __attribute__((overloadable)) __spirv_ReserveWritePipePackets(
    __spirv_Pipe_wo Pipe, int NumPackets, int PacketSize /*, int PacketAlignment*/);

void __attribute__((overloadable)) __spirv_CommitReadPipe(
    __spirv_Pipe_ro   Pipe,
    __spirv_ReserveId ReserveId,
    int               PacketSize /*, int PacketAlignment*/);
void __attribute__((overloadable)) __spirv_CommitWritePipe(
    __spirv_Pipe_wo   Pipe,
    __spirv_ReserveId ReserveId,
    int               PacketSize /*, int PacketAlignment*/);

bool __attribute__((overloadable)) __spirv_IsValidReserveId(__spirv_ReserveId ReserveId);

uint __attribute__((overloadable))
__spirv_GetNumPipePackets(__spirv_Pipe_ro Pipe, int PacketSize /*, int PacketAlignment*/);
uint __attribute__((overloadable))
__spirv_GetNumPipePackets(__spirv_Pipe_wo Pipe, int PacketSize /*, int PacketAlignment*/);
uint __attribute__((overloadable))
__spirv_GetMaxPipePackets(__spirv_Pipe_ro Pipe, int PacketSize /*, int PacketAlignment*/);
uint __attribute__((overloadable))
__spirv_GetMaxPipePackets(__spirv_Pipe_wo Pipe, int PacketSize /*, int PacketAlignment*/);

__spirv_ReserveId __attribute__((overloadable)) __spirv_GroupReserveReadPipePackets(
    int             Execution,
    __spirv_Pipe_ro Pipe,
    int             NumPackets,
    int             PacketSize /*, int PacketAlignment*/);
__spirv_ReserveId __attribute__((overloadable)) __spirv_GroupReserveWritePipePackets(
    int             Execution,
    __spirv_Pipe_wo Pipe,
    int             NumPackets,
    int             PacketSize /*, int PacketAlignment*/);

void __attribute__((overloadable)) __spirv_GroupCommitReadPipe(
    int               Execution,
    __spirv_Pipe_ro   Pipe,
    __spirv_ReserveId ReserveId,
    int               PacketSize /*, int PacketAlignment*/);
void __attribute__((overloadable)) __spirv_GroupCommitWritePipe(
    int               Execution,
    __spirv_Pipe_wo   Pipe,
    __spirv_ReserveId ReserveId,
    int               PacketSize /*, int PacketAlignment*/);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#include "spirv_math.h"

char2 __attribute__((overloadable))   __spirv_ocl_shuffle(char2 v, char2 m);
char2 __attribute__((overloadable))   __spirv_ocl_shuffle(char4 v, char2 m);
char2 __attribute__((overloadable))   __spirv_ocl_shuffle(char8 v, char2 m);
char2 __attribute__((overloadable))   __spirv_ocl_shuffle(char16 v, char2 m);
char4 __attribute__((overloadable))   __spirv_ocl_shuffle(char2 v, char4 m);
char4 __attribute__((overloadable))   __spirv_ocl_shuffle(char4 v, char4 m);
char4 __attribute__((overloadable))   __spirv_ocl_shuffle(char8 v, char4 m);
char4 __attribute__((overloadable))   __spirv_ocl_shuffle(char16 v, char4 m);
char8 __attribute__((overloadable))   __spirv_ocl_shuffle(char2 v, char8 m);
char8 __attribute__((overloadable))   __spirv_ocl_shuffle(char4 v, char8 m);
char8 __attribute__((overloadable))   __spirv_ocl_shuffle(char8 v, char8 m);
char8 __attribute__((overloadable))   __spirv_ocl_shuffle(char16 v, char8 m);
char16 __attribute__((overloadable))  __spirv_ocl_shuffle(char2 v, char16 m);
char16 __attribute__((overloadable))  __spirv_ocl_shuffle(char4 v, char16 m);
char16 __attribute__((overloadable))  __spirv_ocl_shuffle(char8 v, char16 m);
char16 __attribute__((overloadable))  __spirv_ocl_shuffle(char16 v, char16 m);
short2 __attribute__((overloadable))  __spirv_ocl_shuffle(short2 v, short2 m);
short2 __attribute__((overloadable))  __spirv_ocl_shuffle(short4 v, short2 m);
short2 __attribute__((overloadable))  __spirv_ocl_shuffle(short8 v, short2 m);
short2 __attribute__((overloadable))  __spirv_ocl_shuffle(short16 v, short2 m);
short4 __attribute__((overloadable))  __spirv_ocl_shuffle(short2 v, short4 m);
short4 __attribute__((overloadable))  __spirv_ocl_shuffle(short4 v, short4 m);
short4 __attribute__((overloadable))  __spirv_ocl_shuffle(short8 v, short4 m);
short4 __attribute__((overloadable))  __spirv_ocl_shuffle(short16 v, short4 m);
short8 __attribute__((overloadable))  __spirv_ocl_shuffle(short2 v, short8 m);
short8 __attribute__((overloadable))  __spirv_ocl_shuffle(short4 v, short8 m);
short8 __attribute__((overloadable))  __spirv_ocl_shuffle(short8 v, short8 m);
short8 __attribute__((overloadable))  __spirv_ocl_shuffle(short16 v, short8 m);
short16 __attribute__((overloadable)) __spirv_ocl_shuffle(short2 v, short16 m);
short16 __attribute__((overloadable)) __spirv_ocl_shuffle(short4 v, short16 m);
short16 __attribute__((overloadable)) __spirv_ocl_shuffle(short8 v, short16 m);
short16 __attribute__((overloadable)) __spirv_ocl_shuffle(short16 v, short16 m);
int2 __attribute__((overloadable))    __spirv_ocl_shuffle(int2 v, int2 m);
int2 __attribute__((overloadable))    __spirv_ocl_shuffle(int4 v, int2 m);
int2 __attribute__((overloadable))    __spirv_ocl_shuffle(int8 v, int2 m);
int2 __attribute__((overloadable))    __spirv_ocl_shuffle(int16 v, int2 m);
int4 __attribute__((overloadable))    __spirv_ocl_shuffle(int2 v, int4 m);
int4 __attribute__((overloadable))    __spirv_ocl_shuffle(int4 v, int4 m);
int4 __attribute__((overloadable))    __spirv_ocl_shuffle(int8 v, int4 m);
int4 __attribute__((overloadable))    __spirv_ocl_shuffle(int16 v, int4 m);
int8 __attribute__((overloadable))    __spirv_ocl_shuffle(int2 v, int8 m);
int8 __attribute__((overloadable))    __spirv_ocl_shuffle(int4 v, int8 m);
int8 __attribute__((overloadable))    __spirv_ocl_shuffle(int8 v, int8 m);
int8 __attribute__((overloadable))    __spirv_ocl_shuffle(int16 v, int8 m);
int16 __attribute__((overloadable))   __spirv_ocl_shuffle(int2 v, int16 m);
int16 __attribute__((overloadable))   __spirv_ocl_shuffle(int4 v, int16 m);
int16 __attribute__((overloadable))   __spirv_ocl_shuffle(int8 v, int16 m);
int16 __attribute__((overloadable))   __spirv_ocl_shuffle(int16 v, int16 m);
long2 __attribute__((overloadable))   __spirv_ocl_shuffle(long2 v, long2 m);
long2 __attribute__((overloadable))   __spirv_ocl_shuffle(long4 v, long2 m);
long2 __attribute__((overloadable))   __spirv_ocl_shuffle(long8 v, long2 m);
long2 __attribute__((overloadable))   __spirv_ocl_shuffle(long16 v, long2 m);
long4 __attribute__((overloadable))   __spirv_ocl_shuffle(long2 v, long4 m);
long4 __attribute__((overloadable))   __spirv_ocl_shuffle(long4 v, long4 m);
long4 __attribute__((overloadable))   __spirv_ocl_shuffle(long8 v, long4 m);
long4 __attribute__((overloadable))   __spirv_ocl_shuffle(long16 v, long4 m);
long8 __attribute__((overloadable))   __spirv_ocl_shuffle(long2 v, long8 m);
long8 __attribute__((overloadable))   __spirv_ocl_shuffle(long4 v, long8 m);
long8 __attribute__((overloadable))   __spirv_ocl_shuffle(long8 v, long8 m);
long8 __attribute__((overloadable))   __spirv_ocl_shuffle(long16 v, long8 m);
long16 __attribute__((overloadable))  __spirv_ocl_shuffle(long2 v, long16 m);
long16 __attribute__((overloadable))  __spirv_ocl_shuffle(long4 v, long16 m);
long16 __attribute__((overloadable))  __spirv_ocl_shuffle(long8 v, long16 m);
long16 __attribute__((overloadable))  __spirv_ocl_shuffle(long16 v, long16 m);
float2 __attribute__((overloadable))  __spirv_ocl_shuffle(float2 v, int2 m);
float2 __attribute__((overloadable))  __spirv_ocl_shuffle(float4 v, int2 m);
float2 __attribute__((overloadable))  __spirv_ocl_shuffle(float8 v, int2 m);
float2 __attribute__((overloadable))  __spirv_ocl_shuffle(float16 v, int2 m);
float4 __attribute__((overloadable))  __spirv_ocl_shuffle(float2 v, int4 m);
float4 __attribute__((overloadable))  __spirv_ocl_shuffle(float4 v, int4 m);
float4 __attribute__((overloadable))  __spirv_ocl_shuffle(float8 v, int4 m);
float4 __attribute__((overloadable))  __spirv_ocl_shuffle(float16 v, int4 m);
float8 __attribute__((overloadable))  __spirv_ocl_shuffle(float2 v, int8 m);
float8 __attribute__((overloadable))  __spirv_ocl_shuffle(float4 v, int8 m);
float8 __attribute__((overloadable))  __spirv_ocl_shuffle(float8 v, int8 m);
float8 __attribute__((overloadable))  __spirv_ocl_shuffle(float16 v, int8 m);
float16 __attribute__((overloadable)) __spirv_ocl_shuffle(float2 v, int16 m);
float16 __attribute__((overloadable)) __spirv_ocl_shuffle(float4 v, int16 m);
float16 __attribute__((overloadable)) __spirv_ocl_shuffle(float8 v, int16 m);
float16 __attribute__((overloadable)) __spirv_ocl_shuffle(float16 v, int16 m);

char2 __attribute__((overloadable))  __spirv_ocl_shuffle2(char2 v0, char2 v1, char2 m);
char2 __attribute__((overloadable))  __spirv_ocl_shuffle2(char4 v0, char4 v1, char2 m);
char2 __attribute__((overloadable))  __spirv_ocl_shuffle2(char8 v0, char8 v1, char2 m);
char2 __attribute__((overloadable))  __spirv_ocl_shuffle2(char16 v0, char16 v1, char2 m);
char4 __attribute__((overloadable))  __spirv_ocl_shuffle2(char2 v0, char2 v1, char4 m);
char4 __attribute__((overloadable))  __spirv_ocl_shuffle2(char4 v0, char4 v1, char4 m);
char4 __attribute__((overloadable))  __spirv_ocl_shuffle2(char8 v0, char8 v1, char4 m);
char4 __attribute__((overloadable))  __spirv_ocl_shuffle2(char16 v0, char16 v1, char4 m);
char8 __attribute__((overloadable))  __spirv_ocl_shuffle2(char2 v0, char2 v1, char8 m);
char8 __attribute__((overloadable))  __spirv_ocl_shuffle2(char4 v0, char4 v1, char8 m);
char8 __attribute__((overloadable))  __spirv_ocl_shuffle2(char8 v0, char8 v1, char8 m);
char8 __attribute__((overloadable))  __spirv_ocl_shuffle2(char16 v0, char16 v1, char8 m);
char16 __attribute__((overloadable)) __spirv_ocl_shuffle2(char2 v0, char2 v1, char16 m);
char16 __attribute__((overloadable)) __spirv_ocl_shuffle2(char4 v0, char4 v1, char16 m);
char16 __attribute__((overloadable)) __spirv_ocl_shuffle2(char8 v0, char8 v1, char16 m);
char16 __attribute__((overloadable)) __spirv_ocl_shuffle2(char16 v0, char16 v1, char16 m);
short2 __attribute__((overloadable)) __spirv_ocl_shuffle2(short2 v0, short2 v1, short2 m);
short2 __attribute__((overloadable)) __spirv_ocl_shuffle2(short4 v0, short4 v1, short2 m);
short2 __attribute__((overloadable)) __spirv_ocl_shuffle2(short8 v0, short8 v1, short2 m);
short2 __attribute__((overloadable))
__spirv_ocl_shuffle2(short16 v0, short16 v1, short2 m);
short4 __attribute__((overloadable)) __spirv_ocl_shuffle2(short2 v0, short2 v1, short4 m);
short4 __attribute__((overloadable)) __spirv_ocl_shuffle2(short4 v0, short4 v1, short4 m);
short4 __attribute__((overloadable)) __spirv_ocl_shuffle2(short8 v0, short8 v1, short4 m);
short4 __attribute__((overloadable))
__spirv_ocl_shuffle2(short16 v0, short16 v1, short4 m);
short8 __attribute__((overloadable)) __spirv_ocl_shuffle2(short2 v0, short2 v1, short8 m);
short8 __attribute__((overloadable)) __spirv_ocl_shuffle2(short4 v0, short4 v1, short8 m);
short8 __attribute__((overloadable)) __spirv_ocl_shuffle2(short8 v0, short8 v1, short8 m);
short8 __attribute__((overloadable))
__spirv_ocl_shuffle2(short16 v0, short16 v1, short8 m);
short16 __attribute__((overloadable))
__spirv_ocl_shuffle2(short2 v0, short2 v1, short16 m);
short16 __attribute__((overloadable))
__spirv_ocl_shuffle2(short4 v0, short4 v1, short16 m);
short16 __attribute__((overloadable))
__spirv_ocl_shuffle2(short8 v0, short8 v1, short16 m);
short16 __attribute__((overloadable))
__spirv_ocl_shuffle2(short16 v0, short16 v1, short16 m);
int2 __attribute__((overloadable))   __spirv_ocl_shuffle2(int2 v0, int2 v1, int2 m);
int2 __attribute__((overloadable))   __spirv_ocl_shuffle2(int4 v0, int4 v1, int2 m);
int2 __attribute__((overloadable))   __spirv_ocl_shuffle2(int8 v0, int8 v1, int2 m);
int2 __attribute__((overloadable))   __spirv_ocl_shuffle2(int16 v0, int16 v1, int2 m);
int4 __attribute__((overloadable))   __spirv_ocl_shuffle2(int2 v0, int2 v1, int4 m);
int4 __attribute__((overloadable))   __spirv_ocl_shuffle2(int4 v0, int4 v1, int4 m);
int4 __attribute__((overloadable))   __spirv_ocl_shuffle2(int8 v0, int8 v1, int4 m);
int4 __attribute__((overloadable))   __spirv_ocl_shuffle2(int16 v0, int16 v1, int4 m);
int8 __attribute__((overloadable))   __spirv_ocl_shuffle2(int2 v0, int2 v1, int8 m);
int8 __attribute__((overloadable))   __spirv_ocl_shuffle2(int4 v0, int4 v1, int8 m);
int8 __attribute__((overloadable))   __spirv_ocl_shuffle2(int8 v0, int8 v1, int8 m);
int8 __attribute__((overloadable))   __spirv_ocl_shuffle2(int16 v0, int16 v1, int8 m);
int16 __attribute__((overloadable))  __spirv_ocl_shuffle2(int2 v0, int2 v1, int16 m);
int16 __attribute__((overloadable))  __spirv_ocl_shuffle2(int4 v0, int4 v1, int16 m);
int16 __attribute__((overloadable))  __spirv_ocl_shuffle2(int8 v0, int8 v1, int16 m);
int16 __attribute__((overloadable))  __spirv_ocl_shuffle2(int16 v0, int16 v1, int16 m);
long2 __attribute__((overloadable))  __spirv_ocl_shuffle2(long2 v0, long2 v1, long2 m);
long2 __attribute__((overloadable))  __spirv_ocl_shuffle2(long4 v0, long4 v1, long2 m);
long2 __attribute__((overloadable))  __spirv_ocl_shuffle2(long8 v0, long8 v1, long2 m);
long2 __attribute__((overloadable))  __spirv_ocl_shuffle2(long16 v0, long16 v1, long2 m);
long4 __attribute__((overloadable))  __spirv_ocl_shuffle2(long2 v0, long2 v1, long4 m);
long4 __attribute__((overloadable))  __spirv_ocl_shuffle2(long4 v0, long4 v1, long4 m);
long4 __attribute__((overloadable))  __spirv_ocl_shuffle2(long8 v0, long8 v1, long4 m);
long4 __attribute__((overloadable))  __spirv_ocl_shuffle2(long16 v0, long16 v1, long4 m);
long8 __attribute__((overloadable))  __spirv_ocl_shuffle2(long2 v0, long2 v1, long8 m);
long8 __attribute__((overloadable))  __spirv_ocl_shuffle2(long4 v0, long4 v1, long8 m);
long8 __attribute__((overloadable))  __spirv_ocl_shuffle2(long8 v0, long8 v1, long8 m);
long8 __attribute__((overloadable))  __spirv_ocl_shuffle2(long16 v0, long16 v1, long8 m);
long16 __attribute__((overloadable)) __spirv_ocl_shuffle2(long2 v0, long2 v1, long16 m);
long16 __attribute__((overloadable)) __spirv_ocl_shuffle2(long4 v0, long4 v1, long16 m);
long16 __attribute__((overloadable)) __spirv_ocl_shuffle2(long8 v0, long8 v1, long16 m);
long16 __attribute__((overloadable)) __spirv_ocl_shuffle2(long16 v0, long16 v1, long16 m);
float2 __attribute__((overloadable)) __spirv_ocl_shuffle2(float2 v0, float2 v1, int2 m);
float2 __attribute__((overloadable)) __spirv_ocl_shuffle2(float4 v0, float4 v1, int2 m);
float2 __attribute__((overloadable)) __spirv_ocl_shuffle2(float8 v0, float8 v1, int2 m);
float2 __attribute__((overloadable)) __spirv_ocl_shuffle2(float16 v0, float16 v1, int2 m);
float4 __attribute__((overloadable)) __spirv_ocl_shuffle2(float2 v0, float2 v1, int4 m);
float4 __attribute__((overloadable)) __spirv_ocl_shuffle2(float4 v0, float4 v1, int4 m);
float4 __attribute__((overloadable)) __spirv_ocl_shuffle2(float8 v0, float8 v1, int4 m);
float4 __attribute__((overloadable)) __spirv_ocl_shuffle2(float16 v0, float16 v1, int4 m);
float8 __attribute__((overloadable)) __spirv_ocl_shuffle2(float2 v0, float2 v1, int8 m);
float8 __attribute__((overloadable)) __spirv_ocl_shuffle2(float4 v0, float4 v1, int8 m);
float8 __attribute__((overloadable)) __spirv_ocl_shuffle2(float8 v0, float8 v1, int8 m);
float8 __attribute__((overloadable)) __spirv_ocl_shuffle2(float16 v0, float16 v1, int8 m);
float16 __attribute__((overloadable)) __spirv_ocl_shuffle2(float2 v0, float2 v1, int16 m);
float16 __attribute__((overloadable)) __spirv_ocl_shuffle2(float4 v0, float4 v1, int16 m);
float16 __attribute__((overloadable)) __spirv_ocl_shuffle2(float8 v0, float8 v1, int16 m);
float16 __attribute__((overloadable))
__spirv_ocl_shuffle2(float16 v0, float16 v1, int16 m);

half2 __attribute__((overloadable))  __spirv_ocl_shuffle(half2 v, short2 m);
half2 __attribute__((overloadable))  __spirv_ocl_shuffle(half4 v, short2 m);
half2 __attribute__((overloadable))  __spirv_ocl_shuffle(half8 v, short2 m);
half2 __attribute__((overloadable))  __spirv_ocl_shuffle(half16 v, short2 m);
half4 __attribute__((overloadable))  __spirv_ocl_shuffle(half2 v, short4 m);
half4 __attribute__((overloadable))  __spirv_ocl_shuffle(half4 v, short4 m);
half4 __attribute__((overloadable))  __spirv_ocl_shuffle(half8 v, short4 m);
half4 __attribute__((overloadable))  __spirv_ocl_shuffle(half16 v, short4 m);
half8 __attribute__((overloadable))  __spirv_ocl_shuffle(half2 v, short8 m);
half8 __attribute__((overloadable))  __spirv_ocl_shuffle(half4 v, short8 m);
half8 __attribute__((overloadable))  __spirv_ocl_shuffle(half8 v, short8 m);
half8 __attribute__((overloadable))  __spirv_ocl_shuffle(half16 v, short8 m);
half16 __attribute__((overloadable)) __spirv_ocl_shuffle(half2 v, short16 m);
half16 __attribute__((overloadable)) __spirv_ocl_shuffle(half4 v, short16 m);
half16 __attribute__((overloadable)) __spirv_ocl_shuffle(half8 v, short16 m);
half16 __attribute__((overloadable)) __spirv_ocl_shuffle(half16 v, short16 m);

half2 __attribute__((overloadable))  __spirv_ocl_shuffle2(half2 v0, half2 v1, short2 m);
half2 __attribute__((overloadable))  __spirv_ocl_shuffle2(half4 v0, half4 v1, short2 m);
half2 __attribute__((overloadable))  __spirv_ocl_shuffle2(half8 v0, half8 v1, short2 m);
half2 __attribute__((overloadable))  __spirv_ocl_shuffle2(half16 v0, half16 v1, short2 m);
half4 __attribute__((overloadable))  __spirv_ocl_shuffle2(half2 v0, half2 v1, short4 m);
half4 __attribute__((overloadable))  __spirv_ocl_shuffle2(half4 v0, half4 v1, short4 m);
half4 __attribute__((overloadable))  __spirv_ocl_shuffle2(half8 v0, half8 v1, short4 m);
half4 __attribute__((overloadable))  __spirv_ocl_shuffle2(half16 v0, half16 v1, short4 m);
half8 __attribute__((overloadable))  __spirv_ocl_shuffle2(half2 v0, half2 v1, short8 m);
half8 __attribute__((overloadable))  __spirv_ocl_shuffle2(half4 v0, half4 v1, short8 m);
half8 __attribute__((overloadable))  __spirv_ocl_shuffle2(half8 v0, half8 v1, short8 m);
half8 __attribute__((overloadable))  __spirv_ocl_shuffle2(half16 v0, half16 v1, short8 m);
half16 __attribute__((overloadable)) __spirv_ocl_shuffle2(half2 v0, half2 v1, short16 m);
half16 __attribute__((overloadable)) __spirv_ocl_shuffle2(half4 v0, half4 v1, short16 m);
half16 __attribute__((overloadable)) __spirv_ocl_shuffle2(half8 v0, half8 v1, short16 m);
half16 __attribute__((overloadable))
__spirv_ocl_shuffle2(half16 v0, half16 v1, short16 m);

#if defined(cl_khr_fp64)
double2 __attribute__((overloadable))  __spirv_ocl_shuffle(double2 v, long2 m);
double2 __attribute__((overloadable))  __spirv_ocl_shuffle(double4 v, long2 m);
double2 __attribute__((overloadable))  __spirv_ocl_shuffle(double8 v, long2 m);
double2 __attribute__((overloadable))  __spirv_ocl_shuffle(double16 v, long2 m);
double4 __attribute__((overloadable))  __spirv_ocl_shuffle(double2 v, long4 m);
double4 __attribute__((overloadable))  __spirv_ocl_shuffle(double4 v, long4 m);
double4 __attribute__((overloadable))  __spirv_ocl_shuffle(double8 v, long4 m);
double4 __attribute__((overloadable))  __spirv_ocl_shuffle(double16 v, long4 m);
double8 __attribute__((overloadable))  __spirv_ocl_shuffle(double2 v, long8 m);
double8 __attribute__((overloadable))  __spirv_ocl_shuffle(double4 v, long8 m);
double8 __attribute__((overloadable))  __spirv_ocl_shuffle(double8 v, long8 m);
double8 __attribute__((overloadable))  __spirv_ocl_shuffle(double16 v, long8 m);
double16 __attribute__((overloadable)) __spirv_ocl_shuffle(double2 v, long16 m);
double16 __attribute__((overloadable)) __spirv_ocl_shuffle(double4 v, long16 m);
double16 __attribute__((overloadable)) __spirv_ocl_shuffle(double8 v, long16 m);
double16 __attribute__((overloadable)) __spirv_ocl_shuffle(double16 v, long16 m);

double2 __attribute__((overloadable))
__spirv_ocl_shuffle2(double2 v0, double2 v1, long2 m);
double2 __attribute__((overloadable))
__spirv_ocl_shuffle2(double4 v0, double4 v1, long2 m);
double2 __attribute__((overloadable))
__spirv_ocl_shuffle2(double8 v0, double8 v1, long2 m);
double2 __attribute__((overloadable))
__spirv_ocl_shuffle2(double16 v0, double16 v1, long2 m);
double4 __attribute__((overloadable))
__spirv_ocl_shuffle2(double2 v0, double2 v1, long4 m);
double4 __attribute__((overloadable))
__spirv_ocl_shuffle2(double4 v0, double4 v1, long4 m);
double4 __attribute__((overloadable))
__spirv_ocl_shuffle2(double8 v0, double8 v1, long4 m);
double4 __attribute__((overloadable))
__spirv_ocl_shuffle2(double16 v0, double16 v1, long4 m);
double8 __attribute__((overloadable))
__spirv_ocl_shuffle2(double2 v0, double2 v1, long8 m);
double8 __attribute__((overloadable))
__spirv_ocl_shuffle2(double4 v0, double4 v1, long8 m);
double8 __attribute__((overloadable))
__spirv_ocl_shuffle2(double8 v0, double8 v1, long8 m);
double8 __attribute__((overloadable))
__spirv_ocl_shuffle2(double16 v0, double16 v1, long8 m);
double16 __attribute__((overloadable))
__spirv_ocl_shuffle2(double2 v0, double2 v1, long16 m);
double16 __attribute__((overloadable))
__spirv_ocl_shuffle2(double4 v0, double4 v1, long16 m);
double16 __attribute__((overloadable))
__spirv_ocl_shuffle2(double8 v0, double8 v1, long16 m);
double16 __attribute__((overloadable))
__spirv_ocl_shuffle2(double16 v0, double16 v1, long16 m);
#endif // defined(cl_khr_fp64)

char __attribute__((overloadable))     __spirv_ocl_s_min(char x, char y);
char2 __attribute__((overloadable))    __spirv_ocl_s_min(char2 x, char2 y);
char3 __attribute__((overloadable))    __spirv_ocl_s_min(char3 x, char3 y);
char4 __attribute__((overloadable))    __spirv_ocl_s_min(char4 x, char4 y);
char8 __attribute__((overloadable))    __spirv_ocl_s_min(char8 x, char8 y);
char16 __attribute__((overloadable))   __spirv_ocl_s_min(char16 x, char16 y);
uchar __attribute__((overloadable))    __spirv_ocl_u_min(uchar x, uchar y);
uchar2 __attribute__((overloadable))   __spirv_ocl_u_min(uchar2 x, uchar2 y);
uchar3 __attribute__((overloadable))   __spirv_ocl_u_min(uchar3 x, uchar3 y);
uchar4 __attribute__((overloadable))   __spirv_ocl_u_min(uchar4 x, uchar4 y);
uchar8 __attribute__((overloadable))   __spirv_ocl_u_min(uchar8 x, uchar8 y);
uchar16 __attribute__((overloadable))  __spirv_ocl_u_min(uchar16 x, uchar16 y);
short __attribute__((overloadable))    __spirv_ocl_s_min(short x, short y);
short2 __attribute__((overloadable))   __spirv_ocl_s_min(short2 x, short2 y);
short3 __attribute__((overloadable))   __spirv_ocl_s_min(short3 x, short3 y);
short4 __attribute__((overloadable))   __spirv_ocl_s_min(short4 x, short4 y);
short8 __attribute__((overloadable))   __spirv_ocl_s_min(short8 x, short8 y);
short16 __attribute__((overloadable))  __spirv_ocl_s_min(short16 x, short16 y);
ushort __attribute__((overloadable))   __spirv_ocl_u_min(ushort x, ushort y);
ushort2 __attribute__((overloadable))  __spirv_ocl_u_min(ushort2 x, ushort2 y);
ushort3 __attribute__((overloadable))  __spirv_ocl_u_min(ushort3 x, ushort3 y);
ushort4 __attribute__((overloadable))  __spirv_ocl_u_min(ushort4 x, ushort4 y);
ushort8 __attribute__((overloadable))  __spirv_ocl_u_min(ushort8 x, ushort8 y);
ushort16 __attribute__((overloadable)) __spirv_ocl_u_min(ushort16 x, ushort16 y);
int __attribute__((overloadable))      __spirv_ocl_s_min(int x, int y);
int2 __attribute__((overloadable))     __spirv_ocl_s_min(int2 x, int2 y);
int3 __attribute__((overloadable))     __spirv_ocl_s_min(int3 x, int3 y);
int4 __attribute__((overloadable))     __spirv_ocl_s_min(int4 x, int4 y);
int8 __attribute__((overloadable))     __spirv_ocl_s_min(int8 x, int8 y);
int16 __attribute__((overloadable))    __spirv_ocl_s_min(int16 x, int16 y);
uint __attribute__((overloadable))     __spirv_ocl_u_min(uint x, uint y);
uint2 __attribute__((overloadable))    __spirv_ocl_u_min(uint2 x, uint2 y);
uint3 __attribute__((overloadable))    __spirv_ocl_u_min(uint3 x, uint3 y);
uint4 __attribute__((overloadable))    __spirv_ocl_u_min(uint4 x, uint4 y);
uint8 __attribute__((overloadable))    __spirv_ocl_u_min(uint8 x, uint8 y);
uint16 __attribute__((overloadable))   __spirv_ocl_u_min(uint16 x, uint16 y);
long __attribute__((overloadable))     __spirv_ocl_s_min(long x, long y);
long2 __attribute__((overloadable))    __spirv_ocl_s_min(long2 x, long2 y);
long3 __attribute__((overloadable))    __spirv_ocl_s_min(long3 x, long3 y);
long4 __attribute__((overloadable))    __spirv_ocl_s_min(long4 x, long4 y);
long8 __attribute__((overloadable))    __spirv_ocl_s_min(long8 x, long8 y);
long16 __attribute__((overloadable))   __spirv_ocl_s_min(long16 x, long16 y);
ulong __attribute__((overloadable))    __spirv_ocl_u_min(ulong x, ulong y);
ulong2 __attribute__((overloadable))   __spirv_ocl_u_min(ulong2 x, ulong2 y);
ulong3 __attribute__((overloadable))   __spirv_ocl_u_min(ulong3 x, ulong3 y);
ulong4 __attribute__((overloadable))   __spirv_ocl_u_min(ulong4 x, ulong4 y);
ulong8 __attribute__((overloadable))   __spirv_ocl_u_min(ulong8 x, ulong8 y);
ulong16 __attribute__((overloadable))  __spirv_ocl_u_min(ulong16 x, ulong16 y);

// Misc Instructions

void __attribute__((overloadable)) __spirv_ocl_prefetch(global char *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global char2 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global char3 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global char4 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global char8 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global char16 *p, int num_elements);

void __attribute__((overloadable))
__spirv_ocl_prefetch(global short *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global short2 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global short3 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global short4 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global short8 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global short16 *p, int num_elements);

void __attribute__((overloadable)) __spirv_ocl_prefetch(global int *p, int num_elements);
void __attribute__((overloadable)) __spirv_ocl_prefetch(global int2 *p, int num_elements);
void __attribute__((overloadable)) __spirv_ocl_prefetch(global int3 *p, int num_elements);
void __attribute__((overloadable)) __spirv_ocl_prefetch(global int4 *p, int num_elements);
void __attribute__((overloadable)) __spirv_ocl_prefetch(global int8 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global int16 *p, int num_elements);

void __attribute__((overloadable)) __spirv_ocl_prefetch(global long *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global long2 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global long3 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global long4 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global long8 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global long16 *p, int num_elements);

void __attribute__((overloadable))
__spirv_ocl_prefetch(global float *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global float2 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global float3 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global float4 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global float8 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global float16 *p, int num_elements);

void __attribute__((overloadable)) __spirv_ocl_prefetch(global half *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global half2 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global half3 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global half4 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global half8 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global half16 *p, int num_elements);

#if defined(cl_khr_fp64)
void __attribute__((overloadable))
__spirv_ocl_prefetch(global double *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global double2 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global double3 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global double4 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global double8 *p, int num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global double16 *p, int num_elements);
#endif // defined(cl_khr_fp64)

void
    __attribute__((overloadable)) __spirv_ocl_prefetch(global char *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global char2 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global char3 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global char4 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global char8 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global char16 *p, long num_elements);

void __attribute__((overloadable))
__spirv_ocl_prefetch(global short *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global short2 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global short3 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global short4 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global short8 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global short16 *p, long num_elements);

void __attribute__((overloadable)) __spirv_ocl_prefetch(global int *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global int2 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global int3 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global int4 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global int8 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global int16 *p, long num_elements);

void __attribute__((overloadable))
__spirv_ocl_prefetch(global long *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global long2 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global long3 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global long4 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global long8 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global long16 *p, long num_elements);

void __attribute__((overloadable))
__spirv_ocl_prefetch(global float *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global float2 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global float3 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global float4 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global float8 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global float16 *p, long num_elements);

void __attribute__((overloadable))
__spirv_ocl_prefetch(global half *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global half2 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global half3 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global half4 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global half8 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global half16 *p, long num_elements);

#if defined(cl_khr_fp64)
void __attribute__((overloadable))
__spirv_ocl_prefetch(global double *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global double2 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global double3 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global double4 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global double8 *p, long num_elements);
void __attribute__((overloadable))
__spirv_ocl_prefetch(global double16 *p, long num_elements);
#endif // defined(cl_khr_fp64)

ulong __attribute__((overloadable)) __spirv_ReadClockKHR_Rulong(int scope);
uint2 __attribute__((overloadable)) __spirv_ReadClockKHR_Ruint2(int scope);

int __attribute__((overloadable)) __spirv_BuiltInSubDeviceIDINTEL(void);
int __attribute__((overloadable)) __spirv_GlobalHWThreadIDINTEL(void);

#endif // __SPIRV_H__
