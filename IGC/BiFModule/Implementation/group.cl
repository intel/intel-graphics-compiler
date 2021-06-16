/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

 extern __constant int __UseNative64BitIntSubgroupBuiltin;
 extern __constant int __UseNative64BitFloatSubgroupBuiltin;

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

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i8_p3i8_i64_i64_i64, )(int Execution, global char *Destination, local char *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i8_p3i8_i32_i32_i64, )(int Execution, global char *Destination, local char *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i16_p3i16_i64_i64_i64, )(int Execution, global short *Destination, local short *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i16_p3i16_i32_i32_i64, )(int Execution, global short *Destination, local short *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i32_p3i32_i64_i64_i64, )(int Execution, global int *Destination, local int *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i32_p3i32_i32_i32_i64, )(int Execution, global int *Destination, local int *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i64_p3i64_i64_i64_i64, )(int Execution, global long *Destination, local long *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1i64_p3i64_i32_i32_i64, )(int Execution, global long *Destination, local long *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1f16_p3f16_i64_i64_i64, )(int Execution, global half *Destination, local half *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1f16_p3f16_i32_i32_i64, )(int Execution, global half *Destination, local half *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1f32_p3f32_i64_i64_i64, )(int Execution, global float *Destination, local float *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1f32_p3f32_i32_i32_i64, )(int Execution, global float *Destination, local float *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

#if defined(cl_khr_fp64)

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1f64_p3f64_i64_i64_i64, )(int Execution, global double *Destination, local double *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1f64_p3f64_i32_i32_i64, )(int Execution, global double *Destination, local double *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

#endif

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i8_p3v2i8_i64_i64_i64, )(int Execution, global char2 *Destination, local char2 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i8_p3v2i8_i32_i32_i64, )(int Execution, global char2 *Destination, local char2 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i8_p3v3i8_i64_i64_i64, )(int Execution, global char3 *Destination, local char3 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i8_p3v3i8_i32_i32_i64, )(int Execution, global char3 *Destination, local char3 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i8_p3v4i8_i64_i64_i64, )(int Execution, global char4 *Destination, local char4 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i8_p3v4i8_i32_i32_i64, )(int Execution, global char4 *Destination, local char4 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i8_p3v8i8_i64_i64_i64, )(int Execution, global char8 *Destination, local char8 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i8_p3v8i8_i32_i32_i64, )(int Execution, global char8 *Destination, local char8 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i8_p3v16i8_i64_i64_i64, )(int Execution, global char16 *Destination, local char16 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i8_p3v16i8_i32_i32_i64, )(int Execution, global char16 *Destination, local char16 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i16_p3v2i16_i64_i64_i64, )(int Execution, global short2 *Destination, local short2 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i16_p3v2i16_i32_i32_i64, )(int Execution, global short2 *Destination, local short2 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i16_p3v3i16_i64_i64_i64, )(int Execution, global short3 *Destination, local short3 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i16_p3v3i16_i32_i32_i64, )(int Execution, global short3 *Destination, local short3 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i16_p3v4i16_i64_i64_i64, )(int Execution, global short4 *Destination, local short4 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i16_p3v4i16_i32_i32_i64, )(int Execution, global short4 *Destination, local short4 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i16_p3v8i16_i64_i64_i64, )(int Execution, global short8 *Destination, local short8 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i16_p3v8i16_i32_i32_i64, )(int Execution, global short8 *Destination, local short8 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i16_p3v16i16_i64_i64_i64, )(int Execution, global short16 *Destination, local short16 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i16_p3v16i16_i32_i32_i64, )(int Execution, global short16 *Destination, local short16 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i32_p3v2i32_i64_i64_i64, )(int Execution, global int2 *Destination, local int2 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i32_p3v2i32_i32_i32_i64, )(int Execution, global int2 *Destination, local int2 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i32_p3v3i32_i64_i64_i64, )(int Execution, global int3 *Destination, local int3 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i32_p3v3i32_i32_i32_i64, )(int Execution, global int3 *Destination, local int3 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i32_p3v4i32_i64_i64_i64, )(int Execution, global int4 *Destination, local int4 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i32_p3v4i32_i32_i32_i64, )(int Execution, global int4 *Destination, local int4 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i32_p3v8i32_i64_i64_i64, )(int Execution, global int8 *Destination, local int8 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i32_p3v8i32_i32_i32_i64, )(int Execution, global int8 *Destination, local int8 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i32_p3v16i32_i64_i64_i64, )(int Execution, global int16 *Destination, local int16 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i32_p3v16i32_i32_i32_i64, )(int Execution, global int16 *Destination, local int16 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i64_p3v2i64_i64_i64_i64, )(int Execution, global long2 *Destination, local long2 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2i64_p3v2i64_i32_i32_i64, )(int Execution, global long2 *Destination, local long2 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i64_p3v3i64_i64_i64_i64, )(int Execution, global long3 *Destination, local long3 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3i64_p3v3i64_i32_i32_i64, )(int Execution, global long3 *Destination, local long3 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i64_p3v4i64_i64_i64_i64, )(int Execution, global long4 *Destination, local long4 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4i64_p3v4i64_i32_i32_i64, )(int Execution, global long4 *Destination, local long4 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i64_p3v8i64_i64_i64_i64, )(int Execution, global long8 *Destination, local long8 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8i64_p3v8i64_i32_i32_i64, )(int Execution, global long8 *Destination, local long8 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i64_p3v16i64_i64_i64_i64, )(int Execution, global long16 *Destination, local long16 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16i64_p3v16i64_i32_i32_i64, )(int Execution, global long16 *Destination, local long16 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2f16_p3v2f16_i64_i64_i64, )(int Execution, global half2 *Destination, local half2 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2f16_p3v2f16_i32_i32_i64, )(int Execution, global half2 *Destination, local half2 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3f16_p3v3f16_i64_i64_i64, )(int Execution, global half3 *Destination, local half3 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3f16_p3v3f16_i32_i32_i64, )(int Execution, global half3 *Destination, local half3 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4f16_p3v4f16_i64_i64_i64, )(int Execution, global half4 *Destination, local half4 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4f16_p3v4f16_i32_i32_i64, )(int Execution, global half4 *Destination, local half4 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8f16_p3v8f16_i64_i64_i64, )(int Execution, global half8 *Destination, local half8 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8f16_p3v8f16_i32_i32_i64, )(int Execution, global half8 *Destination, local half8 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16f16_p3v16f16_i64_i64_i64, )(int Execution, global half16 *Destination, local half16 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16f16_p3v16f16_i32_i32_i64, )(int Execution, global half16 *Destination, local half16 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2f32_p3v2f32_i64_i64_i64, )(int Execution, global float2 *Destination, local float2 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2f32_p3v2f32_i32_i32_i64, )(int Execution, global float2 *Destination, local float2 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3f32_p3v3f32_i64_i64_i64, )(int Execution, global float3 *Destination, local float3 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3f32_p3v3f32_i32_i32_i64, )(int Execution, global float3 *Destination, local float3 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4f32_p3v4f32_i64_i64_i64, )(int Execution, global float4 *Destination, local float4 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4f32_p3v4f32_i32_i32_i64, )(int Execution, global float4 *Destination, local float4 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8f32_p3v8f32_i64_i64_i64, )(int Execution, global float8 *Destination, local float8 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8f32_p3v8f32_i32_i32_i64, )(int Execution, global float8 *Destination, local float8 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16f32_p3v16f32_i64_i64_i64, )(int Execution, global float16 *Destination, local float16 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16f32_p3v16f32_i32_i32_i64, )(int Execution, global float16 *Destination, local float16 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

#if defined(cl_khr_fp64)

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2f64_p3v2f64_i64_i64_i64, )(int Execution, global double2 *Destination, local double2 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v2f64_p3v2f64_i32_i32_i64, )(int Execution, global double2 *Destination, local double2 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3f64_p3v3f64_i64_i64_i64, )(int Execution, global double3 *Destination, local double3 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v3f64_p3v3f64_i32_i32_i64, )(int Execution, global double3 *Destination, local double3 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4f64_p3v4f64_i64_i64_i64, )(int Execution, global double4 *Destination, local double4 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v4f64_p3v4f64_i32_i32_i64, )(int Execution, global double4 *Destination, local double4 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8f64_p3v8f64_i64_i64_i64, )(int Execution, global double8 *Destination, local double8 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v8f64_p3v8f64_i32_i32_i64, )(int Execution, global double8 *Destination, local double8 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16f64_p3v16f64_i64_i64_i64, )(int Execution, global double16 *Destination, local double16 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p1v16f64_p3v16f64_i32_i32_i64, )(int Execution, global double16 *Destination, local double16 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

#endif //defined(cl_khr_fp64)

//G2L


Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i8_p1i8_i64_i64_i64, )(int Execution, local char *Destination, global char *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i8_p1i8_i32_i32_i64, )(int Execution, local char *Destination, global char *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i16_p1i16_i64_i64_i64, )(int Execution, local short *Destination, global short *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i16_p1i16_i32_i32_i64, )(int Execution, local short *Destination, global short *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i32_p1i32_i64_i64_i64, )(int Execution, local int *Destination, global int *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i32_p1i32_i32_i32_i64, )(int Execution, local int *Destination, global int *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i64_p1i64_i64_i64_i64, )(int Execution, local long *Destination, global long *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3i64_p1i64_i32_i32_i64, )(int Execution, local long *Destination, global long *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3f16_p1f16_i64_i64_i64, )(int Execution, local half *Destination, global half *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3f16_p1f16_i32_i32_i64, )(int Execution, local half *Destination, global half *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3f32_p1f32_i64_i64_i64, )(int Execution, local float *Destination, global float *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3f32_p1f32_i32_i32_i64, )(int Execution, local float *Destination, global float *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

#if defined(cl_khr_fp64)

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3f64_p1f64_i64_i64_i64, )(int Execution, local double *Destination, global double *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3f64_p1f64_i32_i32_i64, )(int Execution, local double *Destination, global double *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

#endif

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i8_p1v2i8_i64_i64_i64, )(int Execution, local char2 *Destination, global char2 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i8_p1v2i8_i32_i32_i64, )(int Execution, local char2 *Destination, global char2 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i8_p1v3i8_i64_i64_i64, )(int Execution, local char3 *Destination, global char3 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i8_p1v3i8_i32_i32_i64, )(int Execution, local char3 *Destination, global char3 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i8_p1v4i8_i64_i64_i64, )(int Execution, local char4 *Destination, global char4 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i8_p1v4i8_i32_i32_i64, )(int Execution, local char4 *Destination, global char4 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i8_p1v8i8_i64_i64_i64, )(int Execution, local char8 *Destination, global char8 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i8_p1v8i8_i32_i32_i64, )(int Execution, local char8 *Destination, global char8 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i8_p1v16i8_i64_i64_i64, )(int Execution, local char16 *Destination, global char16 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i8_p1v16i8_i32_i32_i64, )(int Execution, local char16 *Destination, global char16 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i16_p1v2i16_i64_i64_i64, )(int Execution, local short2 *Destination, global short2 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i16_p1v2i16_i32_i32_i64, )(int Execution, local short2 *Destination, global short2 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i16_p1v3i16_i64_i64_i64, )(int Execution, local short3 *Destination, global short3 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i16_p1v3i16_i32_i32_i64, )(int Execution, local short3 *Destination, global short3 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i16_p1v4i16_i64_i64_i64, )(int Execution, local short4 *Destination, global short4 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i16_p1v4i16_i32_i32_i64, )(int Execution, local short4 *Destination, global short4 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i16_p1v8i16_i64_i64_i64, )(int Execution, local short8 *Destination, global short8 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i16_p1v8i16_i32_i32_i64, )(int Execution, local short8 *Destination, global short8 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i16_p1v16i16_i64_i64_i64, )(int Execution, local short16 *Destination, global short16 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i16_p1v16i16_i32_i32_i64, )(int Execution, local short16 *Destination, global short16 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i32_p1v2i32_i64_i64_i64, )(int Execution, local int2 *Destination, global int2 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i32_p1v2i32_i32_i32_i64, )(int Execution, local int2 *Destination, global int2 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i32_p1v3i32_i64_i64_i64, )(int Execution, local int3 *Destination, global int3 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i32_p1v3i32_i32_i32_i64, )(int Execution, local int3 *Destination, global int3 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i32_p1v4i32_i64_i64_i64, )(int Execution, local int4 *Destination, global int4 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i32_p1v4i32_i32_i32_i64, )(int Execution, local int4 *Destination, global int4 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i32_p1v8i32_i64_i64_i64, )(int Execution, local int8 *Destination, global int8 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i32_p1v8i32_i32_i32_i64, )(int Execution, local int8 *Destination, global int8 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i32_p1v16i32_i64_i64_i64, )(int Execution, local int16 *Destination, global int16 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i32_p1v16i32_i32_i32_i64, )(int Execution, local int16 *Destination, global int16 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i64_p1v2i64_i64_i64_i64, )(int Execution, local long2 *Destination, global long2 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2i64_p1v2i64_i32_i32_i64, )(int Execution, local long2 *Destination, global long2 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i64_p1v3i64_i64_i64_i64, )(int Execution, local long3 *Destination, global long3 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3i64_p1v3i64_i32_i32_i64, )(int Execution, local long3 *Destination, global long3 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i64_p1v4i64_i64_i64_i64, )(int Execution, local long4 *Destination, global long4 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4i64_p1v4i64_i32_i32_i64, )(int Execution, local long4 *Destination, global long4 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i64_p1v8i64_i64_i64_i64, )(int Execution, local long8 *Destination, global long8 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8i64_p1v8i64_i32_i32_i64, )(int Execution, local long8 *Destination, global long8 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i64_p1v16i64_i64_i64_i64, )(int Execution, local long16 *Destination, global long16 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16i64_p1v16i64_i32_i32_i64, )(int Execution, local long16 *Destination, global long16 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2f16_p1v2f16_i64_i64_i64, )(int Execution, local half2 *Destination, global half2 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2f16_p1v2f16_i32_i32_i64, )(int Execution, local half2 *Destination, global half2 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3f16_p1v3f16_i64_i64_i64, )(int Execution, local half3 *Destination, global half3 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3f16_p1v3f16_i32_i32_i64, )(int Execution, local half3 *Destination, global half3 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4f16_p1v4f16_i64_i64_i64, )(int Execution, local half4 *Destination, global half4 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4f16_p1v4f16_i32_i32_i64, )(int Execution, local half4 *Destination, global half4 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8f16_p1v8f16_i64_i64_i64, )(int Execution, local half8 *Destination, global half8 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8f16_p1v8f16_i32_i32_i64, )(int Execution, local half8 *Destination, global half8 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16f16_p1v16f16_i64_i64_i64, )(int Execution, local half16 *Destination, global half16 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16f16_p1v16f16_i32_i32_i64, )(int Execution, local half16 *Destination, global half16 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2f32_p1v2f32_i64_i64_i64, )(int Execution, local float2 *Destination, global float2 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2f32_p1v2f32_i32_i32_i64, )(int Execution, local float2 *Destination, global float2 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3f32_p1v3f32_i64_i64_i64, )(int Execution, local float3 *Destination, global float3 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3f32_p1v3f32_i32_i32_i64, )(int Execution, local float3 *Destination, global float3 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4f32_p1v4f32_i64_i64_i64, )(int Execution, local float4 *Destination, global float4 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4f32_p1v4f32_i32_i32_i64, )(int Execution, local float4 *Destination, global float4 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8f32_p1v8f32_i64_i64_i64, )(int Execution, local float8 *Destination, global float8 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8f32_p1v8f32_i32_i32_i64, )(int Execution, local float8 *Destination, global float8 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16f32_p1v16f32_i64_i64_i64, )(int Execution, local float16 *Destination, global float16 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16f32_p1v16f32_i32_i32_i64, )(int Execution, local float16 *Destination, global float16 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

#if defined(cl_khr_fp64)

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2f64_p1v2f64_i64_i64_i64, )(int Execution, local double2 *Destination, global double2 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v2f64_p1v2f64_i32_i32_i64, )(int Execution, local double2 *Destination, global double2 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3f64_p1v3f64_i64_i64_i64, )(int Execution, local double3 *Destination, global double3 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v3f64_p1v3f64_i32_i32_i64, )(int Execution, local double3 *Destination, global double3 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4f64_p1v4f64_i64_i64_i64, )(int Execution, local double4 *Destination, global double4 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v4f64_p1v4f64_i32_i32_i64, )(int Execution, local double4 *Destination, global double4 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8f64_p1v8f64_i64_i64_i64, )(int Execution, local double8 *Destination, global double8 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v8f64_p1v8f64_i32_i32_i64, )(int Execution, local double8 *Destination, global double8 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16f64_p1v16f64_i64_i64_i64, )(int Execution, local double16 *Destination, global double16 *Source, long NumElements, long Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAsyncCopy, _i32_p3v16f64_p1v16f64_i32_i32_i64, )(int Execution, local double16 *Destination, global double16 *Source, int NumElements, int Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

#endif // defined(cl_khr_fp64)


void SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupWaitEvents, _i32_i32_p0i64, )(int Execution, int NumEvents, private Event_t *EventsList)
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
void SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupWaitEvents, _i32_i32_p4i64, )(int Execution, int NumEvents, generic Event_t *EventsList)
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
        GET_MEMPOOL_PTR(tmp, int, false, 1)
        *tmp = 0;
        SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Execution, 0, AcquireRelease | WorkgroupMemory); // Wait for tmp to be initialized
        SPIRV_BUILTIN(AtomicOr, _p3i32_i32_i32_i32, )((local int*)tmp, Device, Relaxed, Predicate == 0); // Set to true if predicate is zero
        SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Execution, 0, AcquireRelease | WorkgroupMemory); // Wait for threads
        return (*tmp == 0); // Return true if none of them failed the test
    }
    else if (Execution == Subgroup)
    {
            int value = ( Predicate == 0 ) ? 1 : 0;
            value = SPIRV_BUILTIN(GroupIAdd, _i32_i32_i32, )(Subgroup, GroupOperationReduce, value );
            value = ( value == 0 ) ? 1 : 0;
            return value;
    }
    else
    {
         return 0;
    }
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupAny, _i32_i1, )(int Execution, bool Predicate)
{
    if (Execution == Workgroup)
    {
        GET_MEMPOOL_PTR(tmp, int, false, 1)
        *tmp = 0;
        SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Execution, 0, AcquireRelease | WorkgroupMemory); // Wait for tmp to be initialized
        SPIRV_BUILTIN(AtomicOr, _p3i32_i32_i32_i32, )((local int*)tmp, Device, Relaxed, Predicate != 0); // Set to true if predicate is non-zero
        SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Execution, 0, AcquireRelease | WorkgroupMemory);
        return *tmp; // Return true if any of them passed the test
    }
    else if (Execution == Subgroup)
    {
            int value = ( Predicate != 0 ) ? 1 : 0;
            value = SPIRV_BUILTIN(GroupIAdd, _i32_i32_i32, )(Subgroup, GroupOperationReduce, value );
            return value;
    }
    else
    {
         return 0;
    }
}

