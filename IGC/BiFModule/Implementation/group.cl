/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Group Instructions

uint __intel_LocalInvocationIndex();
uint __intel_WorkgroupSize();
uint OVERLOADABLE __intel_LocalInvocationId(uint dim);

int OVERLOADABLE intel_sub_group_shuffle( int X, uint c );
long OVERLOADABLE intel_sub_group_shuffle( long X, uint c );
uint OVERLOADABLE intel_sub_group_shuffle( uint X, uint c );
ulong OVERLOADABLE intel_sub_group_shuffle( ulong X, uint c );
half OVERLOADABLE intel_sub_group_shuffle( half X, uint c );
float OVERLOADABLE intel_sub_group_shuffle( float X, uint c );
#if defined(cl_khr_fp64)
double OVERLOADABLE intel_sub_group_shuffle( double X, uint c );
#endif

int OVERLOADABLE intel_sub_group_shuffle_up( int identity, int X, uint c );
long OVERLOADABLE intel_sub_group_shuffle_up( long identity, long X, uint c );
uint OVERLOADABLE intel_sub_group_shuffle_up( uint identity, uint X, uint c );
ulong OVERLOADABLE intel_sub_group_shuffle_up( ulong identity, ulong X, uint c );
half OVERLOADABLE intel_sub_group_shuffle_up( half identity, half X, uint c );
float OVERLOADABLE intel_sub_group_shuffle_up( float identity, float X, uint c );
#if defined(cl_khr_fp64)
double OVERLOADABLE intel_sub_group_shuffle_up( double identity, double X, uint c );
#endif


#define ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, type)                         \
{                                                                                                    \
    if ( Stride == 0 )                                                                                \
    {                                                                                                \
        ASYNC_WORK_GROUP_COPY(Destination, Source, NumElements, Event, type)                        \
        return Event;                                                                                \
    }                                                                                                \
    else                                                                                            \
    {                                                                                                \
        ASYNC_WORK_GROUP_STRIDED_COPY_L2G(Destination, Source, NumElements, Stride, Event, type)    \
        return Event;                                                                                \
    }                                                                                                \
}

#define ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, type)                         \
{                                                                                                    \
    if ( Stride == 0 )                                                                                \
    {                                                                                                \
        ASYNC_WORK_GROUP_COPY(Destination, Source, NumElements, Event, type)                        \
        return Event;                                                                                \
    }                                                                                                \
    else                                                                                            \
    {                                                                                                \
        ASYNC_WORK_GROUP_STRIDED_COPY_G2L(Destination, Source, NumElements, Stride, Event, type)    \
        return Event;                                                                                \
    }                                                                                                \
}


//L2G

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i8_p3i8_i64_i64_i64, )(int Execution, global char *Destination, local char *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i8_p3i8_i32_i32_i64, )(int Execution, global char *Destination, local char *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i16_p3i16_i64_i64_i64, )(int Execution, global short *Destination, local short *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i16_p3i16_i32_i32_i64, )(int Execution, global short *Destination, local short *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i32_p3i32_i64_i64_i64, )(int Execution, global int *Destination, local int *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i32_p3i32_i32_i32_i64, )(int Execution, global int *Destination, local int *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i64_p3i64_i64_i64_i64, )(int Execution, global long *Destination, local long *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i64_p3i64_i32_i32_i64, )(int Execution, global long *Destination, local long *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1f16_p3f16_i64_i64_i64, )(int Execution, global half *Destination, local half *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1f16_p3f16_i32_i32_i64, )(int Execution, global half *Destination, local half *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1f32_p3f32_i64_i64_i64, )(int Execution, global float *Destination, local float *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1f32_p3f32_i32_i32_i64, )(int Execution, global float *Destination, local float *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

#if defined(cl_khr_fp64)

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1f64_p3f64_i64_i64_i64, )(int Execution, global double *Destination, local double *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1f64_p3f64_i32_i32_i64, )(int Execution, global double *Destination, local double *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

#endif

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i8_p3v2i8_i64_i64_i64, )(int Execution, global char2 *Destination, local char2 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i8_p3v2i8_i32_i32_i64, )(int Execution, global char2 *Destination, local char2 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i8_p3v3i8_i64_i64_i64, )(int Execution, global char3 *Destination, local char3 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i8_p3v3i8_i32_i32_i64, )(int Execution, global char3 *Destination, local char3 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i8_p3v4i8_i64_i64_i64, )(int Execution, global char4 *Destination, local char4 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i8_p3v4i8_i32_i32_i64, )(int Execution, global char4 *Destination, local char4 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i8_p3v8i8_i64_i64_i64, )(int Execution, global char8 *Destination, local char8 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i8_p3v8i8_i32_i32_i64, )(int Execution, global char8 *Destination, local char8 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i8_p3v16i8_i64_i64_i64, )(int Execution, global char16 *Destination, local char16 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i8_p3v16i8_i32_i32_i64, )(int Execution, global char16 *Destination, local char16 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i16_p3v2i16_i64_i64_i64, )(int Execution, global short2 *Destination, local short2 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i16_p3v2i16_i32_i32_i64, )(int Execution, global short2 *Destination, local short2 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i16_p3v3i16_i64_i64_i64, )(int Execution, global short3 *Destination, local short3 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i16_p3v3i16_i32_i32_i64, )(int Execution, global short3 *Destination, local short3 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i16_p3v4i16_i64_i64_i64, )(int Execution, global short4 *Destination, local short4 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i16_p3v4i16_i32_i32_i64, )(int Execution, global short4 *Destination, local short4 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i16_p3v8i16_i64_i64_i64, )(int Execution, global short8 *Destination, local short8 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i16_p3v8i16_i32_i32_i64, )(int Execution, global short8 *Destination, local short8 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i16_p3v16i16_i64_i64_i64, )(int Execution, global short16 *Destination, local short16 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i16_p3v16i16_i32_i32_i64, )(int Execution, global short16 *Destination, local short16 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i32_p3v2i32_i64_i64_i64, )(int Execution, global int2 *Destination, local int2 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i32_p3v2i32_i32_i32_i64, )(int Execution, global int2 *Destination, local int2 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i32_p3v3i32_i64_i64_i64, )(int Execution, global int3 *Destination, local int3 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i32_p3v3i32_i32_i32_i64, )(int Execution, global int3 *Destination, local int3 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i32_p3v4i32_i64_i64_i64, )(int Execution, global int4 *Destination, local int4 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i32_p3v4i32_i32_i32_i64, )(int Execution, global int4 *Destination, local int4 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i32_p3v8i32_i64_i64_i64, )(int Execution, global int8 *Destination, local int8 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i32_p3v8i32_i32_i32_i64, )(int Execution, global int8 *Destination, local int8 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i32_p3v16i32_i64_i64_i64, )(int Execution, global int16 *Destination, local int16 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i32_p3v16i32_i32_i32_i64, )(int Execution, global int16 *Destination, local int16 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i64_p3v2i64_i64_i64_i64, )(int Execution, global long2 *Destination, local long2 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i64_p3v2i64_i32_i32_i64, )(int Execution, global long2 *Destination, local long2 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i64_p3v3i64_i64_i64_i64, )(int Execution, global long3 *Destination, local long3 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i64_p3v3i64_i32_i32_i64, )(int Execution, global long3 *Destination, local long3 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i64_p3v4i64_i64_i64_i64, )(int Execution, global long4 *Destination, local long4 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i64_p3v4i64_i32_i32_i64, )(int Execution, global long4 *Destination, local long4 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i64_p3v8i64_i64_i64_i64, )(int Execution, global long8 *Destination, local long8 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i64_p3v8i64_i32_i32_i64, )(int Execution, global long8 *Destination, local long8 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i64_p3v16i64_i64_i64_i64, )(int Execution, global long16 *Destination, local long16 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i64_p3v16i64_i32_i32_i64, )(int Execution, global long16 *Destination, local long16 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2f16_p3v2f16_i64_i64_i64, )(int Execution, global half2 *Destination, local half2 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2f16_p3v2f16_i32_i32_i64, )(int Execution, global half2 *Destination, local half2 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3f16_p3v3f16_i64_i64_i64, )(int Execution, global half3 *Destination, local half3 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3f16_p3v3f16_i32_i32_i64, )(int Execution, global half3 *Destination, local half3 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4f16_p3v4f16_i64_i64_i64, )(int Execution, global half4 *Destination, local half4 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4f16_p3v4f16_i32_i32_i64, )(int Execution, global half4 *Destination, local half4 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8f16_p3v8f16_i64_i64_i64, )(int Execution, global half8 *Destination, local half8 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8f16_p3v8f16_i32_i32_i64, )(int Execution, global half8 *Destination, local half8 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16f16_p3v16f16_i64_i64_i64, )(int Execution, global half16 *Destination, local half16 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16f16_p3v16f16_i32_i32_i64, )(int Execution, global half16 *Destination, local half16 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2f32_p3v2f32_i64_i64_i64, )(int Execution, global float2 *Destination, local float2 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2f32_p3v2f32_i32_i32_i64, )(int Execution, global float2 *Destination, local float2 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3f32_p3v3f32_i64_i64_i64, )(int Execution, global float3 *Destination, local float3 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3f32_p3v3f32_i32_i32_i64, )(int Execution, global float3 *Destination, local float3 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4f32_p3v4f32_i64_i64_i64, )(int Execution, global float4 *Destination, local float4 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4f32_p3v4f32_i32_i32_i64, )(int Execution, global float4 *Destination, local float4 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8f32_p3v8f32_i64_i64_i64, )(int Execution, global float8 *Destination, local float8 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8f32_p3v8f32_i32_i32_i64, )(int Execution, global float8 *Destination, local float8 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16f32_p3v16f32_i64_i64_i64, )(int Execution, global float16 *Destination, local float16 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16f32_p3v16f32_i32_i32_i64, )(int Execution, global float16 *Destination, local float16 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

#if defined(cl_khr_fp64)

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2f64_p3v2f64_i64_i64_i64, )(int Execution, global double2 *Destination, local double2 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2f64_p3v2f64_i32_i32_i64, )(int Execution, global double2 *Destination, local double2 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3f64_p3v3f64_i64_i64_i64, )(int Execution, global double3 *Destination, local double3 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3f64_p3v3f64_i32_i32_i64, )(int Execution, global double3 *Destination, local double3 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4f64_p3v4f64_i64_i64_i64, )(int Execution, global double4 *Destination, local double4 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4f64_p3v4f64_i32_i32_i64, )(int Execution, global double4 *Destination, local double4 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8f64_p3v8f64_i64_i64_i64, )(int Execution, global double8 *Destination, local double8 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8f64_p3v8f64_i32_i32_i64, )(int Execution, global double8 *Destination, local double8 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16f64_p3v16f64_i64_i64_i64, )(int Execution, global double16 *Destination, local double16 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16f64_p3v16f64_i32_i32_i64, )(int Execution, global double16 *Destination, local double16 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

#endif //defined(cl_khr_fp64)

//G2L


__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i8_p1i8_i64_i64_i64, )(int Execution, local char *Destination, global char *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i8_p1i8_i32_i32_i64, )(int Execution, local char *Destination, global char *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i16_p1i16_i64_i64_i64, )(int Execution, local short *Destination, global short *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i16_p1i16_i32_i32_i64, )(int Execution, local short *Destination, global short *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i32_p1i32_i64_i64_i64, )(int Execution, local int *Destination, global int *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i32_p1i32_i32_i32_i64, )(int Execution, local int *Destination, global int *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i64_p1i64_i64_i64_i64, )(int Execution, local long *Destination, global long *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i64_p1i64_i32_i32_i64, )(int Execution, local long *Destination, global long *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3f16_p1f16_i64_i64_i64, )(int Execution, local half *Destination, global half *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3f16_p1f16_i32_i32_i64, )(int Execution, local half *Destination, global half *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3f32_p1f32_i64_i64_i64, )(int Execution, local float *Destination, global float *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3f32_p1f32_i32_i32_i64, )(int Execution, local float *Destination, global float *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

#if defined(cl_khr_fp64)

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3f64_p1f64_i64_i64_i64, )(int Execution, local double *Destination, global double *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3f64_p1f64_i32_i32_i64, )(int Execution, local double *Destination, global double *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

#endif

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i8_p1v2i8_i64_i64_i64, )(int Execution, local char2 *Destination, global char2 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i8_p1v2i8_i32_i32_i64, )(int Execution, local char2 *Destination, global char2 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i8_p1v3i8_i64_i64_i64, )(int Execution, local char3 *Destination, global char3 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i8_p1v3i8_i32_i32_i64, )(int Execution, local char3 *Destination, global char3 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i8_p1v4i8_i64_i64_i64, )(int Execution, local char4 *Destination, global char4 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i8_p1v4i8_i32_i32_i64, )(int Execution, local char4 *Destination, global char4 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i8_p1v8i8_i64_i64_i64, )(int Execution, local char8 *Destination, global char8 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i8_p1v8i8_i32_i32_i64, )(int Execution, local char8 *Destination, global char8 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i8_p1v16i8_i64_i64_i64, )(int Execution, local char16 *Destination, global char16 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i8_p1v16i8_i32_i32_i64, )(int Execution, local char16 *Destination, global char16 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i16_p1v2i16_i64_i64_i64, )(int Execution, local short2 *Destination, global short2 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i16_p1v2i16_i32_i32_i64, )(int Execution, local short2 *Destination, global short2 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i16_p1v3i16_i64_i64_i64, )(int Execution, local short3 *Destination, global short3 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i16_p1v3i16_i32_i32_i64, )(int Execution, local short3 *Destination, global short3 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i16_p1v4i16_i64_i64_i64, )(int Execution, local short4 *Destination, global short4 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i16_p1v4i16_i32_i32_i64, )(int Execution, local short4 *Destination, global short4 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i16_p1v8i16_i64_i64_i64, )(int Execution, local short8 *Destination, global short8 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i16_p1v8i16_i32_i32_i64, )(int Execution, local short8 *Destination, global short8 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i16_p1v16i16_i64_i64_i64, )(int Execution, local short16 *Destination, global short16 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i16_p1v16i16_i32_i32_i64, )(int Execution, local short16 *Destination, global short16 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i32_p1v2i32_i64_i64_i64, )(int Execution, local int2 *Destination, global int2 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i32_p1v2i32_i32_i32_i64, )(int Execution, local int2 *Destination, global int2 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i32_p1v3i32_i64_i64_i64, )(int Execution, local int3 *Destination, global int3 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i32_p1v3i32_i32_i32_i64, )(int Execution, local int3 *Destination, global int3 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i32_p1v4i32_i64_i64_i64, )(int Execution, local int4 *Destination, global int4 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i32_p1v4i32_i32_i32_i64, )(int Execution, local int4 *Destination, global int4 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i32_p1v8i32_i64_i64_i64, )(int Execution, local int8 *Destination, global int8 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i32_p1v8i32_i32_i32_i64, )(int Execution, local int8 *Destination, global int8 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i32_p1v16i32_i64_i64_i64, )(int Execution, local int16 *Destination, global int16 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i32_p1v16i32_i32_i32_i64, )(int Execution, local int16 *Destination, global int16 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i64_p1v2i64_i64_i64_i64, )(int Execution, local long2 *Destination, global long2 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i64_p1v2i64_i32_i32_i64, )(int Execution, local long2 *Destination, global long2 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i64_p1v3i64_i64_i64_i64, )(int Execution, local long3 *Destination, global long3 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i64_p1v3i64_i32_i32_i64, )(int Execution, local long3 *Destination, global long3 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i64_p1v4i64_i64_i64_i64, )(int Execution, local long4 *Destination, global long4 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i64_p1v4i64_i32_i32_i64, )(int Execution, local long4 *Destination, global long4 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i64_p1v8i64_i64_i64_i64, )(int Execution, local long8 *Destination, global long8 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i64_p1v8i64_i32_i32_i64, )(int Execution, local long8 *Destination, global long8 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i64_p1v16i64_i64_i64_i64, )(int Execution, local long16 *Destination, global long16 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i64_p1v16i64_i32_i32_i64, )(int Execution, local long16 *Destination, global long16 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2f16_p1v2f16_i64_i64_i64, )(int Execution, local half2 *Destination, global half2 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2f16_p1v2f16_i32_i32_i64, )(int Execution, local half2 *Destination, global half2 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3f16_p1v3f16_i64_i64_i64, )(int Execution, local half3 *Destination, global half3 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3f16_p1v3f16_i32_i32_i64, )(int Execution, local half3 *Destination, global half3 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4f16_p1v4f16_i64_i64_i64, )(int Execution, local half4 *Destination, global half4 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4f16_p1v4f16_i32_i32_i64, )(int Execution, local half4 *Destination, global half4 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8f16_p1v8f16_i64_i64_i64, )(int Execution, local half8 *Destination, global half8 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8f16_p1v8f16_i32_i32_i64, )(int Execution, local half8 *Destination, global half8 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16f16_p1v16f16_i64_i64_i64, )(int Execution, local half16 *Destination, global half16 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16f16_p1v16f16_i32_i32_i64, )(int Execution, local half16 *Destination, global half16 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2f32_p1v2f32_i64_i64_i64, )(int Execution, local float2 *Destination, global float2 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2f32_p1v2f32_i32_i32_i64, )(int Execution, local float2 *Destination, global float2 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3f32_p1v3f32_i64_i64_i64, )(int Execution, local float3 *Destination, global float3 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3f32_p1v3f32_i32_i32_i64, )(int Execution, local float3 *Destination, global float3 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4f32_p1v4f32_i64_i64_i64, )(int Execution, local float4 *Destination, global float4 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4f32_p1v4f32_i32_i32_i64, )(int Execution, local float4 *Destination, global float4 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8f32_p1v8f32_i64_i64_i64, )(int Execution, local float8 *Destination, global float8 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8f32_p1v8f32_i32_i32_i64, )(int Execution, local float8 *Destination, global float8 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16f32_p1v16f32_i64_i64_i64, )(int Execution, local float16 *Destination, global float16 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16f32_p1v16f32_i32_i32_i64, )(int Execution, local float16 *Destination, global float16 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

#if defined(cl_khr_fp64)

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2f64_p1v2f64_i64_i64_i64, )(int Execution, local double2 *Destination, global double2 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2f64_p1v2f64_i32_i32_i64, )(int Execution, local double2 *Destination, global double2 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3f64_p1v3f64_i64_i64_i64, )(int Execution, local double3 *Destination, global double3 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3f64_p1v3f64_i32_i32_i64, )(int Execution, local double3 *Destination, global double3 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4f64_p1v4f64_i64_i64_i64, )(int Execution, local double4 *Destination, global double4 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4f64_p1v4f64_i32_i32_i64, )(int Execution, local double4 *Destination, global double4 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8f64_p1v8f64_i64_i64_i64, )(int Execution, local double8 *Destination, global double8 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8f64_p1v8f64_i32_i32_i64, )(int Execution, local double8 *Destination, global double8 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16f64_p1v16f64_i64_i64_i64, )(int Execution, local double16 *Destination, global double16 *Source, long NumElements, long Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

__spirv_Event SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16f64_p1v16f64_i32_i32_i64, )(int Execution, local double16 *Destination, global double16 *Source, int NumElements, int Stride, __spirv_Event Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

#endif // defined(cl_khr_fp64)


void SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupWaitEvents, _i32_i32_p0i64, )(int Execution, int NumEvents, private __spirv_Event *EventsList)
{
    if (Execution == Workgroup)
    {
        SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Execution,0, AcquireRelease | CrossWorkgroupMemory);
        SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Execution,0, AcquireRelease | WorkgroupMemory );
    }
    else if (Execution == Subgroup)
    {
        // This is a no op for now
    }

}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupWaitEvents, _i32_i32_p4i64, )(int Execution, int NumEvents, generic __spirv_Event *EventsList)
{
    if (Execution == Workgroup)
    {
        SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Execution,0, AcquireRelease | CrossWorkgroupMemory);
        SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Execution,0, AcquireRelease | WorkgroupMemory);
    }
    else if (Execution == Subgroup)
    {
        // This is a no op for now
    }
}
#endif __OPENCL_C_VERSION__ >= CL_VERSION_2_0

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAll, _i32_i1, )(int Execution, bool Predicate)
{
    if (Execution == Workgroup)
    {
        // if wg-size is equal to sg-size, don't bother SLM, just do it using subgroups
        if(SPIRV_BUILTIN_NO_OP(BuiltInNumSubgroups, , )() == 1)
            return SPIRV_BUILTIN(GroupUMin, _i32_i32_i32, )(Subgroup, GroupOperationReduce, (uint)(Predicate) );

        GET_SAFE_MEMPOOL_PTR(tmp, int, false, 1)
        *tmp = 1;
        SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Execution, 0, AcquireRelease | WorkgroupMemory); // Wait for tmp to be initialized
        if(Predicate == 0)
            *tmp = 0; // intentional data race here, as we do not care for the value itself, rather than the fact it was overriden
        SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Execution, 0, AcquireRelease | WorkgroupMemory); // Wait for threads
        return *tmp; // Return true if none of them failed the test
    }
    else
    {
        return SPIRV_BUILTIN(GroupUMin, _i32_i32_i32, )(Subgroup, GroupOperationReduce, (uint)Predicate );
    }
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAny, _i32_i1, )(int Execution, bool Predicate)
{
    if (Execution == Workgroup)
    {
        // if wg-size is equal to sg-size, don't bother SLM, just do it using subgroups
        if(SPIRV_BUILTIN_NO_OP(BuiltInNumSubgroups, , )() == 1)
            return SPIRV_BUILTIN(GroupUMax, _i32_i32_i32, )(Subgroup, GroupOperationReduce, (uint)Predicate );

        GET_SAFE_MEMPOOL_PTR(tmp, int, false, 1)
        *tmp = 0;
        SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Execution, 0, AcquireRelease | WorkgroupMemory); // Wait for tmp to be initialized
        if(Predicate == 1)
            *tmp = 1; // intentional data race here, as we do not care for the value itself, rather than the fact it was overriden
        SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Execution, 0, AcquireRelease | WorkgroupMemory);
        return *tmp; // Return true if any of them passed the test
    }
    else
    {
        return SPIRV_BUILTIN(GroupUMax, _i32_i32_i32, )(Subgroup, GroupOperationReduce, (uint)Predicate );
    }
}

