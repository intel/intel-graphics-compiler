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

extern __constant int __UseNative64BitSubgroupBuiltin;

// Group Instructions

uint __intel_LocalInvocationIndex();
uint __intel_WorkgroupSize();
uint OVERLOADABLE __intel_LocalInvocationId(uint dim);

double OVERLOADABLE intel_sub_group_shuffle( double X, uint c );

double OVERLOADABLE intel_sub_group_shuffle_up( double identity, double X, uint c );

// L2G

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1f64_p3f64_i64_i64_i64(uint Execution, global double *Destination, const local double *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1f64_p3f64_i32_i32_i64(uint Execution, global double *Destination, const local double *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2f64_p3v2f64_i64_i64_i64(uint Execution, global double2 *Destination, const local double2 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v2f64_p3v2f64_i32_i32_i64(uint Execution, global double2 *Destination, const local double2 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3f64_p3v3f64_i64_i64_i64(uint Execution, global double3 *Destination, const local double3 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v3f64_p3v3f64_i32_i32_i64(uint Execution, global double3 *Destination, const local double3 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4f64_p3v4f64_i64_i64_i64(uint Execution, global double4 *Destination, const local double4 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v4f64_p3v4f64_i32_i32_i64(uint Execution, global double4 *Destination, const local double4 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8f64_p3v8f64_i64_i64_i64(uint Execution, global double8 *Destination, const local double8 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v8f64_p3v8f64_i32_i32_i64(uint Execution, global double8 *Destination, const local double8 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16f64_p3v16f64_i64_i64_i64(uint Execution, global double16 *Destination, const local double16 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p1v16f64_p3v16f64_i32_i32_i64(uint Execution, global double16 *Destination, const local double16 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_L2G(Destination, Source, NumElements, Stride, Event, uint)
}


// G2L

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3f64_p1f64_i64_i64_i64(uint Execution, local double *Destination, const global double *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3f64_p1f64_i32_i32_i64(uint Execution, local double *Destination, const global double *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2f64_p1v2f64_i64_i64_i64(uint Execution, local double2 *Destination, const global double2 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v2f64_p1v2f64_i32_i32_i64(uint Execution, local double2 *Destination, const global double2 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3f64_p1v3f64_i64_i64_i64(uint Execution, local double3 *Destination, const global double3 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v3f64_p1v3f64_i32_i32_i64(uint Execution, local double3 *Destination, const global double3 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4f64_p1v4f64_i64_i64_i64(uint Execution, local double4 *Destination, const global double4 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v4f64_p1v4f64_i32_i32_i64(uint Execution, local double4 *Destination, const global double4 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8f64_p1v8f64_i64_i64_i64(uint Execution, local double8 *Destination, const global double8 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v8f64_p1v8f64_i32_i32_i64(uint Execution, local double8 *Destination, const global double8 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16f64_p1v16f64_i64_i64_i64(uint Execution, local double16 *Destination, const global double16 *Source, ulong NumElements, ulong Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, ulong)
}

Event_t __builtin_spirv_OpGroupAsyncCopy_i32_p3v16f64_p1v16f64_i32_i32_i64(uint Execution, local double16 *Destination, const global double16 *Source, uint NumElements, uint Stride, Event_t Event)
{
    ASYNC_COPY_G2L(Destination, Source, NumElements, Stride, Event, uint)
}

#if defined(cl_khr_subgroup_non_uniform_vote)
DEFN_NON_UNIFORM_ALL_EQUAL(double, f64)
#endif

double __builtin_spirv_OpGroupBroadcast_i32_f64_v3i32(uint Execution, double Value, uint3 LocalId)
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


double __builtin_spirv_OpGroupBroadcast_i32_f64_v3i64(uint Execution, double Value, ulong3 LocalId)
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

double __builtin_spirv_OpGroupBroadcast_i32_f64_v2i32(uint Execution, double Value, uint2 LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_f64_v3i32(Execution,Value,(uint3)(LocalId.s0,LocalId.s1,0));
}

double __builtin_spirv_OpGroupBroadcast_i32_f64_v2i64(uint Execution, double Value, ulong2 LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_f64_v3i64(Execution,Value,(ulong3)(LocalId.s0,LocalId.s1,0));
}

double __builtin_spirv_OpGroupBroadcast_i32_f64_i32(uint Execution, double Value, uint LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_f64_v3i32(Execution,Value,(uint3)(LocalId,0,0));
}

double __builtin_spirv_OpGroupBroadcast_i32_f64_i64(uint Execution, double Value, ulong LocalId)
{
    return __builtin_spirv_OpGroupBroadcast_i32_f64_v3i64(Execution,Value,(ulong3)(LocalId,0,0));
}

DEFN_SUB_GROUP_BROADCAST_VEC(double, f64)

#if defined(cl_khr_subgroup_ballot)
DEFN_NON_UNIFORM_BROADCAST(double, f64)
#endif

DEFN_ARITH_OPERATIONS(double)

DEFN_UNIFORM_GROUP_FUNC(FAdd, double, f64, __intel_add, 0)

DEFN_UNIFORM_GROUP_FUNC(FMin, double, f64, __builtin_spirv_OpenCL_fmin_f64_f64, INFINITY)

DEFN_UNIFORM_GROUP_FUNC(FMax, double, f64, __builtin_spirv_OpenCL_fmax_f64_f64, -INFINITY)

#if defined(cl_khr_subgroup_non_uniform_arithmetic) || defined(cl_khr_subgroup_clustered_reduce)

DEFN_NON_UNIFORM_GROUP_FUNC(FAdd, double, f64, __intel_add, 0)

DEFN_NON_UNIFORM_GROUP_FUNC(FMin, double, f64, __builtin_spirv_OpenCL_fmin_f64_f64,  INFINITY)

DEFN_NON_UNIFORM_GROUP_FUNC(FMax, double, f64, __builtin_spirv_OpenCL_fmax_f64_f64, -INFINITY)

DEFN_NON_UNIFORM_GROUP_FUNC(FMul, double, f64, __intel_mul, 1)

#endif

#if defined(cl_khr_subgroup_shuffle)

double __builtin_spirv_OpGroupNonUniformShuffle_i32_f64_i32(uint Execution, double x, uint c)
{
    if (Execution == Subgroup)
    {
        return __builtin_IB_simd_shuffle_df(x, c);
    }
    return 0;
}

DEFN_SUB_GROUP_SHUFFLE_XOR(double, f64)

#endif

#if defined(cl_khr_subgroup_shuffle_relative)

double __builtin_spirv_OpGroupNonUniformShuffleDown_i32_f64_i32(uint Execution, double x, uint c)
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

DEFN_NON_UNIFORM_SHUFFLE_UP(double, f64)

#endif
