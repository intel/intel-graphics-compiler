/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __SPIRV_H__
#define __SPIRV_H__

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
    None = 0x0,
    CmdExecTime = 0x1
} KernelProfilingInfo_t;

typedef enum
{
    CrossDevice = 0,
    Device      = 1,
    Workgroup   = 2,
    Subgroup    = 3,
    Invocation  = 4
} Scope_t;

typedef enum
{
    Relaxed                = 0x0,
    Acquire                = 0x2,
    Release                = 0x4,
    AcquireRelease         = 0x8,
    SequentiallyConsistent = 0x10,
    UniformMemory          = 0x40,
    SubgroupMemory         = 0x80,
    WorkgroupMemory        = 0x100,
    CrossWorkgroupMemory   = 0x200,
    AtomicCounterMemory    = 0x400,
    ImageMemory            = 0x800
} MemorySemantics_t;

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
    GroupOperationReduce = 0,
    GroupOperationInclusiveScan = 1,
    GroupOperationExclusiveScan = 2,
    GroupOperationClusteredReduce = 3
} GroupOperations_t;

typedef enum
{
    rte = 0,
    rtz = 1,
    rtp = 2,
    rtn = 3
} RoundingMode_t;

// Note: Unify these defines in a common header that this and
// the cth include.

#if __32bit__ > 0
typedef uint size_t;
typedef uint3 size_t3;
typedef int ptrdiff_t;
#else
typedef ulong size_t;
typedef ulong3 size_t3;
typedef long ptrdiff_t;
#endif

typedef ptrdiff_t intptr_t;
typedef size_t uintptr_t;

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
#define M_E_H         2.71828182845904523536028747135266250h
#define M_LOG2E_H     1.44269504088896340735992468100189214h
#define M_LOG10E_H    0.434294481903251827651128918916605082h
#define M_LN2_H       0.693147180559945309417232121458176568h
#define M_LN10_H      2.30258509299404568401799145468436421h
#define M_PI_H        3.14159265358979323846264338327950288h
#define M_PI_2_H      1.57079632679489661923132169163975144h
#define M_PI_4_H      0.785398163397448309615660845819875721h
#define M_1_PI_H      0.318309886183790671537767526745028724h
#define M_2_PI_H      0.636619772367581343075535053490057448h
#define M_2_SQRTPI_H  1.12837916709551257389615890312154517h
#define M_SQRT2_H     1.41421356237309504880168872420969808h
#define M_SQRT1_2_H   0.707106781186547524400844362104849039h

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

#define M_E           0x1.5bf0a8b145769p+1
#define M_LOG2E       0x1.71547652b82fep+0
#define M_LOG10E      0x1.bcb7b1526e50ep-2
#define M_LN2         0x1.62e42fefa39efp-1
#define M_LN10        0x1.26bb1bbb55516p+1
#define M_PI          0x1.921fb54442d18p+1
#define M_PI_2        0x1.921fb54442d18p+0
#define M_PI_4        0x1.921fb54442d18p-1
#define M_1_PI        0x1.45f306dc9c883p-2
#define M_2_PI        0x1.45f306dc9c883p-1
#define M_2_SQRTPI    0x1.20dd750429b6dp+0
#define M_SQRT2       0x1.6a09e667f3bcdp+0
#define M_SQRT1_2     0x1.6a09e667f3bcdp-1

#define OLD_SPIRV_BUILTINS

#if defined(OLD_SPIRV_BUILTINS)
#define SPIRV_OVERLOADABLE
#define SPIRV_BUILTIN(opcode, old_mangling, new_mangling) \
    __builtin_spirv_Op##opcode##old_mangling
#define SPIRV_BUILTIN_NO_OP(opcode, old_mangling, new_mangling) \
    __builtin_spirv_##opcode##old_mangling
#define SPIRV_OCL_BUILTIN(func, old_mangling, new_mangling) \
    __builtin_spirv_OpenCL_##func##old_mangling
#else
#define SPIRV_OVERLOADABLE __attribute__((overloadable))
#define SPIRV_BUILTIN(opcode, old_mangling, new_mangling) \
    __spirv_##opcode##new_mangling
#define SPIRV_BUILTIN_NO_OP(opcode, old_mangling, new_mangling) \
    __spirv_##opcode##new_mangling
#define SPIRV_OCL_BUILTIN(func, old_mangling, new_mangling) \
    __spirv_ocl_##func##new_mangling
#endif


#if defined(cl_khr_fp64)
#define cl_fp64_basic_ops
#endif // cl_fp64_basic_ops

typedef uchar __bool2 __attribute__((ext_vector_type(2)));
typedef uchar __bool3 __attribute__((ext_vector_type(3)));
typedef uchar __bool4 __attribute__((ext_vector_type(4)));
typedef uchar __bool8 __attribute__((ext_vector_type(8)));
typedef uchar __bool16 __attribute__((ext_vector_type(16)));

typedef ulong    ImageType_t;
typedef ulong    Image_t;
typedef ulong    Sampler_t;
typedef ulong3   SampledImage_t;
typedef event_t  Event_t;
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
typedef queue_t Queue_t;
typedef clk_event_t ClkEvent_t;
typedef reserve_id_t ReserveId_t;
typedef pipe int Pipe_t;
typedef write_only pipe int Pipe_wo_t;

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#define INTEL_PIPE_RESERVE_ID_VALID_BIT (1U << 30)
#define CLK_NULL_RESERVE_ID (__builtin_astype(((void*)(~INTEL_PIPE_RESERVE_ID_VALID_BIT)), reserve_id_t))

// ImageType_t Encoding
//  Sampeled       & 0x3 << 62
//      0 = Unknown at compile time
//      1 = Will be sampled
//      2 = Will not be sampled
//  Dimension      & 0x7 << 59
//      0 = 1D
//      1 = 2D
//      2 = 3D
//      3 = Cube
//      4 = Rect
//      5 = Buffer
//      6 = SubpassData
//  Depth          & 0x1 << 58
//      0 == Non-Depth
//      1 == Depth
//  Arrayed        & 0x1 << 57
//      0 == Non-Arrayed
//      1 == Arrayed
//  MultiSampled  & 0x1 << 56
//      0 == Non-Multisampled
//      1 == Multisampled
//  AccessQualifer & 0x3 << 54
//      0 = Read Only
//      1 = Write Only
//      2 = Read/Write

typedef enum
{
    Dim1D = 0,
    Dim2D,
    Dim3D,
    DimCube,
    DimRect,
    DimBuffer,
    DimSubpassData,
    DimEnd
} Dimensionality_t;

#define IMAGETYPE_SAMPLED_SHIFT 62
#define IMAGETYPE_DIM_SHIFT 59
#define IMAGETYPE_DEPTH_SHIFT 58
#define IMAGETYPE_ARRAYED_SHIFT 57
#define IMAGETYPE_MULTISAMPLED_SHIFT 56
#define IMAGETYPE_ACCESSQUALIFER_SHIFT 54

// Keep track of SaturatedConversion

// Work-item functions

#if defined(OLD_SPIRV_BUILTINS)
size_t3 __builtin_spirv_BuiltInNumWorkgroups(void);
size_t3 __builtin_spirv_BuiltInWorkgroupSize(void);
size_t3 __builtin_spirv_BuiltInWorkgroupId(void);
size_t3 __builtin_spirv_BuiltInLocalInvocationId(void);
size_t3 __builtin_spirv_BuiltInGlobalInvocationId(void);
size_t3 __builtin_spirv_BuiltInGlobalSize(void);
size_t3 __builtin_spirv_BuiltInEnqueuedWorkgroupSize(void);
size_t3 __builtin_spirv_BuiltInGlobalOffset(void);
#else
size_t __attribute__((overloadable)) __spirv_BuiltInNumWorkgroups(int dimindx);
size_t __attribute__((overloadable)) __spirv_BuiltInWorkgroupSize(int dimindx);
size_t __attribute__((overloadable)) __spirv_BuiltInWorkgroupId(int dimindx);
size_t __attribute__((overloadable)) __spirv_BuiltInLocalInvocationId(int dimindx);
size_t __attribute__((overloadable)) __spirv_BuiltInGlobalInvocationId(int dimindx);
size_t __attribute__((overloadable)) __spirv_BuiltInGlobalSize(int dimindx);
size_t __attribute__((overloadable)) __spirv_BuiltInEnqueuedWorkgroupSize(int dimindx);
size_t __attribute__((overloadable)) __spirv_BuiltInGlobalOffset(int dimindx);
#endif // defined(OLD_SPIRV_BUILTINS)

size_t  SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInLocalInvocationIndex, , )(void);
size_t  SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInGlobalLinearId, , )(void);
uint    SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInWorkDim, , )(void);
uint    SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupMaxSize, , )(void);
uint    SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupId, , )(void);
uint    SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInNumSubgroups, , )(void);
uint    SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupSize, , )(void);
uint    SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInNumEnqueuedSubgroups, , )(void);
uint    SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )(void);

// Image Instructions
//
struct ImageDummy;
#define INLINEWA global struct ImageDummy* __dummy

SampledImage_t __builtin_spirv_OpSampledImage_i64_i64_i64(Image_t Image, ImageType_t ImageType, Sampler_t Sampler);

uint4  __builtin_spirv_OpImageSampleExplicitLod_v4i32_v3i64_v4i32_i32_f32_i64(SampledImage_t SampledImage, int4 Coordinate, uint ImageOperands, float Lod, INLINEWA);
uint4  __builtin_spirv_OpImageSampleExplicitLod_v4i32_v3i64_v4f32_i32_f32_i64(SampledImage_t SampledImage, float4 Coordinate, uint ImageOperands, float Lod, INLINEWA);
float4 __builtin_spirv_OpImageSampleExplicitLod_v4f32_v3i64_v4i32_i32_f32_i64(SampledImage_t SampledImage, int4 Coordinate, uint ImageOperands, float Lod, INLINEWA);
float4 __builtin_spirv_OpImageSampleExplicitLod_v4f32_v3i64_v4f32_i32_f32_i64(SampledImage_t SampledImage, float4 Coordinate, uint ImageOperands, float Lod, INLINEWA);

#ifdef cl_khr_fp16
half4 __builtin_spirv_OpImageSampleExplicitLod_v4f16_v3i64_v4i32_i32_f32_i64( SampledImage_t SampledImage, int4 Coordinate, uint ImageOperands, float Lod , INLINEWA);
half4 __builtin_spirv_OpImageSampleExplicitLod_v4f16_v3i64_v4f32_i32_f32_i64( SampledImage_t SampledImage, float4 Coordinate, uint ImageOperands, float Lod , INLINEWA);
void __builtin_spirv_OpImageWrite_i64_i64_v4i32_v4f16_i64(Image_t Image, ImageType_t ImageType, int4 Coordinate, half4 Texel, INLINEWA);
half4 __builtin_spirv_OpImageRead_v4f16_i64_i64_v4i32_i64( Image_t Image, ImageType_t ImageType, int4 Coordinate , INLINEWA);
#endif // cl_khr_fp16

// Pipe in image type information.
uint4  __builtin_spirv_OpImageRead_v4i32_i64_i64_v4i32_i64(Image_t Image, ImageType_t ImageType, int4 Coordinate, INLINEWA);
uint4  __builtin_spirv_OpImageRead_v4i32_i64_i64_v4f32_i64(Image_t Image, ImageType_t ImageType, float4 Coordinate, INLINEWA);
float4 __builtin_spirv_OpImageRead_v4f32_i64_i64_v4i32_i64(Image_t Image, ImageType_t ImageType, int4 Coordinate, INLINEWA);
float4 __builtin_spirv_OpImageRead_v4f32_i64_i64_v4f32_i64(Image_t Image, ImageType_t ImageType, float4 Coordinate, INLINEWA);

float4 __builtin_spirv_OpImageRead_v4f32_i64_i64_v4i32_i32_i32_i64( Image_t Image, ImageType_t ImageType, int4 Coordinate, uint ImageOperands, uint Sample, INLINEWA);
float __builtin_spirv_OpImageRead_f32_i64_i64_v4i32_i32_i32_i64( Image_t Image, ImageType_t ImageType, int4 Coordinate, uint ImageOperands, uint Sample, INLINEWA);
uint4 __builtin_spirv_OpImageRead_v4i32_i64_i64_v4i32_i32_i32_i64( Image_t Image, ImageType_t ImageType, int4 Coordinate, uint ImageOperands, uint Sample, INLINEWA);

void __builtin_spirv_OpImageWrite_i64_i64_v4i32_v4i32_i64(Image_t Image, ImageType_t ImageType, int4 Coordinate, uint4 Texel, INLINEWA);
void __builtin_spirv_OpImageWrite_i64_i64_v4i32_v4f32_i64(Image_t Image, ImageType_t ImageType, int4 Coordinate, float4 Texel, INLINEWA);

uint  __builtin_spirv_OpImageQueryFormat_i64_i64(Image_t Image, INLINEWA);
uint  __builtin_spirv_OpImageQueryOrder_i64_i64(Image_t Image, INLINEWA);

uint  __builtin_spirv_OpImageQuerySizeLod_i32_i64_i64_i32_i64(Image_t Image, ImageType_t ImageType, int Lod, INLINEWA);
uint2 __builtin_spirv_OpImageQuerySizeLod_v2i32_i64_i64_i32_i64(Image_t Image, ImageType_t ImageType, int Lod, INLINEWA);
uint3 __builtin_spirv_OpImageQuerySizeLod_v3i32_i64_i64_i32_i64(Image_t Image, ImageType_t ImageType, int Lod, INLINEWA);
uint4 __builtin_spirv_OpImageQuerySizeLod_v4i32_i64_i64_i32_i64(Image_t Image, ImageType_t ImageType, int Lod, INLINEWA);

uint  __builtin_spirv_OpImageQuerySize_i32_i64_i64_i64(Image_t Image, ImageType_t ImageType, INLINEWA);
uint2 __builtin_spirv_OpImageQuerySize_v2i32_i64_i64_i64(Image_t Image, ImageType_t ImageType, INLINEWA);
uint3 __builtin_spirv_OpImageQuerySize_v3i32_i64_i64_i64(Image_t Image, ImageType_t ImageType, INLINEWA);
uint4  __builtin_spirv_OpImageQuerySize_v4i32_i64_i64_i64(Image_t Image, ImageType_t ImageType, INLINEWA);

uint __builtin_spirv_OpImageQueryLevels_i64_i64(Image_t Image, INLINEWA);
uint __builtin_spirv_OpImageQuerySamples_i64_i64(Image_t Image, INLINEWA);

// Conversion Instructions

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(half FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i16_f16, _Rushort)(half FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i32_f16, _Ruint)(half FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i64_f16, _Rulong)(half FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i8_f32, _Ruchar)(float FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i16_f32, _Rushort)(float FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i32_f32, _Ruint)(float FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i64_f32, _Rulong)(float FloatValue);
#if defined(cl_khr_fp64)
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i8_f64, _Ruchar)(double FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i16_f64, _Rushort)(double FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i32_f64, _Ruint)(double FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i64_f64, _Rulong)(double FloatValue);
#endif // defined(cl_khr_fp64)
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(half FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(half FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(half FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i64_f16, _Rlong)(half FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i8_f32, _Rchar)(float FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i16_f32, _Rshort)(float FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i32_f32, _Rint)(float FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i64_f32, _Rlong)(float FloatValue);
#if defined(cl_khr_fp64)
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i8_f64, _Rchar)(double FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i16_f64, _Rshort)(double FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i32_f64, _Rint)(double FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i64_f64, _Rlong)(double FloatValue);
#endif // defined(cl_khr_fp64)
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f16_i8, _Rhalf)(char SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f32_i8, _Rfloat)(char SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f16_i16, _Rhalf)(short SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f32_i16, _Rfloat)(short SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f16_i32, _Rhalf)(int SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f32_i32, _Rfloat)(int SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f16_i64, _Rhalf)(long SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f32_i64, _Rfloat)(long SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f16_i8, _Rhalf)(uchar UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f32_i8, _Rfloat)(uchar UnsignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f16_i16, _Rhalf)(ushort UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f32_i16, _Rfloat)(ushort UnsignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f16_i32, _Rhalf)(uint UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f32_i32, _Rfloat)(uint UnsignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f16_i64, _Rhalf)(ulong UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f32_i64, _Rfloat)(ulong UnsignedValue);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f64_i8, _Rdouble)(char SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f64_i16, _Rdouble)(short SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f64_i32, _Rdouble)(int SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f64_i64, _Rdouble)(long SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f64_i8, _Rdouble)(uchar UnsignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f64_i16, _Rdouble)(ushort UnsignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f64_i32, _Rdouble)(uint UnsignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f64_i64, _Rdouble)(ulong UnsignedValue);
#endif // defined(cl_khr_fp64)
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i8_i8, _Ruchar)(uchar UnsignedValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i16_i8, _Rushort)(uchar UnsignedValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i32_i8, _Ruint)(uchar UnsignedValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i64_i8, _Rulong)(uchar UnsignedValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i8_i16, _Ruchar)(ushort UnsignedValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i16_i16, _Rushort)(ushort UnsignedValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i32_i16, _Ruint)(ushort UnsignedValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i64_i16, _Rulong)(ushort UnsignedValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i8_i32, _Ruchar)(uint UnsignedValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i16_i32, _Rushort)(uint UnsignedValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i32_i32, _Ruint)(uint UnsignedValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i64_i32, _Rulong)(uint UnsignedValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i8_i64, _Ruchar)(ulong UnsignedValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i16_i64, _Rushort)(ulong UnsignedValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i32_i64, _Ruint)(ulong UnsignedValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i64_i64, _Rulong)(ulong UnsignedValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i8_i8, _Rchar)(char SignedValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i16_i8, _Rshort)(char SignedValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i32_i8, _Rint)(char SignedValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i64_i8, _Rlong)(char SignedValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i8_i16, _Rchar)(short SignedValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i16_i16, _Rshort)(short SignedValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i32_i16, _Rint)(short SignedValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i64_i16, _Rlong)(short SignedValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i8_i32, _Rchar)(int SignedValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i16_i32, _Rshort)(int SignedValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i32_i32, _Rint)(int SignedValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i64_i32, _Rlong)(int SignedValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i8_i64, _Rchar)(long SignedValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i16_i64, _Rshort)(long SignedValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i32_i64, _Rint)(long SignedValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i64_i64, _Rlong)(long SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f16_f16, _Rhalf)(half FloatValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f32_f16, _Rfloat)(half FloatValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)(float FloatValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f32_f32, _Rfloat)(float FloatValue);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f64_f16, _Rdouble)(half FloatValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f64_f32, _Rdouble)(float FloatValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f16_f64, _Rhalf)(double FloatValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f32_f64, _Rfloat)(double FloatValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f64_f64, _Rdouble)(double FloatValue);
#endif // defined(cl_khr_fp64)
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i8_i8, _Ruchar)(char SignedValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i16_i8, _Rushort)(char SignedValue);
uint  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i32_i8, _Ruint)(char SignedValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i64_i8, _Rulong)(char SignedValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i8_i16, _Ruchar)(short SignedValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i16_i16, _Rushort)(short SignedValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i32_i16, _Ruint)(short SignedValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i64_i16, _Rulong)(short SignedValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i8_i32, _Ruchar)(int SignedValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i16_i32, _Rushort)(int SignedValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i32_i32, _Ruint)(int SignedValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i64_i32, _Rulong)(int SignedValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i8_i64, _Ruchar)(long SignedValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i16_i64, _Rushort)(long SignedValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i32_i64, _Ruint)(long SignedValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i64_i64, _Rulong)(long SignedValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i8_i8, _Rchar)(uchar UnsignedValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i16_i8, _Rshort)(uchar UnsignedValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i32_i8, _Rint)(uchar UnsignedValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i64_i8, _Rlong)(uchar UnsignedValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i8_i16, _Rchar)(ushort UnsignedValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i16_i16, _Rshort)(ushort UnsignedValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i32_i16, _Rint)(ushort UnsignedValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i64_i16, _Rlong)(ushort UnsignedValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i8_i32, _Rchar)(uint UnsignedValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i16_i32, _Rshort)(uint UnsignedValue);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i32_i32, _Rint)(uint UnsignedValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i64_i32, _Rlong)(uint UnsignedValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i8_i64, _Rchar)(ulong UnsignedValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i16_i64, _Rshort)(ulong UnsignedValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i32_i64, _Rint)(ulong UnsignedValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i64_i64, _Rlong)(ulong UnsignedValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i8_f16, _Ruchar_rte)(half FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i8_f16, _Ruchar_rtz)(half FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i8_f16, _Ruchar_rtp)(half FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i8_f16, _Ruchar_rtn)(half FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f16, _Ruchar_sat)(half FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i8_f16, _Ruchar_sat_rte)(half FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i8_f16, _Ruchar_sat_rtz)(half FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i8_f16, _Ruchar_sat_rtp)(half FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i8_f16, _Ruchar_sat_rtn)(half FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i16_f16, _Rushort_rte)(half FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i16_f16, _Rushort_rtz)(half FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i16_f16, _Rushort_rtp)(half FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i16_f16, _Rushort_rtn)(half FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f16, _Rushort_sat)(half FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i16_f16, _Rushort_sat_rte)(half FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i16_f16, _Rushort_sat_rtz)(half FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i16_f16, _Rushort_sat_rtp)(half FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i16_f16, _Rushort_sat_rtn)(half FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i32_f16, _Ruint_rte)(half FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i32_f16, _Ruint_rtz)(half FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i32_f16, _Ruint_rtp)(half FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i32_f16, _Ruint_rtn)(half FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f16, _Ruint_sat)(half FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i32_f16, _Ruint_sat_rte)(half FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i32_f16, _Ruint_sat_rtz)(half FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i32_f16, _Ruint_sat_rtp)(half FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i32_f16, _Ruint_sat_rtn)(half FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i64_f16, _Rulong_rte)(half FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i64_f16, _Rulong_rtz)(half FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i64_f16, _Rulong_rtp)(half FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i64_f16, _Rulong_rtn)(half FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f16, _Rulong_sat)(half FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i64_f16, _Rulong_sat_rte)(half FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i64_f16, _Rulong_sat_rtz)(half FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i64_f16, _Rulong_sat_rtp)(half FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i64_f16, _Rulong_sat_rtn)(half FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i8_f32, _Ruchar_rte)(float FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i8_f32, _Ruchar_rtz)(float FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i8_f32, _Ruchar_rtp)(float FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i8_f32, _Ruchar_rtn)(float FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f32, _Ruchar_sat)(float FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i8_f32, _Ruchar_sat_rte)(float FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i8_f32, _Ruchar_sat_rtz)(float FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i8_f32, _Ruchar_sat_rtp)(float FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i8_f32, _Ruchar_sat_rtn)(float FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i16_f32, _Rushort_rte)(float FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i16_f32, _Rushort_rtz)(float FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i16_f32, _Rushort_rtp)(float FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i16_f32, _Rushort_rtn)(float FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f32, _Rushort_sat)(float FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i16_f32, _Rushort_sat_rte)(float FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i16_f32, _Rushort_sat_rtz)(float FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i16_f32, _Rushort_sat_rtp)(float FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i16_f32, _Rushort_sat_rtn)(float FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i32_f32, _Ruint_rte)(float FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i32_f32, _Ruint_rtz)(float FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i32_f32, _Ruint_rtp)(float FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i32_f32, _Ruint_rtn)(float FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f32, _Ruint_sat)(float FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i32_f32, _Ruint_sat_rte)(float FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i32_f32, _Ruint_sat_rtz)(float FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i32_f32, _Ruint_sat_rtp)(float FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i32_f32, _Ruint_sat_rtn)(float FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i64_f32, _Rulong_rte)(float FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i64_f32, _Rulong_rtz)(float FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i64_f32, _Rulong_rtp)(float FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i64_f32, _Rulong_rtn)(float FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f32, _Rulong_sat)(float FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i64_f32, _Rulong_sat_rte)(float FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i64_f32, _Rulong_sat_rtz)(float FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i64_f32, _Rulong_sat_rtp)(float FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i64_f32, _Rulong_sat_rtn)(float FloatValue);
#if defined(cl_khr_fp64)
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i8_f64, _Ruchar_rte)(double FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i8_f64, _Ruchar_rtz)(double FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i8_f64, _Ruchar_rtp)(double FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i8_f64, _Ruchar_rtn)(double FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f64, _Ruchar_sat)(double FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i8_f64, _Ruchar_sat_rte)(double FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i8_f64, _Ruchar_sat_rtz)(double FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i8_f64, _Ruchar_sat_rtp)(double FloatValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i8_f64, _Ruchar_sat_rtn)(double FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i16_f64, _Rushort_rte)(double FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i16_f64, _Rushort_rtz)(double FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i16_f64, _Rushort_rtp)(double FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i16_f64, _Rushort_rtn)(double FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f64, _Rushort_sat)(double FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i16_f64, _Rushort_sat_rte)(double FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i16_f64, _Rushort_sat_rtz)(double FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i16_f64, _Rushort_sat_rtp)(double FloatValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i16_f64, _Rushort_sat_rtn)(double FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i32_f64, _Ruint_rte)(double FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i32_f64, _Ruint_rtz)(double FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i32_f64, _Ruint_rtp)(double FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i32_f64, _Ruint_rtn)(double FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f64, _Ruint_sat)(double FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i32_f64, _Ruint_sat_rte)(double FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i32_f64, _Ruint_sat_rtz)(double FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i32_f64, _Ruint_sat_rtp)(double FloatValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i32_f64, _Ruint_sat_rtn)(double FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i64_f64, _Rulong_rte)(double FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i64_f64, _Rulong_rtz)(double FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i64_f64, _Rulong_rtp)(double FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i64_f64, _Rulong_rtn)(double FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f64, _Rulong_sat)(double FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i64_f64, _Rulong_sat_rte)(double FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i64_f64, _Rulong_sat_rtz)(double FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i64_f64, _Rulong_sat_rtp)(double FloatValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i64_f64, _Rulong_sat_rtn)(double FloatValue);
#endif // defined(cl_khr_fp64)
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i8_f16, _Rchar_rte)(half FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i8_f16, _Rchar_rtz)(half FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i8_f16, _Rchar_rtp)(half FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i8_f16, _Rchar_rtn)(half FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f16, _Rchar_sat)(half FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i8_f16, _Rchar_sat_rte)(half FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i8_f16, _Rchar_sat_rtz)(half FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i8_f16, _Rchar_sat_rtp)(half FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i8_f16, _Rchar_sat_rtn)(half FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i16_f16, _Rshort_rte)(half FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i16_f16, _Rshort_rtz)(half FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i16_f16, _Rshort_rtp)(half FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i16_f16, _Rshort_rtn)(half FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f16, _Rshort_sat)(half FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i16_f16, _Rshort_sat_rte)(half FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i16_f16, _Rshort_sat_rtz)(half FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i16_f16, _Rshort_sat_rtp)(half FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i16_f16, _Rshort_sat_rtn)(half FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i32_f16, _Rint_rte)(half FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i32_f16, _Rint_rtz)(half FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i32_f16, _Rint_rtp)(half FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i32_f16, _Rint_rtn)(half FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f16, _Rint_sat)(half FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i32_f16, _Rint_sat_rte)(half FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i32_f16, _Rint_sat_rtz)(half FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i32_f16, _Rint_sat_rtp)(half FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i32_f16, _Rint_sat_rtn)(half FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i64_f16, _Rlong_rte)(half FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i64_f16, _Rlong_rtz)(half FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i64_f16, _Rlong_rtp)(half FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i64_f16, _Rlong_rtn)(half FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f16, _Rlong_sat)(half FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i64_f16, _Rlong_sat_rte)(half FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i64_f16, _Rlong_sat_rtz)(half FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i64_f16, _Rlong_sat_rtp)(half FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i64_f16, _Rlong_sat_rtn)(half FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i8_f32, _Rchar_rte)(float FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i8_f32, _Rchar_rtz)(float FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i8_f32, _Rchar_rtp)(float FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i8_f32, _Rchar_rtn)(float FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f32, _Rchar_sat)(float FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i8_f32, _Rchar_sat_rte)(float FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i8_f32, _Rchar_sat_rtz)(float FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i8_f32, _Rchar_sat_rtp)(float FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i8_f32, _Rchar_sat_rtn)(float FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i16_f32, _Rshort_rte)(float FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i16_f32, _Rshort_rtz)(float FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i16_f32, _Rshort_rtp)(float FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i16_f32, _Rshort_rtn)(float FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f32, _Rshort_sat)(float FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i16_f32, _Rshort_sat_rte)(float FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i16_f32, _Rshort_sat_rtz)(float FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i16_f32, _Rshort_sat_rtp)(float FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i16_f32, _Rshort_sat_rtn)(float FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i32_f32, _Rint_rte)(float FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i32_f32, _Rint_rtz)(float FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i32_f32, _Rint_rtp)(float FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i32_f32, _Rint_rtn)(float FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f32, _Rint_sat)(float FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i32_f32, _Rint_sat_rte)(float FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i32_f32, _Rint_sat_rtz)(float FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i32_f32, _Rint_sat_rtp)(float FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i32_f32, _Rint_sat_rtn)(float FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i64_f32, _Rlong_rte)(float FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i64_f32, _Rlong_rtz)(float FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i64_f32, _Rlong_rtp)(float FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i64_f32, _Rlong_rtn)(float FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f32, _Rlong_sat)(float FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i64_f32, _Rlong_sat_rte)(float FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i64_f32, _Rlong_sat_rtz)(float FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i64_f32, _Rlong_sat_rtp)(float FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i64_f32, _Rlong_sat_rtn)(float FloatValue);
#if defined(cl_khr_fp64)
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i8_f64, _Rchar_rte)(double FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i8_f64, _Rchar_rtz)(double FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i8_f64, _Rchar_rtp)(double FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i8_f64, _Rchar_rtn)(double FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f64, _Rchar_sat)(double FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i8_f64, _Rchar_sat_rte)(double FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i8_f64, _Rchar_sat_rtz)(double FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i8_f64, _Rchar_sat_rtp)(double FloatValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i8_f64, _Rchar_sat_rtn)(double FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i16_f64, _Rshort_rte)(double FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i16_f64, _Rshort_rtz)(double FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i16_f64, _Rshort_rtp)(double FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i16_f64, _Rshort_rtn)(double FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f64, _Rshort_sat)(double FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i16_f64, _Rshort_sat_rte)(double FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i16_f64, _Rshort_sat_rtz)(double FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i16_f64, _Rshort_sat_rtp)(double FloatValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i16_f64, _Rshort_sat_rtn)(double FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i32_f64, _Rint_rte)(double FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i32_f64, _Rint_rtz)(double FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i32_f64, _Rint_rtp)(double FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i32_f64, _Rint_rtn)(double FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f64, _Rint_sat)(double FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i32_f64, _Rint_sat_rte)(double FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i32_f64, _Rint_sat_rtz)(double FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i32_f64, _Rint_sat_rtp)(double FloatValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i32_f64, _Rint_sat_rtn)(double FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i64_f64, _Rlong_rte)(double FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i64_f64, _Rlong_rtz)(double FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i64_f64, _Rlong_rtp)(double FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i64_f64, _Rlong_rtn)(double FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f64, _Rlong_sat)(double FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i64_f64, _Rlong_sat_rte)(double FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i64_f64, _Rlong_sat_rtz)(double FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i64_f64, _Rlong_sat_rtp)(double FloatValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i64_f64, _Rlong_sat_rtn)(double FloatValue);
#endif // defined(cl_khr_fp64)
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f16_i8, _Rhalf_rte)(char SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f16_i8, _Rhalf_rtz)(char SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f16_i8, _Rhalf_rtp)(char SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f16_i8, _Rhalf_rtn)(char SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f32_i8, _Rfloat_rte)(char SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f32_i8, _Rfloat_rtz)(char SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f32_i8, _Rfloat_rtp)(char SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f32_i8, _Rfloat_rtn)(char SignedValue);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f64_i8, _Rdouble_rte)(char SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f64_i8, _Rdouble_rtz)(char SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f64_i8, _Rdouble_rtp)(char SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f64_i8, _Rdouble_rtn)(char SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f64_i16, _Rdouble_rte)(short SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f64_i16, _Rdouble_rtz)(short SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f64_i16, _Rdouble_rtp)(short SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f64_i16, _Rdouble_rtn)(short SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f64_i32, _Rdouble_rte)(int SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f64_i32, _Rdouble_rtz)(int SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f64_i32, _Rdouble_rtp)(int SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f64_i32, _Rdouble_rtn)(int SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f64_i64, _Rdouble_rte)(long SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f64_i64, _Rdouble_rtz)(long SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f64_i64, _Rdouble_rtp)(long SignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f64_i64, _Rdouble_rtn)(long SignedValue);
#endif // defined(cl_khr_fp64)
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f16_i16, _Rhalf_rte)(short SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f16_i16, _Rhalf_rtz)(short SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f16_i16, _Rhalf_rtp)(short SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f16_i16, _Rhalf_rtn)(short SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f32_i16, _Rfloat_rte)(short SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f32_i16, _Rfloat_rtz)(short SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f32_i16, _Rfloat_rtp)(short SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f32_i16, _Rfloat_rtn)(short SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f16_i32, _Rhalf_rte)(int SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f16_i32, _Rhalf_rtz)(int SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f16_i32, _Rhalf_rtp)(int SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f16_i32, _Rhalf_rtn)(int SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f32_i32, _Rfloat_rte)(int SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f32_i32, _Rfloat_rtz)(int SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f32_i32, _Rfloat_rtp)(int SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f32_i32, _Rfloat_rtn)(int SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f16_i64, _Rhalf_rte)(long SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f16_i64, _Rhalf_rtz)(long SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f16_i64, _Rhalf_rtp)(long SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f16_i64, _Rhalf_rtn)(long SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f32_i64, _Rfloat_rte)(long SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f32_i64, _Rfloat_rtz)(long SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f32_i64, _Rfloat_rtp)(long SignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f32_i64, _Rfloat_rtn)(long SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f16_i8, _Rhalf_rte)(uchar UnsignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f16_i8, _Rhalf_rtz)(uchar UnsignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f16_i8, _Rhalf_rtp)(uchar UnsignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f16_i8, _Rhalf_rtn)(uchar UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f32_i8, _Rfloat_rte)(uchar UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i8, _Rfloat_rtz)(uchar UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i8, _Rfloat_rtp)(uchar UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f32_i8, _Rfloat_rtn)(uchar UnsignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f16_i16, _Rhalf_rte)(ushort UnsignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f16_i16, _Rhalf_rtz)(ushort UnsignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f16_i16, _Rhalf_rtp)(ushort UnsignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f16_i16, _Rhalf_rtn)(ushort UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f32_i16, _Rfloat_rte)(ushort UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i16, _Rfloat_rtz)(ushort UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i16, _Rfloat_rtp)(ushort UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f32_i16, _Rfloat_rtn)(ushort UnsignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f16_i32, _Rhalf_rte)(uint UnsignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f16_i32, _Rhalf_rtz)(uint UnsignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f16_i32, _Rhalf_rtp)(uint UnsignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f16_i32, _Rhalf_rtn)(uint UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f32_i32, _Rfloat_rte)(uint UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i32, _Rfloat_rtz)(uint UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i32, _Rfloat_rtp)(uint UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f32_i32, _Rfloat_rtn)(uint UnsignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f16_i64, _Rhalf_rte)(ulong UnsignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f16_i64, _Rhalf_rtz)(ulong UnsignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f16_i64, _Rhalf_rtp)(ulong UnsignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f16_i64, _Rhalf_rtn)(ulong UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f32_i64, _Rfloat_rte)(ulong UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i64, _Rfloat_rtz)(ulong UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i64, _Rfloat_rtp)(ulong UnsignedValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f32_i64, _Rfloat_rtn)(ulong UnsignedValue);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f64_i8, _Rdouble_rte)(uchar UnsignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f64_i8, _Rdouble_rtz)(uchar UnsignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f64_i8, _Rdouble_rtp)(uchar UnsignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f64_i8, _Rdouble_rtn)(uchar UnsignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f64_i16, _Rdouble_rte)(ushort UnsignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f64_i16, _Rdouble_rtz)(ushort UnsignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f64_i16, _Rdouble_rtp)(ushort UnsignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f64_i16, _Rdouble_rtn)(ushort UnsignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f64_i32, _Rdouble_rte)(uint UnsignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f64_i32, _Rdouble_rtz)(uint UnsignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f64_i32, _Rdouble_rtp)(uint UnsignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f64_i32, _Rdouble_rtn)(uint UnsignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f64_i64, _Rdouble_rte)(ulong UnsignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f64_i64, _Rdouble_rtz)(ulong UnsignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f64_i64, _Rdouble_rtp)(ulong UnsignedValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f64_i64, _Rdouble_rtn)(ulong UnsignedValue);
#endif // defined(cl_khr_fp64)
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i8_i8, _Ruchar_sat)(uchar UnsignedValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i16_i8, _Rushort_sat)(uchar UnsignedValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i32_i8, _Ruint_sat)(uchar UnsignedValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i64_i8, _Rulong_sat)(uchar UnsignedValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i8_i16, _Ruchar_sat)(ushort UnsignedValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i16_i16, _Rushort_sat)(ushort UnsignedValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i32_i16, _Ruint_sat)(ushort UnsignedValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i64_i16, _Rulong_sat)(ushort UnsignedValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i8_i32, _Ruchar_sat)(uint UnsignedValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i16_i32, _Rushort_sat)(uint UnsignedValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i32_i32, _Ruint_sat)(uint UnsignedValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i64_i32, _Rulong_sat)(uint UnsignedValue);
uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i8_i64, _Ruchar_sat)(ulong UnsignedValue);
ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i16_i64, _Rushort_sat)(ulong UnsignedValue);
uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i32_i64, _Ruint_sat)(ulong UnsignedValue);
ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i64_i64, _Rulong_sat)(ulong UnsignedValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i8_i8, _Rchar_sat)(char SignedValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i16_i8, _Rshort_sat)(char SignedValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i32_i8, _Rint_sat)(char SignedValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i64_i8, _Rlong_sat)(char SignedValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i8_i16, _Rchar_sat)(short SignedValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i16_i16, _Rshort_sat)(short SignedValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i32_i16, _Rint_sat)(short SignedValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i64_i16, _Rlong_sat)(short SignedValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i8_i32, _Rchar_sat)(int SignedValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i16_i32, _Rshort_sat)(int SignedValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i32_i32, _Rint_sat)(int SignedValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i64_i32, _Rlong_sat)(int SignedValue);
char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i8_i64, _Rchar_sat)(long SignedValue);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i16_i64, _Rshort_sat)(long SignedValue);
int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i32_i64, _Rint_sat)(long SignedValue);
long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i64_i64, _Rlong_sat)(long SignedValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f16_f16, _Rhalf_rte)(half FloatValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f16_f16, _Rhalf_rtz)(half FloatValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f16_f16, _Rhalf_rtp)(half FloatValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f16_f16, _Rhalf_rtn)(half FloatValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f32_f16, _Rfloat_rte)(half FloatValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f32_f16, _Rfloat_rtz)(half FloatValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f32_f16, _Rfloat_rtp)(half FloatValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f32_f16, _Rfloat_rtn)(half FloatValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f16_f32, _Rhalf_rte)(float FloatValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _Rhalf_rtz)(float FloatValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _Rhalf_rtp)(float FloatValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _Rhalf_rtn)(float FloatValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f32_f32, _Rfloat_rte)(float FloatValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f32_f32, _Rfloat_rtz)(float FloatValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f32_f32, _Rfloat_rtp)(float FloatValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f32_f32, _Rfloat_rtn)(float FloatValue);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f64_f16, _Rdouble_rte)(half FloatValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f64_f16, _Rdouble_rtz)(half FloatValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f64_f16, _Rdouble_rtp)(half FloatValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f64_f16, _Rdouble_rtn)(half FloatValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f64_f32, _Rdouble_rte)(float FloatValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f64_f32, _Rdouble_rtz)(float FloatValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f64_f32, _Rdouble_rtp)(float FloatValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f64_f32, _Rdouble_rtn)(float FloatValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f16_f64, _Rhalf_rte)(double FloatValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f16_f64, _Rhalf_rtz)(double FloatValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f16_f64, _Rhalf_rtp)(double FloatValue);
half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f16_f64, _Rhalf_rtn)(double FloatValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f32_f64, _Rfloat_rte)(double FloatValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f32_f64, _Rfloat_rtz)(double FloatValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f32_f64, _Rfloat_rtp)(double FloatValue);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f32_f64, _Rfloat_rtn)(double FloatValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f64_f64, _Rdouble_rte)(double FloatValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f64_f64, _Rdouble_rtz)(double FloatValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f64_f64, _Rdouble_rtp)(double FloatValue);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f64_f64, _Rdouble_rtn)(double FloatValue);
#endif // defined(cl_khr_fp64)
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v2f16_v2i8, _Rhalf2)(char2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v3f16_v3i8, _Rhalf3)(char3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v4f16_v4i8, _Rhalf4)(char4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v8f16_v8i8, _Rhalf8)(char8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f16_v16i8, _Rhalf16)(char16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v2f16_v2i8, _Rhalf2_rte)(char2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v3f16_v3i8, _Rhalf3_rte)(char3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v4f16_v4i8, _Rhalf4_rte)(char4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v8f16_v8i8, _Rhalf8_rte)(char8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v16f16_v16i8, _Rhalf16_rte)(char16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f16_v2i8, _Rhalf2_rtz)(char2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f16_v3i8, _Rhalf3_rtz)(char3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f16_v4i8, _Rhalf4_rtz)(char4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f16_v8i8, _Rhalf8_rtz)(char8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f16_v16i8, _Rhalf16_rtz)(char16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v2f16_v2i8, _Rhalf2_rtp)(char2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v3f16_v3i8, _Rhalf3_rtp)(char3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v4f16_v4i8, _Rhalf4_rtp)(char4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v8f16_v8i8, _Rhalf8_rtp)(char8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v16f16_v16i8, _Rhalf16_rtp)(char16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v2f16_v2i8, _Rhalf2_rtn)(char2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v3f16_v3i8, _Rhalf3_rtn)(char3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v4f16_v4i8, _Rhalf4_rtn)(char4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v8f16_v8i8, _Rhalf8_rtn)(char8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v16f16_v16i8, _Rhalf16_rtn)(char16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v2f16_v2i16, _Rhalf2)(short2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v3f16_v3i16, _Rhalf3)(short3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v4f16_v4i16, _Rhalf4)(short4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v8f16_v8i16, _Rhalf8)(short8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f16_v16i16, _Rhalf16)(short16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v2f16_v2i16, _Rhalf2_rte)(short2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v3f16_v3i16, _Rhalf3_rte)(short3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v4f16_v4i16, _Rhalf4_rte)(short4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v8f16_v8i16, _Rhalf8_rte)(short8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v16f16_v16i16, _Rhalf16_rte)(short16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f16_v2i16, _Rhalf2_rtz)(short2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f16_v3i16, _Rhalf3_rtz)(short3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f16_v4i16, _Rhalf4_rtz)(short4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f16_v8i16, _Rhalf8_rtz)(short8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f16_v16i16, _Rhalf16_rtz)(short16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v2f16_v2i16, _Rhalf2_rtp)(short2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v3f16_v3i16, _Rhalf3_rtp)(short3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v4f16_v4i16, _Rhalf4_rtp)(short4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v8f16_v8i16, _Rhalf8_rtp)(short8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v16f16_v16i16, _Rhalf16_rtp)(short16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v2f16_v2i16, _Rhalf2_rtn)(short2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v3f16_v3i16, _Rhalf3_rtn)(short3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v4f16_v4i16, _Rhalf4_rtn)(short4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v8f16_v8i16, _Rhalf8_rtn)(short8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v16f16_v16i16, _Rhalf16_rtn)(short16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v2f16_v2i32, _Rhalf2)(int2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v3f16_v3i32, _Rhalf3)(int3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v4f16_v4i32, _Rhalf4)(int4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v8f16_v8i32, _Rhalf8)(int8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f16_v16i32, _Rhalf16)(int16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v2f16_v2i32, _Rhalf2_rte)(int2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v3f16_v3i32, _Rhalf3_rte)(int3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v4f16_v4i32, _Rhalf4_rte)(int4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v8f16_v8i32, _Rhalf8_rte)(int8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v16f16_v16i32, _Rhalf16_rte)(int16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f16_v2i32, _Rhalf2_rtz)(int2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f16_v3i32, _Rhalf3_rtz)(int3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f16_v4i32, _Rhalf4_rtz)(int4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f16_v8i32, _Rhalf8_rtz)(int8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f16_v16i32, _Rhalf16_rtz)(int16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v2f16_v2i32, _Rhalf2_rtp)(int2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v3f16_v3i32, _Rhalf3_rtp)(int3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v4f16_v4i32, _Rhalf4_rtp)(int4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v8f16_v8i32, _Rhalf8_rtp)(int8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v16f16_v16i32, _Rhalf16_rtp)(int16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v2f16_v2i32, _Rhalf2_rtn)(int2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v3f16_v3i32, _Rhalf3_rtn)(int3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v4f16_v4i32, _Rhalf4_rtn)(int4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v8f16_v8i32, _Rhalf8_rtn)(int8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v16f16_v16i32, _Rhalf16_rtn)(int16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v2f16_v2i64, _Rhalf2)(long2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v3f16_v3i64, _Rhalf3)(long3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v4f16_v4i64, _Rhalf4)(long4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v8f16_v8i64, _Rhalf8)(long8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f16_v16i64, _Rhalf16)(long16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v2f16_v2i64, _Rhalf2_rte)(long2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v3f16_v3i64, _Rhalf3_rte)(long3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v4f16_v4i64, _Rhalf4_rte)(long4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v8f16_v8i64, _Rhalf8_rte)(long8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v16f16_v16i64, _Rhalf16_rte)(long16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f16_v2i64, _Rhalf2_rtz)(long2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f16_v3i64, _Rhalf3_rtz)(long3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f16_v4i64, _Rhalf4_rtz)(long4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f16_v8i64, _Rhalf8_rtz)(long8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f16_v16i64, _Rhalf16_rtz)(long16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v2f16_v2i64, _Rhalf2_rtp)(long2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v3f16_v3i64, _Rhalf3_rtp)(long3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v4f16_v4i64, _Rhalf4_rtp)(long4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v8f16_v8i64, _Rhalf8_rtp)(long8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v16f16_v16i64, _Rhalf16_rtp)(long16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v2f16_v2i64, _Rhalf2_rtn)(long2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v3f16_v3i64, _Rhalf3_rtn)(long3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v4f16_v4i64, _Rhalf4_rtn)(long4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v8f16_v8i64, _Rhalf8_rtn)(long8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v16f16_v16i64, _Rhalf16_rtn)(long16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v2f32_v2i8, _Rfloat2)(char2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v3f32_v3i8, _Rfloat3)(char3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v4f32_v4i8, _Rfloat4)(char4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v8f32_v8i8, _Rfloat8)(char8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f32_v16i8, _Rfloat16)(char16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v2f32_v2i8, _Rfloat2_rte)(char2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v3f32_v3i8, _Rfloat3_rte)(char3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v4f32_v4i8, _Rfloat4_rte)(char4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v8f32_v8i8, _Rfloat8_rte)(char8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v16f32_v16i8, _Rfloat16_rte)(char16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f32_v2i8, _Rfloat2_rtz)(char2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f32_v3i8, _Rfloat3_rtz)(char3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f32_v4i8, _Rfloat4_rtz)(char4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f32_v8i8, _Rfloat8_rtz)(char8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f32_v16i8, _Rfloat16_rtz)(char16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v2f32_v2i8, _Rfloat2_rtp)(char2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v3f32_v3i8, _Rfloat3_rtp)(char3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v4f32_v4i8, _Rfloat4_rtp)(char4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v8f32_v8i8, _Rfloat8_rtp)(char8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v16f32_v16i8, _Rfloat16_rtp)(char16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v2f32_v2i8, _Rfloat2_rtn)(char2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v3f32_v3i8, _Rfloat3_rtn)(char3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v4f32_v4i8, _Rfloat4_rtn)(char4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v8f32_v8i8, _Rfloat8_rtn)(char8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v16f32_v16i8, _Rfloat16_rtn)(char16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v2f32_v2i16, _Rfloat2)(short2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v3f32_v3i16, _Rfloat3)(short3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v4f32_v4i16, _Rfloat4)(short4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v8f32_v8i16, _Rfloat8)(short8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f32_v16i16, _Rfloat16)(short16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v2f32_v2i16, _Rfloat2_rte)(short2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v3f32_v3i16, _Rfloat3_rte)(short3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v4f32_v4i16, _Rfloat4_rte)(short4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v8f32_v8i16, _Rfloat8_rte)(short8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v16f32_v16i16, _Rfloat16_rte)(short16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f32_v2i16, _Rfloat2_rtz)(short2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f32_v3i16, _Rfloat3_rtz)(short3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f32_v4i16, _Rfloat4_rtz)(short4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f32_v8i16, _Rfloat8_rtz)(short8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f32_v16i16, _Rfloat16_rtz)(short16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v2f32_v2i16, _Rfloat2_rtp)(short2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v3f32_v3i16, _Rfloat3_rtp)(short3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v4f32_v4i16, _Rfloat4_rtp)(short4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v8f32_v8i16, _Rfloat8_rtp)(short8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v16f32_v16i16, _Rfloat16_rtp)(short16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v2f32_v2i16, _Rfloat2_rtn)(short2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v3f32_v3i16, _Rfloat3_rtn)(short3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v4f32_v4i16, _Rfloat4_rtn)(short4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v8f32_v8i16, _Rfloat8_rtn)(short8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v16f32_v16i16, _Rfloat16_rtn)(short16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v2f32_v2i32, _Rfloat2)(int2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v3f32_v3i32, _Rfloat3)(int3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v4f32_v4i32, _Rfloat4)(int4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v8f32_v8i32, _Rfloat8)(int8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f32_v16i32, _Rfloat16)(int16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v2f32_v2i32, _Rfloat2_rte)(int2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v3f32_v3i32, _Rfloat3_rte)(int3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v4f32_v4i32, _Rfloat4_rte)(int4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v8f32_v8i32, _Rfloat8_rte)(int8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v16f32_v16i32, _Rfloat16_rte)(int16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f32_v2i32, _Rfloat2_rtz)(int2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f32_v3i32, _Rfloat3_rtz)(int3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f32_v4i32, _Rfloat4_rtz)(int4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f32_v8i32, _Rfloat8_rtz)(int8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f32_v16i32, _Rfloat16_rtz)(int16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v2f32_v2i32, _Rfloat2_rtp)(int2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v3f32_v3i32, _Rfloat3_rtp)(int3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v4f32_v4i32, _Rfloat4_rtp)(int4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v8f32_v8i32, _Rfloat8_rtp)(int8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v16f32_v16i32, _Rfloat16_rtp)(int16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v2f32_v2i32, _Rfloat2_rtn)(int2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v3f32_v3i32, _Rfloat3_rtn)(int3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v4f32_v4i32, _Rfloat4_rtn)(int4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v8f32_v8i32, _Rfloat8_rtn)(int8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v16f32_v16i32, _Rfloat16_rtn)(int16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v2f32_v2i64, _Rfloat2)(long2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v3f32_v3i64, _Rfloat3)(long3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v4f32_v4i64, _Rfloat4)(long4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v8f32_v8i64, _Rfloat8)(long8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f32_v16i64, _Rfloat16)(long16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v2f32_v2i64, _Rfloat2_rte)(long2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v3f32_v3i64, _Rfloat3_rte)(long3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v4f32_v4i64, _Rfloat4_rte)(long4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v8f32_v8i64, _Rfloat8_rte)(long8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v16f32_v16i64, _Rfloat16_rte)(long16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f32_v2i64, _Rfloat2_rtz)(long2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f32_v3i64, _Rfloat3_rtz)(long3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f32_v4i64, _Rfloat4_rtz)(long4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f32_v8i64, _Rfloat8_rtz)(long8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f32_v16i64, _Rfloat16_rtz)(long16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v2f32_v2i64, _Rfloat2_rtp)(long2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v3f32_v3i64, _Rfloat3_rtp)(long3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v4f32_v4i64, _Rfloat4_rtp)(long4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v8f32_v8i64, _Rfloat8_rtp)(long8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v16f32_v16i64, _Rfloat16_rtp)(long16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v2f32_v2i64, _Rfloat2_rtn)(long2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v3f32_v3i64, _Rfloat3_rtn)(long3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v4f32_v4i64, _Rfloat4_rtn)(long4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v8f32_v8i64, _Rfloat8_rtn)(long8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v16f32_v16i64, _Rfloat16_rtn)(long16 x);
#if defined(cl_khr_fp64)
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v2f64_v2i8, _Rdouble2)(char2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v3f64_v3i8, _Rdouble3)(char3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v4f64_v4i8, _Rdouble4)(char4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v8f64_v8i8, _Rdouble8)(char8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f64_v16i8, _Rdouble16)(char16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v2f64_v2i8, _Rdouble2_rte)(char2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v3f64_v3i8, _Rdouble3_rte)(char3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v4f64_v4i8, _Rdouble4_rte)(char4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v8f64_v8i8, _Rdouble8_rte)(char8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v16f64_v16i8, _Rdouble16_rte)(char16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f64_v2i8, _Rdouble2_rtz)(char2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f64_v3i8, _Rdouble3_rtz)(char3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f64_v4i8, _Rdouble4_rtz)(char4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f64_v8i8, _Rdouble8_rtz)(char8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f64_v16i8, _Rdouble16_rtz)(char16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v2f64_v2i8, _Rdouble2_rtp)(char2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v3f64_v3i8, _Rdouble3_rtp)(char3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v4f64_v4i8, _Rdouble4_rtp)(char4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v8f64_v8i8, _Rdouble8_rtp)(char8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v16f64_v16i8, _Rdouble16_rtp)(char16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v2f64_v2i8, _Rdouble2_rtn)(char2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v3f64_v3i8, _Rdouble3_rtn)(char3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v4f64_v4i8, _Rdouble4_rtn)(char4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v8f64_v8i8, _Rdouble8_rtn)(char8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v16f64_v16i8, _Rdouble16_rtn)(char16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v2f64_v2i16, _Rdouble2)(short2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v3f64_v3i16, _Rdouble3)(short3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v4f64_v4i16, _Rdouble4)(short4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v8f64_v8i16, _Rdouble8)(short8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f64_v16i16, _Rdouble16)(short16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v2f64_v2i16, _Rdouble2_rte)(short2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v3f64_v3i16, _Rdouble3_rte)(short3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v4f64_v4i16, _Rdouble4_rte)(short4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v8f64_v8i16, _Rdouble8_rte)(short8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v16f64_v16i16, _Rdouble16_rte)(short16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f64_v2i16, _Rdouble2_rtz)(short2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f64_v3i16, _Rdouble3_rtz)(short3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f64_v4i16, _Rdouble4_rtz)(short4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f64_v8i16, _Rdouble8_rtz)(short8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f64_v16i16, _Rdouble16_rtz)(short16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v2f64_v2i16, _Rdouble2_rtp)(short2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v3f64_v3i16, _Rdouble3_rtp)(short3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v4f64_v4i16, _Rdouble4_rtp)(short4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v8f64_v8i16, _Rdouble8_rtp)(short8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v16f64_v16i16, _Rdouble16_rtp)(short16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v2f64_v2i16, _Rdouble2_rtn)(short2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v3f64_v3i16, _Rdouble3_rtn)(short3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v4f64_v4i16, _Rdouble4_rtn)(short4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v8f64_v8i16, _Rdouble8_rtn)(short8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v16f64_v16i16, _Rdouble16_rtn)(short16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v2f64_v2i32, _Rdouble2)(int2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v3f64_v3i32, _Rdouble3)(int3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v4f64_v4i32, _Rdouble4)(int4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v8f64_v8i32, _Rdouble8)(int8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f64_v16i32, _Rdouble16)(int16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v2f64_v2i32, _Rdouble2_rte)(int2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v3f64_v3i32, _Rdouble3_rte)(int3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v4f64_v4i32, _Rdouble4_rte)(int4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v8f64_v8i32, _Rdouble8_rte)(int8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v16f64_v16i32, _Rdouble16_rte)(int16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f64_v2i32, _Rdouble2_rtz)(int2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f64_v3i32, _Rdouble3_rtz)(int3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f64_v4i32, _Rdouble4_rtz)(int4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f64_v8i32, _Rdouble8_rtz)(int8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f64_v16i32, _Rdouble16_rtz)(int16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v2f64_v2i32, _Rdouble2_rtp)(int2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v3f64_v3i32, _Rdouble3_rtp)(int3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v4f64_v4i32, _Rdouble4_rtp)(int4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v8f64_v8i32, _Rdouble8_rtp)(int8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v16f64_v16i32, _Rdouble16_rtp)(int16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v2f64_v2i32, _Rdouble2_rtn)(int2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v3f64_v3i32, _Rdouble3_rtn)(int3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v4f64_v4i32, _Rdouble4_rtn)(int4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v8f64_v8i32, _Rdouble8_rtn)(int8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v16f64_v16i32, _Rdouble16_rtn)(int16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v2f64_v2i64, _Rdouble2)(long2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v3f64_v3i64, _Rdouble3)(long3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v4f64_v4i64, _Rdouble4)(long4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v8f64_v8i64, _Rdouble8)(long8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _v16f64_v16i64, _Rdouble16)(long16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v2f64_v2i64, _Rdouble2_rte)(long2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v3f64_v3i64, _Rdouble3_rte)(long3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v4f64_v4i64, _Rdouble4_rte)(long4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v8f64_v8i64, _Rdouble8_rte)(long8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_v16f64_v16i64, _Rdouble16_rte)(long16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v2f64_v2i64, _Rdouble2_rtz)(long2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v3f64_v3i64, _Rdouble3_rtz)(long3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v4f64_v4i64, _Rdouble4_rtz)(long4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v8f64_v8i64, _Rdouble8_rtz)(long8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_v16f64_v16i64, _Rdouble16_rtz)(long16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v2f64_v2i64, _Rdouble2_rtp)(long2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v3f64_v3i64, _Rdouble3_rtp)(long3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v4f64_v4i64, _Rdouble4_rtp)(long4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v8f64_v8i64, _Rdouble8_rtp)(long8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_v16f64_v16i64, _Rdouble16_rtp)(long16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v2f64_v2i64, _Rdouble2_rtn)(long2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v3f64_v3i64, _Rdouble3_rtn)(long3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v4f64_v4i64, _Rdouble4_rtn)(long4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v8f64_v8i64, _Rdouble8_rtn)(long8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_v16f64_v16i64, _Rdouble16_rtn)(long16 x);
#endif // defined(cl_khr_fp64)
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f16_v2i8, _Rhalf2)(uchar2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f16_v3i8, _Rhalf3)(uchar3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f16_v4i8, _Rhalf4)(uchar4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f16_v8i8, _Rhalf8)(uchar8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f16_v16i8, _Rhalf16)(uchar16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v2f16_v2i8, _Rhalf2_rte)(uchar2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v3f16_v3i8, _Rhalf3_rte)(uchar3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v4f16_v4i8, _Rhalf4_rte)(uchar4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v8f16_v8i8, _Rhalf8_rte)(uchar8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v16f16_v16i8, _Rhalf16_rte)(uchar16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f16_v2i8, _Rhalf2_rtz)(uchar2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f16_v3i8, _Rhalf3_rtz)(uchar3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f16_v4i8, _Rhalf4_rtz)(uchar4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f16_v8i8, _Rhalf8_rtz)(uchar8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f16_v16i8, _Rhalf16_rtz)(uchar16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v2f16_v2i8, _Rhalf2_rtp)(uchar2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v3f16_v3i8, _Rhalf3_rtp)(uchar3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v4f16_v4i8, _Rhalf4_rtp)(uchar4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v8f16_v8i8, _Rhalf8_rtp)(uchar8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v16f16_v16i8, _Rhalf16_rtp)(uchar16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v2f16_v2i8, _Rhalf2_rtn)(uchar2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v3f16_v3i8, _Rhalf3_rtn)(uchar3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v4f16_v4i8, _Rhalf4_rtn)(uchar4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v8f16_v8i8, _Rhalf8_rtn)(uchar8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v16f16_v16i8, _Rhalf16_rtn)(uchar16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f16_v2i16, _Rhalf2)(ushort2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f16_v3i16, _Rhalf3)(ushort3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f16_v4i16, _Rhalf4)(ushort4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f16_v8i16, _Rhalf8)(ushort8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f16_v16i16, _Rhalf16)(ushort16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v2f16_v2i16, _Rhalf2_rte)(ushort2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v3f16_v3i16, _Rhalf3_rte)(ushort3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v4f16_v4i16, _Rhalf4_rte)(ushort4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v8f16_v8i16, _Rhalf8_rte)(ushort8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v16f16_v16i16, _Rhalf16_rte)(ushort16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f16_v2i16, _Rhalf2_rtz)(ushort2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f16_v3i16, _Rhalf3_rtz)(ushort3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f16_v4i16, _Rhalf4_rtz)(ushort4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f16_v8i16, _Rhalf8_rtz)(ushort8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f16_v16i16, _Rhalf16_rtz)(ushort16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v2f16_v2i16, _Rhalf2_rtp)(ushort2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v3f16_v3i16, _Rhalf3_rtp)(ushort3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v4f16_v4i16, _Rhalf4_rtp)(ushort4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v8f16_v8i16, _Rhalf8_rtp)(ushort8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v16f16_v16i16, _Rhalf16_rtp)(ushort16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v2f16_v2i16, _Rhalf2_rtn)(ushort2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v3f16_v3i16, _Rhalf3_rtn)(ushort3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v4f16_v4i16, _Rhalf4_rtn)(ushort4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v8f16_v8i16, _Rhalf8_rtn)(ushort8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v16f16_v16i16, _Rhalf16_rtn)(ushort16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f16_v2i32, _Rhalf2)(uint2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f16_v3i32, _Rhalf3)(uint3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f16_v4i32, _Rhalf4)(uint4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f16_v8i32, _Rhalf8)(uint8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f16_v16i32, _Rhalf16)(uint16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v2f16_v2i32, _Rhalf2_rte)(uint2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v3f16_v3i32, _Rhalf3_rte)(uint3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v4f16_v4i32, _Rhalf4_rte)(uint4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v8f16_v8i32, _Rhalf8_rte)(uint8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v16f16_v16i32, _Rhalf16_rte)(uint16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f16_v2i32, _Rhalf2_rtz)(uint2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f16_v3i32, _Rhalf3_rtz)(uint3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f16_v4i32, _Rhalf4_rtz)(uint4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f16_v8i32, _Rhalf8_rtz)(uint8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f16_v16i32, _Rhalf16_rtz)(uint16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v2f16_v2i32, _Rhalf2_rtp)(uint2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v3f16_v3i32, _Rhalf3_rtp)(uint3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v4f16_v4i32, _Rhalf4_rtp)(uint4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v8f16_v8i32, _Rhalf8_rtp)(uint8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v16f16_v16i32, _Rhalf16_rtp)(uint16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v2f16_v2i32, _Rhalf2_rtn)(uint2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v3f16_v3i32, _Rhalf3_rtn)(uint3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v4f16_v4i32, _Rhalf4_rtn)(uint4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v8f16_v8i32, _Rhalf8_rtn)(uint8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v16f16_v16i32, _Rhalf16_rtn)(uint16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f16_v2i64, _Rhalf2)(ulong2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f16_v3i64, _Rhalf3)(ulong3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f16_v4i64, _Rhalf4)(ulong4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f16_v8i64, _Rhalf8)(ulong8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f16_v16i64, _Rhalf16)(ulong16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v2f16_v2i64, _Rhalf2_rte)(ulong2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v3f16_v3i64, _Rhalf3_rte)(ulong3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v4f16_v4i64, _Rhalf4_rte)(ulong4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v8f16_v8i64, _Rhalf8_rte)(ulong8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v16f16_v16i64, _Rhalf16_rte)(ulong16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f16_v2i64, _Rhalf2_rtz)(ulong2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f16_v3i64, _Rhalf3_rtz)(ulong3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f16_v4i64, _Rhalf4_rtz)(ulong4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f16_v8i64, _Rhalf8_rtz)(ulong8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f16_v16i64, _Rhalf16_rtz)(ulong16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v2f16_v2i64, _Rhalf2_rtp)(ulong2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v3f16_v3i64, _Rhalf3_rtp)(ulong3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v4f16_v4i64, _Rhalf4_rtp)(ulong4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v8f16_v8i64, _Rhalf8_rtp)(ulong8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v16f16_v16i64, _Rhalf16_rtp)(ulong16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v2f16_v2i64, _Rhalf2_rtn)(ulong2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v3f16_v3i64, _Rhalf3_rtn)(ulong3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v4f16_v4i64, _Rhalf4_rtn)(ulong4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v8f16_v8i64, _Rhalf8_rtn)(ulong8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v16f16_v16i64, _Rhalf16_rtn)(ulong16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f32_v2i8, _Rfloat2)(uchar2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f32_v3i8, _Rfloat3)(uchar3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f32_v4i8, _Rfloat4)(uchar4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f32_v8i8, _Rfloat8)(uchar8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f32_v16i8, _Rfloat16)(uchar16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v2f32_v2i8, _Rfloat2_rte)(uchar2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v3f32_v3i8, _Rfloat3_rte)(uchar3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v4f32_v4i8, _Rfloat4_rte)(uchar4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v8f32_v8i8, _Rfloat8_rte)(uchar8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v16f32_v16i8, _Rfloat16_rte)(uchar16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f32_v2i8, _Rfloat2_rtz)(uchar2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f32_v3i8, _Rfloat3_rtz)(uchar3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f32_v4i8, _Rfloat4_rtz)(uchar4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f32_v8i8, _Rfloat8_rtz)(uchar8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f32_v16i8, _Rfloat16_rtz)(uchar16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v2f32_v2i8, _Rfloat2_rtp)(uchar2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v3f32_v3i8, _Rfloat3_rtp)(uchar3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v4f32_v4i8, _Rfloat4_rtp)(uchar4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v8f32_v8i8, _Rfloat8_rtp)(uchar8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v16f32_v16i8, _Rfloat16_rtp)(uchar16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v2f32_v2i8, _Rfloat2_rtn)(uchar2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v3f32_v3i8, _Rfloat3_rtn)(uchar3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v4f32_v4i8, _Rfloat4_rtn)(uchar4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v8f32_v8i8, _Rfloat8_rtn)(uchar8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v16f32_v16i8, _Rfloat16_rtn)(uchar16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f32_v2i16, _Rfloat2)(ushort2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f32_v3i16, _Rfloat3)(ushort3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f32_v4i16, _Rfloat4)(ushort4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f32_v8i16, _Rfloat8)(ushort8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f32_v16i16, _Rfloat16)(ushort16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v2f32_v2i16, _Rfloat2_rte)(ushort2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v3f32_v3i16, _Rfloat3_rte)(ushort3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v4f32_v4i16, _Rfloat4_rte)(ushort4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v8f32_v8i16, _Rfloat8_rte)(ushort8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v16f32_v16i16, _Rfloat16_rte)(ushort16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f32_v2i16, _Rfloat2_rtz)(ushort2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f32_v3i16, _Rfloat3_rtz)(ushort3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f32_v4i16, _Rfloat4_rtz)(ushort4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f32_v8i16, _Rfloat8_rtz)(ushort8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f32_v16i16, _Rfloat16_rtz)(ushort16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v2f32_v2i16, _Rfloat2_rtp)(ushort2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v3f32_v3i16, _Rfloat3_rtp)(ushort3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v4f32_v4i16, _Rfloat4_rtp)(ushort4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v8f32_v8i16, _Rfloat8_rtp)(ushort8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v16f32_v16i16, _Rfloat16_rtp)(ushort16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v2f32_v2i16, _Rfloat2_rtn)(ushort2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v3f32_v3i16, _Rfloat3_rtn)(ushort3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v4f32_v4i16, _Rfloat4_rtn)(ushort4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v8f32_v8i16, _Rfloat8_rtn)(ushort8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v16f32_v16i16, _Rfloat16_rtn)(ushort16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f32_v2i32, _Rfloat2)(uint2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f32_v3i32, _Rfloat3)(uint3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f32_v4i32, _Rfloat4)(uint4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f32_v8i32, _Rfloat8)(uint8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f32_v16i32, _Rfloat16)(uint16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v2f32_v2i32, _Rfloat2_rte)(uint2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v3f32_v3i32, _Rfloat3_rte)(uint3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v4f32_v4i32, _Rfloat4_rte)(uint4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v8f32_v8i32, _Rfloat8_rte)(uint8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v16f32_v16i32, _Rfloat16_rte)(uint16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f32_v2i32, _Rfloat2_rtz)(uint2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f32_v3i32, _Rfloat3_rtz)(uint3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f32_v4i32, _Rfloat4_rtz)(uint4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f32_v8i32, _Rfloat8_rtz)(uint8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f32_v16i32, _Rfloat16_rtz)(uint16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v2f32_v2i32, _Rfloat2_rtp)(uint2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v3f32_v3i32, _Rfloat3_rtp)(uint3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v4f32_v4i32, _Rfloat4_rtp)(uint4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v8f32_v8i32, _Rfloat8_rtp)(uint8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v16f32_v16i32, _Rfloat16_rtp)(uint16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v2f32_v2i32, _Rfloat2_rtn)(uint2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v3f32_v3i32, _Rfloat3_rtn)(uint3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v4f32_v4i32, _Rfloat4_rtn)(uint4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v8f32_v8i32, _Rfloat8_rtn)(uint8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v16f32_v16i32, _Rfloat16_rtn)(uint16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f32_v2i64, _Rfloat2)(ulong2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f32_v3i64, _Rfloat3)(ulong3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f32_v4i64, _Rfloat4)(ulong4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f32_v8i64, _Rfloat8)(ulong8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f32_v16i64, _Rfloat16)(ulong16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v2f32_v2i64, _Rfloat2_rte)(ulong2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v3f32_v3i64, _Rfloat3_rte)(ulong3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v4f32_v4i64, _Rfloat4_rte)(ulong4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v8f32_v8i64, _Rfloat8_rte)(ulong8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v16f32_v16i64, _Rfloat16_rte)(ulong16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f32_v2i64, _Rfloat2_rtz)(ulong2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f32_v3i64, _Rfloat3_rtz)(ulong3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f32_v4i64, _Rfloat4_rtz)(ulong4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f32_v8i64, _Rfloat8_rtz)(ulong8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f32_v16i64, _Rfloat16_rtz)(ulong16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v2f32_v2i64, _Rfloat2_rtp)(ulong2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v3f32_v3i64, _Rfloat3_rtp)(ulong3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v4f32_v4i64, _Rfloat4_rtp)(ulong4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v8f32_v8i64, _Rfloat8_rtp)(ulong8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v16f32_v16i64, _Rfloat16_rtp)(ulong16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v2f32_v2i64, _Rfloat2_rtn)(ulong2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v3f32_v3i64, _Rfloat3_rtn)(ulong3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v4f32_v4i64, _Rfloat4_rtn)(ulong4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v8f32_v8i64, _Rfloat8_rtn)(ulong8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v16f32_v16i64, _Rfloat16_rtn)(ulong16 x);
#if defined(cl_khr_fp64)
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f64_v2i8, _Rdouble2)(uchar2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f64_v3i8, _Rdouble3)(uchar3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f64_v4i8, _Rdouble4)(uchar4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f64_v8i8, _Rdouble8)(uchar8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f64_v16i8, _Rdouble16)(uchar16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v2f64_v2i8, _Rdouble2_rte)(uchar2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v3f64_v3i8, _Rdouble3_rte)(uchar3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v4f64_v4i8, _Rdouble4_rte)(uchar4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v8f64_v8i8, _Rdouble8_rte)(uchar8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v16f64_v16i8, _Rdouble16_rte)(uchar16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f64_v2i8, _Rdouble2_rtz)(uchar2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f64_v3i8, _Rdouble3_rtz)(uchar3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f64_v4i8, _Rdouble4_rtz)(uchar4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f64_v8i8, _Rdouble8_rtz)(uchar8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f64_v16i8, _Rdouble16_rtz)(uchar16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v2f64_v2i8, _Rdouble2_rtp)(uchar2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v3f64_v3i8, _Rdouble3_rtp)(uchar3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v4f64_v4i8, _Rdouble4_rtp)(uchar4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v8f64_v8i8, _Rdouble8_rtp)(uchar8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v16f64_v16i8, _Rdouble16_rtp)(uchar16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v2f64_v2i8, _Rdouble2_rtn)(uchar2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v3f64_v3i8, _Rdouble3_rtn)(uchar3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v4f64_v4i8, _Rdouble4_rtn)(uchar4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v8f64_v8i8, _Rdouble8_rtn)(uchar8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v16f64_v16i8, _Rdouble16_rtn)(uchar16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f64_v2i16, _Rdouble2)(ushort2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f64_v3i16, _Rdouble3)(ushort3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f64_v4i16, _Rdouble4)(ushort4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f64_v8i16, _Rdouble8)(ushort8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f64_v16i16, _Rdouble16)(ushort16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v2f64_v2i16, _Rdouble2_rte)(ushort2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v3f64_v3i16, _Rdouble3_rte)(ushort3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v4f64_v4i16, _Rdouble4_rte)(ushort4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v8f64_v8i16, _Rdouble8_rte)(ushort8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v16f64_v16i16, _Rdouble16_rte)(ushort16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f64_v2i16, _Rdouble2_rtz)(ushort2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f64_v3i16, _Rdouble3_rtz)(ushort3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f64_v4i16, _Rdouble4_rtz)(ushort4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f64_v8i16, _Rdouble8_rtz)(ushort8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f64_v16i16, _Rdouble16_rtz)(ushort16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v2f64_v2i16, _Rdouble2_rtp)(ushort2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v3f64_v3i16, _Rdouble3_rtp)(ushort3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v4f64_v4i16, _Rdouble4_rtp)(ushort4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v8f64_v8i16, _Rdouble8_rtp)(ushort8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v16f64_v16i16, _Rdouble16_rtp)(ushort16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v2f64_v2i16, _Rdouble2_rtn)(ushort2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v3f64_v3i16, _Rdouble3_rtn)(ushort3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v4f64_v4i16, _Rdouble4_rtn)(ushort4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v8f64_v8i16, _Rdouble8_rtn)(ushort8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v16f64_v16i16, _Rdouble16_rtn)(ushort16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f64_v2i32, _Rdouble2)(uint2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f64_v3i32, _Rdouble3)(uint3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f64_v4i32, _Rdouble4)(uint4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f64_v8i32, _Rdouble8)(uint8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f64_v16i32, _Rdouble16)(uint16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v2f64_v2i32, _Rdouble2_rte)(uint2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v3f64_v3i32, _Rdouble3_rte)(uint3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v4f64_v4i32, _Rdouble4_rte)(uint4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v8f64_v8i32, _Rdouble8_rte)(uint8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v16f64_v16i32, _Rdouble16_rte)(uint16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f64_v2i32, _Rdouble2_rtz)(uint2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f64_v3i32, _Rdouble3_rtz)(uint3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f64_v4i32, _Rdouble4_rtz)(uint4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f64_v8i32, _Rdouble8_rtz)(uint8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f64_v16i32, _Rdouble16_rtz)(uint16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v2f64_v2i32, _Rdouble2_rtp)(uint2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v3f64_v3i32, _Rdouble3_rtp)(uint3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v4f64_v4i32, _Rdouble4_rtp)(uint4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v8f64_v8i32, _Rdouble8_rtp)(uint8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v16f64_v16i32, _Rdouble16_rtp)(uint16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v2f64_v2i32, _Rdouble2_rtn)(uint2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v3f64_v3i32, _Rdouble3_rtn)(uint3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v4f64_v4i32, _Rdouble4_rtn)(uint4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v8f64_v8i32, _Rdouble8_rtn)(uint8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v16f64_v16i32, _Rdouble16_rtn)(uint16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v2f64_v2i64, _Rdouble2)(ulong2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v3f64_v3i64, _Rdouble3)(ulong3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v4f64_v4i64, _Rdouble4)(ulong4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v8f64_v8i64, _Rdouble8)(ulong8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _v16f64_v16i64, _Rdouble16)(ulong16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v2f64_v2i64, _Rdouble2_rte)(ulong2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v3f64_v3i64, _Rdouble3_rte)(ulong3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v4f64_v4i64, _Rdouble4_rte)(ulong4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v8f64_v8i64, _Rdouble8_rte)(ulong8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_v16f64_v16i64, _Rdouble16_rte)(ulong16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v2f64_v2i64, _Rdouble2_rtz)(ulong2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v3f64_v3i64, _Rdouble3_rtz)(ulong3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v4f64_v4i64, _Rdouble4_rtz)(ulong4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v8f64_v8i64, _Rdouble8_rtz)(ulong8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_v16f64_v16i64, _Rdouble16_rtz)(ulong16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v2f64_v2i64, _Rdouble2_rtp)(ulong2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v3f64_v3i64, _Rdouble3_rtp)(ulong3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v4f64_v4i64, _Rdouble4_rtp)(ulong4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v8f64_v8i64, _Rdouble8_rtp)(ulong8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_v16f64_v16i64, _Rdouble16_rtp)(ulong16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v2f64_v2i64, _Rdouble2_rtn)(ulong2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v3f64_v3i64, _Rdouble3_rtn)(ulong3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v4f64_v4i64, _Rdouble4_rtn)(ulong4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v8f64_v8i64, _Rdouble8_rtn)(ulong8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_v16f64_v16i64, _Rdouble16_rtn)(ulong16 x);
#endif // defined(cl_khr_fp64)
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v2f16_v2f32, _Rhalf2)(float2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v3f16_v3f32, _Rhalf3)(float3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v4f16_v4f32, _Rhalf4)(float4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v8f16_v8f32, _Rhalf8)(float8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v16f16_v16f32, _Rhalf16)(float16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v2f16_v2f32, _Rhalf2_rte)(float2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v3f16_v3f32, _Rhalf3_rte)(float3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v4f16_v4f32, _Rhalf4_rte)(float4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v8f16_v8f32, _Rhalf8_rte)(float8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v16f16_v16f32, _Rhalf16_rte)(float16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v2f16_v2f32, _Rhalf2_rtz)(float2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v3f16_v3f32, _Rhalf3_rtz)(float3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v4f16_v4f32, _Rhalf4_rtz)(float4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v8f16_v8f32, _Rhalf8_rtz)(float8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v16f16_v16f32, _Rhalf16_rtz)(float16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v2f16_v2f32, _Rhalf2_rtp)(float2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v3f16_v3f32, _Rhalf3_rtp)(float3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v4f16_v4f32, _Rhalf4_rtp)(float4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v8f16_v8f32, _Rhalf8_rtp)(float8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v16f16_v16f32, _Rhalf16_rtp)(float16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v2f16_v2f32, _Rhalf2_rtn)(float2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v3f16_v3f32, _Rhalf3_rtn)(float3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v4f16_v4f32, _Rhalf4_rtn)(float4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v8f16_v8f32, _Rhalf8_rtn)(float8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v16f16_v16f32, _Rhalf16_rtn)(float16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v2f16_v2f16, _Rhalf2)(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v3f16_v3f16, _Rhalf3)(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v4f16_v4f16, _Rhalf4)(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v8f16_v8f16, _Rhalf8)(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v16f16_v16f16, _Rhalf16)(half16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v2f16_v2f16, _Rhalf2_rte)(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v3f16_v3f16, _Rhalf3_rte)(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v4f16_v4f16, _Rhalf4_rte)(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v8f16_v8f16, _Rhalf8_rte)(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v16f16_v16f16, _Rhalf16_rte)(half16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v2f16_v2f16, _Rhalf2_rtz)(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v3f16_v3f16, _Rhalf3_rtz)(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v4f16_v4f16, _Rhalf4_rtz)(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v8f16_v8f16, _Rhalf8_rtz)(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v16f16_v16f16, _Rhalf16_rtz)(half16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v2f16_v2f16, _Rhalf2_rtp)(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v3f16_v3f16, _Rhalf3_rtp)(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v4f16_v4f16, _Rhalf4_rtp)(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v8f16_v8f16, _Rhalf8_rtp)(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v16f16_v16f16, _Rhalf16_rtp)(half16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v2f16_v2f16, _Rhalf2_rtn)(half2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v3f16_v3f16, _Rhalf3_rtn)(half3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v4f16_v4f16, _Rhalf4_rtn)(half4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v8f16_v8f16, _Rhalf8_rtn)(half8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v16f16_v16f16, _Rhalf16_rtn)(half16 x);
#if defined(cl_khr_fp64)
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v2f16_v2f64, _Rhalf2)(double2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v3f16_v3f64, _Rhalf3)(double3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v4f16_v4f64, _Rhalf4)(double4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v8f16_v8f64, _Rhalf8)(double8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v16f16_v16f64, _Rhalf16)(double16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v2f16_v2f64, _Rhalf2_rte)(double2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v3f16_v3f64, _Rhalf3_rte)(double3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v4f16_v4f64, _Rhalf4_rte)(double4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v8f16_v8f64, _Rhalf8_rte)(double8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v16f16_v16f64, _Rhalf16_rte)(double16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v2f16_v2f64, _Rhalf2_rtz)(double2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v3f16_v3f64, _Rhalf3_rtz)(double3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v4f16_v4f64, _Rhalf4_rtz)(double4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v8f16_v8f64, _Rhalf8_rtz)(double8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v16f16_v16f64, _Rhalf16_rtz)(double16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v2f16_v2f64, _Rhalf2_rtp)(double2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v3f16_v3f64, _Rhalf3_rtp)(double3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v4f16_v4f64, _Rhalf4_rtp)(double4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v8f16_v8f64, _Rhalf8_rtp)(double8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v16f16_v16f64, _Rhalf16_rtp)(double16 x);
half2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v2f16_v2f64, _Rhalf2_rtn)(double2 x);
half3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v3f16_v3f64, _Rhalf3_rtn)(double3 x);
half4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v4f16_v4f64, _Rhalf4_rtn)(double4 x);
half8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v8f16_v8f64, _Rhalf8_rtn)(double8 x);
half16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v16f16_v16f64, _Rhalf16_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v2f32_v2f32, _Rfloat2)(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v3f32_v3f32, _Rfloat3)(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v4f32_v4f32, _Rfloat4)(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v8f32_v8f32, _Rfloat8)(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v16f32_v16f32, _Rfloat16)(float16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v2f32_v2f32, _Rfloat2_rte)(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v3f32_v3f32, _Rfloat3_rte)(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v4f32_v4f32, _Rfloat4_rte)(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v8f32_v8f32, _Rfloat8_rte)(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v16f32_v16f32, _Rfloat16_rte)(float16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v2f32_v2f32, _Rfloat2_rtz)(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v3f32_v3f32, _Rfloat3_rtz)(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v4f32_v4f32, _Rfloat4_rtz)(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v8f32_v8f32, _Rfloat8_rtz)(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v16f32_v16f32, _Rfloat16_rtz)(float16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v2f32_v2f32, _Rfloat2_rtp)(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v3f32_v3f32, _Rfloat3_rtp)(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v4f32_v4f32, _Rfloat4_rtp)(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v8f32_v8f32, _Rfloat8_rtp)(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v16f32_v16f32, _Rfloat16_rtp)(float16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v2f32_v2f32, _Rfloat2_rtn)(float2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v3f32_v3f32, _Rfloat3_rtn)(float3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v4f32_v4f32, _Rfloat4_rtn)(float4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v8f32_v8f32, _Rfloat8_rtn)(float8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v16f32_v16f32, _Rfloat16_rtn)(float16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v2f32_v2f16, _Rfloat2)(half2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v3f32_v3f16, _Rfloat3)(half3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v4f32_v4f16, _Rfloat4)(half4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v8f32_v8f16, _Rfloat8)(half8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v16f32_v16f16, _Rfloat16)(half16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v2f32_v2f16, _Rfloat2_rte)(half2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v3f32_v3f16, _Rfloat3_rte)(half3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v4f32_v4f16, _Rfloat4_rte)(half4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v8f32_v8f16, _Rfloat8_rte)(half8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v16f32_v16f16, _Rfloat16_rte)(half16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v2f32_v2f16, _Rfloat2_rtz)(half2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v3f32_v3f16, _Rfloat3_rtz)(half3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v4f32_v4f16, _Rfloat4_rtz)(half4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v8f32_v8f16, _Rfloat8_rtz)(half8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v16f32_v16f16, _Rfloat16_rtz)(half16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v2f32_v2f16, _Rfloat2_rtp)(half2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v3f32_v3f16, _Rfloat3_rtp)(half3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v4f32_v4f16, _Rfloat4_rtp)(half4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v8f32_v8f16, _Rfloat8_rtp)(half8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v16f32_v16f16, _Rfloat16_rtp)(half16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v2f32_v2f16, _Rfloat2_rtn)(half2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v3f32_v3f16, _Rfloat3_rtn)(half3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v4f32_v4f16, _Rfloat4_rtn)(half4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v8f32_v8f16, _Rfloat8_rtn)(half8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v16f32_v16f16, _Rfloat16_rtn)(half16 x);
#if defined(cl_khr_fp64)
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v2f32_v2f64, _Rfloat2)(double2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v3f32_v3f64, _Rfloat3)(double3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v4f32_v4f64, _Rfloat4)(double4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v8f32_v8f64, _Rfloat8)(double8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v16f32_v16f64, _Rfloat16)(double16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v2f32_v2f64, _Rfloat2_rte)(double2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v3f32_v3f64, _Rfloat3_rte)(double3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v4f32_v4f64, _Rfloat4_rte)(double4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v8f32_v8f64, _Rfloat8_rte)(double8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v16f32_v16f64, _Rfloat16_rte)(double16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v2f32_v2f64, _Rfloat2_rtz)(double2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v3f32_v3f64, _Rfloat3_rtz)(double3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v4f32_v4f64, _Rfloat4_rtz)(double4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v8f32_v8f64, _Rfloat8_rtz)(double8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v16f32_v16f64, _Rfloat16_rtz)(double16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v2f32_v2f64, _Rfloat2_rtp)(double2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v3f32_v3f64, _Rfloat3_rtp)(double3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v4f32_v4f64, _Rfloat4_rtp)(double4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v8f32_v8f64, _Rfloat8_rtp)(double8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v16f32_v16f64, _Rfloat16_rtp)(double16 x);
float2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v2f32_v2f64, _Rfloat2_rtn)(double2 x);
float3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v3f32_v3f64, _Rfloat3_rtn)(double3 x);
float4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v4f32_v4f64, _Rfloat4_rtn)(double4 x);
float8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v8f32_v8f64, _Rfloat8_rtn)(double8 x);
float16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v16f32_v16f64, _Rfloat16_rtn)(double16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v2f64_v2f32, _Rdouble2)(float2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v3f64_v3f32, _Rdouble3)(float3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v4f64_v4f32, _Rdouble4)(float4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v8f64_v8f32, _Rdouble8)(float8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v16f64_v16f32, _Rdouble16)(float16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v2f64_v2f32, _Rdouble2_rte)(float2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v3f64_v3f32, _Rdouble3_rte)(float3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v4f64_v4f32, _Rdouble4_rte)(float4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v8f64_v8f32, _Rdouble8_rte)(float8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v16f64_v16f32, _Rdouble16_rte)(float16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v2f64_v2f32, _Rdouble2_rtz)(float2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v3f64_v3f32, _Rdouble3_rtz)(float3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v4f64_v4f32, _Rdouble4_rtz)(float4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v8f64_v8f32, _Rdouble8_rtz)(float8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v16f64_v16f32, _Rdouble16_rtz)(float16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v2f64_v2f32, _Rdouble2_rtp)(float2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v3f64_v3f32, _Rdouble3_rtp)(float3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v4f64_v4f32, _Rdouble4_rtp)(float4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v8f64_v8f32, _Rdouble8_rtp)(float8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v16f64_v16f32, _Rdouble16_rtp)(float16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v2f64_v2f32, _Rdouble2_rtn)(float2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v3f64_v3f32, _Rdouble3_rtn)(float3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v4f64_v4f32, _Rdouble4_rtn)(float4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v8f64_v8f32, _Rdouble8_rtn)(float8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v16f64_v16f32, _Rdouble16_rtn)(float16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v2f64_v2f16, _Rdouble2)(half2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v3f64_v3f16, _Rdouble3)(half3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v4f64_v4f16, _Rdouble4)(half4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v8f64_v8f16, _Rdouble8)(half8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v16f64_v16f16, _Rdouble16)(half16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v2f64_v2f16, _Rdouble2_rte)(half2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v3f64_v3f16, _Rdouble3_rte)(half3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v4f64_v4f16, _Rdouble4_rte)(half4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v8f64_v8f16, _Rdouble8_rte)(half8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v16f64_v16f16, _Rdouble16_rte)(half16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v2f64_v2f16, _Rdouble2_rtz)(half2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v3f64_v3f16, _Rdouble3_rtz)(half3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v4f64_v4f16, _Rdouble4_rtz)(half4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v8f64_v8f16, _Rdouble8_rtz)(half8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v16f64_v16f16, _Rdouble16_rtz)(half16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v2f64_v2f16, _Rdouble2_rtp)(half2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v3f64_v3f16, _Rdouble3_rtp)(half3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v4f64_v4f16, _Rdouble4_rtp)(half4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v8f64_v8f16, _Rdouble8_rtp)(half8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v16f64_v16f16, _Rdouble16_rtp)(half16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v2f64_v2f16, _Rdouble2_rtn)(half2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v3f64_v3f16, _Rdouble3_rtn)(half3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v4f64_v4f16, _Rdouble4_rtn)(half4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v8f64_v8f16, _Rdouble8_rtn)(half8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v16f64_v16f16, _Rdouble16_rtn)(half16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v2f64_v2f64, _Rdouble2)(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v3f64_v3f64, _Rdouble3)(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v4f64_v4f64, _Rdouble4)(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v8f64_v8f64, _Rdouble8)(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _v16f64_v16f64, _Rdouble16)(double16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v2f64_v2f64, _Rdouble2_rte)(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v3f64_v3f64, _Rdouble3_rte)(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v4f64_v4f64, _Rdouble4_rte)(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v8f64_v8f64, _Rdouble8_rte)(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_v16f64_v16f64, _Rdouble16_rte)(double16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v2f64_v2f64, _Rdouble2_rtz)(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v3f64_v3f64, _Rdouble3_rtz)(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v4f64_v4f64, _Rdouble4_rtz)(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v8f64_v8f64, _Rdouble8_rtz)(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_v16f64_v16f64, _Rdouble16_rtz)(double16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v2f64_v2f64, _Rdouble2_rtp)(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v3f64_v3f64, _Rdouble3_rtp)(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v4f64_v4f64, _Rdouble4_rtp)(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v8f64_v8f64, _Rdouble8_rtp)(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_v16f64_v16f64, _Rdouble16_rtp)(double16 x);
double2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v2f64_v2f64, _Rdouble2_rtn)(double2 x);
double3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v3f64_v3f64, _Rdouble3_rtn)(double3 x);
double4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v4f64_v4f64, _Rdouble4_rtn)(double4 x);
double8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v8f64_v8f64, _Rdouble8_rtn)(double8 x);
double16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_v16f64_v16f64, _Rdouble16_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v2i8_v2i8, _Rchar2)(char2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v3i8_v3i8, _Rchar3)(char3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v4i8_v4i8, _Rchar4)(char4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v8i8_v8i8, _Rchar8)(char8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i8_v16i8, _Rchar16)(char16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i8_v2i8, _Rchar2_sat)(char2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i8_v3i8, _Rchar3_sat)(char3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i8_v4i8, _Rchar4_sat)(char4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i8_v8i8, _Rchar8_sat)(char8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i8_v16i8, _Rchar16_sat)(char16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v2i8_v2i16, _Rchar2)(short2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v3i8_v3i16, _Rchar3)(short3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v4i8_v4i16, _Rchar4)(short4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v8i8_v8i16, _Rchar8)(short8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i8_v16i16, _Rchar16)(short16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i8_v2i16, _Rchar2_sat)(short2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i8_v3i16, _Rchar3_sat)(short3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i8_v4i16, _Rchar4_sat)(short4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i8_v8i16, _Rchar8_sat)(short8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i8_v16i16, _Rchar16_sat)(short16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v2i8_v2i32, _Rchar2)(int2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v3i8_v3i32, _Rchar3)(int3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v4i8_v4i32, _Rchar4)(int4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v8i8_v8i32, _Rchar8)(int8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i8_v16i32, _Rchar16)(int16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i8_v2i32, _Rchar2_sat)(int2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i8_v3i32, _Rchar3_sat)(int3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i8_v4i32, _Rchar4_sat)(int4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i8_v8i32, _Rchar8_sat)(int8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i8_v16i32, _Rchar16_sat)(int16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v2i8_v2i64, _Rchar2)(long2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v3i8_v3i64, _Rchar3)(long3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v4i8_v4i64, _Rchar4)(long4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v8i8_v8i64, _Rchar8)(long8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i8_v16i64, _Rchar16)(long16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i8_v2i64, _Rchar2_sat)(long2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i8_v3i64, _Rchar3_sat)(long3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i8_v4i64, _Rchar4_sat)(long4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i8_v8i64, _Rchar8_sat)(long8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i8_v16i64, _Rchar16_sat)(long16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v2i16_v2i8, _Rshort2)(char2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v3i16_v3i8, _Rshort3)(char3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v4i16_v4i8, _Rshort4)(char4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v8i16_v8i8, _Rshort8)(char8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i16_v16i8, _Rshort16)(char16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i16_v2i8, _Rshort2_sat)(char2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i16_v3i8, _Rshort3_sat)(char3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i16_v4i8, _Rshort4_sat)(char4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i16_v8i8, _Rshort8_sat)(char8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i16_v16i8, _Rshort16_sat)(char16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v2i16_v2i16, _Rshort2)(short2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v3i16_v3i16, _Rshort3)(short3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v4i16_v4i16, _Rshort4)(short4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v8i16_v8i16, _Rshort8)(short8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i16_v16i16, _Rshort16)(short16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i16_v2i16, _Rshort2_sat)(short2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i16_v3i16, _Rshort3_sat)(short3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i16_v4i16, _Rshort4_sat)(short4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i16_v8i16, _Rshort8_sat)(short8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i16_v16i16, _Rshort16_sat)(short16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v2i16_v2i32, _Rshort2)(int2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v3i16_v3i32, _Rshort3)(int3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v4i16_v4i32, _Rshort4)(int4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v8i16_v8i32, _Rshort8)(int8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i16_v16i32, _Rshort16)(int16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i16_v2i32, _Rshort2_sat)(int2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i16_v3i32, _Rshort3_sat)(int3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i16_v4i32, _Rshort4_sat)(int4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i16_v8i32, _Rshort8_sat)(int8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i16_v16i32, _Rshort16_sat)(int16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v2i16_v2i64, _Rshort2)(long2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v3i16_v3i64, _Rshort3)(long3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v4i16_v4i64, _Rshort4)(long4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v8i16_v8i64, _Rshort8)(long8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i16_v16i64, _Rshort16)(long16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i16_v2i64, _Rshort2_sat)(long2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i16_v3i64, _Rshort3_sat)(long3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i16_v4i64, _Rshort4_sat)(long4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i16_v8i64, _Rshort8_sat)(long8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i16_v16i64, _Rshort16_sat)(long16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v2i32_v2i8, _Rint2)(char2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v3i32_v3i8, _Rint3)(char3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v4i32_v4i8, _Rint4)(char4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v8i32_v8i8, _Rint8)(char8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i32_v16i8, _Rint16)(char16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i32_v2i8, _Rint2_sat)(char2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i32_v3i8, _Rint3_sat)(char3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i32_v4i8, _Rint4_sat)(char4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i32_v8i8, _Rint8_sat)(char8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i32_v16i8, _Rint16_sat)(char16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v2i32_v2i16, _Rint2)(short2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v3i32_v3i16, _Rint3)(short3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v4i32_v4i16, _Rint4)(short4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v8i32_v8i16, _Rint8)(short8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i32_v16i16, _Rint16)(short16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i32_v2i16, _Rint2_sat)(short2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i32_v3i16, _Rint3_sat)(short3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i32_v4i16, _Rint4_sat)(short4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i32_v8i16, _Rint8_sat)(short8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i32_v16i16, _Rint16_sat)(short16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v2i32_v2i32, _Rint2)(int2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v3i32_v3i32, _Rint3)(int3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v4i32_v4i32, _Rint4)(int4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v8i32_v8i32, _Rint8)(int8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i32_v16i32, _Rint16)(int16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i32_v2i32, _Rint2_sat)(int2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i32_v3i32, _Rint3_sat)(int3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i32_v4i32, _Rint4_sat)(int4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i32_v8i32, _Rint8_sat)(int8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i32_v16i32, _Rint16_sat)(int16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v2i32_v2i64, _Rint2)(long2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v3i32_v3i64, _Rint3)(long3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v4i32_v4i64, _Rint4)(long4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v8i32_v8i64, _Rint8)(long8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i32_v16i64, _Rint16)(long16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i32_v2i64, _Rint2_sat)(long2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i32_v3i64, _Rint3_sat)(long3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i32_v4i64, _Rint4_sat)(long4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i32_v8i64, _Rint8_sat)(long8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i32_v16i64, _Rint16_sat)(long16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v2i64_v2i8, _Rlong2)(char2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v3i64_v3i8, _Rlong3)(char3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v4i64_v4i8, _Rlong4)(char4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v8i64_v8i8, _Rlong8)(char8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i64_v16i8, _Rlong16)(char16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i64_v2i8, _Rlong2_sat)(char2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i64_v3i8, _Rlong3_sat)(char3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i64_v4i8, _Rlong4_sat)(char4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i64_v8i8, _Rlong8_sat)(char8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i64_v16i8, _Rlong16_sat)(char16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v2i64_v2i16, _Rlong2)(short2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v3i64_v3i16, _Rlong3)(short3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v4i64_v4i16, _Rlong4)(short4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v8i64_v8i16, _Rlong8)(short8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i64_v16i16, _Rlong16)(short16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i64_v2i16, _Rlong2_sat)(short2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i64_v3i16, _Rlong3_sat)(short3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i64_v4i16, _Rlong4_sat)(short4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i64_v8i16, _Rlong8_sat)(short8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i64_v16i16, _Rlong16_sat)(short16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v2i64_v2i32, _Rlong2)(int2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v3i64_v3i32, _Rlong3)(int3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v4i64_v4i32, _Rlong4)(int4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v8i64_v8i32, _Rlong8)(int8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i64_v16i32, _Rlong16)(int16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i64_v2i32, _Rlong2_sat)(int2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i64_v3i32, _Rlong3_sat)(int3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i64_v4i32, _Rlong4_sat)(int4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i64_v8i32, _Rlong8_sat)(int8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i64_v16i32, _Rlong16_sat)(int16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v2i64_v2i64, _Rlong2)(long2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v3i64_v3i64, _Rlong3)(long3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v4i64_v4i64, _Rlong4)(long4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v8i64_v8i64, _Rlong8)(long8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _v16i64_v16i64, _Rlong16)(long16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v2i64_v2i64, _Rlong2_sat)(long2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v3i64_v3i64, _Rlong3_sat)(long3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v4i64_v4i64, _Rlong4_sat)(long4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v8i64_v8i64, _Rlong8_sat)(long8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_v16i64_v16i64, _Rlong16_sat)(long16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v2i8_v2i8, _Ruchar2)(uchar2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v3i8_v3i8, _Ruchar3)(uchar3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v4i8_v4i8, _Ruchar4)(uchar4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v8i8_v8i8, _Ruchar8)(uchar8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i8_v16i8, _Ruchar16)(uchar16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i8_v2i8, _Ruchar2_sat)(uchar2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i8_v3i8, _Ruchar3_sat)(uchar3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i8_v4i8, _Ruchar4_sat)(uchar4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i8_v8i8, _Ruchar8_sat)(uchar8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v16i8_v16i8, _Ruchar16_sat)(uchar16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v2i8_v2i16, _Ruchar2)(ushort2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v3i8_v3i16, _Ruchar3)(ushort3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v4i8_v4i16, _Ruchar4)(ushort4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v8i8_v8i16, _Ruchar8)(ushort8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i8_v16i16, _Ruchar16)(ushort16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i8_v2i16, _Ruchar2_sat)(ushort2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i8_v3i16, _Ruchar3_sat)(ushort3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i8_v4i16, _Ruchar4_sat)(ushort4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i8_v8i16, _Ruchar8_sat)(ushort8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v16i8_v16i16, _Ruchar16_sat)(ushort16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v2i8_v2i32, _Ruchar2)(uint2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v3i8_v3i32, _Ruchar3)(uint3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v4i8_v4i32, _Ruchar4)(uint4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v8i8_v8i32, _Ruchar8)(uint8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i8_v16i32, _Ruchar16)(uint16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i8_v2i32, _Ruchar2_sat)(uint2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i8_v3i32, _Ruchar3_sat)(uint3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i8_v4i32, _Ruchar4_sat)(uint4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i8_v8i32, _Ruchar8_sat)(uint8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v16i8_v16i32, _Ruchar16_sat)(uint16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v2i8_v2i64, _Ruchar2)(ulong2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v3i8_v3i64, _Ruchar3)(ulong3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v4i8_v4i64, _Ruchar4)(ulong4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v8i8_v8i64, _Ruchar8)(ulong8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i8_v16i64, _Ruchar16)(ulong16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i8_v2i64, _Ruchar2_sat)(ulong2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i8_v3i64, _Ruchar3_sat)(ulong3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i8_v4i64, _Ruchar4_sat)(ulong4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i8_v8i64, _Ruchar8_sat)(ulong8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v16i8_v16i64, _Ruchar16_sat)(ulong16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v2i16_v2i8, _Rushort2)(uchar2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v3i16_v3i8, _Rushort3)(uchar3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v4i16_v4i8, _Rushort4)(uchar4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v8i16_v8i8, _Rushort8)(uchar8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i16_v16i8, _Rushort16)(uchar16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i16_v2i8, _Rushort2_sat)(uchar2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i16_v3i8, _Rushort3_sat)(uchar3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i16_v4i8, _Rushort4_sat)(uchar4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i16_v8i8, _Rushort8_sat)(uchar8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v16i16_v16i8, _Rushort16_sat)(uchar16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v2i16_v2i16, _Rushort2)(ushort2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v3i16_v3i16, _Rushort3)(ushort3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v4i16_v4i16, _Rushort4)(ushort4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v8i16_v8i16, _Rushort8)(ushort8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i16_v16i16, _Rushort16)(ushort16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i16_v2i16, _Rushort2_sat)(ushort2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i16_v3i16, _Rushort3_sat)(ushort3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i16_v4i16, _Rushort4_sat)(ushort4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i16_v8i16, _Rushort8_sat)(ushort8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v16i16_v16i16, _Rushort16_sat)(ushort16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v2i16_v2i32, _Rushort2)(uint2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v3i16_v3i32, _Rushort3)(uint3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v4i16_v4i32, _Rushort4)(uint4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v8i16_v8i32, _Rushort8)(uint8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i16_v16i32, _Rushort16)(uint16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i16_v2i32, _Rushort2_sat)(uint2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i16_v3i32, _Rushort3_sat)(uint3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i16_v4i32, _Rushort4_sat)(uint4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i16_v8i32, _Rushort8_sat)(uint8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v16i16_v16i32, _Rushort16_sat)(uint16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v2i16_v2i64, _Rushort2)(ulong2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v3i16_v3i64, _Rushort3)(ulong3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v4i16_v4i64, _Rushort4)(ulong4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v8i16_v8i64, _Rushort8)(ulong8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i16_v16i64, _Rushort16)(ulong16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i16_v2i64, _Rushort2_sat)(ulong2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i16_v3i64, _Rushort3_sat)(ulong3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i16_v4i64, _Rushort4_sat)(ulong4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i16_v8i64, _Rushort8_sat)(ulong8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v16i16_v16i64, _Rushort16_sat)(ulong16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v2i32_v2i8, _Ruint2)(uchar2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v3i32_v3i8, _Ruint3)(uchar3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v4i32_v4i8, _Ruint4)(uchar4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v8i32_v8i8, _Ruint8)(uchar8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i32_v16i8, _Ruint16)(uchar16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i32_v2i8, _Ruint2_sat)(uchar2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i32_v3i8, _Ruint3_sat)(uchar3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i32_v4i8, _Ruint4_sat)(uchar4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i32_v8i8, _Ruint8_sat)(uchar8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v16i32_v16i8, _Ruint16_sat)(uchar16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v2i32_v2i16, _Ruint2)(ushort2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v3i32_v3i16, _Ruint3)(ushort3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v4i32_v4i16, _Ruint4)(ushort4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v8i32_v8i16, _Ruint8)(ushort8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i32_v16i16, _Ruint16)(ushort16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i32_v2i16, _Ruint2_sat)(ushort2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i32_v3i16, _Ruint3_sat)(ushort3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i32_v4i16, _Ruint4_sat)(ushort4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i32_v8i16, _Ruint8_sat)(ushort8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v16i32_v16i16, _Ruint16_sat)(ushort16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v2i32_v2i32, _Ruint2)(uint2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v3i32_v3i32, _Ruint3)(uint3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v4i32_v4i32, _Ruint4)(uint4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v8i32_v8i32, _Ruint8)(uint8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i32_v16i32, _Ruint16)(uint16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i32_v2i32, _Ruint2_sat)(uint2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i32_v3i32, _Ruint3_sat)(uint3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i32_v4i32, _Ruint4_sat)(uint4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i32_v8i32, _Ruint8_sat)(uint8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v16i32_v16i32, _Ruint16_sat)(uint16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v2i32_v2i64, _Ruint2)(ulong2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v3i32_v3i64, _Ruint3)(ulong3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v4i32_v4i64, _Ruint4)(ulong4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v8i32_v8i64, _Ruint8)(ulong8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i32_v16i64, _Ruint16)(ulong16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i32_v2i64, _Ruint2_sat)(ulong2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i32_v3i64, _Ruint3_sat)(ulong3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i32_v4i64, _Ruint4_sat)(ulong4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i32_v8i64, _Ruint8_sat)(ulong8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v16i32_v16i64, _Ruint16_sat)(ulong16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v2i64_v2i8, _Rulong2)(uchar2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v3i64_v3i8, _Rulong3)(uchar3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v4i64_v4i8, _Rulong4)(uchar4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v8i64_v8i8, _Rulong8)(uchar8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i64_v16i8, _Rulong16)(uchar16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i64_v2i8, _Rulong2_sat)(uchar2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i64_v3i8, _Rulong3_sat)(uchar3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i64_v4i8, _Rulong4_sat)(uchar4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i64_v8i8, _Rulong8_sat)(uchar8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v16i64_v16i8, _Rulong16_sat)(uchar16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v2i64_v2i16, _Rulong2)(ushort2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v3i64_v3i16, _Rulong3)(ushort3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v4i64_v4i16, _Rulong4)(ushort4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v8i64_v8i16, _Rulong8)(ushort8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i64_v16i16, _Rulong16)(ushort16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i64_v2i16, _Rulong2_sat)(ushort2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i64_v3i16, _Rulong3_sat)(ushort3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i64_v4i16, _Rulong4_sat)(ushort4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i64_v8i16, _Rulong8_sat)(ushort8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v16i64_v16i16, _Rulong16_sat)(ushort16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v2i64_v2i32, _Rulong2)(uint2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v3i64_v3i32, _Rulong3)(uint3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v4i64_v4i32, _Rulong4)(uint4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v8i64_v8i32, _Rulong8)(uint8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i64_v16i32, _Rulong16)(uint16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i64_v2i32, _Rulong2_sat)(uint2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i64_v3i32, _Rulong3_sat)(uint3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i64_v4i32, _Rulong4_sat)(uint4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i64_v8i32, _Rulong8_sat)(uint8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v16i64_v16i32, _Rulong16_sat)(uint16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v2i64_v2i64, _Rulong2)(ulong2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v3i64_v3i64, _Rulong3)(ulong3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v4i64_v4i64, _Rulong4)(ulong4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v8i64_v8i64, _Rulong8)(ulong8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _v16i64_v16i64, _Rulong16)(ulong16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v2i64_v2i64, _Rulong2_sat)(ulong2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v3i64_v3i64, _Rulong3_sat)(ulong3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v4i64_v4i64, _Rulong4_sat)(ulong4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v8i64_v8i64, _Rulong8_sat)(ulong8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_v16i64_v16i64, _Rulong16_sat)(ulong16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v2i8_v2f32, _Rchar2)(float2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v3i8_v3f32, _Rchar3)(float3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v4i8_v4f32, _Rchar4)(float4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v8i8_v8f32, _Rchar8)(float8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i8_v16f32, _Rchar16)(float16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v2i8_v2f32, _Rchar2_sat)(float2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v3i8_v3f32, _Rchar3_sat)(float3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v4i8_v4f32, _Rchar4_sat)(float4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v8i8_v8f32, _Rchar8_sat)(float8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v16i8_v16f32, _Rchar16_sat)(float16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v2i8_v2f32, _Rchar2_rte)(float2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v3i8_v3f32, _Rchar3_rte)(float3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v4i8_v4f32, _Rchar4_rte)(float4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v8i8_v8f32, _Rchar8_rte)(float8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v16i8_v16f32, _Rchar16_rte)(float16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i8_v2f32, _Rchar2_rtz)(float2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i8_v3f32, _Rchar3_rtz)(float3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i8_v4f32, _Rchar4_rtz)(float4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i8_v8f32, _Rchar8_rtz)(float8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i8_v16f32, _Rchar16_rtz)(float16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v2i8_v2f32, _Rchar2_rtp)(float2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v3i8_v3f32, _Rchar3_rtp)(float3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v4i8_v4f32, _Rchar4_rtp)(float4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v8i8_v8f32, _Rchar8_rtp)(float8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v16i8_v16f32, _Rchar16_rtp)(float16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v2i8_v2f32, _Rchar2_rtn)(float2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v3i8_v3f32, _Rchar3_rtn)(float3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v4i8_v4f32, _Rchar4_rtn)(float4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v8i8_v8f32, _Rchar8_rtn)(float8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v16i8_v16f32, _Rchar16_rtn)(float16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i8_v2f32, _Rchar2_sat_rte)(float2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i8_v3f32, _Rchar3_sat_rte)(float3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i8_v4f32, _Rchar4_sat_rte)(float4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i8_v8f32, _Rchar8_sat_rte)(float8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i8_v16f32, _Rchar16_sat_rte)(float16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i8_v2f32, _Rchar2_sat_rtz)(float2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i8_v3f32, _Rchar3_sat_rtz)(float3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i8_v4f32, _Rchar4_sat_rtz)(float4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i8_v8f32, _Rchar8_sat_rtz)(float8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i8_v16f32, _Rchar16_sat_rtz)(float16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i8_v2f32, _Rchar2_sat_rtp)(float2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i8_v3f32, _Rchar3_sat_rtp)(float3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i8_v4f32, _Rchar4_sat_rtp)(float4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i8_v8f32, _Rchar8_sat_rtp)(float8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i8_v16f32, _Rchar16_sat_rtp)(float16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i8_v2f32, _Rchar2_sat_rtn)(float2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i8_v3f32, _Rchar3_sat_rtn)(float3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i8_v4f32, _Rchar4_sat_rtn)(float4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i8_v8f32, _Rchar8_sat_rtn)(float8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i8_v16f32, _Rchar16_sat_rtn)(float16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v2i8_v2f16, _Rchar2)(half2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v3i8_v3f16, _Rchar3)(half3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v4i8_v4f16, _Rchar4)(half4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v8i8_v8f16, _Rchar8)(half8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i8_v16f16, _Rchar16)(half16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v2i8_v2f16, _Rchar2_sat)(half2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v3i8_v3f16, _Rchar3_sat)(half3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v4i8_v4f16, _Rchar4_sat)(half4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v8i8_v8f16, _Rchar8_sat)(half8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v16i8_v16f16, _Rchar16_sat)(half16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v2i8_v2f16, _Rchar2_rte)(half2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v3i8_v3f16, _Rchar3_rte)(half3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v4i8_v4f16, _Rchar4_rte)(half4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v8i8_v8f16, _Rchar8_rte)(half8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v16i8_v16f16, _Rchar16_rte)(half16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i8_v2f16, _Rchar2_rtz)(half2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i8_v3f16, _Rchar3_rtz)(half3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i8_v4f16, _Rchar4_rtz)(half4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i8_v8f16, _Rchar8_rtz)(half8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i8_v16f16, _Rchar16_rtz)(half16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v2i8_v2f16, _Rchar2_rtp)(half2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v3i8_v3f16, _Rchar3_rtp)(half3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v4i8_v4f16, _Rchar4_rtp)(half4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v8i8_v8f16, _Rchar8_rtp)(half8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v16i8_v16f16, _Rchar16_rtp)(half16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v2i8_v2f16, _Rchar2_rtn)(half2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v3i8_v3f16, _Rchar3_rtn)(half3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v4i8_v4f16, _Rchar4_rtn)(half4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v8i8_v8f16, _Rchar8_rtn)(half8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v16i8_v16f16, _Rchar16_rtn)(half16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i8_v2f16, _Rchar2_sat_rte)(half2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i8_v3f16, _Rchar3_sat_rte)(half3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i8_v4f16, _Rchar4_sat_rte)(half4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i8_v8f16, _Rchar8_sat_rte)(half8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i8_v16f16, _Rchar16_sat_rte)(half16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i8_v2f16, _Rchar2_sat_rtz)(half2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i8_v3f16, _Rchar3_sat_rtz)(half3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i8_v4f16, _Rchar4_sat_rtz)(half4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i8_v8f16, _Rchar8_sat_rtz)(half8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i8_v16f16, _Rchar16_sat_rtz)(half16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i8_v2f16, _Rchar2_sat_rtp)(half2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i8_v3f16, _Rchar3_sat_rtp)(half3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i8_v4f16, _Rchar4_sat_rtp)(half4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i8_v8f16, _Rchar8_sat_rtp)(half8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i8_v16f16, _Rchar16_sat_rtp)(half16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i8_v2f16, _Rchar2_sat_rtn)(half2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i8_v3f16, _Rchar3_sat_rtn)(half3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i8_v4f16, _Rchar4_sat_rtn)(half4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i8_v8f16, _Rchar8_sat_rtn)(half8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i8_v16f16, _Rchar16_sat_rtn)(half16 x);
#if defined(cl_khr_fp64)
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v2i8_v2f64, _Rchar2)(double2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v3i8_v3f64, _Rchar3)(double3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v4i8_v4f64, _Rchar4)(double4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v8i8_v8f64, _Rchar8)(double8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i8_v16f64, _Rchar16)(double16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v2i8_v2f64, _Rchar2_sat)(double2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v3i8_v3f64, _Rchar3_sat)(double3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v4i8_v4f64, _Rchar4_sat)(double4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v8i8_v8f64, _Rchar8_sat)(double8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v16i8_v16f64, _Rchar16_sat)(double16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v2i8_v2f64, _Rchar2_rte)(double2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v3i8_v3f64, _Rchar3_rte)(double3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v4i8_v4f64, _Rchar4_rte)(double4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v8i8_v8f64, _Rchar8_rte)(double8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v16i8_v16f64, _Rchar16_rte)(double16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i8_v2f64, _Rchar2_rtz)(double2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i8_v3f64, _Rchar3_rtz)(double3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i8_v4f64, _Rchar4_rtz)(double4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i8_v8f64, _Rchar8_rtz)(double8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i8_v16f64, _Rchar16_rtz)(double16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v2i8_v2f64, _Rchar2_rtp)(double2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v3i8_v3f64, _Rchar3_rtp)(double3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v4i8_v4f64, _Rchar4_rtp)(double4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v8i8_v8f64, _Rchar8_rtp)(double8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v16i8_v16f64, _Rchar16_rtp)(double16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v2i8_v2f64, _Rchar2_rtn)(double2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v3i8_v3f64, _Rchar3_rtn)(double3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v4i8_v4f64, _Rchar4_rtn)(double4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v8i8_v8f64, _Rchar8_rtn)(double8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v16i8_v16f64, _Rchar16_rtn)(double16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i8_v2f64, _Rchar2_sat_rte)(double2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i8_v3f64, _Rchar3_sat_rte)(double3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i8_v4f64, _Rchar4_sat_rte)(double4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i8_v8f64, _Rchar8_sat_rte)(double8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i8_v16f64, _Rchar16_sat_rte)(double16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i8_v2f64, _Rchar2_sat_rtz)(double2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i8_v3f64, _Rchar3_sat_rtz)(double3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i8_v4f64, _Rchar4_sat_rtz)(double4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i8_v8f64, _Rchar8_sat_rtz)(double8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i8_v16f64, _Rchar16_sat_rtz)(double16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i8_v2f64, _Rchar2_sat_rtp)(double2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i8_v3f64, _Rchar3_sat_rtp)(double3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i8_v4f64, _Rchar4_sat_rtp)(double4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i8_v8f64, _Rchar8_sat_rtp)(double8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i8_v16f64, _Rchar16_sat_rtp)(double16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i8_v2f64, _Rchar2_sat_rtn)(double2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i8_v3f64, _Rchar3_sat_rtn)(double3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i8_v4f64, _Rchar4_sat_rtn)(double4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i8_v8f64, _Rchar8_sat_rtn)(double8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i8_v16f64, _Rchar16_sat_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v2i16_v2f32, _Rshort2)(float2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v3i16_v3f32, _Rshort3)(float3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v4i16_v4f32, _Rshort4)(float4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v8i16_v8f32, _Rshort8)(float8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i16_v16f32, _Rshort16)(float16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v2i16_v2f32, _Rshort2_sat)(float2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v3i16_v3f32, _Rshort3_sat)(float3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v4i16_v4f32, _Rshort4_sat)(float4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v8i16_v8f32, _Rshort8_sat)(float8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v16i16_v16f32, _Rshort16_sat)(float16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v2i16_v2f32, _Rshort2_rte)(float2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v3i16_v3f32, _Rshort3_rte)(float3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v4i16_v4f32, _Rshort4_rte)(float4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v8i16_v8f32, _Rshort8_rte)(float8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v16i16_v16f32, _Rshort16_rte)(float16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i16_v2f32, _Rshort2_rtz)(float2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i16_v3f32, _Rshort3_rtz)(float3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i16_v4f32, _Rshort4_rtz)(float4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i16_v8f32, _Rshort8_rtz)(float8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i16_v16f32, _Rshort16_rtz)(float16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v2i16_v2f32, _Rshort2_rtp)(float2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v3i16_v3f32, _Rshort3_rtp)(float3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v4i16_v4f32, _Rshort4_rtp)(float4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v8i16_v8f32, _Rshort8_rtp)(float8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v16i16_v16f32, _Rshort16_rtp)(float16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v2i16_v2f32, _Rshort2_rtn)(float2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v3i16_v3f32, _Rshort3_rtn)(float3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v4i16_v4f32, _Rshort4_rtn)(float4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v8i16_v8f32, _Rshort8_rtn)(float8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v16i16_v16f32, _Rshort16_rtn)(float16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i16_v2f32, _Rshort2_sat_rte)(float2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i16_v3f32, _Rshort3_sat_rte)(float3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i16_v4f32, _Rshort4_sat_rte)(float4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i16_v8f32, _Rshort8_sat_rte)(float8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i16_v16f32, _Rshort16_sat_rte)(float16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i16_v2f32, _Rshort2_sat_rtz)(float2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i16_v3f32, _Rshort3_sat_rtz)(float3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i16_v4f32, _Rshort4_sat_rtz)(float4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i16_v8f32, _Rshort8_sat_rtz)(float8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i16_v16f32, _Rshort16_sat_rtz)(float16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i16_v2f32, _Rshort2_sat_rtp)(float2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i16_v3f32, _Rshort3_sat_rtp)(float3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i16_v4f32, _Rshort4_sat_rtp)(float4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i16_v8f32, _Rshort8_sat_rtp)(float8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i16_v16f32, _Rshort16_sat_rtp)(float16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i16_v2f32, _Rshort2_sat_rtn)(float2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i16_v3f32, _Rshort3_sat_rtn)(float3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i16_v4f32, _Rshort4_sat_rtn)(float4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i16_v8f32, _Rshort8_sat_rtn)(float8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i16_v16f32, _Rshort16_sat_rtn)(float16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v2i16_v2f16, _Rshort2)(half2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v3i16_v3f16, _Rshort3)(half3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v4i16_v4f16, _Rshort4)(half4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v8i16_v8f16, _Rshort8)(half8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i16_v16f16, _Rshort16)(half16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v2i16_v2f16, _Rshort2_sat)(half2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v3i16_v3f16, _Rshort3_sat)(half3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v4i16_v4f16, _Rshort4_sat)(half4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v8i16_v8f16, _Rshort8_sat)(half8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v16i16_v16f16, _Rshort16_sat)(half16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v2i16_v2f16, _Rshort2_rte)(half2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v3i16_v3f16, _Rshort3_rte)(half3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v4i16_v4f16, _Rshort4_rte)(half4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v8i16_v8f16, _Rshort8_rte)(half8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v16i16_v16f16, _Rshort16_rte)(half16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i16_v2f16, _Rshort2_rtz)(half2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i16_v3f16, _Rshort3_rtz)(half3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i16_v4f16, _Rshort4_rtz)(half4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i16_v8f16, _Rshort8_rtz)(half8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i16_v16f16, _Rshort16_rtz)(half16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v2i16_v2f16, _Rshort2_rtp)(half2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v3i16_v3f16, _Rshort3_rtp)(half3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v4i16_v4f16, _Rshort4_rtp)(half4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v8i16_v8f16, _Rshort8_rtp)(half8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v16i16_v16f16, _Rshort16_rtp)(half16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v2i16_v2f16, _Rshort2_rtn)(half2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v3i16_v3f16, _Rshort3_rtn)(half3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v4i16_v4f16, _Rshort4_rtn)(half4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v8i16_v8f16, _Rshort8_rtn)(half8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v16i16_v16f16, _Rshort16_rtn)(half16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i16_v2f16, _Rshort2_sat_rte)(half2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i16_v3f16, _Rshort3_sat_rte)(half3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i16_v4f16, _Rshort4_sat_rte)(half4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i16_v8f16, _Rshort8_sat_rte)(half8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i16_v16f16, _Rshort16_sat_rte)(half16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i16_v2f16, _Rshort2_sat_rtz)(half2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i16_v3f16, _Rshort3_sat_rtz)(half3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i16_v4f16, _Rshort4_sat_rtz)(half4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i16_v8f16, _Rshort8_sat_rtz)(half8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i16_v16f16, _Rshort16_sat_rtz)(half16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i16_v2f16, _Rshort2_sat_rtp)(half2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i16_v3f16, _Rshort3_sat_rtp)(half3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i16_v4f16, _Rshort4_sat_rtp)(half4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i16_v8f16, _Rshort8_sat_rtp)(half8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i16_v16f16, _Rshort16_sat_rtp)(half16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i16_v2f16, _Rshort2_sat_rtn)(half2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i16_v3f16, _Rshort3_sat_rtn)(half3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i16_v4f16, _Rshort4_sat_rtn)(half4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i16_v8f16, _Rshort8_sat_rtn)(half8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i16_v16f16, _Rshort16_sat_rtn)(half16 x);
#if defined(cl_khr_fp64)
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v2i16_v2f64, _Rshort2)(double2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v3i16_v3f64, _Rshort3)(double3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v4i16_v4f64, _Rshort4)(double4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v8i16_v8f64, _Rshort8)(double8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i16_v16f64, _Rshort16)(double16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v2i16_v2f64, _Rshort2_sat)(double2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v3i16_v3f64, _Rshort3_sat)(double3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v4i16_v4f64, _Rshort4_sat)(double4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v8i16_v8f64, _Rshort8_sat)(double8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v16i16_v16f64, _Rshort16_sat)(double16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v2i16_v2f64, _Rshort2_rte)(double2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v3i16_v3f64, _Rshort3_rte)(double3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v4i16_v4f64, _Rshort4_rte)(double4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v8i16_v8f64, _Rshort8_rte)(double8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v16i16_v16f64, _Rshort16_rte)(double16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i16_v2f64, _Rshort2_rtz)(double2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i16_v3f64, _Rshort3_rtz)(double3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i16_v4f64, _Rshort4_rtz)(double4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i16_v8f64, _Rshort8_rtz)(double8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i16_v16f64, _Rshort16_rtz)(double16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v2i16_v2f64, _Rshort2_rtp)(double2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v3i16_v3f64, _Rshort3_rtp)(double3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v4i16_v4f64, _Rshort4_rtp)(double4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v8i16_v8f64, _Rshort8_rtp)(double8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v16i16_v16f64, _Rshort16_rtp)(double16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v2i16_v2f64, _Rshort2_rtn)(double2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v3i16_v3f64, _Rshort3_rtn)(double3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v4i16_v4f64, _Rshort4_rtn)(double4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v8i16_v8f64, _Rshort8_rtn)(double8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v16i16_v16f64, _Rshort16_rtn)(double16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i16_v2f64, _Rshort2_sat_rte)(double2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i16_v3f64, _Rshort3_sat_rte)(double3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i16_v4f64, _Rshort4_sat_rte)(double4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i16_v8f64, _Rshort8_sat_rte)(double8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i16_v16f64, _Rshort16_sat_rte)(double16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i16_v2f64, _Rshort2_sat_rtz)(double2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i16_v3f64, _Rshort3_sat_rtz)(double3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i16_v4f64, _Rshort4_sat_rtz)(double4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i16_v8f64, _Rshort8_sat_rtz)(double8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i16_v16f64, _Rshort16_sat_rtz)(double16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i16_v2f64, _Rshort2_sat_rtp)(double2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i16_v3f64, _Rshort3_sat_rtp)(double3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i16_v4f64, _Rshort4_sat_rtp)(double4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i16_v8f64, _Rshort8_sat_rtp)(double8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i16_v16f64, _Rshort16_sat_rtp)(double16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i16_v2f64, _Rshort2_sat_rtn)(double2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i16_v3f64, _Rshort3_sat_rtn)(double3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i16_v4f64, _Rshort4_sat_rtn)(double4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i16_v8f64, _Rshort8_sat_rtn)(double8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i16_v16f64, _Rshort16_sat_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v2i32_v2f32, _Rint2)(float2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v3i32_v3f32, _Rint3)(float3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v4i32_v4f32, _Rint4)(float4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v8i32_v8f32, _Rint8)(float8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i32_v16f32, _Rint16)(float16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v2i32_v2f32, _Rint2_sat)(float2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v3i32_v3f32, _Rint3_sat)(float3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v4i32_v4f32, _Rint4_sat)(float4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v8i32_v8f32, _Rint8_sat)(float8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v16i32_v16f32, _Rint16_sat)(float16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v2i32_v2f32, _Rint2_rte)(float2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v3i32_v3f32, _Rint3_rte)(float3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v4i32_v4f32, _Rint4_rte)(float4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v8i32_v8f32, _Rint8_rte)(float8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v16i32_v16f32, _Rint16_rte)(float16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i32_v2f32, _Rint2_rtz)(float2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i32_v3f32, _Rint3_rtz)(float3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i32_v4f32, _Rint4_rtz)(float4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i32_v8f32, _Rint8_rtz)(float8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i32_v16f32, _Rint16_rtz)(float16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v2i32_v2f32, _Rint2_rtp)(float2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v3i32_v3f32, _Rint3_rtp)(float3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v4i32_v4f32, _Rint4_rtp)(float4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v8i32_v8f32, _Rint8_rtp)(float8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v16i32_v16f32, _Rint16_rtp)(float16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v2i32_v2f32, _Rint2_rtn)(float2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v3i32_v3f32, _Rint3_rtn)(float3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v4i32_v4f32, _Rint4_rtn)(float4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v8i32_v8f32, _Rint8_rtn)(float8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v16i32_v16f32, _Rint16_rtn)(float16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i32_v2f32, _Rint2_sat_rte)(float2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i32_v3f32, _Rint3_sat_rte)(float3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i32_v4f32, _Rint4_sat_rte)(float4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i32_v8f32, _Rint8_sat_rte)(float8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i32_v16f32, _Rint16_sat_rte)(float16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i32_v2f32, _Rint2_sat_rtz)(float2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i32_v3f32, _Rint3_sat_rtz)(float3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i32_v4f32, _Rint4_sat_rtz)(float4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i32_v8f32, _Rint8_sat_rtz)(float8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i32_v16f32, _Rint16_sat_rtz)(float16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i32_v2f32, _Rint2_sat_rtp)(float2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i32_v3f32, _Rint3_sat_rtp)(float3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i32_v4f32, _Rint4_sat_rtp)(float4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i32_v8f32, _Rint8_sat_rtp)(float8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i32_v16f32, _Rint16_sat_rtp)(float16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i32_v2f32, _Rint2_sat_rtn)(float2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i32_v3f32, _Rint3_sat_rtn)(float3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i32_v4f32, _Rint4_sat_rtn)(float4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i32_v8f32, _Rint8_sat_rtn)(float8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i32_v16f32, _Rint16_sat_rtn)(float16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v2i32_v2f16, _Rint2)(half2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v3i32_v3f16, _Rint3)(half3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v4i32_v4f16, _Rint4)(half4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v8i32_v8f16, _Rint8)(half8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i32_v16f16, _Rint16)(half16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v2i32_v2f16, _Rint2_sat)(half2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v3i32_v3f16, _Rint3_sat)(half3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v4i32_v4f16, _Rint4_sat)(half4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v8i32_v8f16, _Rint8_sat)(half8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v16i32_v16f16, _Rint16_sat)(half16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v2i32_v2f16, _Rint2_rte)(half2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v3i32_v3f16, _Rint3_rte)(half3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v4i32_v4f16, _Rint4_rte)(half4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v8i32_v8f16, _Rint8_rte)(half8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v16i32_v16f16, _Rint16_rte)(half16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i32_v2f16, _Rint2_rtz)(half2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i32_v3f16, _Rint3_rtz)(half3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i32_v4f16, _Rint4_rtz)(half4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i32_v8f16, _Rint8_rtz)(half8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i32_v16f16, _Rint16_rtz)(half16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v2i32_v2f16, _Rint2_rtp)(half2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v3i32_v3f16, _Rint3_rtp)(half3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v4i32_v4f16, _Rint4_rtp)(half4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v8i32_v8f16, _Rint8_rtp)(half8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v16i32_v16f16, _Rint16_rtp)(half16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v2i32_v2f16, _Rint2_rtn)(half2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v3i32_v3f16, _Rint3_rtn)(half3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v4i32_v4f16, _Rint4_rtn)(half4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v8i32_v8f16, _Rint8_rtn)(half8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v16i32_v16f16, _Rint16_rtn)(half16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i32_v2f16, _Rint2_sat_rte)(half2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i32_v3f16, _Rint3_sat_rte)(half3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i32_v4f16, _Rint4_sat_rte)(half4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i32_v8f16, _Rint8_sat_rte)(half8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i32_v16f16, _Rint16_sat_rte)(half16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i32_v2f16, _Rint2_sat_rtz)(half2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i32_v3f16, _Rint3_sat_rtz)(half3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i32_v4f16, _Rint4_sat_rtz)(half4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i32_v8f16, _Rint8_sat_rtz)(half8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i32_v16f16, _Rint16_sat_rtz)(half16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i32_v2f16, _Rint2_sat_rtp)(half2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i32_v3f16, _Rint3_sat_rtp)(half3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i32_v4f16, _Rint4_sat_rtp)(half4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i32_v8f16, _Rint8_sat_rtp)(half8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i32_v16f16, _Rint16_sat_rtp)(half16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i32_v2f16, _Rint2_sat_rtn)(half2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i32_v3f16, _Rint3_sat_rtn)(half3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i32_v4f16, _Rint4_sat_rtn)(half4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i32_v8f16, _Rint8_sat_rtn)(half8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i32_v16f16, _Rint16_sat_rtn)(half16 x);
#if defined(cl_khr_fp64)
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v2i32_v2f64, _Rint2)(double2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v3i32_v3f64, _Rint3)(double3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v4i32_v4f64, _Rint4)(double4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v8i32_v8f64, _Rint8)(double8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i32_v16f64, _Rint16)(double16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v2i32_v2f64, _Rint2_sat)(double2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v3i32_v3f64, _Rint3_sat)(double3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v4i32_v4f64, _Rint4_sat)(double4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v8i32_v8f64, _Rint8_sat)(double8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v16i32_v16f64, _Rint16_sat)(double16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v2i32_v2f64, _Rint2_rte)(double2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v3i32_v3f64, _Rint3_rte)(double3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v4i32_v4f64, _Rint4_rte)(double4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v8i32_v8f64, _Rint8_rte)(double8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v16i32_v16f64, _Rint16_rte)(double16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i32_v2f64, _Rint2_rtz)(double2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i32_v3f64, _Rint3_rtz)(double3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i32_v4f64, _Rint4_rtz)(double4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i32_v8f64, _Rint8_rtz)(double8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i32_v16f64, _Rint16_rtz)(double16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v2i32_v2f64, _Rint2_rtp)(double2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v3i32_v3f64, _Rint3_rtp)(double3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v4i32_v4f64, _Rint4_rtp)(double4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v8i32_v8f64, _Rint8_rtp)(double8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v16i32_v16f64, _Rint16_rtp)(double16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v2i32_v2f64, _Rint2_rtn)(double2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v3i32_v3f64, _Rint3_rtn)(double3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v4i32_v4f64, _Rint4_rtn)(double4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v8i32_v8f64, _Rint8_rtn)(double8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v16i32_v16f64, _Rint16_rtn)(double16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i32_v2f64, _Rint2_sat_rte)(double2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i32_v3f64, _Rint3_sat_rte)(double3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i32_v4f64, _Rint4_sat_rte)(double4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i32_v8f64, _Rint8_sat_rte)(double8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i32_v16f64, _Rint16_sat_rte)(double16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i32_v2f64, _Rint2_sat_rtz)(double2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i32_v3f64, _Rint3_sat_rtz)(double3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i32_v4f64, _Rint4_sat_rtz)(double4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i32_v8f64, _Rint8_sat_rtz)(double8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i32_v16f64, _Rint16_sat_rtz)(double16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i32_v2f64, _Rint2_sat_rtp)(double2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i32_v3f64, _Rint3_sat_rtp)(double3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i32_v4f64, _Rint4_sat_rtp)(double4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i32_v8f64, _Rint8_sat_rtp)(double8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i32_v16f64, _Rint16_sat_rtp)(double16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i32_v2f64, _Rint2_sat_rtn)(double2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i32_v3f64, _Rint3_sat_rtn)(double3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i32_v4f64, _Rint4_sat_rtn)(double4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i32_v8f64, _Rint8_sat_rtn)(double8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i32_v16f64, _Rint16_sat_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v2i64_v2f32, _Rlong2)(float2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v3i64_v3f32, _Rlong3)(float3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v4i64_v4f32, _Rlong4)(float4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v8i64_v8f32, _Rlong8)(float8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i64_v16f32, _Rlong16)(float16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v2i64_v2f32, _Rlong2_sat)(float2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v3i64_v3f32, _Rlong3_sat)(float3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v4i64_v4f32, _Rlong4_sat)(float4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v8i64_v8f32, _Rlong8_sat)(float8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v16i64_v16f32, _Rlong16_sat)(float16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v2i64_v2f32, _Rlong2_rte)(float2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v3i64_v3f32, _Rlong3_rte)(float3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v4i64_v4f32, _Rlong4_rte)(float4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v8i64_v8f32, _Rlong8_rte)(float8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v16i64_v16f32, _Rlong16_rte)(float16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i64_v2f32, _Rlong2_rtz)(float2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i64_v3f32, _Rlong3_rtz)(float3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i64_v4f32, _Rlong4_rtz)(float4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i64_v8f32, _Rlong8_rtz)(float8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i64_v16f32, _Rlong16_rtz)(float16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v2i64_v2f32, _Rlong2_rtp)(float2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v3i64_v3f32, _Rlong3_rtp)(float3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v4i64_v4f32, _Rlong4_rtp)(float4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v8i64_v8f32, _Rlong8_rtp)(float8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v16i64_v16f32, _Rlong16_rtp)(float16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v2i64_v2f32, _Rlong2_rtn)(float2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v3i64_v3f32, _Rlong3_rtn)(float3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v4i64_v4f32, _Rlong4_rtn)(float4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v8i64_v8f32, _Rlong8_rtn)(float8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v16i64_v16f32, _Rlong16_rtn)(float16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i64_v2f32, _Rlong2_sat_rte)(float2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i64_v3f32, _Rlong3_sat_rte)(float3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i64_v4f32, _Rlong4_sat_rte)(float4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i64_v8f32, _Rlong8_sat_rte)(float8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i64_v16f32, _Rlong16_sat_rte)(float16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i64_v2f32, _Rlong2_sat_rtz)(float2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i64_v3f32, _Rlong3_sat_rtz)(float3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i64_v4f32, _Rlong4_sat_rtz)(float4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i64_v8f32, _Rlong8_sat_rtz)(float8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i64_v16f32, _Rlong16_sat_rtz)(float16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i64_v2f32, _Rlong2_sat_rtp)(float2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i64_v3f32, _Rlong3_sat_rtp)(float3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i64_v4f32, _Rlong4_sat_rtp)(float4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i64_v8f32, _Rlong8_sat_rtp)(float8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i64_v16f32, _Rlong16_sat_rtp)(float16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i64_v2f32, _Rlong2_sat_rtn)(float2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i64_v3f32, _Rlong3_sat_rtn)(float3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i64_v4f32, _Rlong4_sat_rtn)(float4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i64_v8f32, _Rlong8_sat_rtn)(float8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i64_v16f32, _Rlong16_sat_rtn)(float16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v2i64_v2f16, _Rlong2)(half2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v3i64_v3f16, _Rlong3)(half3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v4i64_v4f16, _Rlong4)(half4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v8i64_v8f16, _Rlong8)(half8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i64_v16f16, _Rlong16)(half16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v2i64_v2f16, _Rlong2_sat)(half2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v3i64_v3f16, _Rlong3_sat)(half3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v4i64_v4f16, _Rlong4_sat)(half4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v8i64_v8f16, _Rlong8_sat)(half8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v16i64_v16f16, _Rlong16_sat)(half16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v2i64_v2f16, _Rlong2_rte)(half2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v3i64_v3f16, _Rlong3_rte)(half3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v4i64_v4f16, _Rlong4_rte)(half4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v8i64_v8f16, _Rlong8_rte)(half8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v16i64_v16f16, _Rlong16_rte)(half16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i64_v2f16, _Rlong2_rtz)(half2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i64_v3f16, _Rlong3_rtz)(half3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i64_v4f16, _Rlong4_rtz)(half4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i64_v8f16, _Rlong8_rtz)(half8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i64_v16f16, _Rlong16_rtz)(half16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v2i64_v2f16, _Rlong2_rtp)(half2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v3i64_v3f16, _Rlong3_rtp)(half3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v4i64_v4f16, _Rlong4_rtp)(half4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v8i64_v8f16, _Rlong8_rtp)(half8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v16i64_v16f16, _Rlong16_rtp)(half16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v2i64_v2f16, _Rlong2_rtn)(half2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v3i64_v3f16, _Rlong3_rtn)(half3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v4i64_v4f16, _Rlong4_rtn)(half4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v8i64_v8f16, _Rlong8_rtn)(half8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v16i64_v16f16, _Rlong16_rtn)(half16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i64_v2f16, _Rlong2_sat_rte)(half2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i64_v3f16, _Rlong3_sat_rte)(half3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i64_v4f16, _Rlong4_sat_rte)(half4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i64_v8f16, _Rlong8_sat_rte)(half8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i64_v16f16, _Rlong16_sat_rte)(half16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i64_v2f16, _Rlong2_sat_rtz)(half2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i64_v3f16, _Rlong3_sat_rtz)(half3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i64_v4f16, _Rlong4_sat_rtz)(half4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i64_v8f16, _Rlong8_sat_rtz)(half8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i64_v16f16, _Rlong16_sat_rtz)(half16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i64_v2f16, _Rlong2_sat_rtp)(half2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i64_v3f16, _Rlong3_sat_rtp)(half3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i64_v4f16, _Rlong4_sat_rtp)(half4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i64_v8f16, _Rlong8_sat_rtp)(half8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i64_v16f16, _Rlong16_sat_rtp)(half16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i64_v2f16, _Rlong2_sat_rtn)(half2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i64_v3f16, _Rlong3_sat_rtn)(half3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i64_v4f16, _Rlong4_sat_rtn)(half4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i64_v8f16, _Rlong8_sat_rtn)(half8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i64_v16f16, _Rlong16_sat_rtn)(half16 x);
#if defined(cl_khr_fp64)
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v2i64_v2f64, _Rlong2)(double2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v3i64_v3f64, _Rlong3)(double3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v4i64_v4f64, _Rlong4)(double4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v8i64_v8f64, _Rlong8)(double8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _v16i64_v16f64, _Rlong16)(double16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v2i64_v2f64, _Rlong2_sat)(double2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v3i64_v3f64, _Rlong3_sat)(double3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v4i64_v4f64, _Rlong4_sat)(double4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v8i64_v8f64, _Rlong8_sat)(double8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_v16i64_v16f64, _Rlong16_sat)(double16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v2i64_v2f64, _Rlong2_rte)(double2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v3i64_v3f64, _Rlong3_rte)(double3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v4i64_v4f64, _Rlong4_rte)(double4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v8i64_v8f64, _Rlong8_rte)(double8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_v16i64_v16f64, _Rlong16_rte)(double16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v2i64_v2f64, _Rlong2_rtz)(double2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v3i64_v3f64, _Rlong3_rtz)(double3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v4i64_v4f64, _Rlong4_rtz)(double4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v8i64_v8f64, _Rlong8_rtz)(double8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_v16i64_v16f64, _Rlong16_rtz)(double16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v2i64_v2f64, _Rlong2_rtp)(double2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v3i64_v3f64, _Rlong3_rtp)(double3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v4i64_v4f64, _Rlong4_rtp)(double4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v8i64_v8f64, _Rlong8_rtp)(double8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_v16i64_v16f64, _Rlong16_rtp)(double16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v2i64_v2f64, _Rlong2_rtn)(double2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v3i64_v3f64, _Rlong3_rtn)(double3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v4i64_v4f64, _Rlong4_rtn)(double4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v8i64_v8f64, _Rlong8_rtn)(double8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_v16i64_v16f64, _Rlong16_rtn)(double16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v2i64_v2f64, _Rlong2_sat_rte)(double2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v3i64_v3f64, _Rlong3_sat_rte)(double3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v4i64_v4f64, _Rlong4_sat_rte)(double4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v8i64_v8f64, _Rlong8_sat_rte)(double8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_v16i64_v16f64, _Rlong16_sat_rte)(double16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v2i64_v2f64, _Rlong2_sat_rtz)(double2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v3i64_v3f64, _Rlong3_sat_rtz)(double3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v4i64_v4f64, _Rlong4_sat_rtz)(double4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v8i64_v8f64, _Rlong8_sat_rtz)(double8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_v16i64_v16f64, _Rlong16_sat_rtz)(double16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v2i64_v2f64, _Rlong2_sat_rtp)(double2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v3i64_v3f64, _Rlong3_sat_rtp)(double3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v4i64_v4f64, _Rlong4_sat_rtp)(double4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v8i64_v8f64, _Rlong8_sat_rtp)(double8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_v16i64_v16f64, _Rlong16_sat_rtp)(double16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v2i64_v2f64, _Rlong2_sat_rtn)(double2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v3i64_v3f64, _Rlong3_sat_rtn)(double3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v4i64_v4f64, _Rlong4_sat_rtn)(double4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v8i64_v8f64, _Rlong8_sat_rtn)(double8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_v16i64_v16f64, _Rlong16_sat_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i8_v2f32, _Ruchar2)(float2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i8_v3f32, _Ruchar3)(float3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i8_v4f32, _Ruchar4)(float4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i8_v8f32, _Ruchar8)(float8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i8_v16f32, _Ruchar16)(float16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v2i8_v2f32, _Ruchar2_sat)(float2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v3i8_v3f32, _Ruchar3_sat)(float3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v4i8_v4f32, _Ruchar4_sat)(float4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v8i8_v8f32, _Ruchar8_sat)(float8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v16i8_v16f32, _Ruchar16_sat)(float16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v2i8_v2f32, _Ruchar2_rte)(float2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v3i8_v3f32, _Ruchar3_rte)(float3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v4i8_v4f32, _Ruchar4_rte)(float4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v8i8_v8f32, _Ruchar8_rte)(float8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v16i8_v16f32, _Ruchar16_rte)(float16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i8_v2f32, _Ruchar2_rtz)(float2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i8_v3f32, _Ruchar3_rtz)(float3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i8_v4f32, _Ruchar4_rtz)(float4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i8_v8f32, _Ruchar8_rtz)(float8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i8_v16f32, _Ruchar16_rtz)(float16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v2i8_v2f32, _Ruchar2_rtp)(float2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v3i8_v3f32, _Ruchar3_rtp)(float3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v4i8_v4f32, _Ruchar4_rtp)(float4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v8i8_v8f32, _Ruchar8_rtp)(float8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v16i8_v16f32, _Ruchar16_rtp)(float16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v2i8_v2f32, _Ruchar2_rtn)(float2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v3i8_v3f32, _Ruchar3_rtn)(float3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v4i8_v4f32, _Ruchar4_rtn)(float4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v8i8_v8f32, _Ruchar8_rtn)(float8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v16i8_v16f32, _Ruchar16_rtn)(float16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i8_v2f32, _Ruchar2_sat_rte)(float2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i8_v3f32, _Ruchar3_sat_rte)(float3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i8_v4f32, _Ruchar4_sat_rte)(float4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i8_v8f32, _Ruchar8_sat_rte)(float8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i8_v16f32, _Ruchar16_sat_rte)(float16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i8_v2f32, _Ruchar2_sat_rtz)(float2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i8_v3f32, _Ruchar3_sat_rtz)(float3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i8_v4f32, _Ruchar4_sat_rtz)(float4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i8_v8f32, _Ruchar8_sat_rtz)(float8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i8_v16f32, _Ruchar16_sat_rtz)(float16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i8_v2f32, _Ruchar2_sat_rtp)(float2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i8_v3f32, _Ruchar3_sat_rtp)(float3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i8_v4f32, _Ruchar4_sat_rtp)(float4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i8_v8f32, _Ruchar8_sat_rtp)(float8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i8_v16f32, _Ruchar16_sat_rtp)(float16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i8_v2f32, _Ruchar2_sat_rtn)(float2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i8_v3f32, _Ruchar3_sat_rtn)(float3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i8_v4f32, _Ruchar4_sat_rtn)(float4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i8_v8f32, _Ruchar8_sat_rtn)(float8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i8_v16f32, _Ruchar16_sat_rtn)(float16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i8_v2f16, _Ruchar2)(half2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i8_v3f16, _Ruchar3)(half3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i8_v4f16, _Ruchar4)(half4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i8_v8f16, _Ruchar8)(half8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i8_v16f16, _Ruchar16)(half16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v2i8_v2f16, _Ruchar2_sat)(half2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v3i8_v3f16, _Ruchar3_sat)(half3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v4i8_v4f16, _Ruchar4_sat)(half4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v8i8_v8f16, _Ruchar8_sat)(half8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v16i8_v16f16, _Ruchar16_sat)(half16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v2i8_v2f16, _Ruchar2_rte)(half2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v3i8_v3f16, _Ruchar3_rte)(half3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v4i8_v4f16, _Ruchar4_rte)(half4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v8i8_v8f16, _Ruchar8_rte)(half8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v16i8_v16f16, _Ruchar16_rte)(half16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i8_v2f16, _Ruchar2_rtz)(half2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i8_v3f16, _Ruchar3_rtz)(half3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i8_v4f16, _Ruchar4_rtz)(half4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i8_v8f16, _Ruchar8_rtz)(half8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i8_v16f16, _Ruchar16_rtz)(half16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v2i8_v2f16, _Ruchar2_rtp)(half2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v3i8_v3f16, _Ruchar3_rtp)(half3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v4i8_v4f16, _Ruchar4_rtp)(half4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v8i8_v8f16, _Ruchar8_rtp)(half8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v16i8_v16f16, _Ruchar16_rtp)(half16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v2i8_v2f16, _Ruchar2_rtn)(half2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v3i8_v3f16, _Ruchar3_rtn)(half3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v4i8_v4f16, _Ruchar4_rtn)(half4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v8i8_v8f16, _Ruchar8_rtn)(half8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v16i8_v16f16, _Ruchar16_rtn)(half16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i8_v2f16, _Ruchar2_sat_rte)(half2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i8_v3f16, _Ruchar3_sat_rte)(half3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i8_v4f16, _Ruchar4_sat_rte)(half4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i8_v8f16, _Ruchar8_sat_rte)(half8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i8_v16f16, _Ruchar16_sat_rte)(half16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i8_v2f16, _Ruchar2_sat_rtz)(half2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i8_v3f16, _Ruchar3_sat_rtz)(half3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i8_v4f16, _Ruchar4_sat_rtz)(half4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i8_v8f16, _Ruchar8_sat_rtz)(half8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i8_v16f16, _Ruchar16_sat_rtz)(half16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i8_v2f16, _Ruchar2_sat_rtp)(half2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i8_v3f16, _Ruchar3_sat_rtp)(half3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i8_v4f16, _Ruchar4_sat_rtp)(half4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i8_v8f16, _Ruchar8_sat_rtp)(half8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i8_v16f16, _Ruchar16_sat_rtp)(half16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i8_v2f16, _Ruchar2_sat_rtn)(half2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i8_v3f16, _Ruchar3_sat_rtn)(half3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i8_v4f16, _Ruchar4_sat_rtn)(half4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i8_v8f16, _Ruchar8_sat_rtn)(half8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i8_v16f16, _Ruchar16_sat_rtn)(half16 x);
#if defined(cl_khr_fp64)
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i8_v2f64, _Ruchar2)(double2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i8_v3f64, _Ruchar3)(double3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i8_v4f64, _Ruchar4)(double4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i8_v8f64, _Ruchar8)(double8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i8_v16f64, _Ruchar16)(double16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v2i8_v2f64, _Ruchar2_sat)(double2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v3i8_v3f64, _Ruchar3_sat)(double3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v4i8_v4f64, _Ruchar4_sat)(double4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v8i8_v8f64, _Ruchar8_sat)(double8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v16i8_v16f64, _Ruchar16_sat)(double16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v2i8_v2f64, _Ruchar2_rte)(double2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v3i8_v3f64, _Ruchar3_rte)(double3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v4i8_v4f64, _Ruchar4_rte)(double4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v8i8_v8f64, _Ruchar8_rte)(double8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v16i8_v16f64, _Ruchar16_rte)(double16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i8_v2f64, _Ruchar2_rtz)(double2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i8_v3f64, _Ruchar3_rtz)(double3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i8_v4f64, _Ruchar4_rtz)(double4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i8_v8f64, _Ruchar8_rtz)(double8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i8_v16f64, _Ruchar16_rtz)(double16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v2i8_v2f64, _Ruchar2_rtp)(double2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v3i8_v3f64, _Ruchar3_rtp)(double3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v4i8_v4f64, _Ruchar4_rtp)(double4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v8i8_v8f64, _Ruchar8_rtp)(double8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v16i8_v16f64, _Ruchar16_rtp)(double16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v2i8_v2f64, _Ruchar2_rtn)(double2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v3i8_v3f64, _Ruchar3_rtn)(double3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v4i8_v4f64, _Ruchar4_rtn)(double4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v8i8_v8f64, _Ruchar8_rtn)(double8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v16i8_v16f64, _Ruchar16_rtn)(double16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i8_v2f64, _Ruchar2_sat_rte)(double2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i8_v3f64, _Ruchar3_sat_rte)(double3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i8_v4f64, _Ruchar4_sat_rte)(double4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i8_v8f64, _Ruchar8_sat_rte)(double8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i8_v16f64, _Ruchar16_sat_rte)(double16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i8_v2f64, _Ruchar2_sat_rtz)(double2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i8_v3f64, _Ruchar3_sat_rtz)(double3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i8_v4f64, _Ruchar4_sat_rtz)(double4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i8_v8f64, _Ruchar8_sat_rtz)(double8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i8_v16f64, _Ruchar16_sat_rtz)(double16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i8_v2f64, _Ruchar2_sat_rtp)(double2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i8_v3f64, _Ruchar3_sat_rtp)(double3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i8_v4f64, _Ruchar4_sat_rtp)(double4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i8_v8f64, _Ruchar8_sat_rtp)(double8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i8_v16f64, _Ruchar16_sat_rtp)(double16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i8_v2f64, _Ruchar2_sat_rtn)(double2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i8_v3f64, _Ruchar3_sat_rtn)(double3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i8_v4f64, _Ruchar4_sat_rtn)(double4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i8_v8f64, _Ruchar8_sat_rtn)(double8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i8_v16f64, _Ruchar16_sat_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i16_v2f32, _Rushort2)(float2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i16_v3f32, _Rushort3)(float3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i16_v4f32, _Rushort4)(float4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i16_v8f32, _Rushort8)(float8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i16_v16f32, _Rushort16)(float16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v2i16_v2f32, _Rushort2_sat)(float2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v3i16_v3f32, _Rushort3_sat)(float3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v4i16_v4f32, _Rushort4_sat)(float4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v8i16_v8f32, _Rushort8_sat)(float8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v16i16_v16f32, _Rushort16_sat)(float16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v2i16_v2f32, _Rushort2_rte)(float2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v3i16_v3f32, _Rushort3_rte)(float3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v4i16_v4f32, _Rushort4_rte)(float4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v8i16_v8f32, _Rushort8_rte)(float8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v16i16_v16f32, _Rushort16_rte)(float16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i16_v2f32, _Rushort2_rtz)(float2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i16_v3f32, _Rushort3_rtz)(float3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i16_v4f32, _Rushort4_rtz)(float4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i16_v8f32, _Rushort8_rtz)(float8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i16_v16f32, _Rushort16_rtz)(float16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v2i16_v2f32, _Rushort2_rtp)(float2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v3i16_v3f32, _Rushort3_rtp)(float3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v4i16_v4f32, _Rushort4_rtp)(float4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v8i16_v8f32, _Rushort8_rtp)(float8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v16i16_v16f32, _Rushort16_rtp)(float16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v2i16_v2f32, _Rushort2_rtn)(float2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v3i16_v3f32, _Rushort3_rtn)(float3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v4i16_v4f32, _Rushort4_rtn)(float4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v8i16_v8f32, _Rushort8_rtn)(float8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v16i16_v16f32, _Rushort16_rtn)(float16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i16_v2f32, _Rushort2_sat_rte)(float2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i16_v3f32, _Rushort3_sat_rte)(float3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i16_v4f32, _Rushort4_sat_rte)(float4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i16_v8f32, _Rushort8_sat_rte)(float8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i16_v16f32, _Rushort16_sat_rte)(float16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i16_v2f32, _Rushort2_sat_rtz)(float2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i16_v3f32, _Rushort3_sat_rtz)(float3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i16_v4f32, _Rushort4_sat_rtz)(float4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i16_v8f32, _Rushort8_sat_rtz)(float8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i16_v16f32, _Rushort16_sat_rtz)(float16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i16_v2f32, _Rushort2_sat_rtp)(float2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i16_v3f32, _Rushort3_sat_rtp)(float3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i16_v4f32, _Rushort4_sat_rtp)(float4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i16_v8f32, _Rushort8_sat_rtp)(float8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i16_v16f32, _Rushort16_sat_rtp)(float16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i16_v2f32, _Rushort2_sat_rtn)(float2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i16_v3f32, _Rushort3_sat_rtn)(float3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i16_v4f32, _Rushort4_sat_rtn)(float4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i16_v8f32, _Rushort8_sat_rtn)(float8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i16_v16f32, _Rushort16_sat_rtn)(float16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i16_v2f16, _Rushort2)(half2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i16_v3f16, _Rushort3)(half3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i16_v4f16, _Rushort4)(half4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i16_v8f16, _Rushort8)(half8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i16_v16f16, _Rushort16)(half16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v2i16_v2f16, _Rushort2_sat)(half2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v3i16_v3f16, _Rushort3_sat)(half3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v4i16_v4f16, _Rushort4_sat)(half4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v8i16_v8f16, _Rushort8_sat)(half8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v16i16_v16f16, _Rushort16_sat)(half16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v2i16_v2f16, _Rushort2_rte)(half2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v3i16_v3f16, _Rushort3_rte)(half3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v4i16_v4f16, _Rushort4_rte)(half4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v8i16_v8f16, _Rushort8_rte)(half8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v16i16_v16f16, _Rushort16_rte)(half16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i16_v2f16, _Rushort2_rtz)(half2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i16_v3f16, _Rushort3_rtz)(half3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i16_v4f16, _Rushort4_rtz)(half4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i16_v8f16, _Rushort8_rtz)(half8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i16_v16f16, _Rushort16_rtz)(half16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v2i16_v2f16, _Rushort2_rtp)(half2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v3i16_v3f16, _Rushort3_rtp)(half3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v4i16_v4f16, _Rushort4_rtp)(half4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v8i16_v8f16, _Rushort8_rtp)(half8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v16i16_v16f16, _Rushort16_rtp)(half16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v2i16_v2f16, _Rushort2_rtn)(half2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v3i16_v3f16, _Rushort3_rtn)(half3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v4i16_v4f16, _Rushort4_rtn)(half4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v8i16_v8f16, _Rushort8_rtn)(half8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v16i16_v16f16, _Rushort16_rtn)(half16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i16_v2f16, _Rushort2_sat_rte)(half2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i16_v3f16, _Rushort3_sat_rte)(half3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i16_v4f16, _Rushort4_sat_rte)(half4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i16_v8f16, _Rushort8_sat_rte)(half8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i16_v16f16, _Rushort16_sat_rte)(half16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i16_v2f16, _Rushort2_sat_rtz)(half2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i16_v3f16, _Rushort3_sat_rtz)(half3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i16_v4f16, _Rushort4_sat_rtz)(half4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i16_v8f16, _Rushort8_sat_rtz)(half8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i16_v16f16, _Rushort16_sat_rtz)(half16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i16_v2f16, _Rushort2_sat_rtp)(half2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i16_v3f16, _Rushort3_sat_rtp)(half3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i16_v4f16, _Rushort4_sat_rtp)(half4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i16_v8f16, _Rushort8_sat_rtp)(half8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i16_v16f16, _Rushort16_sat_rtp)(half16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i16_v2f16, _Rushort2_sat_rtn)(half2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i16_v3f16, _Rushort3_sat_rtn)(half3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i16_v4f16, _Rushort4_sat_rtn)(half4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i16_v8f16, _Rushort8_sat_rtn)(half8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i16_v16f16, _Rushort16_sat_rtn)(half16 x);
#if defined(cl_khr_fp64)
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i16_v2f64, _Rushort2)(double2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i16_v3f64, _Rushort3)(double3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i16_v4f64, _Rushort4)(double4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i16_v8f64, _Rushort8)(double8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i16_v16f64, _Rushort16)(double16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v2i16_v2f64, _Rushort2_sat)(double2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v3i16_v3f64, _Rushort3_sat)(double3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v4i16_v4f64, _Rushort4_sat)(double4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v8i16_v8f64, _Rushort8_sat)(double8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v16i16_v16f64, _Rushort16_sat)(double16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v2i16_v2f64, _Rushort2_rte)(double2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v3i16_v3f64, _Rushort3_rte)(double3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v4i16_v4f64, _Rushort4_rte)(double4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v8i16_v8f64, _Rushort8_rte)(double8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v16i16_v16f64, _Rushort16_rte)(double16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i16_v2f64, _Rushort2_rtz)(double2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i16_v3f64, _Rushort3_rtz)(double3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i16_v4f64, _Rushort4_rtz)(double4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i16_v8f64, _Rushort8_rtz)(double8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i16_v16f64, _Rushort16_rtz)(double16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v2i16_v2f64, _Rushort2_rtp)(double2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v3i16_v3f64, _Rushort3_rtp)(double3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v4i16_v4f64, _Rushort4_rtp)(double4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v8i16_v8f64, _Rushort8_rtp)(double8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v16i16_v16f64, _Rushort16_rtp)(double16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v2i16_v2f64, _Rushort2_rtn)(double2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v3i16_v3f64, _Rushort3_rtn)(double3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v4i16_v4f64, _Rushort4_rtn)(double4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v8i16_v8f64, _Rushort8_rtn)(double8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v16i16_v16f64, _Rushort16_rtn)(double16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i16_v2f64, _Rushort2_sat_rte)(double2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i16_v3f64, _Rushort3_sat_rte)(double3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i16_v4f64, _Rushort4_sat_rte)(double4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i16_v8f64, _Rushort8_sat_rte)(double8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i16_v16f64, _Rushort16_sat_rte)(double16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i16_v2f64, _Rushort2_sat_rtz)(double2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i16_v3f64, _Rushort3_sat_rtz)(double3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i16_v4f64, _Rushort4_sat_rtz)(double4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i16_v8f64, _Rushort8_sat_rtz)(double8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i16_v16f64, _Rushort16_sat_rtz)(double16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i16_v2f64, _Rushort2_sat_rtp)(double2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i16_v3f64, _Rushort3_sat_rtp)(double3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i16_v4f64, _Rushort4_sat_rtp)(double4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i16_v8f64, _Rushort8_sat_rtp)(double8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i16_v16f64, _Rushort16_sat_rtp)(double16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i16_v2f64, _Rushort2_sat_rtn)(double2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i16_v3f64, _Rushort3_sat_rtn)(double3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i16_v4f64, _Rushort4_sat_rtn)(double4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i16_v8f64, _Rushort8_sat_rtn)(double8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i16_v16f64, _Rushort16_sat_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i32_v2f32, _Ruint2)(float2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i32_v3f32, _Ruint3)(float3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i32_v4f32, _Ruint4)(float4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i32_v8f32, _Ruint8)(float8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i32_v16f32, _Ruint16)(float16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v2i32_v2f32, _Ruint2_sat)(float2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v3i32_v3f32, _Ruint3_sat)(float3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v4i32_v4f32, _Ruint4_sat)(float4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v8i32_v8f32, _Ruint8_sat)(float8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v16i32_v16f32, _Ruint16_sat)(float16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v2i32_v2f32, _Ruint2_rte)(float2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v3i32_v3f32, _Ruint3_rte)(float3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v4i32_v4f32, _Ruint4_rte)(float4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v8i32_v8f32, _Ruint8_rte)(float8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v16i32_v16f32, _Ruint16_rte)(float16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i32_v2f32, _Ruint2_rtz)(float2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i32_v3f32, _Ruint3_rtz)(float3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i32_v4f32, _Ruint4_rtz)(float4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i32_v8f32, _Ruint8_rtz)(float8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i32_v16f32, _Ruint16_rtz)(float16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v2i32_v2f32, _Ruint2_rtp)(float2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v3i32_v3f32, _Ruint3_rtp)(float3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v4i32_v4f32, _Ruint4_rtp)(float4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v8i32_v8f32, _Ruint8_rtp)(float8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v16i32_v16f32, _Ruint16_rtp)(float16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v2i32_v2f32, _Ruint2_rtn)(float2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v3i32_v3f32, _Ruint3_rtn)(float3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v4i32_v4f32, _Ruint4_rtn)(float4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v8i32_v8f32, _Ruint8_rtn)(float8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v16i32_v16f32, _Ruint16_rtn)(float16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i32_v2f32, _Ruint2_sat_rte)(float2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i32_v3f32, _Ruint3_sat_rte)(float3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i32_v4f32, _Ruint4_sat_rte)(float4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i32_v8f32, _Ruint8_sat_rte)(float8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i32_v16f32, _Ruint16_sat_rte)(float16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i32_v2f32, _Ruint2_sat_rtz)(float2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i32_v3f32, _Ruint3_sat_rtz)(float3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i32_v4f32, _Ruint4_sat_rtz)(float4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i32_v8f32, _Ruint8_sat_rtz)(float8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i32_v16f32, _Ruint16_sat_rtz)(float16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i32_v2f32, _Ruint2_sat_rtp)(float2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i32_v3f32, _Ruint3_sat_rtp)(float3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i32_v4f32, _Ruint4_sat_rtp)(float4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i32_v8f32, _Ruint8_sat_rtp)(float8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i32_v16f32, _Ruint16_sat_rtp)(float16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i32_v2f32, _Ruint2_sat_rtn)(float2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i32_v3f32, _Ruint3_sat_rtn)(float3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i32_v4f32, _Ruint4_sat_rtn)(float4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i32_v8f32, _Ruint8_sat_rtn)(float8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i32_v16f32, _Ruint16_sat_rtn)(float16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i32_v2f16, _Ruint2)(half2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i32_v3f16, _Ruint3)(half3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i32_v4f16, _Ruint4)(half4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i32_v8f16, _Ruint8)(half8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i32_v16f16, _Ruint16)(half16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v2i32_v2f16, _Ruint2_sat)(half2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v3i32_v3f16, _Ruint3_sat)(half3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v4i32_v4f16, _Ruint4_sat)(half4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v8i32_v8f16, _Ruint8_sat)(half8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v16i32_v16f16, _Ruint16_sat)(half16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v2i32_v2f16, _Ruint2_rte)(half2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v3i32_v3f16, _Ruint3_rte)(half3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v4i32_v4f16, _Ruint4_rte)(half4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v8i32_v8f16, _Ruint8_rte)(half8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v16i32_v16f16, _Ruint16_rte)(half16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i32_v2f16, _Ruint2_rtz)(half2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i32_v3f16, _Ruint3_rtz)(half3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i32_v4f16, _Ruint4_rtz)(half4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i32_v8f16, _Ruint8_rtz)(half8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i32_v16f16, _Ruint16_rtz)(half16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v2i32_v2f16, _Ruint2_rtp)(half2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v3i32_v3f16, _Ruint3_rtp)(half3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v4i32_v4f16, _Ruint4_rtp)(half4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v8i32_v8f16, _Ruint8_rtp)(half8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v16i32_v16f16, _Ruint16_rtp)(half16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v2i32_v2f16, _Ruint2_rtn)(half2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v3i32_v3f16, _Ruint3_rtn)(half3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v4i32_v4f16, _Ruint4_rtn)(half4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v8i32_v8f16, _Ruint8_rtn)(half8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v16i32_v16f16, _Ruint16_rtn)(half16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i32_v2f16, _Ruint2_sat_rte)(half2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i32_v3f16, _Ruint3_sat_rte)(half3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i32_v4f16, _Ruint4_sat_rte)(half4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i32_v8f16, _Ruint8_sat_rte)(half8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i32_v16f16, _Ruint16_sat_rte)(half16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i32_v2f16, _Ruint2_sat_rtz)(half2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i32_v3f16, _Ruint3_sat_rtz)(half3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i32_v4f16, _Ruint4_sat_rtz)(half4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i32_v8f16, _Ruint8_sat_rtz)(half8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i32_v16f16, _Ruint16_sat_rtz)(half16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i32_v2f16, _Ruint2_sat_rtp)(half2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i32_v3f16, _Ruint3_sat_rtp)(half3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i32_v4f16, _Ruint4_sat_rtp)(half4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i32_v8f16, _Ruint8_sat_rtp)(half8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i32_v16f16, _Ruint16_sat_rtp)(half16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i32_v2f16, _Ruint2_sat_rtn)(half2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i32_v3f16, _Ruint3_sat_rtn)(half3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i32_v4f16, _Ruint4_sat_rtn)(half4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i32_v8f16, _Ruint8_sat_rtn)(half8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i32_v16f16, _Ruint16_sat_rtn)(half16 x);
#if defined(cl_khr_fp64)
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i32_v2f64, _Ruint2)(double2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i32_v3f64, _Ruint3)(double3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i32_v4f64, _Ruint4)(double4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i32_v8f64, _Ruint8)(double8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i32_v16f64, _Ruint16)(double16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v2i32_v2f64, _Ruint2_sat)(double2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v3i32_v3f64, _Ruint3_sat)(double3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v4i32_v4f64, _Ruint4_sat)(double4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v8i32_v8f64, _Ruint8_sat)(double8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v16i32_v16f64, _Ruint16_sat)(double16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v2i32_v2f64, _Ruint2_rte)(double2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v3i32_v3f64, _Ruint3_rte)(double3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v4i32_v4f64, _Ruint4_rte)(double4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v8i32_v8f64, _Ruint8_rte)(double8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v16i32_v16f64, _Ruint16_rte)(double16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i32_v2f64, _Ruint2_rtz)(double2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i32_v3f64, _Ruint3_rtz)(double3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i32_v4f64, _Ruint4_rtz)(double4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i32_v8f64, _Ruint8_rtz)(double8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i32_v16f64, _Ruint16_rtz)(double16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v2i32_v2f64, _Ruint2_rtp)(double2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v3i32_v3f64, _Ruint3_rtp)(double3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v4i32_v4f64, _Ruint4_rtp)(double4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v8i32_v8f64, _Ruint8_rtp)(double8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v16i32_v16f64, _Ruint16_rtp)(double16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v2i32_v2f64, _Ruint2_rtn)(double2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v3i32_v3f64, _Ruint3_rtn)(double3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v4i32_v4f64, _Ruint4_rtn)(double4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v8i32_v8f64, _Ruint8_rtn)(double8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v16i32_v16f64, _Ruint16_rtn)(double16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i32_v2f64, _Ruint2_sat_rte)(double2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i32_v3f64, _Ruint3_sat_rte)(double3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i32_v4f64, _Ruint4_sat_rte)(double4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i32_v8f64, _Ruint8_sat_rte)(double8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i32_v16f64, _Ruint16_sat_rte)(double16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i32_v2f64, _Ruint2_sat_rtz)(double2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i32_v3f64, _Ruint3_sat_rtz)(double3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i32_v4f64, _Ruint4_sat_rtz)(double4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i32_v8f64, _Ruint8_sat_rtz)(double8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i32_v16f64, _Ruint16_sat_rtz)(double16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i32_v2f64, _Ruint2_sat_rtp)(double2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i32_v3f64, _Ruint3_sat_rtp)(double3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i32_v4f64, _Ruint4_sat_rtp)(double4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i32_v8f64, _Ruint8_sat_rtp)(double8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i32_v16f64, _Ruint16_sat_rtp)(double16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i32_v2f64, _Ruint2_sat_rtn)(double2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i32_v3f64, _Ruint3_sat_rtn)(double3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i32_v4f64, _Ruint4_sat_rtn)(double4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i32_v8f64, _Ruint8_sat_rtn)(double8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i32_v16f64, _Ruint16_sat_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i64_v2f32, _Rulong2)(float2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i64_v3f32, _Rulong3)(float3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i64_v4f32, _Rulong4)(float4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i64_v8f32, _Rulong8)(float8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i64_v16f32, _Rulong16)(float16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v2i64_v2f32, _Rulong2_sat)(float2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v3i64_v3f32, _Rulong3_sat)(float3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v4i64_v4f32, _Rulong4_sat)(float4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v8i64_v8f32, _Rulong8_sat)(float8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v16i64_v16f32, _Rulong16_sat)(float16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v2i64_v2f32, _Rulong2_rte)(float2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v3i64_v3f32, _Rulong3_rte)(float3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v4i64_v4f32, _Rulong4_rte)(float4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v8i64_v8f32, _Rulong8_rte)(float8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v16i64_v16f32, _Rulong16_rte)(float16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i64_v2f32, _Rulong2_rtz)(float2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i64_v3f32, _Rulong3_rtz)(float3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i64_v4f32, _Rulong4_rtz)(float4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i64_v8f32, _Rulong8_rtz)(float8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i64_v16f32, _Rulong16_rtz)(float16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v2i64_v2f32, _Rulong2_rtp)(float2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v3i64_v3f32, _Rulong3_rtp)(float3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v4i64_v4f32, _Rulong4_rtp)(float4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v8i64_v8f32, _Rulong8_rtp)(float8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v16i64_v16f32, _Rulong16_rtp)(float16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v2i64_v2f32, _Rulong2_rtn)(float2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v3i64_v3f32, _Rulong3_rtn)(float3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v4i64_v4f32, _Rulong4_rtn)(float4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v8i64_v8f32, _Rulong8_rtn)(float8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v16i64_v16f32, _Rulong16_rtn)(float16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i64_v2f32, _Rulong2_sat_rte)(float2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i64_v3f32, _Rulong3_sat_rte)(float3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i64_v4f32, _Rulong4_sat_rte)(float4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i64_v8f32, _Rulong8_sat_rte)(float8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i64_v16f32, _Rulong16_sat_rte)(float16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i64_v2f32, _Rulong2_sat_rtz)(float2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i64_v3f32, _Rulong3_sat_rtz)(float3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i64_v4f32, _Rulong4_sat_rtz)(float4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i64_v8f32, _Rulong8_sat_rtz)(float8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i64_v16f32, _Rulong16_sat_rtz)(float16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i64_v2f32, _Rulong2_sat_rtp)(float2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i64_v3f32, _Rulong3_sat_rtp)(float3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i64_v4f32, _Rulong4_sat_rtp)(float4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i64_v8f32, _Rulong8_sat_rtp)(float8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i64_v16f32, _Rulong16_sat_rtp)(float16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i64_v2f32, _Rulong2_sat_rtn)(float2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i64_v3f32, _Rulong3_sat_rtn)(float3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i64_v4f32, _Rulong4_sat_rtn)(float4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i64_v8f32, _Rulong8_sat_rtn)(float8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i64_v16f32, _Rulong16_sat_rtn)(float16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i64_v2f16, _Rulong2)(half2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i64_v3f16, _Rulong3)(half3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i64_v4f16, _Rulong4)(half4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i64_v8f16, _Rulong8)(half8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i64_v16f16, _Rulong16)(half16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v2i64_v2f16, _Rulong2_sat)(half2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v3i64_v3f16, _Rulong3_sat)(half3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v4i64_v4f16, _Rulong4_sat)(half4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v8i64_v8f16, _Rulong8_sat)(half8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v16i64_v16f16, _Rulong16_sat)(half16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v2i64_v2f16, _Rulong2_rte)(half2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v3i64_v3f16, _Rulong3_rte)(half3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v4i64_v4f16, _Rulong4_rte)(half4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v8i64_v8f16, _Rulong8_rte)(half8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v16i64_v16f16, _Rulong16_rte)(half16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i64_v2f16, _Rulong2_rtz)(half2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i64_v3f16, _Rulong3_rtz)(half3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i64_v4f16, _Rulong4_rtz)(half4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i64_v8f16, _Rulong8_rtz)(half8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i64_v16f16, _Rulong16_rtz)(half16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v2i64_v2f16, _Rulong2_rtp)(half2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v3i64_v3f16, _Rulong3_rtp)(half3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v4i64_v4f16, _Rulong4_rtp)(half4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v8i64_v8f16, _Rulong8_rtp)(half8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v16i64_v16f16, _Rulong16_rtp)(half16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v2i64_v2f16, _Rulong2_rtn)(half2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v3i64_v3f16, _Rulong3_rtn)(half3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v4i64_v4f16, _Rulong4_rtn)(half4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v8i64_v8f16, _Rulong8_rtn)(half8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v16i64_v16f16, _Rulong16_rtn)(half16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i64_v2f16, _Rulong2_sat_rte)(half2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i64_v3f16, _Rulong3_sat_rte)(half3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i64_v4f16, _Rulong4_sat_rte)(half4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i64_v8f16, _Rulong8_sat_rte)(half8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i64_v16f16, _Rulong16_sat_rte)(half16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i64_v2f16, _Rulong2_sat_rtz)(half2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i64_v3f16, _Rulong3_sat_rtz)(half3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i64_v4f16, _Rulong4_sat_rtz)(half4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i64_v8f16, _Rulong8_sat_rtz)(half8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i64_v16f16, _Rulong16_sat_rtz)(half16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i64_v2f16, _Rulong2_sat_rtp)(half2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i64_v3f16, _Rulong3_sat_rtp)(half3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i64_v4f16, _Rulong4_sat_rtp)(half4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i64_v8f16, _Rulong8_sat_rtp)(half8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i64_v16f16, _Rulong16_sat_rtp)(half16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i64_v2f16, _Rulong2_sat_rtn)(half2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i64_v3f16, _Rulong3_sat_rtn)(half3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i64_v4f16, _Rulong4_sat_rtn)(half4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i64_v8f16, _Rulong8_sat_rtn)(half8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i64_v16f16, _Rulong16_sat_rtn)(half16 x);
#if defined(cl_khr_fp64)
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v2i64_v2f64, _Rulong2)(double2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v3i64_v3f64, _Rulong3)(double3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v4i64_v4f64, _Rulong4)(double4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v8i64_v8f64, _Rulong8)(double8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _v16i64_v16f64, _Rulong16)(double16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v2i64_v2f64, _Rulong2_sat)(double2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v3i64_v3f64, _Rulong3_sat)(double3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v4i64_v4f64, _Rulong4_sat)(double4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v8i64_v8f64, _Rulong8_sat)(double8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_v16i64_v16f64, _Rulong16_sat)(double16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v2i64_v2f64, _Rulong2_rte)(double2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v3i64_v3f64, _Rulong3_rte)(double3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v4i64_v4f64, _Rulong4_rte)(double4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v8i64_v8f64, _Rulong8_rte)(double8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_v16i64_v16f64, _Rulong16_rte)(double16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v2i64_v2f64, _Rulong2_rtz)(double2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v3i64_v3f64, _Rulong3_rtz)(double3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v4i64_v4f64, _Rulong4_rtz)(double4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v8i64_v8f64, _Rulong8_rtz)(double8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_v16i64_v16f64, _Rulong16_rtz)(double16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v2i64_v2f64, _Rulong2_rtp)(double2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v3i64_v3f64, _Rulong3_rtp)(double3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v4i64_v4f64, _Rulong4_rtp)(double4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v8i64_v8f64, _Rulong8_rtp)(double8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_v16i64_v16f64, _Rulong16_rtp)(double16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v2i64_v2f64, _Rulong2_rtn)(double2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v3i64_v3f64, _Rulong3_rtn)(double3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v4i64_v4f64, _Rulong4_rtn)(double4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v8i64_v8f64, _Rulong8_rtn)(double8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_v16i64_v16f64, _Rulong16_rtn)(double16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v2i64_v2f64, _Rulong2_sat_rte)(double2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v3i64_v3f64, _Rulong3_sat_rte)(double3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v4i64_v4f64, _Rulong4_sat_rte)(double4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v8i64_v8f64, _Rulong8_sat_rte)(double8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_v16i64_v16f64, _Rulong16_sat_rte)(double16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v2i64_v2f64, _Rulong2_sat_rtz)(double2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v3i64_v3f64, _Rulong3_sat_rtz)(double3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v4i64_v4f64, _Rulong4_sat_rtz)(double4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v8i64_v8f64, _Rulong8_sat_rtz)(double8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_v16i64_v16f64, _Rulong16_sat_rtz)(double16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v2i64_v2f64, _Rulong2_sat_rtp)(double2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v3i64_v3f64, _Rulong3_sat_rtp)(double3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v4i64_v4f64, _Rulong4_sat_rtp)(double4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v8i64_v8f64, _Rulong8_sat_rtp)(double8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_v16i64_v16f64, _Rulong16_sat_rtp)(double16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v2i64_v2f64, _Rulong2_sat_rtn)(double2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v3i64_v3f64, _Rulong3_sat_rtn)(double3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v4i64_v4f64, _Rulong4_sat_rtn)(double4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v8i64_v8f64, _Rulong8_sat_rtn)(double8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_v16i64_v16f64, _Rulong16_sat_rtn)(double16 x);
#endif // defined(cl_khr_fp64)
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i8_v2i8, _Ruchar2)(char2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i8_v3i8, _Ruchar3)(char3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i8_v4i8, _Ruchar4)(char4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i8_v8i8, _Ruchar8)(char8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i8_v16i8, _Ruchar16)(char16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i8_v2i16, _Ruchar2)(short2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i8_v3i16, _Ruchar3)(short3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i8_v4i16, _Ruchar4)(short4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i8_v8i16, _Ruchar8)(short8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i8_v16i16, _Ruchar16)(short16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i8_v2i32, _Ruchar2)(int2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i8_v3i32, _Ruchar3)(int3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i8_v4i32, _Ruchar4)(int4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i8_v8i32, _Ruchar8)(int8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i8_v16i32, _Ruchar16)(int16 x);
uchar2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i8_v2i64, _Ruchar2)(long2 x);
uchar3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i8_v3i64, _Ruchar3)(long3 x);
uchar4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i8_v4i64, _Ruchar4)(long4 x);
uchar8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i8_v8i64, _Ruchar8)(long8 x);
uchar16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i8_v16i64, _Ruchar16)(long16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i16_v2i8, _Rushort2)(char2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i16_v3i8, _Rushort3)(char3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i16_v4i8, _Rushort4)(char4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i16_v8i8, _Rushort8)(char8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i16_v16i8, _Rushort16)(char16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i16_v2i16, _Rushort2)(short2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i16_v3i16, _Rushort3)(short3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i16_v4i16, _Rushort4)(short4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i16_v8i16, _Rushort8)(short8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i16_v16i16, _Rushort16)(short16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i16_v2i32, _Rushort2)(int2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i16_v3i32, _Rushort3)(int3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i16_v4i32, _Rushort4)(int4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i16_v8i32, _Rushort8)(int8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i16_v16i32, _Rushort16)(int16 x);
ushort2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i16_v2i64, _Rushort2)(long2 x);
ushort3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i16_v3i64, _Rushort3)(long3 x);
ushort4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i16_v4i64, _Rushort4)(long4 x);
ushort8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i16_v8i64, _Rushort8)(long8 x);
ushort16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i16_v16i64, _Rushort16)(long16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i32_v2i8, _Ruint2)(char2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i32_v3i8, _Ruint3)(char3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i32_v4i8, _Ruint4)(char4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i32_v8i8, _Ruint8)(char8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i32_v16i8, _Ruint16)(char16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i32_v2i16, _Ruint2)(short2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i32_v3i16, _Ruint3)(short3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i32_v4i16, _Ruint4)(short4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i32_v8i16, _Ruint8)(short8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i32_v16i16, _Ruint16)(short16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i32_v2i32, _Ruint2)(int2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i32_v3i32, _Ruint3)(int3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i32_v4i32, _Ruint4)(int4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i32_v8i32, _Ruint8)(int8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i32_v16i32, _Ruint16)(int16 x);
uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i32_v2i64, _Ruint2)(long2 x);
uint3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i32_v3i64, _Ruint3)(long3 x);
uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i32_v4i64, _Ruint4)(long4 x);
uint8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i32_v8i64, _Ruint8)(long8 x);
uint16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i32_v16i64, _Ruint16)(long16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i64_v2i8, _Rulong2)(char2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i64_v3i8, _Rulong3)(char3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i64_v4i8, _Rulong4)(char4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i64_v8i8, _Rulong8)(char8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i64_v16i8, _Rulong16)(char16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i64_v2i16, _Rulong2)(short2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i64_v3i16, _Rulong3)(short3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i64_v4i16, _Rulong4)(short4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i64_v8i16, _Rulong8)(short8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i64_v16i16, _Rulong16)(short16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i64_v2i32, _Rulong2)(int2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i64_v3i32, _Rulong3)(int3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i64_v4i32, _Rulong4)(int4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i64_v8i32, _Rulong8)(int8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i64_v16i32, _Rulong16)(int16 x);
ulong2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v2i64_v2i64, _Rulong2)(long2 x);
ulong3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v3i64_v3i64, _Rulong3)(long3 x);
ulong4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v4i64_v4i64, _Rulong4)(long4 x);
ulong8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v8i64_v8i64, _Rulong8)(long8 x);
ulong16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _v16i64_v16i64, _Rulong16)(long16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i8_v2i8, _Rchar2)(uchar2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i8_v3i8, _Rchar3)(uchar3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i8_v4i8, _Rchar4)(uchar4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i8_v8i8, _Rchar8)(uchar8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i8_v16i8, _Rchar16)(uchar16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i8_v2i16, _Rchar2)(ushort2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i8_v3i16, _Rchar3)(ushort3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i8_v4i16, _Rchar4)(ushort4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i8_v8i16, _Rchar8)(ushort8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i8_v16i16, _Rchar16)(ushort16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i8_v2i32, _Rchar2)(uint2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i8_v3i32, _Rchar3)(uint3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i8_v4i32, _Rchar4)(uint4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i8_v8i32, _Rchar8)(uint8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i8_v16i32, _Rchar16)(uint16 x);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i8_v2i64, _Rchar2)(ulong2 x);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i8_v3i64, _Rchar3)(ulong3 x);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i8_v4i64, _Rchar4)(ulong4 x);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i8_v8i64, _Rchar8)(ulong8 x);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i8_v16i64, _Rchar16)(ulong16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i16_v2i8, _Rshort2)(uchar2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i16_v3i8, _Rshort3)(uchar3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i16_v4i8, _Rshort4)(uchar4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i16_v8i8, _Rshort8)(uchar8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i16_v16i8, _Rshort16)(uchar16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i16_v2i16, _Rshort2)(ushort2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i16_v3i16, _Rshort3)(ushort3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i16_v4i16, _Rshort4)(ushort4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i16_v8i16, _Rshort8)(ushort8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i16_v16i16, _Rshort16)(ushort16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i16_v2i32, _Rshort2)(uint2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i16_v3i32, _Rshort3)(uint3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i16_v4i32, _Rshort4)(uint4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i16_v8i32, _Rshort8)(uint8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i16_v16i32, _Rshort16)(uint16 x);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i16_v2i64, _Rshort2)(ulong2 x);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i16_v3i64, _Rshort3)(ulong3 x);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i16_v4i64, _Rshort4)(ulong4 x);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i16_v8i64, _Rshort8)(ulong8 x);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i16_v16i64, _Rshort16)(ulong16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i32_v2i8, _Rint2)(uchar2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i32_v3i8, _Rint3)(uchar3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i32_v4i8, _Rint4)(uchar4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i32_v8i8, _Rint8)(uchar8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i32_v16i8, _Rint16)(uchar16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i32_v2i16, _Rint2)(ushort2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i32_v3i16, _Rint3)(ushort3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i32_v4i16, _Rint4)(ushort4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i32_v8i16, _Rint8)(ushort8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i32_v16i16, _Rint16)(ushort16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i32_v2i32, _Rint2)(uint2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i32_v3i32, _Rint3)(uint3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i32_v4i32, _Rint4)(uint4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i32_v8i32, _Rint8)(uint8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i32_v16i32, _Rint16)(uint16 x);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i32_v2i64, _Rint2)(ulong2 x);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i32_v3i64, _Rint3)(ulong3 x);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i32_v4i64, _Rint4)(ulong4 x);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i32_v8i64, _Rint8)(ulong8 x);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i32_v16i64, _Rint16)(ulong16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i64_v2i8, _Rlong2)(uchar2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i64_v3i8, _Rlong3)(uchar3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i64_v4i8, _Rlong4)(uchar4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i64_v8i8, _Rlong8)(uchar8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i64_v16i8, _Rlong16)(uchar16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i64_v2i16, _Rlong2)(ushort2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i64_v3i16, _Rlong3)(ushort3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i64_v4i16, _Rlong4)(ushort4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i64_v8i16, _Rlong8)(ushort8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i64_v16i16, _Rlong16)(ushort16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i64_v2i32, _Rlong2)(uint2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i64_v3i32, _Rlong3)(uint3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i64_v4i32, _Rlong4)(uint4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i64_v8i32, _Rlong8)(uint8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i64_v16i32, _Rlong16)(uint16 x);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v2i64_v2i64, _Rlong2)(ulong2 x);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v3i64_v3i64, _Rlong3)(ulong3 x);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v4i64_v4i64, _Rlong4)(ulong4 x);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v8i64_v8i64, _Rlong8)(ulong8 x);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _v16i64_v16i64, _Rlong16)(ulong16 x);

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
private void* SPIRV_OVERLOADABLE SPIRV_BUILTIN(GenericCastToPtrExplicit, _p0i8_p4i8_i32, _ToPrivate)(const generic void *Pointer, StorageClass_t Storage);
local   void* SPIRV_OVERLOADABLE SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(const generic void *Pointer, StorageClass_t Storage);
global  void* SPIRV_OVERLOADABLE SPIRV_BUILTIN(GenericCastToPtrExplicit, _p1i8_p4i8_i32, _ToGlobal)(const generic void *Pointer, StorageClass_t Storage);
uint __builtin_spirv_OpGenericPtrMemSemantics_p4i8(const generic void *Pointer);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

// Arithmetic Instructions

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v2f16_v2f16, )(half2 Vector1, half2 Vector2);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v3f16_v3f16, )(half3 Vector1, half3 Vector2);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v4f16_v4f16, )(half4 Vector1, half4 Vector2);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v8f16_v8f16, )(half8 Vector1, half8 Vector2);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v16f16_v16f16, )(half16 Vector1, half16 Vector2);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v2f32_v2f32, )(float2 Vector1, float2 Vector2);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v3f32_v3f32, )(float3 Vector1, float3 Vector2);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v4f32_v4f32, )(float4 Vector1, float4 Vector2);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v8f32_v8f32, )(float8 Vector1, float8 Vector2);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v16f32_v16f32, )(float16 Vector1, float16 Vector2);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v2f64_v2f64, )(double2 Vector1, double2 Vector2);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v3f64_v3f64, )(double3 Vector1, double3 Vector2);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v4f64_v4f64, )(double4 Vector1, double4 Vector2);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v8f64_v8f64, )(double8 Vector1, double8 Vector2);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v16f64_v16f64, )(double16 Vector1, double16 Vector2);
#endif // defined(cl_khr_fp64)
typedef struct { uchar a; uchar b; } TwoOp_i8;
typedef struct { uchar2 a; uchar2 b; } TwoOp_v2i8;
typedef struct { uchar3 a; uchar3 b; } TwoOp_v3i8;
typedef struct { uchar4 a; uchar4 b; } TwoOp_v4i8;
typedef struct { uchar8 a; uchar8 b; } TwoOp_v8i8;
typedef struct { uchar16 a; uchar16 b; } TwoOp_v16i8;
typedef struct { ushort a; ushort b; } TwoOp_i16;
typedef struct { ushort2 a; ushort2 b; } TwoOp_v2i16;
typedef struct { ushort3 a; ushort3 b; } TwoOp_v3i16;
typedef struct { ushort4 a; ushort4 b; } TwoOp_v4i16;
typedef struct { ushort8 a; ushort8 b; } TwoOp_v8i16;
typedef struct { ushort16 a; ushort16 b; } TwoOp_v16i16;
typedef struct { uint a; uint b; } TwoOp_i32;
typedef struct { uint2 a; uint2 b; } TwoOp_v2i32;
typedef struct { uint3 a; uint3 b; } TwoOp_v3i32;
typedef struct { uint4 a; uint4 b; } TwoOp_v4i32;
typedef struct { uint8 a; uint8 b; } TwoOp_v8i32;
typedef struct { uint16 a; uint16 b; } TwoOp_v16i32;
typedef struct { ulong a; ulong b; } TwoOp_i64;
typedef struct { ulong2 a; ulong2 b; } TwoOp_v2i64;
typedef struct { ulong3 a; ulong3 b; } TwoOp_v3i64;
typedef struct { ulong4 a; ulong4 b; } TwoOp_v4i64;
typedef struct { ulong8 a; ulong8 b; } TwoOp_v8i64;
typedef struct { ulong16 a; ulong16 b; } TwoOp_v16i64;

TwoOp_i8    __builtin_spirv_OpUMulExtended_i8_i8(uchar Operand1, uchar Operand2);
TwoOp_v2i8  __builtin_spirv_OpUMulExtended_v2i8_v2i8(uchar2 Operand1, uchar2 Operand2);
TwoOp_v3i8  __builtin_spirv_OpUMulExtended_v3i8_v3i8(uchar3 Operand1, uchar3 Operand2);
TwoOp_v4i8  __builtin_spirv_OpUMulExtended_v4i8_v4i8(uchar4 Operand1, uchar4 Operand2);
TwoOp_v8i8  __builtin_spirv_OpUMulExtended_v8i8_v8i8(uchar8 Operand1, uchar8 Operand2);
TwoOp_v16i8 __builtin_spirv_OpUMulExtended_v16i8_v16i8(uchar16 Operand1, uchar16 Operand2);
TwoOp_i16    __builtin_spirv_OpUMulExtended_i16_i16(ushort Operand1, ushort Operand2);
TwoOp_v2i16  __builtin_spirv_OpUMulExtended_v2i16_v2i16(ushort2 Operand1, ushort2 Operand2);
TwoOp_v3i16  __builtin_spirv_OpUMulExtended_v3i16_v3i16(ushort3 Operand1, ushort3 Operand2);
TwoOp_v4i16  __builtin_spirv_OpUMulExtended_v4i16_v4i16(ushort4 Operand1, ushort4 Operand2);
TwoOp_v8i16  __builtin_spirv_OpUMulExtended_v8i16_v8i16(ushort8 Operand1, ushort8 Operand2);
TwoOp_v16i16 __builtin_spirv_OpUMulExtended_v16i16_v16i16(ushort16 Operand1, ushort16 Operand2);
TwoOp_i32    __builtin_spirv_OpUMulExtended_i32_i32(uint Operand1, uint Operand2);
TwoOp_v2i32  __builtin_spirv_OpUMulExtended_v2i32_v2i32(uint2 Operand1, uint2 Operand2);
TwoOp_v3i32  __builtin_spirv_OpUMulExtended_v3i32_v3i32(uint3 Operand1, uint3 Operand2);
TwoOp_v4i32  __builtin_spirv_OpUMulExtended_v4i32_v4i32(uint4 Operand1, uint4 Operand2);
TwoOp_v8i32  __builtin_spirv_OpUMulExtended_v8i32_v8i32(uint8 Operand1, uint8 Operand2);
TwoOp_v16i32 __builtin_spirv_OpUMulExtended_v16i32_v16i32(uint16 Operand1, uint16 Operand2);
TwoOp_i64    __builtin_spirv_OpUMulExtended_i64_i64(ulong Operand1, ulong Operand2);
TwoOp_v2i64  __builtin_spirv_OpUMulExtended_v2i64_v2i64(ulong2 Operand1, ulong2 Operand2);
TwoOp_v3i64  __builtin_spirv_OpUMulExtended_v3i64_v3i64(ulong3 Operand1, ulong3 Operand2);
TwoOp_v4i64  __builtin_spirv_OpUMulExtended_v4i64_v4i64(ulong4 Operand1, ulong4 Operand2);
TwoOp_v8i64  __builtin_spirv_OpUMulExtended_v8i64_v8i64(ulong8 Operand1, ulong8 Operand2);
TwoOp_v16i64 __builtin_spirv_OpUMulExtended_v16i64_v16i64(ulong16 Operand1, ulong16 Operand2);

TwoOp_i8    __builtin_spirv_OpSMulExtended_i8_i8(char Operand1, char Operand2);
TwoOp_v2i8  __builtin_spirv_OpSMulExtended_v2i8_v2i8(char2 Operand1, char2 Operand2);
TwoOp_v3i8  __builtin_spirv_OpSMulExtended_v3i8_v3i8(char3 Operand1, char3 Operand2);
TwoOp_v4i8  __builtin_spirv_OpSMulExtended_v4i8_v4i8(char4 Operand1, char4 Operand2);
TwoOp_v8i8  __builtin_spirv_OpSMulExtended_v8i8_v8i8(char8 Operand1, char8 Operand2);
TwoOp_v16i8 __builtin_spirv_OpSMulExtended_v16i8_v16i8(char16 Operand1, char16 Operand2);
TwoOp_i16    __builtin_spirv_OpSMulExtended_i16_i16(short Operand1, short Operand2);
TwoOp_v2i16  __builtin_spirv_OpSMulExtended_v2i16_v2i16(short2 Operand1, short2 Operand2);
TwoOp_v3i16  __builtin_spirv_OpSMulExtended_v3i16_v3i16(short3 Operand1, short3 Operand2);
TwoOp_v4i16  __builtin_spirv_OpSMulExtended_v4i16_v4i16(short4 Operand1, short4 Operand2);
TwoOp_v8i16  __builtin_spirv_OpSMulExtended_v8i16_v8i16(short8 Operand1, short8 Operand2);
TwoOp_v16i16 __builtin_spirv_OpSMulExtended_v16i16_v16i16(short16 Operand1, short16 Operand2);
TwoOp_i32    __builtin_spirv_OpSMulExtended_i32_i32(int Operand1, int Operand2);
TwoOp_v2i32  __builtin_spirv_OpSMulExtended_v2i32_v2i32(int2 Operand1, int2 Operand2);
TwoOp_v3i32  __builtin_spirv_OpSMulExtended_v3i32_v3i32(int3 Operand1, int3 Operand2);
TwoOp_v4i32  __builtin_spirv_OpSMulExtended_v4i32_v4i32(int4 Operand1, int4 Operand2);
TwoOp_v8i32  __builtin_spirv_OpSMulExtended_v8i32_v8i32(int8 Operand1, int8 Operand2);
TwoOp_v16i32 __builtin_spirv_OpSMulExtended_v16i32_v16i32(int16 Operand1, int16 Operand2);
TwoOp_i64    __builtin_spirv_OpSMulExtended_i64_i64(long Operand1, long Operand2);
TwoOp_v2i64  __builtin_spirv_OpSMulExtended_v2i64_v2i64(long2 Operand1, long2 Operand2);
TwoOp_v3i64  __builtin_spirv_OpSMulExtended_v3i64_v3i64(long3 Operand1, long3 Operand2);
TwoOp_v4i64  __builtin_spirv_OpSMulExtended_v4i64_v4i64(long4 Operand1, long4 Operand2);
TwoOp_v8i64  __builtin_spirv_OpSMulExtended_v8i64_v8i64(long8 Operand1, long8 Operand2);
TwoOp_v16i64 __builtin_spirv_OpSMulExtended_v16i64_v16i64(long16 Operand1, long16 Operand2);

// Bit Instructions

char SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _i8_i8_i32_i32, )(char Base, char Insert, uint Offset, uint Count);
char2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v2i8_v2i8_v2i32_v2i32, )(char2 Base, char2 Insert, uint2 Offset, uint2 Count);
char3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v3i8_v3i8_v3i32_v3i32, )(char3 Base, char3 Insert, uint3 Offset, uint3 Count);
char4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v4i8_v4i8_v4i32_v4i32, )(char4 Base, char4 Insert, uint4 Offset, uint4 Count);
char8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v8i8_v8i8_v8i32_v8i32, )(char8 Base, char8 Insert, uint8 Offset, uint8 Count);
char16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v16i8_v16i8_v16i32_v16i32, )(char16 Base, char16 Insert, uint16 Offset, uint16 Count);
short SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _i16_i16_i32_i32, )(short Base, short Insert, uint Offset, uint Count);
short2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v2i16_v2i16_v2i32_v2i32, )(short2 Base, short2 Insert, uint2 Offset, uint2 Count);
short3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v3i16_v3i16_v3i32_v3i32, )(short3 Base, short3 Insert, uint3 Offset, uint3 Count);
short4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v4i16_v4i16_v4i32_v4i32, )(short4 Base, short4 Insert, uint4 Offset, uint4 Count);
short8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v8i16_v8i16_v8i32_v8i32, )(short8 Base, short8 Insert, uint8 Offset, uint8 Count);
short16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v16i16_v16i16_v16i32_v16i32, )(short16 Base, short16 Insert, uint16 Offset, uint16 Count);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _i32_i32_i32_i32, )(int Base, int Insert, uint Offset, uint Count);
int2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v2i32_v2i32_v2i32_v2i32, )(int2 Base, int2 Insert, uint2 Offset, uint2 Count);
int3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v3i32_v3i32_v3i32_v3i32, )(int3 Base, int3 Insert, uint3 Offset, uint3 Count);
int4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v4i32_v4i32_v4i32_v4i32, )(int4 Base, int4 Insert, uint4 Offset, uint4 Count);
int8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v8i32_v8i32_v8i32_v8i32, )(int8 Base, int8 Insert, uint8 Offset, uint8 Count);
int16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v16i32_v16i32_v16i32_v16i32, )(int16 Base, int16 Insert, uint16 Offset, uint16 Count);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _i64_i64_i32_i32, )(long Base, long Insert, uint Offset, uint Count);
long2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v2i64_v2i64_v2i32_v2i32, )(long2 Base, long2 Insert, uint2 Offset, uint2 Count);
long3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v3i64_v3i64_v3i32_v3i32, )(long3 Base, long3 Insert, uint3 Offset, uint3 Count);
long4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v4i64_v4i64_v4i32_v4i32, )(long4 Base, long4 Insert, uint4 Offset, uint4 Count);
long8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v8i64_v8i64_v8i32_v8i32, )(long8 Base, long8 Insert, uint8 Offset, uint8 Count);
long16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(BitFieldInsert, _v16i64_v16i64_v16i32_v16i32, )(long16 Base, long16 Insert, uint16 Offset, uint16 Count);

char __builtin_spirv_OpBitFieldSExtract_i8_i32_i32(char Base, uint Offset, uint Count);
char2 __builtin_spirv_OpBitFieldSExtract_v2i8_v2i32_v2i32(char2 Base, uint2 Offset, uint2 Count);
char3 __builtin_spirv_OpBitFieldSExtract_v3i8_v3i32_v3i32(char3 Base, uint3 Offset, uint3 Count);
char4 __builtin_spirv_OpBitFieldSExtract_v4i8_v4i32_v4i32(char4 Base, uint4 Offset, uint4 Count);
char8 __builtin_spirv_OpBitFieldSExtract_v8i8_v8i32_v8i32(char8 Base, uint8 Offset, uint8 Count);
char16 __builtin_spirv_OpBitFieldSExtract_v16i8_v16i32_v16i32(char16 Base, uint16 Offset, uint16 Count);
short __builtin_spirv_OpBitFieldSExtract_i16_i32_i32(short Base, uint Offset, uint Count);
short2 __builtin_spirv_OpBitFieldSExtract_v2i16_v2i32_v2i32(short2 Base, uint2 Offset, uint2 Count);
short3 __builtin_spirv_OpBitFieldSExtract_v3i16_v3i32_v3i32(short3 Base, uint3 Offset, uint3 Count);
short4 __builtin_spirv_OpBitFieldSExtract_v4i16_v4i32_v4i32(short4 Base, uint4 Offset, uint4 Count);
short8 __builtin_spirv_OpBitFieldSExtract_v8i16_v8i32_v8i32(short8 Base, uint8 Offset, uint8 Count);
short16 __builtin_spirv_OpBitFieldSExtract_v16i16_v16i32_v16i32(short16 Base, uint16 Offset, uint16 Count);
int __builtin_spirv_OpBitFieldSExtract_i32_i32_i32(int Base, uint Offset, uint Count);
int2 __builtin_spirv_OpBitFieldSExtract_v2i32_v2i32_v2i32(int2 Base, uint2 Offset, uint2 Count);
int3 __builtin_spirv_OpBitFieldSExtract_v3i32_v3i32_v3i32(int3 Base, uint3 Offset, uint3 Count);
int4 __builtin_spirv_OpBitFieldSExtract_v4i32_v4i32_v4i32(int4 Base, uint4 Offset, uint4 Count);
int8 __builtin_spirv_OpBitFieldSExtract_v8i32_v8i32_v8i32(int8 Base, uint8 Offset, uint8 Count);
int16 __builtin_spirv_OpBitFieldSExtract_v16i32_v16i32_v16i32(int16 Base, uint16 Offset, uint16 Count);
long __builtin_spirv_OpBitFieldSExtract_i64_i32_i32(long Base, uint Offset, uint Count);
long2 __builtin_spirv_OpBitFieldSExtract_v2i64_v2i32_v2i32(long2 Base, uint2 Offset, uint2 Count);
long3 __builtin_spirv_OpBitFieldSExtract_v3i64_v3i32_v3i32(long3 Base, uint3 Offset, uint3 Count);
long4 __builtin_spirv_OpBitFieldSExtract_v4i64_v4i32_v4i32(long4 Base, uint4 Offset, uint4 Count);
long8 __builtin_spirv_OpBitFieldSExtract_v8i64_v8i32_v8i32(long8 Base, uint8 Offset, uint8 Count);
long16 __builtin_spirv_OpBitFieldSExtract_v16i64_v16i32_v16i32(long16 Base, uint16 Offset, uint16 Count);

uchar __builtin_spirv_OpBitFieldUExtract_i8_i32_i32(uchar Base, uint Offset, uint Count);
uchar2 __builtin_spirv_OpBitFieldUExtract_v2i8_v2i32_v2i32(uchar2 Base, uint2 Offset, uint2 Count);
uchar3 __builtin_spirv_OpBitFieldUExtract_v3i8_v3i32_v3i32(uchar3 Base, uint3 Offset, uint3 Count);
uchar4 __builtin_spirv_OpBitFieldUExtract_v4i8_v4i32_v4i32(uchar4 Base, uint4 Offset, uint4 Count);
uchar8 __builtin_spirv_OpBitFieldUExtract_v8i8_v8i32_v8i32(uchar8 Base, uint8 Offset, uint8 Count);
uchar16 __builtin_spirv_OpBitFieldUExtract_v16i8_v16i32_v16i32(uchar16 Base, uint16 Offset, uint16 Count);
ushort __builtin_spirv_OpBitFieldUExtract_i16_i32_i32(ushort Base, uint Offset, uint Count);
ushort2 __builtin_spirv_OpBitFieldUExtract_v2i16_v2i32_v2i32(ushort2 Base, uint2 Offset, uint2 Count);
ushort3 __builtin_spirv_OpBitFieldUExtract_v3i16_v3i32_v3i32(ushort3 Base, uint3 Offset, uint3 Count);
ushort4 __builtin_spirv_OpBitFieldUExtract_v4i16_v4i32_v4i32(ushort4 Base, uint4 Offset, uint4 Count);
ushort8 __builtin_spirv_OpBitFieldUExtract_v8i16_v8i32_v8i32(ushort8 Base, uint8 Offset, uint8 Count);
ushort16 __builtin_spirv_OpBitFieldUExtract_v16i16_v16i32_v16i32(ushort16 Base, uint16 Offset, uint16 Count);
uint __builtin_spirv_OpBitFieldUExtract_i32_i32_i32(uint Base, uint Offset, uint Count);
uint2 __builtin_spirv_OpBitFieldUExtract_v2i32_v2i32_v2i32(uint2 Base, uint2 Offset, uint2 Count);
uint3 __builtin_spirv_OpBitFieldUExtract_v3i32_v3i32_v3i32(uint3 Base, uint3 Offset, uint3 Count);
uint4 __builtin_spirv_OpBitFieldUExtract_v4i32_v4i32_v4i32(uint4 Base, uint4 Offset, uint4 Count);
uint8 __builtin_spirv_OpBitFieldUExtract_v8i32_v8i32_v8i32(uint8 Base, uint8 Offset, uint8 Count);
uint16 __builtin_spirv_OpBitFieldUExtract_v16i32_v16i32_v16i32(uint16 Base, uint16 Offset, uint16 Count);
ulong __builtin_spirv_OpBitFieldUExtract_i64_i32_i32(ulong Base, uint Offset, uint Count);
ulong2 __builtin_spirv_OpBitFieldUExtract_v2i64_v2i32_v2i32(ulong2 Base, uint2 Offset, uint2 Count);
ulong3 __builtin_spirv_OpBitFieldUExtract_v3i64_v3i32_v3i32(ulong3 Base, uint3 Offset, uint3 Count);
ulong4 __builtin_spirv_OpBitFieldUExtract_v4i64_v4i32_v4i32(ulong4 Base, uint4 Offset, uint4 Count);
ulong8 __builtin_spirv_OpBitFieldUExtract_v8i64_v8i32_v8i32(ulong8 Base, uint8 Offset, uint8 Count);
ulong16 __builtin_spirv_OpBitFieldUExtract_v16i64_v16i32_v16i32(ulong16 Base, uint16 Offset, uint16 Count);


uchar __builtin_spirv_OpBitReverse_i8(uchar Base);
uchar2 __builtin_spirv_OpBitReverse_v2i8(uchar2 Base);
uchar3 __builtin_spirv_OpBitReverse_v3i8(uchar3 Base);
uchar4 __builtin_spirv_OpBitReverse_v4i8(uchar4 Base);
uchar8 __builtin_spirv_OpBitReverse_v8i8(uchar8 Base);
uchar16 __builtin_spirv_OpBitReverse_v16i8(uchar16 Base);
ushort __builtin_spirv_OpBitReverse_i16(ushort Base);
ushort2 __builtin_spirv_OpBitReverse_v2i16(ushort2 Base);
ushort3 __builtin_spirv_OpBitReverse_v3i16(ushort3 Base);
ushort4 __builtin_spirv_OpBitReverse_v4i16(ushort4 Base);
ushort8 __builtin_spirv_OpBitReverse_v8i16(ushort8 Base);
ushort16 __builtin_spirv_OpBitReverse_v16i16(ushort16 Base);
uint __builtin_spirv_OpBitReverse_i32(uint Base);
uint2 __builtin_spirv_OpBitReverse_v2i32(uint2 Base);
uint3 __builtin_spirv_OpBitReverse_v3i32(uint3 Base);
uint4 __builtin_spirv_OpBitReverse_v4i32(uint4 Base);
uint8 __builtin_spirv_OpBitReverse_v8i32(uint8 Base);
uint16 __builtin_spirv_OpBitReverse_v16i32(uint16 Base);
ulong __builtin_spirv_OpBitReverse_i64(ulong Base);
ulong2 __builtin_spirv_OpBitReverse_v2i64(ulong2 Base);
ulong3 __builtin_spirv_OpBitReverse_v3i64(ulong3 Base);
ulong4 __builtin_spirv_OpBitReverse_v4i64(ulong4 Base);
ulong8 __builtin_spirv_OpBitReverse_v8i64(ulong8 Base);
ulong16 __builtin_spirv_OpBitReverse_v16i64(ulong16 Base);

uchar __builtin_spirv_OpBitCount_i8(uchar Base);
uchar2 __builtin_spirv_OpBitCount_v2i8(uchar2 Base);
uchar3 __builtin_spirv_OpBitCount_v3i8(uchar3 Base);
uchar4 __builtin_spirv_OpBitCount_v4i8(uchar4 Base);
uchar8 __builtin_spirv_OpBitCount_v8i8(uchar8 Base);
uchar16 __builtin_spirv_OpBitCount_v16i8(uchar16 Base);
ushort __builtin_spirv_OpBitCount_i16(ushort Base);
ushort2 __builtin_spirv_OpBitCount_v2i16(ushort2 Base);
ushort3 __builtin_spirv_OpBitCount_v3i16(ushort3 Base);
ushort4 __builtin_spirv_OpBitCount_v4i16(ushort4 Base);
ushort8 __builtin_spirv_OpBitCount_v8i16(ushort8 Base);
ushort16 __builtin_spirv_OpBitCount_v16i16(ushort16 Base);
uint __builtin_spirv_OpBitCount_i32(uint Base);
uint2 __builtin_spirv_OpBitCount_v2i32(uint2 Base);
uint3 __builtin_spirv_OpBitCount_v3i32(uint3 Base);
uint4 __builtin_spirv_OpBitCount_v4i32(uint4 Base);
uint8 __builtin_spirv_OpBitCount_v8i32(uint8 Base);
uint16 __builtin_spirv_OpBitCount_v16i32(uint16 Base);
ulong __builtin_spirv_OpBitCount_i64(ulong Base);
ulong2 __builtin_spirv_OpBitCount_v2i64(ulong2 Base);
ulong3 __builtin_spirv_OpBitCount_v3i64(ulong3 Base);
ulong4 __builtin_spirv_OpBitCount_v4i64(ulong4 Base);
ulong8 __builtin_spirv_OpBitCount_v8i64(ulong8 Base);
ulong16 __builtin_spirv_OpBitCount_v16i64(ulong16 Base);

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

bool  SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _f16, )(half x);
bool  SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _f32, )(float x);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _v2f16, )(half2 x);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _v2f32, )(float2 x);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _v3f16, )(half3 x);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _v3f32, )(float3 x);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _v4f16, )(half4 x);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _v4f32, )(float4 x);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _v8f16, )(half8 x);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _v8f32, )(float8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _v16f16, )(half16 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
bool  SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _f64, )(double x);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _v2f64, )(double2 x);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _v3f64, )(double3 x);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _v4f64, )(double4 x);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _v8f64, )(double8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _f16, )(half x);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _f32, )(float x);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _v2f16, )(half2 x);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _v2f32, )(float2 x);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _v3f16, )(half3 x);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _v3f32, )(float3 x);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _v4f16, )(half4 x);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _v4f32, )(float4 x);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _v8f16, )(half8 x);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _v8f32, )(float8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _v16f16, )(half16 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _f64, )(double x);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _v2f64, )(double2 x);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _v3f64, )(double3 x);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _v4f64, )(double4 x);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _v8f64, )(double8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _f16, )(half x);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _f32, )(float x);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _v2f16, )(half2 x);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _v2f32, )(float2 x);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _v3f16, )(half3 x);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _v3f32, )(float3 x);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _v4f16, )(half4 x);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _v4f32, )(float4 x);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _v8f16, )(half8 x);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _v8f32, )(float8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _v16f16, )(half16 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _f64, )(double x);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _v2f64, )(double2 x);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _v3f64, )(double3 x);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _v4f64, )(double4 x);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _v8f64, )(double8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _f16, )(half x);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _f32, )(float x);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _v2f16, )(half2 x);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _v2f32, )(float2 x);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _v3f16, )(half3 x);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _v3f32, )(float3 x);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _v4f16, )(half4 x);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _v4f32, )(float4 x);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _v8f16, )(half8 x);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _v8f32, )(float8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _v16f16, )(half16 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _f64, )(double x);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _v2f64, )(double2 x);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _v3f64, )(double3 x);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _v4f64, )(double4 x);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _v8f64, )(double8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _f16, )(half x);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _f32, )(float x);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _v2f16, )(half2 x);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _v2f32, )(float2 x);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _v3f16, )(half3 x);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _v3f32, )(float3 x);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _v4f16, )(half4 x);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _v4f32, )(float4 x);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _v8f16, )(half8 x);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _v8f32, )(float8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _v16f16, )(half16 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _v16f32, )(float16 x);
#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _f64, )(double x);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _v2f64, )(double2 x);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _v3f64, )(double3 x);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _v4f64, )(double4 x);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _v8f64, )(double8 x);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _v16f64, )(double16 x);
#endif // defined(cl_khr_fp64)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _f16_f16, )(half x, half y);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _f32_f32, )(float x, float y);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v2f16_v2f16, )(half2 x, half2 y);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v2f32_v2f32, )(float2 x, float2 y);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v3f16_v3f16, )(half3 x, half3 y);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v3f32_v3f32, )(float3 x, float3 y);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v4f16_v4f16, )(half4 x, half4 y);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v4f32_v4f32, )(float4 x, float4 y);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v8f16_v8f16, )(half8 x, half8 y);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v8f32_v8f32, )(float8 x, float8 y);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v16f16_v16f16, )(half16 x, half16 y);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _f64_f64, )(double x, double y);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v2f64_v2f64, )(double2 x, double2 y);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v3f64_v3f64, )(double3 x, double3 y);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v4f64_v4f64, )(double4 x, double4 y);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v8f64_v8f64, )(double8 x, double8 y);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _f16_f16, )(half x, half y);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _f32_f32, )(float x, float y);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v2f16_v2f16, )(half2 x, half2 y);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v2f32_v2f32, )(float2 x, float2 y);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v3f16_v3f16, )(half3 x, half3 y);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v3f32_v3f32, )(float3 x, float3 y);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v4f16_v4f16, )(half4 x, half4 y);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v4f32_v4f32, )(float4 x, float4 y);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v8f16_v8f16, )(half8 x, half8 y);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v8f32_v8f32, )(float8 x, float8 y);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v16f16_v16f16, )(half16 x, half16 y);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _f64_f64, )(double x, double y);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v2f64_v2f64, )(double2 x, double2 y);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v3f64_v3f64, )(double3 x, double3 y);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v4f64_v4f64, )(double4 x, double4 y);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v8f64_v8f64, )(double8 x, double8 y);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _f16_f16, )(half x, half y);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _f32_f32, )(float x, float y);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v2f16_v2f16, )(half2 x, half2 y);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v2f32_v2f32, )(float2 x, float2 y);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v3f16_v3f16, )(half3 x, half3 y);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v3f32_v3f32, )(float3 x, float3 y);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v4f16_v4f16, )(half4 x, half4 y);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v4f32_v4f32, )(float4 x, float4 y);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v8f16_v8f16, )(half8 x, half8 y);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v8f32_v8f32, )(float8 x, float8 y);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v16f16_v16f16, )(half16 x, half16 y);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v16f32_v16f32, )(float16 x, float16 y);
#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _f64_f64, )(double x, double y);
__bool2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v2f64_v2f64, )(double2 x, double2 y);
__bool3 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v3f64_v3f64, )(double3 x, double3 y);
__bool4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v4f64_v4f64, )(double4 x, double4 y);
__bool8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v8f64_v8f64, )(double8 x, double8 y);
__bool16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _v16f64_v16f64, )(double16 x, double16 y);
#endif // defined(cl_khr_fp64)

// Atomic Instructions

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p0i32_i32_i32, )(private int *Pointer, int Scope, int Semantics);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p1i32_i32_i32, )(global int *Pointer, int Scope, int Semantics);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p3i32_i32_i32, )(local int *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p4i32_i32_i32, )(generic int *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p0i64_i32_i32, )(private long *Pointer, int Scope, int Semantics);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p1i64_i32_i32, )(global long *Pointer, int Scope, int Semantics);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p3i64_i32_i32, )(local long *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p4i64_i32_i32, )(generic long *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p0f32_i32_i32, )(private float *Pointer, int Scope, int Semantics);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p1f32_i32_i32, )(global float *Pointer, int Scope, int Semantics);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p3f32_i32_i32, )(local float *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p4f32_i32_i32, )(generic float *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
#if defined(cl_khr_int64_base_atomics)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p0f64_i32_i32, )(private double *Pointer, int Scope, int Semantics);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p1f64_i32_i32, )(global double *Pointer, int Scope, int Semantics);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p3f64_i32_i32, )(local double *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicLoad, _p4f64_i32_i32, )(generic double *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)
#endif // defined(cl_khr_fp64)

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p0i32_i32_i32_i32, )(private int *Pointer, int Scope, int Semantics, int Value);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p1i32_i32_i32_i32, )(global int *Pointer, int Scope, int Semantics, int Value);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p3i32_i32_i32_i32, )(local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p4i32_i32_i32_i32, )(generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p0i64_i32_i32_i64, )(private long *Pointer, int Scope, int Semantics, long Value);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p1i64_i32_i32_i64, )(global long *Pointer, int Scope, int Semantics, long Value);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p3i64_i32_i32_i64, )(local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p4i64_i32_i32_i64, )(generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p0f32_i32_i32_f32, )(private float *Pointer, int Scope, int Semantics, float Value);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p1f32_i32_i32_f32, )(global float *Pointer, int Scope, int Semantics, float Value);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p3f32_i32_i32_f32, )(local float *Pointer, int Scope, int Semantics, float Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p4f32_i32_i32_f32, )(generic float *Pointer, int Scope, int Semantics, float Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
#if defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p0f64_i32_i32_f64, )(private double *Pointer, int Scope, int Semantics, double Value);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p1f64_i32_i32_f64, )(global double *Pointer, int Scope, int Semantics, double Value);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p3f64_i32_i32_f64, )(local double *Pointer, int Scope, int Semantics, double Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicStore, _p4f64_i32_i32_f64, )(generic double *Pointer, int Scope, int Semantics, double Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics) || defined(cl_khr_int64_extended_atomics)
#endif // defined(cl_khr_fp64)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p0i32_i32_i32_i32, )(private int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p1i32_i32_i32_i32, )(global int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p3i32_i32_i32_i32, )(local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p4i32_i32_i32_i32, )(generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p0i64_i32_i32_i64, )(private long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p1i64_i32_i32_i64, )(global long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p3i64_i32_i32_i64, )(local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p4i64_i32_i32_i64, )(generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p0f32_i32_i32_f32, )(private float *Pointer, int Scope, int Semantics, float Value);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p1f32_i32_i32_f32, )(global float *Pointer, int Scope, int Semantics, float Value);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p3f32_i32_i32_f32, )(local float *Pointer, int Scope, int Semantics, float Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p4f32_i32_i32_f32, )(generic float *Pointer, int Scope, int Semantics, float Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#if defined(cl_khr_fp64)
#if defined(cl_khr_int64_base_atomics)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p0f64_i32_i32_f64, )(private double *Pointer, int Scope, int Semantics, double Value);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p1f64_i32_i32_f64, )(global double *Pointer, int Scope, int Semantics, double Value);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p3f64_i32_i32_f64, )(local double *Pointer, int Scope, int Semantics, double Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicExchange, _p4f64_i32_i32_f64, )(generic double *Pointer, int Scope, int Semantics, double Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)
#endif // defined(cl_khr_fp64)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p0i32_i32_i32_i32_i32_i32, )(private int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p1i32_i32_i32_i32_i32_i32, )(global int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p3i32_i32_i32_i32_i32_i32, )(local int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p4i32_i32_i32_i32_i32_i32, )(generic int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p0i64_i32_i32_i32_i64_i64, )(private long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p1i64_i32_i32_i32_i64_i64, )(global long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p3i64_i32_i32_i32_i64_i64, )(local long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p4i64_i32_i32_i32_i64_i64, )(generic long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p0f32_i32_i32_i32_f32_f32, )(private float *Pointer, int Scope, int Equal, int Unequal, float Value, float Comparator);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p1f32_i32_i32_i32_f32_f32, )(global float *Pointer, int Scope, int Equal, int Unequal, float Value, float Comparator);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p3f32_i32_i32_i32_f32_f32, )(local float *Pointer, int Scope, int Equal, int Unequal, float Value, float Comparator);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchange, _p4f32_i32_i32_i32_f32_f32, )(generic float *Pointer, int Scope, int Equal, int Unequal, float Value, float Comparator);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p0i32_i32_i32_i32_i32_i32, )(private int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p1i32_i32_i32_i32_i32_i32, )(global int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p3i32_i32_i32_i32_i32_i32, )(local int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p4i32_i32_i32_i32_i32_i32, )(generic int *Pointer, int Scope, int Equal, int Unequal, int Value, int Comparator);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p0i64_i32_i32_i32_i64_i64, )(private long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p1i64_i32_i32_i32_i64_i64, )(global long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p3i64_i32_i32_i32_i64_i64, )(local long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicCompareExchangeWeak, _p4i64_i32_i32_i32_i64_i64, )(generic long *Pointer, int Scope, int Equal, int Unequal, long Value, long Comparator);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p0i32_i32_i32, )(private int *Pointer, int Scope, int Semantics);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p1i32_i32_i32, )(global int *Pointer, int Scope, int Semantics);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p3i32_i32_i32, )(local int *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p4i32_i32_i32, )(generic int *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p0i64_i32_i32, )(private long *Pointer, int Scope, int Semantics);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p1i64_i32_i32, )(global long *Pointer, int Scope, int Semantics);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p3i64_i32_i32, )(local long *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIIncrement, _p4i64_i32_i32, )(generic long *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p0i32_i32_i32, )(private int *Pointer, int Scope, int Semantics);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p1i32_i32_i32, )(global int *Pointer, int Scope, int Semantics);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p3i32_i32_i32, )(local int *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p4i32_i32_i32, )(generic int *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p0i64_i32_i32, )(private long *Pointer, int Scope, int Semantics);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p1i64_i32_i32, )(global long *Pointer, int Scope, int Semantics);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p3i64_i32_i32, )(local long *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIDecrement, _p4i64_i32_i32, )(generic long *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p0i32_i32_i32_i32, )(private int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p1i32_i32_i32_i32, )(global int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p3i32_i32_i32_i32, )(local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p4i32_i32_i32_i32, )(generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p0i64_i32_i32_i64, )(private long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p1i64_i32_i32_i64, )(global long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p3i64_i32_i32_i64, )(local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicIAdd, _p4i64_i32_i32_i64, )(generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p0i32_i32_i32_i32, )(private int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p1i32_i32_i32_i32, )(global int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p3i32_i32_i32_i32, )(local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p4i32_i32_i32_i32, )(generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_base_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p0i64_i32_i32_i64, )(private long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p1i64_i32_i32_i64, )(global long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p3i64_i32_i32_i64, )(local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicISub, _p4i64_i32_i32_i64, )(generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_base_atomics)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p0i32_i32_i32_i32, )(private int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p1i32_i32_i32_i32, )(global int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p3i32_i32_i32_i32, )(local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p4i32_i32_i32_i32, )(generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p0i64_i32_i32_i64, )(private long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p1i64_i32_i32_i64, )(global long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p3i64_i32_i32_i64, )(local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMin, _p4i64_i32_i32_i64, )(generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p0i32_i32_i32_i32, )(private uint *Pointer, int Scope, int Semantics, uint Value);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p1i32_i32_i32_i32, )(global uint *Pointer, int Scope, int Semantics, uint Value);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p3i32_i32_i32_i32, )(local uint *Pointer, int Scope, int Semantics, uint Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p4i32_i32_i32_i32, )(generic uint *Pointer, int Scope, int Semantics, uint Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p0i64_i32_i32_i64, )(private ulong *Pointer, int Scope, int Semantics, ulong Value);
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p1i64_i32_i32_i64, )(global ulong *Pointer, int Scope, int Semantics, ulong Value);
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p3i64_i32_i32_i64, )(local ulong *Pointer, int Scope, int Semantics, ulong Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMin, _p4i64_i32_i32_i64, )(generic ulong *Pointer, int Scope, int Semantics, ulong Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p0i32_i32_i32_i32, )(private int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p1i32_i32_i32_i32, )(global int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p3i32_i32_i32_i32, )(local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p4i32_i32_i32_i32, )(generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p0i64_i32_i32_i64, )(private long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p1i64_i32_i32_i64, )(global long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p3i64_i32_i32_i64, )(local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicSMax, _p4i64_i32_i32_i64, )(generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p0i32_i32_i32_i32, )(private uint *Pointer, int Scope, int Semantics, uint Value);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p1i32_i32_i32_i32, )(global uint *Pointer, int Scope, int Semantics, uint Value);
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p3i32_i32_i32_i32, )(local uint *Pointer, int Scope, int Semantics, uint Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p4i32_i32_i32_i32, )(generic uint *Pointer, int Scope, int Semantics, uint Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p0i64_i32_i32_i64, )(private ulong *Pointer, int Scope, int Semantics, ulong Value);
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p1i64_i32_i32_i64, )(global ulong *Pointer, int Scope, int Semantics, ulong Value);
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p3i64_i32_i32_i64, )(local ulong *Pointer, int Scope, int Semantics, ulong Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicUMax, _p4i64_i32_i32_i64, )(generic ulong *Pointer, int Scope, int Semantics, ulong Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p0i32_i32_i32_i32, )(private int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p1i32_i32_i32_i32, )(global int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p3i32_i32_i32_i32, )(local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p4i32_i32_i32_i32, )(generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p0i64_i32_i32_i64, )(private long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p1i64_i32_i32_i64, )(global long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p3i64_i32_i32_i64, )(local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicAnd, _p4i64_i32_i32_i64, )(generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p0i32_i32_i32_i32, )(private int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p1i32_i32_i32_i32, )(global int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p3i32_i32_i32_i32, )(local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p4i32_i32_i32_i32, )(generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p0i64_i32_i32_i64, )(private long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p1i64_i32_i32_i64, )(global long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p3i64_i32_i32_i64, )(local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicOr, _p4i64_i32_i32_i64, )(generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p0i32_i32_i32_i32, )(private int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p1i32_i32_i32_i32, )(global int *Pointer, int Scope, int Semantics, int Value);
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p3i32_i32_i32_i32, )(local int *Pointer, int Scope, int Semantics, int Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
int SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p4i32_i32_i32_i32, )(generic int *Pointer, int Scope, int Semantics, int Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_int64_extended_atomics)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p0i64_i32_i32_i64, )(private long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p1i64_i32_i32_i64, )(global long *Pointer, int Scope, int Semantics, long Value);
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p3i64_i32_i32_i64, )(local long *Pointer, int Scope, int Semantics, long Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
long SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicXor, _p4i64_i32_i32_i64, )(generic long *Pointer, int Scope, int Semantics, long Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_int64_extended_atomics)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagTestAndSet, _p0i32_i32_i32, )(private int *Pointer, int Scope, int Semantics);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagTestAndSet, _p1i32_i32_i32, )(global int *Pointer, int Scope, int Semantics);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagTestAndSet, _p3i32_i32_i32, )(local int *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagTestAndSet, _p4i32_i32_i32, )(generic int *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagClear, _p0i32_i32_i32, )(private int *Pointer, int Scope, int Semantics);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagClear, _p1i32_i32_i32, )(global int *Pointer, int Scope, int Semantics);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagClear, _p3i32_i32_i32, )(local int *Pointer, int Scope, int Semantics);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFlagClear, _p4i32_i32_i32, )(generic int *Pointer, int Scope, int Semantics);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
// SPV_EXT_shader_atomic_float_add
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p0f32_i32_i32_f32, )(private float *Pointer, int Scope, int Semantics, float Value);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p1f32_i32_i32_f32, )(global float *Pointer, int Scope, int Semantics, float Value);
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p3f32_i32_i32_f32, )(local float *Pointer, int Scope, int Semantics, float Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p4f32_i32_i32_f32, )(generic float *Pointer, int Scope, int Semantics, float Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p0f64_i32_i32_f64, )(private double *Pointer, int Scope, int Semantics, double Value);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p1f64_i32_i32_f64, )(global double *Pointer, int Scope, int Semantics, double Value);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p3f64_i32_i32_f64, )(local double *Pointer, int Scope, int Semantics, double Value);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFAddEXT, _p4f64_i32_i32_f64, )(generic double *Pointer, int Scope, int Semantics, double Value);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

// SPV_EXT_shader_atomic_float_min_max
#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p0f16_i32_i32_f16, )(private half* Pointer, int Scope, int Semantics, half Value);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p1f16_i32_i32_f16, )(global half* Pointer, int Scope, int Semantics, half Value);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p3f16_i32_i32_f16, )(local half* Pointer, int Scope, int Semantics, half Value);
#endif // defined(cl_khr_fp16)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p0f32_i32_i32_f32, )(private float* Pointer, int Scope, int Semantics, float Value);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p1f32_i32_i32_f32, )(global float* Pointer, int Scope, int Semantics, float Value);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p3f32_i32_i32_f32, )(local float* Pointer, int Scope, int Semantics, float Value);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p0f64_i32_i32_f64, )(private double* Pointer, int Scope, int Semantics, double Value);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p1f64_i32_i32_f64, )(global double* Pointer, int Scope, int Semantics, double Value);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p3f64_i32_i32_f64, )(local double* Pointer, int Scope, int Semantics, double Value);
#endif // defined(cl_khr_fp64)
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p4f16_i32_i32_f16, )(generic half* Pointer, int Scope, int Semantics, half Value);
#endif // defined(cl_khr_fp16)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p4f32_i32_i32_f32, )(generic float* Pointer, int Scope, int Semantics, float Value);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMinEXT, _p4f64_i32_i32_f64, )(generic double* Pointer, int Scope, int Semantics, double Value);
#endif // defined(cl_khr_fp64)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p0f16_i32_i32_f16, )(private half *Pointer, int Scope, int Semantics, half Value);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p1f16_i32_i32_f16, )(global half *Pointer, int Scope, int Semantics, half Value);
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p3f16_i32_i32_f16, )(local half *Pointer, int Scope, int Semantics, half Value);
#endif // defined(cl_khr_fp16)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p0f32_i32_i32_f32, )(private float* Pointer, int Scope, int Semantics, float Value);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p1f32_i32_i32_f32, )(global float* Pointer, int Scope, int Semantics, float Value);
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p3f32_i32_i32_f32, )(local float* Pointer, int Scope, int Semantics, float Value);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p0f64_i32_i32_f64, )(private double* Pointer, int Scope, int Semantics, double Value);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p1f64_i32_i32_f64, )(global double* Pointer, int Scope, int Semantics, double Value);
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p3f64_i32_i32_f64, )(local double* Pointer, int Scope, int Semantics, double Value);
#endif // defined(cl_khr_fp64)
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p4f16_i32_i32_f16, )(generic half *Pointer, int Scope, int Semantics, half Value);
#endif // defined(cl_khr_fp16)
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p4f32_i32_i32_f32, )(generic float* Pointer, int Scope, int Semantics, float Value);
#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(AtomicFMaxEXT, _p4f64_i32_i32_f64, )(generic double* Pointer, int Scope, int Semantics, double Value);
#endif // defined(cl_khr_fp64)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

// Barrier Instructions

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(int Execution, int Memory, int Semantics);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(MemoryBarrier, _i32_i32, )(int Memory, int Semantics);

#ifndef NAMED_BARRIER_STRUCT_TYPE
#define NAMED_BARRIER_STRUCT_TYPE
typedef struct
{
    int count;
    int orig_count;
    int inc;
} __namedBarrier;
#endif

local __namedBarrier* __builtin_spirv_OpNamedBarrierInitialize_i32_p3__namedBarrier_p3i32(int Count, local __namedBarrier* nb_array, local uint* id);
void __builtin_spirv_OpMemoryNamedBarrier_p3__namedBarrier_i32_i32(local __namedBarrier* NB, Scope_t Memory, uint Semantics);

// Group Instructions
// TODO: Do we want to split out size_t into i64 and i32 as we've done here?

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1i8_p3i8_i64_i64_i64(uint Execution, global uchar *Destination, const local uchar *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1i8_p3i8_i32_i32_i64(uint Execution, global uchar *Destination, const local uchar *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1i16_p3i16_i64_i64_i64(uint Execution, global ushort *Destination, const local ushort *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1i16_p3i16_i32_i32_i64(uint Execution, global ushort *Destination, const local ushort *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1i32_p3i32_i64_i64_i64(uint Execution, global uint *Destination, const local uint *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1i32_p3i32_i32_i32_i64(uint Execution, global uint *Destination, const local uint *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1i64_p3i64_i64_i64_i64(uint Execution, global ulong *Destination, const local ulong *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1i64_p3i64_i32_i32_i64(uint Execution, global ulong *Destination, const local ulong *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1f16_p3f16_i64_i64_i64(uint Execution, global half *Destination, const local half *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1f16_p3f16_i32_i32_i64(uint Execution, global half *Destination, const local half *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1f32_p3f32_i64_i64_i64(uint Execution, global float *Destination, const local float *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1f32_p3f32_i32_i32_i64(uint Execution, global float *Destination, const local float *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2i8_p3v2i8_i64_i64_i64(uint Execution, global uchar2 *Destination, const local uchar2 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2i8_p3v2i8_i32_i32_i64(uint Execution, global uchar2 *Destination, const local uchar2 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3i8_p3v3i8_i64_i64_i64(uint Execution, global uchar3 *Destination, const local uchar3 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3i8_p3v3i8_i32_i32_i64(uint Execution, global uchar3 *Destination, const local uchar3 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4i8_p3v4i8_i64_i64_i64(uint Execution, global uchar4 *Destination, const local uchar4 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4i8_p3v4i8_i32_i32_i64(uint Execution, global uchar4 *Destination, const local uchar4 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8i8_p3v8i8_i64_i64_i64(uint Execution, global uchar8 *Destination, const local uchar8 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8i8_p3v8i8_i32_i32_i64(uint Execution, global uchar8 *Destination, const local uchar8 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16i8_p3v16i8_i64_i64_i64(uint Execution, global uchar16 *Destination, const local uchar16 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16i8_p3v16i8_i32_i32_i64(uint Execution, global uchar16 *Destination, const local uchar16 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2i16_p3v2i16_i64_i64_i64(uint Execution, global ushort2 *Destination, const local ushort2 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2i16_p3v2i16_i32_i32_i64(uint Execution, global ushort2 *Destination, const local ushort2 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3i16_p3v3i16_i64_i64_i64(uint Execution, global ushort3 *Destination, const local ushort3 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3i16_p3v3i16_i32_i32_i64(uint Execution, global ushort3 *Destination, const local ushort3 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4i16_p3v4i16_i64_i64_i64(uint Execution, global ushort4 *Destination, const local ushort4 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4i16_p3v4i16_i32_i32_i64(uint Execution, global ushort4 *Destination, const local ushort4 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8i16_p3v8i16_i64_i64_i64(uint Execution, global ushort8 *Destination, const local ushort8 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8i16_p3v8i16_i32_i32_i64(uint Execution, global ushort8 *Destination, const local ushort8 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16i16_p3v16i16_i64_i64_i64(uint Execution, global ushort16 *Destination, const local ushort16 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16i16_p3v16i16_i32_i32_i64(uint Execution, global ushort16 *Destination, const local ushort16 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2i32_p3v2i32_i64_i64_i64(uint Execution, global uint2 *Destination, const local uint2 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2i32_p3v2i32_i32_i32_i64(uint Execution, global uint2 *Destination, const local uint2 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3i32_p3v3i32_i64_i64_i64(uint Execution, global uint3 *Destination, const local uint3 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3i32_p3v3i32_i32_i32_i64(uint Execution, global uint3 *Destination, const local uint3 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4i32_p3v4i32_i64_i64_i64(uint Execution, global uint4 *Destination, const local uint4 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4i32_p3v4i32_i32_i32_i64(uint Execution, global uint4 *Destination, const local uint4 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8i32_p3v8i32_i64_i64_i64(uint Execution, global uint8 *Destination, const local uint8 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8i32_p3v8i32_i32_i32_i64(uint Execution, global uint8 *Destination, const local uint8 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16i32_p3v16i32_i64_i64_i64(uint Execution, global uint16 *Destination, const local uint16 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16i32_p3v16i32_i32_i32_i64(uint Execution, global uint16 *Destination, const local uint16 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2i64_p3v2i64_i64_i64_i64(uint Execution, global ulong2 *Destination, const local ulong2 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2i64_p3v2i64_i32_i32_i64(uint Execution, global ulong2 *Destination, const local ulong2 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3i64_p3v3i64_i64_i64_i64(uint Execution, global ulong3 *Destination, const local ulong3 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3i64_p3v3i64_i32_i32_i64(uint Execution, global ulong3 *Destination, const local ulong3 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4i64_p3v4i64_i64_i64_i64(uint Execution, global ulong4 *Destination, const local ulong4 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4i64_p3v4i64_i32_i32_i64(uint Execution, global ulong4 *Destination, const local ulong4 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8i64_p3v8i64_i64_i64_i64(uint Execution, global ulong8 *Destination, const local ulong8 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8i64_p3v8i64_i32_i32_i64(uint Execution, global ulong8 *Destination, const local ulong8 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16i64_p3v16i64_i64_i64_i64(uint Execution, global ulong16 *Destination, const local ulong16 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16i64_p3v16i64_i32_i32_i64(uint Execution, global ulong16 *Destination, const local ulong16 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2f16_p3v2f16_i64_i64_i64(uint Execution, global half2 *Destination, const local half2 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2f16_p3v2f16_i32_i32_i64(uint Execution, global half2 *Destination, const local half2 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3f16_p3v3f16_i64_i64_i64(uint Execution, global half3 *Destination, const local half3 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3f16_p3v3f16_i32_i32_i64(uint Execution, global half3 *Destination, const local half3 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4f16_p3v4f16_i64_i64_i64(uint Execution, global half4 *Destination, const local half4 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4f16_p3v4f16_i32_i32_i64(uint Execution, global half4 *Destination, const local half4 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8f16_p3v8f16_i64_i64_i64(uint Execution, global half8 *Destination, const local half8 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8f16_p3v8f16_i32_i32_i64(uint Execution, global half8 *Destination, const local half8 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16f16_p3v16f16_i64_i64_i64(uint Execution, global half16 *Destination, const local half16 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16f16_p3v16f16_i32_i32_i64(uint Execution, global half16 *Destination, const local half16 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2f32_p3v2f32_i64_i64_i64(uint Execution, global float2 *Destination, const local float2 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2f32_p3v2f32_i32_i32_i64(uint Execution, global float2 *Destination, const local float2 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3f32_p3v3f32_i64_i64_i64(uint Execution, global float3 *Destination, const local float3 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3f32_p3v3f32_i32_i32_i64(uint Execution, global float3 *Destination, const local float3 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4f32_p3v4f32_i64_i64_i64(uint Execution, global float4 *Destination, const local float4 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4f32_p3v4f32_i32_i32_i64(uint Execution, global float4 *Destination, const local float4 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8f32_p3v8f32_i64_i64_i64(uint Execution, global float8 *Destination, const local float8 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8f32_p3v8f32_i32_i32_i64(uint Execution, global float8 *Destination, const local float8 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16f32_p3v16f32_i64_i64_i64(uint Execution, global float16 *Destination, const local float16 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16f32_p3v16f32_i32_i32_i64(uint Execution, global float16 *Destination, const local float16 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3i8_p1i8_i64_i64_i64(uint Execution, local uchar *Destination, const global uchar *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3i8_p1i8_i32_i32_i64(uint Execution, local uchar *Destination, const global uchar *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3i16_p1i16_i64_i64_i64(uint Execution, local ushort *Destination, const global ushort *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3i16_p1i16_i32_i32_i64(uint Execution, local ushort *Destination, const global ushort *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3i32_p1i32_i64_i64_i64(uint Execution, local uint *Destination, const global uint *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3i32_p1i32_i32_i32_i64(uint Execution, local uint *Destination, const global uint *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3i64_p1i64_i64_i64_i64(uint Execution, local ulong *Destination, const global ulong *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3i64_p1i64_i32_i32_i64(uint Execution, local ulong *Destination, const global ulong *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3f16_p1f16_i64_i64_i64(uint Execution, local half *Destination, const global half *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3f16_p1f16_i32_i32_i64(uint Execution, local half *Destination, const global half *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3f32_p1f32_i64_i64_i64(uint Execution, local float *Destination, const global float *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2i8_p1v2i8_i64_i64_i64(uint Execution, local uchar2 *Destination, const global uchar2 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2i8_p1v2i8_i32_i32_i64(uint Execution, local uchar2 *Destination, const global uchar2 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3i8_p1v3i8_i64_i64_i64(uint Execution, local uchar3 *Destination, const global uchar3 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3i8_p1v3i8_i32_i32_i64(uint Execution, local uchar3 *Destination, const global uchar3 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4i8_p1v4i8_i64_i64_i64(uint Execution, local uchar4 *Destination, const global uchar4 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4i8_p1v4i8_i32_i32_i64(uint Execution, local uchar4 *Destination, const global uchar4 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8i8_p1v8i8_i64_i64_i64(uint Execution, local uchar8 *Destination, const global uchar8 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8i8_p1v8i8_i32_i32_i64(uint Execution, local uchar8 *Destination, const global uchar8 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16i8_p1v16i8_i64_i64_i64(uint Execution, local uchar16 *Destination, const global uchar16 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16i8_p1v16i8_i32_i32_i64(uint Execution, local uchar16 *Destination, const global uchar16 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2i16_p1v2i16_i64_i64_i64(uint Execution, local ushort2 *Destination, const global ushort2 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2i16_p1v2i16_i32_i32_i64(uint Execution, local ushort2 *Destination, const global ushort2 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3i16_p1v3i16_i64_i64_i64(uint Execution, local ushort3 *Destination, const global ushort3 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3i16_p1v3i16_i32_i32_i64(uint Execution, local ushort3 *Destination, const global ushort3 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4i16_p1v4i16_i64_i64_i64(uint Execution, local ushort4 *Destination, const global ushort4 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4i16_p1v4i16_i32_i32_i64(uint Execution, local ushort4 *Destination, const global ushort4 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8i16_p1v8i16_i64_i64_i64(uint Execution, local ushort8 *Destination, const global ushort8 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8i16_p1v8i16_i32_i32_i64(uint Execution, local ushort8 *Destination, const global ushort8 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16i16_p1v16i16_i64_i64_i64(uint Execution, local ushort16 *Destination, const global ushort16 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16i16_p1v16i16_i32_i32_i64(uint Execution, local ushort16 *Destination, const global ushort16 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2i32_p1v2i32_i64_i64_i64(uint Execution, local uint2 *Destination, const global uint2 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2i32_p1v2i32_i32_i32_i64(uint Execution, local uint2 *Destination, const global uint2 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3i32_p1v3i32_i64_i64_i64(uint Execution, local uint3 *Destination, const global uint3 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3i32_p1v3i32_i32_i32_i64(uint Execution, local uint3 *Destination, const global uint3 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4i32_p1v4i32_i64_i64_i64(uint Execution, local uint4 *Destination, const global uint4 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4i32_p1v4i32_i32_i32_i64(uint Execution, local uint4 *Destination, const global uint4 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8i32_p1v8i32_i64_i64_i64(uint Execution, local uint8 *Destination, const global uint8 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8i32_p1v8i32_i32_i32_i64(uint Execution, local uint8 *Destination, const global uint8 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16i32_p1v16i32_i64_i64_i64(uint Execution, local uint16 *Destination, const global uint16 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16i32_p1v16i32_i32_i32_i64(uint Execution, local uint16 *Destination, const global uint16 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2i64_p1v2i64_i64_i64_i64(uint Execution, local ulong2 *Destination, const global ulong2 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2i64_p1v2i64_i32_i32_i64(uint Execution, local ulong2 *Destination, const global ulong2 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3i64_p1v3i64_i64_i64_i64(uint Execution, local ulong3 *Destination, const global ulong3 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3i64_p1v3i64_i32_i32_i64(uint Execution, local ulong3 *Destination, const global ulong3 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4i64_p1v4i64_i64_i64_i64(uint Execution, local ulong4 *Destination, const global ulong4 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4i64_p1v4i64_i32_i32_i64(uint Execution, local ulong4 *Destination, const global ulong4 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8i64_p1v8i64_i64_i64_i64(uint Execution, local ulong8 *Destination, const global ulong8 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8i64_p1v8i64_i32_i32_i64(uint Execution, local ulong8 *Destination, const global ulong8 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16i64_p1v16i64_i64_i64_i64(uint Execution, local ulong16 *Destination, const global ulong16 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16i64_p1v16i64_i32_i32_i64(uint Execution, local ulong16 *Destination, const global ulong16 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2f16_p1v2f16_i64_i64_i64(uint Execution, local half2 *Destination, const global half2 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2f16_p1v2f16_i32_i32_i64(uint Execution, local half2 *Destination, const global half2 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3f16_p1v3f16_i64_i64_i64(uint Execution, local half3 *Destination, const global half3 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3f16_p1v3f16_i32_i32_i64(uint Execution, local half3 *Destination, const global half3 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4f16_p1v4f16_i64_i64_i64(uint Execution, local half4 *Destination, const global half4 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4f16_p1v4f16_i32_i32_i64(uint Execution, local half4 *Destination, const global half4 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8f16_p1v8f16_i64_i64_i64(uint Execution, local half8 *Destination, const global half8 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8f16_p1v8f16_i32_i32_i64(uint Execution, local half8 *Destination, const global half8 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16f16_p1v16f16_i64_i64_i64(uint Execution, local half16 *Destination, const global half16 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16f16_p1v16f16_i32_i32_i64(uint Execution, local half16 *Destination, const global half16 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2f32_p1v2f32_i64_i64_i64(uint Execution, local float2 *Destination, const global float2 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2f32_p1v2f32_i32_i32_i64(uint Execution, local float2 *Destination, const global float2 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3f32_p1v3f32_i64_i64_i64(uint Execution, local float3 *Destination, const global float3 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3f32_p1v3f32_i32_i32_i64(uint Execution, local float3 *Destination, const global float3 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4f32_p1v4f32_i64_i64_i64(uint Execution, local float4 *Destination, const global float4 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4f32_p1v4f32_i32_i32_i64(uint Execution, local float4 *Destination, const global float4 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8f32_p1v8f32_i64_i64_i64(uint Execution, local float8 *Destination, const global float8 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8f32_p1v8f32_i32_i32_i64(uint Execution, local float8 *Destination, const global float8 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16f32_p1v16f32_i64_i64_i64(uint Execution, local float16 *Destination, const global float16 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16f32_p1v16f32_i32_i32_i64(uint Execution, local float16 *Destination, const global float16 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3f32_p1f32_i32_i32_i64(uint Execution, local float *Destination, const global float *Source, uint NumElements, uint Stride, Event_t Event);
#if defined(cl_khr_fp64)
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1f64_p3f64_i64_i64_i64(uint Execution, global double *Destination, const local double *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1f64_p3f64_i32_i32_i64(uint Execution, global double *Destination, const local double *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2f64_p3v2f64_i64_i64_i64(uint Execution, global double2 *Destination, const local double2 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2f64_p3v2f64_i32_i32_i64(uint Execution, global double2 *Destination, const local double2 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3f64_p3v3f64_i64_i64_i64(uint Execution, global double3 *Destination, const local double3 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3f64_p3v3f64_i32_i32_i64(uint Execution, global double3 *Destination, const local double3 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4f64_p3v4f64_i64_i64_i64(uint Execution, global double4 *Destination, const local double4 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4f64_p3v4f64_i32_i32_i64(uint Execution, global double4 *Destination, const local double4 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8f64_p3v8f64_i64_i64_i64(uint Execution, global double8 *Destination, const local double8 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8f64_p3v8f64_i32_i32_i64(uint Execution, global double8 *Destination, const local double8 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16f64_p3v16f64_i64_i64_i64(uint Execution, global double16 *Destination, const local double16 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16f64_p3v16f64_i32_i32_i64(uint Execution, global double16 *Destination, const local double16 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3f64_p1f64_i64_i64_i64(uint Execution, local double *Destination, const global double *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3f64_p1f64_i32_i32_i64(uint Execution, local double *Destination, const global double *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2f64_p1v2f64_i64_i64_i64(uint Execution, local double2 *Destination, const global double2 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2f64_p1v2f64_i32_i32_i64(uint Execution, local double2 *Destination, const global double2 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3f64_p1v3f64_i64_i64_i64(uint Execution, local double3 *Destination, const global double3 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3f64_p1v3f64_i32_i32_i64(uint Execution, local double3 *Destination, const global double3 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4f64_p1v4f64_i64_i64_i64(uint Execution, local double4 *Destination, const global double4 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4f64_p1v4f64_i32_i32_i64(uint Execution, local double4 *Destination, const global double4 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8f64_p1v8f64_i64_i64_i64(uint Execution, local double8 *Destination, const global double8 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8f64_p1v8f64_i32_i32_i64(uint Execution, local double8 *Destination, const global double8 *Source, uint NumElements, uint Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16f64_p1v16f64_i64_i64_i64(uint Execution, local double16 *Destination, const global double16 *Source, ulong NumElements, ulong Stride, Event_t Event);
Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16f64_p1v16f64_i32_i32_i64(uint Execution, local double16 *Destination, const global double16 *Source, uint NumElements, uint Stride, Event_t Event);
#endif // defined(cl_khr_fp64)

void __builtin_spirv_OpGroupWaitEvents_i32_i32_p0i64(uint Execution, uint NumEvents, private Event_t *EventsList);
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void __builtin_spirv_OpGroupWaitEvents_i32_i32_p4i64(uint Execution, uint NumEvents, generic Event_t *EventsList);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#if defined(cl_khr_subgroup_non_uniform_vote)
bool __builtin_spirv_OpGroupNonUniformElect_i32(uint Execution);
bool __builtin_spirv_OpGroupNonUniformAll_i32_i1(uint Execution, bool Predicate);
bool __builtin_spirv_OpGroupNonUniformAny_i32_i1(uint Execution, bool Predicate);
bool __builtin_spirv_OpGroupNonUniformAllEqual_i32_i8(uint Execution, uchar Value);
bool __builtin_spirv_OpGroupNonUniformAllEqual_i32_i16(uint Execution, ushort Value);
bool __builtin_spirv_OpGroupNonUniformAllEqual_i32_i32(uint Execution, uint Value);
bool __builtin_spirv_OpGroupNonUniformAllEqual_i32_i64(uint Execution, ulong Value);
bool __builtin_spirv_OpGroupNonUniformAllEqual_i32_f32(uint Execution, float Value);
#if defined(cl_khr_fp64)
bool __builtin_spirv_OpGroupNonUniformAllEqual_i32_f64(uint Execution, double Value);
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
bool __builtin_spirv_OpGroupNonUniformAllEqual_i32_f16(uint Execution, half Value);
#endif // defined(cl_khr_fp16)
#endif // defined(cl_khr_subgroup_non_uniform_vote)

bool __builtin_spirv_OpGroupAll_i32_i1(uint Execution, bool Predicate);
bool __builtin_spirv_OpGroupAny_i32_i1(uint Execution, bool Predicate);
#define DECL_SUB_GROUP_BROADCAST_BASE(TYPE, TYPE_ABBR)                                                      \
TYPE __builtin_spirv_OpGroupBroadcast_i32_##TYPE_ABBR##_v3i32(uint Execution, TYPE Value, uint3 LocalId);   \
TYPE __builtin_spirv_OpGroupBroadcast_i32_##TYPE_ABBR##_v3i64(uint Execution, TYPE Value, ulong3 LocalId);  \
TYPE __builtin_spirv_OpGroupBroadcast_i32_##TYPE_ABBR##_v2i32(uint Execution, TYPE Value, uint2 LocalId);   \
TYPE __builtin_spirv_OpGroupBroadcast_i32_##TYPE_ABBR##_v2i64(uint Execution, TYPE Value, ulong2 LocalId);  \
TYPE __builtin_spirv_OpGroupBroadcast_i32_##TYPE_ABBR##_i32(uint Execution, TYPE Value, uint LocalId);      \
TYPE __builtin_spirv_OpGroupBroadcast_i32_##TYPE_ABBR##_i64(uint Execution, TYPE Value, ulong LocalId);

#define DECL_SUB_GROUP_BROADCAST(TYPE, TYPE_ABBR)       \
DECL_SUB_GROUP_BROADCAST_BASE(TYPE, TYPE_ABBR)          \
DECL_SUB_GROUP_BROADCAST_BASE(TYPE##2, v2##TYPE_ABBR)   \
DECL_SUB_GROUP_BROADCAST_BASE(TYPE##3, v3##TYPE_ABBR)   \
DECL_SUB_GROUP_BROADCAST_BASE(TYPE##4, v4##TYPE_ABBR)   \
DECL_SUB_GROUP_BROADCAST_BASE(TYPE##8, v8##TYPE_ABBR)   \
DECL_SUB_GROUP_BROADCAST_BASE(TYPE##16, v16##TYPE_ABBR)

DECL_SUB_GROUP_BROADCAST(uchar, i8)
DECL_SUB_GROUP_BROADCAST(ushort, i16)
DECL_SUB_GROUP_BROADCAST(uint, i32)
DECL_SUB_GROUP_BROADCAST(ulong, i64)
DECL_SUB_GROUP_BROADCAST(float, f32)
#if defined(cl_khr_fp16)
DECL_SUB_GROUP_BROADCAST(half, f16)
#endif // defined(cl_khr_fp16)
#if defined(cl_khr_fp64)
DECL_SUB_GROUP_BROADCAST(double, f64)
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_subgroup_ballot)
#define DECL_NON_UNIFORM_BROADCAST_BASE(TYPE, TYPE_ABBR)                        \
    TYPE __builtin_spirv_OpGroupNonUniformBroadcast_i32_##TYPE_ABBR##_i32(      \
        uint Execution, TYPE Value, uint Id);                                   \
    TYPE __builtin_spirv_OpGroupNonUniformBroadcastFirst_i32_##TYPE_ABBR(uint Execution, TYPE Value);

#define DECL_NON_UNIFORM_BROADCAST(TYPE, TYPE_ABBR)         \
    DECL_NON_UNIFORM_BROADCAST_BASE(TYPE, TYPE_ABBR)        \
    DECL_NON_UNIFORM_BROADCAST_BASE(TYPE##2, v2##TYPE_ABBR) \
    DECL_NON_UNIFORM_BROADCAST_BASE(TYPE##3, v3##TYPE_ABBR) \
    DECL_NON_UNIFORM_BROADCAST_BASE(TYPE##4, v4##TYPE_ABBR) \
    DECL_NON_UNIFORM_BROADCAST_BASE(TYPE##8, v8##TYPE_ABBR) \
    DECL_NON_UNIFORM_BROADCAST_BASE(TYPE##16, v16##TYPE_ABBR)

DECL_NON_UNIFORM_BROADCAST(uchar,  i8)
DECL_NON_UNIFORM_BROADCAST(ushort, i16)
DECL_NON_UNIFORM_BROADCAST(uint,   i32)
DECL_NON_UNIFORM_BROADCAST(ulong,  i64)
DECL_NON_UNIFORM_BROADCAST(float,  f32)
#if defined(cl_khr_fp64)
DECL_NON_UNIFORM_BROADCAST(double, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DECL_NON_UNIFORM_BROADCAST(half,   f16)
#endif // defined(cl_khr_fp16)

uint4 __builtin_spirv_OpGroupNonUniformBallot_i32_i1(uint Execution, bool Predicate);
bool __builtin_spirv_OpGroupNonUniformInverseBallot_i32_v4i32(uint Execution, uint4 Value);
bool __builtin_spirv_OpGroupNonUniformBallotBitExtract_i32_v4i32_i32(uint Execution, uint4 Value, uint Index);
uint __builtin_spirv_OpGroupNonUniformBallotBitCount_i32_i32_v4i32(uint Execution, uint Operation, uint4 Value);
uint __builtin_spirv_OpGroupNonUniformBallotFindLSB_i32_v4i32(uint Execution, uint4 Value);
uint __builtin_spirv_OpGroupNonUniformBallotFindMSB_i32_v4i32(uint Execution, uint4 Value);
uint4 __builtin_spirv_BuiltInSubgroupEqMask();
uint4 __builtin_spirv_BuiltInSubgroupGeMask();
uint4 __builtin_spirv_BuiltInSubgroupGtMask();
uint4 __builtin_spirv_BuiltInSubgroupLeMask();
uint4 __builtin_spirv_BuiltInSubgroupLtMask();
#endif // defined(cl_khr_subgroup_ballot)

#if defined(cl_khr_subgroup_shuffle)
#define DECL_NON_UNIFORM_SHUFFLE(TYPE, TYPE_ABBR)                           \
    TYPE __builtin_spirv_OpGroupNonUniformShuffle_i32_##TYPE_ABBR##_i32(    \
        uint Execution, TYPE Value, uint Id);                               \
    TYPE __builtin_spirv_OpGroupNonUniformShuffleXor_i32_##TYPE_ABBR##_i32(uint Execution, TYPE Value, uint Mask);

DECL_NON_UNIFORM_SHUFFLE(uchar,  i8)
DECL_NON_UNIFORM_SHUFFLE(ushort, i16)
DECL_NON_UNIFORM_SHUFFLE(uint,   i32)
DECL_NON_UNIFORM_SHUFFLE(ulong,  i64)
DECL_NON_UNIFORM_SHUFFLE(float,  f32)
#if defined(cl_khr_fp64)
DECL_NON_UNIFORM_SHUFFLE(double, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DECL_NON_UNIFORM_SHUFFLE(half,   f16)
#endif // defined(cl_khr_fp16)
#endif // defined(cl_khr_subgroup_shuffle)

#if defined(cl_khr_subgroup_shuffle_relative)
#define DECL_NON_UNIFORM_SHUFFLE_RELATIVE(TYPE, TYPE_ABBR)                   \
    TYPE __builtin_spirv_OpGroupNonUniformShuffleUp_i32_##TYPE_ABBR##_i32(   \
        uint Execution, TYPE Value, uint Delta);                             \
    TYPE __builtin_spirv_OpGroupNonUniformShuffleDown_i32_##TYPE_ABBR##_i32(uint Execution, TYPE Value, uint Delta);

DECL_NON_UNIFORM_SHUFFLE_RELATIVE(uchar,  i8)
DECL_NON_UNIFORM_SHUFFLE_RELATIVE(ushort, i16)
DECL_NON_UNIFORM_SHUFFLE_RELATIVE(uint,   i32)
DECL_NON_UNIFORM_SHUFFLE_RELATIVE(ulong,  i64)
DECL_NON_UNIFORM_SHUFFLE_RELATIVE(float,  f32)
#if defined(cl_khr_fp64)
DECL_NON_UNIFORM_SHUFFLE_RELATIVE(double, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DECL_NON_UNIFORM_SHUFFLE_RELATIVE(half,   f16)
#endif // defined(cl_khr_fp16)
#endif // defined(cl_khr_subgroup_shuffle_relative)

#if defined(cl_khr_subgroup_non_uniform_arithmetic) || defined(cl_khr_subgroup_clustered_reduce)
#define DEFN_NON_UNIFORM_OPERATION_BASE(TYPE, OPERATION, TYPE_ABBR)
#define DEFN_NON_UNIFORM_CLUSTERED_OPERATION(TYPE, OPERATION, TYPE_ABBR)

#if defined(cl_khr_subgroup_non_uniform_arithmetic)
#define DEFN_NON_UNIFORM_OPERATION_BASE(TYPE, OPERATION, TYPE_ABBR)   \
TYPE  __builtin_spirv_OpGroupNonUniform##OPERATION##_i32_i32_##TYPE_ABBR(uint Execution, uint Operation, TYPE X);
#endif // defined(cl_khr_subgroup_non_uniform_arithmetic)

#if defined(cl_khr_subgroup_clustered_reduce)
#define DEFN_NON_UNIFORM_CLUSTERED_OPERATION(TYPE, OPERATION, TYPE_ABBR)   \
TYPE  __builtin_spirv_OpGroupNonUniform##OPERATION##_i32_i32_##TYPE_ABBR##_i32(uint Execution, uint Operation, TYPE X, uint ClusterSize);
#endif // defined(cl_khr_subgroup_clustered_reduce)

#define DEFN_NON_UNIFORM_OPERATION(TYPE, OPERATION, TYPE_ABBR)     \
DEFN_NON_UNIFORM_OPERATION_BASE(TYPE, OPERATION, TYPE_ABBR)        \
DEFN_NON_UNIFORM_CLUSTERED_OPERATION(TYPE, OPERATION, TYPE_ABBR)

// ARITHMETIC OPERATIONS

// - OpGroupNonUniformIAdd, OpGroupNonUniformFAdd
// - OpGroupNonUniformIMul, OpGroupNonUniformFMul
#define DEFN_NON_UNIFORM_ADD_MUL(TYPE, TYPE_SIGN, TYPE_ABBR)   \
DEFN_NON_UNIFORM_OPERATION(TYPE, TYPE_SIGN##Add, TYPE_ABBR)    \
DEFN_NON_UNIFORM_OPERATION(TYPE, TYPE_SIGN##Mul, TYPE_ABBR)

DEFN_NON_UNIFORM_ADD_MUL(uchar,  I, i8)
DEFN_NON_UNIFORM_ADD_MUL(ushort, I, i16)
DEFN_NON_UNIFORM_ADD_MUL(uint,   I, i32)
DEFN_NON_UNIFORM_ADD_MUL(ulong,  I, i64)
DEFN_NON_UNIFORM_ADD_MUL(float,  F, f32)
#if defined(cl_khr_fp64)
DEFN_NON_UNIFORM_ADD_MUL(double, F, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_ADD_MUL(half,   F, f16)
#endif // defined(cl_khr_fp16)

// - OpGroupNonUniformSMin, OpGroupNonUniformUMin, OpGroupNonUniformFMin
// - OpGroupNonUniformSMax, OpGroupNonUniformUMax, OpGroupNonUniformFMax
#define DEFN_NON_UNIFORM_MIN_MAX(TYPE, TYPE_SIGN, TYPE_ABBR)   \
DEFN_NON_UNIFORM_OPERATION(TYPE, TYPE_SIGN##Min, TYPE_ABBR)    \
DEFN_NON_UNIFORM_OPERATION(TYPE, TYPE_SIGN##Max, TYPE_ABBR)

DEFN_NON_UNIFORM_MIN_MAX(char,   S, i8)
DEFN_NON_UNIFORM_MIN_MAX(uchar,  U, i8)
DEFN_NON_UNIFORM_MIN_MAX(short,  S, i16)
DEFN_NON_UNIFORM_MIN_MAX(ushort, U, i16)
DEFN_NON_UNIFORM_MIN_MAX(int,    S, i32)
DEFN_NON_UNIFORM_MIN_MAX(uint,   U, i32)
DEFN_NON_UNIFORM_MIN_MAX(long,   S, i64)
DEFN_NON_UNIFORM_MIN_MAX(ulong,  U, i64)
DEFN_NON_UNIFORM_MIN_MAX(float,  F, f32)
#if defined(cl_khr_fp64)
DEFN_NON_UNIFORM_MIN_MAX(double, F, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_MIN_MAX(half,   F, f16)
#endif // defined(cl_khr_fp16)

// BITWISE OPERATIONS

// - OpGroupNonUniformBitwiseAnd
// - OpGroupNonUniformBitwiseOr
// - OpGroupNonUniformBitwiseXor
#define DEFN_NON_UNIFORM_BITWISE_OPERATIONS(TYPE, TYPE_ABBR)  \
DEFN_NON_UNIFORM_OPERATION(TYPE, BitwiseAnd, TYPE_ABBR)       \
DEFN_NON_UNIFORM_OPERATION(TYPE, BitwiseOr, TYPE_ABBR)        \
DEFN_NON_UNIFORM_OPERATION(TYPE, BitwiseXor, TYPE_ABBR)

DEFN_NON_UNIFORM_BITWISE_OPERATIONS(uchar,  i8)
DEFN_NON_UNIFORM_BITWISE_OPERATIONS(ushort, i16)
DEFN_NON_UNIFORM_BITWISE_OPERATIONS(uint,   i32)
DEFN_NON_UNIFORM_BITWISE_OPERATIONS(ulong,  i64)

// LOGICAL OPERATIONS

// - OpGroupNonUniformLogicalAnd
// - OpGroupNonUniformLogicalOr
// - OpGroupNonUniformLogicalXor
#define DEFN_NON_UNIFORM_LOGICAL_OPERATIONS(TYPE, TYPE_ABBR)  \
DEFN_NON_UNIFORM_OPERATION(TYPE, LogicalAnd, TYPE_ABBR)       \
DEFN_NON_UNIFORM_OPERATION(TYPE, LogicalOr, TYPE_ABBR)        \
DEFN_NON_UNIFORM_OPERATION(TYPE, LogicalXor, TYPE_ABBR)

DEFN_NON_UNIFORM_LOGICAL_OPERATIONS(bool, i1)
#endif // defined(cl_khr_subgroup_non_uniform_arithmetic) || defined(cl_khr_subgroup_clustered_reduce)

uchar  __builtin_spirv_OpGroupIAdd_i32_i32_i8(uint Execution, uint Operation, uchar X);
ushort __builtin_spirv_OpGroupIAdd_i32_i32_i16(uint Execution, uint Operation, ushort X);
uint   __builtin_spirv_OpGroupIAdd_i32_i32_i32(uint Execution, uint Operation, uint X);
ulong  __builtin_spirv_OpGroupIAdd_i32_i32_i64(uint Execution, uint Operation, ulong X);

half   __builtin_spirv_OpGroupFAdd_i32_i32_f16(uint Execution, uint Operation, half X);
float  __builtin_spirv_OpGroupFAdd_i32_i32_f32(uint Execution, uint Operation, float X);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpGroupFAdd_i32_i32_f64(uint Execution, uint Operation, double X);
#endif // defined(cl_khr_fp64)

uchar  __builtin_spirv_OpGroupUMin_i32_i32_i8(uint Execution, uint Operation, uchar X);
ushort __builtin_spirv_OpGroupUMin_i32_i32_i16(uint Execution, uint Operation, ushort X);
uint   __builtin_spirv_OpGroupUMin_i32_i32_i32(uint Execution, uint Operation, uint X);
ulong  __builtin_spirv_OpGroupUMin_i32_i32_i64(uint Execution, uint Operation, ulong X);

half   __builtin_spirv_OpGroupFMin_i32_i32_f16(uint Execution, uint Operation, half X);
float  __builtin_spirv_OpGroupFMin_i32_i32_f32(uint Execution, uint Operation, float X);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpGroupFMin_i32_i32_f64(uint Execution, uint Operation, double X);
#endif // defined(cl_khr_fp64)

char   __builtin_spirv_OpGroupSMin_i32_i32_i8(uint Execution, uint Operation, char X);
short  __builtin_spirv_OpGroupSMin_i32_i32_i16(uint Execution, uint Operation, short X);
int    __builtin_spirv_OpGroupSMin_i32_i32_i32(uint Execution, uint Operation, int X);
long   __builtin_spirv_OpGroupSMin_i32_i32_i64(uint Execution, uint Operation, long X);

half   __builtin_spirv_OpGroupFMax_i32_i32_f16(uint Execution, uint Operation, half X);
float  __builtin_spirv_OpGroupFMax_i32_i32_f32(uint Execution, uint Operation, float X);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpGroupFMax_i32_i32_f64(uint Execution, uint Operation, double X);
#endif // defined(cl_khr_fp64)

uchar  __builtin_spirv_OpGroupUMax_i32_i32_i8(uint Execution, uint Operation, uchar X);
ushort __builtin_spirv_OpGroupUMax_i32_i32_i16(uint Execution, uint Operation, ushort X);
uint   __builtin_spirv_OpGroupUMax_i32_i32_i32(uint Execution, uint Operation, uint X);
ulong  __builtin_spirv_OpGroupUMax_i32_i32_i64(uint Execution, uint Operation, ulong X);

char  __builtin_spirv_OpGroupSMax_i32_i32_i8(uint Execution, uint Operation, char X);
short __builtin_spirv_OpGroupSMax_i32_i32_i16(uint Execution, uint Operation, short X);
int   __builtin_spirv_OpGroupSMax_i32_i32_i32(uint Execution, uint Operation, int X);
long  __builtin_spirv_OpGroupSMax_i32_i32_i64(uint Execution, uint Operation, long X);

// Device-Side Enqueue Instructions
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
uint __builtin_spirv_OpEnqueueMarker_i64_i32_p0i64_p0i64(Queue_t Queue, uint NumEvents, private ClkEvent_t *WaitEvents, private ClkEvent_t *RetEvent);
uint __builtin_spirv_OpEnqueueMarker_i64_i32_p3i64_p3i64(Queue_t Queue, uint NumEvents, local ClkEvent_t *WaitEvents, local ClkEvent_t *RetEvent);
uint __builtin_spirv_OpEnqueueMarker_i64_i32_p4i64_p4i64(Queue_t Queue, uint NumEvents, generic ClkEvent_t *WaitEvents, generic ClkEvent_t *RetEvent);

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

typedef struct
{
    uint  dim;
    size_t globalWorkOffset[3];
    size_t globalWorkSize[3];
    size_t localWorkSize[3];
} Ndrange_t;

// OpEnqueueKernel will be custom lowered?
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
uint __builtin_spirv_OpGetKernelWorkGroupSize_fp0i32_p0i8_i32_i32(uchar* Invoke, private uchar *Param, uint ParamSize, uint ParamAlign);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(RetainEvent, _i64, )(ClkEvent_t Event);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(ReleaseEvent, _i64, )(ClkEvent_t Event);
ClkEvent_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(CreateUserEvent, , )(void);
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsValidEvent, _i64, )(ClkEvent_t Event);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(SetUserEventStatus, _i64_i32, )(ClkEvent_t Event, int Status);
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(CaptureEventProfilingInfo, _i64_i32_p1i8, )(ClkEvent_t Event, int ProfilingInfo, global char *Value);
Queue_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GetDefaultQueue, , )(void);

Ndrange_t __builtin_spirv_OpBuildNDRange_i32_i32_i32(uint GlobalWorkSize, uint LocalWorkSize, uint GlobalWorkOffset);
Ndrange_t __builtin_spirv_OpBuildNDRange_i64_i64_i64(ulong GlobalWorkSize, ulong LocalWorkSize, ulong GlobalWorkOffset);
Ndrange_t __builtin_spirv_OpBuildNDRange_a2i32_a2i32_a2i32(uint GlobalWorkSize[2], uint LocalWorkSize[2], uint GlobalWorkOffset[2]);
Ndrange_t __builtin_spirv_OpBuildNDRange_a2i64_a2i64_a2i64(ulong GlobalWorkSize[2], ulong LocalWorkSize[2], ulong GlobalWorkOffset[2]);
Ndrange_t __builtin_spirv_OpBuildNDRange_a3i32_a3i32_a3i32(uint GlobalWorkSize[3], uint LocalWorkSize[3], uint GlobalWorkOffset[3]);
Ndrange_t __builtin_spirv_OpBuildNDRange_a3i64_a3i64_a3i64(ulong GlobalWorkSize[3], ulong LocalWorkSize[3], ulong GlobalWorkOffset[3]);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

// Pipe Instructions
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

int __builtin_spirv_OpReadPipe_i64_p4i8_i32(Pipe_t Pipe, generic void *Pointer, uint PacketSize/*, uint PacketAlignment*/);
int __builtin_spirv_OpWritePipe_i64_p4i8_i32(Pipe_wo_t Pipe, const generic void *Pointer, uint PacketSize/*, uint PacketAlignment*/);

int __builtin_spirv_OpReservedReadPipe_i64_i64_i32_p4i8_i32(Pipe_t Pipe, ReserveId_t ReserveId, uint Index, generic void *Pointer, uint PacketSize/*, uint PacketAlignment*/);
int __builtin_spirv_OpReservedWritePipe_i64_i64_i32_p4i8_i32(Pipe_wo_t Pipe, ReserveId_t ReserveId, uint Index, generic const void *Pointer, uint PacketSize/*, uint PacketAlignment*/);

ReserveId_t __builtin_spirv_OpReserveReadPipePackets_i64_i32_i32(Pipe_t Pipe, uint NumPackets, uint PacketSize/*, uint PacketAlignment*/);
ReserveId_t __builtin_spirv_OpReserveWritePipePackets_i64_i32_i32(Pipe_wo_t Pipe, uint NumPackets, uint PacketSize/*, uint PacketAlignment*/);

void __builtin_spirv_OpCommitReadPipe_i64_i64_i32(Pipe_t Pipe, ReserveId_t ReserveId, uint PacketSize/*, uint PacketAlignment*/);
void __builtin_spirv_OpCommitWritePipe_i64_i64_i32(Pipe_wo_t Pipe, ReserveId_t ReserveId, uint PacketSize/*, uint PacketAlignment*/);

bool __builtin_spirv_OpIsValidReserveId_i64(ReserveId_t ReserveId);

uint __builtin_spirv_OpGetNumPipePackets_i64_i32(Pipe_t Pipe, uint PacketSize/*, uint PacketAlignment*/);
uint __builtin_spirv_OpGetMaxPipePackets_i64_i32(Pipe_t Pipe, uint PacketSize/*, uint PacketAlignment*/);

ReserveId_t __builtin_spirv_OpGroupReserveReadPipePackets_i32_i64_i32_i32(uint Execution, Pipe_t Pipe, uint NumPackets, uint PacketSize/*, uint PacketAlignment*/);
ReserveId_t __builtin_spirv_OpGroupReserveWritePipePackets_i32_i64_i32_i32(uint Execution, Pipe_wo_t Pipe, uint NumPackets, uint PacketSize/*, uint PacketAlignment*/);

void __builtin_spirv_OpGroupCommitReadPipe_i32_i64_i64_i32(uint Execution, Pipe_t Pipe, ReserveId_t ReserveId, uint PacketSize/*, uint PacketAlignment*/);
void __builtin_spirv_OpGroupCommitWritePipe_i32_i64_i64_i32(uint Execution, Pipe_wo_t Pipe, ReserveId_t ReserveId, uint PacketSize/*, uint PacketAlignment*/);
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

/*******************************************Math*******************************************************/

//
//  Common
//        -degrees,fclamp,fmax_common,fmin_common,mix,radians,sign,smoothstep,step
//

float __builtin_spirv_OpenCL_degrees_f32(float r );
float2 __builtin_spirv_OpenCL_degrees_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_degrees_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_degrees_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_degrees_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_degrees_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_degrees_f64(double r );
double2 __builtin_spirv_OpenCL_degrees_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_degrees_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_degrees_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_degrees_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_degrees_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_degrees_f16(half r );
half2 __builtin_spirv_OpenCL_degrees_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_degrees_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_degrees_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_degrees_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_degrees_v16f16(half16 x);

float __builtin_spirv_OpenCL_fclamp_f32_f32_f32(float x, float y, float z);
float2 __builtin_spirv_OpenCL_fclamp_v2f32_v2f32_v2f32(float2 x, float2 y, float2 z);
float3 __builtin_spirv_OpenCL_fclamp_v3f32_v3f32_v3f32(float3 x, float3 y, float3 z);
float4 __builtin_spirv_OpenCL_fclamp_v4f32_v4f32_v4f32(float4 x, float4 y, float4 z);
float8 __builtin_spirv_OpenCL_fclamp_v8f32_v8f32_v8f32(float8 x, float8 y, float8 z);
float16 __builtin_spirv_OpenCL_fclamp_v16f32_v16f32_v16f32(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_fclamp_f64_f64_f64(double x, double y, double z);
double2 __builtin_spirv_OpenCL_fclamp_v2f64_v2f64_v2f64(double2 x, double2 y, double2 z);
double3 __builtin_spirv_OpenCL_fclamp_v3f64_v3f64_v3f64(double3 x, double3 y, double3 z);
double4 __builtin_spirv_OpenCL_fclamp_v4f64_v4f64_v4f64(double4 x, double4 y, double4 z);
double8 __builtin_spirv_OpenCL_fclamp_v8f64_v8f64_v8f64(double8 x, double8 y, double8 z);
double16 __builtin_spirv_OpenCL_fclamp_v16f64_v16f64_v16f64(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_fclamp_f16_f16_f16(half x, half y, half z);
half2 __builtin_spirv_OpenCL_fclamp_v2f16_v2f16_v2f16(half2 x, half2 y, half2 z);
half3 __builtin_spirv_OpenCL_fclamp_v3f16_v3f16_v3f16(half3 x, half3 y, half3 z);
half4 __builtin_spirv_OpenCL_fclamp_v4f16_v4f16_v4f16(half4 x, half4 y, half4 z);
half8 __builtin_spirv_OpenCL_fclamp_v8f16_v8f16_v8f16(half8 x, half8 y, half8 z);
half16 __builtin_spirv_OpenCL_fclamp_v16f16_v16f16_v16f16(half16 x, half16 y, half16 z);

float __builtin_spirv_OpenCL_fmax_common_f32_f32(float x, float y);
float2 __builtin_spirv_OpenCL_fmax_common_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_fmax_common_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_fmax_common_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_fmax_common_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_fmax_common_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_fmax_common_f64_f64(double x, double y);
double2 __builtin_spirv_OpenCL_fmax_common_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_fmax_common_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_fmax_common_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_fmax_common_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_fmax_common_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_fmax_common_f16_f16(half x, half y);
half2 __builtin_spirv_OpenCL_fmax_common_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_fmax_common_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_fmax_common_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_fmax_common_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_fmax_common_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_fmin_common_f32_f32(float x, float y);
half __builtin_spirv_OpenCL_fmin_common_f16_f16(half x, half y);
float2 __builtin_spirv_OpenCL_fmin_common_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_fmin_common_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_fmin_common_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_fmin_common_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_fmin_common_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_fmin_common_f64_f64(double x, double y);
double2 __builtin_spirv_OpenCL_fmin_common_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_fmin_common_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_fmin_common_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_fmin_common_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_fmin_common_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half2 __builtin_spirv_OpenCL_fmin_common_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_fmin_common_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_fmin_common_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_fmin_common_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_fmin_common_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_mix_f32_f32_f32(float x, float y, float a );
float2 __builtin_spirv_OpenCL_mix_v2f32_v2f32_v2f32(float2 x, float2 y, float2 z);
float3 __builtin_spirv_OpenCL_mix_v3f32_v3f32_v3f32(float3 x, float3 y, float3 z);
float4 __builtin_spirv_OpenCL_mix_v4f32_v4f32_v4f32(float4 x, float4 y, float4 z);
float8 __builtin_spirv_OpenCL_mix_v8f32_v8f32_v8f32(float8 x, float8 y, float8 z);
float16 __builtin_spirv_OpenCL_mix_v16f32_v16f32_v16f32(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_mix_f64_f64_f64(double x, double y, double a );
double2 __builtin_spirv_OpenCL_mix_v2f64_v2f64_v2f64(double2 x, double2 y, double2 z);
double3 __builtin_spirv_OpenCL_mix_v3f64_v3f64_v3f64(double3 x, double3 y, double3 z);
double4 __builtin_spirv_OpenCL_mix_v4f64_v4f64_v4f64(double4 x, double4 y, double4 z);
double8 __builtin_spirv_OpenCL_mix_v8f64_v8f64_v8f64(double8 x, double8 y, double8 z);
double16 __builtin_spirv_OpenCL_mix_v16f64_v16f64_v16f64(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_mix_f16_f16_f16(half x, half y, half a );
half2 __builtin_spirv_OpenCL_mix_v2f16_v2f16_v2f16(half2 x, half2 y, half2 z);
half3 __builtin_spirv_OpenCL_mix_v3f16_v3f16_v3f16(half3 x, half3 y, half3 z);
half4 __builtin_spirv_OpenCL_mix_v4f16_v4f16_v4f16(half4 x, half4 y, half4 z);
half8 __builtin_spirv_OpenCL_mix_v8f16_v8f16_v8f16(half8 x, half8 y, half8 z);
half16 __builtin_spirv_OpenCL_mix_v16f16_v16f16_v16f16(half16 x, half16 y, half16 z);

float __builtin_spirv_OpenCL_radians_f32(float d );
float2 __builtin_spirv_OpenCL_radians_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_radians_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_radians_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_radians_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_radians_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_radians_f64(double d );
double2 __builtin_spirv_OpenCL_radians_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_radians_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_radians_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_radians_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_radians_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_radians_f16(half d );
half2 __builtin_spirv_OpenCL_radians_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_radians_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_radians_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_radians_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_radians_v16f16(half16 x);

float __builtin_spirv_OpenCL_sign_f32(float x );
float2 __builtin_spirv_OpenCL_sign_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_sign_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_sign_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_sign_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_sign_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_sign_f64(double x );
double2 __builtin_spirv_OpenCL_sign_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_sign_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_sign_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_sign_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_sign_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_sign_f16(half x );
half2 __builtin_spirv_OpenCL_sign_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_sign_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_sign_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_sign_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_sign_v16f16(half16 x);

float __builtin_spirv_OpenCL_smoothstep_f32_f32_f32(float edge0, float edge1, float x );
float2 __builtin_spirv_OpenCL_smoothstep_v2f32_v2f32_v2f32(float2 x, float2 y, float2 z);
float3 __builtin_spirv_OpenCL_smoothstep_v3f32_v3f32_v3f32(float3 x, float3 y, float3 z);
float4 __builtin_spirv_OpenCL_smoothstep_v4f32_v4f32_v4f32(float4 x, float4 y, float4 z);
float8 __builtin_spirv_OpenCL_smoothstep_v8f32_v8f32_v8f32(float8 x, float8 y, float8 z);
float16 __builtin_spirv_OpenCL_smoothstep_v16f32_v16f32_v16f32(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_smoothstep_f64_f64_f64(double edge0, double edge1, double x );
double2 __builtin_spirv_OpenCL_smoothstep_v2f64_v2f64_v2f64(double2 x, double2 y, double2 z);
double3 __builtin_spirv_OpenCL_smoothstep_v3f64_v3f64_v3f64(double3 x, double3 y, double3 z);
double4 __builtin_spirv_OpenCL_smoothstep_v4f64_v4f64_v4f64(double4 x, double4 y, double4 z);
double8 __builtin_spirv_OpenCL_smoothstep_v8f64_v8f64_v8f64(double8 x, double8 y, double8 z);
double16 __builtin_spirv_OpenCL_smoothstep_v16f64_v16f64_v16f64(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_smoothstep_f16_f16_f16(half edge0, half edge1, half x );
half2 __builtin_spirv_OpenCL_smoothstep_v2f16_v2f16_v2f16(half2 x, half2 y, half2 z);
half3 __builtin_spirv_OpenCL_smoothstep_v3f16_v3f16_v3f16(half3 x, half3 y, half3 z);
half4 __builtin_spirv_OpenCL_smoothstep_v4f16_v4f16_v4f16(half4 x, half4 y, half4 z);
half8 __builtin_spirv_OpenCL_smoothstep_v8f16_v8f16_v8f16(half8 x, half8 y, half8 z);
half16 __builtin_spirv_OpenCL_smoothstep_v16f16_v16f16_v16f16(half16 x, half16 y, half16 z);

float __builtin_spirv_OpenCL_step_f32_f32(float edge, float x );
float2 __builtin_spirv_OpenCL_step_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_step_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_step_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_step_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_step_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_step_f64_f64(double edge, double x );
double2 __builtin_spirv_OpenCL_step_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_step_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_step_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_step_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_step_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_step_f16_f16(half edge, half x );
half2 __builtin_spirv_OpenCL_step_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_step_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_step_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_step_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_step_v16f16_v16f16(half16 x, half16 y);


//
//  Geometric
//        -cross,distance,fast_distance,fast_length,fast_normalize,length,normalize
//

float3 __builtin_spirv_OpenCL_cross_v3f32_v3f32(float3 p0, float3 p1 );
float4 __builtin_spirv_OpenCL_cross_v4f32_v4f32(float4 p0, float4 p1 );
#if defined(cl_khr_fp64)
double3 __builtin_spirv_OpenCL_cross_v3f64_v3f64(double3 p0, double3 p1 );
double4 __builtin_spirv_OpenCL_cross_v4f64_v4f64(double4 p0, double4 p1 );
#endif // defined(cl_khr_fp64)
half3 __builtin_spirv_OpenCL_cross_v3f16_v3f16(half3 p0, half3 p1 );
half4 __builtin_spirv_OpenCL_cross_v4f16_v4f16(half4 p0, half4 p1 );

float __builtin_spirv_OpenCL_distance_f32_f32(float p0, float p1 );
float __builtin_spirv_OpenCL_distance_v2f32_v2f32(float2 p0, float2 p1 );
float __builtin_spirv_OpenCL_distance_v3f32_v3f32(float3 p0, float3 p1 );
float __builtin_spirv_OpenCL_distance_v4f32_v4f32(float4 p0, float4 p1 );
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_distance_f64_f64(double p0, double p1 );
double __builtin_spirv_OpenCL_distance_v2f64_v2f64(double2 p0, double2 p1 );
double __builtin_spirv_OpenCL_distance_v3f64_v3f64(double3 p0, double3 p1 );
double __builtin_spirv_OpenCL_distance_v4f64_v4f64(double4 p0, double4 p1 );
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_distance_f16_f16(half p0, half p1 );
half __builtin_spirv_OpenCL_distance_v2f16_v2f16(half2 p0, half2 p1 );
half __builtin_spirv_OpenCL_distance_v3f16_v3f16(half3 p0, half3 p1 );
half __builtin_spirv_OpenCL_distance_v4f16_v4f16(half4 p0, half4 p1 );

float __builtin_spirv_OpenCL_fast_distance_f32_f32(float p0, float p1 );
float __builtin_spirv_OpenCL_fast_distance_v2f32_v2f32(float2 p0, float2 p1 );
float __builtin_spirv_OpenCL_fast_distance_v3f32_v3f32(float3 p0, float3 p1 );
float __builtin_spirv_OpenCL_fast_distance_v4f32_v4f32(float4 p0, float4 p1 );

float __builtin_spirv_OpenCL_fast_length_f32(float p );
float __builtin_spirv_OpenCL_fast_length_v2f32(float2 p );
float __builtin_spirv_OpenCL_fast_length_v3f32(float3 p );
float __builtin_spirv_OpenCL_fast_length_v4f32(float4 p );

float __builtin_spirv_OpenCL_fast_normalize_f32(float p );
float2 __builtin_spirv_OpenCL_fast_normalize_v2f32(float2 p );
float3 __builtin_spirv_OpenCL_fast_normalize_v3f32(float3 p );
float4 __builtin_spirv_OpenCL_fast_normalize_v4f32(float4 p );

float __builtin_spirv_OpenCL_length_f32(float p );
float __builtin_spirv_OpenCL_length_v2f32(float2 p );
float __builtin_spirv_OpenCL_length_v3f32(float3 p );
float __builtin_spirv_OpenCL_length_v4f32(float4 p );
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_length_f64(double p );
double __builtin_spirv_OpenCL_length_v2f64(double2 p );
double __builtin_spirv_OpenCL_length_v3f64(double3 p );
double __builtin_spirv_OpenCL_length_v4f64(double4 p );
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
half __builtin_spirv_OpenCL_length_f16(half p );
half __builtin_spirv_OpenCL_length_v2f16(half2 p );
half __builtin_spirv_OpenCL_length_v3f16(half3 p );
half __builtin_spirv_OpenCL_length_v4f16(half4 p );
#endif // defined(cl_khr_fp16)

float __builtin_spirv_OpenCL_normalize_f32(float p );
float2 __builtin_spirv_OpenCL_normalize_v2f32(float2 p );
float3 __builtin_spirv_OpenCL_normalize_v3f32(float3 p );
float4 __builtin_spirv_OpenCL_normalize_v4f32(float4 p );
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_normalize_f64(double p );
double2 __builtin_spirv_OpenCL_normalize_v2f64(double2 p );
double3 __builtin_spirv_OpenCL_normalize_v3f64(double3 p );
double4 __builtin_spirv_OpenCL_normalize_v4f64(double4 p );
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_normalize_f16(half p );
half2 __builtin_spirv_OpenCL_normalize_v2f16(half2 p );
half3 __builtin_spirv_OpenCL_normalize_v3f16(half3 p );
half4 __builtin_spirv_OpenCL_normalize_v4f16(half4 p );


//
//  Half
//        -half_cos,half_divide,half_exp,half_exp2,half_exp10,half_log,half_log2,half_log10
//         half_powr,half_recip,half_rsqrt,half_sin,half_sqrt,half_tan
//

float __builtin_spirv_OpenCL_half_cos_f32(float x );
float2 __builtin_spirv_OpenCL_half_cos_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_cos_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_cos_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_cos_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_cos_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_divide_f32_f32(float x, float y );
float2 __builtin_spirv_OpenCL_half_divide_v2f32_v2f32(float2 x, float2 y );
float3 __builtin_spirv_OpenCL_half_divide_v3f32_v3f32(float3 x, float3 y );
float4 __builtin_spirv_OpenCL_half_divide_v4f32_v4f32(float4 x, float4 y );
float8 __builtin_spirv_OpenCL_half_divide_v8f32_v8f32(float8 x, float8 y );
float16 __builtin_spirv_OpenCL_half_divide_v16f32_v16f32(float16 x, float16 y );

float __builtin_spirv_OpenCL_half_exp_f32(float x );
float2 __builtin_spirv_OpenCL_half_exp_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_exp_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_exp_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_exp_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_exp_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_exp2_f32(float x );
float2 __builtin_spirv_OpenCL_half_exp2_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_exp2_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_exp2_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_exp2_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_exp2_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_exp10_f32(float x );
float2 __builtin_spirv_OpenCL_half_exp10_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_exp10_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_exp10_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_exp10_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_exp10_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_log_f32(float x );
float2 __builtin_spirv_OpenCL_half_log_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_log_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_log_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_log_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_log_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_log2_f32(float x );
float2 __builtin_spirv_OpenCL_half_log2_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_log2_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_log2_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_log2_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_log2_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_log10_f32(float x );
float2 __builtin_spirv_OpenCL_half_log10_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_log10_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_log10_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_log10_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_log10_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_powr_f32_f32(float x, float y );
float2 __builtin_spirv_OpenCL_half_powr_v2f32_v2f32(float2 x, float2 y );
float3 __builtin_spirv_OpenCL_half_powr_v3f32_v3f32(float3 x, float3 y );
float4 __builtin_spirv_OpenCL_half_powr_v4f32_v4f32(float4 x, float4 y );
float8 __builtin_spirv_OpenCL_half_powr_v8f32_v8f32(float8 x, float8 y );
float16 __builtin_spirv_OpenCL_half_powr_v16f32_v16f32(float16 x, float16 y );

float __builtin_spirv_OpenCL_half_recip_f32(float x );
float2 __builtin_spirv_OpenCL_half_recip_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_recip_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_recip_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_recip_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_recip_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_rsqrt_f32(float x );
float2 __builtin_spirv_OpenCL_half_rsqrt_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_rsqrt_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_rsqrt_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_rsqrt_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_rsqrt_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_sin_f32(float x );
float2 __builtin_spirv_OpenCL_half_sin_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_sin_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_sin_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_sin_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_sin_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_sqrt_f32(float x );
float2 __builtin_spirv_OpenCL_half_sqrt_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_sqrt_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_sqrt_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_sqrt_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_sqrt_v16f32(float16 x );

float __builtin_spirv_OpenCL_half_tan_f32(float x );
float2 __builtin_spirv_OpenCL_half_tan_v2f32(float2 x );
float3 __builtin_spirv_OpenCL_half_tan_v3f32(float3 x );
float4 __builtin_spirv_OpenCL_half_tan_v4f32(float4 x );
float8 __builtin_spirv_OpenCL_half_tan_v8f32(float8 x );
float16 __builtin_spirv_OpenCL_half_tan_v16f32(float16 x );


//
//  Integer (signed and unsigned )
//        -abs,abs_diff,add_sat,clamp,clz,ctz,hadd,mad_hi,mad_sat,mad24,max,min,mul_hi
//         mul24,popcnt,rhadd,rotate,sub_sat,upsample
//

uchar __builtin_spirv_OpenCL_s_abs_i8( char x );
uchar2 __builtin_spirv_OpenCL_s_abs_v2i8( char2 x );
uchar3 __builtin_spirv_OpenCL_s_abs_v3i8( char3 x );
uchar4 __builtin_spirv_OpenCL_s_abs_v4i8( char4 x );
uchar8 __builtin_spirv_OpenCL_s_abs_v8i8( char8 x );
uchar16 __builtin_spirv_OpenCL_s_abs_v16i8( char16 x );
uchar __builtin_spirv_OpenCL_u_abs_i8( uchar x );
uchar2 __builtin_spirv_OpenCL_u_abs_v2i8( uchar2 x );
uchar3 __builtin_spirv_OpenCL_u_abs_v3i8( uchar3 x );
uchar4 __builtin_spirv_OpenCL_u_abs_v4i8( uchar4 x );
uchar8 __builtin_spirv_OpenCL_u_abs_v8i8( uchar8 x );
uchar16 __builtin_spirv_OpenCL_u_abs_v16i8( uchar16 x );
ushort __builtin_spirv_OpenCL_s_abs_i16( short x );
ushort2 __builtin_spirv_OpenCL_s_abs_v2i16( short2 x );
ushort3 __builtin_spirv_OpenCL_s_abs_v3i16( short3 x );
ushort4 __builtin_spirv_OpenCL_s_abs_v4i16( short4 x );
ushort8 __builtin_spirv_OpenCL_s_abs_v8i16( short8 x );
ushort16 __builtin_spirv_OpenCL_s_abs_v16i16( short16 x );
ushort __builtin_spirv_OpenCL_u_abs_i16( ushort x );
ushort2 __builtin_spirv_OpenCL_u_abs_v2i16( ushort2 x );
ushort3 __builtin_spirv_OpenCL_u_abs_v3i16( ushort3 x );
ushort4 __builtin_spirv_OpenCL_u_abs_v4i16( ushort4 x );
ushort8 __builtin_spirv_OpenCL_u_abs_v8i16( ushort8 x );
ushort16 __builtin_spirv_OpenCL_u_abs_v16i16( ushort16 x );
uint __builtin_spirv_OpenCL_s_abs_i32( int x );
uint2 __builtin_spirv_OpenCL_s_abs_v2i32( int2 x );
uint3 __builtin_spirv_OpenCL_s_abs_v3i32( int3 x );
uint4 __builtin_spirv_OpenCL_s_abs_v4i32( int4 x );
uint8 __builtin_spirv_OpenCL_s_abs_v8i32( int8 x );
uint16 __builtin_spirv_OpenCL_s_abs_v16i32( int16 x );
uint __builtin_spirv_OpenCL_u_abs_i32( uint x );
uint2 __builtin_spirv_OpenCL_u_abs_v2i32( uint2 x );
uint3 __builtin_spirv_OpenCL_u_abs_v3i32( uint3 x );
uint4 __builtin_spirv_OpenCL_u_abs_v4i32( uint4 x );
uint8 __builtin_spirv_OpenCL_u_abs_v8i32( uint8 x );
uint16 __builtin_spirv_OpenCL_u_abs_v16i32( uint16 x );
ulong __builtin_spirv_OpenCL_s_abs_i64( long x );
ulong2 __builtin_spirv_OpenCL_s_abs_v2i64( long2 x );
ulong3 __builtin_spirv_OpenCL_s_abs_v3i64( long3 x );
ulong4 __builtin_spirv_OpenCL_s_abs_v4i64( long4 x );
ulong8 __builtin_spirv_OpenCL_s_abs_v8i64( long8 x );
ulong16 __builtin_spirv_OpenCL_s_abs_v16i64( long16 x );
ulong __builtin_spirv_OpenCL_u_abs_i64( ulong x );
ulong2 __builtin_spirv_OpenCL_u_abs_v2i64( ulong2 x );
ulong3 __builtin_spirv_OpenCL_u_abs_v3i64( ulong3 x );
ulong4 __builtin_spirv_OpenCL_u_abs_v4i64( ulong4 x );
ulong8 __builtin_spirv_OpenCL_u_abs_v8i64( ulong8 x );
ulong16 __builtin_spirv_OpenCL_u_abs_v16i64( ulong16 x );

uchar __builtin_spirv_OpenCL_s_abs_diff_i8_i8( char x, char y );
uchar2 __builtin_spirv_OpenCL_s_abs_diff_v2i8_v2i8( char2 x, char2 y );
uchar3 __builtin_spirv_OpenCL_s_abs_diff_v3i8_v3i8( char3 x, char3 y );
uchar4 __builtin_spirv_OpenCL_s_abs_diff_v4i8_v4i8( char4 x, char4 y );
uchar8 __builtin_spirv_OpenCL_s_abs_diff_v8i8_v8i8( char8 x, char8 y );
uchar16 __builtin_spirv_OpenCL_s_abs_diff_v16i8_v16i8( char16 x, char16 y );
uchar __builtin_spirv_OpenCL_u_abs_diff_i8_i8( uchar x, uchar y );
uchar2 __builtin_spirv_OpenCL_u_abs_diff_v2i8_v2i8( uchar2 x, uchar2 y );
uchar3 __builtin_spirv_OpenCL_u_abs_diff_v3i8_v3i8( uchar3 x, uchar3 y );
uchar4 __builtin_spirv_OpenCL_u_abs_diff_v4i8_v4i8( uchar4 x, uchar4 y );
uchar8 __builtin_spirv_OpenCL_u_abs_diff_v8i8_v8i8( uchar8 x, uchar8 y );
uchar16 __builtin_spirv_OpenCL_u_abs_diff_v16i8_v16i8( uchar16 x, uchar16 y );
ushort __builtin_spirv_OpenCL_s_abs_diff_i16_i16( short x, short y );
ushort2 __builtin_spirv_OpenCL_s_abs_diff_v2i16_v2i16( short2 x, short2 y );
ushort3 __builtin_spirv_OpenCL_s_abs_diff_v3i16_v3i16( short3 x, short3 y );
ushort4 __builtin_spirv_OpenCL_s_abs_diff_v4i16_v4i16( short4 x, short4 y );
ushort8 __builtin_spirv_OpenCL_s_abs_diff_v8i16_v8i16( short8 x, short8 y );
ushort16 __builtin_spirv_OpenCL_s_abs_diff_v16i16_v16i16( short16 x, short16 y );
ushort __builtin_spirv_OpenCL_u_abs_diff_i16_i16( ushort x, ushort y );
ushort2 __builtin_spirv_OpenCL_u_abs_diff_v2i16_v2i16( ushort2 x, ushort2 y );
ushort3 __builtin_spirv_OpenCL_u_abs_diff_v3i16_v3i16( ushort3 x, ushort3 y );
ushort4 __builtin_spirv_OpenCL_u_abs_diff_v4i16_v4i16( ushort4 x, ushort4 y );
ushort8 __builtin_spirv_OpenCL_u_abs_diff_v8i16_v8i16( ushort8 x, ushort8 y );
ushort16 __builtin_spirv_OpenCL_u_abs_diff_v16i16_v16i16( ushort16 x, ushort16 y );
uint __builtin_spirv_OpenCL_s_abs_diff_i32_i32( int x, int y );
uint2 __builtin_spirv_OpenCL_s_abs_diff_v2i32_v2i32( int2 x, int2 y );
uint3 __builtin_spirv_OpenCL_s_abs_diff_v3i32_v3i32( int3 x, int3 y );
uint4 __builtin_spirv_OpenCL_s_abs_diff_v4i32_v4i32( int4 x, int4 y );
uint8 __builtin_spirv_OpenCL_s_abs_diff_v8i32_v8i32( int8 x, int8 y );
uint16 __builtin_spirv_OpenCL_s_abs_diff_v16i32_v16i32( int16 x, int16 y );
uint __builtin_spirv_OpenCL_u_abs_diff_i32_i32( uint x, uint y );
uint2 __builtin_spirv_OpenCL_u_abs_diff_v2i32_v2i32( uint2 x, uint2 y );
uint3 __builtin_spirv_OpenCL_u_abs_diff_v3i32_v3i32( uint3 x, uint3 y );
uint4 __builtin_spirv_OpenCL_u_abs_diff_v4i32_v4i32( uint4 x, uint4 y );
uint8 __builtin_spirv_OpenCL_u_abs_diff_v8i32_v8i32( uint8 x, uint8 y );
uint16 __builtin_spirv_OpenCL_u_abs_diff_v16i32_v16i32( uint16 x, uint16 y );
ulong __builtin_spirv_OpenCL_s_abs_diff_i64_i64( long x, long y );
ulong2 __builtin_spirv_OpenCL_s_abs_diff_v2i64_v2i64( long2 x, long2 y );
ulong3 __builtin_spirv_OpenCL_s_abs_diff_v3i64_v3i64( long3 x, long3 y );
ulong4 __builtin_spirv_OpenCL_s_abs_diff_v4i64_v4i64( long4 x, long4 y );
ulong8 __builtin_spirv_OpenCL_s_abs_diff_v8i64_v8i64( long8 x, long8 y );
ulong16 __builtin_spirv_OpenCL_s_abs_diff_v16i64_v16i64( long16 x, long16 y );
ulong __builtin_spirv_OpenCL_u_abs_diff_i64_i64( ulong x, ulong y );
ulong2 __builtin_spirv_OpenCL_u_abs_diff_v2i64_v2i64( ulong2 x, ulong2 y );
ulong3 __builtin_spirv_OpenCL_u_abs_diff_v3i64_v3i64( ulong3 x, ulong3 y );
ulong4 __builtin_spirv_OpenCL_u_abs_diff_v4i64_v4i64( ulong4 x, ulong4 y );
ulong8 __builtin_spirv_OpenCL_u_abs_diff_v8i64_v8i64( ulong8 x, ulong8 y );
ulong16 __builtin_spirv_OpenCL_u_abs_diff_v16i64_v16i64( ulong16 x, ulong16 y );

char __builtin_spirv_OpenCL_s_add_sat_i8_i8( char x, char y );
char2 __builtin_spirv_OpenCL_s_add_sat_v2i8_v2i8( char2 x, char2 y );
char3 __builtin_spirv_OpenCL_s_add_sat_v3i8_v3i8( char3 x, char3 y );
char4 __builtin_spirv_OpenCL_s_add_sat_v4i8_v4i8( char4 x, char4 y );
char8 __builtin_spirv_OpenCL_s_add_sat_v8i8_v8i8( char8 x, char8 y );
char16 __builtin_spirv_OpenCL_s_add_sat_v16i8_v16i8( char16 x, char16 y );
uchar __builtin_spirv_OpenCL_u_add_sat_i8_i8( uchar x, uchar y );
uchar2 __builtin_spirv_OpenCL_u_add_sat_v2i8_v2i8( uchar2 x, uchar2 y );
uchar3 __builtin_spirv_OpenCL_u_add_sat_v3i8_v3i8( uchar3 x, uchar3 y );
uchar4 __builtin_spirv_OpenCL_u_add_sat_v4i8_v4i8( uchar4 x, uchar4 y );
uchar8 __builtin_spirv_OpenCL_u_add_sat_v8i8_v8i8( uchar8 x, uchar8 y );
uchar16 __builtin_spirv_OpenCL_u_add_sat_v16i8_v16i8( uchar16 x, uchar16 y );
short __builtin_spirv_OpenCL_s_add_sat_i16_i16( short x, short y );
short2 __builtin_spirv_OpenCL_s_add_sat_v2i16_v2i16( short2 x, short2 y );
short3 __builtin_spirv_OpenCL_s_add_sat_v3i16_v3i16( short3 x, short3 y );
short4 __builtin_spirv_OpenCL_s_add_sat_v4i16_v4i16( short4 x, short4 y );
short8 __builtin_spirv_OpenCL_s_add_sat_v8i16_v8i16( short8 x, short8 y );
short16 __builtin_spirv_OpenCL_s_add_sat_v16i16_v16i16( short16 x, short16 y );
ushort __builtin_spirv_OpenCL_u_add_sat_i16_i16( ushort x, ushort y );
ushort2 __builtin_spirv_OpenCL_u_add_sat_v2i16_v2i16( ushort2 x, ushort2 y );
ushort3 __builtin_spirv_OpenCL_u_add_sat_v3i16_v3i16( ushort3 x, ushort3 y );
ushort4 __builtin_spirv_OpenCL_u_add_sat_v4i16_v4i16( ushort4 x, ushort4 y );
ushort8 __builtin_spirv_OpenCL_u_add_sat_v8i16_v8i16( ushort8 x, ushort8 y );
ushort16 __builtin_spirv_OpenCL_u_add_sat_v16i16_v16i16( ushort16 x, ushort16 y );
int __builtin_spirv_OpenCL_s_add_sat_i32_i32( int x, int y );
int2 __builtin_spirv_OpenCL_s_add_sat_v2i32_v2i32( int2 x, int2 y );
int3 __builtin_spirv_OpenCL_s_add_sat_v3i32_v3i32( int3 x, int3 y );
int4 __builtin_spirv_OpenCL_s_add_sat_v4i32_v4i32( int4 x, int4 y );
int8 __builtin_spirv_OpenCL_s_add_sat_v8i32_v8i32( int8 x, int8 y );
int16 __builtin_spirv_OpenCL_s_add_sat_v16i32_v16i32( int16 x, int16 y );
uint __builtin_spirv_OpenCL_u_add_sat_i32_i32( uint x, uint y );
uint2 __builtin_spirv_OpenCL_u_add_sat_v2i32_v2i32( uint2 x, uint2 y );
uint3 __builtin_spirv_OpenCL_u_add_sat_v3i32_v3i32( uint3 x, uint3 y );
uint4 __builtin_spirv_OpenCL_u_add_sat_v4i32_v4i32( uint4 x, uint4 y );
uint8 __builtin_spirv_OpenCL_u_add_sat_v8i32_v8i32( uint8 x, uint8 y );
uint16 __builtin_spirv_OpenCL_u_add_sat_v16i32_v16i32( uint16 x, uint16 y );
long __builtin_spirv_OpenCL_s_add_sat_i64_i64( long x, long y );
long2 __builtin_spirv_OpenCL_s_add_sat_v2i64_v2i64( long2 x, long2 y );
long3 __builtin_spirv_OpenCL_s_add_sat_v3i64_v3i64( long3 x, long3 y );
long4 __builtin_spirv_OpenCL_s_add_sat_v4i64_v4i64( long4 x, long4 y );
long8 __builtin_spirv_OpenCL_s_add_sat_v8i64_v8i64( long8 x, long8 y );
long16 __builtin_spirv_OpenCL_s_add_sat_v16i64_v16i64( long16 x, long16 y );
ulong __builtin_spirv_OpenCL_u_add_sat_i64_i64( ulong x, ulong y );
ulong2 __builtin_spirv_OpenCL_u_add_sat_v2i64_v2i64( ulong2 x, ulong2 y );
ulong3 __builtin_spirv_OpenCL_u_add_sat_v3i64_v3i64( ulong3 x, ulong3 y );
ulong4 __builtin_spirv_OpenCL_u_add_sat_v4i64_v4i64( ulong4 x, ulong4 y );
ulong8 __builtin_spirv_OpenCL_u_add_sat_v8i64_v8i64( ulong8 x, ulong8 y );
ulong16 __builtin_spirv_OpenCL_u_add_sat_v16i64_v16i64( ulong16 x, ulong16 y );

char __builtin_spirv_OpenCL_s_clamp_i8_i8_i8(char x, char minval, char maxval );
uchar __builtin_spirv_OpenCL_u_clamp_i8_i8_i8(uchar x, uchar minval, uchar maxval );
short __builtin_spirv_OpenCL_s_clamp_i16_i16_i16(short x, short minval, short maxval );
ushort __builtin_spirv_OpenCL_u_clamp_i16_i16_i16(ushort x, ushort minval, ushort maxval );
int __builtin_spirv_OpenCL_s_clamp_i32_i32_i32(int x, int minval, int maxval );
uint __builtin_spirv_OpenCL_u_clamp_i32_i32_i32(uint x, uint minval, uint maxval );
long __builtin_spirv_OpenCL_s_clamp_i64_i64_i64(long x, long minval, long maxval );
ulong __builtin_spirv_OpenCL_u_clamp_i64_i64_i64(ulong x, ulong minval, ulong maxval );
char2 __builtin_spirv_OpenCL_s_clamp_v2i8_v2i8_v2i8(char2 x, char2 y, char2 z);
char3 __builtin_spirv_OpenCL_s_clamp_v3i8_v3i8_v3i8(char3 x, char3 y, char3 z);
char4 __builtin_spirv_OpenCL_s_clamp_v4i8_v4i8_v4i8(char4 x, char4 y, char4 z);
char8 __builtin_spirv_OpenCL_s_clamp_v8i8_v8i8_v8i8(char8 x, char8 y, char8 z);
char16 __builtin_spirv_OpenCL_s_clamp_v16i8_v16i8_v16i8(char16 x, char16 y, char16 z);
uchar2 __builtin_spirv_OpenCL_u_clamp_v2i8_v2i8_v2i8(uchar2 x, uchar2 y, uchar2 z);
uchar3 __builtin_spirv_OpenCL_u_clamp_v3i8_v3i8_v3i8(uchar3 x, uchar3 y, uchar3 z);
uchar4 __builtin_spirv_OpenCL_u_clamp_v4i8_v4i8_v4i8(uchar4 x, uchar4 y, uchar4 z);
uchar8 __builtin_spirv_OpenCL_u_clamp_v8i8_v8i8_v8i8(uchar8 x, uchar8 y, uchar8 z);
uchar16 __builtin_spirv_OpenCL_u_clamp_v16i8_v16i8_v16i8(uchar16 x, uchar16 y, uchar16 z);
short2 __builtin_spirv_OpenCL_s_clamp_v2i16_v2i16_v2i16(short2 x, short2 y, short2 z);
short3 __builtin_spirv_OpenCL_s_clamp_v3i16_v3i16_v3i16(short3 x, short3 y, short3 z);
short4 __builtin_spirv_OpenCL_s_clamp_v4i16_v4i16_v4i16(short4 x, short4 y, short4 z);
short8 __builtin_spirv_OpenCL_s_clamp_v8i16_v8i16_v8i16(short8 x, short8 y, short8 z);
short16 __builtin_spirv_OpenCL_s_clamp_v16i16_v16i16_v16i16(short16 x, short16 y, short16 z);
ushort2 __builtin_spirv_OpenCL_u_clamp_v2i16_v2i16_v2i16(ushort2 x, ushort2 y, ushort2 z);
ushort3 __builtin_spirv_OpenCL_u_clamp_v3i16_v3i16_v3i16(ushort3 x, ushort3 y, ushort3 z);
ushort4 __builtin_spirv_OpenCL_u_clamp_v4i16_v4i16_v4i16(ushort4 x, ushort4 y, ushort4 z);
ushort8 __builtin_spirv_OpenCL_u_clamp_v8i16_v8i16_v8i16(ushort8 x, ushort8 y, ushort8 z);
ushort16 __builtin_spirv_OpenCL_u_clamp_v16i16_v16i16_v16i16(ushort16 x, ushort16 y, ushort16 z);
int2 __builtin_spirv_OpenCL_s_clamp_v2i32_v2i32_v2i32(int2 x, int2 y, int2 z);
int3 __builtin_spirv_OpenCL_s_clamp_v3i32_v3i32_v3i32(int3 x, int3 y, int3 z);
int4 __builtin_spirv_OpenCL_s_clamp_v4i32_v4i32_v4i32(int4 x, int4 y, int4 z);
int8 __builtin_spirv_OpenCL_s_clamp_v8i32_v8i32_v8i32(int8 x, int8 y, int8 z);
int16 __builtin_spirv_OpenCL_s_clamp_v16i32_v16i32_v16i32(int16 x, int16 y, int16 z);
uint2 __builtin_spirv_OpenCL_u_clamp_v2i32_v2i32_v2i32(uint2 x, uint2 y, uint2 z);
uint3 __builtin_spirv_OpenCL_u_clamp_v3i32_v3i32_v3i32(uint3 x, uint3 y, uint3 z);
uint4 __builtin_spirv_OpenCL_u_clamp_v4i32_v4i32_v4i32(uint4 x, uint4 y, uint4 z);
uint8 __builtin_spirv_OpenCL_u_clamp_v8i32_v8i32_v8i32(uint8 x, uint8 y, uint8 z);
uint16 __builtin_spirv_OpenCL_u_clamp_v16i32_v16i32_v16i32(uint16 x, uint16 y, uint16 z);
long2 __builtin_spirv_OpenCL_s_clamp_v2i64_v2i64_v2i64(long2 x, long2 y, long2 z);
long3 __builtin_spirv_OpenCL_s_clamp_v3i64_v3i64_v3i64(long3 x, long3 y, long3 z);
long4 __builtin_spirv_OpenCL_s_clamp_v4i64_v4i64_v4i64(long4 x, long4 y, long4 z);
long8 __builtin_spirv_OpenCL_s_clamp_v8i64_v8i64_v8i64(long8 x, long8 y, long8 z);
long16 __builtin_spirv_OpenCL_s_clamp_v16i64_v16i64_v16i64(long16 x, long16 y, long16 z);
ulong2 __builtin_spirv_OpenCL_u_clamp_v2i64_v2i64_v2i64(ulong2 x, ulong2 y, ulong2 z);
ulong3 __builtin_spirv_OpenCL_u_clamp_v3i64_v3i64_v3i64(ulong3 x, ulong3 y, ulong3 z);
ulong4 __builtin_spirv_OpenCL_u_clamp_v4i64_v4i64_v4i64(ulong4 x, ulong4 y, ulong4 z);
ulong8 __builtin_spirv_OpenCL_u_clamp_v8i64_v8i64_v8i64(ulong8 x, ulong8 y, ulong8 z);
ulong16 __builtin_spirv_OpenCL_u_clamp_v16i64_v16i64_v16i64(ulong16 x, ulong16 y, ulong16 z);

uchar __builtin_spirv_OpenCL_clz_i8( uchar x );
ushort __builtin_spirv_OpenCL_clz_i16( ushort x );
uint __builtin_spirv_OpenCL_clz_i32( uint x );
uchar2 __builtin_spirv_OpenCL_clz_v2i8( uchar2 x );
uchar3 __builtin_spirv_OpenCL_clz_v3i8( uchar3 x );
uchar4 __builtin_spirv_OpenCL_clz_v4i8( uchar4 x );
uchar8 __builtin_spirv_OpenCL_clz_v8i8( uchar8 x );
uchar16 __builtin_spirv_OpenCL_clz_v16i8( uchar16 x );
ushort2 __builtin_spirv_OpenCL_clz_v2i16( ushort2 x );
ushort3 __builtin_spirv_OpenCL_clz_v3i16( ushort3 x );
ushort4 __builtin_spirv_OpenCL_clz_v4i16( ushort4 x );
ushort8 __builtin_spirv_OpenCL_clz_v8i16( ushort8 x );
ushort16 __builtin_spirv_OpenCL_clz_v16i16( ushort16 x );
uint2 __builtin_spirv_OpenCL_clz_v2i32( uint2 x );
uint3 __builtin_spirv_OpenCL_clz_v3i32( uint3 x );
uint4 __builtin_spirv_OpenCL_clz_v4i32( uint4 x );
uint8 __builtin_spirv_OpenCL_clz_v8i32( uint8 x );
uint16 __builtin_spirv_OpenCL_clz_v16i32( uint16 x );
ulong __builtin_spirv_OpenCL_clz_i64( ulong x );
ulong2 __builtin_spirv_OpenCL_clz_v2i64( ulong2 x );
ulong3 __builtin_spirv_OpenCL_clz_v3i64( ulong3 x );
ulong4 __builtin_spirv_OpenCL_clz_v4i64( ulong4 x );
ulong8 __builtin_spirv_OpenCL_clz_v8i64( ulong8 x );
ulong16 __builtin_spirv_OpenCL_clz_v16i64( ulong16 x );

uchar __builtin_spirv_OpenCL_ctz_i8(uchar x );
uchar2 __builtin_spirv_OpenCL_ctz_v2i8( uchar2 x );
uchar3 __builtin_spirv_OpenCL_ctz_v3i8( uchar3 x );
uchar4 __builtin_spirv_OpenCL_ctz_v4i8( uchar4 x );
uchar8 __builtin_spirv_OpenCL_ctz_v8i8( uchar8 x );
uchar16 __builtin_spirv_OpenCL_ctz_v16i8( uchar16 x );
ushort __builtin_spirv_OpenCL_ctz_i16(ushort x );
ushort2 __builtin_spirv_OpenCL_ctz_v2i16( ushort2 x );
ushort3 __builtin_spirv_OpenCL_ctz_v3i16( ushort3 x );
ushort4 __builtin_spirv_OpenCL_ctz_v4i16( ushort4 x );
ushort8 __builtin_spirv_OpenCL_ctz_v8i16( ushort8 x );
ushort16 __builtin_spirv_OpenCL_ctz_v16i16( ushort16 x );
uint __builtin_spirv_OpenCL_ctz_i32(uint x );
uint2 __builtin_spirv_OpenCL_ctz_v2i32( uint2 x );
uint3 __builtin_spirv_OpenCL_ctz_v3i32( uint3 x );
uint4 __builtin_spirv_OpenCL_ctz_v4i32( uint4 x );
uint8 __builtin_spirv_OpenCL_ctz_v8i32( uint8 x );
uint16 __builtin_spirv_OpenCL_ctz_v16i32( uint16 x );
ulong __builtin_spirv_OpenCL_ctz_i64( ulong x );
ulong2 __builtin_spirv_OpenCL_ctz_v2i64( ulong2 x );
ulong3 __builtin_spirv_OpenCL_ctz_v3i64( ulong3 x );
ulong4 __builtin_spirv_OpenCL_ctz_v4i64( ulong4 x );
ulong8 __builtin_spirv_OpenCL_ctz_v8i64( ulong8 x );
ulong16 __builtin_spirv_OpenCL_ctz_v16i64( ulong16 x );

char __builtin_spirv_OpenCL_s_hadd_i8_i8( char x, char y );
char2 __builtin_spirv_OpenCL_s_hadd_v2i8_v2i8( char2 x, char2 y );
char3 __builtin_spirv_OpenCL_s_hadd_v3i8_v3i8( char3 x, char3 y );
char4 __builtin_spirv_OpenCL_s_hadd_v4i8_v4i8( char4 x, char4 y );
char8 __builtin_spirv_OpenCL_s_hadd_v8i8_v8i8( char8 x, char8 y );
char16 __builtin_spirv_OpenCL_s_hadd_v16i8_v16i8( char16 x, char16 y );
uchar __builtin_spirv_OpenCL_u_hadd_i8_i8( uchar x, uchar y );
uchar2 __builtin_spirv_OpenCL_u_hadd_v2i8_v2i8( uchar2 x, uchar2 y );
uchar3 __builtin_spirv_OpenCL_u_hadd_v3i8_v3i8( uchar3 x, uchar3 y );
uchar4 __builtin_spirv_OpenCL_u_hadd_v4i8_v4i8( uchar4 x, uchar4 y );
uchar8 __builtin_spirv_OpenCL_u_hadd_v8i8_v8i8( uchar8 x, uchar8 y );
uchar16 __builtin_spirv_OpenCL_u_hadd_v16i8_v16i8( uchar16 x, uchar16 y );
short __builtin_spirv_OpenCL_s_hadd_i16_i16( short x, short y );
short2 __builtin_spirv_OpenCL_s_hadd_v2i16_v2i16( short2 x, short2 y );
short3 __builtin_spirv_OpenCL_s_hadd_v3i16_v3i16( short3 x, short3 y );
short4 __builtin_spirv_OpenCL_s_hadd_v4i16_v4i16( short4 x, short4 y );
short8 __builtin_spirv_OpenCL_s_hadd_v8i16_v8i16( short8 x, short8 y );
short16 __builtin_spirv_OpenCL_s_hadd_v16i16_v16i16( short16 x, short16 y );
ushort __builtin_spirv_OpenCL_u_hadd_i16_i16( ushort x, ushort y );
ushort2 __builtin_spirv_OpenCL_u_hadd_v2i16_v2i16( ushort2 x, ushort2 y );
ushort3 __builtin_spirv_OpenCL_u_hadd_v3i16_v3i16( ushort3 x, ushort3 y );
ushort4 __builtin_spirv_OpenCL_u_hadd_v4i16_v4i16( ushort4 x, ushort4 y );
ushort8 __builtin_spirv_OpenCL_u_hadd_v8i16_v8i16( ushort8 x, ushort8 y );
ushort16 __builtin_spirv_OpenCL_u_hadd_v16i16_v16i16( ushort16 x, ushort16 y );
int __builtin_spirv_OpenCL_s_hadd_i32_i32( int x, int y );
int2 __builtin_spirv_OpenCL_s_hadd_v2i32_v2i32( int2 x, int2 y );
int3 __builtin_spirv_OpenCL_s_hadd_v3i32_v3i32( int3 x, int3 y );
int4 __builtin_spirv_OpenCL_s_hadd_v4i32_v4i32( int4 x, int4 y );
int8 __builtin_spirv_OpenCL_s_hadd_v8i32_v8i32( int8 x, int8 y );
int16 __builtin_spirv_OpenCL_s_hadd_v16i32_v16i32( int16 x, int16 y );
uint __builtin_spirv_OpenCL_u_hadd_i32_i32( uint x, uint y );
uint2 __builtin_spirv_OpenCL_u_hadd_v2i32_v2i32( uint2 x, uint2 y );
uint3 __builtin_spirv_OpenCL_u_hadd_v3i32_v3i32( uint3 x, uint3 y );
uint4 __builtin_spirv_OpenCL_u_hadd_v4i32_v4i32( uint4 x, uint4 y );
uint8 __builtin_spirv_OpenCL_u_hadd_v8i32_v8i32( uint8 x, uint8 y );
uint16 __builtin_spirv_OpenCL_u_hadd_v16i32_v16i32( uint16 x, uint16 y );
long __builtin_spirv_OpenCL_s_hadd_i64_i64( long x, long y );
long2 __builtin_spirv_OpenCL_s_hadd_v2i64_v2i64( long2 x, long2 y );
long3 __builtin_spirv_OpenCL_s_hadd_v3i64_v3i64( long3 x, long3 y );
long4 __builtin_spirv_OpenCL_s_hadd_v4i64_v4i64( long4 x, long4 y );
long8 __builtin_spirv_OpenCL_s_hadd_v8i64_v8i64( long8 x, long8 y );
long16 __builtin_spirv_OpenCL_s_hadd_v16i64_v16i64( long16 x, long16 y );
ulong __builtin_spirv_OpenCL_u_hadd_i64_i64( ulong x, ulong y );
ulong2 __builtin_spirv_OpenCL_u_hadd_v2i64_v2i64( ulong2 x, ulong2 y );
ulong3 __builtin_spirv_OpenCL_u_hadd_v3i64_v3i64( ulong3 x, ulong3 y );
ulong4 __builtin_spirv_OpenCL_u_hadd_v4i64_v4i64( ulong4 x, ulong4 y );
ulong8 __builtin_spirv_OpenCL_u_hadd_v8i64_v8i64( ulong8 x, ulong8 y );
ulong16 __builtin_spirv_OpenCL_u_hadd_v16i64_v16i64( ulong16 x, ulong16 y );

char __builtin_spirv_OpenCL_s_mad_hi_i8_i8_i8( char a, char b, char c );
char2 __builtin_spirv_OpenCL_s_mad_hi_v2i8_v2i8_v2i8( char2 a, char2 b, char2 c );
char3 __builtin_spirv_OpenCL_s_mad_hi_v3i8_v3i8_v3i8( char3 a, char3 b, char3 c );
char4 __builtin_spirv_OpenCL_s_mad_hi_v4i8_v4i8_v4i8( char4 a, char4 b, char4 c );
char8 __builtin_spirv_OpenCL_s_mad_hi_v8i8_v8i8_v8i8( char8 a, char8 b, char8 c );
char16 __builtin_spirv_OpenCL_s_mad_hi_v16i8_v16i8_v16i8( char16 a, char16 b, char16 c );
uchar __builtin_spirv_OpenCL_u_mad_hi_i8_i8_i8( uchar a, uchar b, uchar c );
uchar2 __builtin_spirv_OpenCL_u_mad_hi_v2i8_v2i8_v2i8( uchar2 a, uchar2 b, uchar2 c );
uchar3 __builtin_spirv_OpenCL_u_mad_hi_v3i8_v3i8_v3i8( uchar3 a, uchar3 b, uchar3 c );
uchar4 __builtin_spirv_OpenCL_u_mad_hi_v4i8_v4i8_v4i8( uchar4 a, uchar4 b, uchar4 c );
uchar8 __builtin_spirv_OpenCL_u_mad_hi_v8i8_v8i8_v8i8( uchar8 a, uchar8 b, uchar8 c );
uchar16 __builtin_spirv_OpenCL_u_mad_hi_v16i8_v16i8_v16i8( uchar16 a, uchar16 b, uchar16 c );
short __builtin_spirv_OpenCL_s_mad_hi_i16_i16_i16( short a, short b, short c );
short2 __builtin_spirv_OpenCL_s_mad_hi_v2i16_v2i16_v2i16( short2 a, short2 b, short2 c );
short3 __builtin_spirv_OpenCL_s_mad_hi_v3i16_v3i16_v3i16( short3 a, short3 b, short3 c );
short4 __builtin_spirv_OpenCL_s_mad_hi_v4i16_v4i16_v4i16( short4 a, short4 b, short4 c );
short8 __builtin_spirv_OpenCL_s_mad_hi_v8i16_v8i16_v8i16( short8 a, short8 b, short8 c );
short16 __builtin_spirv_OpenCL_s_mad_hi_v16i16_v16i16_v16i16( short16 a, short16 b, short16 c );
ushort __builtin_spirv_OpenCL_u_mad_hi_i16_i16_i16( ushort a, ushort b, ushort c );
ushort2 __builtin_spirv_OpenCL_u_mad_hi_v2i16_v2i16_v2i16( ushort2 a, ushort2 b, ushort2 c );
ushort3 __builtin_spirv_OpenCL_u_mad_hi_v3i16_v3i16_v3i16( ushort3 a, ushort3 b, ushort3 c );
ushort4 __builtin_spirv_OpenCL_u_mad_hi_v4i16_v4i16_v4i16( ushort4 a, ushort4 b, ushort4 c );
ushort8 __builtin_spirv_OpenCL_u_mad_hi_v8i16_v8i16_v8i16( ushort8 a, ushort8 b, ushort8 c );
ushort16 __builtin_spirv_OpenCL_u_mad_hi_v16i16_v16i16_v16i16( ushort16 a, ushort16 b, ushort16 c );
int __builtin_spirv_OpenCL_s_mad_hi_i32_i32_i32( int a, int b, int c );
int2 __builtin_spirv_OpenCL_s_mad_hi_v2i32_v2i32_v2i32( int2 a, int2 b, int2 c );
int3 __builtin_spirv_OpenCL_s_mad_hi_v3i32_v3i32_v3i32( int3 a, int3 b, int3 c );
int4 __builtin_spirv_OpenCL_s_mad_hi_v4i32_v4i32_v4i32( int4 a, int4 b, int4 c );
int8 __builtin_spirv_OpenCL_s_mad_hi_v8i32_v8i32_v8i32( int8 a, int8 b, int8 c );
int16 __builtin_spirv_OpenCL_s_mad_hi_v16i32_v16i32_v16i32( int16 a, int16 b, int16 c );
uint __builtin_spirv_OpenCL_u_mad_hi_i32_i32_i32( uint a, uint b, uint c );
uint2 __builtin_spirv_OpenCL_u_mad_hi_v2i32_v2i32_v2i32( uint2 a, uint2 b, uint2 c );
uint3 __builtin_spirv_OpenCL_u_mad_hi_v3i32_v3i32_v3i32( uint3 a, uint3 b, uint3 c );
uint4 __builtin_spirv_OpenCL_u_mad_hi_v4i32_v4i32_v4i32( uint4 a, uint4 b, uint4 c );
uint8 __builtin_spirv_OpenCL_u_mad_hi_v8i32_v8i32_v8i32( uint8 a, uint8 b, uint8 c );
uint16 __builtin_spirv_OpenCL_u_mad_hi_v16i32_v16i32_v16i32( uint16 a, uint16 b, uint16 c );
long __builtin_spirv_OpenCL_s_mad_hi_i64_i64_i64( long a, long b, long c );
long2 __builtin_spirv_OpenCL_s_mad_hi_v2i64_v2i64_v2i64( long2 a, long2 b, long2 c );
long3 __builtin_spirv_OpenCL_s_mad_hi_v3i64_v3i64_v3i64( long3 a, long3 b, long3 c );
long4 __builtin_spirv_OpenCL_s_mad_hi_v4i64_v4i64_v4i64( long4 a, long4 b, long4 c );
long8 __builtin_spirv_OpenCL_s_mad_hi_v8i64_v8i64_v8i64( long8 a, long8 b, long8 c );
long16 __builtin_spirv_OpenCL_s_mad_hi_v16i64_v16i64_v16i64( long16 a, long16 b, long16 c );
ulong __builtin_spirv_OpenCL_u_mad_hi_i64_i64_i64( ulong a, ulong b, ulong c );
ulong2 __builtin_spirv_OpenCL_u_mad_hi_v2i64_v2i64_v2i64( ulong2 a, ulong2 b, ulong2 c );
ulong3 __builtin_spirv_OpenCL_u_mad_hi_v3i64_v3i64_v3i64( ulong3 a, ulong3 b, ulong3 c );
ulong4 __builtin_spirv_OpenCL_u_mad_hi_v4i64_v4i64_v4i64( ulong4 a, ulong4 b, ulong4 c );
ulong8 __builtin_spirv_OpenCL_u_mad_hi_v8i64_v8i64_v8i64( ulong8 a, ulong8 b, ulong8 c );
ulong16 __builtin_spirv_OpenCL_u_mad_hi_v16i64_v16i64_v16i64( ulong16 a, ulong16 b, ulong16 c );

char __builtin_spirv_OpenCL_s_mad_sat_i8_i8_i8( char a, char b, char c );
char2 __builtin_spirv_OpenCL_s_mad_sat_v2i8_v2i8_v2i8( char2 a, char2 b, char2 c );
char3 __builtin_spirv_OpenCL_s_mad_sat_v3i8_v3i8_v3i8( char3 a, char3 b, char3 c );
char4 __builtin_spirv_OpenCL_s_mad_sat_v4i8_v4i8_v4i8( char4 a, char4 b, char4 c );
char8 __builtin_spirv_OpenCL_s_mad_sat_v8i8_v8i8_v8i8( char8 a, char8 b, char8 c );
char16 __builtin_spirv_OpenCL_s_mad_sat_v16i8_v16i8_v16i8( char16 a, char16 b, char16 c );
uchar __builtin_spirv_OpenCL_u_mad_sat_i8_i8_i8( uchar a, uchar b, uchar c );
uchar2 __builtin_spirv_OpenCL_u_mad_sat_v2i8_v2i8_v2i8( uchar2 a, uchar2 b, uchar2 c );
uchar3 __builtin_spirv_OpenCL_u_mad_sat_v3i8_v3i8_v3i8( uchar3 a, uchar3 b, uchar3 c );
uchar4 __builtin_spirv_OpenCL_u_mad_sat_v4i8_v4i8_v4i8( uchar4 a, uchar4 b, uchar4 c );
uchar8 __builtin_spirv_OpenCL_u_mad_sat_v8i8_v8i8_v8i8( uchar8 a, uchar8 b, uchar8 c );
uchar16 __builtin_spirv_OpenCL_u_mad_sat_v16i8_v16i8_v16i8( uchar16 a, uchar16 b, uchar16 c );
short __builtin_spirv_OpenCL_s_mad_sat_i16_i16_i16( short a, short b, short c );
short2 __builtin_spirv_OpenCL_s_mad_sat_v2i16_v2i16_v2i16( short2 a, short2 b, short2 c );
short3 __builtin_spirv_OpenCL_s_mad_sat_v3i16_v3i16_v3i16( short3 a, short3 b, short3 c );
short4 __builtin_spirv_OpenCL_s_mad_sat_v4i16_v4i16_v4i16( short4 a, short4 b, short4 c );
short8 __builtin_spirv_OpenCL_s_mad_sat_v8i16_v8i16_v8i16( short8 a, short8 b, short8 c );
short16 __builtin_spirv_OpenCL_s_mad_sat_v16i16_v16i16_v16i16( short16 a, short16 b, short16 c );
ushort __builtin_spirv_OpenCL_u_mad_sat_i16_i16_i16( ushort a, ushort b, ushort c );
ushort2 __builtin_spirv_OpenCL_u_mad_sat_v2i16_v2i16_v2i16( ushort2 a, ushort2 b, ushort2 c );
ushort3 __builtin_spirv_OpenCL_u_mad_sat_v3i16_v3i16_v3i16( ushort3 a, ushort3 b, ushort3 c );
ushort4 __builtin_spirv_OpenCL_u_mad_sat_v4i16_v4i16_v4i16( ushort4 a, ushort4 b, ushort4 c );
ushort8 __builtin_spirv_OpenCL_u_mad_sat_v8i16_v8i16_v8i16( ushort8 a, ushort8 b, ushort8 c );
ushort16 __builtin_spirv_OpenCL_u_mad_sat_v16i16_v16i16_v16i16( ushort16 a, ushort16 b, ushort16 c );
int __builtin_spirv_OpenCL_s_mad_sat_i32_i32_i32( int a, int b, int c );
int2 __builtin_spirv_OpenCL_s_mad_sat_v2i32_v2i32_v2i32( int2 a, int2 b, int2 c );
int3 __builtin_spirv_OpenCL_s_mad_sat_v3i32_v3i32_v3i32( int3 a, int3 b, int3 c );
int4 __builtin_spirv_OpenCL_s_mad_sat_v4i32_v4i32_v4i32( int4 a, int4 b, int4 c );
int8 __builtin_spirv_OpenCL_s_mad_sat_v8i32_v8i32_v8i32( int8 a, int8 b, int8 c );
int16 __builtin_spirv_OpenCL_s_mad_sat_v16i32_v16i32_v16i32( int16 a, int16 b, int16 c );
uint __builtin_spirv_OpenCL_u_mad_sat_i32_i32_i32( uint a, uint b, uint c );
uint2 __builtin_spirv_OpenCL_u_mad_sat_v2i32_v2i32_v2i32( uint2 a, uint2 b, uint2 c );
uint3 __builtin_spirv_OpenCL_u_mad_sat_v3i32_v3i32_v3i32( uint3 a, uint3 b, uint3 c );
uint4 __builtin_spirv_OpenCL_u_mad_sat_v4i32_v4i32_v4i32( uint4 a, uint4 b, uint4 c );
uint8 __builtin_spirv_OpenCL_u_mad_sat_v8i32_v8i32_v8i32( uint8 a, uint8 b, uint8 c );
uint16 __builtin_spirv_OpenCL_u_mad_sat_v16i32_v16i32_v16i32( uint16 a, uint16 b, uint16 c );
long __builtin_spirv_OpenCL_s_mad_sat_i64_i64_i64( long a, long b, long c );
long2 __builtin_spirv_OpenCL_s_mad_sat_v2i64_v2i64_v2i64( long2 a, long2 b, long2 c );
long3 __builtin_spirv_OpenCL_s_mad_sat_v3i64_v3i64_v3i64( long3 a, long3 b, long3 c );
long4 __builtin_spirv_OpenCL_s_mad_sat_v4i64_v4i64_v4i64( long4 a, long4 b, long4 c );
long8 __builtin_spirv_OpenCL_s_mad_sat_v8i64_v8i64_v8i64( long8 a, long8 b, long8 c );
long16 __builtin_spirv_OpenCL_s_mad_sat_v16i64_v16i64_v16i64( long16 a, long16 b, long16 c );
ulong __builtin_spirv_OpenCL_u_mad_sat_i64_i64_i64( ulong a, ulong b, ulong c );
ulong2 __builtin_spirv_OpenCL_u_mad_sat_v2i64_v2i64_v2i64( ulong2 a, ulong2 b, ulong2 c );
ulong3 __builtin_spirv_OpenCL_u_mad_sat_v3i64_v3i64_v3i64( ulong3 a, ulong3 b, ulong3 c );
ulong4 __builtin_spirv_OpenCL_u_mad_sat_v4i64_v4i64_v4i64( ulong4 a, ulong4 b, ulong4 c );
ulong8 __builtin_spirv_OpenCL_u_mad_sat_v8i64_v8i64_v8i64( ulong8 a, ulong8 b, ulong8 c );
ulong16 __builtin_spirv_OpenCL_u_mad_sat_v16i64_v16i64_v16i64( ulong16 a, ulong16 b, ulong16 c );

int __builtin_spirv_OpenCL_s_mad24_i32_i32_i32( int x, int y, int z );
int2 __builtin_spirv_OpenCL_s_mad24_v2i32_v2i32_v2i32( int2 x, int2 y, int2 z );
int3 __builtin_spirv_OpenCL_s_mad24_v3i32_v3i32_v3i32( int3 x, int3 y, int3 z );
int4 __builtin_spirv_OpenCL_s_mad24_v4i32_v4i32_v4i32( int4 x, int4 y, int4 z );
int8 __builtin_spirv_OpenCL_s_mad24_v8i32_v8i32_v8i32( int8 x, int8 y, int8 z );
int16 __builtin_spirv_OpenCL_s_mad24_v16i32_v16i32_v16i32( int16 x, int16 y, int16 z );
uint __builtin_spirv_OpenCL_u_mad24_i32_i32_i32( uint x, uint y, uint z );
uint2 __builtin_spirv_OpenCL_u_mad24_v2i32_v2i32_v2i32( uint2 x, uint2 y, uint2 z );
uint3 __builtin_spirv_OpenCL_u_mad24_v3i32_v3i32_v3i32( uint3 x, uint3 y, uint3 z );
uint4 __builtin_spirv_OpenCL_u_mad24_v4i32_v4i32_v4i32( uint4 x, uint4 y, uint4 z );
uint8 __builtin_spirv_OpenCL_u_mad24_v8i32_v8i32_v8i32( uint8 x, uint8 y, uint8 z );
uint16 __builtin_spirv_OpenCL_u_mad24_v16i32_v16i32_v16i32( uint16 x, uint16 y, uint16 z );

char __builtin_spirv_OpenCL_s_max_i8_i8(char x, char y);
char2 __builtin_spirv_OpenCL_s_max_v2i8_v2i8(char2 x, char2 y);
char3 __builtin_spirv_OpenCL_s_max_v3i8_v3i8(char3 x, char3 y);
char4 __builtin_spirv_OpenCL_s_max_v4i8_v4i8(char4 x, char4 y);
char8 __builtin_spirv_OpenCL_s_max_v8i8_v8i8(char8 x, char8 y);
char16 __builtin_spirv_OpenCL_s_max_v16i8_v16i8(char16 x, char16 y);
uchar __builtin_spirv_OpenCL_u_max_i8_i8(uchar x, uchar y);
uchar2 __builtin_spirv_OpenCL_u_max_v2i8_v2i8(uchar2 x, uchar2 y);
uchar3 __builtin_spirv_OpenCL_u_max_v3i8_v3i8(uchar3 x, uchar3 y);
uchar4 __builtin_spirv_OpenCL_u_max_v4i8_v4i8(uchar4 x, uchar4 y);
uchar8 __builtin_spirv_OpenCL_u_max_v8i8_v8i8(uchar8 x, uchar8 y);
uchar16 __builtin_spirv_OpenCL_u_max_v16i8_v16i8(uchar16 x, uchar16 y);
short __builtin_spirv_OpenCL_s_max_i16_i16(short x, short y);
short2 __builtin_spirv_OpenCL_s_max_v2i16_v2i16(short2 x, short2 y);
short3 __builtin_spirv_OpenCL_s_max_v3i16_v3i16(short3 x, short3 y);
short4 __builtin_spirv_OpenCL_s_max_v4i16_v4i16(short4 x, short4 y);
short8 __builtin_spirv_OpenCL_s_max_v8i16_v8i16(short8 x, short8 y);
short16 __builtin_spirv_OpenCL_s_max_v16i16_v16i16(short16 x, short16 y);
ushort __builtin_spirv_OpenCL_u_max_i16_i16(ushort x, ushort y);
ushort2 __builtin_spirv_OpenCL_u_max_v2i16_v2i16(ushort2 x, ushort2 y);
ushort3 __builtin_spirv_OpenCL_u_max_v3i16_v3i16(ushort3 x, ushort3 y);
ushort4 __builtin_spirv_OpenCL_u_max_v4i16_v4i16(ushort4 x, ushort4 y);
ushort8 __builtin_spirv_OpenCL_u_max_v8i16_v8i16(ushort8 x, ushort8 y);
ushort16 __builtin_spirv_OpenCL_u_max_v16i16_v16i16(ushort16 x, ushort16 y);
int __builtin_spirv_OpenCL_s_max_i32_i32(int x, int y);
int2 __builtin_spirv_OpenCL_s_max_v2i32_v2i32(int2 x, int2 y);
int3 __builtin_spirv_OpenCL_s_max_v3i32_v3i32(int3 x, int3 y);
int4 __builtin_spirv_OpenCL_s_max_v4i32_v4i32(int4 x, int4 y);
int8 __builtin_spirv_OpenCL_s_max_v8i32_v8i32(int8 x, int8 y);
int16 __builtin_spirv_OpenCL_s_max_v16i32_v16i32(int16 x, int16 y);
uint __builtin_spirv_OpenCL_u_max_i32_i32(uint x, uint y);
uint2 __builtin_spirv_OpenCL_u_max_v2i32_v2i32(uint2 x, uint2 y);
uint3 __builtin_spirv_OpenCL_u_max_v3i32_v3i32(uint3 x, uint3 y);
uint4 __builtin_spirv_OpenCL_u_max_v4i32_v4i32(uint4 x, uint4 y);
uint8 __builtin_spirv_OpenCL_u_max_v8i32_v8i32(uint8 x, uint8 y);
uint16 __builtin_spirv_OpenCL_u_max_v16i32_v16i32(uint16 x, uint16 y);
long __builtin_spirv_OpenCL_s_max_i64_i64(long x, long y);
long2 __builtin_spirv_OpenCL_s_max_v2i64_v2i64(long2 x, long2 y);
long3 __builtin_spirv_OpenCL_s_max_v3i64_v3i64(long3 x, long3 y);
long4 __builtin_spirv_OpenCL_s_max_v4i64_v4i64(long4 x, long4 y);
long8 __builtin_spirv_OpenCL_s_max_v8i64_v8i64(long8 x, long8 y);
long16 __builtin_spirv_OpenCL_s_max_v16i64_v16i64(long16 x, long16 y);
ulong __builtin_spirv_OpenCL_u_max_i64_i64(ulong x, ulong y);
ulong2 __builtin_spirv_OpenCL_u_max_v2i64_v2i64(ulong2 x, ulong2 y);
ulong3 __builtin_spirv_OpenCL_u_max_v3i64_v3i64(ulong3 x, ulong3 y);
ulong4 __builtin_spirv_OpenCL_u_max_v4i64_v4i64(ulong4 x, ulong4 y);
ulong8 __builtin_spirv_OpenCL_u_max_v8i64_v8i64(ulong8 x, ulong8 y);
ulong16 __builtin_spirv_OpenCL_u_max_v16i64_v16i64(ulong16 x, ulong16 y);

char __builtin_spirv_OpenCL_s_mul_hi_i8_i8( char x, char y );
char2 __builtin_spirv_OpenCL_s_mul_hi_v2i8_v2i8( char2 x, char2 y );
char3 __builtin_spirv_OpenCL_s_mul_hi_v3i8_v3i8( char3 x, char3 y );
char4 __builtin_spirv_OpenCL_s_mul_hi_v4i8_v4i8( char4 x, char4 y );
char8 __builtin_spirv_OpenCL_s_mul_hi_v8i8_v8i8( char8 x, char8 y );
char16 __builtin_spirv_OpenCL_s_mul_hi_v16i8_v16i8( char16 x, char16 y );
uchar __builtin_spirv_OpenCL_u_mul_hi_i8_i8( uchar x, uchar y );
uchar2 __builtin_spirv_OpenCL_u_mul_hi_v2i8_v2i8( uchar2 x, uchar2 y );
uchar3 __builtin_spirv_OpenCL_u_mul_hi_v3i8_v3i8( uchar3 x, uchar3 y );
uchar4 __builtin_spirv_OpenCL_u_mul_hi_v4i8_v4i8( uchar4 x, uchar4 y );
uchar8 __builtin_spirv_OpenCL_u_mul_hi_v8i8_v8i8( uchar8 x, uchar8 y );
uchar16 __builtin_spirv_OpenCL_u_mul_hi_v16i8_v16i8( uchar16 x, uchar16 y );
short __builtin_spirv_OpenCL_s_mul_hi_i16_i16( short x, short y );
short2 __builtin_spirv_OpenCL_s_mul_hi_v2i16_v2i16( short2 x, short2 y );
short3 __builtin_spirv_OpenCL_s_mul_hi_v3i16_v3i16( short3 x, short3 y );
short4 __builtin_spirv_OpenCL_s_mul_hi_v4i16_v4i16( short4 x, short4 y );
short8 __builtin_spirv_OpenCL_s_mul_hi_v8i16_v8i16( short8 x, short8 y );
short16 __builtin_spirv_OpenCL_s_mul_hi_v16i16_v16i16( short16 x, short16 y );
ushort __builtin_spirv_OpenCL_u_mul_hi_i16_i16( ushort x, ushort y );
ushort2 __builtin_spirv_OpenCL_u_mul_hi_v2i16_v2i16( ushort2 x, ushort2 y );
ushort3 __builtin_spirv_OpenCL_u_mul_hi_v3i16_v3i16( ushort3 x, ushort3 y );
ushort4 __builtin_spirv_OpenCL_u_mul_hi_v4i16_v4i16( ushort4 x, ushort4 y );
ushort8 __builtin_spirv_OpenCL_u_mul_hi_v8i16_v8i16( ushort8 x, ushort8 y );
ushort16 __builtin_spirv_OpenCL_u_mul_hi_v16i16_v16i16( ushort16 x, ushort16 y );
int __builtin_spirv_OpenCL_s_mul_hi_i32_i32( int x, int y );
int2 __builtin_spirv_OpenCL_s_mul_hi_v2i32_v2i32( int2 x, int2 y );
int3 __builtin_spirv_OpenCL_s_mul_hi_v3i32_v3i32( int3 x, int3 y );
int4 __builtin_spirv_OpenCL_s_mul_hi_v4i32_v4i32( int4 x, int4 y );
int8 __builtin_spirv_OpenCL_s_mul_hi_v8i32_v8i32( int8 x, int8 y );
int16 __builtin_spirv_OpenCL_s_mul_hi_v16i32_v16i32( int16 x, int16 y );
uint __builtin_spirv_OpenCL_u_mul_hi_i32_i32( uint x, uint y );
uint2 __builtin_spirv_OpenCL_u_mul_hi_v2i32_v2i32( uint2 x, uint2 y );
uint3 __builtin_spirv_OpenCL_u_mul_hi_v3i32_v3i32( uint3 x, uint3 y );
uint4 __builtin_spirv_OpenCL_u_mul_hi_v4i32_v4i32( uint4 x, uint4 y );
uint8 __builtin_spirv_OpenCL_u_mul_hi_v8i32_v8i32( uint8 x, uint8 y );
uint16 __builtin_spirv_OpenCL_u_mul_hi_v16i32_v16i32( uint16 x, uint16 y );
long __builtin_spirv_OpenCL_s_mul_hi_i64_i64( long x, long y );
long2 __builtin_spirv_OpenCL_s_mul_hi_v2i64_v2i64( long2 x, long2 y );
long3 __builtin_spirv_OpenCL_s_mul_hi_v3i64_v3i64( long3 x, long3 y );
long4 __builtin_spirv_OpenCL_s_mul_hi_v4i64_v4i64( long4 x, long4 y );
long8 __builtin_spirv_OpenCL_s_mul_hi_v8i64_v8i64( long8 x, long8 y );
long16 __builtin_spirv_OpenCL_s_mul_hi_v16i64_v16i64( long16 x, long16 y );
ulong __builtin_spirv_OpenCL_u_mul_hi_i64_i64( ulong x, ulong y );
ulong2 __builtin_spirv_OpenCL_u_mul_hi_v2i64_v2i64( ulong2 x, ulong2 y );
ulong3 __builtin_spirv_OpenCL_u_mul_hi_v3i64_v3i64( ulong3 x, ulong3 y );
ulong4 __builtin_spirv_OpenCL_u_mul_hi_v4i64_v4i64( ulong4 x, ulong4 y );
ulong8 __builtin_spirv_OpenCL_u_mul_hi_v8i64_v8i64( ulong8 x, ulong8 y );
ulong16 __builtin_spirv_OpenCL_u_mul_hi_v16i64_v16i64( ulong16 x, ulong16 y );

int __builtin_spirv_OpenCL_s_mul24_i32_i32( int x, int y );
int2 __builtin_spirv_OpenCL_s_mul24_v2i32_v2i32( int2 x, int2 y );
int3 __builtin_spirv_OpenCL_s_mul24_v3i32_v3i32( int3 x, int3 y );
int4 __builtin_spirv_OpenCL_s_mul24_v4i32_v4i32( int4 x, int4 y );
int8 __builtin_spirv_OpenCL_s_mul24_v8i32_v8i32( int8 x, int8 y );
int16 __builtin_spirv_OpenCL_s_mul24_v16i32_v16i32( int16 x, int16 y );
uint __builtin_spirv_OpenCL_u_mul24_i32_i32( uint x, uint y );
uint2 __builtin_spirv_OpenCL_u_mul24_v2i32_v2i32( uint2 x, uint2 y );
uint3 __builtin_spirv_OpenCL_u_mul24_v3i32_v3i32( uint3 x, uint3 y );
uint4 __builtin_spirv_OpenCL_u_mul24_v4i32_v4i32( uint4 x, uint4 y );
uint8 __builtin_spirv_OpenCL_u_mul24_v8i32_v8i32( uint8 x, uint8 y );
uint16 __builtin_spirv_OpenCL_u_mul24_v16i32_v16i32( uint16 x, uint16 y );

uchar __builtin_spirv_OpenCL_popcount_i8(uchar x );
uchar2 __builtin_spirv_OpenCL_popcount_v2i8(uchar2 x);
uchar3 __builtin_spirv_OpenCL_popcount_v3i8(uchar3 x);
uchar4 __builtin_spirv_OpenCL_popcount_v4i8(uchar4 x);
uchar8 __builtin_spirv_OpenCL_popcount_v8i8(uchar8 x);
uchar16 __builtin_spirv_OpenCL_popcount_v16i8(uchar16 x);
ushort __builtin_spirv_OpenCL_popcount_i16(ushort x );
ushort2 __builtin_spirv_OpenCL_popcount_v2i16(ushort2 x);
ushort3 __builtin_spirv_OpenCL_popcount_v3i16(ushort3 x);
ushort4 __builtin_spirv_OpenCL_popcount_v4i16(ushort4 x);
ushort8 __builtin_spirv_OpenCL_popcount_v8i16(ushort8 x);
ushort16 __builtin_spirv_OpenCL_popcount_v16i16(ushort16 x);
uint __builtin_spirv_OpenCL_popcount_i32(uint x );
uint2 __builtin_spirv_OpenCL_popcount_v2i32(uint2 x);
uint3 __builtin_spirv_OpenCL_popcount_v3i32(uint3 x);
uint4 __builtin_spirv_OpenCL_popcount_v4i32(uint4 x);
uint8 __builtin_spirv_OpenCL_popcount_v8i32(uint8 x);
uint16 __builtin_spirv_OpenCL_popcount_v16i32(uint16 x);
ulong __builtin_spirv_OpenCL_popcount_i64(ulong x);
ulong2 __builtin_spirv_OpenCL_popcount_v2i64(ulong2 x);
ulong3 __builtin_spirv_OpenCL_popcount_v3i64(ulong3 x);
ulong4 __builtin_spirv_OpenCL_popcount_v4i64(ulong4 x);
ulong8 __builtin_spirv_OpenCL_popcount_v8i64(ulong8 x);
ulong16 __builtin_spirv_OpenCL_popcount_v16i64(ulong16 x);

char __builtin_spirv_OpenCL_s_rhadd_i8_i8( char x, char y );
char2 __builtin_spirv_OpenCL_s_rhadd_v2i8_v2i8( char2 x, char2 y );
char3 __builtin_spirv_OpenCL_s_rhadd_v3i8_v3i8( char3 x, char3 y );
char4 __builtin_spirv_OpenCL_s_rhadd_v4i8_v4i8( char4 x, char4 y );
char8 __builtin_spirv_OpenCL_s_rhadd_v8i8_v8i8( char8 x, char8 y );
char16 __builtin_spirv_OpenCL_s_rhadd_v16i8_v16i8( char16 x, char16 y );
uchar __builtin_spirv_OpenCL_u_rhadd_i8_i8( uchar x, uchar y );
uchar2 __builtin_spirv_OpenCL_u_rhadd_v2i8_v2i8( uchar2 x, uchar2 y );
uchar3 __builtin_spirv_OpenCL_u_rhadd_v3i8_v3i8( uchar3 x, uchar3 y );
uchar4 __builtin_spirv_OpenCL_u_rhadd_v4i8_v4i8( uchar4 x, uchar4 y );
uchar8 __builtin_spirv_OpenCL_u_rhadd_v8i8_v8i8( uchar8 x, uchar8 y );
uchar16 __builtin_spirv_OpenCL_u_rhadd_v16i8_v16i8( uchar16 x, uchar16 y );
short __builtin_spirv_OpenCL_s_rhadd_i16_i16( short x, short y );
short2 __builtin_spirv_OpenCL_s_rhadd_v2i16_v2i16( short2 x, short2 y );
short3 __builtin_spirv_OpenCL_s_rhadd_v3i16_v3i16( short3 x, short3 y );
short4 __builtin_spirv_OpenCL_s_rhadd_v4i16_v4i16( short4 x, short4 y );
short8 __builtin_spirv_OpenCL_s_rhadd_v8i16_v8i16( short8 x, short8 y );
short16 __builtin_spirv_OpenCL_s_rhadd_v16i16_v16i16( short16 x, short16 y );
ushort __builtin_spirv_OpenCL_u_rhadd_i16_i16( ushort x, ushort y );
ushort2 __builtin_spirv_OpenCL_u_rhadd_v2i16_v2i16( ushort2 x, ushort2 y );
ushort3 __builtin_spirv_OpenCL_u_rhadd_v3i16_v3i16( ushort3 x, ushort3 y );
ushort4 __builtin_spirv_OpenCL_u_rhadd_v4i16_v4i16( ushort4 x, ushort4 y );
ushort8 __builtin_spirv_OpenCL_u_rhadd_v8i16_v8i16( ushort8 x, ushort8 y );
ushort16 __builtin_spirv_OpenCL_u_rhadd_v16i16_v16i16( ushort16 x, ushort16 y );
int __builtin_spirv_OpenCL_s_rhadd_i32_i32( int x, int y );
int2 __builtin_spirv_OpenCL_s_rhadd_v2i32_v2i32( int2 x, int2 y );
int3 __builtin_spirv_OpenCL_s_rhadd_v3i32_v3i32( int3 x, int3 y );
int4 __builtin_spirv_OpenCL_s_rhadd_v4i32_v4i32( int4 x, int4 y );
int8 __builtin_spirv_OpenCL_s_rhadd_v8i32_v8i32( int8 x, int8 y );
int16 __builtin_spirv_OpenCL_s_rhadd_v16i32_v16i32( int16 x, int16 y );
uint __builtin_spirv_OpenCL_u_rhadd_i32_i32( uint x, uint y );
uint2 __builtin_spirv_OpenCL_u_rhadd_v2i32_v2i32( uint2 x, uint2 y );
uint3 __builtin_spirv_OpenCL_u_rhadd_v3i32_v3i32( uint3 x, uint3 y );
uint4 __builtin_spirv_OpenCL_u_rhadd_v4i32_v4i32( uint4 x, uint4 y );
uint8 __builtin_spirv_OpenCL_u_rhadd_v8i32_v8i32( uint8 x, uint8 y );
uint16 __builtin_spirv_OpenCL_u_rhadd_v16i32_v16i32( uint16 x, uint16 y );
long __builtin_spirv_OpenCL_s_rhadd_i64_i64( long x, long y );
long2 __builtin_spirv_OpenCL_s_rhadd_v2i64_v2i64( long2 x, long2 y );
long3 __builtin_spirv_OpenCL_s_rhadd_v3i64_v3i64( long3 x, long3 y );
long4 __builtin_spirv_OpenCL_s_rhadd_v4i64_v4i64( long4 x, long4 y );
long8 __builtin_spirv_OpenCL_s_rhadd_v8i64_v8i64( long8 x, long8 y );
long16 __builtin_spirv_OpenCL_s_rhadd_v16i64_v16i64( long16 x, long16 y );
ulong __builtin_spirv_OpenCL_u_rhadd_i64_i64( ulong x, ulong y );
ulong2 __builtin_spirv_OpenCL_u_rhadd_v2i64_v2i64( ulong2 x, ulong2 y );
ulong3 __builtin_spirv_OpenCL_u_rhadd_v3i64_v3i64( ulong3 x, ulong3 y );
ulong4 __builtin_spirv_OpenCL_u_rhadd_v4i64_v4i64( ulong4 x, ulong4 y );
ulong8 __builtin_spirv_OpenCL_u_rhadd_v8i64_v8i64( ulong8 x, ulong8 y );
ulong16 __builtin_spirv_OpenCL_u_rhadd_v16i64_v16i64( ulong16 x, ulong16 y );

uchar __builtin_spirv_OpenCL_rotate_i8_i8( uchar v, uchar i );
uchar2 __builtin_spirv_OpenCL_rotate_v2i8_v2i8( uchar2 v, uchar2 i );
uchar3 __builtin_spirv_OpenCL_rotate_v3i8_v3i8( uchar3 v, uchar3 i );
uchar4 __builtin_spirv_OpenCL_rotate_v4i8_v4i8( uchar4 v, uchar4 i );
uchar8 __builtin_spirv_OpenCL_rotate_v8i8_v8i8( uchar8 v, uchar8 i );
uchar16 __builtin_spirv_OpenCL_rotate_v16i8_v16i8( uchar16 v, uchar16 i );
ushort __builtin_spirv_OpenCL_rotate_i16_i16( ushort v, ushort i );
ushort2 __builtin_spirv_OpenCL_rotate_v2i16_v2i16( ushort2 v, ushort2 i );
ushort3 __builtin_spirv_OpenCL_rotate_v3i16_v3i16( ushort3 v, ushort3 i );
ushort4 __builtin_spirv_OpenCL_rotate_v4i16_v4i16( ushort4 v, ushort4 i );
ushort8 __builtin_spirv_OpenCL_rotate_v8i16_v8i16( ushort8 v, ushort8 i );
ushort16 __builtin_spirv_OpenCL_rotate_v16i16_v16i16( ushort16 v, ushort16 i );
uint __builtin_spirv_OpenCL_rotate_i32_i32( uint v, uint i );
uint2 __builtin_spirv_OpenCL_rotate_v2i32_v2i32( uint2 v, uint2 i );
uint3 __builtin_spirv_OpenCL_rotate_v3i32_v3i32( uint3 v, uint3 i );
uint4 __builtin_spirv_OpenCL_rotate_v4i32_v4i32( uint4 v, uint4 i );
uint8 __builtin_spirv_OpenCL_rotate_v8i32_v8i32( uint8 v, uint8 i );
uint16 __builtin_spirv_OpenCL_rotate_v16i32_v16i32( uint16 v, uint16 i );
ulong __builtin_spirv_OpenCL_rotate_i64_i64( ulong v, ulong i );
ulong2 __builtin_spirv_OpenCL_rotate_v2i64_v2i64( ulong2 v, ulong2 i );
ulong3 __builtin_spirv_OpenCL_rotate_v3i64_v3i64( ulong3 v, ulong3 i );
ulong4 __builtin_spirv_OpenCL_rotate_v4i64_v4i64( ulong4 v, ulong4 i );
ulong8 __builtin_spirv_OpenCL_rotate_v8i64_v8i64( ulong8 v, ulong8 i );
ulong16 __builtin_spirv_OpenCL_rotate_v16i64_v16i64( ulong16 v, ulong16 i );

char __builtin_spirv_OpenCL_s_sub_sat_i8_i8( char x, char y );
char2 __builtin_spirv_OpenCL_s_sub_sat_v2i8_v2i8( char2 x, char2 y );
char3 __builtin_spirv_OpenCL_s_sub_sat_v3i8_v3i8( char3 x, char3 y );
char4 __builtin_spirv_OpenCL_s_sub_sat_v4i8_v4i8( char4 x, char4 y );
char8 __builtin_spirv_OpenCL_s_sub_sat_v8i8_v8i8( char8 x, char8 y );
char16 __builtin_spirv_OpenCL_s_sub_sat_v16i8_v16i8( char16 x, char16 y );
uchar __builtin_spirv_OpenCL_u_sub_sat_i8_i8( uchar x, uchar y );
uchar2 __builtin_spirv_OpenCL_u_sub_sat_v2i8_v2i8( uchar2 x, uchar2 y );
uchar3 __builtin_spirv_OpenCL_u_sub_sat_v3i8_v3i8( uchar3 x, uchar3 y );
uchar4 __builtin_spirv_OpenCL_u_sub_sat_v4i8_v4i8( uchar4 x, uchar4 y );
uchar8 __builtin_spirv_OpenCL_u_sub_sat_v8i8_v8i8( uchar8 x, uchar8 y );
uchar16 __builtin_spirv_OpenCL_u_sub_sat_v16i8_v16i8( uchar16 x, uchar16 y );
short __builtin_spirv_OpenCL_s_sub_sat_i16_i16( short x, short y );
short2 __builtin_spirv_OpenCL_s_sub_sat_v2i16_v2i16( short2 x, short2 y );
short3 __builtin_spirv_OpenCL_s_sub_sat_v3i16_v3i16( short3 x, short3 y );
short4 __builtin_spirv_OpenCL_s_sub_sat_v4i16_v4i16( short4 x, short4 y );
short8 __builtin_spirv_OpenCL_s_sub_sat_v8i16_v8i16( short8 x, short8 y );
short16 __builtin_spirv_OpenCL_s_sub_sat_v16i16_v16i16( short16 x, short16 y );
ushort __builtin_spirv_OpenCL_u_sub_sat_i16_i16( ushort x, ushort y );
ushort2 __builtin_spirv_OpenCL_u_sub_sat_v2i16_v2i16( ushort2 x, ushort2 y );
ushort3 __builtin_spirv_OpenCL_u_sub_sat_v3i16_v3i16( ushort3 x, ushort3 y );
ushort4 __builtin_spirv_OpenCL_u_sub_sat_v4i16_v4i16( ushort4 x, ushort4 y );
ushort8 __builtin_spirv_OpenCL_u_sub_sat_v8i16_v8i16( ushort8 x, ushort8 y );
ushort16 __builtin_spirv_OpenCL_u_sub_sat_v16i16_v16i16( ushort16 x, ushort16 y );
int __builtin_spirv_OpenCL_s_sub_sat_i32_i32( int x, int y );
int2 __builtin_spirv_OpenCL_s_sub_sat_v2i32_v2i32( int2 x, int2 y );
int3 __builtin_spirv_OpenCL_s_sub_sat_v3i32_v3i32( int3 x, int3 y );
int4 __builtin_spirv_OpenCL_s_sub_sat_v4i32_v4i32( int4 x, int4 y );
int8 __builtin_spirv_OpenCL_s_sub_sat_v8i32_v8i32( int8 x, int8 y );
int16 __builtin_spirv_OpenCL_s_sub_sat_v16i32_v16i32( int16 x, int16 y );
uint __builtin_spirv_OpenCL_u_sub_sat_i32_i32( uint x, uint y );
uint2 __builtin_spirv_OpenCL_u_sub_sat_v2i32_v2i32( uint2 x, uint2 y );
uint3 __builtin_spirv_OpenCL_u_sub_sat_v3i32_v3i32( uint3 x, uint3 y );
uint4 __builtin_spirv_OpenCL_u_sub_sat_v4i32_v4i32( uint4 x, uint4 y );
uint8 __builtin_spirv_OpenCL_u_sub_sat_v8i32_v8i32( uint8 x, uint8 y );
uint16 __builtin_spirv_OpenCL_u_sub_sat_v16i32_v16i32( uint16 x, uint16 y );
long __builtin_spirv_OpenCL_s_sub_sat_i64_i64( long x, long y );
long2 __builtin_spirv_OpenCL_s_sub_sat_v2i64_v2i64( long2 x, long2 y );
long3 __builtin_spirv_OpenCL_s_sub_sat_v3i64_v3i64( long3 x, long3 y );
long4 __builtin_spirv_OpenCL_s_sub_sat_v4i64_v4i64( long4 x, long4 y );
long8 __builtin_spirv_OpenCL_s_sub_sat_v8i64_v8i64( long8 x, long8 y );
long16 __builtin_spirv_OpenCL_s_sub_sat_v16i64_v16i64( long16 x, long16 y );
ulong __builtin_spirv_OpenCL_u_sub_sat_i64_i64( ulong x, ulong y );
ulong2 __builtin_spirv_OpenCL_u_sub_sat_v2i64_v2i64( ulong2 x, ulong2 y );
ulong3 __builtin_spirv_OpenCL_u_sub_sat_v3i64_v3i64( ulong3 x, ulong3 y );
ulong4 __builtin_spirv_OpenCL_u_sub_sat_v4i64_v4i64( ulong4 x, ulong4 y );
ulong8 __builtin_spirv_OpenCL_u_sub_sat_v8i64_v8i64( ulong8 x, ulong8 y );
ulong16 __builtin_spirv_OpenCL_u_sub_sat_v16i64_v16i64( ulong16 x, ulong16 y );

short __builtin_spirv_OpenCL_s_upsample_i8_i8( char  hi, uchar lo );
short2 __builtin_spirv_OpenCL_s_upsample_v2i8_v2i8( char2  hi, uchar2 lo );
short3 __builtin_spirv_OpenCL_s_upsample_v3i8_v3i8( char3  hi, uchar3 lo );
short4 __builtin_spirv_OpenCL_s_upsample_v4i8_v4i8( char4  hi, uchar4 lo );
short8 __builtin_spirv_OpenCL_s_upsample_v8i8_v8i8( char8  hi, uchar8 lo );
short16 __builtin_spirv_OpenCL_s_upsample_v16i8_v16i8( char16  hi, uchar16 lo );
ushort __builtin_spirv_OpenCL_u_upsample_i8_i8( uchar hi, uchar lo );
ushort2 __builtin_spirv_OpenCL_u_upsample_v2i8_v2i8( uchar2 hi, uchar2 lo );
ushort3 __builtin_spirv_OpenCL_u_upsample_v3i8_v3i8( uchar3 hi, uchar3 lo );
ushort4 __builtin_spirv_OpenCL_u_upsample_v4i8_v4i8( uchar4 hi, uchar4 lo );
ushort8 __builtin_spirv_OpenCL_u_upsample_v8i8_v8i8( uchar8 hi, uchar8 lo );
ushort16 __builtin_spirv_OpenCL_u_upsample_v16i8_v16i8( uchar16 hi, uchar16 lo );
int __builtin_spirv_OpenCL_s_upsample_i16_i16( short  hi, ushort lo );
int2 __builtin_spirv_OpenCL_s_upsample_v2i16_v2i16( short2  hi, ushort2 lo );
int3 __builtin_spirv_OpenCL_s_upsample_v3i16_v3i16( short3  hi, ushort3 lo );
int4 __builtin_spirv_OpenCL_s_upsample_v4i16_v4i16( short4  hi, ushort4 lo );
int8 __builtin_spirv_OpenCL_s_upsample_v8i16_v8i16( short8  hi, ushort8 lo );
int16 __builtin_spirv_OpenCL_s_upsample_v16i16_v16i16( short16  hi, ushort16 lo );
uint __builtin_spirv_OpenCL_u_upsample_i16_i16( ushort hi, ushort lo );
uint2 __builtin_spirv_OpenCL_u_upsample_v2i16_v2i16( ushort2 hi, ushort2 lo );
uint3 __builtin_spirv_OpenCL_u_upsample_v3i16_v3i16( ushort3 hi, ushort3 lo );
uint4 __builtin_spirv_OpenCL_u_upsample_v4i16_v4i16( ushort4 hi, ushort4 lo );
uint8 __builtin_spirv_OpenCL_u_upsample_v8i16_v8i16( ushort8 hi, ushort8 lo );
uint16 __builtin_spirv_OpenCL_u_upsample_v16i16_v16i16( ushort16 hi, ushort16 lo );
long __builtin_spirv_OpenCL_s_upsample_i32_i32( int  hi, uint lo );
long2 __builtin_spirv_OpenCL_s_upsample_v2i32_v2i32( int2  hi, uint2 lo );
long3 __builtin_spirv_OpenCL_s_upsample_v3i32_v3i32( int3  hi, uint3 lo );
long4 __builtin_spirv_OpenCL_s_upsample_v4i32_v4i32( int4  hi, uint4 lo );
long8 __builtin_spirv_OpenCL_s_upsample_v8i32_v8i32( int8  hi, uint8 lo );
long16 __builtin_spirv_OpenCL_s_upsample_v16i32_v16i32( int16  hi, uint16 lo );
ulong __builtin_spirv_OpenCL_u_upsample_i32_i32( uint hi, uint lo );
ulong2 __builtin_spirv_OpenCL_u_upsample_v2i32_v2i32( uint2 hi, uint2 lo );
ulong3 __builtin_spirv_OpenCL_u_upsample_v3i32_v3i32( uint3 hi, uint3 lo );
ulong4 __builtin_spirv_OpenCL_u_upsample_v4i32_v4i32( uint4 hi, uint4 lo );
ulong8 __builtin_spirv_OpenCL_u_upsample_v8i32_v8i32( uint8 hi, uint8 lo );
ulong16 __builtin_spirv_OpenCL_u_upsample_v16i32_v16i32( uint16 hi, uint16 lo );


//
//  Math_ext
//        -acos,acosh,acospi,asin,asinh,asinpi,atan,atan2,atan2pi,atanh,atanpi,cbrt,ceil,copysign,
//       cos,cosh,cospi,divide_cr,erf,erfc,exp,exp2,exp10,expm1,fabs,fdim,floor,fma,fmax,fmin,
//       fmod,fract,frexp,hypot,ilogb,ldexp,lgamma,lgamma_r,log,log1p,log2,log10,logb,mad,maxmag,minmag,
//         modf,nan,nextafter,pow,pown,powr,remainder,remquo,rint,rootn,round,rsqrt,sin,sincos,
//       sinh,sinpi,sqrt,sqrt_cr,tan,tanh,tanpi,tgamma,trunc
//

float __builtin_spirv_OpenCL_acos_f32(float x );
float2 __builtin_spirv_OpenCL_acos_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_acos_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_acos_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_acos_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_acos_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_acos_f64( double x );
double2 __builtin_spirv_OpenCL_acos_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_acos_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_acos_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_acos_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_acos_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_acos_f16(half x );
half2 __builtin_spirv_OpenCL_acos_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_acos_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_acos_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_acos_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_acos_v16f16(half16 x);

float  __builtin_spirv_OpenCL_acosh_f32( float x );
float2 __builtin_spirv_OpenCL_acosh_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_acosh_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_acosh_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_acosh_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_acosh_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_acosh_f64( double x );
double2 __builtin_spirv_OpenCL_acosh_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_acosh_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_acosh_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_acosh_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_acosh_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_acosh_f16( half x );
half2 __builtin_spirv_OpenCL_acosh_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_acosh_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_acosh_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_acosh_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_acosh_v16f16(half16 x);

float  __builtin_spirv_OpenCL_acospi_f32( float x );
float2 __builtin_spirv_OpenCL_acospi_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_acospi_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_acospi_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_acospi_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_acospi_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_acospi_f64( double x );
double2 __builtin_spirv_OpenCL_acospi_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_acospi_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_acospi_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_acospi_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_acospi_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_acospi_f16( half x );
half2 __builtin_spirv_OpenCL_acospi_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_acospi_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_acospi_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_acospi_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_acospi_v16f16(half16 x);

float __builtin_spirv_OpenCL_asin_f32(float value );
float2 __builtin_spirv_OpenCL_asin_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_asin_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_asin_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_asin_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_asin_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_asin_f64( double x );
double2 __builtin_spirv_OpenCL_asin_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_asin_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_asin_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_asin_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_asin_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_asin_f16(half x );
half2 __builtin_spirv_OpenCL_asin_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_asin_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_asin_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_asin_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_asin_v16f16(half16 x);

float  __builtin_spirv_OpenCL_asinh_f32( float x );
float2 __builtin_spirv_OpenCL_asinh_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_asinh_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_asinh_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_asinh_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_asinh_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_asinh_f64( double x );
double2 __builtin_spirv_OpenCL_asinh_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_asinh_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_asinh_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_asinh_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_asinh_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_asinh_f16( half x );
half2 __builtin_spirv_OpenCL_asinh_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_asinh_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_asinh_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_asinh_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_asinh_v16f16(half16 x);

float  __builtin_spirv_OpenCL_asinpi_f32( float x );
float2 __builtin_spirv_OpenCL_asinpi_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_asinpi_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_asinpi_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_asinpi_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_asinpi_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_asinpi_f64( double x );
double2 __builtin_spirv_OpenCL_asinpi_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_asinpi_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_asinpi_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_asinpi_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_asinpi_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_asinpi_f16( half x );
half2 __builtin_spirv_OpenCL_asinpi_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_asinpi_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_asinpi_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_asinpi_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_asinpi_v16f16(half16 x);

float __builtin_spirv_OpenCL_atan_f32(float value );
float2 __builtin_spirv_OpenCL_atan_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_atan_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_atan_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_atan_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_atan_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_atan_f64( double x );
double2 __builtin_spirv_OpenCL_atan_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_atan_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_atan_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_atan_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_atan_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_atan_f16(half x );
half2 __builtin_spirv_OpenCL_atan_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_atan_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_atan_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_atan_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_atan_v16f16(half16 x);

float  __builtin_spirv_OpenCL_atan2_f32_f32( float y, float x );
float2 __builtin_spirv_OpenCL_atan2_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_atan2_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_atan2_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_atan2_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_atan2_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_atan2_f64_f64( double y, double x );
double2 __builtin_spirv_OpenCL_atan2_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_atan2_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_atan2_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_atan2_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_atan2_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_atan2_f16_f16( half y, half x );
half2 __builtin_spirv_OpenCL_atan2_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_atan2_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_atan2_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_atan2_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_atan2_v16f16_v16f16(half16 x, half16 y);

float  __builtin_spirv_OpenCL_atan2pi_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_atan2pi_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_atan2pi_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_atan2pi_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_atan2pi_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_atan2pi_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_atan2pi_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_atan2pi_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_atan2pi_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_atan2pi_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_atan2pi_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_atan2pi_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_atan2pi_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_atan2pi_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_atan2pi_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_atan2pi_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_atan2pi_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_atan2pi_v16f16_v16f16(half16 x, half16 y);

float  __builtin_spirv_OpenCL_atanh_f32( float x );
float2 __builtin_spirv_OpenCL_atanh_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_atanh_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_atanh_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_atanh_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_atanh_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_atanh_f64( double x );
double2 __builtin_spirv_OpenCL_atanh_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_atanh_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_atanh_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_atanh_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_atanh_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_atanh_f16( half x );
half2 __builtin_spirv_OpenCL_atanh_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_atanh_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_atanh_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_atanh_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_atanh_v16f16(half16 x);

float  __builtin_spirv_OpenCL_atanpi_f32( float x );
float2 __builtin_spirv_OpenCL_atanpi_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_atanpi_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_atanpi_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_atanpi_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_atanpi_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_atanpi_f64( double x );
double2 __builtin_spirv_OpenCL_atanpi_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_atanpi_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_atanpi_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_atanpi_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_atanpi_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_atanpi_f16( half x );
half2 __builtin_spirv_OpenCL_atanpi_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_atanpi_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_atanpi_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_atanpi_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_atanpi_v16f16(half16 x);

float  __builtin_spirv_OpenCL_cbrt_f32( float x );
float2 __builtin_spirv_OpenCL_cbrt_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_cbrt_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_cbrt_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_cbrt_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_cbrt_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_cbrt_f64( double x );
double2 __builtin_spirv_OpenCL_cbrt_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_cbrt_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_cbrt_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_cbrt_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_cbrt_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_cbrt_f16( half x );
half2 __builtin_spirv_OpenCL_cbrt_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_cbrt_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_cbrt_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_cbrt_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_cbrt_v16f16(half16 x);

float __builtin_spirv_OpenCL_ceil_f32(float x );
float2 __builtin_spirv_OpenCL_ceil_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_ceil_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_ceil_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_ceil_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_ceil_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_ceil_f64(double x );
double2 __builtin_spirv_OpenCL_ceil_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_ceil_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_ceil_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_ceil_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_ceil_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_ceil_f16(half x );
half2 __builtin_spirv_OpenCL_ceil_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_ceil_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_ceil_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_ceil_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_ceil_v16f16(half16 x);

float  __builtin_spirv_OpenCL_copysign_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_copysign_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_copysign_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_copysign_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_copysign_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_copysign_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_copysign_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_copysign_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_copysign_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_copysign_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_copysign_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_copysign_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_copysign_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_copysign_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_copysign_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_copysign_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_copysign_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_copysign_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_cos_f32( float x );
float2 __builtin_spirv_OpenCL_cos_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_cos_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_cos_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_cos_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_cos_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_cos_f64( double x );
double2 __builtin_spirv_OpenCL_cos_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_cos_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_cos_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_cos_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_cos_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_cos_f16( half x );
half2 __builtin_spirv_OpenCL_cos_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_cos_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_cos_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_cos_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_cos_v16f16(half16 x);

float  __builtin_spirv_OpenCL_cosh_f32( float x );
float2 __builtin_spirv_OpenCL_cosh_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_cosh_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_cosh_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_cosh_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_cosh_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_cosh_f64( double x );
double2 __builtin_spirv_OpenCL_cosh_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_cosh_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_cosh_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_cosh_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_cosh_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_cosh_f16( half x );
half2 __builtin_spirv_OpenCL_cosh_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_cosh_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_cosh_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_cosh_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_cosh_v16f16(half16 x);

float  __builtin_spirv_OpenCL_cospi_f32( float x );
float2 __builtin_spirv_OpenCL_cospi_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_cospi_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_cospi_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_cospi_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_cospi_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_cospi_f64( double x );
double2 __builtin_spirv_OpenCL_cospi_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_cospi_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_cospi_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_cospi_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_cospi_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_cospi_f16( half x );
half2 __builtin_spirv_OpenCL_cospi_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_cospi_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_cospi_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_cospi_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_cospi_v16f16(half16 x);

float __builtin_spirv_divide_cr_f32_f32( float a, float b );
float2 __builtin_spirv_divide_cr_v2f32_v2f32( float2 a, float2 b );
float3 __builtin_spirv_divide_cr_v3f32_v3f32( float3 a, float3 b );
float4 __builtin_spirv_divide_cr_v4f32_v4f32( float4 a, float4 b );
float8 __builtin_spirv_divide_cr_v8f32_v8f32( float8 a, float8 b );
float16 __builtin_spirv_divide_cr_v16f32_v16f32( float16 a, float16 b );

#if defined(cl_khr_fp64)
double __builtin_spirv_divide_cr_f64_f64(double a, double b);
#endif // defined(cl_khr_fp64)

float  __builtin_spirv_OpenCL_erf_f32( float x );
float2 __builtin_spirv_OpenCL_erf_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_erf_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_erf_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_erf_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_erf_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_erf_f64( double x );
double2 __builtin_spirv_OpenCL_erf_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_erf_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_erf_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_erf_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_erf_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_erf_f16( half x );
half2 __builtin_spirv_OpenCL_erf_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_erf_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_erf_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_erf_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_erf_v16f16(half16 x);

float __builtin_spirv_OpenCL_erfc_f32( float x );
float2 __builtin_spirv_OpenCL_erfc_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_erfc_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_erfc_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_erfc_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_erfc_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_erfc_f64( double x );
double2 __builtin_spirv_OpenCL_erfc_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_erfc_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_erfc_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_erfc_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_erfc_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_erfc_f16( half x );
half2 __builtin_spirv_OpenCL_erfc_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_erfc_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_erfc_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_erfc_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_erfc_v16f16(half16 x);

float __builtin_spirv_OpenCL_exp_f32(float x);
float2 __builtin_spirv_OpenCL_exp_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_exp_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_exp_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_exp_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_exp_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_exp_f64( double x );
double2 __builtin_spirv_OpenCL_exp_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_exp_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_exp_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_exp_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_exp_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_exp_f16( half x );
half2 __builtin_spirv_OpenCL_exp_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_exp_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_exp_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_exp_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_exp_v16f16(half16 x);

float  __builtin_spirv_OpenCL_exp2_f32( float x );
float2 __builtin_spirv_OpenCL_exp2_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_exp2_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_exp2_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_exp2_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_exp2_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_exp2_f64( double x );
double2 __builtin_spirv_OpenCL_exp2_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_exp2_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_exp2_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_exp2_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_exp2_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_exp2_f16( half x );
half2 __builtin_spirv_OpenCL_exp2_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_exp2_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_exp2_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_exp2_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_exp2_v16f16(half16 x);

float  __builtin_spirv_OpenCL_exp10_f32( float x );
float2 __builtin_spirv_OpenCL_exp10_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_exp10_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_exp10_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_exp10_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_exp10_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_exp10_f64( double x );
double2 __builtin_spirv_OpenCL_exp10_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_exp10_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_exp10_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_exp10_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_exp10_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_exp10_f16( half x );
half2 __builtin_spirv_OpenCL_exp10_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_exp10_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_exp10_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_exp10_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_exp10_v16f16(half16 x);

float  __builtin_spirv_OpenCL_expm1_f32( float a );
float2 __builtin_spirv_OpenCL_expm1_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_expm1_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_expm1_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_expm1_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_expm1_v16f32(float16 x);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_expm1_f64( double x );
double2 __builtin_spirv_OpenCL_expm1_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_expm1_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_expm1_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_expm1_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_expm1_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_expm1_f16( half x );
half2 __builtin_spirv_OpenCL_expm1_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_expm1_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_expm1_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_expm1_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_expm1_v16f16(half16 x);

float __builtin_spirv_OpenCL_fabs_f32(float x );
float2 __builtin_spirv_OpenCL_fabs_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_fabs_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_fabs_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_fabs_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_fabs_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_fabs_f64(double x );
double2 __builtin_spirv_OpenCL_fabs_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_fabs_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_fabs_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_fabs_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_fabs_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_fabs_f16(half x );
half2 __builtin_spirv_OpenCL_fabs_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_fabs_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_fabs_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_fabs_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_fabs_v16f16(half16 x);

float  __builtin_spirv_OpenCL_fdim_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_fdim_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_fdim_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_fdim_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_fdim_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_fdim_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_fdim_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_fdim_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_fdim_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_fdim_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_fdim_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_fdim_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_fdim_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_fdim_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_fdim_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_fdim_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_fdim_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_fdim_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_floor_f32(float x );
float2 __builtin_spirv_OpenCL_floor_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_floor_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_floor_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_floor_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_floor_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_floor_f64(double x );
double2 __builtin_spirv_OpenCL_floor_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_floor_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_floor_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_floor_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_floor_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_floor_f16(half x );
half2 __builtin_spirv_OpenCL_floor_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_floor_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_floor_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_floor_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_floor_v16f16(half16 x);

float  __builtin_spirv_OpenCL_fma_f32_f32_f32( float a, float b, float c );
float2 __builtin_spirv_OpenCL_fma_v2f32_v2f32_v2f32(float2 x, float2 y, float2 z);
float3 __builtin_spirv_OpenCL_fma_v3f32_v3f32_v3f32(float3 x, float3 y, float3 z);
float4 __builtin_spirv_OpenCL_fma_v4f32_v4f32_v4f32(float4 x, float4 y, float4 z);
float8 __builtin_spirv_OpenCL_fma_v8f32_v8f32_v8f32(float8 x, float8 y, float8 z);
float16 __builtin_spirv_OpenCL_fma_v16f32_v16f32_v16f32(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_fma_f64_f64_f64( double a, double b, double c );
double2 __builtin_spirv_OpenCL_fma_v2f64_v2f64_v2f64(double2 x, double2 y, double2 z);
double3 __builtin_spirv_OpenCL_fma_v3f64_v3f64_v3f64(double3 x, double3 y, double3 z);
double4 __builtin_spirv_OpenCL_fma_v4f64_v4f64_v4f64(double4 x, double4 y, double4 z);
double8 __builtin_spirv_OpenCL_fma_v8f64_v8f64_v8f64(double8 x, double8 y, double8 z);
double16 __builtin_spirv_OpenCL_fma_v16f64_v16f64_v16f64(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_fma_f16_f16_f16(half x, half y, half z);
half2 __builtin_spirv_OpenCL_fma_v2f16_v2f16_v2f16(half2 x, half2 y, half2 z);
half3 __builtin_spirv_OpenCL_fma_v3f16_v3f16_v3f16(half3 x, half3 y, half3 z);
half4 __builtin_spirv_OpenCL_fma_v4f16_v4f16_v4f16(half4 x, half4 y, half4 z);
half8 __builtin_spirv_OpenCL_fma_v8f16_v8f16_v8f16(half8 x, half8 y, half8 z);
half16 __builtin_spirv_OpenCL_fma_v16f16_v16f16_v16f16(half16 x, half16 y, half16 z);

float  __builtin_spirv_OpenCL_fmax_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_fmax_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_fmax_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_fmax_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_fmax_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_fmax_v16f32_v16f32(float16 x, float16 y);
float2 __builtin_spirv_OpenCL_fmax_v2f32_f32(float2 x, float y);
float3 __builtin_spirv_OpenCL_fmax_v3f32_f32(float3 x, float y);
float4 __builtin_spirv_OpenCL_fmax_v4f32_f32(float4 x, float y);
float8 __builtin_spirv_OpenCL_fmax_v8f32_f32(float8 x, float y);
float16 __builtin_spirv_OpenCL_fmax_v16f32_f32(float16 x, float y);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_fmax_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_fmax_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_fmax_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_fmax_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_fmax_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_fmax_v16f64_v16f64(double16 x, double16 y);
double2 __builtin_spirv_OpenCL_fmax_v2f64_f64(double2 x, double y);
double3 __builtin_spirv_OpenCL_fmax_v3f64_f64(double3 x, double y);
double4 __builtin_spirv_OpenCL_fmax_v4f64_f64(double4 x, double y);
double8 __builtin_spirv_OpenCL_fmax_v8f64_f64(double8 x, double y);
double16 __builtin_spirv_OpenCL_fmax_v16f64_f64(double16 x, double y);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_fmax_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_fmax_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_fmax_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_fmax_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_fmax_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_fmax_v16f16_v16f16(half16 x, half16 y);
half2 __builtin_spirv_OpenCL_fmax_v2f16_f16(half2 x, half y);
half3 __builtin_spirv_OpenCL_fmax_v3f16_f16(half3 x, half y);
half4 __builtin_spirv_OpenCL_fmax_v4f16_f16(half4 x, half y);
half8 __builtin_spirv_OpenCL_fmax_v8f16_f16(half8 x, half y);
half16 __builtin_spirv_OpenCL_fmax_v16f16_f16(half16 x, half y);

float  __builtin_spirv_OpenCL_fmin_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_fmin_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_fmin_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_fmin_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_fmin_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_fmin_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_fmin_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_fmin_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_fmin_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_fmin_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_fmin_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_fmin_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_fmin_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_fmin_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_fmin_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_fmin_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_fmin_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_fmin_v16f16_v16f16(half16 x, half16 y);

float  __builtin_spirv_OpenCL_fmod_f32_f32( float xx, float yy );
float2 __builtin_spirv_OpenCL_fmod_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_fmod_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_fmod_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_fmod_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_fmod_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double  __builtin_spirv_OpenCL_fmod_f64_f64( double xx, double yy );
double2 __builtin_spirv_OpenCL_fmod_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_fmod_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_fmod_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_fmod_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_fmod_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half  __builtin_spirv_OpenCL_fmod_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_fmod_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_fmod_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_fmod_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_fmod_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_fmod_v16f16_v16f16(half16 x, half16 y);

/* Helper function for fmod */
float  __builtin_spirv_fast_fmod_f32_f32( float xx, float yy );
float2  __builtin_spirv_fast_fmod_v2f32_v2f32( float2 xx, float2 yy );
float3  __builtin_spirv_fast_fmod_v3f32_v3f32( float3 xx, float3 yy );
float4  __builtin_spirv_fast_fmod_v4f32_v4f32( float4 xx, float4 yy );
half  __builtin_spirv_fast_fmod_f16_f16( half xx, half yy );
half2  __builtin_spirv_fast_fmod_v2f16_v2f16( half2 xx, half2 yy );
half3  __builtin_spirv_fast_fmod_v3f16_v3f16( half3 xx, half3 yy );
half4  __builtin_spirv_fast_fmod_v4f16_v4f16( half4 xx, half4 yy );
#if defined(cl_khr_fp64)
double  __builtin_spirv_fast_fmod_f64_f64( double xx, double yy );
double2  __builtin_spirv_fast_fmod_v2f64_v2f64( double2 xx, double2 yy );
double3  __builtin_spirv_fast_fmod_v3f64_v3f64( double3 xx, double3 yy );
double4  __builtin_spirv_fast_fmod_v4f64_v4f64( double4 xx, double4 yy );
#endif // defined(cl_khr_fp64)

float __builtin_spirv_OpenCL_fract_f32_p1f32( float x, __global float* iptr );
float2 __builtin_spirv_OpenCL_fract_v2f32_p1v2f32( float2 x, __global float2* iptr );
float3 __builtin_spirv_OpenCL_fract_v3f32_p1v3f32( float3 x, __global float3* iptr );
float4 __builtin_spirv_OpenCL_fract_v4f32_p1v4f32( float4 x, __global float4* iptr );
float8 __builtin_spirv_OpenCL_fract_v8f32_p1v8f32( float8 x, __global float8* iptr );
float16 __builtin_spirv_OpenCL_fract_v16f32_p1v16f32( float16 x, __global float16* iptr );
float __builtin_spirv_OpenCL_fract_f32_p0f32( float x, __private float* iptr );
float2 __builtin_spirv_OpenCL_fract_v2f32_p0v2f32( float2 x, __private float2* iptr );
float3 __builtin_spirv_OpenCL_fract_v3f32_p0v3f32( float3 x, __private float3* iptr );
float4 __builtin_spirv_OpenCL_fract_v4f32_p0v4f32( float4 x, __private float4* iptr );
float8 __builtin_spirv_OpenCL_fract_v8f32_p0v8f32( float8 x, __private float8* iptr );
float16 __builtin_spirv_OpenCL_fract_v16f32_p0v16f32( float16 x, __private float16* iptr );
float __builtin_spirv_OpenCL_fract_f32_p3f32( float x, __local float* iptr );
float2 __builtin_spirv_OpenCL_fract_v2f32_p3v2f32( float2 x, __local float2* iptr );
float3 __builtin_spirv_OpenCL_fract_v3f32_p3v3f32( float3 x, __local float3* iptr );
float4 __builtin_spirv_OpenCL_fract_v4f32_p3v4f32( float4 x, __local float4* iptr );
float8 __builtin_spirv_OpenCL_fract_v8f32_p3v8f32( float8 x, __local float8* iptr );
float16 __builtin_spirv_OpenCL_fract_v16f32_p3v16f32( float16 x, __local float16* iptr );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __builtin_spirv_OpenCL_fract_f32_p4f32( float x, __generic float* iptr );
float2 __builtin_spirv_OpenCL_fract_v2f32_p4v2f32( float2 x, __generic float2* iptr );
float3 __builtin_spirv_OpenCL_fract_v3f32_p4v3f32( float3 x, __generic float3* iptr );
float4 __builtin_spirv_OpenCL_fract_v4f32_p4v4f32( float4 x, __generic float4* iptr );
float8 __builtin_spirv_OpenCL_fract_v8f32_p4v8f32( float8 x, __generic float8* iptr );
float16 __builtin_spirv_OpenCL_fract_v16f32_p4v16f32( float16 x, __generic float16* iptr );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half __builtin_spirv_OpenCL_fract_f16_p1f16( half x, __global half* iptr );
half2 __builtin_spirv_OpenCL_fract_v2f16_p1v2f16( half2 x, __global half2* iptr );
half3 __builtin_spirv_OpenCL_fract_v3f16_p1v3f16( half3 x, __global half3* iptr );
half4 __builtin_spirv_OpenCL_fract_v4f16_p1v4f16( half4 x, __global half4* iptr );
half8 __builtin_spirv_OpenCL_fract_v8f16_p1v8f16( half8 x, __global half8* iptr );
half16 __builtin_spirv_OpenCL_fract_v16f16_p1v16f16( half16 x, __global half16* iptr );
half __builtin_spirv_OpenCL_fract_f16_p0f16( half x, __private half* iptr );
half2 __builtin_spirv_OpenCL_fract_v2f16_p0v2f16( half2 x, __private half2* iptr );
half3 __builtin_spirv_OpenCL_fract_v3f16_p0v3f16( half3 x, __private half3* iptr );
half4 __builtin_spirv_OpenCL_fract_v4f16_p0v4f16( half4 x, __private half4* iptr );
half8 __builtin_spirv_OpenCL_fract_v8f16_p0v8f16( half8 x, __private half8* iptr );
half16 __builtin_spirv_OpenCL_fract_v16f16_p0v16f16( half16 x, __private half16* iptr );
half __builtin_spirv_OpenCL_fract_f16_p3f16( half x, __local half* iptr );
half2 __builtin_spirv_OpenCL_fract_v2f16_p3v2f16( half2 x, __local half2* iptr );
half3 __builtin_spirv_OpenCL_fract_v3f16_p3v3f16( half3 x, __local half3* iptr );
half4 __builtin_spirv_OpenCL_fract_v4f16_p3v4f16( half4 x, __local half4* iptr );
half8 __builtin_spirv_OpenCL_fract_v8f16_p3v8f16( half8 x, __local half8* iptr );
half16 __builtin_spirv_OpenCL_fract_v16f16_p3v16f16( half16 x, __local half16* iptr );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __builtin_spirv_OpenCL_fract_f16_p4f16( half x, __generic half* iptr );
half2 __builtin_spirv_OpenCL_fract_v2f16_p4v2f16( half2 x, __generic half2* iptr );
half3 __builtin_spirv_OpenCL_fract_v3f16_p4v3f16( half3 x, __generic half3* iptr );
half4 __builtin_spirv_OpenCL_fract_v4f16_p4v4f16( half4 x, __generic half4* iptr );
half8 __builtin_spirv_OpenCL_fract_v8f16_p4v8f16( half8 x, __generic half8* iptr );
half16 __builtin_spirv_OpenCL_fract_v16f16_p4v16f16( half16 x, __generic half16* iptr );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_fract_f64_p1f64( double x, __global double* iptr );
double2 __builtin_spirv_OpenCL_fract_v2f64_p1v2f64( double2 x, __global double2* iptr );
double3 __builtin_spirv_OpenCL_fract_v3f64_p1v3f64( double3 x, __global double3* iptr );
double4 __builtin_spirv_OpenCL_fract_v4f64_p1v4f64( double4 x, __global double4* iptr );
double8 __builtin_spirv_OpenCL_fract_v8f64_p1v8f64( double8 x, __global double8* iptr );
double16 __builtin_spirv_OpenCL_fract_v16f64_p1v16f64( double16 x, __global double16* iptr );
double __builtin_spirv_OpenCL_fract_f64_p0f64( double x, __private double* iptr );
double2 __builtin_spirv_OpenCL_fract_v2f64_p0v2f64( double2 x, __private double2* iptr );
double3 __builtin_spirv_OpenCL_fract_v3f64_p0v3f64( double3 x, __private double3* iptr );
double4 __builtin_spirv_OpenCL_fract_v4f64_p0v4f64( double4 x, __private double4* iptr );
double8 __builtin_spirv_OpenCL_fract_v8f64_p0v8f64( double8 x, __private double8* iptr );
double16 __builtin_spirv_OpenCL_fract_v16f64_p0v16f64( double16 x, __private double16* iptr );
double __builtin_spirv_OpenCL_fract_f64_p3f64( double x, __local double* iptr );
double2 __builtin_spirv_OpenCL_fract_v2f64_p3v2f64( double2 x, __local double2* iptr );
double3 __builtin_spirv_OpenCL_fract_v3f64_p3v3f64( double3 x, __local double3* iptr );
double4 __builtin_spirv_OpenCL_fract_v4f64_p3v4f64( double4 x, __local double4* iptr );
double8 __builtin_spirv_OpenCL_fract_v8f64_p3v8f64( double8 x, __local double8* iptr );
double16 __builtin_spirv_OpenCL_fract_v16f64_p3v16f64( double16 x, __local double16* iptr );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __builtin_spirv_OpenCL_fract_f64_p4f64( double x, __generic double* iptr );
double2 __builtin_spirv_OpenCL_fract_v2f64_p4v2f64( double2 x, __generic double2* iptr );
double3 __builtin_spirv_OpenCL_fract_v3f64_p4v3f64( double3 x, __generic double3* iptr );
double4 __builtin_spirv_OpenCL_fract_v4f64_p4v4f64( double4 x, __generic double4* iptr );
double8 __builtin_spirv_OpenCL_fract_v8f64_p4v8f64( double8 x, __generic double8* iptr );
double16 __builtin_spirv_OpenCL_fract_v16f64_p4v16f64( double16 x, __generic double16* iptr );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float __builtin_spirv_OpenCL_frexp_f32_p1i32( float x, __global int* exp );
float2 __builtin_spirv_OpenCL_frexp_v2f32_p1v2i32( float2 x, __global int2* exp );
float3 __builtin_spirv_OpenCL_frexp_v3f32_p1v3i32( float3 x, __global int3* exp );
float4 __builtin_spirv_OpenCL_frexp_v4f32_p1v4i32( float4 x, __global int4* exp );
float8 __builtin_spirv_OpenCL_frexp_v8f32_p1v8i32( float8 x, __global int8* exp );
float16 __builtin_spirv_OpenCL_frexp_v16f32_p1v16i32( float16 x, __global int16* exp );
float __builtin_spirv_OpenCL_frexp_f32_p0i32( float x, __private int* exp );
float2 __builtin_spirv_OpenCL_frexp_v2f32_p0v2i32( float2 x, __private int2* exp );
float3 __builtin_spirv_OpenCL_frexp_v3f32_p0v3i32( float3 x, __private int3* exp );
float4 __builtin_spirv_OpenCL_frexp_v4f32_p0v4i32( float4 x, __private int4* exp );
float8 __builtin_spirv_OpenCL_frexp_v8f32_p0v8i32( float8 x, __private int8* exp );
float16 __builtin_spirv_OpenCL_frexp_v16f32_p0v16i32( float16 x, __private int16* exp );
float __builtin_spirv_OpenCL_frexp_f32_p3i32( float x, __local int* exp );
float2 __builtin_spirv_OpenCL_frexp_v2f32_p3v2i32( float2 x, __local int2* exp );
float3 __builtin_spirv_OpenCL_frexp_v3f32_p3v3i32( float3 x, __local int3* exp );
float4 __builtin_spirv_OpenCL_frexp_v4f32_p3v4i32( float4 x, __local int4* exp );
float8 __builtin_spirv_OpenCL_frexp_v8f32_p3v8i32( float8 x, __local int8* exp );
float16 __builtin_spirv_OpenCL_frexp_v16f32_p3v16i32( float16 x, __local int16* exp );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __builtin_spirv_OpenCL_frexp_f32_p4i32( float x, __generic int* exp );
float2 __builtin_spirv_OpenCL_frexp_v2f32_p4v2i32( float2 x, __generic int2* exp );
float3 __builtin_spirv_OpenCL_frexp_v3f32_p4v3i32( float3 x, __generic int3* exp );
float4 __builtin_spirv_OpenCL_frexp_v4f32_p4v4i32( float4 x, __generic int4* exp );
float8 __builtin_spirv_OpenCL_frexp_v8f32_p4v8i32( float8 x, __generic int8* exp );
float16 __builtin_spirv_OpenCL_frexp_v16f32_p4v16i32( float16 x, __generic int16* exp );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half __builtin_spirv_OpenCL_frexp_f16_p1i32( half x, __global int* exp );
half2 __builtin_spirv_OpenCL_frexp_v2f16_p1v2i32( half2 x, __global int2* exp );
half3 __builtin_spirv_OpenCL_frexp_v3f16_p1v3i32( half3 x, __global int3* exp );
half4 __builtin_spirv_OpenCL_frexp_v4f16_p1v4i32( half4 x, __global int4* exp );
half8 __builtin_spirv_OpenCL_frexp_v8f16_p1v8i32( half8 x, __global int8* exp );
half16 __builtin_spirv_OpenCL_frexp_v16f16_p1v16i32( half16 x, __global int16* exp );
half __builtin_spirv_OpenCL_frexp_f16_p0i32( half x, __private int* exp );
half2 __builtin_spirv_OpenCL_frexp_v2f16_p0v2i32( half2 x, __private int2* exp );
half3 __builtin_spirv_OpenCL_frexp_v3f16_p0v3i32( half3 x, __private int3* exp );
half4 __builtin_spirv_OpenCL_frexp_v4f16_p0v4i32( half4 x, __private int4* exp );
half8 __builtin_spirv_OpenCL_frexp_v8f16_p0v8i32( half8 x, __private int8* exp );
half16 __builtin_spirv_OpenCL_frexp_v16f16_p0v16i32( half16 x, __private int16* exp );
half __builtin_spirv_OpenCL_frexp_f16_p3i32( half x, __local int* exp );
half2 __builtin_spirv_OpenCL_frexp_v2f16_p3v2i32( half2 x, __local int2* exp );
half3 __builtin_spirv_OpenCL_frexp_v3f16_p3v3i32( half3 x, __local int3* exp );
half4 __builtin_spirv_OpenCL_frexp_v4f16_p3v4i32( half4 x, __local int4* exp );
half8 __builtin_spirv_OpenCL_frexp_v8f16_p3v8i32( half8 x, __local int8* exp );
half16 __builtin_spirv_OpenCL_frexp_v16f16_p3v16i32( half16 x, __local int16* exp );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __builtin_spirv_OpenCL_frexp_f16_p4i32( half x, __generic int* exp );
half2 __builtin_spirv_OpenCL_frexp_v2f16_p4v2i32( half2 x, __generic int2* exp );
half3 __builtin_spirv_OpenCL_frexp_v3f16_p4v3i32( half3 x, __generic int3* exp );
half4 __builtin_spirv_OpenCL_frexp_v4f16_p4v4i32( half4 x, __generic int4* exp );
half8 __builtin_spirv_OpenCL_frexp_v8f16_p4v8i32( half8 x, __generic int8* exp );
half16 __builtin_spirv_OpenCL_frexp_v16f16_p4v16i32( half16 x, __generic int16* exp );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_frexp_f64_p1i32( double x, __global int* exp );
double2 __builtin_spirv_OpenCL_frexp_v2f64_p1v2i32( double2 x, __global int2* exp );
double3 __builtin_spirv_OpenCL_frexp_v3f64_p1v3i32( double3 x, __global int3* exp );
double4 __builtin_spirv_OpenCL_frexp_v4f64_p1v4i32( double4 x, __global int4* exp );
double8 __builtin_spirv_OpenCL_frexp_v8f64_p1v8i32( double8 x, __global int8* exp );
double16 __builtin_spirv_OpenCL_frexp_v16f64_p1v16i32( double16 x, __global int16* exp );
double __builtin_spirv_OpenCL_frexp_f64_p0i32( double x, __private int* exp );
double2 __builtin_spirv_OpenCL_frexp_v2f64_p0v2i32( double2 x, __private int2* exp );
double3 __builtin_spirv_OpenCL_frexp_v3f64_p0v3i32( double3 x, __private int3* exp );
double4 __builtin_spirv_OpenCL_frexp_v4f64_p0v4i32( double4 x, __private int4* exp );
double8 __builtin_spirv_OpenCL_frexp_v8f64_p0v8i32( double8 x, __private int8* exp );
double16 __builtin_spirv_OpenCL_frexp_v16f64_p0v16i32( double16 x, __private int16* exp );
double __builtin_spirv_OpenCL_frexp_f64_p3i32( double x, __local int* exp );
double2 __builtin_spirv_OpenCL_frexp_v2f64_p3v2i32( double2 x, __local int2* exp );
double3 __builtin_spirv_OpenCL_frexp_v3f64_p3v3i32( double3 x, __local int3* exp );
double4 __builtin_spirv_OpenCL_frexp_v4f64_p3v4i32( double4 x, __local int4* exp );
double8 __builtin_spirv_OpenCL_frexp_v8f64_p3v8i32( double8 x, __local int8* exp );
double16 __builtin_spirv_OpenCL_frexp_v16f64_p3v16i32( double16 x, __local int16* exp );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __builtin_spirv_OpenCL_frexp_f64_p4i32( double x, __generic int* exp );
double2 __builtin_spirv_OpenCL_frexp_v2f64_p4v2i32( double2 x, __generic int2* exp );
double3 __builtin_spirv_OpenCL_frexp_v3f64_p4v3i32( double3 x, __generic int3* exp );
double4 __builtin_spirv_OpenCL_frexp_v4f64_p4v4i32( double4 x, __generic int4* exp );
double8 __builtin_spirv_OpenCL_frexp_v8f64_p4v8i32( double8 x, __generic int8* exp );
double16 __builtin_spirv_OpenCL_frexp_v16f64_p4v16i32( double16 x, __generic int16* exp );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float __builtin_spirv_OpenCL_hypot_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_hypot_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_hypot_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_hypot_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_hypot_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_hypot_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_hypot_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_hypot_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_hypot_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_hypot_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_hypot_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_hypot_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_hypot_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_hypot_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_hypot_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_hypot_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_hypot_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_hypot_v16f16_v16f16(half16 x, half16 y);

int __builtin_spirv_OpenCL_ilogb_f32( float x );
int __builtin_spirv_OpenCL_ilogb_f32( float x );
int2 __builtin_spirv_OpenCL_ilogb_v2f32(float2 x);
int3 __builtin_spirv_OpenCL_ilogb_v3f32(float3 x);
int4 __builtin_spirv_OpenCL_ilogb_v4f32(float4 x);
int8 __builtin_spirv_OpenCL_ilogb_v8f32(float8 x);
int16 __builtin_spirv_OpenCL_ilogb_v16f32(float16 x);
#if defined(cl_khr_fp64)
int __builtin_spirv_OpenCL_ilogb_f64( double x );
int2 __builtin_spirv_OpenCL_ilogb_v2f64(double2 x);
int3 __builtin_spirv_OpenCL_ilogb_v3f64(double3 x);
int4 __builtin_spirv_OpenCL_ilogb_v4f64(double4 x);
int8 __builtin_spirv_OpenCL_ilogb_v8f64(double8 x);
int16 __builtin_spirv_OpenCL_ilogb_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
int __builtin_spirv_OpenCL_ilogb_f16( half x );
int2 __builtin_spirv_OpenCL_ilogb_v2f16(half2 x);
int3 __builtin_spirv_OpenCL_ilogb_v3f16(half3 x);
int4 __builtin_spirv_OpenCL_ilogb_v4f16(half4 x);
int8 __builtin_spirv_OpenCL_ilogb_v8f16(half8 x);
int16 __builtin_spirv_OpenCL_ilogb_v16f16(half16 x);

float __builtin_spirv_OpenCL_ldexp_f32_i32( float x, int n );
float2 __builtin_spirv_OpenCL_ldexp_v2f32_v2i32(float2 x, int2 y);
float3 __builtin_spirv_OpenCL_ldexp_v3f32_v3i32(float3 x, int3 y);
float4 __builtin_spirv_OpenCL_ldexp_v4f32_v4i32(float4 x, int4 y);
float8 __builtin_spirv_OpenCL_ldexp_v8f32_v8i32(float8 x, int8 y);
float16 __builtin_spirv_OpenCL_ldexp_v16f32_v16i32(float16 x, int16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_ldexp_f64_i32( double x, int n );
double2 __builtin_spirv_OpenCL_ldexp_v2f64_v2i32(double2 x, int2 y);
double3 __builtin_spirv_OpenCL_ldexp_v3f64_v3i32(double3 x, int3 y);
double4 __builtin_spirv_OpenCL_ldexp_v4f64_v4i32(double4 x, int4 y);
double8 __builtin_spirv_OpenCL_ldexp_v8f64_v8i32(double8 x, int8 y);
double16 __builtin_spirv_OpenCL_ldexp_v16f64_v16i32(double16 x, int16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_ldexp_f16_i32( half x, int n );
half2 __builtin_spirv_OpenCL_ldexp_v2f16_v2i32(half2 x, int2 y);
half3 __builtin_spirv_OpenCL_ldexp_v3f16_v3i32(half3 x, int3 y);
half4 __builtin_spirv_OpenCL_ldexp_v4f16_v4i32(half4 x, int4 y);
half8 __builtin_spirv_OpenCL_ldexp_v8f16_v8i32(half8 x, int8 y);
half16 __builtin_spirv_OpenCL_ldexp_v16f16_v16i32(half16 x, int16 y);

float __builtin_spirv_OpenCL_lgamma_f32( float x );
float2 __builtin_spirv_OpenCL_lgamma_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_lgamma_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_lgamma_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_lgamma_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_lgamma_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_lgamma_f64( double x );
double2 __builtin_spirv_OpenCL_lgamma_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_lgamma_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_lgamma_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_lgamma_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_lgamma_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_lgamma_f16( half x );
half2 __builtin_spirv_OpenCL_lgamma_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_lgamma_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_lgamma_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_lgamma_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_lgamma_v16f16(half16 x);

float __builtin_spirv_OpenCL_lgamma_r_f32_p1i32( float x, __global int* signp );
float2 __builtin_spirv_OpenCL_lgamma_r_v2f32_p1v2i32( float2 x, __global int2* signp );
float3 __builtin_spirv_OpenCL_lgamma_r_v3f32_p1v3i32( float3 x, __global int3* signp );
float4 __builtin_spirv_OpenCL_lgamma_r_v4f32_p1v4i32( float4 x, __global int4* signp );
float8 __builtin_spirv_OpenCL_lgamma_r_v8f32_p1v8i32( float8 x, __global int8* signp );
float16 __builtin_spirv_OpenCL_lgamma_r_v16f32_p1v16i32( float16 x, __global int16* signp );
float __builtin_spirv_OpenCL_lgamma_r_f32_p3i32( float x, __local int* signp );
float2 __builtin_spirv_OpenCL_lgamma_r_v2f32_p3v2i32( float2 x, __local int2* signp );
float3 __builtin_spirv_OpenCL_lgamma_r_v3f32_p3v3i32( float3 x, __local int3* signp );
float4 __builtin_spirv_OpenCL_lgamma_r_v4f32_p3v4i32( float4 x, __local int4* signp );
float8 __builtin_spirv_OpenCL_lgamma_r_v8f32_p3v8i32( float8 x, __local int8* signp );
float16 __builtin_spirv_OpenCL_lgamma_r_v16f32_p3v16i32( float16 x, __local int16* signp );
float __builtin_spirv_OpenCL_lgamma_r_f32_p0i32( float x, __private int* signp );
float2 __builtin_spirv_OpenCL_lgamma_r_v2f32_p0v2i32( float2 x, __private int2* signp );
float3 __builtin_spirv_OpenCL_lgamma_r_v3f32_p0v3i32( float3 x, __private int3* signp );
float4 __builtin_spirv_OpenCL_lgamma_r_v4f32_p0v4i32( float4 x, __private int4* signp );
float8 __builtin_spirv_OpenCL_lgamma_r_v8f32_p0v8i32( float8 x, __private int8* signp );
float16 __builtin_spirv_OpenCL_lgamma_r_v16f32_p0v16i32( float16 x, __private int16* signp );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __builtin_spirv_OpenCL_lgamma_r_f32_p4i32( float x, __generic int* signp );
float2 __builtin_spirv_OpenCL_lgamma_r_v2f32_p4v2i32( float2 x, __generic int2* signp );
float3 __builtin_spirv_OpenCL_lgamma_r_v3f32_p4v3i32( float3 x, __generic int3* signp );
float4 __builtin_spirv_OpenCL_lgamma_r_v4f32_p4v4i32( float4 x, __generic int4* signp );
float8 __builtin_spirv_OpenCL_lgamma_r_v8f32_p4v8i32( float8 x, __generic int8* signp );
float16 __builtin_spirv_OpenCL_lgamma_r_v16f32_p4v16i32( float16 x, __generic int16* signp );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half __builtin_spirv_OpenCL_lgamma_r_f16_p1i32( half x, __global int* signp );
half2 __builtin_spirv_OpenCL_lgamma_r_v2f16_p1v2i32( half2 x, __global int2* signp );
half3 __builtin_spirv_OpenCL_lgamma_r_v3f16_p1v3i32( half3 x, __global int3* signp );
half4 __builtin_spirv_OpenCL_lgamma_r_v4f16_p1v4i32( half4 x, __global int4* signp );
half8 __builtin_spirv_OpenCL_lgamma_r_v8f16_p1v8i32( half8 x, __global int8* signp );
half16 __builtin_spirv_OpenCL_lgamma_r_v16f16_p1v16i32( half16 x, __global int16* signp );
half __builtin_spirv_OpenCL_lgamma_r_f16_p0i32( half x, __private int* signp );
half2 __builtin_spirv_OpenCL_lgamma_r_v2f16_p0v2i32( half2 x, __private int2* signp );
half3 __builtin_spirv_OpenCL_lgamma_r_v3f16_p0v3i32( half3 x, __private int3* signp );
half4 __builtin_spirv_OpenCL_lgamma_r_v4f16_p0v4i32( half4 x, __private int4* signp );
half8 __builtin_spirv_OpenCL_lgamma_r_v8f16_p0v8i32( half8 x, __private int8* signp );
half16 __builtin_spirv_OpenCL_lgamma_r_v16f16_p0v16i32( half16 x, __private int16* signp );
half __builtin_spirv_OpenCL_lgamma_r_f16_p3i32( half x, __local int* signp );
half2 __builtin_spirv_OpenCL_lgamma_r_v2f16_p3v2i32( half2 x, __local int2* signp );
half3 __builtin_spirv_OpenCL_lgamma_r_v3f16_p3v3i32( half3 x, __local int3* signp );
half4 __builtin_spirv_OpenCL_lgamma_r_v4f16_p3v4i32( half4 x, __local int4* signp );
half8 __builtin_spirv_OpenCL_lgamma_r_v8f16_p3v8i32( half8 x, __local int8* signp );
half16 __builtin_spirv_OpenCL_lgamma_r_v16f16_p3v16i32( half16 x, __local int16* signp );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __builtin_spirv_OpenCL_lgamma_r_f16_p4i32( half x, __generic int* signp );
half2 __builtin_spirv_OpenCL_lgamma_r_v2f16_p4v2i32( half2 x, __generic int2* signp );
half3 __builtin_spirv_OpenCL_lgamma_r_v3f16_p4v3i32( half3 x, __generic int3* signp );
half4 __builtin_spirv_OpenCL_lgamma_r_v4f16_p4v4i32( half4 x, __generic int4* signp );
half8 __builtin_spirv_OpenCL_lgamma_r_v8f16_p4v8i32( half8 x, __generic int8* signp );
half16 __builtin_spirv_OpenCL_lgamma_r_v16f16_p4v16i32( half16 x, __generic int16* signp );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_lgamma_r_f64_p1i32( double x, __global int* signp );
double2 __builtin_spirv_OpenCL_lgamma_r_v2f64_p1v2i32( double2 x, __global int2* signp );
double3 __builtin_spirv_OpenCL_lgamma_r_v3f64_p1v3i32( double3 x, __global int3* signp );
double4 __builtin_spirv_OpenCL_lgamma_r_v4f64_p1v4i32( double4 x, __global int4* signp );
double8 __builtin_spirv_OpenCL_lgamma_r_v8f64_p1v8i32( double8 x, __global int8* signp );
double16 __builtin_spirv_OpenCL_lgamma_r_v16f64_p1v16i32( double16 x, __global int16* signp );
double __builtin_spirv_OpenCL_lgamma_r_f64_p0i32( double x, __private int* signp );
double2 __builtin_spirv_OpenCL_lgamma_r_v2f64_p0v2i32( double2 x, __private int2* signp );
double3 __builtin_spirv_OpenCL_lgamma_r_v3f64_p0v3i32( double3 x, __private int3* signp );
double4 __builtin_spirv_OpenCL_lgamma_r_v4f64_p0v4i32( double4 x, __private int4* signp );
double8 __builtin_spirv_OpenCL_lgamma_r_v8f64_p0v8i32( double8 x, __private int8* signp );
double16 __builtin_spirv_OpenCL_lgamma_r_v16f64_p0v16i32( double16 x, __private int16* signp );
double __builtin_spirv_OpenCL_lgamma_r_f64_p3i32( double x, __local int* signp );
double2 __builtin_spirv_OpenCL_lgamma_r_v2f64_p3v2i32( double2 x, __local int2* signp );
double3 __builtin_spirv_OpenCL_lgamma_r_v3f64_p3v3i32( double3 x, __local int3* signp );
double4 __builtin_spirv_OpenCL_lgamma_r_v4f64_p3v4i32( double4 x, __local int4* signp );
double8 __builtin_spirv_OpenCL_lgamma_r_v8f64_p3v8i32( double8 x, __local int8* signp );
double16 __builtin_spirv_OpenCL_lgamma_r_v16f64_p3v16i32( double16 x, __local int16* signp );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __builtin_spirv_OpenCL_lgamma_r_f64_p4i32( double x, __generic int* signp );
double2 __builtin_spirv_OpenCL_lgamma_r_v2f64_p4v2i32( double2 x, __generic int2* signp );
double3 __builtin_spirv_OpenCL_lgamma_r_v3f64_p4v3i32( double3 x, __generic int3* signp );
double4 __builtin_spirv_OpenCL_lgamma_r_v4f64_p4v4i32( double4 x, __generic int4* signp );
double8 __builtin_spirv_OpenCL_lgamma_r_v8f64_p4v8i32( double8 x, __generic int8* signp );
double16 __builtin_spirv_OpenCL_lgamma_r_v16f64_p4v16i32( double16 x, __generic int16* signp );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float __builtin_spirv_OpenCL_log_f32( float x );
float2 __builtin_spirv_OpenCL_log_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_log_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_log_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_log_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_log_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_log_f64( double x );
double2 __builtin_spirv_OpenCL_log_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_log_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_log_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_log_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_log_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_log_f16( half x );
half2 __builtin_spirv_OpenCL_log_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_log_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_log_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_log_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_log_v16f16(half16 x);

float __builtin_spirv_OpenCL_log1p_f32( float a );
float2 __builtin_spirv_OpenCL_log1p_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_log1p_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_log1p_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_log1p_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_log1p_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_log1p_f64( double a );
double2 __builtin_spirv_OpenCL_log1p_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_log1p_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_log1p_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_log1p_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_log1p_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_log1p_f16( half x );
half2 __builtin_spirv_OpenCL_log1p_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_log1p_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_log1p_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_log1p_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_log1p_v16f16(half16 x);

float __builtin_spirv_OpenCL_log2_f32( float x );
float2 __builtin_spirv_OpenCL_log2_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_log2_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_log2_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_log2_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_log2_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_log2_f64( double x );
double2 __builtin_spirv_OpenCL_log2_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_log2_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_log2_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_log2_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_log2_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_log2_f16( half x );
half2 __builtin_spirv_OpenCL_log2_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_log2_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_log2_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_log2_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_log2_v16f16(half16 x);

float __builtin_spirv_OpenCL_log10_f32( float x );
float2 __builtin_spirv_OpenCL_log10_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_log10_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_log10_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_log10_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_log10_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_log10_f64( double x );
double2 __builtin_spirv_OpenCL_log10_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_log10_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_log10_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_log10_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_log10_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_log10_f16( half x );
half2 __builtin_spirv_OpenCL_log10_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_log10_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_log10_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_log10_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_log10_v16f16(half16 x);

float __builtin_spirv_OpenCL_logb_f32( float x );
float2 __builtin_spirv_OpenCL_logb_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_logb_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_logb_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_logb_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_logb_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_logb_f64( double x );
double2 __builtin_spirv_OpenCL_logb_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_logb_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_logb_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_logb_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_logb_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_logb_f16( half x );
half2 __builtin_spirv_OpenCL_logb_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_logb_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_logb_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_logb_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_logb_v16f16(half16 x);

float __builtin_spirv_OpenCL_mad_f32_f32_f32( float a, float b, float c );
float2 __builtin_spirv_OpenCL_mad_v2f32_v2f32_v2f32(float2 x, float2 y, float2 z);
float3 __builtin_spirv_OpenCL_mad_v3f32_v3f32_v3f32(float3 x, float3 y, float3 z);
float4 __builtin_spirv_OpenCL_mad_v4f32_v4f32_v4f32(float4 x, float4 y, float4 z);
float8 __builtin_spirv_OpenCL_mad_v8f32_v8f32_v8f32(float8 x, float8 y, float8 z);
float16 __builtin_spirv_OpenCL_mad_v16f32_v16f32_v16f32(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_mad_f64_f64_f64( double a, double b, double c );
double2 __builtin_spirv_OpenCL_mad_v2f64_v2f64_v2f64(double2 x, double2 y, double2 z);
double3 __builtin_spirv_OpenCL_mad_v3f64_v3f64_v3f64(double3 x, double3 y, double3 z);
double4 __builtin_spirv_OpenCL_mad_v4f64_v4f64_v4f64(double4 x, double4 y, double4 z);
double8 __builtin_spirv_OpenCL_mad_v8f64_v8f64_v8f64(double8 x, double8 y, double8 z);
double16 __builtin_spirv_OpenCL_mad_v16f64_v16f64_v16f64(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_mad_f16_f16_f16(half x, half y, half z);
half2 __builtin_spirv_OpenCL_mad_v2f16_v2f16_v2f16(half2 x, half2 y, half2 z);
half3 __builtin_spirv_OpenCL_mad_v3f16_v3f16_v3f16(half3 x, half3 y, half3 z);
half4 __builtin_spirv_OpenCL_mad_v4f16_v4f16_v4f16(half4 x, half4 y, half4 z);
half8 __builtin_spirv_OpenCL_mad_v8f16_v8f16_v8f16(half8 x, half8 y, half8 z);
half16 __builtin_spirv_OpenCL_mad_v16f16_v16f16_v16f16(half16 x, half16 y, half16 z);

float __builtin_spirv_OpenCL_maxmag_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_maxmag_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_maxmag_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_maxmag_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_maxmag_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_maxmag_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_maxmag_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_maxmag_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_maxmag_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_maxmag_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_maxmag_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_maxmag_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_maxmag_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_maxmag_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_maxmag_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_maxmag_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_maxmag_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_maxmag_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_minmag_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_minmag_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_minmag_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_minmag_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_minmag_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_minmag_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_minmag_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_minmag_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_minmag_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_minmag_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_minmag_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_minmag_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_minmag_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_minmag_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_minmag_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_minmag_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_minmag_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_minmag_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_modf_f32_p1f32( float x, __global float* iptr );
float2 __builtin_spirv_OpenCL_modf_v2f32_p1v2f32( float2 x, __global float2* iptr );
float3 __builtin_spirv_OpenCL_modf_v3f32_p1v3f32( float3 x, __global float3* iptr );
float4 __builtin_spirv_OpenCL_modf_v4f32_p1v4f32( float4 x, __global float4* iptr );
float8 __builtin_spirv_OpenCL_modf_v8f32_p1v8f32( float8 x, __global float8* iptr );
float16 __builtin_spirv_OpenCL_modf_v16f32_p1v16f32( float16 x, __global float16* iptr );
float __builtin_spirv_OpenCL_modf_f32_p0f32( float x, __private float* iptr );
float2 __builtin_spirv_OpenCL_modf_v2f32_p0v2f32( float2 x, __private float2* iptr );
float3 __builtin_spirv_OpenCL_modf_v3f32_p0v3f32( float3 x, __private float3* iptr );
float4 __builtin_spirv_OpenCL_modf_v4f32_p0v4f32( float4 x, __private float4* iptr );
float8 __builtin_spirv_OpenCL_modf_v8f32_p0v8f32( float8 x, __private float8* iptr );
float16 __builtin_spirv_OpenCL_modf_v16f32_p0v16f32( float16 x, __private float16* iptr );
float __builtin_spirv_OpenCL_modf_f32_p3f32( float x, __local float* iptr );
float2 __builtin_spirv_OpenCL_modf_v2f32_p3v2f32( float2 x, __local float2* iptr );
float3 __builtin_spirv_OpenCL_modf_v3f32_p3v3f32( float3 x, __local float3* iptr );
float4 __builtin_spirv_OpenCL_modf_v4f32_p3v4f32( float4 x, __local float4* iptr );
float8 __builtin_spirv_OpenCL_modf_v8f32_p3v8f32( float8 x, __local float8* iptr );
float16 __builtin_spirv_OpenCL_modf_v16f32_p3v16f32( float16 x, __local float16* iptr );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __builtin_spirv_OpenCL_modf_f32_p4f32( float x, __generic float* iptr );
float2 __builtin_spirv_OpenCL_modf_v2f32_p4v2f32( float2 x, __generic float2* iptr );
float3 __builtin_spirv_OpenCL_modf_v3f32_p4v3f32( float3 x, __generic float3* iptr );
float4 __builtin_spirv_OpenCL_modf_v4f32_p4v4f32( float4 x, __generic float4* iptr );
float8 __builtin_spirv_OpenCL_modf_v8f32_p4v8f32( float8 x, __generic float8* iptr );
float16 __builtin_spirv_OpenCL_modf_v16f32_p4v16f32( float16 x, __generic float16* iptr );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half __builtin_spirv_OpenCL_modf_f16_p1f16( half x, __global half* iptr );
half2 __builtin_spirv_OpenCL_modf_v2f16_p1v2f16( half2 x, __global half2* iptr );
half3 __builtin_spirv_OpenCL_modf_v3f16_p1v3f16( half3 x, __global half3* iptr );
half4 __builtin_spirv_OpenCL_modf_v4f16_p1v4f16( half4 x, __global half4* iptr );
half8 __builtin_spirv_OpenCL_modf_v8f16_p1v8f16( half8 x, __global half8* iptr );
half16 __builtin_spirv_OpenCL_modf_v16f16_p1v16f16( half16 x, __global half16* iptr );
half __builtin_spirv_OpenCL_modf_f16_p0f16( half x, __private half* iptr );
half2 __builtin_spirv_OpenCL_modf_v2f16_p0v2f16( half2 x, __private half2* iptr );
half3 __builtin_spirv_OpenCL_modf_v3f16_p0v3f16( half3 x, __private half3* iptr );
half4 __builtin_spirv_OpenCL_modf_v4f16_p0v4f16( half4 x, __private half4* iptr );
half8 __builtin_spirv_OpenCL_modf_v8f16_p0v8f16( half8 x, __private half8* iptr );
half16 __builtin_spirv_OpenCL_modf_v16f16_p0v16f16( half16 x, __private half16* iptr );
half __builtin_spirv_OpenCL_modf_f16_p3f16( half x, __local half* iptr );
half2 __builtin_spirv_OpenCL_modf_v2f16_p3v2f16( half2 x, __local half2* iptr );
half3 __builtin_spirv_OpenCL_modf_v3f16_p3v3f16( half3 x, __local half3* iptr );
half4 __builtin_spirv_OpenCL_modf_v4f16_p3v4f16( half4 x, __local half4* iptr );
half8 __builtin_spirv_OpenCL_modf_v8f16_p3v8f16( half8 x, __local half8* iptr );
half16 __builtin_spirv_OpenCL_modf_v16f16_p3v16f16( half16 x, __local half16* iptr );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __builtin_spirv_OpenCL_modf_f16_p4f16( half x, __generic half* iptr );
half2 __builtin_spirv_OpenCL_modf_v2f16_p4v2f16( half2 x, __generic half2* iptr );
half3 __builtin_spirv_OpenCL_modf_v3f16_p4v3f16( half3 x, __generic half3* iptr );
half4 __builtin_spirv_OpenCL_modf_v4f16_p4v4f16( half4 x, __generic half4* iptr );
half8 __builtin_spirv_OpenCL_modf_v8f16_p4v8f16( half8 x, __generic half8* iptr );
half16 __builtin_spirv_OpenCL_modf_v16f16_p4v16f16( half16 x, __generic half16* iptr );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_modf_f64_p1f64( double x, __global double* iptr );
double2 __builtin_spirv_OpenCL_modf_v2f64_p1v2f64( double2 x, __global double2* iptr );
double3 __builtin_spirv_OpenCL_modf_v3f64_p1v3f64( double3 x, __global double3* iptr );
double4 __builtin_spirv_OpenCL_modf_v4f64_p1v4f64( double4 x, __global double4* iptr );
double8 __builtin_spirv_OpenCL_modf_v8f64_p1v8f64( double8 x, __global double8* iptr );
double16 __builtin_spirv_OpenCL_modf_v16f64_p1v16f64( double16  x, __global double16* iptr );
double __builtin_spirv_OpenCL_modf_f64_p0f64( double x, __private double* iptr );
double2 __builtin_spirv_OpenCL_modf_v2f64_p0v2f64( double2 x, __private double2* iptr );
double3 __builtin_spirv_OpenCL_modf_v3f64_p0v3f64( double3 x, __private double3* iptr );
double4 __builtin_spirv_OpenCL_modf_v4f64_p0v4f64( double4 x, __private double4* iptr );
double8 __builtin_spirv_OpenCL_modf_v8f64_p0v8f64( double8 x, __private double8* iptr );
double16 __builtin_spirv_OpenCL_modf_v16f64_p0v16f64( double16   x, __private double16* iptr );
double __builtin_spirv_OpenCL_modf_f64_p3f64( double x, __local double* iptr );
double2 __builtin_spirv_OpenCL_modf_v2f64_p3v2f64( double2 x, __local double2* iptr );
double3 __builtin_spirv_OpenCL_modf_v3f64_p3v3f64( double3 x, __local double3* iptr );
double4 __builtin_spirv_OpenCL_modf_v4f64_p3v4f64( double4 x, __local double4* iptr );
double8 __builtin_spirv_OpenCL_modf_v8f64_p3v8f64( double8 x, __local double8* iptr );
double16 __builtin_spirv_OpenCL_modf_v16f64_p3v16f64( double16 x, __local double16* iptr );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __builtin_spirv_OpenCL_modf_f64_p4f64( double x, __generic double* iptr );
double2 __builtin_spirv_OpenCL_modf_v2f64_p4v2f64( double2 x, __generic double2* iptr );
double3 __builtin_spirv_OpenCL_modf_v3f64_p4v3f64( double3 x, __generic double3* iptr );
double4 __builtin_spirv_OpenCL_modf_v4f64_p4v4f64( double4 x, __generic double4* iptr );
double8 __builtin_spirv_OpenCL_modf_v8f64_p4v8f64( double8 x, __generic double8* iptr );
double16 __builtin_spirv_OpenCL_modf_v16f64_p4v16f64( double16 x, __generic double16* iptr );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float __builtin_spirv_OpenCL_nan_i32( uint nancode );
float2 __builtin_spirv_OpenCL_nan_v2i32(uint2 x);
float3 __builtin_spirv_OpenCL_nan_v3i32(uint3 x);
float4 __builtin_spirv_OpenCL_nan_v4i32(uint4 x);
float8 __builtin_spirv_OpenCL_nan_v8i32(uint8 x);
float16 __builtin_spirv_OpenCL_nan_v16i32(uint16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_nan_i64( ulong nancode );
double2 __builtin_spirv_OpenCL_nan_v2i64(ulong2 x);
double3 __builtin_spirv_OpenCL_nan_v3i64(ulong3 x);
double4 __builtin_spirv_OpenCL_nan_v4i64(ulong4 x);
double8 __builtin_spirv_OpenCL_nan_v8i64(ulong8 x);
double16 __builtin_spirv_OpenCL_nan_v16i64(ulong16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_nan_i16( ushort nancode );
half2 __builtin_spirv_OpenCL_nan_v2i16(ushort2 x);
half3 __builtin_spirv_OpenCL_nan_v3i16(ushort3 x);
half4 __builtin_spirv_OpenCL_nan_v4i16(ushort4 x);
half8 __builtin_spirv_OpenCL_nan_v8i16(ushort8 x);
half16 __builtin_spirv_OpenCL_nan_v16i16(ushort16 x);

float __builtin_spirv_OpenCL_nextafter_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_nextafter_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_nextafter_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_nextafter_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_nextafter_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_nextafter_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_nextafter_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_nextafter_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_nextafter_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_nextafter_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_nextafter_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_nextafter_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_nextafter_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_nextafter_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_nextafter_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_nextafter_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_nextafter_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_nextafter_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_pow_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_pow_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_pow_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_pow_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_pow_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_pow_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_pow_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_pow_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_pow_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_pow_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_pow_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_pow_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_pow_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_pow_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_pow_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_pow_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_pow_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_pow_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_pown_f32_i32( float x, int y );
float2 __builtin_spirv_OpenCL_pown_v2f32_v2i32(float2 x, int2 y);
float3 __builtin_spirv_OpenCL_pown_v3f32_v3i32(float3 x, int3 y);
float4 __builtin_spirv_OpenCL_pown_v4f32_v4i32(float4 x, int4 y);
float8 __builtin_spirv_OpenCL_pown_v8f32_v8i32(float8 x, int8 y);
float16 __builtin_spirv_OpenCL_pown_v16f32_v16i32(float16 x, int16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_pown_f64_i32( double x, int y );
double2 __builtin_spirv_OpenCL_pown_v2f64_v2i32(double2 x, int2 y);
double3 __builtin_spirv_OpenCL_pown_v3f64_v3i32(double3 x, int3 y);
double4 __builtin_spirv_OpenCL_pown_v4f64_v4i32(double4 x, int4 y);
double8 __builtin_spirv_OpenCL_pown_v8f64_v8i32(double8 x, int8 y);
double16 __builtin_spirv_OpenCL_pown_v16f64_v16i32(double16 x, int16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_pown_f16_i32( half x, int y );
half2 __builtin_spirv_OpenCL_pown_v2f16_v2i32(half2 x, int2 y);
half3 __builtin_spirv_OpenCL_pown_v3f16_v3i32(half3 x, int3 y);
half4 __builtin_spirv_OpenCL_pown_v4f16_v4i32(half4 x, int4 y);
half8 __builtin_spirv_OpenCL_pown_v8f16_v8i32(half8 x, int8 y);
half16 __builtin_spirv_OpenCL_pown_v16f16_v16i32(half16 x, int16 y);

float __builtin_spirv_OpenCL_powr_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_powr_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_powr_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_powr_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_powr_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_powr_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_powr_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_powr_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_powr_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_powr_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_powr_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_powr_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_powr_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_powr_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_powr_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_powr_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_powr_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_powr_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_remainder_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_remainder_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_remainder_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_remainder_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_remainder_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_remainder_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_remainder_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_remainder_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_remainder_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_remainder_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_remainder_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_remainder_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_remainder_f16_f16( half y, half x );
half2 __builtin_spirv_OpenCL_remainder_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_remainder_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_remainder_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_remainder_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_remainder_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_remquo_f32_f32_p1i32( float xx, float yy, __global int* quo );
float2 __builtin_spirv_OpenCL_remquo_v2f32_v2f32_p1v2i32( float2 xx, float2 yy, __global int2* quo );
float3 __builtin_spirv_OpenCL_remquo_v3f32_v3f32_p1v3i32( float3 xx, float3 yy, __global int3* quo );
float4 __builtin_spirv_OpenCL_remquo_v4f32_v4f32_p1v4i32( float4 xx, float4 yy, __global int4* quo );
float8 __builtin_spirv_OpenCL_remquo_v8f32_v8f32_p1v8i32( float8 xx, float8 yy, __global int8* quo );
float16 __builtin_spirv_OpenCL_remquo_v16f32_v16f32_p1v16i32( float16 xx, float16 yy, __global int16* quo );
float __builtin_spirv_OpenCL_remquo_f32_f32_p0i32( float xx, float yy, __private int* quo );
float2 __builtin_spirv_OpenCL_remquo_v2f32_v2f32_p0v2i32( float2 xx, float2 yy, __private int2* quo );
float3 __builtin_spirv_OpenCL_remquo_v3f32_v3f32_p0v3i32( float3 xx, float3 yy, __private int3* quo );
float4 __builtin_spirv_OpenCL_remquo_v4f32_v4f32_p0v4i32( float4 xx, float4 yy, __private int4* quo );
float8 __builtin_spirv_OpenCL_remquo_v8f32_v8f32_p0v8i32( float8 xx, float8 yy, __private int8* quo );
float16 __builtin_spirv_OpenCL_remquo_v16f32_v16f32_p0v16i32( float16 xx, float16 yy, __private int16* quo );
float __builtin_spirv_OpenCL_remquo_f32_f32_p3i32( float xx, float yy, __local int* quo );
float2 __builtin_spirv_OpenCL_remquo_v2f32_v2f32_p3v2i32( float2 xx, float2 yy, __local int2* quo );
float3 __builtin_spirv_OpenCL_remquo_v3f32_v3f32_p3v3i32( float3 xx, float3 yy, __local int3* quo );
float4 __builtin_spirv_OpenCL_remquo_v4f32_v4f32_p3v4i32( float4 xx, float4 yy, __local int4* quo );
float8 __builtin_spirv_OpenCL_remquo_v8f32_v8f32_p3v8i32( float8 xx, float8 yy, __local int8* quo );
float16 __builtin_spirv_OpenCL_remquo_v16f32_v16f32_p3v16i32( float16 xx, float16 yy, __local int16* quo );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __builtin_spirv_OpenCL_remquo_f32_f32_p4i32( float xx, float yy, __generic int* quo );
float2 __builtin_spirv_OpenCL_remquo_v2f32_v2f32_p4v2i32( float2 xx, float2 yy, __generic int2* quo );
float3 __builtin_spirv_OpenCL_remquo_v3f32_v3f32_p4v3i32( float3 xx, float3 yy, __generic int3* quo );
float4 __builtin_spirv_OpenCL_remquo_v4f32_v4f32_p4v4i32( float4 xx, float4 yy, __generic int4* quo );
float8 __builtin_spirv_OpenCL_remquo_v8f32_v8f32_p4v8i32( float8 xx, float8 yy, __generic int8* quo );
float16 __builtin_spirv_OpenCL_remquo_v16f32_v16f32_p4v16i32( float16 xx, float16 yy, __generic int16* quo );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half __builtin_spirv_OpenCL_remquo_f16_f16_p1i32( half xx, half yy, __global int* quo );
half2 __builtin_spirv_OpenCL_remquo_v2f16_v2f16_p1v2i32( half2 xx, half2 yy, __global int2* quo );
half3 __builtin_spirv_OpenCL_remquo_v3f16_v3f16_p1v3i32( half3 xx, half3 yy, __global int3* quo );
half4 __builtin_spirv_OpenCL_remquo_v4f16_v4f16_p1v4i32( half4 xx, half4 yy, __global int4* quo );
half8 __builtin_spirv_OpenCL_remquo_v8f16_v8f16_p1v8i32( half8 xx, half8 yy, __global int8* quo );
half16 __builtin_spirv_OpenCL_remquo_v16f16_v16f16_p1v16i32( half16 xx, half16 yy, __global int16* quo );
half __builtin_spirv_OpenCL_remquo_f16_f16_p0i32( half xx, half yy, __private int* quo );
half2 __builtin_spirv_OpenCL_remquo_v2f16_v2f16_p0v2i32( half2 xx, half2 yy, __private int2* quo );
half3 __builtin_spirv_OpenCL_remquo_v3f16_v3f16_p0v3i32( half3 xx, half3 yy, __private int3* quo );
half4 __builtin_spirv_OpenCL_remquo_v4f16_v4f16_p0v4i32( half4 xx, half4 yy, __private int4* quo );
half8 __builtin_spirv_OpenCL_remquo_v8f16_v8f16_p0v8i32( half8 xx, half8 yy, __private int8* quo );
half16 __builtin_spirv_OpenCL_remquo_v16f16_v16f16_p0v16i32( half16 xx, half16 yy, __private int16* quo );
half __builtin_spirv_OpenCL_remquo_f16_f16_p3i32( half xx, half yy, __local int* quo );
half2 __builtin_spirv_OpenCL_remquo_v2f16_v2f16_p3v2i32( half2 xx, half2 yy, __local int2* quo );
half3 __builtin_spirv_OpenCL_remquo_v3f16_v3f16_p3v3i32( half3 xx, half3 yy, __local int3* quo );
half4 __builtin_spirv_OpenCL_remquo_v4f16_v4f16_p3v4i32( half4 xx, half4 yy, __local int4* quo );
half8 __builtin_spirv_OpenCL_remquo_v8f16_v8f16_p3v8i32( half8 xx, half8 yy, __local int8* quo );
half16 __builtin_spirv_OpenCL_remquo_v16f16_v16f16_p3v16i32( half16 xx, half16 yy, __local int16* quo );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __builtin_spirv_OpenCL_remquo_f16_f16_p4i32( half xx, half yy, __generic int* quo );
half2 __builtin_spirv_OpenCL_remquo_v2f16_v2f16_p4v2i32( half2 xx, half2 yy, __generic int2* quo );
half3 __builtin_spirv_OpenCL_remquo_v3f16_v3f16_p4v3i32( half3 xx, half3 yy, __generic int3* quo );
half4 __builtin_spirv_OpenCL_remquo_v4f16_v4f16_p4v4i32( half4 xx, half4 yy, __generic int4* quo );
half8 __builtin_spirv_OpenCL_remquo_v8f16_v8f16_p4v8i32( half8 xx, half8 yy, __generic int8* quo );
half16 __builtin_spirv_OpenCL_remquo_v16f16_v16f16_p4v16i32( half16 xx, half16 yy, __generic int16* quo );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_remquo_f64_f64_p1i32( double xx, double yy, __global int* quo );
double2 __builtin_spirv_OpenCL_remquo_v2f64_v2f64_p1v2i32( double2 xx, double2 yy, __global int2* quo );
double3 __builtin_spirv_OpenCL_remquo_v3f64_v3f64_p1v3i32( double3 xx, double3 yy, __global int3* quo );
double4 __builtin_spirv_OpenCL_remquo_v4f64_v4f64_p1v4i32( double4 xx, double4 yy, __global int4* quo );
double8 __builtin_spirv_OpenCL_remquo_v8f64_v8f64_p1v8i32( double8 xx, double8 yy, __global int8* quo );
double16 __builtin_spirv_OpenCL_remquo_v16f64_v16f64_p1v16i32( double16 xx, double16 yy, __global int16* quo );
double __builtin_spirv_OpenCL_remquo_f64_f64_p0i32( double xx, double yy, __private int* quo );
double2 __builtin_spirv_OpenCL_remquo_v2f64_v2f64_p0v2i32( double2 xx, double2 yy, __private int2* quo );
double3 __builtin_spirv_OpenCL_remquo_v3f64_v3f64_p0v3i32( double3 xx, double3 yy, __private int3* quo );
double4 __builtin_spirv_OpenCL_remquo_v4f64_v4f64_p0v4i32( double4 xx, double4 yy, __private int4* quo );
double8 __builtin_spirv_OpenCL_remquo_v8f64_v8f64_p0v8i32( double8 xx, double8 yy, __private int8* quo );
double16 __builtin_spirv_OpenCL_remquo_v16f64_v16f64_p0v16i32( double16 xx, double16 yy, __private int16* quo );
double __builtin_spirv_OpenCL_remquo_f64_f64_p3i32( double xx, double yy, __local int* quo );
double2 __builtin_spirv_OpenCL_remquo_v2f64_v2f64_p3v2i32( double2 xx, double2 yy, __local int2* quo );
double3 __builtin_spirv_OpenCL_remquo_v3f64_v3f64_p3v3i32( double3 xx, double3 yy, __local int3* quo );
double4 __builtin_spirv_OpenCL_remquo_v4f64_v4f64_p3v4i32( double4 xx, double4 yy, __local int4* quo );
double8 __builtin_spirv_OpenCL_remquo_v8f64_v8f64_p3v8i32( double8 xx, double8 yy, __local int8* quo );
double16 __builtin_spirv_OpenCL_remquo_v16f64_v16f64_p3v16i32( double16 xx, double16 yy, __local int16* quo );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __builtin_spirv_OpenCL_remquo_f64_f64_p4i32( double xx, double yy, __generic int* quo );
double2 __builtin_spirv_OpenCL_remquo_v2f64_v2f64_p4v2i32( double2 xx, double2 yy, __generic int2* quo );
double3 __builtin_spirv_OpenCL_remquo_v3f64_v3f64_p4v3i32( double3 xx, double3 yy, __generic int3* quo );
double4 __builtin_spirv_OpenCL_remquo_v4f64_v4f64_p4v4i32( double4 xx, double4 yy, __generic int4* quo );
double8 __builtin_spirv_OpenCL_remquo_v8f64_v8f64_p4v8i32( double8 xx, double8 yy, __generic int8* quo );
double16 __builtin_spirv_OpenCL_remquo_v16f64_v16f64_p4v16i32( double16 xx, double16 yy, __generic int16* quo );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float __builtin_spirv_OpenCL_rint_f32(float x );
float2 __builtin_spirv_OpenCL_rint_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_rint_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_rint_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_rint_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_rint_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_rint_f64( double x );
double2 __builtin_spirv_OpenCL_rint_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_rint_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_rint_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_rint_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_rint_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_rint_f16(half x );
half2 __builtin_spirv_OpenCL_rint_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_rint_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_rint_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_rint_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_rint_v16f16(half16 x);

float __builtin_spirv_OpenCL_rootn_f32_i32( float x, int n );
float2 __builtin_spirv_OpenCL_rootn_v2f32_v2i32(float2 x, int2 y);
float3 __builtin_spirv_OpenCL_rootn_v3f32_v3i32(float3 x, int3 y);
float4 __builtin_spirv_OpenCL_rootn_v4f32_v4i32(float4 x, int4 y);
float8 __builtin_spirv_OpenCL_rootn_v8f32_v8i32(float8 x, int8 y);
float16 __builtin_spirv_OpenCL_rootn_v16f32_v16i32(float16 x, int16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_rootn_f64_i32( double y, int x );
double2 __builtin_spirv_OpenCL_rootn_v2f64_v2i32(double2 x, int2 y);
double3 __builtin_spirv_OpenCL_rootn_v3f64_v3i32(double3 x, int3 y);
double4 __builtin_spirv_OpenCL_rootn_v4f64_v4i32(double4 x, int4 y);
double8 __builtin_spirv_OpenCL_rootn_v8f64_v8i32(double8 x, int8 y);
double16 __builtin_spirv_OpenCL_rootn_v16f64_v16i32(double16 x, int16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_rootn_f16_i32( half y, int x );
half2 __builtin_spirv_OpenCL_rootn_v2f16_v2i32(half2 x, int2 y);
half3 __builtin_spirv_OpenCL_rootn_v3f16_v3i32(half3 x, int3 y);
half4 __builtin_spirv_OpenCL_rootn_v4f16_v4i32(half4 x, int4 y);
half8 __builtin_spirv_OpenCL_rootn_v8f16_v8i32(half8 x, int8 y);
half16 __builtin_spirv_OpenCL_rootn_v16f16_v16i32(half16 x, int16 y);

float __builtin_spirv_OpenCL_round_f32( float x );
float2 __builtin_spirv_OpenCL_round_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_round_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_round_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_round_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_round_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_round_f64( double x );
double2 __builtin_spirv_OpenCL_round_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_round_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_round_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_round_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_round_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_round_f16( half x );
half2 __builtin_spirv_OpenCL_round_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_round_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_round_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_round_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_round_v16f16(half16 x);

float __builtin_spirv_OpenCL_rsqrt_f32( float x );
float2 __builtin_spirv_OpenCL_rsqrt_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_rsqrt_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_rsqrt_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_rsqrt_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_rsqrt_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_rsqrt_f64( double x );
double2 __builtin_spirv_OpenCL_rsqrt_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_rsqrt_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_rsqrt_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_rsqrt_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_rsqrt_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_rsqrt_f16( half x );
half2 __builtin_spirv_OpenCL_rsqrt_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_rsqrt_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_rsqrt_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_rsqrt_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_rsqrt_v16f16(half16 x);

float __builtin_spirv_OpenCL_sin_f32( float x );
float2 __builtin_spirv_OpenCL_sin_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_sin_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_sin_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_sin_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_sin_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_sin_f64( double x );
double2 __builtin_spirv_OpenCL_sin_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_sin_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_sin_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_sin_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_sin_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_sin_f16( half x );
half2 __builtin_spirv_OpenCL_sin_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_sin_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_sin_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_sin_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_sin_v16f16(half16 x);

float __builtin_spirv_OpenCL_sincos_f32_p0f32( float x, __private float* cosval );
float __builtin_spirv_OpenCL_sincos_f32_p1f32( float x, __global float* cosval );
float __builtin_spirv_OpenCL_sincos_f32_p3f32( float x, __local float* cosval );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
float __builtin_spirv_OpenCL_sincos_f32_p4f32( float x, __generic float* cosval );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
half __builtin_spirv_OpenCL_sincos_f16_p0f16( half x, __private half* cosval );
half __builtin_spirv_OpenCL_sincos_f16_p1f16( half x, __global half* cosval );
half __builtin_spirv_OpenCL_sincos_f16_p3f16( half x, __local half* cosval );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
half __builtin_spirv_OpenCL_sincos_f16_p4f16( half x, __generic half* cosval );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_sincos_f64_p0f64( double x, __private double* cosval );
double __builtin_spirv_OpenCL_sincos_f64_p3f64( double x, __local double* cosval );
double __builtin_spirv_OpenCL_sincos_f64_p1f64( double x, __global double* cosval );
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
double __builtin_spirv_OpenCL_sincos_f64_p4f64( double x, __generic double* cosval );
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
#endif // defined(cl_khr_fp64)

float __builtin_spirv_OpenCL_sinh_f32( float x );
float2 __builtin_spirv_OpenCL_sinh_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_sinh_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_sinh_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_sinh_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_sinh_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_sinh_f64( double x );
double2 __builtin_spirv_OpenCL_sinh_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_sinh_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_sinh_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_sinh_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_sinh_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_sinh_f16( half x );
half2 __builtin_spirv_OpenCL_sinh_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_sinh_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_sinh_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_sinh_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_sinh_v16f16(half16 x);

float __builtin_spirv_OpenCL_sinpi_f32( float x );
float2 __builtin_spirv_OpenCL_sinpi_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_sinpi_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_sinpi_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_sinpi_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_sinpi_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_sinpi_f64( double x );
double2 __builtin_spirv_OpenCL_sinpi_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_sinpi_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_sinpi_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_sinpi_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_sinpi_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_sinpi_f16( half x );
half2 __builtin_spirv_OpenCL_sinpi_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_sinpi_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_sinpi_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_sinpi_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_sinpi_v16f16(half16 x);

float __builtin_spirv_OpenCL_sqrt_f32( float x );
float2 __builtin_spirv_OpenCL_sqrt_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_sqrt_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_sqrt_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_sqrt_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_sqrt_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_sqrt_f64( double x );
double2 __builtin_spirv_OpenCL_sqrt_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_sqrt_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_sqrt_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_sqrt_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_sqrt_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_sqrt_f16( half x );
half2 __builtin_spirv_OpenCL_sqrt_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_sqrt_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_sqrt_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_sqrt_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_sqrt_v16f16(half16 x);

float __builtin_spirv_OpenCL_sqrt_cr_f32( float a );
float2 __builtin_spirv_OpenCL_sqrt_cr_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_sqrt_cr_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_sqrt_cr_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_sqrt_cr_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_sqrt_cr_v16f32(float16 x);
#ifdef cl_fp64_basic_ops
double __builtin_spirv_OpenCL_sqrt_cr_f64( double x );
double2 __builtin_spirv_OpenCL_sqrt_cr_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_sqrt_cr_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_sqrt_cr_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_sqrt_cr_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_sqrt_cr_v16f64(double16 x);
#endif // cl_fp64_basic_ops
half __builtin_spirv_OpenCL_sqrt_cr_f16( half a );
half2 __builtin_spirv_OpenCL_sqrt_cr_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_sqrt_cr_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_sqrt_cr_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_sqrt_cr_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_sqrt_cr_v16f16(half16 x);

float __builtin_spirv_OpenCL_tan_f32( float x );
float2 __builtin_spirv_OpenCL_tan_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_tan_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_tan_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_tan_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_tan_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_tan_f64( double x );
double2 __builtin_spirv_OpenCL_tan_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_tan_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_tan_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_tan_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_tan_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_tan_f16( half x );
half2 __builtin_spirv_OpenCL_tan_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_tan_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_tan_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_tan_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_tan_v16f16(half16 x);

float __builtin_spirv_OpenCL_tanh_f32( float x );
float2 __builtin_spirv_OpenCL_tanh_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_tanh_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_tanh_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_tanh_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_tanh_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_tanh_f64( double x );
double2 __builtin_spirv_OpenCL_tanh_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_tanh_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_tanh_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_tanh_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_tanh_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_tanh_f16( half x );
half2 __builtin_spirv_OpenCL_tanh_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_tanh_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_tanh_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_tanh_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_tanh_v16f16(half16 x);

float __builtin_spirv_OpenCL_tanpi_f32( float x );
float2 __builtin_spirv_OpenCL_tanpi_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_tanpi_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_tanpi_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_tanpi_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_tanpi_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_tanpi_f64( double x );
double2 __builtin_spirv_OpenCL_tanpi_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_tanpi_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_tanpi_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_tanpi_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_tanpi_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_tanpi_f16( half x );
half2 __builtin_spirv_OpenCL_tanpi_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_tanpi_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_tanpi_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_tanpi_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_tanpi_v16f16(half16 x);

float __builtin_spirv_OpenCL_tgamma_f32( float x );
float2 __builtin_spirv_OpenCL_tgamma_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_tgamma_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_tgamma_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_tgamma_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_tgamma_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_tgamma_f64( double x );
double2 __builtin_spirv_OpenCL_tgamma_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_tgamma_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_tgamma_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_tgamma_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_tgamma_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_tgamma_f16( half x );
half2 __builtin_spirv_OpenCL_tgamma_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_tgamma_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_tgamma_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_tgamma_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_tgamma_v16f16(half16 x);

float __builtin_spirv_OpenCL_trunc_f32(float x );
float2 __builtin_spirv_OpenCL_trunc_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_trunc_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_trunc_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_trunc_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_trunc_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_trunc_f64(double x );
double2 __builtin_spirv_OpenCL_trunc_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_trunc_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_trunc_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_trunc_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_trunc_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_trunc_f16(half x );
half2 __builtin_spirv_OpenCL_trunc_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_trunc_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_trunc_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_trunc_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_trunc_v16f16(half16 x);

//
//  Native
//        -native_cos,native_divide,native_exp,native_exp2,native_exp10,native_log,native_log2,
//         native_log10,native_powr,native_recip,native_rsqrt,native_sin,native_sqrt,native_tan
//

float __builtin_spirv_OpenCL_native_cos_f32(float x );
float2 __builtin_spirv_OpenCL_native_cos_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_cos_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_cos_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_cos_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_cos_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_cos_f64( double x );
double2 __builtin_spirv_OpenCL_native_cos_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_cos_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_cos_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_cos_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_cos_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_cos_f16(half x );
half2 __builtin_spirv_OpenCL_native_cos_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_cos_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_cos_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_cos_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_cos_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_divide_f32_f32( float x, float y );
float2 __builtin_spirv_OpenCL_native_divide_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_native_divide_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_native_divide_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_native_divide_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_native_divide_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_divide_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_native_divide_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_native_divide_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_native_divide_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_native_divide_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_native_divide_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_divide_f16_f16( half x, half y );
half2 __builtin_spirv_OpenCL_native_divide_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_native_divide_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_native_divide_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_native_divide_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_native_divide_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_native_exp_f32( float x );
float2 __builtin_spirv_OpenCL_native_exp_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_exp_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_exp_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_exp_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_exp_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_exp_f64( double x );
double2 __builtin_spirv_OpenCL_native_exp_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_exp_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_exp_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_exp_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_exp_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_exp_f16( half x );
half2 __builtin_spirv_OpenCL_native_exp_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_exp_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_exp_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_exp_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_exp_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_exp2_f32(float x );
float2 __builtin_spirv_OpenCL_native_exp2_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_exp2_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_exp2_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_exp2_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_exp2_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_exp2_f64( double x );
double2 __builtin_spirv_OpenCL_native_exp2_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_exp2_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_exp2_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_exp2_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_exp2_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_exp2_f16(half x );
half2 __builtin_spirv_OpenCL_native_exp2_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_exp2_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_exp2_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_exp2_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_exp2_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_exp10_f32( float x );
float2 __builtin_spirv_OpenCL_native_exp10_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_exp10_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_exp10_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_exp10_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_exp10_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_exp10_f64( double x );
double2 __builtin_spirv_OpenCL_native_exp10_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_exp10_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_exp10_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_exp10_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_exp10_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_exp10_f16( half x );
half2 __builtin_spirv_OpenCL_native_exp10_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_exp10_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_exp10_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_exp10_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_exp10_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_log_f32( float x );
float2 __builtin_spirv_OpenCL_native_log_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_log_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_log_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_log_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_log_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_log_f64( double x );
double2 __builtin_spirv_OpenCL_native_log_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_log_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_log_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_log_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_log_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_log_f16( half x );
half2 __builtin_spirv_OpenCL_native_log_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_log_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_log_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_log_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_log_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_log2_f32(float x );
float2 __builtin_spirv_OpenCL_native_log2_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_log2_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_log2_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_log2_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_log2_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_log2_f64( double x );
double2 __builtin_spirv_OpenCL_native_log2_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_log2_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_log2_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_log2_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_log2_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_log2_f16(half x );
half2 __builtin_spirv_OpenCL_native_log2_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_log2_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_log2_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_log2_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_log2_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_log10_f32( float x );
float2 __builtin_spirv_OpenCL_native_log10_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_log10_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_log10_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_log10_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_log10_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_log10_f64( double x );
double2 __builtin_spirv_OpenCL_native_log10_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_log10_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_log10_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_log10_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_log10_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_log10_f16( half x );
half2 __builtin_spirv_OpenCL_native_log10_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_log10_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_log10_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_log10_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_log10_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_powr_f32_f32(float x, float y);
float2 __builtin_spirv_OpenCL_native_powr_v2f32_v2f32(float2 x, float2 y);
float3 __builtin_spirv_OpenCL_native_powr_v3f32_v3f32(float3 x, float3 y);
float4 __builtin_spirv_OpenCL_native_powr_v4f32_v4f32(float4 x, float4 y);
float8 __builtin_spirv_OpenCL_native_powr_v8f32_v8f32(float8 x, float8 y);
float16 __builtin_spirv_OpenCL_native_powr_v16f32_v16f32(float16 x, float16 y);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_powr_f64_f64( double x, double y );
double2 __builtin_spirv_OpenCL_native_powr_v2f64_v2f64(double2 x, double2 y);
double3 __builtin_spirv_OpenCL_native_powr_v3f64_v3f64(double3 x, double3 y);
double4 __builtin_spirv_OpenCL_native_powr_v4f64_v4f64(double4 x, double4 y);
double8 __builtin_spirv_OpenCL_native_powr_v8f64_v8f64(double8 x, double8 y);
double16 __builtin_spirv_OpenCL_native_powr_v16f64_v16f64(double16 x, double16 y);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_powr_f16_f16(half x, half y);
half2 __builtin_spirv_OpenCL_native_powr_v2f16_v2f16(half2 x, half2 y);
half3 __builtin_spirv_OpenCL_native_powr_v3f16_v3f16(half3 x, half3 y);
half4 __builtin_spirv_OpenCL_native_powr_v4f16_v4f16(half4 x, half4 y);
half8 __builtin_spirv_OpenCL_native_powr_v8f16_v8f16(half8 x, half8 y);
half16 __builtin_spirv_OpenCL_native_powr_v16f16_v16f16(half16 x, half16 y);

float __builtin_spirv_OpenCL_native_recip_f32(float x );
float2 __builtin_spirv_OpenCL_native_recip_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_recip_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_recip_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_recip_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_recip_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_recip_f64( double x );
double2 __builtin_spirv_OpenCL_native_recip_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_recip_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_recip_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_recip_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_recip_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_recip_f16(half x );
half2 __builtin_spirv_OpenCL_native_recip_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_recip_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_recip_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_recip_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_recip_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_rsqrt_f32(float x );
float2 __builtin_spirv_OpenCL_native_rsqrt_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_rsqrt_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_rsqrt_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_rsqrt_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_rsqrt_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_rsqrt_f64(double x );
double2 __builtin_spirv_OpenCL_native_rsqrt_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_rsqrt_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_rsqrt_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_rsqrt_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_rsqrt_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_rsqrt_f16(half x );
half2 __builtin_spirv_OpenCL_native_rsqrt_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_rsqrt_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_rsqrt_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_rsqrt_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_rsqrt_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_sin_f32(float x );
float2 __builtin_spirv_OpenCL_native_sin_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_sin_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_sin_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_sin_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_sin_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_sin_f64( double x );
double2 __builtin_spirv_OpenCL_native_sin_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_sin_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_sin_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_sin_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_sin_v16f64(double16 x);
#endif // cl_khr_fp64
half __builtin_spirv_OpenCL_native_sin_f16(half x );
half2 __builtin_spirv_OpenCL_native_sin_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_sin_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_sin_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_sin_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_sin_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_sqrt_f32(float x );
float2 __builtin_spirv_OpenCL_native_sqrt_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_sqrt_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_sqrt_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_sqrt_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_sqrt_v16f32(float16 x);
#ifdef cl_fp64_basic_ops
double __builtin_spirv_OpenCL_native_sqrt_f64(double x );
double2 __builtin_spirv_OpenCL_native_sqrt_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_sqrt_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_sqrt_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_sqrt_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_sqrt_v16f64(double16 x);
#endif // cl_fp64_basic_ops
half __builtin_spirv_OpenCL_native_sqrt_f16(half x );
half2 __builtin_spirv_OpenCL_native_sqrt_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_sqrt_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_sqrt_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_sqrt_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_sqrt_v16f16(half16 x);

float __builtin_spirv_OpenCL_native_tan_f32(float x );
float2 __builtin_spirv_OpenCL_native_tan_v2f32(float2 x);
float3 __builtin_spirv_OpenCL_native_tan_v3f32(float3 x);
float4 __builtin_spirv_OpenCL_native_tan_v4f32(float4 x);
float8 __builtin_spirv_OpenCL_native_tan_v8f32(float8 x);
float16 __builtin_spirv_OpenCL_native_tan_v16f32(float16 x);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_native_tan_f64( double x );
double2 __builtin_spirv_OpenCL_native_tan_v2f64(double2 x);
double3 __builtin_spirv_OpenCL_native_tan_v3f64(double3 x);
double4 __builtin_spirv_OpenCL_native_tan_v4f64(double4 x);
double8 __builtin_spirv_OpenCL_native_tan_v8f64(double8 x);
double16 __builtin_spirv_OpenCL_native_tan_v16f64(double16 x);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_native_tan_f16(half x );
half2 __builtin_spirv_OpenCL_native_tan_v2f16(half2 x);
half3 __builtin_spirv_OpenCL_native_tan_v3f16(half3 x);
half4 __builtin_spirv_OpenCL_native_tan_v4f16(half4 x);
half8 __builtin_spirv_OpenCL_native_tan_v8f16(half8 x);
half16 __builtin_spirv_OpenCL_native_tan_v16f16(half16 x);

//
//  Relational
//        -bitselect,select
//

uchar __builtin_spirv_OpenCL_bitselect_i8_i8_i8( uchar a, uchar b, uchar c );
uchar2 __builtin_spirv_OpenCL_bitselect_v2i8_v2i8_v2i8(uchar2 x, uchar2 y, uchar2 z);
uchar3 __builtin_spirv_OpenCL_bitselect_v3i8_v3i8_v3i8(uchar3 x, uchar3 y, uchar3 z);
uchar4 __builtin_spirv_OpenCL_bitselect_v4i8_v4i8_v4i8(uchar4 x, uchar4 y, uchar4 z);
uchar8 __builtin_spirv_OpenCL_bitselect_v8i8_v8i8_v8i8(uchar8 x, uchar8 y, uchar8 z);
uchar16 __builtin_spirv_OpenCL_bitselect_v16i8_v16i8_v16i8(uchar16 x, uchar16 y, uchar16 z);
ushort __builtin_spirv_OpenCL_bitselect_i16_i16_i16( ushort a, ushort b, ushort c );
ushort2 __builtin_spirv_OpenCL_bitselect_v2i16_v2i16_v2i16(ushort2 x, ushort2 y, ushort2 z);
ushort3 __builtin_spirv_OpenCL_bitselect_v3i16_v3i16_v3i16(ushort3 x, ushort3 y, ushort3 z);
ushort4 __builtin_spirv_OpenCL_bitselect_v4i16_v4i16_v4i16(ushort4 x, ushort4 y, ushort4 z);
ushort8 __builtin_spirv_OpenCL_bitselect_v8i16_v8i16_v8i16(ushort8 x, ushort8 y, ushort8 z);
ushort16 __builtin_spirv_OpenCL_bitselect_v16i16_v16i16_v16i16(ushort16 x, ushort16 y, ushort16 z);
uint __builtin_spirv_OpenCL_bitselect_i32_i32_i32( uint a, uint b, uint c );
uint2 __builtin_spirv_OpenCL_bitselect_v2i32_v2i32_v2i32(uint2 x, uint2 y, uint2 z);
uint3 __builtin_spirv_OpenCL_bitselect_v3i32_v3i32_v3i32(uint3 x, uint3 y, uint3 z);
uint4 __builtin_spirv_OpenCL_bitselect_v4i32_v4i32_v4i32(uint4 x, uint4 y, uint4 z);
uint8 __builtin_spirv_OpenCL_bitselect_v8i32_v8i32_v8i32(uint8 x, uint8 y, uint8 z);
uint16 __builtin_spirv_OpenCL_bitselect_v16i32_v16i32_v16i32(uint16 x, uint16 y, uint16 z);
ulong __builtin_spirv_OpenCL_bitselect_i64_i64_i64( ulong a, ulong b, ulong c );
ulong2 __builtin_spirv_OpenCL_bitselect_v2i64_v2i64_v2i64(ulong2 x, ulong2 y, ulong2 z);
ulong3 __builtin_spirv_OpenCL_bitselect_v3i64_v3i64_v3i64(ulong3 x, ulong3 y, ulong3 z);
ulong4 __builtin_spirv_OpenCL_bitselect_v4i64_v4i64_v4i64(ulong4 x, ulong4 y, ulong4 z);
ulong8 __builtin_spirv_OpenCL_bitselect_v8i64_v8i64_v8i64(ulong8 x, ulong8 y, ulong8 z);
ulong16 __builtin_spirv_OpenCL_bitselect_v16i64_v16i64_v16i64(ulong16 x, ulong16 y, ulong16 z);
float __builtin_spirv_OpenCL_bitselect_f32_f32_f32( float a, float b, float c );
float2 __builtin_spirv_OpenCL_bitselect_v2f32_v2f32_v2f32(float2 x, float2 y, float2 z);
float3 __builtin_spirv_OpenCL_bitselect_v3f32_v3f32_v3f32(float3 x, float3 y, float3 z);
float4 __builtin_spirv_OpenCL_bitselect_v4f32_v4f32_v4f32(float4 x, float4 y, float4 z);
float8 __builtin_spirv_OpenCL_bitselect_v8f32_v8f32_v8f32(float8 x, float8 y, float8 z);
float16 __builtin_spirv_OpenCL_bitselect_v16f32_v16f32_v16f32(float16 x, float16 y, float16 z);
#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_bitselect_f64_f64_f64( double a, double b, double c );
double2 __builtin_spirv_OpenCL_bitselect_v2f64_v2f64_v2f64(double2 x, double2 y, double2 z);
double3 __builtin_spirv_OpenCL_bitselect_v3f64_v3f64_v3f64(double3 x, double3 y, double3 z);
double4 __builtin_spirv_OpenCL_bitselect_v4f64_v4f64_v4f64(double4 x, double4 y, double4 z);
double8 __builtin_spirv_OpenCL_bitselect_v8f64_v8f64_v8f64(double8 x, double8 y, double8 z);
double16 __builtin_spirv_OpenCL_bitselect_v16f64_v16f64_v16f64(double16 x, double16 y, double16 z);
#endif // defined(cl_khr_fp64)
half __builtin_spirv_OpenCL_bitselect_f16_f16_f16( half a, half b, half c );
half2 __builtin_spirv_OpenCL_bitselect_v2f16_v2f16_v2f16(half2 x, half2 y, half2 z);
half3 __builtin_spirv_OpenCL_bitselect_v3f16_v3f16_v3f16(half3 x, half3 y, half3 z);
half4 __builtin_spirv_OpenCL_bitselect_v4f16_v4f16_v4f16(half4 x, half4 y, half4 z);
half8 __builtin_spirv_OpenCL_bitselect_v8f16_v8f16_v8f16(half8 x, half8 y, half8 z);
half16 __builtin_spirv_OpenCL_bitselect_v16f16_v16f16_v16f16(half16 x, half16 y, half16 z);

uchar __builtin_spirv_OpenCL_select_i8_i8_i8( uchar a, uchar b, uchar c );
ushort __builtin_spirv_OpenCL_select_i16_i16_i16( ushort a, ushort b, ushort c );
uint __builtin_spirv_OpenCL_select_i32_i32_i32( uint a, uint b, uint c );
ulong __builtin_spirv_OpenCL_select_i64_i64_i64( ulong a, ulong b, ulong c );

float __builtin_spirv_OpenCL_select_f32_f32_i32( float a, float b, uint c );
float2 __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32(float2 a, float2 b, uint2 c);
float3 __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32(float3 a, float3 b, uint3 c);
float4 __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32(float4 a, float4 b, uint4 c);
float8 __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32(float8 a, float8 b, uint8 c);
float16 __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32(float16 a, float16 b, uint16 c);

#if defined(cl_khr_fp64)
double __builtin_spirv_OpenCL_select_f64_f64_i64( double a, double b, ulong c );
double2 __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64(double2 a, double2 b, ulong2 c);
double3 __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64(double3 a, double3 b, ulong3 c);
double4 __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64(double4 a, double4 b, ulong4 c);
double8 __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64(double8 a, double8 b, ulong8 c);
double16 __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64(double16 a, double16 b, ulong16 c);

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
half __builtin_spirv_OpenCL_select_f16_f16_i16( half a, half b, ushort c );
half2 __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16(half2 a, half2 b, ushort2 c);
half3 __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16(half3 a, half3 b, ushort3 c);
half4 __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16(half4 a, half4 b, ushort4 c);
half8 __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16(half8 a, half8 b, ushort8 c);
half16 __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16(half16 a, half16 b, ushort16 c);

#endif // defined(cl_khr_fp16)


/******************************************* End of Math*******************************************************/





uchar2 __builtin_spirv_OpenCL_shuffle_v2i8_v2i8(uchar2 v, uchar2 m);
uchar2 __builtin_spirv_OpenCL_shuffle_v4i8_v2i8(uchar4 v, uchar2 m);
uchar2 __builtin_spirv_OpenCL_shuffle_v8i8_v2i8(uchar8 v, uchar2 m);
uchar2 __builtin_spirv_OpenCL_shuffle_v16i8_v2i8(uchar16 v, uchar2 m);
uchar4 __builtin_spirv_OpenCL_shuffle_v2i8_v4i8(uchar2 v, uchar4 m);
uchar4 __builtin_spirv_OpenCL_shuffle_v4i8_v4i8(uchar4 v, uchar4 m);
uchar4 __builtin_spirv_OpenCL_shuffle_v8i8_v4i8(uchar8 v, uchar4 m);
uchar4 __builtin_spirv_OpenCL_shuffle_v16i8_v4i8(uchar16 v, uchar4 m);
uchar8 __builtin_spirv_OpenCL_shuffle_v2i8_v8i8(uchar2 v, uchar8 m);
uchar8 __builtin_spirv_OpenCL_shuffle_v4i8_v8i8(uchar4 v, uchar8 m);
uchar8 __builtin_spirv_OpenCL_shuffle_v8i8_v8i8(uchar8 v, uchar8 m);
uchar8 __builtin_spirv_OpenCL_shuffle_v16i8_v8i8(uchar16 v, uchar8 m);
uchar16 __builtin_spirv_OpenCL_shuffle_v2i8_v16i8(uchar2 v, uchar16 m);
uchar16 __builtin_spirv_OpenCL_shuffle_v4i8_v16i8(uchar4 v, uchar16 m);
uchar16 __builtin_spirv_OpenCL_shuffle_v8i8_v16i8(uchar8 v, uchar16 m);
uchar16 __builtin_spirv_OpenCL_shuffle_v16i8_v16i8(uchar16 v, uchar16 m);
ushort2 __builtin_spirv_OpenCL_shuffle_v2i16_v2i16(ushort2 v, ushort2 m);
ushort2 __builtin_spirv_OpenCL_shuffle_v4i16_v2i16(ushort4 v, ushort2 m);
ushort2 __builtin_spirv_OpenCL_shuffle_v8i16_v2i16(ushort8 v, ushort2 m);
ushort2 __builtin_spirv_OpenCL_shuffle_v16i16_v2i16(ushort16 v, ushort2 m);
ushort4 __builtin_spirv_OpenCL_shuffle_v2i16_v4i16(ushort2 v, ushort4 m);
ushort4 __builtin_spirv_OpenCL_shuffle_v4i16_v4i16(ushort4 v, ushort4 m);
ushort4 __builtin_spirv_OpenCL_shuffle_v8i16_v4i16(ushort8 v, ushort4 m);
ushort4 __builtin_spirv_OpenCL_shuffle_v16i16_v4i16(ushort16 v, ushort4 m);
ushort8 __builtin_spirv_OpenCL_shuffle_v2i16_v8i16(ushort2 v, ushort8 m);
ushort8 __builtin_spirv_OpenCL_shuffle_v4i16_v8i16(ushort4 v, ushort8 m);
ushort8 __builtin_spirv_OpenCL_shuffle_v8i16_v8i16(ushort8 v, ushort8 m);
ushort8 __builtin_spirv_OpenCL_shuffle_v16i16_v8i16(ushort16 v, ushort8 m);
ushort16 __builtin_spirv_OpenCL_shuffle_v2i16_v16i16(ushort2 v, ushort16 m);
ushort16 __builtin_spirv_OpenCL_shuffle_v4i16_v16i16(ushort4 v, ushort16 m);
ushort16 __builtin_spirv_OpenCL_shuffle_v8i16_v16i16(ushort8 v, ushort16 m);
ushort16 __builtin_spirv_OpenCL_shuffle_v16i16_v16i16(ushort16 v, ushort16 m);
uint2 __builtin_spirv_OpenCL_shuffle_v2i32_v2i32(uint2 v, uint2 m);
uint2 __builtin_spirv_OpenCL_shuffle_v4i32_v2i32(uint4 v, uint2 m);
uint2 __builtin_spirv_OpenCL_shuffle_v8i32_v2i32(uint8 v, uint2 m);
uint2 __builtin_spirv_OpenCL_shuffle_v16i32_v2i32(uint16 v, uint2 m);
uint4 __builtin_spirv_OpenCL_shuffle_v2i32_v4i32(uint2 v, uint4 m);
uint4 __builtin_spirv_OpenCL_shuffle_v4i32_v4i32(uint4 v, uint4 m);
uint4 __builtin_spirv_OpenCL_shuffle_v8i32_v4i32(uint8 v, uint4 m);
uint4 __builtin_spirv_OpenCL_shuffle_v16i32_v4i32(uint16 v, uint4 m);
uint8 __builtin_spirv_OpenCL_shuffle_v2i32_v8i32(uint2 v, uint8 m);
uint8 __builtin_spirv_OpenCL_shuffle_v4i32_v8i32(uint4 v, uint8 m);
uint8 __builtin_spirv_OpenCL_shuffle_v8i32_v8i32(uint8 v, uint8 m);
uint8 __builtin_spirv_OpenCL_shuffle_v16i32_v8i32(uint16 v, uint8 m);
uint16 __builtin_spirv_OpenCL_shuffle_v2i32_v16i32(uint2 v, uint16 m);
uint16 __builtin_spirv_OpenCL_shuffle_v4i32_v16i32(uint4 v, uint16 m);
uint16 __builtin_spirv_OpenCL_shuffle_v8i32_v16i32(uint8 v, uint16 m);
uint16 __builtin_spirv_OpenCL_shuffle_v16i32_v16i32(uint16 v, uint16 m);
ulong2 __builtin_spirv_OpenCL_shuffle_v2i64_v2i64(ulong2 v, ulong2 m);
ulong2 __builtin_spirv_OpenCL_shuffle_v4i64_v2i64(ulong4 v, ulong2 m);
ulong2 __builtin_spirv_OpenCL_shuffle_v8i64_v2i64(ulong8 v, ulong2 m);
ulong2 __builtin_spirv_OpenCL_shuffle_v16i64_v2i64(ulong16 v, ulong2 m);
ulong4 __builtin_spirv_OpenCL_shuffle_v2i64_v4i64(ulong2 v, ulong4 m);
ulong4 __builtin_spirv_OpenCL_shuffle_v4i64_v4i64(ulong4 v, ulong4 m);
ulong4 __builtin_spirv_OpenCL_shuffle_v8i64_v4i64(ulong8 v, ulong4 m);
ulong4 __builtin_spirv_OpenCL_shuffle_v16i64_v4i64(ulong16 v, ulong4 m);
ulong8 __builtin_spirv_OpenCL_shuffle_v2i64_v8i64(ulong2 v, ulong8 m);
ulong8 __builtin_spirv_OpenCL_shuffle_v4i64_v8i64(ulong4 v, ulong8 m);
ulong8 __builtin_spirv_OpenCL_shuffle_v8i64_v8i64(ulong8 v, ulong8 m);
ulong8 __builtin_spirv_OpenCL_shuffle_v16i64_v8i64(ulong16 v, ulong8 m);
ulong16 __builtin_spirv_OpenCL_shuffle_v2i64_v16i64(ulong2 v, ulong16 m);
ulong16 __builtin_spirv_OpenCL_shuffle_v4i64_v16i64(ulong4 v, ulong16 m);
ulong16 __builtin_spirv_OpenCL_shuffle_v8i64_v16i64(ulong8 v, ulong16 m);
ulong16 __builtin_spirv_OpenCL_shuffle_v16i64_v16i64(ulong16 v, ulong16 m);
float2 __builtin_spirv_OpenCL_shuffle_v2f32_v2i32(float2 v, uint2 m);
float2 __builtin_spirv_OpenCL_shuffle_v4f32_v2i32(float4 v, uint2 m);
float2 __builtin_spirv_OpenCL_shuffle_v8f32_v2i32(float8 v, uint2 m);
float2 __builtin_spirv_OpenCL_shuffle_v16f32_v2i32(float16 v, uint2 m);
float4 __builtin_spirv_OpenCL_shuffle_v2f32_v4i32(float2 v, uint4 m);
float4 __builtin_spirv_OpenCL_shuffle_v4f32_v4i32(float4 v, uint4 m);
float4 __builtin_spirv_OpenCL_shuffle_v8f32_v4i32(float8 v, uint4 m);
float4 __builtin_spirv_OpenCL_shuffle_v16f32_v4i32(float16 v, uint4 m);
float8 __builtin_spirv_OpenCL_shuffle_v2f32_v8i32(float2 v, uint8 m);
float8 __builtin_spirv_OpenCL_shuffle_v4f32_v8i32(float4 v, uint8 m);
float8 __builtin_spirv_OpenCL_shuffle_v8f32_v8i32(float8 v, uint8 m);
float8 __builtin_spirv_OpenCL_shuffle_v16f32_v8i32(float16 v, uint8 m);
float16 __builtin_spirv_OpenCL_shuffle_v2f32_v16i32(float2 v, uint16 m);
float16 __builtin_spirv_OpenCL_shuffle_v4f32_v16i32(float4 v, uint16 m);
float16 __builtin_spirv_OpenCL_shuffle_v8f32_v16i32(float8 v, uint16 m);
float16 __builtin_spirv_OpenCL_shuffle_v16f32_v16i32(float16 v, uint16 m);

uchar2 __builtin_spirv_OpenCL_shuffle2_v2i8_v2i8_v2i8(uchar2 v0, uchar2 v1, uchar2 m);
uchar2 __builtin_spirv_OpenCL_shuffle2_v4i8_v4i8_v2i8(uchar4 v0, uchar4 v1, uchar2 m);
uchar2 __builtin_spirv_OpenCL_shuffle2_v8i8_v8i8_v2i8(uchar8 v0, uchar8 v1, uchar2 m);
uchar2 __builtin_spirv_OpenCL_shuffle2_v16i8_v16i8_v2i8(uchar16 v0, uchar16 v1, uchar2 m);
uchar4 __builtin_spirv_OpenCL_shuffle2_v2i8_v2i8_v4i8(uchar2 v0, uchar2 v1, uchar4 m);
uchar4 __builtin_spirv_OpenCL_shuffle2_v4i8_v4i8_v4i8(uchar4 v0, uchar4 v1, uchar4 m);
uchar4 __builtin_spirv_OpenCL_shuffle2_v8i8_v8i8_v4i8(uchar8 v0, uchar8 v1, uchar4 m);
uchar4 __builtin_spirv_OpenCL_shuffle2_v16i8_v16i8_v4i8(uchar16 v0, uchar16 v1, uchar4 m);
uchar8 __builtin_spirv_OpenCL_shuffle2_v2i8_v2i8_v8i8(uchar2 v0, uchar2 v1, uchar8 m);
uchar8 __builtin_spirv_OpenCL_shuffle2_v4i8_v4i8_v8i8(uchar4 v0, uchar4 v1, uchar8 m);
uchar8 __builtin_spirv_OpenCL_shuffle2_v8i8_v8i8_v8i8(uchar8 v0, uchar8 v1, uchar8 m);
uchar8 __builtin_spirv_OpenCL_shuffle2_v16i8_v16i8_v8i8(uchar16 v0, uchar16 v1, uchar8 m);
uchar16 __builtin_spirv_OpenCL_shuffle2_v2i8_v2i8_v16i8(uchar2 v0, uchar2 v1, uchar16 m);
uchar16 __builtin_spirv_OpenCL_shuffle2_v4i8_v4i8_v16i8(uchar4 v0, uchar4 v1, uchar16 m);
uchar16 __builtin_spirv_OpenCL_shuffle2_v8i8_v8i8_v16i8(uchar8 v0, uchar8 v1, uchar16 m);
uchar16 __builtin_spirv_OpenCL_shuffle2_v16i8_v16i8_v16i8(uchar16 v0, uchar16 v1, uchar16 m);
ushort2 __builtin_spirv_OpenCL_shuffle2_v2i16_v2i16_v2i16(ushort2 v0, ushort2 v1, ushort2 m);
ushort2 __builtin_spirv_OpenCL_shuffle2_v4i16_v4i16_v2i16(ushort4 v0, ushort4 v1, ushort2 m);
ushort2 __builtin_spirv_OpenCL_shuffle2_v8i16_v8i16_v2i16(ushort8 v0, ushort8 v1, ushort2 m);
ushort2 __builtin_spirv_OpenCL_shuffle2_v16i16_v16i16_v2i16(ushort16 v0, ushort16 v1, ushort2 m);
ushort4 __builtin_spirv_OpenCL_shuffle2_v2i16_v2i16_v4i16(ushort2 v0, ushort2 v1, ushort4 m);
ushort4 __builtin_spirv_OpenCL_shuffle2_v4i16_v4i16_v4i16(ushort4 v0, ushort4 v1, ushort4 m);
ushort4 __builtin_spirv_OpenCL_shuffle2_v8i16_v8i16_v4i16(ushort8 v0, ushort8 v1, ushort4 m);
ushort4 __builtin_spirv_OpenCL_shuffle2_v16i16_v16i16_v4i16(ushort16 v0, ushort16 v1, ushort4 m);
ushort8 __builtin_spirv_OpenCL_shuffle2_v2i16_v2i16_v8i16(ushort2 v0, ushort2 v1, ushort8 m);
ushort8 __builtin_spirv_OpenCL_shuffle2_v4i16_v4i16_v8i16(ushort4 v0, ushort4 v1, ushort8 m);
ushort8 __builtin_spirv_OpenCL_shuffle2_v8i16_v8i16_v8i16(ushort8 v0, ushort8 v1, ushort8 m);
ushort8 __builtin_spirv_OpenCL_shuffle2_v16i16_v16i16_v8i16(ushort16 v0, ushort16 v1, ushort8 m);
ushort16 __builtin_spirv_OpenCL_shuffle2_v2i16_v2i16_v16i16(ushort2 v0, ushort2 v1, ushort16 m);
ushort16 __builtin_spirv_OpenCL_shuffle2_v4i16_v4i16_v16i16(ushort4 v0, ushort4 v1, ushort16 m);
ushort16 __builtin_spirv_OpenCL_shuffle2_v8i16_v8i16_v16i16(ushort8 v0, ushort8 v1, ushort16 m);
ushort16 __builtin_spirv_OpenCL_shuffle2_v16i16_v16i16_v16i16(ushort16 v0, ushort16 v1, ushort16 m);
uint2 __builtin_spirv_OpenCL_shuffle2_v2i32_v2i32_v2i32(uint2 v0, uint2 v1, uint2 m);
uint2 __builtin_spirv_OpenCL_shuffle2_v4i32_v4i32_v2i32(uint4 v0, uint4 v1, uint2 m);
uint2 __builtin_spirv_OpenCL_shuffle2_v8i32_v8i32_v2i32(uint8 v0, uint8 v1, uint2 m);
uint2 __builtin_spirv_OpenCL_shuffle2_v16i32_v16i32_v2i32(uint16 v0, uint16 v1, uint2 m);
uint4 __builtin_spirv_OpenCL_shuffle2_v2i32_v2i32_v4i32(uint2 v0, uint2 v1, uint4 m);
uint4 __builtin_spirv_OpenCL_shuffle2_v4i32_v4i32_v4i32(uint4 v0, uint4 v1, uint4 m);
uint4 __builtin_spirv_OpenCL_shuffle2_v8i32_v8i32_v4i32(uint8 v0, uint8 v1, uint4 m);
uint4 __builtin_spirv_OpenCL_shuffle2_v16i32_v16i32_v4i32(uint16 v0, uint16 v1, uint4 m);
uint8 __builtin_spirv_OpenCL_shuffle2_v2i32_v2i32_v8i32(uint2 v0, uint2 v1, uint8 m);
uint8 __builtin_spirv_OpenCL_shuffle2_v4i32_v4i32_v8i32(uint4 v0, uint4 v1, uint8 m);
uint8 __builtin_spirv_OpenCL_shuffle2_v8i32_v8i32_v8i32(uint8 v0, uint8 v1, uint8 m);
uint8 __builtin_spirv_OpenCL_shuffle2_v16i32_v16i32_v8i32(uint16 v0, uint16 v1, uint8 m);
uint16 __builtin_spirv_OpenCL_shuffle2_v2i32_v2i32_v16i32(uint2 v0, uint2 v1, uint16 m);
uint16 __builtin_spirv_OpenCL_shuffle2_v4i32_v4i32_v16i32(uint4 v0, uint4 v1, uint16 m);
uint16 __builtin_spirv_OpenCL_shuffle2_v8i32_v8i32_v16i32(uint8 v0, uint8 v1, uint16 m);
uint16 __builtin_spirv_OpenCL_shuffle2_v16i32_v16i32_v16i32(uint16 v0, uint16 v1, uint16 m);
ulong2 __builtin_spirv_OpenCL_shuffle2_v2i64_v2i64_v2i64(ulong2 v0, ulong2 v1, ulong2 m);
ulong2 __builtin_spirv_OpenCL_shuffle2_v4i64_v4i64_v2i64(ulong4 v0, ulong4 v1, ulong2 m);
ulong2 __builtin_spirv_OpenCL_shuffle2_v8i64_v8i64_v2i64(ulong8 v0, ulong8 v1, ulong2 m);
ulong2 __builtin_spirv_OpenCL_shuffle2_v16i64_v16i64_v2i64(ulong16 v0, ulong16 v1, ulong2 m);
ulong4 __builtin_spirv_OpenCL_shuffle2_v2i64_v2i64_v4i64(ulong2 v0, ulong2 v1, ulong4 m);
ulong4 __builtin_spirv_OpenCL_shuffle2_v4i64_v4i64_v4i64(ulong4 v0, ulong4 v1, ulong4 m);
ulong4 __builtin_spirv_OpenCL_shuffle2_v8i64_v8i64_v4i64(ulong8 v0, ulong8 v1, ulong4 m);
ulong4 __builtin_spirv_OpenCL_shuffle2_v16i64_v16i64_v4i64(ulong16 v0, ulong16 v1, ulong4 m);
ulong8 __builtin_spirv_OpenCL_shuffle2_v2i64_v2i64_v8i64(ulong2 v0, ulong2 v1, ulong8 m);
ulong8 __builtin_spirv_OpenCL_shuffle2_v4i64_v4i64_v8i64(ulong4 v0, ulong4 v1, ulong8 m);
ulong8 __builtin_spirv_OpenCL_shuffle2_v8i64_v8i64_v8i64(ulong8 v0, ulong8 v1, ulong8 m);
ulong8 __builtin_spirv_OpenCL_shuffle2_v16i64_v16i64_v8i64(ulong16 v0, ulong16 v1, ulong8 m);
ulong16 __builtin_spirv_OpenCL_shuffle2_v2i64_v2i64_v16i64(ulong2 v0, ulong2 v1, ulong16 m);
ulong16 __builtin_spirv_OpenCL_shuffle2_v4i64_v4i64_v16i64(ulong4 v0, ulong4 v1, ulong16 m);
ulong16 __builtin_spirv_OpenCL_shuffle2_v8i64_v8i64_v16i64(ulong8 v0, ulong8 v1, ulong16 m);
ulong16 __builtin_spirv_OpenCL_shuffle2_v16i64_v16i64_v16i64(ulong16 v0, ulong16 v1, ulong16 m);
float2 __builtin_spirv_OpenCL_shuffle2_v2f32_v2f32_v2i32(float2 v0, float2 v1, uint2 m);
float2 __builtin_spirv_OpenCL_shuffle2_v4f32_v4f32_v2i32(float4 v0, float4 v1, uint2 m);
float2 __builtin_spirv_OpenCL_shuffle2_v8f32_v8f32_v2i32(float8 v0, float8 v1, uint2 m);
float2 __builtin_spirv_OpenCL_shuffle2_v16f32_v16f32_v2i32(float16 v0, float16 v1, uint2 m);
float4 __builtin_spirv_OpenCL_shuffle2_v2f32_v2f32_v4i32(float2 v0, float2 v1, uint4 m);
float4 __builtin_spirv_OpenCL_shuffle2_v4f32_v4f32_v4i32(float4 v0, float4 v1, uint4 m);
float4 __builtin_spirv_OpenCL_shuffle2_v8f32_v8f32_v4i32(float8 v0, float8 v1, uint4 m);
float4 __builtin_spirv_OpenCL_shuffle2_v16f32_v16f32_v4i32(float16 v0, float16 v1, uint4 m);
float8 __builtin_spirv_OpenCL_shuffle2_v2f32_v2f32_v8i32(float2 v0, float2 v1, uint8 m);
float8 __builtin_spirv_OpenCL_shuffle2_v4f32_v4f32_v8i32(float4 v0, float4 v1, uint8 m);
float8 __builtin_spirv_OpenCL_shuffle2_v8f32_v8f32_v8i32(float8 v0, float8 v1, uint8 m);
float8 __builtin_spirv_OpenCL_shuffle2_v16f32_v16f32_v8i32(float16 v0, float16 v1, uint8 m);
float16 __builtin_spirv_OpenCL_shuffle2_v2f32_v2f32_v16i32(float2 v0, float2 v1, uint16 m);
float16 __builtin_spirv_OpenCL_shuffle2_v4f32_v4f32_v16i32(float4 v0, float4 v1, uint16 m);
float16 __builtin_spirv_OpenCL_shuffle2_v8f32_v8f32_v16i32(float8 v0, float8 v1, uint16 m);
float16 __builtin_spirv_OpenCL_shuffle2_v16f32_v16f32_v16i32(float16 v0, float16 v1, uint16 m);

half2 __builtin_spirv_OpenCL_shuffle_v2f16_v2i16(half2 v, ushort2 m);
half2 __builtin_spirv_OpenCL_shuffle_v4f16_v2i16(half4 v, ushort2 m);
half2 __builtin_spirv_OpenCL_shuffle_v8f16_v2i16(half8 v, ushort2 m);
half2 __builtin_spirv_OpenCL_shuffle_v16f16_v2i16(half16 v, ushort2 m);
half4 __builtin_spirv_OpenCL_shuffle_v2f16_v4i16(half2 v, ushort4 m);
half4 __builtin_spirv_OpenCL_shuffle_v4f16_v4i16(half4 v, ushort4 m);
half4 __builtin_spirv_OpenCL_shuffle_v8f16_v4i16(half8 v, ushort4 m);
half4 __builtin_spirv_OpenCL_shuffle_v16f16_v4i16(half16 v, ushort4 m);
half8 __builtin_spirv_OpenCL_shuffle_v2f16_v8i16(half2 v, ushort8 m);
half8 __builtin_spirv_OpenCL_shuffle_v4f16_v8i16(half4 v, ushort8 m);
half8 __builtin_spirv_OpenCL_shuffle_v8f16_v8i16(half8 v, ushort8 m);
half8 __builtin_spirv_OpenCL_shuffle_v16f16_v8i16(half16 v, ushort8 m);
half16 __builtin_spirv_OpenCL_shuffle_v2f16_v16i16(half2 v, ushort16 m);
half16 __builtin_spirv_OpenCL_shuffle_v4f16_v16i16(half4 v, ushort16 m);
half16 __builtin_spirv_OpenCL_shuffle_v8f16_v16i16(half8 v, ushort16 m);
half16 __builtin_spirv_OpenCL_shuffle_v16f16_v16i16(half16 v, ushort16 m);

half2 __builtin_spirv_OpenCL_shuffle2_v2f16_v2f16_v2i16(half2 v0, half2 v1, ushort2 m);
half2 __builtin_spirv_OpenCL_shuffle2_v4f16_v4f16_v2i16(half4 v0, half4 v1, ushort2 m);
half2 __builtin_spirv_OpenCL_shuffle2_v8f16_v8f16_v2i16(half8 v0, half8 v1, ushort2 m);
half2 __builtin_spirv_OpenCL_shuffle2_v16f16_v16f16_v2i16(half16 v0, half16 v1, ushort2 m);
half4 __builtin_spirv_OpenCL_shuffle2_v2f16_v2f16_v4i16(half2 v0, half2 v1, ushort4 m);
half4 __builtin_spirv_OpenCL_shuffle2_v4f16_v4f16_v4i16(half4 v0, half4 v1, ushort4 m);
half4 __builtin_spirv_OpenCL_shuffle2_v8f16_v8f16_v4i16(half8 v0, half8 v1, ushort4 m);
half4 __builtin_spirv_OpenCL_shuffle2_v16f16_v16f16_v4i16(half16 v0, half16 v1, ushort4 m);
half8 __builtin_spirv_OpenCL_shuffle2_v2f16_v2f16_v8i16(half2 v0, half2 v1, ushort8 m);
half8 __builtin_spirv_OpenCL_shuffle2_v4f16_v4f16_v8i16(half4 v0, half4 v1, ushort8 m);
half8 __builtin_spirv_OpenCL_shuffle2_v8f16_v8f16_v8i16(half8 v0, half8 v1, ushort8 m);
half8 __builtin_spirv_OpenCL_shuffle2_v16f16_v16f16_v8i16(half16 v0, half16 v1, ushort8 m);
half16 __builtin_spirv_OpenCL_shuffle2_v2f16_v2f16_v16i16(half2 v0, half2 v1, ushort16 m);
half16 __builtin_spirv_OpenCL_shuffle2_v4f16_v4f16_v16i16(half4 v0, half4 v1, ushort16 m);
half16 __builtin_spirv_OpenCL_shuffle2_v8f16_v8f16_v16i16(half8 v0, half8 v1, ushort16 m);
half16 __builtin_spirv_OpenCL_shuffle2_v16f16_v16f16_v16i16(half16 v0, half16 v1, ushort16 m);

#if defined(cl_khr_fp64)
double2 __builtin_spirv_OpenCL_shuffle_v2f64_v2i64(double2 v, ulong2 m);
double2 __builtin_spirv_OpenCL_shuffle_v4f64_v2i64(double4 v, ulong2 m);
double2 __builtin_spirv_OpenCL_shuffle_v8f64_v2i64(double8 v, ulong2 m);
double2 __builtin_spirv_OpenCL_shuffle_v16f64_v2i64(double16 v, ulong2 m);
double4 __builtin_spirv_OpenCL_shuffle_v2f64_v4i64(double2 v, ulong4 m);
double4 __builtin_spirv_OpenCL_shuffle_v4f64_v4i64(double4 v, ulong4 m);
double4 __builtin_spirv_OpenCL_shuffle_v8f64_v4i64(double8 v, ulong4 m);
double4 __builtin_spirv_OpenCL_shuffle_v16f64_v4i64(double16 v, ulong4 m);
double8 __builtin_spirv_OpenCL_shuffle_v2f64_v8i64(double2 v, ulong8 m);
double8 __builtin_spirv_OpenCL_shuffle_v4f64_v8i64(double4 v, ulong8 m);
double8 __builtin_spirv_OpenCL_shuffle_v8f64_v8i64(double8 v, ulong8 m);
double8 __builtin_spirv_OpenCL_shuffle_v16f64_v8i64(double16 v, ulong8 m);
double16 __builtin_spirv_OpenCL_shuffle_v2f64_v16i64(double2 v, ulong16 m);
double16 __builtin_spirv_OpenCL_shuffle_v4f64_v16i64(double4 v, ulong16 m);
double16 __builtin_spirv_OpenCL_shuffle_v8f64_v16i64(double8 v, ulong16 m);
double16 __builtin_spirv_OpenCL_shuffle_v16f64_v16i64(double16 v, ulong16 m);

double2 __builtin_spirv_OpenCL_shuffle2_v2f64_v2f64_v2i64(double2 v0, double2 v1, ulong2 m);
double2 __builtin_spirv_OpenCL_shuffle2_v4f64_v4f64_v2i64(double4 v0, double4 v1, ulong2 m);
double2 __builtin_spirv_OpenCL_shuffle2_v8f64_v8f64_v2i64(double8 v0, double8 v1, ulong2 m);
double2 __builtin_spirv_OpenCL_shuffle2_v16f64_v16f64_v2i64(double16 v0, double16 v1, ulong2 m);
double4 __builtin_spirv_OpenCL_shuffle2_v2f64_v2f64_v4i64(double2 v0, double2 v1, ulong4 m);
double4 __builtin_spirv_OpenCL_shuffle2_v4f64_v4f64_v4i64(double4 v0, double4 v1, ulong4 m);
double4 __builtin_spirv_OpenCL_shuffle2_v8f64_v8f64_v4i64(double8 v0, double8 v1, ulong4 m);
double4 __builtin_spirv_OpenCL_shuffle2_v16f64_v16f64_v4i64(double16 v0, double16 v1, ulong4 m);
double8 __builtin_spirv_OpenCL_shuffle2_v2f64_v2f64_v8i64(double2 v0, double2 v1, ulong8 m);
double8 __builtin_spirv_OpenCL_shuffle2_v4f64_v4f64_v8i64(double4 v0, double4 v1, ulong8 m);
double8 __builtin_spirv_OpenCL_shuffle2_v8f64_v8f64_v8i64(double8 v0, double8 v1, ulong8 m);
double8 __builtin_spirv_OpenCL_shuffle2_v16f64_v16f64_v8i64(double16 v0, double16 v1, ulong8 m);
double16 __builtin_spirv_OpenCL_shuffle2_v2f64_v2f64_v16i64(double2 v0, double2 v1, ulong16 m);
double16 __builtin_spirv_OpenCL_shuffle2_v4f64_v4f64_v16i64(double4 v0, double4 v1, ulong16 m);
double16 __builtin_spirv_OpenCL_shuffle2_v8f64_v8f64_v16i64(double8 v0, double8 v1, ulong16 m);
double16 __builtin_spirv_OpenCL_shuffle2_v16f64_v16f64_v16i64(double16 v0, double16 v1, ulong16 m);
#endif // defined(cl_khr_fp64)



uint __builtin_spirv_intel_sub_group_shuffle_i32_i32(uint x, uint c );
uchar __builtin_spirv_intel_sub_group_shuffle_i8_i32(uchar x, uint c );
ushort __builtin_spirv_intel_sub_group_shuffle_i16_i32(ushort x, uint c );
float __builtin_spirv_intel_sub_group_shuffle_f32_i32(float x, uint c );
float2 __builtin_spirv_intel_sub_group_shuffle_v2f32_i32(float2 x, uint y);
float3 __builtin_spirv_intel_sub_group_shuffle_v3f32_i32(float3 x, uint y);
float4 __builtin_spirv_intel_sub_group_shuffle_v4f32_i32(float4 x, uint y);
float8 __builtin_spirv_intel_sub_group_shuffle_v8f32_i32(float8 x, uint y);
float16 __builtin_spirv_intel_sub_group_shuffle_v16f32_i32(float16 x, uint y);
uint2 __builtin_spirv_intel_sub_group_shuffle_v2i32_i32(uint2 x, uint y);
uint3 __builtin_spirv_intel_sub_group_shuffle_v3i32_i32(uint3 x, uint y);
uint4 __builtin_spirv_intel_sub_group_shuffle_v4i32_i32(uint4 x, uint y);
uint8 __builtin_spirv_intel_sub_group_shuffle_v8i32_i32(uint8 x, uint y);
uint16 __builtin_spirv_intel_sub_group_shuffle_v16i32_i32(uint16 x, uint y);
ulong __builtin_spirv_intel_sub_group_shuffle_i64_i32(ulong x, uint c );

uint __builtin_spirv_intel_sub_group_shuffle_down_i32_i32_i32(uint cur, uint next, uint c );
uchar __builtin_spirv_intel_sub_group_shuffle_down_i8_i8_i32(uchar cur, uchar next, uint c );
ushort __builtin_spirv_intel_sub_group_shuffle_down_i16_i16_i32(ushort cur, ushort next, uint c );
float __builtin_spirv_intel_sub_group_shuffle_down_f32_f32_i32(float cur, float next, uint c );
float2 __builtin_spirv_intel_sub_group_shuffle_down_v2f32_v2f32_i32(float2 x, float2 y, uint z);
float3 __builtin_spirv_intel_sub_group_shuffle_down_v3f32_v3f32_i32(float3 x, float3 y, uint z);
float4 __builtin_spirv_intel_sub_group_shuffle_down_v4f32_v4f32_i32(float4 x, float4 y, uint z);
float8 __builtin_spirv_intel_sub_group_shuffle_down_v8f32_v8f32_i32(float8 x, float8 y, uint z);
float16 __builtin_spirv_intel_sub_group_shuffle_down_v16f32_v16f32_i32(float16 x, float16 y, uint z);
uint2 __builtin_spirv_intel_sub_group_shuffle_down_v2i32_v2i32_i32(uint2 x, uint2 y, uint z);
uint3 __builtin_spirv_intel_sub_group_shuffle_down_v3i32_v3i32_i32(uint3 x, uint3 y, uint z);
uint4 __builtin_spirv_intel_sub_group_shuffle_down_v4i32_v4i32_i32(uint4 x, uint4 y, uint z);
uint8 __builtin_spirv_intel_sub_group_shuffle_down_v8i32_v8i32_i32(uint8 x, uint8 y, uint z);
uint16 __builtin_spirv_intel_sub_group_shuffle_down_v16i32_v16i32_i32(uint16 x, uint16 y, uint z);
ulong __builtin_spirv_intel_sub_group_shuffle_down_i64_i64_i32(ulong cur, ulong next, uint c );

uchar __builtin_spirv_intel_sub_group_shuffle_up_i8_i8_i32(uchar prev, uchar cur, uint c );
ushort __builtin_spirv_intel_sub_group_shuffle_up_i16_i16_i32(ushort prev, ushort cur, uint c );
uint __builtin_spirv_intel_sub_group_shuffle_up_i32_i32_i32(uint prev, uint cur, uint c );
float __builtin_spirv_intel_sub_group_shuffle_up_f32_f32_i32(float prev, float cur, uint c );
ulong __builtin_spirv_intel_sub_group_shuffle_up_i64_i64_i32(ulong prev, ulong cur, uint c );
uint2 __builtin_spirv_intel_sub_group_shuffle_up_v2i32_v2i32_i32(uint2 x, uint2 y, uint z);
uint3 __builtin_spirv_intel_sub_group_shuffle_up_v3i32_v3i32_i32(uint3 x, uint3 y, uint z);
uint4 __builtin_spirv_intel_sub_group_shuffle_up_v4i32_v4i32_i32(uint4 x, uint4 y, uint z);
uint8 __builtin_spirv_intel_sub_group_shuffle_up_v8i32_v8i32_i32(uint8 x, uint8 y, uint z);
uint16 __builtin_spirv_intel_sub_group_shuffle_up_v16i32_v16i32_i32(uint16 x, uint16 y, uint z);
float2 __builtin_spirv_intel_sub_group_shuffle_up_v2f32_v2f32_i32(float2 x, float2 y, uint z);
float3 __builtin_spirv_intel_sub_group_shuffle_up_v3f32_v3f32_i32(float3 x, float3 y, uint z);
float4 __builtin_spirv_intel_sub_group_shuffle_up_v4f32_v4f32_i32(float4 x, float4 y, uint z);
float8 __builtin_spirv_intel_sub_group_shuffle_up_v8f32_v8f32_i32(float8 x, float8 y, uint z);
float16 __builtin_spirv_intel_sub_group_shuffle_up_v16f32_v16f32_i32(float16 x, float16 y, uint z);

uchar __builtin_spirv_intel_sub_group_shuffle_xor_i8_i32(uchar x, uint c );
ushort __builtin_spirv_intel_sub_group_shuffle_xor_i16_i32(ushort x, uint c );
uint __builtin_spirv_intel_sub_group_shuffle_xor_i32_i32(uint x, uint c );
float __builtin_spirv_intel_sub_group_shuffle_xor_f32_i32(float x, uint c );
ulong __builtin_spirv_intel_sub_group_shuffle_xor_i64_i32(ulong x, uint c );
uint2 __builtin_spirv_intel_sub_group_shuffle_xor_v2i32_i32(uint2 x, uint y);
uint3 __builtin_spirv_intel_sub_group_shuffle_xor_v3i32_i32(uint3 x, uint y);
uint4 __builtin_spirv_intel_sub_group_shuffle_xor_v4i32_i32(uint4 x, uint y);
uint8 __builtin_spirv_intel_sub_group_shuffle_xor_v8i32_i32(uint8 x, uint y);
uint16 __builtin_spirv_intel_sub_group_shuffle_xor_v16i32_i32(uint16 x, uint y);
float2 __builtin_spirv_intel_sub_group_shuffle_xor_v2f32_i32(float2 x, uint y);
float3 __builtin_spirv_intel_sub_group_shuffle_xor_v3f32_i32(float3 x, uint y);
float4 __builtin_spirv_intel_sub_group_shuffle_xor_v4f32_i32(float4 x, uint y);
float8 __builtin_spirv_intel_sub_group_shuffle_xor_v8f32_i32(float8 x, uint y);
float16 __builtin_spirv_intel_sub_group_shuffle_xor_v16f32_i32(float16 x, uint y);

#if defined(cl_khr_fp64)
double __builtin_spirv_intel_sub_group_shuffle_f64_i32(double x, uint c );
double __builtin_spirv_intel_sub_group_shuffle_down_f64_f64_i32(double cur, double next, uint c );
double __builtin_spirv_intel_sub_group_shuffle_up_f64_f64_i32(double prev, double cur, uint c );
double __builtin_spirv_intel_sub_group_shuffle_xor_f64_i32(double x, uint c );
#endif // defined(cl_khr_fp64)

// TODO: how do we want to handle extensions?
#if 0
uint __builtin_spirv_intel_sub_group_half2_add_i32_i32(uint a, uint b );
uint __builtin_spirv_intel_sub_group_half2_sub_i32_i32(uint a, uint b );
uint __builtin_spirv_intel_sub_group_half2_mul_i32_i32(uint a, uint b );
uint __builtin_spirv_intel_sub_group_half2_mad_i32_i32_i32(uint a, uint b, uint c );
short2 __builtin_spirv_intel_sub_group_half2_isequal_i32_i32(uint a, uint b );
short2 __builtin_spirv_intel_sub_group_half2_isnotequal_i32_i32(uint a, uint b );
short2 __builtin_spirv_intel_sub_group_half2_isgreater_i32_i32(uint a, uint b );
short2 __builtin_spirv_intel_sub_group_half2_isgreaterequal_i32_i32(uint a, uint b );
short2 __builtin_spirv_intel_sub_group_half2_isless_i32_i32(uint a, uint b );
short2 __builtin_spirv_intel_sub_group_half2_islessequal_i32_i32(uint a, uint b );
#endif

char __builtin_spirv_OpenCL_s_min_i8_i8(char x, char y);
char2 __builtin_spirv_OpenCL_s_min_v2i8_v2i8(char2 x, char2 y);
char3 __builtin_spirv_OpenCL_s_min_v3i8_v3i8(char3 x, char3 y);
char4 __builtin_spirv_OpenCL_s_min_v4i8_v4i8(char4 x, char4 y);
char8 __builtin_spirv_OpenCL_s_min_v8i8_v8i8(char8 x, char8 y);
char16 __builtin_spirv_OpenCL_s_min_v16i8_v16i8(char16 x, char16 y);
uchar __builtin_spirv_OpenCL_u_min_i8_i8(uchar x, uchar y);
uchar2 __builtin_spirv_OpenCL_u_min_v2i8_v2i8(uchar2 x, uchar2 y);
uchar3 __builtin_spirv_OpenCL_u_min_v3i8_v3i8(uchar3 x, uchar3 y);
uchar4 __builtin_spirv_OpenCL_u_min_v4i8_v4i8(uchar4 x, uchar4 y);
uchar8 __builtin_spirv_OpenCL_u_min_v8i8_v8i8(uchar8 x, uchar8 y);
uchar16 __builtin_spirv_OpenCL_u_min_v16i8_v16i8(uchar16 x, uchar16 y);
short __builtin_spirv_OpenCL_s_min_i16_i16(short x, short y);
short2 __builtin_spirv_OpenCL_s_min_v2i16_v2i16(short2 x, short2 y);
short3 __builtin_spirv_OpenCL_s_min_v3i16_v3i16(short3 x, short3 y);
short4 __builtin_spirv_OpenCL_s_min_v4i16_v4i16(short4 x, short4 y);
short8 __builtin_spirv_OpenCL_s_min_v8i16_v8i16(short8 x, short8 y);
short16 __builtin_spirv_OpenCL_s_min_v16i16_v16i16(short16 x, short16 y);
ushort __builtin_spirv_OpenCL_u_min_i16_i16(ushort x, ushort y);
ushort2 __builtin_spirv_OpenCL_u_min_v2i16_v2i16(ushort2 x, ushort2 y);
ushort3 __builtin_spirv_OpenCL_u_min_v3i16_v3i16(ushort3 x, ushort3 y);
ushort4 __builtin_spirv_OpenCL_u_min_v4i16_v4i16(ushort4 x, ushort4 y);
ushort8 __builtin_spirv_OpenCL_u_min_v8i16_v8i16(ushort8 x, ushort8 y);
ushort16 __builtin_spirv_OpenCL_u_min_v16i16_v16i16(ushort16 x, ushort16 y);
int __builtin_spirv_OpenCL_s_min_i32_i32(int x, int y);
int2 __builtin_spirv_OpenCL_s_min_v2i32_v2i32(int2 x, int2 y);
int3 __builtin_spirv_OpenCL_s_min_v3i32_v3i32(int3 x, int3 y);
int4 __builtin_spirv_OpenCL_s_min_v4i32_v4i32(int4 x, int4 y);
int8 __builtin_spirv_OpenCL_s_min_v8i32_v8i32(int8 x, int8 y);
int16 __builtin_spirv_OpenCL_s_min_v16i32_v16i32(int16 x, int16 y);
uint __builtin_spirv_OpenCL_u_min_i32_i32(uint x, uint y);
uint2 __builtin_spirv_OpenCL_u_min_v2i32_v2i32(uint2 x, uint2 y);
uint3 __builtin_spirv_OpenCL_u_min_v3i32_v3i32(uint3 x, uint3 y);
uint4 __builtin_spirv_OpenCL_u_min_v4i32_v4i32(uint4 x, uint4 y);
uint8 __builtin_spirv_OpenCL_u_min_v8i32_v8i32(uint8 x, uint8 y);
uint16 __builtin_spirv_OpenCL_u_min_v16i32_v16i32(uint16 x, uint16 y);
long __builtin_spirv_OpenCL_s_min_i64_i64(long x, long y);
long2 __builtin_spirv_OpenCL_s_min_v2i64_v2i64(long2 x, long2 y);
long3 __builtin_spirv_OpenCL_s_min_v3i64_v3i64(long3 x, long3 y);
long4 __builtin_spirv_OpenCL_s_min_v4i64_v4i64(long4 x, long4 y);
long8 __builtin_spirv_OpenCL_s_min_v8i64_v8i64(long8 x, long8 y);
long16 __builtin_spirv_OpenCL_s_min_v16i64_v16i64(long16 x, long16 y);
ulong __builtin_spirv_OpenCL_u_min_i64_i64(ulong x, ulong y);
ulong2 __builtin_spirv_OpenCL_u_min_v2i64_v2i64(ulong2 x, ulong2 y);
ulong3 __builtin_spirv_OpenCL_u_min_v3i64_v3i64(ulong3 x, ulong3 y);
ulong4 __builtin_spirv_OpenCL_u_min_v4i64_v4i64(ulong4 x, ulong4 y);
ulong8 __builtin_spirv_OpenCL_u_min_v8i64_v8i64(ulong8 x, ulong8 y);
ulong16 __builtin_spirv_OpenCL_u_min_v16i64_v16i64(ulong16 x, ulong16 y);

// Misc Instructions

void __builtin_spirv_OpenCL_prefetch_p1i8_i32(const global uchar* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v2i8_i32(const global uchar2* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v3i8_i32(const global uchar3* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v4i8_i32(const global uchar4* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v8i8_i32(const global uchar8* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v16i8_i32(const global uchar16* p, uint num_elements);

void __builtin_spirv_OpenCL_prefetch_p1i16_i32(const global ushort* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v2i16_i32(const global ushort2* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v3i16_i32(const global ushort3* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v4i16_i32(const global ushort4* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v8i16_i32(const global ushort8* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v16i16_i32(const global ushort16* p, uint num_elements);

void __builtin_spirv_OpenCL_prefetch_p1i32_i32(const global uint* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v2i32_i32(const global uint2* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v3i32_i32(const global uint3* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v4i32_i32(const global uint4* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v8i32_i32(const global uint8* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v16i32_i32(const global uint16* p, uint num_elements);

void __builtin_spirv_OpenCL_prefetch_p1i64_i32(const global ulong* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v2i64_i32(const global ulong2* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v3i64_i32(const global ulong3* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v4i64_i32(const global ulong4* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v8i64_i32(const global ulong8* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v16i64_i32(const global ulong16* p, uint num_elements);

void __builtin_spirv_OpenCL_prefetch_p1f32_i32(const global float* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v2f32_i32(const global float2* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v3f32_i32(const global float3* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v4f32_i32(const global float4* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v8f32_i32(const global float8* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v16f32_i32(const global float16* p, uint num_elements);

void __builtin_spirv_OpenCL_prefetch_p1f16_i32(const global half* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v2f16_i32(const global half2* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v3f16_i32(const global half3* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v4f16_i32(const global half4* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v8f16_i32(const global half8* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v16f16_i32(const global half16* p, uint num_elements);

#if defined(cl_khr_fp64)
void __builtin_spirv_OpenCL_prefetch_p1f64_i32(const global double* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v2f64_i32(const global double2* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v3f64_i32(const global double3* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v4f64_i32(const global double4* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v8f64_i32(const global double8* p, uint num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v16f64_i32(const global double16* p, uint num_elements);
#endif // defined(cl_khr_fp64)

void __builtin_spirv_OpenCL_prefetch_p1i8_i64(const global uchar* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v2i8_i64(const global uchar2* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v3i8_i64(const global uchar3* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v4i8_i64(const global uchar4* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v8i8_i64(const global uchar8* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v16i8_i64(const global uchar16* p, ulong num_elements);

void __builtin_spirv_OpenCL_prefetch_p1i16_i64(const global ushort* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v2i16_i64(const global ushort2* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v3i16_i64(const global ushort3* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v4i16_i64(const global ushort4* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v8i16_i64(const global ushort8* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v16i16_i64(const global ushort16* p, ulong num_elements);

void __builtin_spirv_OpenCL_prefetch_p1i32_i64(const global uint* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v2i32_i64(const global uint2* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v3i32_i64(const global uint3* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v4i32_i64(const global uint4* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v8i32_i64(const global uint8* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v16i32_i64(const global uint16* p, ulong num_elements);

void __builtin_spirv_OpenCL_prefetch_p1i64_i64(const global ulong* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v2i64_i64(const global ulong2* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v3i64_i64(const global ulong3* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v4i64_i64(const global ulong4* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v8i64_i64(const global ulong8* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v16i64_i64(const global ulong16* p, ulong num_elements);

void __builtin_spirv_OpenCL_prefetch_p1f32_i64(const global float* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v2f32_i64(const global float2* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v3f32_i64(const global float3* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v4f32_i64(const global float4* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v8f32_i64(const global float8* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v16f32_i64(const global float16* p, ulong num_elements);

void __builtin_spirv_OpenCL_prefetch_p1f16_i64(const global half* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v2f16_i64(const global half2* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v3f16_i64(const global half3* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v4f16_i64(const global half4* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v8f16_i64(const global half8* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v16f16_i64(const global half16* p, ulong num_elements);

#if defined(cl_khr_fp64)
void __builtin_spirv_OpenCL_prefetch_p1f64_i64(const global double* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v2f64_i64(const global double2* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v3f64_i64(const global double3* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v4f64_i64(const global double4* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v8f64_i64(const global double8* p, ulong num_elements);
void __builtin_spirv_OpenCL_prefetch_p1v16f64_i64(const global double16* p, ulong num_elements);
#endif // defined(cl_khr_fp64)

uint  __builtin_spirv_OpReadClockKHR_i32_i32(uint scope);
ulong __builtin_spirv_OpReadClockKHR_i64_i32(uint scope);

#endif // __SPIRV_H__