#if defined(cl_khr_subgroup_non_uniform_vote)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformElect, _i32, )(int Execution)
{
    if (Execution == Subgroup)
    {
        uint activeChannels = __builtin_IB_WaveBallot(true);
        uint firstActive = __builtin_spirv_OpenCL_ctz_i32(activeChannels);
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
        uint firstActive = __builtin_spirv_OpenCL_ctz_i32(activeChannels);                                                    \
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
    GET_MEMPOOL_PTR(tmp, type, false, 1)                                                       \
    if( (__intel_LocalInvocationId(0) == LocalId.s0) &                                            \
        (__intel_LocalInvocationId(1) == LocalId.s1) &                                            \
        (__intel_LocalInvocationId(2) == LocalId.s2) )                                            \
    {                                                                                   \
        *tmp = Value;                                                                   \
    }                                                                                   \
    SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Execution, 0, AcquireRelease | WorkgroupMemory);        \
    type ret = *tmp;                                                                    \
    SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Execution, 0, AcquireRelease | WorkgroupMemory);        \
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
        return __builtin_IB_simd_shuffle_b(Value, LocalId.s0);
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
        return __builtin_IB_simd_shuffle_b(Value, (uint)LocalId.s0);
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
        return __builtin_IB_simd_shuffle_c(Value, LocalId.s0);
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
        return __builtin_IB_simd_shuffle_c(Value, (uint)LocalId.s0);
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
        return as_ushort(__builtin_IB_simd_shuffle_h(as_half(Value), LocalId.s0));
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
        return as_ushort(__builtin_IB_simd_shuffle_h(as_half(Value), (uint)LocalId.s0));
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
        return __builtin_IB_simd_shuffle(Value, LocalId.s0);
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
        return __builtin_IB_simd_shuffle(Value, (uint)LocalId.s0);
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
        return ((((ulong)__builtin_IB_simd_shuffle(Value >> 32, LocalId.s0)) << 32 ) | __builtin_IB_simd_shuffle((uint)Value, LocalId.s0));
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
        return ((((ulong)__builtin_IB_simd_shuffle(Value >> 32, (uint)LocalId.s0)) << 32 ) | __builtin_IB_simd_shuffle((uint)Value, (uint)LocalId.s0));
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
        return __builtin_IB_simd_shuffle_h( Value, (uint)LocalId.s0 );
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
        return as_half2(__builtin_IB_simd_shuffle( (uint)(as_short(Value)), (uint)LocalId.s0 )).x;
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
        return __builtin_IB_simd_shuffle_f( Value, LocalId.s0 );
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
        return __builtin_IB_simd_shuffle_f( Value, (uint)LocalId.s0 );
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
        return __builtin_IB_simd_shuffle_df( Value, LocalId.s0 );
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
        return __builtin_IB_simd_shuffle_df( Value, (uint) LocalId.s0 );
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
    uint firstActive = __builtin_spirv_OpenCL_ctz_i32(chanEnable);
    int3 id = (int3)(firstActive, 0, 0);
    return SPIRV_BUILTIN(GroupBroadcast, _i32_i32_v3i32, )(Subgroup, as_int(Value), id);
}