#if defined(cl_khr_subgroup_non_uniform_vote)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformElect, _i32, )(int Execution)
{
    if (Execution == Subgroup)
    {
        uint activeChannels = __builtin_IB_WaveBallot(true);
        uint firstActive = SPIRV_OCL_BUILTIN(ctz, _i32, )(as_int(activeChannels));
        if (__builtin_IB_get_simd_id() == firstActive)
            return true;
        else
            return false;
    }
    else
    {
        return false;
    }
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformAll, _i32_i1, )(int Execution, bool Predicate)
{
    if(Execution == Subgroup)
        return SPIRV_BUILTIN(GroupAll, _i32_i1, )(Execution, Predicate);
    else
        return false;
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformAny, _i32_i1, )(int Execution, bool Predicate)
{
    if (Execution == Subgroup)
        return SPIRV_BUILTIN(GroupAny, _i32_i1, )(Execution, Predicate);
    else
        return false;
}

#define DEFN_NON_UNIFORM_ALL_EQUAL(TYPE, TYPE_ABBR)                                                                           \
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformAllEqual, _i32_##TYPE_ABBR, )(int Execution, TYPE Value)                 \
{                                                                                                                             \
    if (Execution == Subgroup)                                                                                                \
    {                                                                                                                         \
        uint activeChannels = __builtin_IB_WaveBallot(true);                                                                  \
        uint firstActive = SPIRV_OCL_BUILTIN(ctz, _i32, )(as_int(activeChannels));                                            \
                                                                                                                              \
        TYPE firstLaneValue = SPIRV_BUILTIN(GroupBroadcast, _i32_##TYPE_ABBR##_i32, )(Execution, Value, as_int(firstActive)); \
        bool isSame = firstLaneValue == Value;                                                                                \
                                                                                                                              \
        uint4 equalChannels = SPIRV_BUILTIN(GroupNonUniformBallot, _i32_i1, )(Execution, isSame);                             \
                                                                                                                              \
        if (equalChannels.x == activeChannels)                                                                                \
            return true;                                                                                                      \
        else                                                                                                                  \
            return false;                                                                                                     \
    }                                                                                                                         \
    else                                                                                                                      \
        return false;                                                                                                         \
}

DEFN_NON_UNIFORM_ALL_EQUAL(char,   i8)
DEFN_NON_UNIFORM_ALL_EQUAL(short,  i16)
DEFN_NON_UNIFORM_ALL_EQUAL(int,    i32)
DEFN_NON_UNIFORM_ALL_EQUAL(long,   i64)
DEFN_NON_UNIFORM_ALL_EQUAL(float,  f32)
#if defined(cl_khr_fp64)
DEFN_NON_UNIFORM_ALL_EQUAL(double, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_ALL_EQUAL(half,   f16)
#endif // defined(cl_khr_fp16)
#endif // defined(cl_khr_subgroup_non_uniform_vote)

//Broadcast Functions

// ***Note that technically the spec allows for 64 bit local Id's but currently our hardware cannot support
// this in which case we cast all ulong local Id's to uint's at this time.


#define BROADCAST_WORKGROUP(type)                                                       \
{                                                                                       \
    GET_SAFE_MEMPOOL_PTR(tmp, type, false, 1)                                           \
    if( (__intel_LocalInvocationId(0) == LocalId.s0) &                                            \
        (__intel_LocalInvocationId(1) == LocalId.s1) &                                            \
        (__intel_LocalInvocationId(2) == LocalId.s2) )                                            \
    {                                                                                   \
        *tmp = Value;                                                                   \
    }                                                                                   \
    SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Execution, 0, AcquireRelease | WorkgroupMemory);        \
    type ret = *tmp;                                                                    \
    return ret;                                                                         \
}

#define DEFN_SUB_GROUP_BROADCAST_VEC(__vargtype, __abbrvargtype)                                                            \
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupBroadcast, __vargtype, int, __vargtype, int3, i32, __abbrvargtype, v3i32)    \
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupBroadcast, __vargtype, int, __vargtype, long3, i32, __abbrvargtype, v3i64)   \
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupBroadcast, __vargtype, int, __vargtype, int2, i32, __abbrvargtype, v2i32)    \
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupBroadcast, __vargtype, int, __vargtype, long2, i32, __abbrvargtype, v2i64)   \
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupBroadcast, __vargtype, int, __vargtype, int, i32, __abbrvargtype, i32)       \
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupBroadcast, __vargtype, int, __vargtype, long, i32, __abbrvargtype, i64)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i1_v3i32, )(int Execution, bool Value, int3 LocalId)
{
    if (Execution == Workgroup)
    {
        BROADCAST_WORKGROUP(bool)
    }
    else if (Execution == Subgroup)
    {
        return __builtin_IB_simd_broadcast_b(Value, LocalId.s0);
    }
    else
    {
        return false;
    }
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i1_v3i64, )(int Execution, bool Value, long3 LocalId)
{
    if (Execution == Workgroup)
    {
        BROADCAST_WORKGROUP(bool)
    }
    else if (Execution == Subgroup)
    {
        return __builtin_IB_simd_broadcast_b(Value, (uint)LocalId.s0);
    }
    else
    {
        return false;
    }
}

char SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i8_v3i32, )(int Execution, char Value, int3 LocalId)
{
    if (Execution == Workgroup)
    {
        BROADCAST_WORKGROUP(uchar)
    }
    else if (Execution == Subgroup)
    {
        return __builtin_IB_simd_broadcast_c(Value, LocalId.s0);
    }
    else
    {
        return 0;
    }
}

char SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i8_v3i64, )(int Execution, char Value, long3 LocalId)
{
    if (Execution == Workgroup)
    {
        BROADCAST_WORKGROUP(uchar)
    }
    else if (Execution == Subgroup)
    {
        return __builtin_IB_simd_broadcast_c(Value, (uint)LocalId.s0);
    }
    else
    {
        return 0;
    }
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i16_v3i32, )(int Execution, short Value, int3 LocalId)
{
    if (Execution == Workgroup)
    {
        BROADCAST_WORKGROUP(ushort)
    }
    else if (Execution == Subgroup)
    {
        return as_ushort(__builtin_IB_simd_broadcast_h(as_half(Value), LocalId.s0));
    }
    else
    {
        return 0;
    }
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i16_v3i64, )(int Execution, short Value, long3 LocalId)
{
    if (Execution == Workgroup)
    {
        BROADCAST_WORKGROUP(ushort)
    }
    else if (Execution == Subgroup)
    {
        return as_ushort(__builtin_IB_simd_broadcast_h(as_half(Value), (uint)LocalId.s0));
    }
    else
    {
        return 0;
    }
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i32_v3i32, )(int Execution, int Value, int3 LocalId)
{
    if (Execution == Workgroup)
    {
        BROADCAST_WORKGROUP(uint)
    }
    else if (Execution == Subgroup)
    {
        return __builtin_IB_simd_broadcast(Value, LocalId.s0);
    }
    else
    {
        return 0;
    }
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i32_v3i64, )(int Execution, int Value, long3 LocalId)
{
    if (Execution == Workgroup)
    {
        BROADCAST_WORKGROUP(uint)
    }
    else if (Execution == Subgroup)
    {
        return __builtin_IB_simd_broadcast(Value, (uint)LocalId.s0);
    }
    else
    {
        return 0;
    }
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i64_v3i32, )(int Execution, long Value, int3 LocalId)
{
    if (Execution == Workgroup)
    {
        BROADCAST_WORKGROUP(ulong)
    }
    else if (Execution == Subgroup)
    {
        return ((((ulong)__builtin_IB_simd_broadcast(Value >> 32, LocalId.s0)) << 32 ) | __builtin_IB_simd_broadcast((uint)Value, LocalId.s0));
    }
    else
    {
        return 0;
    }
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i64_v3i64, )(int Execution, long Value, long3 LocalId)
{
    if (Execution == Workgroup)
    {
        BROADCAST_WORKGROUP(ulong)
    }
    else if (Execution == Subgroup)
    {
        return ((((ulong)__builtin_IB_simd_broadcast(Value >> 32, (uint)LocalId.s0)) << 32 ) | __builtin_IB_simd_broadcast((uint)Value, (uint)LocalId.s0));
    }
    else
    {
        return 0;
    }
}

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_f16_v3i32, )(int Execution, half Value, int3 LocalId)
{
    if (Execution == Workgroup)
    {
        BROADCAST_WORKGROUP(half)
    }
    else if (Execution == Subgroup)
    {
        return __builtin_IB_simd_broadcast_h( Value, (uint)LocalId.s0 );
    }
    else
    {
        return 0;
    }
}

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_f16_v3i64, )(int Execution, half Value, long3 LocalId)
{
    if (Execution == Workgroup)
    {
        BROADCAST_WORKGROUP(half)
    }
    else if (Execution == Subgroup)
    {
        return as_half2(__builtin_IB_simd_broadcast( (uint)(as_short(Value)), (uint)LocalId.s0 )).x;
    }
    else
    {
        return 0;
    }
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_f32_v3i32, )(int Execution, float Value, int3 LocalId)
{
    if (Execution == Workgroup)
    {
        BROADCAST_WORKGROUP(float)
    }
    else if (Execution == Subgroup)
    {
        return __builtin_IB_simd_broadcast_f( Value, LocalId.s0 );
    }
    else
    {
        return 0;
    }
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_f32_v3i64, )(int Execution, float Value, long3 LocalId)
{
    if (Execution == Workgroup)
    {
        BROADCAST_WORKGROUP(float)
    }
    else if (Execution == Subgroup)
    {
        return __builtin_IB_simd_broadcast_f( Value, (uint)LocalId.s0 );
    }
    else
    {
        return 0;
    }
}

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_f64_v3i32, )(int Execution, double Value, int3 LocalId)
{
    if (Execution == Workgroup)
    {
        BROADCAST_WORKGROUP(double)
    }
    else if (Execution == Subgroup)
    {
        return __builtin_IB_simd_broadcast_df( Value, LocalId.s0 );
    }
    else
    {
        return 0;
    }
}


double SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_f64_v3i64, )(int Execution, double Value, long3 LocalId)
{
    if (Execution == Workgroup)
    {
        BROADCAST_WORKGROUP(double)
    }
    else if (Execution == Subgroup)
    {
        return __builtin_IB_simd_broadcast_df( Value, (uint) LocalId.s0 );
    }
    else
    {
        return 0;
    }
}

#endif // defined(cl_khr_fp64)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i1_v2i32, )(int Execution, bool Value, int2 LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i1_v3i32, )(Execution, Value, (int3)(LocalId.s0, LocalId.s1, 0));
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i1_v2i64, )(int Execution, bool Value, long2 LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i1_v3i64, )(Execution, Value, (long3)(LocalId.s0, LocalId.s1, 0));
}

char SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i8_v2i32, )(int Execution, char Value, int2 LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i8_v3i32, )(Execution,Value,(int3)(LocalId.s0,LocalId.s1,0));
}

char SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i8_v2i64, )(int Execution, char Value, long2 LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i8_v3i64, )(Execution,Value,(long3)(LocalId.s0,LocalId.s1,0));
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i16_v2i32, )(int Execution, short Value, int2 LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i16_v3i32, )(Execution,Value,(int3)(LocalId.s0,LocalId.s1,0));
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i16_v2i64, )(int Execution, short Value, long2 LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i16_v3i64, )(Execution,Value,(long3)(LocalId.s0,LocalId.s1,0));
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i32_v2i32, )(int Execution, int Value, int2 LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i32_v3i32, )(Execution,Value,(int3)(LocalId.s0,LocalId.s1,0));
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i32_v2i64, )(int Execution, int Value, long2 LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i32_v3i64, )(Execution,Value,(long3)(LocalId.s0,LocalId.s1,0));
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i64_v2i32, )(int Execution, long Value, int2 LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i64_v3i32, )(Execution,Value,(int3)(LocalId.s0,LocalId.s1,0));
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i64_v2i64, )(int Execution, long Value, long2 LocalId)
{
   return SPIRV_BUILTIN(GroupBroadcast, _i32_i64_v3i64, )(Execution,Value,(long3)(LocalId.s0,LocalId.s1,0));
}

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_f16_v2i32, )(int Execution, half Value, int2 LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_f16_v3i32, )(Execution,Value,(int3)(LocalId.s0,LocalId.s1,0));
}

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_f16_v2i64, )(int Execution, half Value, long2 LocalId)
{
   return SPIRV_BUILTIN(GroupBroadcast, _i32_f16_v3i64, )(Execution,Value,(long3)(LocalId.s0,LocalId.s1,0));
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_f32_v2i32, )(int Execution, float Value, int2 LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_f32_v3i32, )(Execution,Value,(int3)(LocalId.s0,LocalId.s1,0));
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_f32_v2i64, )(int Execution, float Value, long2 LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_f32_v3i64, )(Execution,Value,(long3)(LocalId.s0,LocalId.s1,0));
}

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_f64_v2i32, )(int Execution, double Value, int2 LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_f64_v3i32, )(Execution,Value,(int3)(LocalId.s0,LocalId.s1,0));
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_f64_v2i64, )(int Execution, double Value, long2 LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_f64_v3i64, )(Execution,Value,(long3)(LocalId.s0,LocalId.s1,0));
}

#endif

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i1_i32, )(int Execution, bool Value, int LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i1_v3i32, )(Execution, Value, (int3)(LocalId, 0, 0));
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i1_i64, )(int Execution, bool Value, long LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i1_v3i64, )(Execution, Value, (long3)(LocalId, 0, 0));
}

char SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i8_i32, )(int Execution, char Value, int LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i8_v3i32, )(Execution,Value,(int3)(LocalId,0,0));
}

char SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i8_i64, )(int Execution, char Value, long LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i8_v3i64, )(Execution,Value,(long3)(LocalId,0,0));
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i16_i32, )(int Execution, short Value, int LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i16_v3i32, )(Execution,Value,(int3)(LocalId,0,0));
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i16_i64, )(int Execution, short Value, long LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i16_v3i64, )(Execution,Value,(long3)(LocalId,0,0));
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i32_i32, )(int Execution, int Value, int LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i32_v3i32, )(Execution,Value,(int3)(LocalId,0,0));
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i32_i64, )(int Execution, int Value, long LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i32_v3i64, )(Execution,Value,(long3)(LocalId,0,0));
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i64_i32, )(int Execution, long Value, int LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i64_v3i32, )(Execution,Value,(int3)(LocalId,0,0));
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_i64_i64, )(int Execution, long Value, long LocalId)
{
   return SPIRV_BUILTIN(GroupBroadcast, _i32_i64_v3i64, )(Execution,Value,(long3)(LocalId,0,0));
}

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_f16_i32, )(int Execution, half Value, int LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_f16_v3i32, )(Execution,Value,(int3)(LocalId,0,0));
}

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_f16_i64, )(int Execution, half Value, long LocalId)
{
   return SPIRV_BUILTIN(GroupBroadcast, _i32_f16_v3i64, )(Execution,Value,(long3)(LocalId,0,0));
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_f32_i32, )(int Execution, float Value, int LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_f32_v3i32, )(Execution,Value,(int3)(LocalId,0,0));
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_f32_i64, )(int Execution, float Value, long LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_f32_v3i64, )(Execution,Value,(long3)(LocalId,0,0));
}

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_f64_i32, )(int Execution, double Value, int LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_f64_v3i32, )(Execution,Value,(int3)(LocalId,0,0));
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupBroadcast, _i32_f64_i64, )(int Execution, double Value, long LocalId)
{
    return SPIRV_BUILTIN(GroupBroadcast, _i32_f64_v3i64, )(Execution,Value,(long3)(LocalId,0,0));
}

#endif

DEFN_SUB_GROUP_BROADCAST_VEC(char,   i8)
DEFN_SUB_GROUP_BROADCAST_VEC(short,  i16)
DEFN_SUB_GROUP_BROADCAST_VEC(int,    i32)
DEFN_SUB_GROUP_BROADCAST_VEC(long,   i64)
DEFN_SUB_GROUP_BROADCAST_VEC(float,  f32)
#if defined(cl_khr_fp16)
DEFN_SUB_GROUP_BROADCAST_VEC(half,   f16)
#endif // defined(cl_khr_fp16)
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_BROADCAST_VEC(double, f64)
#endif // defined(cl_khr_fp64)

// OpSubgroupShuffleINTEL
//
// Allows data to be arbitrarily transferred between invocations in a subgroup.
// The data that is returned for this invocation is the value of 'Data' for the invocation identified by 'InvocationId'.
char SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleINTEL, _i8_i32, )(char Data, uint InvocationId)
{
    if(BIF_FLAG_CTRL_GET(UseOOBChecks))
    {
        InvocationId = InvocationId & (SPIRV_BUILTIN_NO_OP(BuiltInSubgroupMaxSize, , )() - 1);
    }
    return __builtin_IB_simd_shuffle_c(Data, InvocationId);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleINTEL, _i16_i32, )(short Data, uint InvocationId)
{
    if(BIF_FLAG_CTRL_GET(UseOOBChecks))
    {
        InvocationId = InvocationId & (SPIRV_BUILTIN_NO_OP(BuiltInSubgroupMaxSize, , )() - 1);
    }
    return __builtin_IB_simd_shuffle_us(as_ushort(Data), InvocationId);
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleINTEL, _i32_i32, )(int Data, uint InvocationId)
{
    if(BIF_FLAG_CTRL_GET(UseOOBChecks))
    {
        InvocationId = InvocationId & (SPIRV_BUILTIN_NO_OP(BuiltInSubgroupMaxSize, , )() - 1);
    }
    return __builtin_IB_simd_shuffle(as_uint(Data), InvocationId);
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleINTEL, _i64_i32, )(long Data, uint InvocationId)
{
    int2 DataXY = as_int2(Data);
    int2 Result;
    Result.s0 = SPIRV_BUILTIN(SubgroupShuffleINTEL, _i32_i32, )(DataXY.s0, InvocationId);
    Result.s1 = SPIRV_BUILTIN(SubgroupShuffleINTEL, _i32_i32, )(DataXY.s1, InvocationId);
    return as_long(Result);
}

#ifdef cl_khr_fp16
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleINTEL, _f16_i32, )(half Data, uint InvocationId)
{
    if(BIF_FLAG_CTRL_GET(UseOOBChecks))
    {
        InvocationId = InvocationId & (SPIRV_BUILTIN_NO_OP(BuiltInSubgroupMaxSize, , )() - 1);
    }
    return __builtin_IB_simd_shuffle_h(Data, InvocationId);
}
#endif // cl_khr_fp16

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleINTEL, _f32_i32, )(float Data, uint InvocationId)
{
    if(BIF_FLAG_CTRL_GET(UseOOBChecks))
    {
        InvocationId = InvocationId & (SPIRV_BUILTIN_NO_OP(BuiltInSubgroupMaxSize, , )() - 1);
    }
    return __builtin_IB_simd_shuffle_f(Data, InvocationId);
}

#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleINTEL, _f64_i32, )(double Data, uint InvocationId)
{
    if(BIF_FLAG_CTRL_GET(UseOOBChecks))
    {
        InvocationId = InvocationId & (SPIRV_BUILTIN_NO_OP(BuiltInSubgroupMaxSize, , )() - 1);
    }
    return __builtin_IB_simd_shuffle_df(Data, InvocationId);
}
#endif // cl_khr_fp64

GENERATE_SPIRV_VECTOR_FUNCTIONS_2ARGS_VS(SubgroupShuffleINTEL, char,  char,  uint, i8,  i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_2ARGS_VS(SubgroupShuffleINTEL, short, short, uint, i16, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_2ARGS_VS(SubgroupShuffleINTEL, int,   int,   uint, i32, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_2ARGS_VS(SubgroupShuffleINTEL, float, float, uint, f32, i32)

// OpSubgroupShuffleDownINTEL
//
// Allows data to be transferred from an invocation in the subgroup with a higher SubgroupLocalInvocationId down to
// a invocation in the subgroup with a lower SubgroupLocalInvocationId.
char SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _i8_i8_i32, )(char Current, char Next, uint Delta)
{
    return __builtin_IB_simd_shuffle_down_uc(Current, Next, Delta);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _i16_i16_i32, )(short Current, short Next, uint Delta)
{
    return __builtin_IB_simd_shuffle_down_us(Current, Next, Delta);
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _i32_i32_i32, )(int Current, int Next, uint Delta)
{
    return __builtin_IB_simd_shuffle_down(Current, Next, Delta);
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _i64_i64_i32, )(long Current, long Next, uint Delta)
{
    int2 CurrentXY = as_int2(Current);
    int2 NextXY = as_int2(Next);
    int2 Result;
    Result.s0 = SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _i32_i32_i32, )(CurrentXY.s0, NextXY.s0, Delta);
    Result.s1 = SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _i32_i32_i32, )(CurrentXY.s1, NextXY.s1, Delta);
    return as_long(Result);
}

#ifdef cl_khr_fp16
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _f16_f16_i32, )(half Current, half Next, uint Delta)
{
    return as_half(SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _i16_i16_i32, )(as_short(Current), as_short(Next), Delta));
}
#endif // cl_khr_fp16

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _f32_f32_i32, )(float Current, float Next, uint Delta)
{
    return as_float(SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _i32_i32_i32, )(as_int(Current), as_int(Next), Delta));
}

#ifdef cl_khr_fp64
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _f64_f64_i32, )(double Current, double Next, uint Delta)
{
    return as_double(SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _i64_i64_i32, )(as_long(Current), as_long(Next), Delta));
}
#endif // cl_khr_fp64

GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_VVS(SubgroupShuffleDownINTEL, char,  char,  uint, i8,  i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_VVS(SubgroupShuffleDownINTEL, short, short, uint, i16, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_VVS(SubgroupShuffleDownINTEL, int,   int,   uint, i32, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_VVS(SubgroupShuffleDownINTEL, float, float, uint, f32, i32)

// OpSubgroupShuffleUpINTEL
//
// Allows data to be transferred from an invocation in the subgroup with a lower SubgroupLocalInvocationId up to
// an invocation in the subgroup with a higher SubgroupLocalInvocationId.
#define DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(TYPE, TYPE_ABBR)                                                                \
TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleUpINTEL, _##TYPE_ABBR##_##TYPE_ABBR##_i32, )(TYPE Previous, TYPE Current, uint Value)  \
{                                                                                                                       \
    Value = __builtin_IB_get_simd_size() - Value;                                                                       \
    return SPIRV_BUILTIN(SubgroupShuffleDownINTEL, _##TYPE_ABBR##_##TYPE_ABBR##_i32, )(Previous, Current, Value);       \
}

DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(char,   i8)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(short,  i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(int,    i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(long,   i64)
#ifdef cl_khr_fp16
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(half,   f16)
#endif // cl_khr_fp16
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(float,  f32)
#if defined(cl_khr_fp64)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(double, f64)
#endif // defined(cl_khr_fp64)

GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_VVS(SubgroupShuffleUpINTEL, char,  char,  uint, i8,  i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_VVS(SubgroupShuffleUpINTEL, short, short, uint, i16, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_VVS(SubgroupShuffleUpINTEL, int,   int,   uint, i32, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_VVS(SubgroupShuffleUpINTEL, float, float, uint, f32, i32)

// OpSubgroupShuffleXorINTEL
//
// Allows data to be transferred between invocations in a subgroup as a function of the invocation's SubgroupLocalInvocationId.
// The data that is returned for this invocation is the value of 'Data' for the invocation with SubgroupLocalInvocationId equal
// to this invocation's SubgroupLocalInvocationId XOR'd with the specified 'Value'.
#define DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(TYPE, TYPE_ABBR)                                \
TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupShuffleXorINTEL, _##TYPE_ABBR##_i32, )(TYPE Data, uint Value)  \
{                                                                                        \
    Value = __builtin_IB_get_simd_id() ^ Value;                                          \
    return SPIRV_BUILTIN(SubgroupShuffleINTEL, _##TYPE_ABBR##_i32, )(Data, Value);       \
}

DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(char,   i8)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(short,  i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(int,    i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(long,   i64)
#ifdef cl_khr_fp16
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(half,   f16)
#endif // cl_khr_fp16
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(float,  f32)
#if defined(cl_khr_fp64)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(double, f64)
#endif // defined(cl_khr_fp64)

GENERATE_SPIRV_VECTOR_FUNCTIONS_2ARGS_VS(SubgroupShuffleXorINTEL, char,  char,  uint, i8,  i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_2ARGS_VS(SubgroupShuffleXorINTEL, short, short, uint, i16, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_2ARGS_VS(SubgroupShuffleXorINTEL, int,   int,   uint, i32, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_2ARGS_VS(SubgroupShuffleXorINTEL, float, float, uint, f32, i32)

// OpSubgroupBlockReadINTEL
//
// Reads one or more components of 'Result' data for each invocation in the subgroup from the specified 'Ptr'
// as a block operation.

#define DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(TYPE, TYPE_ABBR, ELEM_TYPE, ELEM_TYPE_ABBR, INTERNAL_FUNC)                                  \
TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupBlockReadINTEL, _##TYPE_ABBR##_p1##ELEM_TYPE_ABBR, _R##TYPE)(const global u##ELEM_TYPE* p)  \
{                                                                                                                                         \
    return as_##TYPE(__builtin_IB_##INTERNAL_FUNC((__global void*)p));                                                                    \
}                                                                                                                                         \
TYPE __internal_SubgroupBlockReadINTEL_##TYPE##_cache_controls(const global u##ELEM_TYPE* p, enum LSC_LDCC cache_controls)                \
{                                                                                                                                         \
    return as_##TYPE(__builtin_IB_cache_controls_##INTERNAL_FUNC((__global void*)p, cache_controls));                                     \
}

#ifdef cl_intel_subgroups_char
DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(char,   i8,    char, i8, simd_block_read_1_global_b)
DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(char2,  v2i8,  char, i8, simd_block_read_2_global_b)
DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(char4,  v4i8,  char, i8, simd_block_read_4_global_b)
DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(char8,  v8i8,  char, i8, simd_block_read_8_global_b)
DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(char16, v16i8, char, i8, simd_block_read_16_global_b)
#endif // cl_intel_subgroups_char

DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(short,   i16,    short, i16, simd_block_read_1_global_h)
DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(short2,  v2i16,  short, i16, simd_block_read_2_global_h)
DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(short4,  v4i16,  short, i16, simd_block_read_4_global_h)
DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(short8,  v8i16,  short, i16, simd_block_read_8_global_h)
DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(short16, v16i16, short, i16, simd_block_read_16_global_h)

DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(int,  i32,   int, i32, simd_block_read_1_global)
DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(int2, v2i32, int, i32, simd_block_read_2_global)
DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(int4, v4i32, int, i32, simd_block_read_4_global)
DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(int8, v8i32, int, i32, simd_block_read_8_global)

#ifdef cl_intel_subgroups_long
DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(long,  i64,   long, i64, simd_block_read_1_global_l)
DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(long2, v2i64, long, i64, simd_block_read_2_global_l)
DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(long4, v4i64, long, i64, simd_block_read_4_global_l)
DEF_INTEL_SUB_GROUP_BLOCK_READ_GLOBAL(long8, v8i64, long, i64, simd_block_read_8_global_l)
#endif // cl_intel_subgroups_long

#ifdef cl_intel_subgroup_buffer_prefetch

void __internal_SubgroupBlockPrefetchINTEL_char_cache_controls(const global uchar* ptr, uint num_bytes, enum LSC_LDCC cacheOpt)
{
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        if (num_bytes == 1)
        {
            __builtin_IB_lsc_simd_block_prefetch_uchar(ptr, cacheOpt);
        }
        else if (num_bytes == 2)
        {
            __builtin_IB_lsc_simd_block_prefetch_uchar2(ptr, cacheOpt);
        }
        else if (num_bytes == 4)
        {
            __builtin_IB_lsc_simd_block_prefetch_uchar4(ptr, cacheOpt);
        }
        else if (num_bytes == 8)
        {
            __builtin_IB_lsc_simd_block_prefetch_uchar8(ptr, cacheOpt);
        }
        else if (num_bytes == 16)
        {
            __builtin_IB_lsc_simd_block_prefetch_uchar16(ptr, cacheOpt);
        }
    }
}

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupBlockPrefetchINTEL, _p1i8, )(const global uchar* ptr, uint num_bytes)
{
    __internal_SubgroupBlockPrefetchINTEL_char_cache_controls(ptr, num_bytes, LSC_LDCC_DEFAULT);
}

void __internal_SubgroupBlockPrefetchINTEL_short_cache_controls(const global ushort* ptr, uint num_bytes, enum LSC_LDCC cacheOpt)
{
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        if (num_bytes == 2)
        {
            __builtin_IB_lsc_simd_block_prefetch_ushort(ptr, cacheOpt);
        }
        else if (num_bytes == 4)
        {
            __builtin_IB_lsc_simd_block_prefetch_ushort2(ptr, cacheOpt);
        }
        else if (num_bytes == 8)
        {
            __builtin_IB_lsc_simd_block_prefetch_ushort4(ptr, cacheOpt);
        }
        else if (num_bytes == 16)
        {
            __builtin_IB_lsc_simd_block_prefetch_ushort8(ptr, cacheOpt);
        }
        else if (num_bytes == 32)
        {
            __builtin_IB_lsc_simd_block_prefetch_ushort16(ptr, cacheOpt);
        }
    }
}

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupBlockPrefetchINTEL, _p1i16, )(const global ushort* ptr, uint num_bytes)
{
    __internal_SubgroupBlockPrefetchINTEL_short_cache_controls(ptr, num_bytes, LSC_LDCC_DEFAULT);
}

void __internal_SubgroupBlockPrefetchINTEL_int_cache_controls(const global uint* ptr, uint num_bytes, enum LSC_LDCC cacheOpt)
{
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        if (num_bytes == 4)
        {
            __builtin_IB_lsc_simd_block_prefetch_uint(ptr, cacheOpt);
        }
        else if (num_bytes == 8)
        {
            __builtin_IB_lsc_simd_block_prefetch_uint2(ptr, cacheOpt);
        }
        else if (num_bytes == 16)
        {
            __builtin_IB_lsc_simd_block_prefetch_uint4(ptr, cacheOpt);
        }
        else if (num_bytes == 32)
        {
            __builtin_IB_lsc_simd_block_prefetch_uint8(ptr, cacheOpt);
        }
    }
}

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupBlockPrefetchINTEL, _p1i32, )(const global uint* ptr, uint num_bytes)
{
    __internal_SubgroupBlockPrefetchINTEL_int_cache_controls(ptr, num_bytes, LSC_LDCC_DEFAULT);
}

void __internal_SubgroupBlockPrefetchINTEL_long_cache_controls(const global ulong* ptr, uint num_bytes, enum LSC_LDCC cacheOpt)
{
    if (BIF_FLAG_CTRL_GET(UseLSC))
    {
        if (num_bytes == 8)
        {
            __builtin_IB_lsc_simd_block_prefetch_ulong(ptr, cacheOpt);
        }
        else if (num_bytes == 16)
        {
            __builtin_IB_lsc_simd_block_prefetch_ulong2(ptr, cacheOpt);
        }
        else if (num_bytes == 32)
        {
            __builtin_IB_lsc_simd_block_prefetch_ulong4(ptr, cacheOpt);
        }
        else if (num_bytes == 64)
        {
            __builtin_IB_lsc_simd_block_prefetch_ulong8(ptr, cacheOpt);
        }
    }
}

void SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupBlockPrefetchINTEL, _p1i64, )(const global ulong* ptr, uint num_bytes)
{
    __internal_SubgroupBlockPrefetchINTEL_long_cache_controls(ptr, num_bytes, LSC_LDCC_DEFAULT);
}
#endif // cl_intel_subgroup_buffer_prefetch

#ifdef cl_intel_subgroup_local_block_io

#define DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(TYPE, TYPE_ABBR, ELEM_TYPE, ELEM_TYPE_ABBR, INTERNAL_FUNC)   \
TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupBlockReadINTEL, _##TYPE_ABBR##_p3##ELEM_TYPE_ABBR, _R##TYPE)(const local ELEM_TYPE * p)       \
{                                                                                                         \
    return as_##TYPE(INTERNAL_FUNC(p));                                                                   \
}

#ifdef cl_intel_subgroups_char
DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(char,   i8,    uchar, i8, __builtin_IB_simd_block_read_1_local_b)
DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(char2,  v2i8,  uchar, i8, __builtin_IB_simd_block_read_2_local_b)
DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(char4,  v4i8,  uchar, i8, __builtin_IB_simd_block_read_4_local_b)
DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(char8,  v8i8,  uchar, i8, __builtin_IB_simd_block_read_8_local_b)
DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(char16, v16i8, uchar, i8, __builtin_IB_simd_block_read_16_local_b)
#endif // cl_intel_subgroups_char

DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(short,  i16,   ushort, i16, __builtin_IB_simd_block_read_1_local_h)
DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(short2, v2i16, ushort, i16, __builtin_IB_simd_block_read_2_local_h)
DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(short4, v4i16, ushort, i16, __builtin_IB_simd_block_read_4_local_h)
DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(short8, v8i16, ushort, i16, __builtin_IB_simd_block_read_8_local_h)
DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(short16, v16i16, ushort, i16, __builtin_IB_simd_block_read_16_local_h)

DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(int,  i32,   uint, i32, __builtin_IB_simd_block_read_1_local)
DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(int2, v2i32, uint, i32, __builtin_IB_simd_block_read_2_local)
DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(int4, v4i32, uint, i32, __builtin_IB_simd_block_read_4_local)
DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(int8, v8i32, uint, i32, __builtin_IB_simd_block_read_8_local)

#ifdef cl_intel_subgroups_long
DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(long,  i64,   ulong, i64, __builtin_IB_simd_block_read_1_local_l)
DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(long2, v2i64, ulong, i64, __builtin_IB_simd_block_read_2_local_l)
DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(long4, v4i64, ulong, i64, __builtin_IB_simd_block_read_4_local_l)
DEF_INTEL_SUB_GROUP_BLOCK_READ_LOCAL(long8, v8i64, ulong, i64, __builtin_IB_simd_block_read_8_local_l)
#endif // cl_intel_subgroups_long

#endif // cl_intel_subgroup_local_block_io

// OpSubgroupBlockWriteINTEL
//
// Writes one or more components of 'Data' for each invocation in the subgroup from the specified 'Ptr'
// as a block operation.

#define DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(TYPE, TYPE_ABBR, ELEM_TYPE, ELEM_TYPE_ABBR, INTERNAL_FUNC)                                    \
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupBlockWriteINTEL, _p1##ELEM_TYPE_ABBR##_##TYPE_ABBR, )(__global u##ELEM_TYPE * p, u##TYPE data) \
{                                                                                                                                            \
    __builtin_IB_##INTERNAL_FUNC(p, data);                                                                                                   \
}                                                                                                                                            \
void __internal_SubgroupBlockWriteINTEL_##TYPE##_cache_controls(const global u##ELEM_TYPE* p, u##TYPE data, enum LSC_STCC cache_controls)    \
{                                                                                                                                            \
    __builtin_IB_cache_controls_##INTERNAL_FUNC(p, data, cache_controls);                                                                    \
}

#ifdef cl_intel_subgroups_char
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(char,   i8,    char, i8, simd_block_write_1_global_b)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(char2,  v2i8,  char, i8, simd_block_write_2_global_b)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(char4,  v4i8,  char, i8, simd_block_write_4_global_b)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(char8,  v8i8,  char, i8, simd_block_write_8_global_b)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(char16, v16i8, char, i8, simd_block_write_16_global_b)
#endif // cl_intel_subgroups_char

DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(short,   i16,    short, i16, simd_block_write_1_global_h)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(short2,  v2i16,  short, i16, simd_block_write_2_global_h)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(short4,  v4i16,  short, i16, simd_block_write_4_global_h)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(short8,  v8i16,  short, i16, simd_block_write_8_global_h)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(short16, v16i16, short, i16, simd_block_write_16_global_h)

DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(int,   i32,    int, i32, simd_block_write_1_global)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(int2,  v2i32,  int, i32, simd_block_write_2_global)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(int4,  v4i32,  int, i32, simd_block_write_4_global)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(int8,  v8i32,  int, i32, simd_block_write_8_global)

#ifdef cl_intel_subgroups_long
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(long,   i64,    long, i64, simd_block_write_1_global_l)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(long2,  v2i64,  long, i64, simd_block_write_2_global_l)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(long4,  v4i64,  long, i64, simd_block_write_4_global_l)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_GLOBAL(long8,  v8i64,  long, i64, simd_block_write_8_global_l)
#endif // cl_intel_subgroups_long

#ifdef cl_intel_subgroup_local_block_io

#define DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(TYPE, TYPE_ABBR, ELEM_TYPE, ELEM_TYPE_ABBR, INTERNAL_FUNC)             \
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupBlockWriteINTEL, _p3##ELEM_TYPE_ABBR##_##TYPE_ABBR, )(__local ELEM_TYPE * p, TYPE data)    \
{                                                                                                                    \
    INTERNAL_FUNC(p, data);                                                                                          \
}

#ifdef cl_intel_subgroups_char
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(uchar,   i8,    uchar, i8, __builtin_IB_simd_block_write_1_local_b)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(uchar2,  v2i8,  uchar, i8, __builtin_IB_simd_block_write_2_local_b)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(uchar4,  v4i8,  uchar, i8, __builtin_IB_simd_block_write_4_local_b)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(uchar8,  v8i8,  uchar, i8, __builtin_IB_simd_block_write_8_local_b)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(uchar16, v16i8, uchar, i8, __builtin_IB_simd_block_write_16_local_b)
#endif // cl_intel_subgroups_char

DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(ushort,  i16,   ushort, i16, __builtin_IB_simd_block_write_1_local_h)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(ushort2, v2i16, ushort, i16, __builtin_IB_simd_block_write_2_local_h)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(ushort4, v4i16, ushort, i16, __builtin_IB_simd_block_write_4_local_h)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(ushort8, v8i16, ushort, i16, __builtin_IB_simd_block_write_8_local_h)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(ushort16, v16i16, ushort, i16, __builtin_IB_simd_block_write_16_local_h)

DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(uint,  i32,   uint, i32, __builtin_IB_simd_block_write_1_local)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(uint2, v2i32, uint, i32, __builtin_IB_simd_block_write_2_local)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(uint4, v4i32, uint, i32, __builtin_IB_simd_block_write_4_local)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(uint8, v8i32, uint, i32, __builtin_IB_simd_block_write_8_local)

#ifdef cl_intel_subgroups_long
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(ulong,  i64,   ulong, i64, __builtin_IB_simd_block_write_1_local_l)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(ulong2, v2i64, ulong, i64, __builtin_IB_simd_block_write_2_local_l)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(ulong4, v4i64, ulong, i64, __builtin_IB_simd_block_write_4_local_l)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_LOCAL(ulong8, v8i64, ulong, i64, __builtin_IB_simd_block_write_8_local_l)
#endif // cl_intel_subgroups_long

#endif // cl_intel_subgroup_local_block_io

// OpSubgroupImageBlockReadINTEL
//
// Reads one or more components of 'Result' data for each invocation in the subgroup from the specified 'Image'
// at the specified 'Coordinate' as a block operation.

#define DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(TYPE, TYPE_ABBR, INTERNAL_FUNC)                                         \
TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupImageBlockReadINTEL, _##TYPE_ABBR##_img2d_ro_v2i32, _R##TYPE)(global Img2d_ro* image, int2 coord)  \
{                                                                                                                    \
    long id = (long)__builtin_astype(image, __global void*);                                                           \
    return as_##TYPE(INTERNAL_FUNC(id, coord));                                                                      \
}                                                                                                                    \
TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupImageBlockReadINTEL, _##TYPE_ABBR##_img2d_rw_v2i32, _R##TYPE)(global Img2d_rw* image, int2 coord)  \
{                                                                                                                    \
    long id = (long)__builtin_astype(image, __global void*);                                                           \
    return as_##TYPE(INTERNAL_FUNC(id, coord));                                                                      \
}

#ifdef cl_intel_subgroups_char
DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(char,   i8,    __builtin_IB_simd_media_block_read_1_b)
DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(char2,  v2i8,  __builtin_IB_simd_media_block_read_2_b)
DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(char4,  v4i8,  __builtin_IB_simd_media_block_read_4_b)
DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(char8,  v8i8,  __builtin_IB_simd_media_block_read_8_b)
DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(char16, v16i8, __builtin_IB_simd_media_block_read_16_b)
#endif // cl_intel_subgroups_char

DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(short,  i16,   __builtin_IB_simd_media_block_read_1_h)
DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(short2, v2i16, __builtin_IB_simd_media_block_read_2_h)
DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(short4, v4i16, __builtin_IB_simd_media_block_read_4_h)
DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(short8, v8i16, __builtin_IB_simd_media_block_read_8_h)
DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(short16, v16i16, __builtin_IB_simd_media_block_read_16_h)

DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(int,  i32,   __builtin_IB_simd_media_block_read_1)
DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(int2, v2i32, __builtin_IB_simd_media_block_read_2)
DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(int4, v4i32, __builtin_IB_simd_media_block_read_4)
DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(int8, v8i32, __builtin_IB_simd_media_block_read_8)

#ifdef cl_intel_subgroups_long
DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(long,  i64,   __builtin_IB_simd_media_block_read_1_l)
DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(long2, v2i64, __builtin_IB_simd_media_block_read_2_l)
DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(long4, v4i64, __builtin_IB_simd_media_block_read_4_l)
DEF_INTEL_SUB_GROUP_BLOCK_READ_IMAGE(long8, v8i64, __builtin_IB_simd_media_block_read_8_l)
#endif // cl_intel_subgroups_long

// OpSubgroupImageBlockWriteINTEL
//
// Writes one or more components of 'Data' for each invocation in the subgroup to the specified 'Image'
// at the specified 'Coordinate' as a block operation.

#define DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(TYPE, TYPE_ABBR, INTERNAL_FUNC)      \
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupImageBlockWriteINTEL, _img2d_wo_v2i32_##TYPE_ABBR, )(    \
    global Img2d_wo* image, int2 coord, TYPE data)                                 \
{                                                                                  \
    long id = (long)__builtin_astype(image, __global void*);                         \
    INTERNAL_FUNC((long)image, coord, data);                                        \
}                                                                                  \
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupImageBlockWriteINTEL, _img2d_rw_v2i32_##TYPE_ABBR, )(    \
    global Img2d_rw* image, int2 coord, TYPE data)                                 \
{                                                                                  \
    long id = (long)__builtin_astype(image, __global void*);                         \
    INTERNAL_FUNC((long)image, coord, data);                                        \
}

#ifdef cl_intel_subgroups_char
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(uchar,   i8,    __builtin_IB_simd_media_block_write_1_b)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(uchar2,  v2i8,  __builtin_IB_simd_media_block_write_2_b)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(uchar4,  v4i8,  __builtin_IB_simd_media_block_write_4_b)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(uchar8,  v8i8,  __builtin_IB_simd_media_block_write_8_b)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(uchar16, v16i8, __builtin_IB_simd_media_block_write_16_b)
#endif // cl_intel_subgroups_char

DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(ushort,  i16,   __builtin_IB_simd_media_block_write_1_h)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(ushort2, v2i16, __builtin_IB_simd_media_block_write_2_h)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(ushort4, v4i16, __builtin_IB_simd_media_block_write_4_h)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(ushort8, v8i16, __builtin_IB_simd_media_block_write_8_h)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(ushort16, v16i16, __builtin_IB_simd_media_block_write_16_h)

DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(uint,  i32,   __builtin_IB_simd_media_block_write_1)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(uint2, v2i32, __builtin_IB_simd_media_block_write_2)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(uint4, v4i32, __builtin_IB_simd_media_block_write_4)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(uint8, v8i32, __builtin_IB_simd_media_block_write_8)

#ifdef cl_intel_subgroups_long
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(ulong,  i64,   __builtin_IB_simd_media_block_write_1_l)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(ulong2, v2i64, __builtin_IB_simd_media_block_write_2_l)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(ulong4, v4i64, __builtin_IB_simd_media_block_write_4_l)
DEF_INTEL_SUB_GROUP_BLOCK_WRITE_IMAGE(ulong8, v8i64, __builtin_IB_simd_media_block_write_8_l)
#endif // cl_intel_subgroups_long

// OpSubgroupImageMediaBlockReadINTEL
//
// Reads a block of data from a 2D region of the specified 'Image'.

#define DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_READ_IMAGE(TYPE, TYPE_ABBR)                                                         \
TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupImageMediaBlockReadINTEL, _##TYPE_ABBR##_img2d_ro_v2i32_i32_i32, _R##TYPE)(   \
    global Img2d_ro* image, int2 coord, int width, int height)                                                              \
{                                                                                                                           \
    long id = (long)__builtin_astype(image, global void*);                                                                    \
    return as_##TYPE(__builtin_IB_media_block_read_u##TYPE(id, coord, width, height));                                      \
}                                                                                                                           \
TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupImageMediaBlockReadINTEL, _##TYPE_ABBR##_img2d_rw_v2i32_i32_i32, _R##TYPE)(   \
    global Img2d_rw* image, int2 coord, int width, int height)                                                              \
{                                                                                                                           \
    long id = (long)__builtin_astype(image, global void*);                                                                    \
    return as_##TYPE(__builtin_IB_media_block_read_u##TYPE(id, coord, width, height));                                      \
}

DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_READ_IMAGE(char,   i8)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_READ_IMAGE(char2,  v2i8)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_READ_IMAGE(char4,  v4i8)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_READ_IMAGE(char8,  v8i8)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_READ_IMAGE(char16, v16i8)

DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_READ_IMAGE(short,   i16)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_READ_IMAGE(short2,  v2i16)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_READ_IMAGE(short4,  v4i16)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_READ_IMAGE(short8,  v8i16)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_READ_IMAGE(short16, v16i16)

// Integer block reads don't have 16 element version.
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_READ_IMAGE(int,  i32)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_READ_IMAGE(int2, v2i32)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_READ_IMAGE(int4, v4i32)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_READ_IMAGE(int8, v8i32)

// OpSubgroupImageMediaBlockWriteINTEL
//
// Writes a block of data into a 2D region of the specified 'Image'.

#define DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_WRITE_IMAGE(TYPE, TYPE_ABBR)                                                \
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupImageMediaBlockWriteINTEL, _img2d_wo_v2i32_i32_i32_##TYPE_ABBR, )(    \
    global Img2d_wo* image, int2 coord, int width, int height, TYPE data)                                           \
{                                                                                                                   \
    long id = (long)__builtin_astype(image, global void*);                                                            \
    __builtin_IB_media_block_write_u##TYPE(id, coord, width, height, as_u##TYPE(data));                             \
}                                                                                                                   \
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(SubgroupImageMediaBlockWriteINTEL, _img2d_rw_v2i32_i32_i32_##TYPE_ABBR, )(    \
    global Img2d_rw* image, int2 coord, int width, int height, TYPE data)                                           \
{                                                                                                                   \
    long id = (long)__builtin_astype(image, global void*);                                                            \
    __builtin_IB_media_block_write_u##TYPE(id, coord, width, height, as_u##TYPE(data));                             \
}

DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_WRITE_IMAGE(char,   i8)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_WRITE_IMAGE(char2,  v2i8)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_WRITE_IMAGE(char4,  v4i8)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_WRITE_IMAGE(char8,  v8i8)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_WRITE_IMAGE(char16, v16i8)

DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_WRITE_IMAGE(short,   i16)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_WRITE_IMAGE(short2,  v2i16)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_WRITE_IMAGE(short4,  v4i16)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_WRITE_IMAGE(short8,  v8i16)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_WRITE_IMAGE(short16, v16i16)

// Integer block writes don't have 16 element version.
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_WRITE_IMAGE(int,  i32)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_WRITE_IMAGE(int2, v2i32)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_WRITE_IMAGE(int4, v4i32)
DEF_INTEL_SUB_GROUP_MEDIA_BLOCK_WRITE_IMAGE(int8, v8i32)

// Ballot Functions

uint intel_sub_group_ballot(bool p)
{
    return __builtin_IB_WaveBallot(p);
}

uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupEqMaskKHR, , )(void)
{
    uint id = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();
    uint4 v = 0;

    v.x = 1 << id;

    return v;
}

uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupGeMaskKHR, , )(void)
{
    uint id = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();
    uint4 v = 0;

    v.x = as_uint(as_int(1 << 31) >> (31 - id));

    return v;
}

uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLeMaskKHR, , )(void)
{
    uint id = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();
    uint4 v = 0;

    uint bitIdx = 1 << id;

    v.x = (bitIdx - 1) | bitIdx;

    return v;
}

uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupGtMaskKHR, , )(void)
{
    uint4 v = 0;

    v.x = ~SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLeMaskKHR, , )().x;

    return v;
}

uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLtMaskKHR, , )(void)
{
    uint id = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();
    uint4 v = 0;

    v.x = (1 << id) - 1;

    return v;
}

uint4 __builtin_spirv_OpSubgroupBallotKHR_i1(bool Predicate)
{
    uint4 v = 0;
    v.x = __builtin_IB_WaveBallot(Predicate);
    return v;
}

uint __builtin_spirv_OpSubgroupFirstInvocationKHR_i32(uint Value)
{
    uint chanEnable = __builtin_IB_WaveBallot(true);
    uint firstActive = SPIRV_OCL_BUILTIN(ctz, _i32, )(as_int(chanEnable));
    int3 id = (int3)(firstActive, 0, 0);
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i32_v3i32, )(Subgroup, as_int(Value), id);
}

float __builtin_spirv_OpSubgroupFirstInvocationKHR_f32(float Value)
{
    return as_float(__builtin_spirv_OpSubgroupFirstInvocationKHR_i32(as_uint(Value)));
}

#if defined(cl_khr_subgroup_ballot)
char SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBroadcast, _i32_i8_i32, )(int Execution, char Value, uint Id)
{
    if (Execution == Subgroup)
    {
        return __builtin_IB_simd_shuffle_c(Value, Id);
    }
    else
    {
        return SPIRV_BUILTIN(GroupBroadcast, _i32_i8_i32, )(Execution, Value, as_int(Id));
    }
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBroadcast, _i32_i16_i32, )(int Execution, short Value, uint Id)
{
    if (Execution == Subgroup)
    {
        if(BIF_FLAG_CTRL_GET(UseOOBChecks))
        {
            Id = Id & (get_max_sub_group_size() - 1);
        }
        return as_ushort(__builtin_IB_simd_shuffle_h(as_half(Value), Id));
    }
    else
    {
        return SPIRV_BUILTIN(GroupBroadcast, _i32_i16_i32, )(Execution, Value, as_int(Id));
    }
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBroadcast, _i32_i32_i32, )(int Execution, int Value, uint Id)
{
    if (Execution == Subgroup)
    {
        return __builtin_IB_simd_shuffle(Value, Id);
    }
    else
    {
        return SPIRV_BUILTIN(GroupBroadcast, _i32_i32_i32, )(Execution, Value, as_int(Id));
    }
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBroadcast, _i32_i64_i32, )(int Execution, long Value, uint Id)
{
    if (Execution == Subgroup)
    {
        return ((((ulong)__builtin_IB_simd_shuffle(Value >> 32, Id)) << 32 ) | __builtin_IB_simd_shuffle((uint)Value, Id));
    }
    else
    {
        return SPIRV_BUILTIN(GroupBroadcast, _i32_i64_i32, )(Execution, Value, as_int(Id));
    }
}

#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBroadcast, _i32_f16_i32, )(int Execution, half Value, uint Id)
{
    if (Execution == Subgroup)
    {
        return as_half2(__builtin_IB_simd_shuffle( (uint)(as_short(Value)), Id )).x;
    }
    else
    {
        return SPIRV_BUILTIN(GroupBroadcast, _i32_f16_i32, )(Execution, Value, as_int(Id));
    }
}
#endif // defined(cl_khr_fp16)

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBroadcast, _i32_f32_i32, )(int Execution, float Value, uint Id)
{
    if (Execution == Subgroup)
    {
        return __builtin_IB_simd_shuffle_f( Value, Id );
    }
    else
    {
        return SPIRV_BUILTIN(GroupBroadcast, _i32_f32_i32, )(Execution, Value, as_int(Id));
    }
}

#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBroadcast, _i32_f64_i32, )(int Execution, double Value, uint Id)
{
    if (Execution == Subgroup)
    {
        if(BIF_FLAG_CTRL_GET(UseOOBChecks))
        {
            Id = Id & (get_max_sub_group_size() - 1);
        }
        return __builtin_IB_simd_shuffle_df( Value, Id );
    }
    else
    {
        return SPIRV_BUILTIN(GroupBroadcast, _i32_f64_i32, )(Execution, Value, as_int(Id));
    }
}
#endif // defined(cl_khr_fp64)

#define DEFN_NON_UNIFORM_BROADCAS_VEC(__vargtype, __abbrvargtype)                                                  \
    GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformBroadcast, __vargtype, int, __vargtype, uint, i32, __abbrvargtype, i32)

#define DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE, TYPE_ABBR)                                                                      \
TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBroadcastFirst, _i32_##TYPE_ABBR, )(int Execution, TYPE Value)           \
{                                                                                                                             \
    uint activeChannels = __builtin_IB_WaveBallot(true);                                                                      \
    int firstActive = SPIRV_OCL_BUILTIN(ctz, _i32, )(as_int(activeChannels));                                                 \
    return SPIRV_BUILTIN(GroupNonUniformBroadcast, _i32_##TYPE_ABBR##_i32, )(Execution, Value, as_uint(firstActive));         \
}

#define DEFN_NON_UNIFORM_BROADCAST(TYPE, TYPE_ABBR)             \
    DEFN_NON_UNIFORM_BROADCAS_VEC(TYPE, TYPE_ABBR)              \
    DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE, TYPE_ABBR)            \
    DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE##2, v2##TYPE_ABBR)     \
    DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE##3, v3##TYPE_ABBR)     \
    DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE##4, v4##TYPE_ABBR)     \
    DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE##8, v8##TYPE_ABBR)     \
    DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE##16, v16##TYPE_ABBR)

DEFN_NON_UNIFORM_BROADCAST(char,   i8)
DEFN_NON_UNIFORM_BROADCAST(short,  i16)
DEFN_NON_UNIFORM_BROADCAST(int,    i32)
DEFN_NON_UNIFORM_BROADCAST(long,   i64)
DEFN_NON_UNIFORM_BROADCAST(float,  f32)
#if defined(cl_khr_fp64)
DEFN_NON_UNIFORM_BROADCAST(double, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_BROADCAST(half,   f16)
#endif // defined(cl_khr_fp16)

uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBallot, _i32_i1, )(int Execution, bool Predicate)
{
    uint4 v = 0;
    if (Execution == Subgroup)
    {
        v.x = __builtin_IB_WaveBallot(Predicate);
    }
    return v;
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformInverseBallot, _i32_v4i32, )(int Execution, uint4 Value)
{
    if (Execution == Subgroup)
    {
        return (Value.x & (1 << __builtin_IB_get_simd_id())) ? true : false;
    }
    return false;
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBallotBitExtract, _i32_v4i32_i32, )(int Execution, uint4 Value, uint Index)
{
    if (Execution == Subgroup)
    {
        return (Value.x & (1 << Index)) ? true : false;
    }
    return false;
}

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBallotBitCount, _i32_i32_v4i32, )(int Execution, int Operation, uint4 Value)
{
    uint result = 0;
    if (Execution == Subgroup)
    {
        uint sgsize = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupMaxSize, , )();
        uint sglid = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();
        uint consideredBits = Value.x << (32 - sgsize);
        // intended fallthrough in the switch statement
        switch (Operation)
        {
            case GroupOperationExclusiveScan:
                consideredBits <<= 1;
            case GroupOperationInclusiveScan:
                consideredBits <<= ((sgsize - 1) - sglid);
            case GroupOperationReduce:
                result = SPIRV_OCL_BUILTIN(popcount, _i32, )(as_int(consideredBits));
                break;
        }
    }
    return result;
}

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBallotFindLSB, _i32_v4i32, )(int Execution, uint4 Value)
{
    if (Execution == Subgroup)
    {
        return SPIRV_OCL_BUILTIN(ctz, _i32, )(as_int(Value.x));
    }
    return 0;
}

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBallotFindMSB, _i32_v4i32, )(int Execution, uint4 Value)
{
    if (Execution == Subgroup)
    {
        uint sgsize = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupMaxSize, , )();
        uint consideredBits = Value.x << (32 - sgsize);
        return (sgsize - 1) - SPIRV_OCL_BUILTIN(clz, _i32, )((int)consideredBits);
    }
    return 0;
}

uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupEqMask, , )(void)
{
    return SPIRV_BUILTIN_NO_OP(BuiltInSubgroupEqMaskKHR, , )();
}

uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupGeMask, , )(void)
{
    return SPIRV_BUILTIN_NO_OP(BuiltInSubgroupGeMaskKHR, , )();
}

uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupGtMask, , )(void)
{
    return SPIRV_BUILTIN_NO_OP(BuiltInSubgroupGtMaskKHR, , )();
}

uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLeMask, , )(void)
{
    return SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLeMaskKHR, , )();
}

uint4 SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLtMask, , )(void)
{
    return SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLtMaskKHR, , )();
}
#endif // defined(cl_khr_subgroup_ballot)

#define DEFN_SUPPORTED_OPERATION(op_name, op, type) \
static type    OVERLOADABLE __intel_##op_name(type lhs, type rhs) { return lhs op rhs; }

#define DEFN_BINARY_OPERATIONS(type)    \
DEFN_SUPPORTED_OPERATION(and, &, type)  \
DEFN_SUPPORTED_OPERATION(or,  |, type)  \
DEFN_SUPPORTED_OPERATION(xor, ^, type)

DEFN_BINARY_OPERATIONS(bool)
DEFN_BINARY_OPERATIONS(char)
DEFN_BINARY_OPERATIONS(short)
DEFN_BINARY_OPERATIONS(int)
DEFN_BINARY_OPERATIONS(long)

#define DEFN_ARITH_OPERATIONS(type)    \
DEFN_SUPPORTED_OPERATION(mul, *, type) \
DEFN_SUPPORTED_OPERATION(add, +, type)

DEFN_ARITH_OPERATIONS(char)
DEFN_ARITH_OPERATIONS(short)
DEFN_ARITH_OPERATIONS(int)
DEFN_ARITH_OPERATIONS(long)
DEFN_ARITH_OPERATIONS(float)
#if defined(cl_khr_fp64)
DEFN_ARITH_OPERATIONS(double)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_ARITH_OPERATIONS(half)
#endif // defined(cl_khr_fp16)

#define DEFN_WORK_GROUP_REDUCE(func, type_abbr, type, op, identity)                                                                         \
type __builtin_IB_WorkGroupReduce_##func##_##type_abbr(type X)                                                                              \
{                                                                                                                                           \
    if(BIF_FLAG_CTRL_GET(PlatformType) == IGFX_PVC)                                                                                         \
    {                                                                                                                                       \
        type sg_x = SPIRV_BUILTIN(Group##func, _i32_i32_##type_abbr, )(Subgroup, GroupOperationReduce, X); /* 1 step */                     \
        /* number of values to reduce */                                                                                                    \
        uint values_num = SPIRV_BUILTIN_NO_OP(BuiltInNumSubgroups, , )();                                                                   \
                                                                                                                                            \
        /* In case we have only one small wg - return the ready value quick */                                                              \
        if(values_num == 1) {                                                                                                               \
            return sg_x;                                                                                                                    \
        }                                                                                                                                   \
        else {                                                                                                                              \
            GET_SAFE_MEMPOOL_PTR(scratch, type, true, 0)                                                                                    \
            uint sg_id = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupId, , )();                                                                      \
            uint sg_lid = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();                                                      \
            uint sg_size = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupMaxSize, , )();                                                               \
                                                                                                                                            \
            if (sg_lid == 0) {                                                                                                              \
                scratch[sg_id] = sg_x;                                                                                                      \
            }                                                                                                                               \
            SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Workgroup, 0, AcquireRelease | WorkgroupMemory);                                  \
                                                                                                                                            \
            if(sg_id == 0)                                                                                                                  \
            {                                                                                                                               \
                type low_data;                                                                                                              \
                type high_data;                                                                                                             \
                type reduce;                                                                                                                \
                                                                                                                                            \
                if (sg_size == 32) /* SIMD32 */                                                                                             \
                {                                                                                                                           \
                    low_data = sg_lid < values_num ? scratch[sg_lid] : identity;                                                            \
                    high_data = sg_lid + 32 < values_num ? scratch[sg_lid + 32] : identity;                                                 \
                    /* 64 (from 64) elements reduces to 32 */                                                                               \
                    reduce =  op(low_data, high_data);                                                                                      \
                }                                                                                                                           \
                else if(sg_size == 16) /* SIMD16 */                                                                                         \
                {                                                                                                                           \
                    low_data = sg_lid < values_num ? scratch[sg_lid] : identity;                                                            \
                    type mid_low_data = sg_lid + 16 < values_num ? scratch[sg_lid + 16] : identity;                                         \
                    type mid_high_data = sg_lid + 32 < values_num ? scratch[sg_lid + 32] : identity;                                        \
                    high_data = sg_lid + 32 + 16 < values_num ? scratch[sg_lid + 32 + 16] : identity;                                       \
                    /* 32 first part (from 64) elements reduces to 16 */                                                                    \
                    low_data =  op(low_data, mid_low_data);                                                                                 \
                    /* 32 second part (from 64) elements reduces to 16 */                                                                   \
                    high_data =  op(mid_high_data, high_data);                                                                              \
                    /* 64 (from 64) elements reduces to 16 */                                                                               \
                    reduce =  op(low_data, high_data);                                                                                      \
                }                                                                                                                           \
                /* SIMD8 is not available on PVC */                                                                                         \
                                                                                                                                            \
                sg_x = SPIRV_BUILTIN(Group##func, _i32_i32_##type_abbr, )(Subgroup, GroupOperationReduce, reduce);                          \
                if (sg_lid == 0) {                                                                                                          \
                    scratch[0] = sg_x;                                                                                                      \
                }                                                                                                                           \
            }                                                                                                                               \
            SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Workgroup, 0, AcquireRelease | WorkgroupMemory);                                  \
            return scratch[0];                                                                                                              \
        }                                                                                                                                   \
    }                                                                                                                                       \
    else                                                                                                                                    \
    {                                                                                                                                       \
        type sg_x = SPIRV_BUILTIN(Group##func, _i32_i32_##type_abbr, )(Subgroup, GroupOperationReduce, X);                                  \
        GET_SAFE_MEMPOOL_PTR(scratch, type, true, 0)                                                                                        \
        uint sg_id = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupId, , )();                                                                          \
        uint num_sg = SPIRV_BUILTIN_NO_OP(BuiltInNumSubgroups, , )();                                                                       \
        uint sg_lid = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();                                                          \
        uint sg_size = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupSize, , )();                                                                      \
        uint sg_max_size = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupMaxSize, , )();                                                               \
                                                                                                                                            \
        if (sg_lid == 0) {                                                                                                                  \
            scratch[sg_id] = sg_x;                                                                                                          \
        }                                                                                                                                   \
        SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Workgroup, 0, AcquireRelease | WorkgroupMemory);                                      \
                                                                                                                                            \
        uint global_id = sg_id * sg_max_size + sg_lid;                                                                                      \
        uint values_num = num_sg;                                                                                                           \
        while(values_num > sg_max_size) {                                                                                                   \
            uint max_id = ((values_num + sg_max_size - 1) / sg_max_size) * sg_max_size;                                                     \
            type value = global_id < values_num ? scratch[global_id] : identity;                                                            \
            SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Workgroup, 0, AcquireRelease | WorkgroupMemory);                                  \
            if (global_id < max_id) {                                                                                                       \
                sg_x = SPIRV_BUILTIN(Group##func, _i32_i32_##type_abbr, )(Subgroup, GroupOperationReduce, value);                           \
                if (sg_lid == 0) {                                                                                                          \
                    scratch[sg_id] = sg_x;                                                                                                  \
                }                                                                                                                           \
            }                                                                                                                               \
            values_num = max_id / sg_max_size;                                                                                              \
            SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Workgroup, 0, AcquireRelease | WorkgroupMemory);                                  \
        }                                                                                                                                   \
                                                                                                                                            \
        type result;                                                                                                                        \
        if (values_num > sg_size) {                                                                                                         \
            type sg_aggregate = scratch[0];                                                                                                 \
            for (int s = 1; s < values_num; ++s) {                                                                                          \
                sg_aggregate = op(sg_aggregate, scratch[s]);                                                                                \
            }                                                                                                                               \
            result = sg_aggregate;                                                                                                          \
        } else {                                                                                                                            \
            type value = sg_lid < values_num ? scratch[sg_lid] : identity;                                                                  \
            result = SPIRV_BUILTIN(Group##func, _i32_i32_##type_abbr, )(Subgroup, GroupOperationReduce, value);                             \
        }                                                                                                                                   \
        return result;                                                                                                                      \
    }                                                                                                                                       \
}


