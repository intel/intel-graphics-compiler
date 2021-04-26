/*========================== begin_copyright_notice ============================

Copyright (c) 2016-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

extern __constant int __UseNative64BitSubgroupBuiltin;

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

int OVERLOADABLE intel_sub_group_shuffle_up( int identity, int X, uint c );
long OVERLOADABLE intel_sub_group_shuffle_up( long identity, long X, uint c );
uint OVERLOADABLE intel_sub_group_shuffle_up( uint identity, uint X, uint c );
ulong OVERLOADABLE intel_sub_group_shuffle_up( ulong identity, ulong X, uint c );
half OVERLOADABLE intel_sub_group_shuffle_up( half identity, half X, uint c );
float OVERLOADABLE intel_sub_group_shuffle_up( float identity, float X, uint c );


// L2G

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1i8_p3i8_i64_i64_i64(uint Execution, global uchar *Destination, const local uchar *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1i8_p3i8_i32_i32_i64(uint Execution, global uchar *Destination, const local uchar *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1i16_p3i16_i64_i64_i64(uint Execution, global ushort *Destination, const local ushort *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1i16_p3i16_i32_i32_i64(uint Execution, global ushort *Destination, const local ushort *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1i32_p3i32_i64_i64_i64(uint Execution, global uint *Destination, const local uint *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1i32_p3i32_i32_i32_i64(uint Execution, global uint *Destination, const local uint *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1i64_p3i64_i64_i64_i64(uint Execution, global ulong *Destination, const local ulong *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1i64_p3i64_i32_i32_i64(uint Execution, global ulong *Destination, const local ulong *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1f16_p3f16_i64_i64_i64(uint Execution, global half *Destination, const local half *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1f16_p3f16_i32_i32_i64(uint Execution, global half *Destination, const local half *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1f32_p3f32_i64_i64_i64(uint Execution, global float *Destination, const local float *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1f32_p3f32_i32_i32_i64(uint Execution, global float *Destination, const local float *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2i8_p3v2i8_i64_i64_i64(uint Execution, global uchar2 *Destination, const local uchar2 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2i8_p3v2i8_i32_i32_i64(uint Execution, global uchar2 *Destination, const local uchar2 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3i8_p3v3i8_i64_i64_i64(uint Execution, global uchar3 *Destination, const local uchar3 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3i8_p3v3i8_i32_i32_i64(uint Execution, global uchar3 *Destination, const local uchar3 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4i8_p3v4i8_i64_i64_i64(uint Execution, global uchar4 *Destination, const local uchar4 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4i8_p3v4i8_i32_i32_i64(uint Execution, global uchar4 *Destination, const local uchar4 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8i8_p3v8i8_i64_i64_i64(uint Execution, global uchar8 *Destination, const local uchar8 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8i8_p3v8i8_i32_i32_i64(uint Execution, global uchar8 *Destination, const local uchar8 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16i8_p3v16i8_i64_i64_i64(uint Execution, global uchar16 *Destination, const local uchar16 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16i8_p3v16i8_i32_i32_i64(uint Execution, global uchar16 *Destination, const local uchar16 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2i16_p3v2i16_i64_i64_i64(uint Execution, global ushort2 *Destination, const local ushort2 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2i16_p3v2i16_i32_i32_i64(uint Execution, global ushort2 *Destination, const local ushort2 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3i16_p3v3i16_i64_i64_i64(uint Execution, global ushort3 *Destination, const local ushort3 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3i16_p3v3i16_i32_i32_i64(uint Execution, global ushort3 *Destination, const local ushort3 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4i16_p3v4i16_i64_i64_i64(uint Execution, global ushort4 *Destination, const local ushort4 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4i16_p3v4i16_i32_i32_i64(uint Execution, global ushort4 *Destination, const local ushort4 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8i16_p3v8i16_i64_i64_i64(uint Execution, global ushort8 *Destination, const local ushort8 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8i16_p3v8i16_i32_i32_i64(uint Execution, global ushort8 *Destination, const local ushort8 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16i16_p3v16i16_i64_i64_i64(uint Execution, global ushort16 *Destination, const local ushort16 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16i16_p3v16i16_i32_i32_i64(uint Execution, global ushort16 *Destination, const local ushort16 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2i32_p3v2i32_i64_i64_i64(uint Execution, global uint2 *Destination, const local uint2 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2i32_p3v2i32_i32_i32_i64(uint Execution, global uint2 *Destination, const local uint2 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3i32_p3v3i32_i64_i64_i64(uint Execution, global uint3 *Destination, const local uint3 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3i32_p3v3i32_i32_i32_i64(uint Execution, global uint3 *Destination, const local uint3 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4i32_p3v4i32_i64_i64_i64(uint Execution, global uint4 *Destination, const local uint4 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4i32_p3v4i32_i32_i32_i64(uint Execution, global uint4 *Destination, const local uint4 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8i32_p3v8i32_i64_i64_i64(uint Execution, global uint8 *Destination, const local uint8 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8i32_p3v8i32_i32_i32_i64(uint Execution, global uint8 *Destination, const local uint8 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16i32_p3v16i32_i64_i64_i64(uint Execution, global uint16 *Destination, const local uint16 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16i32_p3v16i32_i32_i32_i64(uint Execution, global uint16 *Destination, const local uint16 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2i64_p3v2i64_i64_i64_i64(uint Execution, global ulong2 *Destination, const local ulong2 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2i64_p3v2i64_i32_i32_i64(uint Execution, global ulong2 *Destination, const local ulong2 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3i64_p3v3i64_i64_i64_i64(uint Execution, global ulong3 *Destination, const local ulong3 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3i64_p3v3i64_i32_i32_i64(uint Execution, global ulong3 *Destination, const local ulong3 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4i64_p3v4i64_i64_i64_i64(uint Execution, global ulong4 *Destination, const local ulong4 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4i64_p3v4i64_i32_i32_i64(uint Execution, global ulong4 *Destination, const local ulong4 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8i64_p3v8i64_i64_i64_i64(uint Execution, global ulong8 *Destination, const local ulong8 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8i64_p3v8i64_i32_i32_i64(uint Execution, global ulong8 *Destination, const local ulong8 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16i64_p3v16i64_i64_i64_i64(uint Execution, global ulong16 *Destination, const local ulong16 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16i64_p3v16i64_i32_i32_i64(uint Execution, global ulong16 *Destination, const local ulong16 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2f16_p3v2f16_i64_i64_i64(uint Execution, global half2 *Destination, const local half2 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2f16_p3v2f16_i32_i32_i64(uint Execution, global half2 *Destination, const local half2 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3f16_p3v3f16_i64_i64_i64(uint Execution, global half3 *Destination, const local half3 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3f16_p3v3f16_i32_i32_i64(uint Execution, global half3 *Destination, const local half3 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4f16_p3v4f16_i64_i64_i64(uint Execution, global half4 *Destination, const local half4 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4f16_p3v4f16_i32_i32_i64(uint Execution, global half4 *Destination, const local half4 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8f16_p3v8f16_i64_i64_i64(uint Execution, global half8 *Destination, const local half8 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8f16_p3v8f16_i32_i32_i64(uint Execution, global half8 *Destination, const local half8 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16f16_p3v16f16_i64_i64_i64(uint Execution, global half16 *Destination, const local half16 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16f16_p3v16f16_i32_i32_i64(uint Execution, global half16 *Destination, const local half16 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2f32_p3v2f32_i64_i64_i64(uint Execution, global float2 *Destination, const local float2 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2f32_p3v2f32_i32_i32_i64(uint Execution, global float2 *Destination, const local float2 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3f32_p3v3f32_i64_i64_i64(uint Execution, global float3 *Destination, const local float3 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3f32_p3v3f32_i32_i32_i64(uint Execution, global float3 *Destination, const local float3 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4f32_p3v4f32_i64_i64_i64(uint Execution, global float4 *Destination, const local float4 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4f32_p3v4f32_i32_i32_i64(uint Execution, global float4 *Destination, const local float4 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8f32_p3v8f32_i64_i64_i64(uint Execution, global float8 *Destination, const local float8 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8f32_p3v8f32_i32_i32_i64(uint Execution, global float8 *Destination, const local float8 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16f32_p3v16f32_i64_i64_i64(uint Execution, global float16 *Destination, const local float16 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16f32_p3v16f32_i32_i32_i64(uint Execution, global float16 *Destination, const local float16 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}


// G2L

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3i8_p1i8_i64_i64_i64(uint Execution, local uchar *Destination, const global uchar *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3i8_p1i8_i32_i32_i64(uint Execution, local uchar *Destination, const global uchar *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3i16_p1i16_i64_i64_i64(uint Execution, local ushort *Destination, const global ushort *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3i16_p1i16_i32_i32_i64(uint Execution, local ushort *Destination, const global ushort *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3i32_p1i32_i64_i64_i64(uint Execution, local uint *Destination, const global uint *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3i32_p1i32_i32_i32_i64(uint Execution, local uint *Destination, const global uint *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3i64_p1i64_i64_i64_i64(uint Execution, local ulong *Destination, const global ulong *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3i64_p1i64_i32_i32_i64(uint Execution, local ulong *Destination, const global ulong *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3f16_p1f16_i64_i64_i64(uint Execution, local half *Destination, const global half *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3f16_p1f16_i32_i32_i64(uint Execution, local half *Destination, const global half *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3f32_p1f32_i64_i64_i64(uint Execution, local float *Destination, const global float *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3f32_p1f32_i32_i32_i64(uint Execution, local float *Destination, const global float *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2i8_p1v2i8_i64_i64_i64(uint Execution, local uchar2 *Destination, const global uchar2 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2i8_p1v2i8_i32_i32_i64(uint Execution, local uchar2 *Destination, const global uchar2 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3i8_p1v3i8_i64_i64_i64(uint Execution, local uchar3 *Destination, const global uchar3 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3i8_p1v3i8_i32_i32_i64(uint Execution, local uchar3 *Destination, const global uchar3 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4i8_p1v4i8_i64_i64_i64(uint Execution, local uchar4 *Destination, const global uchar4 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4i8_p1v4i8_i32_i32_i64(uint Execution, local uchar4 *Destination, const global uchar4 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8i8_p1v8i8_i64_i64_i64(uint Execution, local uchar8 *Destination, const global uchar8 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8i8_p1v8i8_i32_i32_i64(uint Execution, local uchar8 *Destination, const global uchar8 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16i8_p1v16i8_i64_i64_i64(uint Execution, local uchar16 *Destination, const global uchar16 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16i8_p1v16i8_i32_i32_i64(uint Execution, local uchar16 *Destination, const global uchar16 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2i16_p1v2i16_i64_i64_i64(uint Execution, local ushort2 *Destination, const global ushort2 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2i16_p1v2i16_i32_i32_i64(uint Execution, local ushort2 *Destination, const global ushort2 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3i16_p1v3i16_i64_i64_i64(uint Execution, local ushort3 *Destination, const global ushort3 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3i16_p1v3i16_i32_i32_i64(uint Execution, local ushort3 *Destination, const global ushort3 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4i16_p1v4i16_i64_i64_i64(uint Execution, local ushort4 *Destination, const global ushort4 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4i16_p1v4i16_i32_i32_i64(uint Execution, local ushort4 *Destination, const global ushort4 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8i16_p1v8i16_i64_i64_i64(uint Execution, local ushort8 *Destination, const global ushort8 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8i16_p1v8i16_i32_i32_i64(uint Execution, local ushort8 *Destination, const global ushort8 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16i16_p1v16i16_i64_i64_i64(uint Execution, local ushort16 *Destination, const global ushort16 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16i16_p1v16i16_i32_i32_i64(uint Execution, local ushort16 *Destination, const global ushort16 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2i32_p1v2i32_i64_i64_i64(uint Execution, local uint2 *Destination, const global uint2 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2i32_p1v2i32_i32_i32_i64(uint Execution, local uint2 *Destination, const global uint2 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3i32_p1v3i32_i64_i64_i64(uint Execution, local uint3 *Destination, const global uint3 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3i32_p1v3i32_i32_i32_i64(uint Execution, local uint3 *Destination, const global uint3 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4i32_p1v4i32_i64_i64_i64(uint Execution, local uint4 *Destination, const global uint4 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4i32_p1v4i32_i32_i32_i64(uint Execution, local uint4 *Destination, const global uint4 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8i32_p1v8i32_i64_i64_i64(uint Execution, local uint8 *Destination, const global uint8 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8i32_p1v8i32_i32_i32_i64(uint Execution, local uint8 *Destination, const global uint8 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16i32_p1v16i32_i64_i64_i64(uint Execution, local uint16 *Destination, const global uint16 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16i32_p1v16i32_i32_i32_i64(uint Execution, local uint16 *Destination, const global uint16 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2i64_p1v2i64_i64_i64_i64(uint Execution, local ulong2 *Destination, const global ulong2 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2i64_p1v2i64_i32_i32_i64(uint Execution, local ulong2 *Destination, const global ulong2 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3i64_p1v3i64_i64_i64_i64(uint Execution, local ulong3 *Destination, const global ulong3 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3i64_p1v3i64_i32_i32_i64(uint Execution, local ulong3 *Destination, const global ulong3 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4i64_p1v4i64_i64_i64_i64(uint Execution, local ulong4 *Destination, const global ulong4 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4i64_p1v4i64_i32_i32_i64(uint Execution, local ulong4 *Destination, const global ulong4 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8i64_p1v8i64_i64_i64_i64(uint Execution, local ulong8 *Destination, const global ulong8 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8i64_p1v8i64_i32_i32_i64(uint Execution, local ulong8 *Destination, const global ulong8 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16i64_p1v16i64_i64_i64_i64(uint Execution, local ulong16 *Destination, const global ulong16 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16i64_p1v16i64_i32_i32_i64(uint Execution, local ulong16 *Destination, const global ulong16 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2f16_p1v2f16_i64_i64_i64(uint Execution, local half2 *Destination, const global half2 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2f16_p1v2f16_i32_i32_i64(uint Execution, local half2 *Destination, const global half2 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3f16_p1v3f16_i64_i64_i64(uint Execution, local half3 *Destination, const global half3 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3f16_p1v3f16_i32_i32_i64(uint Execution, local half3 *Destination, const global half3 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4f16_p1v4f16_i64_i64_i64(uint Execution, local half4 *Destination, const global half4 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4f16_p1v4f16_i32_i32_i64(uint Execution, local half4 *Destination, const global half4 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8f16_p1v8f16_i64_i64_i64(uint Execution, local half8 *Destination, const global half8 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8f16_p1v8f16_i32_i32_i64(uint Execution, local half8 *Destination, const global half8 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16f16_p1v16f16_i64_i64_i64(uint Execution, local half16 *Destination, const global half16 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16f16_p1v16f16_i32_i32_i64(uint Execution, local half16 *Destination, const global half16 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2f32_p1v2f32_i64_i64_i64(uint Execution, local float2 *Destination, const global float2 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2f32_p1v2f32_i32_i32_i64(uint Execution, local float2 *Destination, const global float2 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3f32_p1v3f32_i64_i64_i64(uint Execution, local float3 *Destination, const global float3 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3f32_p1v3f32_i32_i32_i64(uint Execution, local float3 *Destination, const global float3 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4f32_p1v4f32_i64_i64_i64(uint Execution, local float4 *Destination, const global float4 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4f32_p1v4f32_i32_i32_i64(uint Execution, local float4 *Destination, const global float4 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8f32_p1v8f32_i64_i64_i64(uint Execution, local float8 *Destination, const global float8 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8f32_p1v8f32_i32_i32_i64(uint Execution, local float8 *Destination, const global float8 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16f32_p1v16f32_i64_i64_i64(uint Execution, local float16 *Destination, const global float16 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16f32_p1v16f32_i32_i32_i64(uint Execution, local float16 *Destination, const global float16 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

void __builtin_spirv_OpGroupWaitEvents_i32_i32_p0i64(uint Execution, uint NumEvents, private Event_t *EventsList)
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
void __builtin_spirv_OpGroupWaitEvents_i32_i32_p4i64(uint Execution, uint NumEvents, generic Event_t *EventsList)
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


bool __builtin_spirv_OpGroupAll_i32_i1(uint Execution, bool Predicate)
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
            value = __builtin_spirv_OpGroupIAdd_i32_i32_i32(Subgroup, GroupOperationReduce, as_uint(value) );
            value = ( value == 0 ) ? 1 : 0;
            return value;
    }
    else
    {
         return 0;
    }
}

bool __builtin_spirv_OpGroupAny_i32_i1(uint Execution, bool Predicate)
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
            value = __builtin_spirv_OpGroupIAdd_i32_i32_i32(Subgroup, GroupOperationReduce, as_uint(value) );
            return value;
    }
    else
    {
         return 0;
    }
}

#if defined(cl_khr_subgroup_non_uniform_vote)
bool __builtin_spirv_OpGroupNonUniformElect_i32(uint Execution)
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

bool __builtin_spirv_OpGroupNonUniformAll_i32_i1(uint Execution, bool Predicate)
{
    if(Execution == Subgroup)
        return __builtin_spirv_OpGroupAll_i32_i1(Execution, Predicate);
    else
        return false;
}

bool __builtin_spirv_OpGroupNonUniformAny_i32_i1(uint Execution, bool Predicate)
{
    if (Execution == Subgroup)
        return __builtin_spirv_OpGroupAny_i32_i1(Execution, Predicate);
    else
        return false;
}


DEFN_NON_UNIFORM_ALL_EQUAL(uchar,  i8)
DEFN_NON_UNIFORM_ALL_EQUAL(ushort, i16)
DEFN_NON_UNIFORM_ALL_EQUAL(uint,   i32)
DEFN_NON_UNIFORM_ALL_EQUAL(ulong,  i64)
DEFN_NON_UNIFORM_ALL_EQUAL(float,  f32)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_ALL_EQUAL(half,   f16)
#endif // defined(cl_khr_fp16)
#endif // defined(cl_khr_subgroup_non_uniform_vote)

//Broadcast Functions

// ***Note that technically the spec allows for 64 bit local Id's but currently our hardware cannot support
// this in which case we cast all ulong local Id's to uint's at this time.

bool __builtin_spirv_OpGroupBroadcast_i32_i1_v3i32(uint Execution, bool Value, uint3 LocalId)
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

bool __builtin_spirv_OpGroupBroadcast_i32_i1_v3i64(uint Execution, bool Value, ulong3 LocalId)
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

uchar __builtin_spirv_OpGroupBroadcast_i32_i8_v3i32(uint Execution, uchar Value, uint3 LocalId)
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

uchar __builtin_spirv_OpGroupBroadcast_i32_i8_v3i64(uint Execution, uchar Value, ulong3 LocalId)
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

ushort __builtin_spirv_OpGroupBroadcast_i32_i16_v3i32(uint Execution, ushort Value, uint3 LocalId)
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

ushort __builtin_spirv_OpGroupBroadcast_i32_i16_v3i64(uint Execution, ushort Value, ulong3 LocalId)
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

uint __builtin_spirv_OpGroupBroadcast_i32_i32_v3i32(uint Execution, uint Value, uint3 LocalId)
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

uint __builtin_spirv_OpGroupBroadcast_i32_i32_v3i64(uint Execution, uint Value, ulong3 LocalId)
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

ulong __builtin_spirv_OpGroupBroadcast_i32_i64_v3i32(uint Execution, ulong Value, uint3 LocalId)
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

ulong __builtin_spirv_OpGroupBroadcast_i32_i64_v3i64(uint Execution, ulong Value, ulong3 LocalId)
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

half __builtin_spirv_OpGroupBroadcast_i32_f16_v3i32(uint Execution, half Value, uint3 LocalId)
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

half __builtin_spirv_OpGroupBroadcast_i32_f16_v3i64(uint Execution, half Value, ulong3 LocalId)
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

float __builtin_spirv_OpGroupBroadcast_i32_f32_v3i32(uint Execution, float Value, uint3 LocalId)
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

float __builtin_spirv_OpGroupBroadcast_i32_f32_v3i64(uint Execution, float Value, ulong3 LocalId)
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

bool __builtin_spirv_OpGroupBroadcast_i32_i1_v2i32(uint Execution, bool Value, uint2 LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_i1_v3i32(Execution, Value, (uint3)(LocalId.s0, LocalId.s1, 0));
}

bool __builtin_spirv_OpGroupBroadcast_i32_i1_v2i64(uint Execution, bool Value, ulong2 LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_i1_v3i64(Execution, Value, (ulong3)(LocalId.s0, LocalId.s1, 0));
}

uchar __builtin_spirv_OpGroupBroadcast_i32_i8_v2i32(uint Execution, uchar Value, uint2 LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_i8_v3i32(Execution,Value,(uint3)(LocalId.s0,LocalId.s1,0));
}

uchar __builtin_spirv_OpGroupBroadcast_i32_i8_v2i64(uint Execution, uchar Value, ulong2 LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_i8_v3i64(Execution,Value,(ulong3)(LocalId.s0,LocalId.s1,0));
}

ushort __builtin_spirv_OpGroupBroadcast_i32_i16_v2i32(uint Execution, ushort Value, uint2 LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_i16_v3i32(Execution,Value,(uint3)(LocalId.s0,LocalId.s1,0));
}

ushort __builtin_spirv_OpGroupBroadcast_i32_i16_v2i64(uint Execution, ushort Value, ulong2 LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_i16_v3i64(Execution,Value,(ulong3)(LocalId.s0,LocalId.s1,0));
}

uint __builtin_spirv_OpGroupBroadcast_i32_i32_v2i32(uint Execution, uint Value, uint2 LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_i32_v3i32(Execution,Value,(uint3)(LocalId.s0,LocalId.s1,0));
}

uint __builtin_spirv_OpGroupBroadcast_i32_i32_v2i64(uint Execution, uint Value, ulong2 LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_i32_v3i64(Execution,Value,(ulong3)(LocalId.s0,LocalId.s1,0));
}

ulong __builtin_spirv_OpGroupBroadcast_i32_i64_v2i32(uint Execution, ulong Value, uint2 LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_i64_v3i32(Execution,Value,(uint3)(LocalId.s0,LocalId.s1,0));
}

ulong __builtin_spirv_OpGroupBroadcast_i32_i64_v2i64(uint Execution, ulong Value, ulong2 LocalId)
{
   return __builtin_spirv_OpGroupBroadcast_i32_i64_v3i64(Execution,Value,(ulong3)(LocalId.s0,LocalId.s1,0));
}

half __builtin_spirv_OpGroupBroadcast_i32_f16_v2i32(uint Execution, half Value, uint2 LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_f16_v3i32(Execution,Value,(uint3)(LocalId.s0,LocalId.s1,0));
}

half __builtin_spirv_OpGroupBroadcast_i32_f16_v2i64(uint Execution, half Value, ulong2 LocalId)
{
   return __builtin_spirv_OpGroupBroadcast_i32_f16_v3i64(Execution,Value,(ulong3)(LocalId.s0,LocalId.s1,0));
}

float __builtin_spirv_OpGroupBroadcast_i32_f32_v2i32(uint Execution, float Value, uint2 LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_f32_v3i32(Execution,Value,(uint3)(LocalId.s0,LocalId.s1,0));
}

float __builtin_spirv_OpGroupBroadcast_i32_f32_v2i64(uint Execution, float Value, ulong2 LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_f32_v3i64(Execution,Value,(ulong3)(LocalId.s0,LocalId.s1,0));
}

bool __builtin_spirv_OpGroupBroadcast_i32_i1_i32(uint Execution, bool Value, uint LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_i1_v3i32(Execution, Value, (uint3)(LocalId, 0, 0));
}

bool __builtin_spirv_OpGroupBroadcast_i32_i1_i64(uint Execution, bool Value, ulong LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_i1_v3i64(Execution, Value, (ulong3)(LocalId, 0, 0));
}

uchar __builtin_spirv_OpGroupBroadcast_i32_i8_i32(uint Execution, uchar Value, uint LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_i8_v3i32(Execution,Value,(uint3)(LocalId,0,0));
}

uchar __builtin_spirv_OpGroupBroadcast_i32_i8_i64(uint Execution, uchar Value, ulong LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_i8_v3i64(Execution,Value,(ulong3)(LocalId,0,0));
}

ushort __builtin_spirv_OpGroupBroadcast_i32_i16_i32(uint Execution, ushort Value, uint LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_i16_v3i32(Execution,Value,(uint3)(LocalId,0,0));
}

ushort __builtin_spirv_OpGroupBroadcast_i32_i16_i64(uint Execution, ushort Value, ulong LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_i16_v3i64(Execution,Value,(ulong3)(LocalId,0,0));
}

uint __builtin_spirv_OpGroupBroadcast_i32_i32_i32(uint Execution, uint Value, uint LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_i32_v3i32(Execution,Value,(uint3)(LocalId,0,0));
}

uint __builtin_spirv_OpGroupBroadcast_i32_i32_i64(uint Execution, uint Value, ulong LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_i32_v3i64(Execution,Value,(ulong3)(LocalId,0,0));
}

ulong __builtin_spirv_OpGroupBroadcast_i32_i64_i32(uint Execution, ulong Value, uint LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_i64_v3i32(Execution,Value,(uint3)(LocalId,0,0));
}

ulong __builtin_spirv_OpGroupBroadcast_i32_i64_i64(uint Execution, ulong Value, ulong LocalId)
{
   return __builtin_spirv_OpGroupBroadcast_i32_i64_v3i64(Execution,Value,(ulong3)(LocalId,0,0));
}

half __builtin_spirv_OpGroupBroadcast_i32_f16_i32(uint Execution, half Value, uint LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_f16_v3i32(Execution,Value,(uint3)(LocalId,0,0));
}

half __builtin_spirv_OpGroupBroadcast_i32_f16_i64(uint Execution, half Value, ulong LocalId)
{
   return __builtin_spirv_OpGroupBroadcast_i32_f16_v3i64(Execution,Value,(ulong3)(LocalId,0,0));
}

float __builtin_spirv_OpGroupBroadcast_i32_f32_i32(uint Execution, float Value, uint LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_f32_v3i32(Execution,Value,(uint3)(LocalId,0,0));
}

float __builtin_spirv_OpGroupBroadcast_i32_f32_i64(uint Execution, float Value, ulong LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_f32_v3i64(Execution,Value,(ulong3)(LocalId,0,0));
}

DEFN_SUB_GROUP_BROADCAST_VEC(uchar, i8)
DEFN_SUB_GROUP_BROADCAST_VEC(ushort, i16)
DEFN_SUB_GROUP_BROADCAST_VEC(uint, i32)
DEFN_SUB_GROUP_BROADCAST_VEC(ulong, i64)
DEFN_SUB_GROUP_BROADCAST_VEC(float, f32)
#if defined(cl_khr_fp16)
DEFN_SUB_GROUP_BROADCAST_VEC(half, f16)
#endif // defined(cl_khr_fp16)

// Ballot Functions

uint intel_sub_group_ballot(bool p)
{
    return __builtin_IB_WaveBallot(p);
}

uint4 __builtin_spirv_BuiltInSubgroupEqMaskKHR()
{
    uint id = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();
    uint4 v = 0;

    v.x = 1 << id;

    return v;
}

uint4 __builtin_spirv_BuiltInSubgroupGeMaskKHR()
{
    uint id = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();
    uint4 v = 0;

    v.x = as_uint(as_int(1 << 31) >> (31 - id));

    return v;
}

uint4 __builtin_spirv_BuiltInSubgroupLeMaskKHR()
{
    uint id = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupLocalInvocationId, , )();
    uint4 v = 0;

    uint bitIdx = 1 << id;

    v.x = (bitIdx - 1) | bitIdx;

    return v;
}

uint4 __builtin_spirv_BuiltInSubgroupGtMaskKHR()
{
    uint4 v = 0;

    v.x = ~__builtin_spirv_BuiltInSubgroupLeMaskKHR().x;

    return v;
}

uint4 __builtin_spirv_BuiltInSubgroupLtMaskKHR()
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
    uint3 id = (uint3)(firstActive, 0, 0);
    return __builtin_spirv_OpGroupBroadcast_i32_i32_v3i32(Subgroup, Value, id);
}

float __builtin_spirv_OpSubgroupFirstInvocationKHR_f32(float Value)
{
    return as_float(__builtin_spirv_OpSubgroupFirstInvocationKHR_i32(as_uint(Value)));
}

#if defined(cl_khr_subgroup_ballot)

DEFN_NON_UNIFORM_BROADCAST(uchar,  i8)
DEFN_NON_UNIFORM_BROADCAST(ushort, i16)
DEFN_NON_UNIFORM_BROADCAST(uint,   i32)
DEFN_NON_UNIFORM_BROADCAST(ulong,  i64)
DEFN_NON_UNIFORM_BROADCAST(float,  f32)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_BROADCAST(half,   f16)
#endif // defined(cl_khr_fp16)

uint4 __builtin_spirv_OpGroupNonUniformBallot_i32_i1(uint Execution, bool Predicate)
{
    uint4 v = 0;
    if (Execution == Subgroup)
    {
        v.x = __builtin_IB_WaveBallot(Predicate);
    }
    return v;
}

bool __builtin_spirv_OpGroupNonUniformInverseBallot_i32_v4i32(uint Execution, uint4 Value)
{
    if (Execution == Subgroup)
    {
        return (Value.x & (1 << __builtin_IB_get_simd_id())) ? true : false;
    }
    return false;
}

bool __builtin_spirv_OpGroupNonUniformBallotBitExtract_i32_v4i32_i32(uint Execution, uint4 Value, uint Index)
{
    if (Execution == Subgroup)
    {
        return (Value.x & (1 << Index)) ? true : false;
    }
    return false;
}

uint __builtin_spirv_OpGroupNonUniformBallotBitCount_i32_i32_v4i32(uint Execution, uint Operation, uint4 Value)
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

uint __builtin_spirv_OpGroupNonUniformBallotFindLSB_i32_v4i32(uint Execution, uint4 Value)
{
    if (Execution == Subgroup)
    {
        return __builtin_spirv_OpenCL_ctz_i32(Value.x);
    }
    return 0;
}

uint __builtin_spirv_OpGroupNonUniformBallotFindMSB_i32_v4i32(uint Execution, uint4 Value)
{
    if (Execution == Subgroup)
    {
        uint sgsize = SPIRV_BUILTIN_NO_OP(BuiltInSubgroupSize, , )();
        uint consideredBits = Value.x << (32 - sgsize);
        return (sgsize - 1) - __builtin_spirv_OpenCL_clz_i32(consideredBits);
    }
    return 0;
}

uint4 __builtin_spirv_BuiltInSubgroupEqMask()
{
    return __builtin_spirv_BuiltInSubgroupEqMaskKHR();
}

uint4 __builtin_spirv_BuiltInSubgroupGeMask()
{
    return __builtin_spirv_BuiltInSubgroupGeMaskKHR();
}

uint4 __builtin_spirv_BuiltInSubgroupGtMask()
{
    return __builtin_spirv_BuiltInSubgroupGtMaskKHR();
}

uint4 __builtin_spirv_BuiltInSubgroupLeMask()
{
    return __builtin_spirv_BuiltInSubgroupLeMaskKHR();
}

uint4 __builtin_spirv_BuiltInSubgroupLtMask()
{
    return __builtin_spirv_BuiltInSubgroupLtMaskKHR();
}
#endif // defined(cl_khr_subgroup_ballot)

DEFN_BINARY_OPERATIONS(bool)
DEFN_BINARY_OPERATIONS(uchar)
DEFN_BINARY_OPERATIONS(ushort)
DEFN_BINARY_OPERATIONS(uint)
DEFN_BINARY_OPERATIONS(ulong)

DEFN_ARITH_OPERATIONS(uchar)
DEFN_ARITH_OPERATIONS(ushort)
DEFN_ARITH_OPERATIONS(uint)
DEFN_ARITH_OPERATIONS(ulong)
DEFN_ARITH_OPERATIONS(float)
#if defined(cl_khr_fp16)
DEFN_ARITH_OPERATIONS(half)
#endif // defined(cl_khr_fp16)

// ---- Add ----
DEFN_UNIFORM_GROUP_FUNC(IAdd, uchar,  i8,  __intel_add, 0)
DEFN_UNIFORM_GROUP_FUNC(IAdd, ushort, i16, __intel_add, 0)
DEFN_UNIFORM_GROUP_FUNC(IAdd, uint,   i32, __intel_add, 0)
DEFN_UNIFORM_GROUP_FUNC(IAdd, ulong,  i64, __intel_add, 0)
DEFN_UNIFORM_GROUP_FUNC(FAdd, half,   f16, __intel_add, 0)
DEFN_UNIFORM_GROUP_FUNC(FAdd, float,  f32, __intel_add, 0)

// ---- Min ----
DEFN_UNIFORM_GROUP_FUNC(FMin, half,   f16, __builtin_spirv_OpenCL_fmin_f16_f16, INFINITY)
DEFN_UNIFORM_GROUP_FUNC(FMin, float,  f32, __builtin_spirv_OpenCL_fmin_f32_f32, INFINITY)
DEFN_UNIFORM_GROUP_FUNC(UMin, uchar,  i8,  __builtin_spirv_OpenCL_u_min_i8_i8,   UCHAR_MAX)
DEFN_UNIFORM_GROUP_FUNC(UMin, ushort, i16, __builtin_spirv_OpenCL_u_min_i16_i16, USHRT_MAX)
DEFN_UNIFORM_GROUP_FUNC(UMin, uint,   i32, __builtin_spirv_OpenCL_u_min_i32_i32, UINT_MAX)
DEFN_UNIFORM_GROUP_FUNC(UMin, ulong,  i64, __builtin_spirv_OpenCL_u_min_i64_i64, ULONG_MAX)
DEFN_UNIFORM_GROUP_FUNC(SMin, char,   i8,  __builtin_spirv_OpenCL_s_min_i8_i8,   CHAR_MAX)
DEFN_UNIFORM_GROUP_FUNC(SMin, short,  i16, __builtin_spirv_OpenCL_s_min_i16_i16, SHRT_MAX)
DEFN_UNIFORM_GROUP_FUNC(SMin, int,    i32, __builtin_spirv_OpenCL_s_min_i32_i32, INT_MAX)
DEFN_UNIFORM_GROUP_FUNC(SMin, long,   i64, __builtin_spirv_OpenCL_s_min_i64_i64, LONG_MAX)

// ---- Max ----
DEFN_UNIFORM_GROUP_FUNC(FMax, half,   f16, __builtin_spirv_OpenCL_fmax_f16_f16, -INFINITY)
DEFN_UNIFORM_GROUP_FUNC(FMax, float,  f32, __builtin_spirv_OpenCL_fmax_f32_f32, -INFINITY)
DEFN_UNIFORM_GROUP_FUNC(UMax, uchar,  i8,  __builtin_spirv_OpenCL_u_max_i8_i8,   0)
DEFN_UNIFORM_GROUP_FUNC(UMax, ushort, i16, __builtin_spirv_OpenCL_u_max_i16_i16, 0)
DEFN_UNIFORM_GROUP_FUNC(UMax, uint,   i32, __builtin_spirv_OpenCL_u_max_i32_i32, 0)
DEFN_UNIFORM_GROUP_FUNC(UMax, ulong,  i64, __builtin_spirv_OpenCL_u_max_i64_i64, 0)
DEFN_UNIFORM_GROUP_FUNC(SMax, char,   i8,  __builtin_spirv_OpenCL_s_max_i8_i8,   CHAR_MIN)
DEFN_UNIFORM_GROUP_FUNC(SMax, short,  i16, __builtin_spirv_OpenCL_s_max_i16_i16, SHRT_MIN)
DEFN_UNIFORM_GROUP_FUNC(SMax, int,    i32, __builtin_spirv_OpenCL_s_max_i32_i32, INT_MIN)
DEFN_UNIFORM_GROUP_FUNC(SMax, long,   i64, __builtin_spirv_OpenCL_s_max_i64_i64, LONG_MIN)

#if defined(cl_khr_subgroup_non_uniform_arithmetic) || defined(cl_khr_subgroup_clustered_reduce)

// OpGroupNonUniformIAdd, OpGroupNonUniformFAdd
DEFN_NON_UNIFORM_GROUP_FUNC(IAdd, uchar,  i8,  __intel_add, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(IAdd, ushort, i16, __intel_add, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(IAdd, uint,   i32, __intel_add, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(IAdd, ulong,  i64, __intel_add, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(FAdd, float,  f32, __intel_add, 0)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_GROUP_FUNC(FAdd, half,   f16, __intel_add, 0)
#endif // defined(cl_khr_fp16)

// OpGroupNonUniformSMin, OpGroupNonUniformUMin, OpGroupNonUniformFMin
DEFN_NON_UNIFORM_GROUP_FUNC(SMin, char,   i8,  __builtin_spirv_OpenCL_s_min_i8_i8,   CHAR_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC(UMin, uchar,  i8,  __builtin_spirv_OpenCL_u_min_i8_i8,   UCHAR_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC(SMin, short,  i16, __builtin_spirv_OpenCL_s_min_i16_i16, SHRT_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC(UMin, ushort, i16, __builtin_spirv_OpenCL_u_min_i16_i16, USHRT_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC(SMin, int,    i32, __builtin_spirv_OpenCL_s_min_i32_i32, INT_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC(UMin, uint,   i32, __builtin_spirv_OpenCL_u_min_i32_i32, UINT_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC(SMin, long,   i64, __builtin_spirv_OpenCL_s_min_i64_i64, LONG_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC(UMin, ulong,  i64, __builtin_spirv_OpenCL_u_min_i64_i64, ULONG_MAX)
DEFN_NON_UNIFORM_GROUP_FUNC(FMin, float,  f32, __builtin_spirv_OpenCL_fmin_f32_f32,  INFINITY)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_GROUP_FUNC(FMin, half,   f16, __builtin_spirv_OpenCL_fmin_f16_f16,  INFINITY)
#endif // defined(cl_khr_fp16)

// OpGroupNonUniformSMax, OpGroupNonUniformUMax, OpGroupNonUniformFMax
DEFN_NON_UNIFORM_GROUP_FUNC(SMax, char,   i8,  __builtin_spirv_OpenCL_s_max_i8_i8,   CHAR_MIN)
DEFN_NON_UNIFORM_GROUP_FUNC(UMax, uchar,  i8,  __builtin_spirv_OpenCL_u_max_i8_i8,   0)
DEFN_NON_UNIFORM_GROUP_FUNC(SMax, short,  i16, __builtin_spirv_OpenCL_s_max_i16_i16, SHRT_MIN)
DEFN_NON_UNIFORM_GROUP_FUNC(UMax, ushort, i16, __builtin_spirv_OpenCL_u_max_i16_i16, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(SMax, int,    i32, __builtin_spirv_OpenCL_s_max_i32_i32, INT_MIN)
DEFN_NON_UNIFORM_GROUP_FUNC(UMax, uint,   i32, __builtin_spirv_OpenCL_u_max_i32_i32, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(SMax, long,   i64, __builtin_spirv_OpenCL_s_max_i64_i64, LONG_MIN)
DEFN_NON_UNIFORM_GROUP_FUNC(UMax, ulong,  i64, __builtin_spirv_OpenCL_u_max_i64_i64, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(FMax, float,  f32, __builtin_spirv_OpenCL_fmax_f32_f32, -INFINITY)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_GROUP_FUNC(FMax, half,   f16, __builtin_spirv_OpenCL_fmax_f16_f16, -INFINITY)
#endif // defined(cl_khr_fp16)

// OpGroupNonUniformIMul, OpGroupNonUniformFMul
DEFN_NON_UNIFORM_GROUP_FUNC(IMul, uchar,  i8,  __intel_mul, 1)
DEFN_NON_UNIFORM_GROUP_FUNC(IMul, ushort, i16, __intel_mul, 1)
DEFN_NON_UNIFORM_GROUP_FUNC(IMul, uint,   i32, __intel_mul, 1)
DEFN_NON_UNIFORM_GROUP_FUNC(IMul, ulong,  i64, __intel_mul, 1)
DEFN_NON_UNIFORM_GROUP_FUNC(FMul, float,  f32, __intel_mul, 1)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_GROUP_FUNC(FMul, half,   f16, __intel_mul, 1)
#endif // defined(cl_khr_fp16)

// OpGroupNonUniformBitwiseAnd, OpGroupNonUniformBitwiseOr, OpGroupNonUniformBitwiseXor
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseAnd, uchar,  i8,  __intel_and, 0xFF)
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseAnd, ushort, i16, __intel_and, 0xFFFF)
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseAnd, uint,   i32, __intel_and, 0xFFFFFFFF)
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseAnd, ulong,  i64, __intel_and, 0xFFFFFFFFFFFFFFFF)

DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseOr, uchar,  i8,  __intel_or, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseOr, ushort, i16, __intel_or, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseOr, uint,   i32, __intel_or, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseOr, ulong,  i64, __intel_or, 0)

DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseXor, uchar,  i8,  __intel_xor, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseXor, ushort, i16, __intel_xor, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseXor, uint,   i32, __intel_xor, 0)
DEFN_NON_UNIFORM_GROUP_FUNC(BitwiseXor, ulong,  i64, __intel_xor, 0)

// OpGroupNonUniformLogicalAnd, OpGroupNonUniformLogicalOr, OpGroupNonUniformLogicalXor
DEFN_NON_UNIFORM_GROUP_FUNC(LogicalAnd, bool, i1, __intel_and, 1)
DEFN_NON_UNIFORM_GROUP_FUNC(LogicalOr,  bool, i1, __intel_or,  0)
DEFN_NON_UNIFORM_GROUP_FUNC(LogicalXor, bool, i1, __intel_xor, 0)
#endif // defined(cl_khr_subgroup_non_uniform_arithmetic) || defined(cl_khr_subgroup_clustered_reduce)

#if defined(cl_khr_subgroup_shuffle)
uchar __builtin_spirv_OpGroupNonUniformShuffle_i32_i8_i32(uint Execution, uchar x, uint c)
{
    if (Execution == Subgroup)
    {
        return as_uchar(__builtin_IB_simd_shuffle_c(as_char(x), c));
    }
    return 0;
}

ushort __builtin_spirv_OpGroupNonUniformShuffle_i32_i16_i32(uint Execution, ushort x, uint c)
{
    if (Execution == Subgroup)
    {
        return __builtin_IB_simd_shuffle_us(x, c);
    }
    return 0;
}

uint __builtin_spirv_OpGroupNonUniformShuffle_i32_i32_i32(uint Execution, uint x, uint c)
{
    if (Execution == Subgroup)
    {
        return __builtin_IB_simd_shuffle(x, c);
    }
    return 0;
}

ulong __builtin_spirv_OpGroupNonUniformShuffle_i32_i64_i32(uint Execution, ulong x, uint c)
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

float __builtin_spirv_OpGroupNonUniformShuffle_i32_f32_i32(uint Execution, float x, uint c)
{
    if (Execution == Subgroup)
    {
        return __builtin_IB_simd_shuffle_f(x, c);
    }
    return 0;
}

#if defined(cl_khr_fp16)
half __builtin_spirv_OpGroupNonUniformShuffle_i32_f16_i32(uint Execution, half x, uint c)
{
    if (Execution == Subgroup)
    {
        return __builtin_IB_simd_shuffle_h(x, c);
    }
    return 0;
}
#endif // defined(cl_khr_fp16)

DEFN_SUB_GROUP_SHUFFLE_XOR(uchar, i8)
DEFN_SUB_GROUP_SHUFFLE_XOR(ushort, i16)
DEFN_SUB_GROUP_SHUFFLE_XOR(uint, i32)
DEFN_SUB_GROUP_SHUFFLE_XOR(ulong, i64)
DEFN_SUB_GROUP_SHUFFLE_XOR(float, f32)
#if defined(cl_khr_fp16)
DEFN_SUB_GROUP_SHUFFLE_XOR(half, f16)
#endif // defined(cl_khr_fp16)
#endif // defined(cl_khr_subgroup_shuffle)

#if defined(cl_khr_subgroup_shuffle_relative)
// Shuffle down functions
uchar __builtin_spirv_OpGroupNonUniformShuffleDown_i32_i8_i32(uint Execution, uchar x, uint c)
{
    if (Execution == Subgroup)
    {
        return __builtin_IB_simd_shuffle_down_uc(x, 0, c);
    }
    return 0;
}

ushort __builtin_spirv_OpGroupNonUniformShuffleDown_i32_i16_i32(uint Execution, ushort x, uint c)
{
    if (Execution == Subgroup)
    {
        return __builtin_IB_simd_shuffle_down_us(x, 0, c);
    }
    return 0;
}

uint __builtin_spirv_OpGroupNonUniformShuffleDown_i32_i32_i32(uint Execution, uint x, uint c)
{
    if (Execution == Subgroup)
    {
        return __builtin_IB_simd_shuffle_down(x, 0, c);
    }
    return 0;
}

ulong __builtin_spirv_OpGroupNonUniformShuffleDown_i32_i64_i32(uint Execution, ulong x, uint c)
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

float __builtin_spirv_OpGroupNonUniformShuffleDown_i32_f32_i32(uint Execution, float x, uint c)
{
    if (Execution == Subgroup)
    {
        return as_float(__builtin_IB_simd_shuffle_down(as_uint(x), 0, c));
    }
    return 0;
}

#if defined(cl_khr_fp16)
half __builtin_spirv_OpGroupNonUniformShuffleDown_i32_f16_i32(uint Execution, half x, uint c)
{
    if (Execution == Subgroup)
    {
        return as_half(__builtin_IB_simd_shuffle_down_us(as_ushort(x), 0, c));
    }
    return 0;
}
#endif // defined(cl_khr_fp16)

// Shuffle up functions

DEFN_NON_UNIFORM_SHUFFLE_UP(uchar, i8)
DEFN_NON_UNIFORM_SHUFFLE_UP(ushort, i16)
DEFN_NON_UNIFORM_SHUFFLE_UP(uint, i32)
DEFN_NON_UNIFORM_SHUFFLE_UP(ulong, i64)
DEFN_NON_UNIFORM_SHUFFLE_UP(float, f32)
#if defined(cl_khr_fp16)
DEFN_NON_UNIFORM_SHUFFLE_UP(half, f16)
#endif // defined(cl_khr_fp16)
#endif // defined(cl_khr_subgroup_shuffle_relative)