float __builtin_spirv_OpSubgroupFirstInvocationKHR_f32(float Value)
{
    return as_float(__builtin_spirv_OpSubgroupFirstInvocationKHR_i32(as_uint(Value)));
}

#if defined(cl_khr_subgroup_ballot)
#define DEFN_NON_UNIFORM_BROADCAST_BASE(TYPE, TYPE_ABBR)                                                                      \
TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBroadcast, _i32_##TYPE_ABBR##_i32, )(int Execution, TYPE Value, uint Id) \
{                                                                                                                             \
    return SPIRV_BUILTIN(GroupBroadcast, _i32_##TYPE_ABBR##_i32, )(Execution, Value, as_int(Id));                             \
}                                                                                                                             \
TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBroadcastFirst, _i32_##TYPE_ABBR, )(int Execution, TYPE Value)           \
{                                                                                                                             \
    uint activeChannels = __builtin_IB_WaveBallot(true);                                                                      \
    int firstActive = __builtin_spirv_OpenCL_ctz_i32(activeChannels);                                                         \
    return SPIRV_BUILTIN(GroupBroadcast, _i32_##TYPE_ABBR##_i32, )(Execution, Value, firstActive);                            \
}

#define DEFN_NON_UNIFORM_BROADCAST(TYPE, TYPE_ABBR)             \
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
        uint sgsize = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupSize, , )();
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
                result = __builtin_spirv_OpenCL_popcount_i32(consideredBits);
                break;
        }
    }
    return result;
}

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBallotFindLSB, _i32_v4i32, )(int Execution, uint4 Value)
{
    if (Execution == Subgroup)
    {
        return __builtin_spirv_OpenCL_ctz_i32(Value.x);
    }
    return 0;
}

uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformBallotFindMSB, _i32_v4i32, )(int Execution, uint4 Value)
{
    if (Execution == Subgroup)
    {
        uint sgsize = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupSize, , )();
        uint consideredBits = Value.x << (32 - sgsize);
        return (sgsize - 1) - __builtin_spirv_OpenCL_clz_i32(consideredBits);
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

#define DEFN_WORK_GROUP_REDUCE(func, type_abbr, type, op)                                                  \
type __builtin_IB_WorkGroupReduce_##func##_##type_abbr(type X)                                             \
{                                                                                                          \
    type sg_x = SPIRV_BUILTIN(Group##func, _i32_i32_##type_abbr, )(Subgroup, GroupOperationReduce, X);     \
    GET_MEMPOOL_PTR(scratch, type, true, 0)                                                                \
    uint sg_id = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupId, , )();                                             \
    uint num_sg = SPIRV_BUILTIN_NO_OP(BuiltInNumSubgroups, , )();                                          \
    uint sg_lid = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();                             \
    uint sg_size = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupSize, , )();                                         \
                                                                                                           \
    if (sg_lid == sg_size - 1) {                                                                           \
        scratch[sg_id] = sg_x;                                                                             \
    }                                                                                                      \
    SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Workgroup, 0, AcquireRelease | WorkgroupMemory);         \
                                                                                                           \
    type sg_aggregate = scratch[0];                                                                        \
    for (int s = 1; s < num_sg; ++s) {                                                                     \
        sg_aggregate = op(sg_aggregate, scratch[s]);                                                       \
    }                                                                                                      \
                                                                                                           \
    SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Workgroup, 0, AcquireRelease | WorkgroupMemory);         \
    return sg_aggregate;                                                                                   \
}