#define DEFN_WORK_GROUP_SCAN_INCL(func, type_abbr, type, op)                                                    \
type __builtin_IB_WorkGroupScanInclusive_##func##_##type_abbr(type X)                                           \
{                                                                                                               \
    type sg_x = SPIRV_BUILTIN(Group##func, _i32_i32_##type_abbr, )(Subgroup, GroupOperationInclusiveScan, X);   \
                                                                                                                \
    GET_SAFE_MEMPOOL_PTR(scratch, type, true, 0)                                                                \
    uint sg_id = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupId, , )();                                                  \
    uint num_sg = SPIRV_BUILTIN_NO_OP(BuiltInNumSubgroups, , )();                                               \
    uint sg_lid = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();                                  \
    uint sg_size = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupSize, , )();                                              \
                                                                                                                \
    if (sg_lid == sg_size - 1) {                                                                                \
        scratch[sg_id] = sg_x;                                                                                  \
    }                                                                                                           \
    SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Workgroup, 0, AcquireRelease | WorkgroupMemory);              \
                                                                                                                \
    type sg_prefix;                                                                                             \
    type sg_aggregate = scratch[0];                                                                             \
    for (int s = 1; s < num_sg; ++s) {                                                                          \
        if (sg_id == s) {                                                                                       \
            sg_prefix = sg_aggregate;                                                                           \
            break;                                                                                              \
        }                                                                                                       \
        sg_aggregate = op(sg_aggregate, scratch[s]);                                                            \
    }                                                                                                           \
                                                                                                                \
    type result;                                                                                                \
    if (sg_id == 0) {                                                                                           \
        result = sg_x;                                                                                          \
    } else {                                                                                                    \
        result = op(sg_x, sg_prefix);                                                                           \
    }                                                                                                           \
    return result;                                                                                              \
}


#define DEFN_WORK_GROUP_SCAN_EXCL(func, type_abbr, type, op, identity)                                          \
type __builtin_IB_WorkGroupScanExclusive_##func##_##type_abbr(type X)                                           \
{                                                                                                               \
    type carry = SPIRV_BUILTIN(Group##func, _i32_i32_##type_abbr, )(Subgroup, GroupOperationInclusiveScan, X);  \
                                                                                                                \
    GET_SAFE_MEMPOOL_PTR(scratch, type, true, 0)                                                                \
    uint sg_id = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupId, , )();                                                  \
    uint num_sg = SPIRV_BUILTIN_NO_OP(BuiltInNumSubgroups, , )();                                               \
    uint sg_lid = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();                                  \
    uint sg_size = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupSize, , )();                                              \
                                                                                                                \
    type sg_x = intel_sub_group_shuffle_up((type)identity, carry, 1);                                           \
    if (sg_lid == 0) {                                                                                          \
        sg_x = identity;                                                                                        \
    }                                                                                                           \
                                                                                                                \
    if (sg_lid == sg_size - 1) {                                                                                \
        scratch[sg_id] = carry;                                                                                 \
    }                                                                                                           \
    SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Workgroup, 0, AcquireRelease | WorkgroupMemory);              \
                                                                                                                \
    type sg_prefix;                                                                                             \
    type sg_aggregate = scratch[0];                                                                             \
    for (int s = 1; s < num_sg; ++s) {                                                                          \
        if (sg_id == s) {                                                                                       \
          sg_prefix = sg_aggregate;                                                                             \
          break;                                                                                                \
        }                                                                                                       \
        sg_aggregate = op(sg_aggregate, scratch[s]);                                                            \
    }                                                                                                           \
                                                                                                                \
    type result;                                                                                                \
    if (sg_id == 0) {                                                                                           \
        result = sg_x;                                                                                          \
    } else {                                                                                                    \
        result = op(sg_x, sg_prefix);                                                                           \
    }                                                                                                           \
    return result;                                                                                              \
}

#define DEFN_SUB_GROUP_REDUCE(func, type_abbr, type, op, identity, signed_cast)                         \
type __builtin_IB_SubGroupReduce_##func##_##type_abbr(type X)                                           \
{                                                                                                       \
    uint sgsize = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupSize, , )();                                       \
    if(sgsize == 8)                                                                                     \
    {                                                                                                   \
        X = op(intel_sub_group_shuffle( X, 0 ),                                                         \
                op(intel_sub_group_shuffle( X, 1 ),                                                     \
                op(intel_sub_group_shuffle( X, 2 ),                                                     \
                op(intel_sub_group_shuffle( X, 3 ),                                                     \
                op(intel_sub_group_shuffle( X, 4 ),                                                     \
                op(intel_sub_group_shuffle( X, 5 ),                                                     \
                op(intel_sub_group_shuffle( X, 6 ),intel_sub_group_shuffle( X, 7 )                      \
                )))))));                                                                                \
    }                                                                                                   \
    else if(sgsize == 16)                                                                               \
    {                                                                                                   \
        X = op(intel_sub_group_shuffle( X, 0 ),                                                         \
                op(intel_sub_group_shuffle( X, 1 ),                                                     \
                op(intel_sub_group_shuffle( X, 2 ),                                                     \
                op(intel_sub_group_shuffle( X, 3 ),                                                     \
                op(intel_sub_group_shuffle( X, 4 ),                                                     \
                op(intel_sub_group_shuffle( X, 5 ),                                                     \
                op(intel_sub_group_shuffle( X, 6 ),                                                     \
                op(intel_sub_group_shuffle( X, 7 ),                                                     \
                op(intel_sub_group_shuffle( X, 8 ),                                                     \
                op(intel_sub_group_shuffle( X, 9 ),                                                     \
                op(intel_sub_group_shuffle( X, 10 ),                                                    \
                op(intel_sub_group_shuffle( X, 11 ),                                                    \
                op(intel_sub_group_shuffle( X, 12 ),                                                    \
                op(intel_sub_group_shuffle( X, 13 ),                                                    \
                op(intel_sub_group_shuffle( X, 14 ),intel_sub_group_shuffle( X, 15 )                    \
                )))))))))))))));                                                                        \
    }                                                                                                   \
    else if(sgsize == 32)                                                                               \
    {                                                                                                   \
        X = op(intel_sub_group_shuffle( X, 0 ),                                                         \
                op(intel_sub_group_shuffle( X, 1 ),                                                     \
                op(intel_sub_group_shuffle( X, 2 ),                                                     \
                op(intel_sub_group_shuffle( X, 3 ),                                                     \
                op(intel_sub_group_shuffle( X, 4 ),                                                     \
                op(intel_sub_group_shuffle( X, 5 ),                                                     \
                op(intel_sub_group_shuffle( X, 6 ),                                                     \
                op(intel_sub_group_shuffle( X, 7 ),                                                     \
                op(intel_sub_group_shuffle( X, 8 ),                                                     \
                op(intel_sub_group_shuffle( X, 9 ),                                                     \
                op(intel_sub_group_shuffle( X, 10 ),                                                    \
                op(intel_sub_group_shuffle( X, 11 ),                                                    \
                op(intel_sub_group_shuffle( X, 12 ),                                                    \
                op(intel_sub_group_shuffle( X, 13 ),                                                    \
                op(intel_sub_group_shuffle( X, 14 ),                                                    \
                op(intel_sub_group_shuffle( X, 15 ),                                                    \
                op(intel_sub_group_shuffle( X, 16 ),                                                    \
                op(intel_sub_group_shuffle( X, 17 ),                                                    \
                op(intel_sub_group_shuffle( X, 18 ),                                                    \
                op(intel_sub_group_shuffle( X, 19 ),                                                    \
                op(intel_sub_group_shuffle( X, 20 ),                                                    \
                op(intel_sub_group_shuffle( X, 21 ),                                                    \
                op(intel_sub_group_shuffle( X, 22 ),                                                    \
                op(intel_sub_group_shuffle( X, 23 ),                                                    \
                op(intel_sub_group_shuffle( X, 24 ),                                                    \
                op(intel_sub_group_shuffle( X, 25 ),                                                    \
                op(intel_sub_group_shuffle( X, 26 ),                                                    \
                op(intel_sub_group_shuffle( X, 27 ),                                                    \
                op(intel_sub_group_shuffle( X, 28 ),                                                    \
                op(intel_sub_group_shuffle( X, 29 ),                                                    \
                op(intel_sub_group_shuffle( X, 30 ),intel_sub_group_shuffle( X, 31 )                    \
                )))))))))))))))))))))))))))))));                                                        \
    }                                                                                                   \
    else                                                                                                \
    {                                                                                                   \
        uint sglid = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();                       \
        uint sgMaxSize = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupMaxSize, , )();                             \
                                                                                                        \
        for (uint i = 1; i < sgMaxSize; i <<= 1) {                                                      \
            type shuffle = intel_sub_group_shuffle_up((type)identity, X, i);                            \
            type contribution = (sglid < i) ? identity : shuffle;                                       \
            X = op(X, contribution);                                                                    \
        }                                                                                               \
        X = intel_sub_group_shuffle( X, sgsize - 1 );                                                   \
    }                                                                                                   \
    int3 vec3;                                                                                          \
    vec3.s0 = 0;                                                                                        \
    return SPIRV_BUILTIN(GroupBroadcast, _i32_##type_abbr##_v3i32, )(Subgroup, signed_cast(X), vec3 );  \
}

#define DEFN_SUB_GROUP_SCAN_INCL(func, type_abbr, type, op, identity)                    \
type __builtin_IB_SubGroupScanInclusive_##func##_##type_abbr(type X)                     \
{                                                                                        \
    uint sgsize = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupSize, , )();                        \
    uint offset = 1;                                                                     \
    while( offset < sgsize )                                                             \
    {                                                                                    \
        type other = intel_sub_group_shuffle_up( (type)identity, X, offset );            \
        X = op( X, other );                                                              \
        offset <<= 1;                                                                    \
    }                                                                                    \
    return X;                                                                            \
}

#define DEFN_SUB_GROUP_SCAN_EXCL(func, type_abbr, type, op, identity)                    \
type __builtin_IB_SubGroupScanExclusive_##func##_##type_abbr(type X)                     \
{                                                                                        \
    uint sgsize = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupSize, , )();                        \
    X = intel_sub_group_shuffle_up( (type)identity, X, 1 );                              \
    uint offset = 1;                                                                     \
    while( offset < sgsize )                                                             \
    {                                                                                    \
        type other = intel_sub_group_shuffle_up( (type)identity, X, offset );            \
        X = op( X, other );                                                              \
        offset <<= 1;                                                                    \
    }                                                                                    \
    return X;                                                                            \
}

#define DEFN_UNIFORM_GROUP_FUNC_BASE(func, type, type_gen, type_abbr, op, identity, signed_cast)  \
                                                                                                  \
DEFN_SUB_GROUP_REDUCE(func, type_abbr, type, op, identity, signed_cast)                           \
DEFN_SUB_GROUP_SCAN_INCL(func, type_abbr, type, op, identity)                                     \
DEFN_SUB_GROUP_SCAN_EXCL(func, type_abbr, type, op, identity)                                     \
                                                                                                  \
DEFN_WORK_GROUP_REDUCE(func, type_abbr, type, op, identity)                                       \
DEFN_WORK_GROUP_SCAN_INCL(func, type_abbr, type, op)                                              \
DEFN_WORK_GROUP_SCAN_EXCL(func, type_abbr, type, op, identity)                                    \
                                                                                                  \