#define DEFN_WORK_GROUP_SCAN_INCL(func, type_abbr, type, op)                                                    \
type __builtin_IB_WorkGroupScanInclusive_##func##_##type_abbr(type X)                                           \
{                                                                                                               \
    type sg_x = SPIRV_BUILTIN(Group##func, _i32_i32_##type_abbr, )(Subgroup, GroupOperationInclusiveScan, X);   \
                                                                                                                \
    GET_MEMPOOL_PTR(scratch, type, true, 0)                                                                     \
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
    SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Workgroup, 0, AcquireRelease | WorkgroupMemory);              \
    return result;                                                                                              \
}


#define DEFN_WORK_GROUP_SCAN_EXCL(func, type_abbr, type, op, identity)                                          \
type __builtin_IB_WorkGroupScanExclusive_##func##_##type_abbr(type X)                                           \
{                                                                                                               \
    type carry = SPIRV_BUILTIN(Group##func, _i32_i32_##type_abbr, )(Subgroup, GroupOperationInclusiveScan, X);  \
                                                                                                                \
    GET_MEMPOOL_PTR(scratch, type, true, 0)                                                                     \
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
    SPIRV_BUILTIN(ControlBarrier, _i32_i32_i32, )(Workgroup, 0, AcquireRelease | WorkgroupMemory);              \
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
        uint mask = 1 << ( ((8 * sizeof(uint)) - __builtin_spirv_OpenCL_clz_i32(sgsize - 1)) - 1 );     \
        while( mask > 0 )                                                                               \
        {                                                                                               \
            uint c = sglid ^ mask;                                                                      \
            type other = ( c < sgsize ) ?                                                               \
                            intel_sub_group_shuffle( X, c ):                                            \
                            identity;                                                                   \
            X = op( other, X );                                                                         \
            mask >>= 1;                                                                                 \
        }                                                                                               \
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
DEFN_WORK_GROUP_REDUCE(func, type_abbr, type, op)                                                 \
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
        if (sizeof(X) < 8 || __UseNative64Bit##type_gen##SubgroupBuiltin)                         \
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
DEFN_UNIFORM_GROUP_FUNC(FMin, half,   Float, f16, __builtin_spirv_OpenCL_fmin_f16_f16, INFINITY)
DEFN_UNIFORM_GROUP_FUNC(FMin, float,  Float, f32, __builtin_spirv_OpenCL_fmin_f32_f32, INFINITY)
#if defined(cl_khr_fp64)
DEFN_UNIFORM_GROUP_FUNC(FMin, double, Float, f64, __builtin_spirv_OpenCL_fmin_f64_f64, INFINITY)
#endif
DEFN_UNIFORM_GROUP_FUNC(SMin, char,   Int, i8,  __builtin_spirv_OpenCL_s_min_i8_i8,   CHAR_MAX)
DEFN_UNIFORM_GROUP_FUNC(SMin, short,  Int, i16, __builtin_spirv_OpenCL_s_min_i16_i16, SHRT_MAX)
DEFN_UNIFORM_GROUP_FUNC(SMin, int,    Int, i32, __builtin_spirv_OpenCL_s_min_i32_i32, INT_MAX)
DEFN_UNIFORM_GROUP_FUNC(SMin, long,   Int, i64, __builtin_spirv_OpenCL_s_min_i64_i64, LONG_MAX)
DEFN_UNIFORM_GROUP_FUNC_UNSIGNED(UMin, char,  Int,  i8, __builtin_spirv_OpenCL_u_min_i8_i8,   UCHAR_MAX)
DEFN_UNIFORM_GROUP_FUNC_UNSIGNED(UMin, short, Int, i16, __builtin_spirv_OpenCL_u_min_i16_i16, USHRT_MAX)
DEFN_UNIFORM_GROUP_FUNC_UNSIGNED(UMin, int,   Int, i32, __builtin_spirv_OpenCL_u_min_i32_i32, UINT_MAX)
DEFN_UNIFORM_GROUP_FUNC_UNSIGNED(UMin, long,  Int, i64, __builtin_spirv_OpenCL_u_min_i64_i64, ULONG_MAX)
// ---- Max ----
DEFN_UNIFORM_GROUP_FUNC(FMax, half,   Float, f16, __builtin_spirv_OpenCL_fmax_f16_f16, -INFINITY)
DEFN_UNIFORM_GROUP_FUNC(FMax, float,  Float, f32, __builtin_spirv_OpenCL_fmax_f32_f32, -INFINITY)
#if defined(cl_khr_fp64)
DEFN_UNIFORM_GROUP_FUNC(FMax, double, Float, f64, __builtin_spirv_OpenCL_fmax_f64_f64, -INFINITY)
#endif
DEFN_UNIFORM_GROUP_FUNC(SMax, char,  Int, i8,  __builtin_spirv_OpenCL_s_max_i8_i8,   CHAR_MIN)
DEFN_UNIFORM_GROUP_FUNC(SMax, short, Int, i16, __builtin_spirv_OpenCL_s_max_i16_i16, SHRT_MIN)
DEFN_UNIFORM_GROUP_FUNC(SMax, int,   Int, i32, __builtin_spirv_OpenCL_s_max_i32_i32, INT_MIN)
DEFN_UNIFORM_GROUP_FUNC(SMax, long,  Int, i64, __builtin_spirv_OpenCL_s_max_i64_i64, LONG_MIN)
DEFN_UNIFORM_GROUP_FUNC_UNSIGNED(UMax, char,  Int, i8,  __builtin_spirv_OpenCL_u_max_i8_i8,   0)
DEFN_UNIFORM_GROUP_FUNC_UNSIGNED(UMax, short, Int, i16, __builtin_spirv_OpenCL_u_max_i16_i16, 0)
DEFN_UNIFORM_GROUP_FUNC_UNSIGNED(UMax, int,   Int, i32, __builtin_spirv_OpenCL_u_max_i32_i32, 0)
DEFN_UNIFORM_GROUP_FUNC_UNSIGNED(UMax, long,  Int, i64, __builtin_spirv_OpenCL_u_max_i64_i64, 0)

#if defined(cl_khr_subgroup_non_uniform_arithmetic) || defined(cl_khr_subgroup_clustered_reduce)
#define DEFN_SUB_GROUP_REDUCE_NON_UNIFORM(type, type_abbr, op, identity, X, signed_cast)                 \
{                                                                                                        \
    uint activeChannels = __builtin_IB_WaveBallot(true);                                                 \
    uint firstActive = __builtin_spirv_OpenCL_ctz_i32(activeChannels);                                   \
                                                                                                         \
    type result = identity;                                                                              \
    while (activeChannels)                                                                               \
    {                                                                                                    \
        uint activeId = __builtin_spirv_OpenCL_ctz_i32(activeChannels);                                  \
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
    uint activeId = __builtin_spirv_OpenCL_ctz_i32(activeChannels);                                 \
    activeChannels ^= 1 << activeId;                                                                \
    while (activeChannels)                                                                          \
    {                                                                                               \
        type value = intel_sub_group_shuffle(X, activeId);                                          \
        activeId = __builtin_spirv_OpenCL_ctz_i32(activeChannels);                                  \
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
        uint activeId = __builtin_spirv_OpenCL_ctz_i32(activeChannels);                              \
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

#define DEFN_SUB_GROUP_CLUSTERED_REDUCE(type, type_abbr, op, identity, X, ClusterSize, signed_cast)         \
{                                                                                                           \
    uint clusterIndex = 0;                                                                                  \
    uint activeChannels = __builtin_IB_WaveBallot(true);                                                    \
    uint numActive = __builtin_spirv_OpenCL_popcount_i32(activeChannels);                                   \
    uint numClusters = numActive / ClusterSize;                                                             \
                                                                                                            \
    for (uint clusterIndex = 0; clusterIndex < numClusters; clusterIndex++)                                 \
    {                                                                                                       \
        uint Counter = ClusterSize;                                                                         \
        uint Ballot = activeChannels;                                                                       \
        uint clusterBallot = 0;                                                                             \
        while (Counter--)                                                                                   \
        {                                                                                                   \
            uint trailingOne = 1 << __builtin_spirv_OpenCL_ctz_i32(Ballot);                                 \
            clusterBallot |= trailingOne;                                                                   \
            Ballot ^= trailingOne;                                                                          \
        }                                                                                                   \
        uint active = SPIRV_BUILTIN(GroupNonUniformInverseBallot, _i32_v4i32, )(Subgroup, clusterBallot);   \
        if (active)                                                                                         \
        {                                                                                                   \
            DEFN_SUB_GROUP_REDUCE_NON_UNIFORM(type, type_abbr, op, identity, X, signed_cast)                \
        }                                                                                                   \
        activeChannels ^= clusterBallot;                                                                    \
    }                                                                                                       \
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
        if (sizeof(X) < 8 || __UseNative64Bit##type_gen##SubgroupBuiltin)                                                            \
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
DEFN_NON_UNIFORM_GROUP_FUNC(SMin, char,   Int,   i8,  __builtin_spirv_OpenCL_s_min_i8_i8,   CHAR_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC(SMin, short,  Int,   i16, __builtin_spirv_OpenCL_s_min_i16_i16, SHRT_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC(SMin, int,    Int,   i32, __builtin_spirv_OpenCL_s_min_i32_i32, INT_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC(SMin, long,   Int,   i64, __builtin_spirv_OpenCL_s_min_i64_i64, LONG_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC(FMin, float,  Float, f32, __builtin_spirv_OpenCL_fmin_f32_f32,  INFINITY)
DEFN_NON_UNIFORM_GROUP_FUNC_UNSIGNED(UMin, char,  Int, i8,  __builtin_spirv_OpenCL_u_min_i8_i8,   UCHAR_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC_UNSIGNED(UMin, short, Int, i16, __builtin_spirv_OpenCL_u_min_i16_i16, USHRT_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC_UNSIGNED(UMin, int,   Int, i32, __builtin_spirv_OpenCL_u_min_i32_i32, UINT_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC_UNSIGNED(UMin, long,  Int, i64, __builtin_spirv_OpenCL_u_min_i64_i64, ULONG_MAX)
#if defined(cl_khr_fp64)
DEFN_NON_UNIFORM_GROUP_FUNC(FMin, double, Float, f64, __builtin_spirv_OpenCL_fmin_f64_f64,  INFINITY)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_GROUP_FUNC(FMin, half,   Float, f16, __builtin_spirv_OpenCL_fmin_f16_f16,  INFINITY)
#endif // defined(cl_khr_fp16)

// OpGroupNonUniformSMax, OpGroupNonUniformUMax, OpGroupNonUniformFMax
DEFN_NON_UNIFORM_GROUP_FUNC(SMax, char,   Int,   i8,  __builtin_spirv_OpenCL_s_max_i8_i8,   CHAR_MIN)
DEFN_NON_UNIFORM_GROUP_FUNC(SMax, short,  Int,   i16, __builtin_spirv_OpenCL_s_max_i16_i16, SHRT_MIN)
DEFN_NON_UNIFORM_GROUP_FUNC(SMax, int,    Int,   i32, __builtin_spirv_OpenCL_s_max_i32_i32, INT_MIN)
DEFN_NON_UNIFORM_GROUP_FUNC(SMax, long,   Int,   i64, __builtin_spirv_OpenCL_s_max_i64_i64, LONG_MIN)
DEFN_NON_UNIFORM_GROUP_FUNC(FMax, float,  Float, f32, __builtin_spirv_OpenCL_fmax_f32_f32, -INFINITY)
DEFN_NON_UNIFORM_GROUP_FUNC_UNSIGNED(UMax, char,  Int, i8,  __builtin_spirv_OpenCL_u_max_i8_i8,   0)
DEFN_NON_UNIFORM_GROUP_FUNC_UNSIGNED(UMax, short, Int, i16, __builtin_spirv_OpenCL_u_max_i16_i16, 0)
DEFN_NON_UNIFORM_GROUP_FUNC_UNSIGNED(UMax, int,   Int, i32, __builtin_spirv_OpenCL_u_max_i32_i32, 0)
DEFN_NON_UNIFORM_GROUP_FUNC_UNSIGNED(UMax, long,  Int, i64, __builtin_spirv_OpenCL_u_max_i64_i64, 0)
#if defined(cl_khr_fp64)
DEFN_NON_UNIFORM_GROUP_FUNC(FMax, double, Float, f64, __builtin_spirv_OpenCL_fmax_f64_f64, -INFINITY)
#endif // defined(cl_khr_fp64)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_GROUP_FUNC(FMax, half,   Float, f16, __builtin_spirv_OpenCL_fmax_f16_f16, -INFINITY)
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
        return __builtin_IB_simd_shuffle_h(x, c);
    }
    return 0;
}
#endif // defined(cl_khr_fp16)

#define DEFN_SUB_GROUP_SHUFFLE_XOR(TYPE, TYPE_ABBR)                                                             \
TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffleXor, _i32_##TYPE_ABBR##_i32, )(int Execution, TYPE x, uint c)  \
{                                                                                                               \
    c = get_sub_group_local_id() ^ c;                                                                           \
    return SPIRV_BUILTIN(GroupNonUniformShuffle, _i32_##TYPE_ABBR##_i32, )(Execution, x, c);                    \
}

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
        return __builtin_IB_simd_shuffle_down_uc(x, 0, c);
    }
    return 0;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffleDown, _i32_i16_i32, )(int Execution, short x, uint c)
{
    if (Execution == Subgroup)
    {
        return __builtin_IB_simd_shuffle_down_us(x, 0, c);
    }
    return 0;
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffleDown, _i32_i32_i32, )(int Execution, int x, uint c)
{
    if (Execution == Subgroup)
    {
        return __builtin_IB_simd_shuffle_down(x, 0, c);
    }
    return 0;
}

long SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffleDown, _i32_i64_i32, )(int Execution, long x, uint c)
{
    if (Execution == Subgroup)
    {
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
        return as_float(__builtin_IB_simd_shuffle_down(as_uint(x), 0, c));
    }
    return 0;
}

#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffleDown, _i32_f64_i32, )(int Execution, double x, uint c)
{
    if (Execution == Subgroup)
    {
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
        return as_half(__builtin_IB_simd_shuffle_down_us(as_ushort(x), 0, c));
    }
    return 0;
}
#endif // defined(cl_khr_fp16)

// Shuffle up functions
#define DEFN_NON_UNIFORM_SHUFFLE_UP(TYPE, TYPE_ABBR)                                                                        \
TYPE SPIRV_OVERLOADABLE SPIRV_BUILTIN(GroupNonUniformShuffleUp, _i32_##TYPE_ABBR##_i32, )(int Execution, TYPE x, uint c)    \
{                                                                                                                           \
    if (Execution == Subgroup)                                                                                              \
    {                                                                                                                       \
        return intel_sub_group_shuffle_up((TYPE) 0, x, c);                                                                  \
    }                                                                                                                       \
    return 0;                                                                                                               \
}

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