type  SPIRV_OVERLOADABLE SPIRV_BUILTIN(Group##func, _i32_i32_##type_abbr, )(int Execution, int Operation, type X) \
{                                                                                                 \
    if (Execution == Workgroup)                                                                   \
    {                                                                                             \
        switch(Operation){                                                                        \
            case GroupOperationReduce:                                                            \
                return __builtin_IB_WorkGroupReduce_##func##_##type_abbr(X);                      \
                break;                                                                            \
            case GroupOperationInclusiveScan:                                                     \
                return __builtin_IB_WorkGroupScanInclusive_##func##_##type_abbr(X);               \
                break;                                                                            \
            case GroupOperationExclusiveScan:                                                     \
                return __builtin_IB_WorkGroupScanExclusive_##func##_##type_abbr(X);               \
                break;                                                                            \
            default:                                                                              \
                break;                                                                            \
        }                                                                                         \
    }                                                                                             \
    else if (Execution == Subgroup)                                                               \
    {                                                                                             \
        if (sizeof(X) < 8 || BIF_FLAG_CTRL_N(UseNative64Bit##type_gen##Builtin))                     \
        {                                                                                         \
            switch(Operation){                                                                    \
            case GroupOperationReduce:                                                            \
                return __builtin_IB_sub_group_reduce_##func##_##type_abbr(X);                     \
                break;                                                                            \
            case GroupOperationInclusiveScan:                                                     \
                return op(X, __builtin_IB_sub_group_scan_##func##_##type_abbr(X));                \
                break;                                                                            \
            case GroupOperationExclusiveScan:                                                     \
                return __builtin_IB_sub_group_scan_##func##_##type_abbr(X);                       \
                break;                                                                            \
            default:                                                                              \
                break;                                                                            \
            }                                                                                     \
        }                                                                                         \
        else                                                                                      \
        {                                                                                         \
            switch(Operation){                                                                    \
            case GroupOperationReduce:                                                            \
                return __builtin_IB_SubGroupReduce_##func##_##type_abbr(X);                       \
                break;                                                                            \
            case GroupOperationInclusiveScan:                                                     \
                return __builtin_IB_SubGroupScanInclusive_##func##_##type_abbr(X);                \
                break;                                                                            \
            case GroupOperationExclusiveScan:                                                     \
                return __builtin_IB_SubGroupScanExclusive_##func##_##type_abbr(X);                \
                break;                                                                            \
            default:                                                                              \
                break;                                                                            \
            }                                                                                     \
        }                                                                                         \
    }                                                                                             \
                                                                                                  \
    return 0;                                                                                     \
}                                                                                                 \
                                                                                                           \
type __builtin_IB_WorkGroupReduce_WI0_##func##_##type_abbr(type X)                                         \
{                                                                                                          \
    type sg_x = SPIRV_BUILTIN(Group##func, _i32_i32_##type_abbr, )(Subgroup, GroupOperationReduce, X);     \
    GET_SAFE_MEMPOOL_PTR(scratch, type, true, 0)                                                           \
    uint sg_id = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupId, , )();                                             \
    uint num_sg = SPIRV_BUILTIN_NO_OP(BuiltInNumSubgroups, , )();                                          \
    uint sg_lid = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();                             \
    uint sg_size = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupSize, , )();                                         \
    uint sg_max_size = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupMaxSize, , )();                                  \
                                                                                                           \
    if (sg_lid == 0) {                                                                                     \
        scratch[sg_id] = sg_x;                                                                             \
    }                                                                                                      \
    SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Workgroup, 0, AcquireRelease | WorkgroupMemory);         \
                                                                                                           \
    uint global_id = sg_id * sg_max_size + sg_lid;                                                         \
    uint values_num = num_sg;                                                                              \
    while(values_num > sg_max_size) {                                                                      \
        uint max_id = ((values_num + sg_max_size - 1) / sg_max_size) * sg_max_size;                        \
        type value = global_id < values_num ? scratch[global_id] : identity;                               \
        SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Workgroup, 0, AcquireRelease | WorkgroupMemory);     \
        if (global_id < max_id) {                                                                          \
            sg_x = SPIRV_BUILTIN(Group##func, _i32_i32_##type_abbr, )(Subgroup, GroupOperationReduce, value);\
            if (sg_lid == 0) {                                                                             \
                scratch[sg_id] = sg_x;                                                                     \
            }                                                                                              \
        }                                                                                                  \
        values_num = max_id / sg_max_size;                                                                 \
        SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Workgroup, 0, AcquireRelease | WorkgroupMemory);     \
    }                                                                                                      \
                                                                                                           \
    type result;                                                                                           \
    if (sg_id == 0) {                                                                                      \
        type value = sg_lid < values_num ? scratch[sg_lid] : identity;                                     \
        result = SPIRV_BUILTIN(Group##func, _i32_i32_##type_abbr, )(Subgroup, GroupOperationReduce, value); \
    }                                                                                                      \
    return result;                                                                                         \
}

#define DEFN_UNIFORM_GROUP_FUNC(func, type, type_gen, type_abbr, op, identity) \
DEFN_UNIFORM_GROUP_FUNC_BASE(func, type, type_gen, type_abbr, op, identity, )

#define DEFN_UNIFORM_GROUP_FUNC_UNSIGNED(func, signed_type, type_gen, type_abbr, op, identity) \
DEFN_UNIFORM_GROUP_FUNC_BASE(func, u##signed_type, type_gen, type_abbr, op, identity, as_##signed_type)

// ---- Add ----
DEFN_UNIFORM_GROUP_FUNC(IAdd, char,   Int,   i8,  __intel_add, 0)
DEFN_UNIFORM_GROUP_FUNC(IAdd, short,  Int,   i16, __intel_add, 0)
DEFN_UNIFORM_GROUP_FUNC(IAdd, int,    Int,   i32, __intel_add, 0)
DEFN_UNIFORM_GROUP_FUNC(IAdd, long,   Int,   i64, __intel_add, 0)
DEFN_UNIFORM_GROUP_FUNC(FAdd, half,   Float, f16, __intel_add, 0)
DEFN_UNIFORM_GROUP_FUNC(FAdd, float,  Float, f32, __intel_add, 0)
#if defined(cl_khr_fp64)
DEFN_UNIFORM_GROUP_FUNC(FAdd, double, Float, f64, __intel_add, 0)
#endif
// ---- Min ----
DEFN_UNIFORM_GROUP_FUNC(FMin, half,   Float, f16, SPIRV_OCL_BUILTIN(fmin, _f16_f16, ), INFINITY)
DEFN_UNIFORM_GROUP_FUNC(FMin, float,  Float, f32, SPIRV_OCL_BUILTIN(fmin, _f32_f32, ), INFINITY)
#if defined(cl_khr_fp64)
DEFN_UNIFORM_GROUP_FUNC(FMin, double, Float, f64, SPIRV_OCL_BUILTIN(fmin, _f64_f64, ), INFINITY)
#endif
DEFN_UNIFORM_GROUP_FUNC(SMin, char,   Int, i8,  SPIRV_OCL_BUILTIN(s_min, _i8_i8, ),   CHAR_MAX)
DEFN_UNIFORM_GROUP_FUNC(SMin, short,  Int, i16, SPIRV_OCL_BUILTIN(s_min, _i16_i16, ), SHRT_MAX)
DEFN_UNIFORM_GROUP_FUNC(SMin, int,    Int, i32, SPIRV_OCL_BUILTIN(s_min, _i32_i32, ), INT_MAX)
DEFN_UNIFORM_GROUP_FUNC(SMin, long,   Int, i64, SPIRV_OCL_BUILTIN(s_min, _i64_i64, ), LONG_MAX)
DEFN_UNIFORM_GROUP_FUNC_UNSIGNED(UMin, char,  Int,  i8, SPIRV_OCL_BUILTIN(u_min, _i8_i8, ),   UCHAR_MAX)
DEFN_UNIFORM_GROUP_FUNC_UNSIGNED(UMin, short, Int, i16, SPIRV_OCL_BUILTIN(u_min, _i16_i16, ), USHRT_MAX)
DEFN_UNIFORM_GROUP_FUNC_UNSIGNED(UMin, int,   Int, i32, SPIRV_OCL_BUILTIN(u_min, _i32_i32, ), UINT_MAX)
DEFN_UNIFORM_GROUP_FUNC_UNSIGNED(UMin, long,  Int, i64, SPIRV_OCL_BUILTIN(u_min, _i64_i64, ), ULONG_MAX)
// ---- Max ----
DEFN_UNIFORM_GROUP_FUNC(FMax, half,   Float, f16, SPIRV_OCL_BUILTIN(fmax, _f16_f16, ), -INFINITY)
DEFN_UNIFORM_GROUP_FUNC(FMax, float,  Float, f32, SPIRV_OCL_BUILTIN(fmax, _f32_f32, ), -INFINITY)
#if defined(cl_khr_fp64)
DEFN_UNIFORM_GROUP_FUNC(FMax, double, Float, f64, SPIRV_OCL_BUILTIN(fmax, _f64_f64, ), -INFINITY)
#endif
DEFN_UNIFORM_GROUP_FUNC(SMax, char,  Int, i8,  SPIRV_OCL_BUILTIN(s_max, _i8_i8, ),   CHAR_MIN)
DEFN_UNIFORM_GROUP_FUNC(SMax, short, Int, i16, SPIRV_OCL_BUILTIN(s_max, _i16_i16, ), SHRT_MIN)
DEFN_UNIFORM_GROUP_FUNC(SMax, int,   Int, i32, SPIRV_OCL_BUILTIN(s_max, _i32_i32, ), INT_MIN)
DEFN_UNIFORM_GROUP_FUNC(SMax, long,  Int, i64, SPIRV_OCL_BUILTIN(s_max, _i64_i64, ), LONG_MIN)
DEFN_UNIFORM_GROUP_FUNC_UNSIGNED(UMax, char,  Int, i8,  SPIRV_OCL_BUILTIN(u_max, _i8_i8, ),   0)
DEFN_UNIFORM_GROUP_FUNC_UNSIGNED(UMax, short, Int, i16, SPIRV_OCL_BUILTIN(u_max, _i16_i16, ), 0)
DEFN_UNIFORM_GROUP_FUNC_UNSIGNED(UMax, int,   Int, i32, SPIRV_OCL_BUILTIN(u_max, _i32_i32, ), 0)
DEFN_UNIFORM_GROUP_FUNC_UNSIGNED(UMax, long,  Int, i64, SPIRV_OCL_BUILTIN(u_max, _i64_i64, ), 0)
// ---- Mul ----
DEFN_UNIFORM_GROUP_FUNC(IMulKHR, char,   Int,   i8,  __intel_mul, 1)
DEFN_UNIFORM_GROUP_FUNC(IMulKHR, short,  Int,   i16, __intel_mul, 1)
DEFN_UNIFORM_GROUP_FUNC(IMulKHR, int,    Int,   i32, __intel_mul, 1)
DEFN_UNIFORM_GROUP_FUNC(IMulKHR, long,   Int,   i64, __intel_mul, 1)
DEFN_UNIFORM_GROUP_FUNC(FMulKHR, half,   Float, f16, __intel_mul, 1)
DEFN_UNIFORM_GROUP_FUNC(FMulKHR, float,  Float, f32, __intel_mul, 1)
#if defined(cl_khr_fp64)
DEFN_UNIFORM_GROUP_FUNC(FMulKHR, double, Float, f64, __intel_mul, 1)
#endif
// ---- Bitwise Operations ----
DEFN_UNIFORM_GROUP_FUNC(BitwiseAndKHR, char,  Int, i8,  __intel_and, 0xFF)
DEFN_UNIFORM_GROUP_FUNC(BitwiseAndKHR, short, Int, i16, __intel_and, 0xFFFF)
DEFN_UNIFORM_GROUP_FUNC(BitwiseAndKHR, int,   Int, i32, __intel_and, 0xFFFFFFFF)
DEFN_UNIFORM_GROUP_FUNC(BitwiseAndKHR, long,  Int, i64, __intel_and, 0xFFFFFFFFFFFFFFFF)

DEFN_UNIFORM_GROUP_FUNC(BitwiseOrKHR, char,  Int, i8,  __intel_or, 0)
DEFN_UNIFORM_GROUP_FUNC(BitwiseOrKHR, short, Int, i16, __intel_or, 0)
DEFN_UNIFORM_GROUP_FUNC(BitwiseOrKHR, int,   Int, i32, __intel_or, 0)
DEFN_UNIFORM_GROUP_FUNC(BitwiseOrKHR, long,  Int, i64, __intel_or, 0)

DEFN_UNIFORM_GROUP_FUNC(BitwiseXorKHR, char,  Int, i8,  __intel_xor, 0)
DEFN_UNIFORM_GROUP_FUNC(BitwiseXorKHR, short, Int, i16, __intel_xor, 0)
DEFN_UNIFORM_GROUP_FUNC(BitwiseXorKHR, int,   Int, i32, __intel_xor, 0)
DEFN_UNIFORM_GROUP_FUNC(BitwiseXorKHR, long,  Int, i64, __intel_xor, 0)
// ---- Logical Operations ----
DEFN_UNIFORM_GROUP_FUNC(LogicalAndKHR, bool, Int, i1, __intel_and, 1)
DEFN_UNIFORM_GROUP_FUNC(LogicalOrKHR,  bool, Int, i1, __intel_or,  0)
DEFN_UNIFORM_GROUP_FUNC(LogicalXorKHR, bool, Int, i1, __intel_xor, 0)


#if defined(cl_khr_subgroup_non_uniform_arithmetic) || defined(cl_khr_subgroup_clustered_reduce)
#define DEFN_SUB_GROUP_REDUCE_NON_UNIFORM(type, type_abbr, op, identity, X, signed_cast)                 \
{                                                                                                        \
    uint activeChannels = __builtin_IB_WaveBallot(true);                                                 \
    uint firstActive = SPIRV_OCL_BUILTIN(ctz, _i32, )(as_int(activeChannels));                           \
                                                                                                         \
    type result = identity;                                                                              \
    while (activeChannels)                                                                               \
    {                                                                                                    \
        uint activeId = SPIRV_OCL_BUILTIN(ctz, _i32, )(as_int(activeChannels));                          \
                                                                                                         \
        type value = intel_sub_group_shuffle(X, activeId);                                               \
        result = op(value, result);                                                                      \
                                                                                                         \
        uint disable = 1 << activeId;                                                                    \
        activeChannels ^= disable;                                                                       \
    }                                                                                                    \
                                                                                                         \
    int3 vec3;                                                                                           \
    vec3.s0 = firstActive;                                                                               \
    X = SPIRV_BUILTIN(GroupBroadcast, _i32_##type_abbr##_v3i32, )(Subgroup, signed_cast(result), vec3);  \
}

#define DEFN_SUB_GROUP_SCAN_INCL_NON_UNIFORM(type, type_abbr, op, identity, X)                      \
{                                                                                                   \
    uint sglid = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();                       \
    uint activeChannels = __builtin_IB_WaveBallot(true);                                            \
    uint activeId = SPIRV_OCL_BUILTIN(ctz, _i32, )(as_int(activeChannels));                         \
    activeChannels ^= 1 << activeId;                                                                \
    while (activeChannels)                                                                          \
    {                                                                                               \
        type value = intel_sub_group_shuffle(X, activeId);                                          \
        activeId = SPIRV_OCL_BUILTIN(ctz, _i32, )(as_int(activeChannels));                          \
        if (sglid == activeId)                                                                      \
            X = op(value, X);                                                                       \
        activeChannels ^= 1 << activeId;                                                            \
    }                                                                                               \
}

#define DEFN_SUB_GROUP_SCAN_EXCL_NON_UNIFORM(type, type_abbr, op, identity, X)                       \
{                                                                                                    \
    uint sglid = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();                        \
    uint activeChannels = __builtin_IB_WaveBallot(true);                                             \
    type result = identity;                                                                          \
    while (activeChannels)                                                                           \
    {                                                                                                \
        uint activeId = SPIRV_OCL_BUILTIN(ctz, _i32, )(as_int(activeChannels));                      \
        if (sglid == activeId)                                                                       \
        {                                                                                            \
            type value = X;                                                                          \
            X = result;                                                                              \
            result = op(result, value);                                                              \
        }                                                                                            \
        result = sub_group_shuffle(result, activeId);                                                \
        activeChannels ^= 1 << activeId;                                                             \
    }                                                                                                \
}

#define DEFN_SUB_GROUP_CLUSTERED_REDUCE(type, type_abbr, op, identity, X, ClusterSize, signed_cast)       \
{                                                                                                         \
    uint activeChannels = __builtin_IB_WaveBallot(true);                                                  \
    while (activeChannels != 0)                                                                           \
    {                                                                                                     \
        uint clusterBallot = activeChannels;                                                              \
        for (uint Counter = 0; Counter < ClusterSize; Counter++)                                          \
        {                                                                                                 \
            activeChannels &= activeChannels - 1;                                                         \
        }                                                                                                 \
        clusterBallot ^= activeChannels;                                                                  \
        uint active = SPIRV_BUILTIN(GroupNonUniformInverseBallot, _i32_v4i32, )(Subgroup, clusterBallot); \
        if (active)                                                                                       \
        {                                                                                                 \
            DEFN_SUB_GROUP_REDUCE_NON_UNIFORM(type, type_abbr, op, identity, X, signed_cast)              \
        }                                                                                                 \
    }                                                                                                     \
}

#define SUB_GROUP_SWITCH_NON_UNIFORM(type, type_abbr, op, identity, X, Operation, ClusterSize, signed_cast) \
{                                                                                                           \
    switch (Operation){                                                                                     \
        case GroupOperationReduce:                                                                          \
            DEFN_SUB_GROUP_REDUCE_NON_UNIFORM(type, type_abbr, op, identity, X, signed_cast)                \
            break;                                                                                          \
        case GroupOperationInclusiveScan:                                                                   \
            DEFN_SUB_GROUP_SCAN_INCL_NON_UNIFORM(type, type_abbr, op, identity, X)                          \
            break;                                                                                          \
        case GroupOperationExclusiveScan:                                                                   \
            DEFN_SUB_GROUP_SCAN_EXCL_NON_UNIFORM(type, type_abbr, op, identity, X)                          \
            break;                                                                                          \
        case GroupOperationClusteredReduce:                                                                 \
            DEFN_SUB_GROUP_CLUSTERED_REDUCE(type, type_abbr, op, identity, X, ClusterSize, signed_cast)     \
            break;                                                                                          \
        default:                                                                                            \
            return 0;                                                                                       \
            break;                                                                                          \
    }                                                                                                       \
}

// ClusterSize is an optional parameter
#define DEFN_NON_UNIFORM_GROUP_FUNC_BASE(func, type, type_gen, type_abbr, op, identity, signed_cast)                                 \
type  SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniform##func, _i32_i32_##type_abbr##_i32, )(int Execution, int Operation, type X, uint ClusterSize) \
{                                                                                                                                    \
    if (Execution == Subgroup)                                                                                                       \
    {                                                                                                                                \
        if (sizeof(X) < 8 || BIF_FLAG_CTRL_N(UseNative64Bit##type_gen##Builtin))                                                        \
        {                                                                                                                            \
            if (Operation == GroupOperationReduce)                                                                                   \
            {                                                                                                                        \
                return __builtin_IB_sub_group_reduce_##func##_##type_abbr(X);                                                        \
            }                                                                                                                        \
            else if (Operation == GroupOperationInclusiveScan)                                                                       \
            {                                                                                                                        \
                return op(X, __builtin_IB_sub_group_scan_##func##_##type_abbr(X));                                                   \
            }                                                                                                                        \
            else if (Operation == GroupOperationExclusiveScan)                                                                       \
            {                                                                                                                        \
                return __builtin_IB_sub_group_scan_##func##_##type_abbr(X);                                                          \
            }                                                                                                                        \
            else if (Operation == GroupOperationClusteredReduce)                                                                     \
            {                                                                                                                        \
                return __builtin_IB_sub_group_clustered_reduce_##func##_##type_abbr(X, ClusterSize);                                 \
            }                                                                                                                        \
        }                                                                                                                            \
        else {                                                                                                                       \
            SUB_GROUP_SWITCH_NON_UNIFORM(type, type_abbr, op, identity, X, Operation, ClusterSize, signed_cast)                      \
            return X;                                                                                                                \
        }                                                                                                                            \
        return 0;                                                                                                                    \
    }                                                                                                                                \
    else                                                                                                                             \
    {                                                                                                                                \
        return 0;                                                                                                                    \
    }                                                                                                                                \
}                                                                                                                                    \
type  SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniform##func, _i32_i32_##type_abbr, )(int Execution, int Operation, type X)          \
{                                                                                                                                    \
    return SPIRV_BUILTIN(GroupNonUniform##func, _i32_i32_##type_abbr##_i32, )(Execution, Operation, X, 0);                           \
}

#define DEFN_NON_UNIFORM_GROUP_FUNC(func, type, type_gen, type_abbr, op, identity) \
DEFN_NON_UNIFORM_GROUP_FUNC_BASE(func, type, type_gen, type_abbr, op, identity, )

#define DEFN_NON_UNIFORM_GROUP_FUNC_UNSIGNED(func, signed_type, type_gen, type_abbr, op, identity) \
DEFN_NON_UNIFORM_GROUP_FUNC_BASE(func, u##signed_type, type_gen, type_abbr, op, identity, as_##signed_type)

// OpGroupNonUniformIAdd, OpGroupNonUniformFAdd
DEFN_NON_UNIFORM_GROUP_FUNC(IAdd, char,  Int,   i8,  __intel_add, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(IAdd, short, Int,   i16, __intel_add, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(IAdd, int,   Int,   i32, __intel_add, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(IAdd, long,  Int,   i64, __intel_add, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(FAdd, float,  Float, f32, __intel_add, 0)
#if defined(cl_khr_fp64)
DEFN_NON_UNIFORM_GROUP_FUNC(FAdd, double, Float, f64, __intel_add, 0)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_GROUP_FUNC(FAdd, half,   Float, f16, __intel_add, 0)
#endif // defined(cl_khr_fp16)

// OpGroupNonUniformSMin, OpGroupNonUniformUMin, OpGroupNonUniformFMin
DEFN_NON_UNIFORM_GROUP_FUNC(SMin, char,   Int,   i8,  SPIRV_OCL_BUILTIN(s_min, _i8_i8, ),   CHAR_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC(SMin, short,  Int,   i16, SPIRV_OCL_BUILTIN(s_min, _i16_i16, ), SHRT_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC(SMin, int,    Int,   i32, SPIRV_OCL_BUILTIN(s_min, _i32_i32, ), INT_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC(SMin, long,   Int,   i64, SPIRV_OCL_BUILTIN(s_min, _i64_i64, ), LONG_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC(FMin, float,  Float, f32, SPIRV_OCL_BUILTIN(fmin, _f32_f32, ),  INFINITY)
DEFN_NON_UNIFORM_GROUP_FUNC_UNSIGNED(UMin, char,  Int, i8,  SPIRV_OCL_BUILTIN(u_min, _i8_i8, ),   UCHAR_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC_UNSIGNED(UMin, short, Int, i16, SPIRV_OCL_BUILTIN(u_min, _i16_i16, ), USHRT_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC_UNSIGNED(UMin, int,   Int, i32, SPIRV_OCL_BUILTIN(u_min, _i32_i32, ), UINT_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC_UNSIGNED(UMin, long,  Int, i64, SPIRV_OCL_BUILTIN(u_min, _i64_i64, ), ULONG_MAX)
#if defined(cl_khr_fp64)
DEFN_NON_UNIFORM_GROUP_FUNC(FMin, double, Float, f64, SPIRV_OCL_BUILTIN(fmin, _f64_f64, ),  INFINITY)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_GROUP_FUNC(FMin, half,   Float, f16, SPIRV_OCL_BUILTIN(fmin, _f16_f16, ),  INFINITY)
#endif // defined(cl_khr_fp16)

// OpGroupNonUniformSMax, OpGroupNonUniformUMax, OpGroupNonUniformFMax
DEFN_NON_UNIFORM_GROUP_FUNC(SMax, char,   Int,   i8,  SPIRV_OCL_BUILTIN(s_max, _i8_i8, ),   CHAR_MIN)
DEFN_NON_UNIFORM_GROUP_FUNC(SMax, short,  Int,   i16, SPIRV_OCL_BUILTIN(s_max, _i16_i16, ), SHRT_MIN)
DEFN_NON_UNIFORM_GROUP_FUNC(SMax, int,    Int,   i32, SPIRV_OCL_BUILTIN(s_max, _i32_i32, ), INT_MIN)
DEFN_NON_UNIFORM_GROUP_FUNC(SMax, long,   Int,   i64, SPIRV_OCL_BUILTIN(s_max, _i64_i64, ), LONG_MIN)
DEFN_NON_UNIFORM_GROUP_FUNC(FMax, float,  Float, f32, SPIRV_OCL_BUILTIN(fmax, _f32_f32, ), -INFINITY)
DEFN_NON_UNIFORM_GROUP_FUNC_UNSIGNED(UMax, char,  Int, i8,  SPIRV_OCL_BUILTIN(u_max, _i8_i8, ),   0)
DEFN_NON_UNIFORM_GROUP_FUNC_UNSIGNED(UMax, short, Int, i16, SPIRV_OCL_BUILTIN(u_max, _i16_i16, ), 0)
DEFN_NON_UNIFORM_GROUP_FUNC_UNSIGNED(UMax, int,   Int, i32, SPIRV_OCL_BUILTIN(u_max, _i32_i32, ), 0)
DEFN_NON_UNIFORM_GROUP_FUNC_UNSIGNED(UMax, long,  Int, i64, SPIRV_OCL_BUILTIN(u_max, _i64_i64, ), 0)
#if defined(cl_khr_fp64)
DEFN_NON_UNIFORM_GROUP_FUNC(FMax, double, Float, f64, SPIRV_OCL_BUILTIN(fmax, _f64_f64, ), -INFINITY)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_GROUP_FUNC(FMax, half,   Float, f16, SPIRV_OCL_BUILTIN(fmax, _f16_f16, ), -INFINITY)
#endif // defined(cl_khr_fp16)

// OpGroupNonUniformIMul, OpGroupNonUniformFMul
DEFN_NON_UNIFORM_GROUP_FUNC(IMul, char,  Int,   i8,  __intel_mul, 1)
DEFN_NON_UNIFORM_GROUP_FUNC(IMul, short, Int,   i16, __intel_mul, 1)
DEFN_NON_UNIFORM_GROUP_FUNC(IMul, int,   Int,   i32, __intel_mul, 1)
DEFN_NON_UNIFORM_GROUP_FUNC(IMul, long,  Int,   i64, __intel_mul, 1)
DEFN_NON_UNIFORM_GROUP_FUNC(FMul, float,  Float, f32, __intel_mul, 1)
#if defined(cl_khr_fp64)
DEFN_NON_UNIFORM_GROUP_FUNC(FMul, double, Float, f64, __intel_mul, 1)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_GROUP_FUNC(FMul, half,   Float, f16, __intel_mul, 1)
#endif // defined(cl_khr_fp16)

// OpGroupNonUniformBitwiseAnd, OpGroupNonUniformBitwiseOr, OpGroupNonUniformBitwiseXor
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseAnd, char,  Int, i8,  __intel_and, 0xFF)
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseAnd, short, Int, i16, __intel_and, 0xFFFF)
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseAnd, int,   Int, i32, __intel_and, 0xFFFFFFFF)
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseAnd, long,  Int, i64, __intel_and, 0xFFFFFFFFFFFFFFFF)

DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseOr, char,  Int, i8,  __intel_or, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseOr, short, Int, i16, __intel_or, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseOr, int,   Int, i32, __intel_or, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseOr, long,  Int, i64, __intel_or, 0)

DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseXor, char,  Int, i8,  __intel_xor, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseXor, short, Int, i16, __intel_xor, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseXor, int,   Int, i32, __intel_xor, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseXor, long,  Int, i64, __intel_xor, 0)

// OpGroupNonUniformLogicalAnd, OpGroupNonUniformLogicalOr, OpGroupNonUniformLogicalXor
DEFN_NON_UNIFORM_GROUP_FUNC(LogicalAnd, bool, Int, i1, __intel_and, 1)
DEFN_NON_UNIFORM_GROUP_FUNC(LogicalOr,  bool, Int, i1, __intel_or,  0)
DEFN_NON_UNIFORM_GROUP_FUNC(LogicalXor, bool, Int, i1, __intel_xor, 0)
#endif // defined(cl_khr_subgroup_non_uniform_arithmetic) || defined(cl_khr_subgroup_clustered_reduce)

#if defined(cl_khr_subgroup_shuffle)
char SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffle, _i32_i8_i32, )(int Execution, char x, uint c)
{
    if (Execution == Subgroup)
    {
        return as_uchar(__builtin_IB_simd_shuffle_c(as_char(x), c));
    }
    return 0;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffle, _i32_i16_i32, )(int Execution, short x, uint c)
{
    if (Execution == Subgroup)
    {
        return __builtin_IB_simd_shuffle_us(x, c);
    }
    return 0;
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffle, _i32_i32_i32, )(int Execution, int x, uint c)
{
    if (Execution == Subgroup)
    {
        return __builtin_IB_simd_shuffle(x, c);
    }
    return 0;
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffle, _i32_i64_i32, )(int Execution, long x, uint c)
{
    if (Execution == Subgroup)
    {
        uint2 X = as_uint2(x);
        uint2 result = (uint2)(__builtin_IB_simd_shuffle(X.s0, c),
                               __builtin_IB_simd_shuffle(X.s1, c));
        return as_ulong(result);
    }
    return 0;
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffle, _i32_f32_i32, )(int Execution, float x, uint c)
{
    if (Execution == Subgroup)
    {
        return __builtin_IB_simd_shuffle_f(x, c);
    }
    return 0;
}

#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffle, _i32_f64_i32, )(int Execution, double x, uint c)
{
    if (Execution == Subgroup)
    {
        if(BIF_FLAG_CTRL_GET(UseOOBChecks))
        {
            c = c & (get_max_sub_group_size() - 1);
        }
        return __builtin_IB_simd_shuffle_df(x, c);
    }
    return 0;
}
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffle, _i32_f16_i32, )(int Execution, half x, uint c)
{
    if (Execution == Subgroup)
    {
        if(BIF_FLAG_CTRL_GET(UseOOBChecks))
        {
            c = c & (get_max_sub_group_size() - 1);
        }
        return __builtin_IB_simd_shuffle_h(x, c);
    }
    return 0;
}
#endif // defined(cl_khr_fp16)

GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffle, char,   int, char,   uint, i32, i8,  i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffle, short,  int, short,  uint, i32, i16, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffle, int,    int, int,    uint, i32, i32, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffle, long,   int, long,   uint, i32, i64, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffle, float,  int, float,  uint, i32, f32, i32)
#if defined(cl_khr_fp64)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffle, double, int, double, uint, i32, f64, i32)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffle, half,   int, half,   uint, i32, f16, i32)
#endif // defined(cl_khr_fp16)

#define DEFN_SUB_GROUP_SHUFFLE_XOR(TYPE, TYPE_ABBR)                                                             \
TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffleXor, _i32_##TYPE_ABBR##_i32, )(int Execution, TYPE x, uint c)  \
{                                                                                                               \
    c = get_sub_group_local_id() ^ c;                                                                           \
    return SPIRV_BUILTIN(GroupNonUniformShuffle, _i32_##TYPE_ABBR##_i32, )(Execution, x, c);                    \
}                                                                                                               \
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffleXor, TYPE, int, TYPE, uint, i32, TYPE_ABBR, i32)


DEFN_SUB_GROUP_SHUFFLE_XOR(char,   i8)
DEFN_SUB_GROUP_SHUFFLE_XOR(short,  i16)
DEFN_SUB_GROUP_SHUFFLE_XOR(int,    i32)
DEFN_SUB_GROUP_SHUFFLE_XOR(long,   i64)
DEFN_SUB_GROUP_SHUFFLE_XOR(float,  f32)
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_SHUFFLE_XOR(double, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_SUB_GROUP_SHUFFLE_XOR(half,   f16)
#endif // defined(cl_khr_fp16)
#endif // defined(cl_khr_subgroup_shuffle)

#if defined(cl_khr_subgroup_shuffle_relative)
// Shuffle down functions
char SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffleDown, _i32_i8_i32, )(int Execution, char x, uint c)
{
    if (Execution == Subgroup)
    {
        if(BIF_FLAG_CTRL_GET(UseOOBChecks))
        {
            c = c & (get_max_sub_group_size() - 1);
        }
        return __builtin_IB_simd_shuffle_down_uc(x, 0, c);
    }
    return 0;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffleDown, _i32_i16_i32, )(int Execution, short x, uint c)
{
    if (Execution == Subgroup)
    {
        if(BIF_FLAG_CTRL_GET(UseOOBChecks))
        {
            c = c & (get_max_sub_group_size() - 1);
        }
        return __builtin_IB_simd_shuffle_down_us(x, 0, c);
    }
    return 0;
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffleDown, _i32_i32_i32, )(int Execution, int x, uint c)
{
    if (Execution == Subgroup)
    {
        if(BIF_FLAG_CTRL_GET(UseOOBChecks))
        {
            c = c & (get_max_sub_group_size() - 1);
        }
        return __builtin_IB_simd_shuffle_down(x, 0, c);
    }
    return 0;
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffleDown, _i32_i64_i32, )(int Execution, long x, uint c)
{
    if (Execution == Subgroup)
    {
        if(BIF_FLAG_CTRL_GET(UseOOBChecks))
        {
            c = c & (get_max_sub_group_size() - 1);
        }
        uint2 X = as_uint2(x);
        uint2 result = (uint2)(__builtin_IB_simd_shuffle_down(X.s0, 0, c),
            __builtin_IB_simd_shuffle_down(X.s1, 0, c));
        return as_ulong(result);
    }
    return 0;
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffleDown, _i32_f32_i32, )(int Execution, float x, uint c)
{
    if (Execution == Subgroup)
    {
        if(BIF_FLAG_CTRL_GET(UseOOBChecks))
        {
            c = c & (get_max_sub_group_size() - 1);
        }
        return as_float(__builtin_IB_simd_shuffle_down(as_uint(x), 0, c));
    }
    return 0;
}

#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffleDown, _i32_f64_i32, )(int Execution, double x, uint c)
{
    if (Execution == Subgroup)
    {
        if(BIF_FLAG_CTRL_GET(UseOOBChecks))
        {
            c = c & (get_max_sub_group_size() - 1);
        }
        uint2 X = as_uint2(x);
        uint2 result = (uint2)(__builtin_IB_simd_shuffle_down(X.s0, 0, c),
            __builtin_IB_simd_shuffle_down(X.s1, 0, c));
        return as_double(result);
    }
    return 0;
}
#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffleDown, _i32_f16_i32, )(int Execution, half x, uint c)
{
    if (Execution == Subgroup)
    {
        if(BIF_FLAG_CTRL_GET(UseOOBChecks))
        {
            c = c & (get_max_sub_group_size() - 1);
        }
        return as_half(__builtin_IB_simd_shuffle_down_us(as_ushort(x), 0, c));
    }
    return 0;
}
#endif // defined(cl_khr_fp16)

GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffleDown, char,   int, char,   uint, i32, i8,  i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffleDown, short,  int, short,  uint, i32, i16, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffleDown, int,    int, int,    uint, i32, i32, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffleDown, long,   int, long,   uint, i32, i64, i32)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffleDown, float,  int, float,  uint, i32, f32, i32)
#if defined(cl_khr_fp64)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffleDown, double, int, double, uint, i32, f64, i32)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffleDown, half,   int, half,   uint, i32, f16, i32)
#endif // defined(cl_khr_fp16)


// Shuffle up functions
#define DEFN_NON_UNIFORM_SHUFFLE_UP(TYPE, TYPE_ABBR)                                                                        \
TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffleUp, _i32_##TYPE_ABBR##_i32, )(int Execution, TYPE x, uint c)    \
{                                                                                                                           \
    if (Execution == Subgroup)                                                                                              \
    {                                                                                                                       \
        if(BIF_FLAG_CTRL_GET(UseOOBChecks))                                                                                 \
        {                                                                                                                   \
            c = c & (get_max_sub_group_size() - 1);                                                                         \
        }                                                                                                                   \
        return intel_sub_group_shuffle_up((TYPE) 0, x, c);                                                                  \
    }                                                                                                                       \
    return 0;                                                                                                               \
}                                                                                                                           \
GENERATE_SPIRV_VECTOR_FUNCTIONS_3ARGS_SVS(GroupNonUniformShuffleUp, TYPE, int, TYPE, uint, i32, TYPE_ABBR, i32)

DEFN_NON_UNIFORM_SHUFFLE_UP(char,   i8)
DEFN_NON_UNIFORM_SHUFFLE_UP(short,  i16)
DEFN_NON_UNIFORM_SHUFFLE_UP(int,    i32)
DEFN_NON_UNIFORM_SHUFFLE_UP(long,   i64)
DEFN_NON_UNIFORM_SHUFFLE_UP(float,  f32)
#if defined(cl_khr_fp64)
DEFN_NON_UNIFORM_SHUFFLE_UP(double, f64)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_SHUFFLE_UP(half,   f16)
#endif // defined(cl_khr_fp16)
#endif // defined(cl_khr_subgroup_shuffle_relative)
