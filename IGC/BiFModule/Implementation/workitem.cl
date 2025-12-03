/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


#define MAX_DIM 2
#define BuiltinVector(BuiltinName) \
    (size_t3)(BuiltinName(0),      \
              BuiltinName(1),      \
              BuiltinName(2))

#ifdef NO_ASSUME_SUPPORT
    #define BuiltinAssumeGE0(s)
    #define BuiltinVectorAssumeGE0(v)
#else
    #define BuiltinAssumeGE0(s) __builtin_assume( s >= 0 )
    #define BuiltinVectorAssumeGE0(v) \
        __builtin_assume( (v.x) >= 0 ); \
        __builtin_assume( (v.y) >= 0 ); \
        __builtin_assume( (v.z) >= 0 );
#endif

// Helper functions prefixed with '__intel'

uint __intel_WorkgroupSize()
{
    uint totalWorkGroupSize =
        __builtin_IB_get_local_size(0) *
        __builtin_IB_get_local_size(1) *
        __builtin_IB_get_local_size(2);

    BuiltinAssumeGE0(totalWorkGroupSize);
    return totalWorkGroupSize;
}

size_t __intel_EnqueuedWorkgroupSize()
{
    size_t totalWorkGroupSize =
        (size_t) __builtin_IB_get_enqueued_local_size(0) *
        (size_t) __builtin_IB_get_enqueued_local_size(1) *
        (size_t) __builtin_IB_get_enqueued_local_size(2);

    BuiltinAssumeGE0(totalWorkGroupSize);
    return totalWorkGroupSize;
}

size_t3 OVERLOADABLE __intel_WorkgroupId()
{
    size_t3 v = BuiltinVector(__builtin_IB_get_group_id);
    BuiltinVectorAssumeGE0(v);
    return v;
}

size_t3 OVERLOADABLE __intel_NumWorkgroups()
{
    size_t3 v = BuiltinVector(__builtin_IB_get_num_groups);
    BuiltinVectorAssumeGE0(v);
    return v;
}

uint OVERLOADABLE __intel_LocalInvocationId(uint dim)
{
    uint v = 0;
    if (dim == 0) {
        v = __builtin_IB_get_local_id_x();
    }
    else if (dim == 1) {
        v = __builtin_IB_get_local_id_y();
    }
    else if (dim == 2) {
        v = __builtin_IB_get_local_id_z();
    }

    // local id is a 16 bit number in curbe.
#ifndef NO_ASSUME_SUPPORT
    __builtin_assume(v >= 0);
    __builtin_assume(v <= 0xffff);
#endif
    return v;
}

size_t3 OVERLOADABLE __intel_LocalInvocationId()
{
    size_t3 v = (size_t3)(__builtin_IB_get_local_id_x(),
                     __builtin_IB_get_local_id_y(),
                     __builtin_IB_get_local_id_z());

    BuiltinVectorAssumeGE0(v);
#ifndef NO_ASSUME_SUPPORT
    __builtin_assume(v.x <= 0xffff);
    __builtin_assume(v.y <= 0xffff);
    __builtin_assume(v.z <= 0xffff);
#endif

    return v;
}

size_t OVERLOADABLE __intel_GlobalInvocationId(uint dim)
{
    if (dim > MAX_DIM)
        return 0;

    size_t v =
        (size_t) __builtin_IB_get_group_id(dim) * (size_t) __builtin_IB_get_enqueued_local_size(dim) +
        (size_t) __intel_LocalInvocationId(dim) + (size_t) __builtin_IB_get_global_offset(dim);

#ifndef NO_ASSUME_SUPPORT
     BuiltinAssumeGE0(v);
     // We want to show, that the value is positive.
     // On LLVM level, where the signedness of the type is lost, the only way to prove
     // that the value is positive is to check the sign bit.
     if (BIF_FLAG_CTRL_GET(UseAssumeInGetGlobalId))
        __builtin_assume((v & 0x8000000000000000ULL) == 0);
#endif

    return v;
}

size_t3 OVERLOADABLE __intel_GlobalInvocationId()
{
    return BuiltinVector(__intel_GlobalInvocationId);
}

uint __intel_LocalInvocationIndex()
{
#if 0
    // This doesn't work right now due to a bug in the runtime.
    // If/when they fix their bug we can experiment if spending the
    // register(s) for get_local_linear_id() is better than spending
    // the math to compute the linear local ID.
    return __builtin_IB_get_local_linear_id();
#else
    uint llid;
    llid = (uint)__intel_LocalInvocationId(2);
    llid *= (uint)__builtin_IB_get_local_size(1);
    llid += (uint)__intel_LocalInvocationId(1);
    llid *= (uint)__builtin_IB_get_local_size(0);
    llid += (uint)__intel_LocalInvocationId(0);

    BuiltinAssumeGE0(llid);
    return llid;
#endif
}

////////////////////////

size_t OVERLOADABLE __spirv_BuiltInNumWorkgroups(int dimindx)
{
    size_t v = __builtin_IB_get_num_groups(dimindx);
    BuiltinAssumeGE0(v);
    return v;
}

size_t OVERLOADABLE __spirv_BuiltInWorkgroupSize(int dimindx)
{
    size_t v = __builtin_IB_get_local_size(dimindx);
    BuiltinAssumeGE0(v);
    return v;
}

size_t OVERLOADABLE __spirv_BuiltInWorkgroupId(int dimindx)
{
    size_t v = __builtin_IB_get_group_id(dimindx);
    BuiltinAssumeGE0(v);
    return v;
}

size_t OVERLOADABLE __spirv_BuiltInLocalInvocationId(int dimindx)
{
    return __intel_LocalInvocationId(dimindx);
}

size_t OVERLOADABLE __spirv_BuiltInGlobalInvocationId(int dimindx)
{
    return __intel_GlobalInvocationId(dimindx);
}

size_t OVERLOADABLE __spirv_BuiltInGlobalSize(int dimindx)
{
    size_t v = __builtin_IB_get_global_size(dimindx);
    BuiltinAssumeGE0(v);
    return v;
}

size_t OVERLOADABLE __spirv_BuiltInEnqueuedWorkgroupSize(int dimindx)
{
    size_t v = __builtin_IB_get_enqueued_local_size(dimindx);
    BuiltinAssumeGE0(v);
    return v;
}

size_t OVERLOADABLE __spirv_BuiltInGlobalOffset(int dimindx)
{
    return __builtin_IB_get_global_offset(dimindx);
}

size_t __attribute__((overloadable)) __spirv_BuiltInGlobalLinearId()
{
  uint dim = __spirv_BuiltInWorkDim();
  size_t result = 0;

  switch (dim) {
    default:
    case 1:
    {
      size_t gid0 = __intel_GlobalInvocationId(0);
      size_t globalOffset0 = __builtin_IB_get_global_offset(0);
      result = gid0 - globalOffset0;
      break;
    }
    case 2:
    {
      size_t gid0 = __intel_GlobalInvocationId(0);
      size_t gid1 = __intel_GlobalInvocationId(1);
      size_t globalOffset0 = __builtin_IB_get_global_offset(0);
      size_t globalOffset1 = __builtin_IB_get_global_offset(1);
      size_t globalSize0 = __builtin_IB_get_global_size(0);
      result = (gid1 - globalOffset1) * globalSize0 +
               (gid0 - globalOffset0);
      break;
    }
    case 3:
    {
      size_t gid0 = __intel_GlobalInvocationId(0);
      size_t gid1 = __intel_GlobalInvocationId(1);
      size_t gid2 = __intel_GlobalInvocationId(2);
      size_t globalOffset0 = __builtin_IB_get_global_offset(0);
      size_t globalOffset1 = __builtin_IB_get_global_offset(1);
      size_t globalOffset2 = __builtin_IB_get_global_offset(2);
      size_t globalSize0 = __builtin_IB_get_global_size(0);
      size_t globalSize1 = __builtin_IB_get_global_size(1);
      result = ((gid2 - globalOffset2) * globalSize1 * globalSize0) +
               ((gid1 - globalOffset1) * globalSize0) +
               (gid0 - globalOffset0);
      break;
    }
  }

  BuiltinAssumeGE0(result);
  return result;
}

size_t __attribute__((overloadable)) __spirv_BuiltInLocalInvocationIndex()
{
    return __intel_LocalInvocationIndex();
}

uint __attribute__((overloadable)) __spirv_BuiltInWorkDim()
{
    uint dim = __builtin_IB_get_work_dim();

#ifndef NO_ASSUME_SUPPORT
    __builtin_assume(dim >= 0);
    __builtin_assume(dim <= 3);
#endif

    return dim;
}

uint __attribute__((overloadable)) __spirv_BuiltInSubgroupMaxSize()
{
    uint v = __builtin_IB_get_simd_size();

#ifndef NO_ASSUME_SUPPORT
    __builtin_assume(v >= 8);
    __builtin_assume(v <= 32);
#endif

    return v;
}

uint __attribute__((overloadable)) __spirv_BuiltInSubgroupId()
{
    if(BIF_FLAG_CTRL_GET(hasHWLocalThreadID))
    {
        return __builtin_IB_get_local_thread_id();
    }

    uint v = (uint)__spirv_BuiltInLocalInvocationIndex() / __spirv_BuiltInSubgroupMaxSize();

    BuiltinAssumeGE0(v);
    return v;
}

uint __attribute__((overloadable)) __spirv_BuiltInNumSubgroups()
{
    uint totalWorkGroupSize = __intel_WorkgroupSize() + __spirv_BuiltInSubgroupMaxSize() - 1;
    return totalWorkGroupSize / __spirv_BuiltInSubgroupMaxSize();
}

uint __attribute__((overloadable)) __spirv_BuiltInSubgroupSize()
{
    uint    remainder =
                __intel_WorkgroupSize() & ( __spirv_BuiltInSubgroupMaxSize() - 1 );
    bool    fullSubGroup =
                ( remainder == 0 ) ||
                ( __spirv_BuiltInSubgroupId() < __spirv_BuiltInNumSubgroups() - 1 );

    return fullSubGroup ? __spirv_BuiltInSubgroupMaxSize() : remainder;
}

uint __attribute__((overloadable)) __spirv_BuiltInNumEnqueuedSubgroups()
{
    uint totalEnqueuedWorkGroupSize = __intel_EnqueuedWorkgroupSize() + __spirv_BuiltInSubgroupMaxSize() - 1;
    return totalEnqueuedWorkGroupSize / __spirv_BuiltInSubgroupMaxSize();
}

uint __attribute__((overloadable)) __spirv_BuiltInSubgroupLocalInvocationId()
{
    uint simd_id = __builtin_IB_get_simd_id();

#ifndef NO_ASSUME_SUPPORT
    __builtin_assume(simd_id >= 0);
    __builtin_assume(simd_id < 32);
#endif

    return simd_id;
}

// SYCL: get_work_group_range_in_region
size_t OVERLOADABLE __spirv_BuiltInNumWorkgroupsInRegionGroupINTEL(int dim)
{
    return __builtin_IB_get_region_group_size(dim);
}

// SYCL: get_work_group_id_in_region
size_t OVERLOADABLE __spirv_BuiltInWorkgroupIdInRegionGroupINTEL(int dim)
{
    return __spirv_BuiltInWorkgroupId(dim) %
           __spirv_BuiltInNumWorkgroupsInRegionGroupINTEL(dim);
}

// SYCL: get_group_range
size_t OVERLOADABLE __spirv_BuiltInNumRegionGroupsINTEL(int dim)
{
    return __spirv_BuiltInNumWorkgroups(dim) /
           __spirv_BuiltInNumWorkgroupsInRegionGroupINTEL(dim);
}

// SYCL: get_group_id
size_t OVERLOADABLE __spirv_BuiltInRegionGroupIdINTEL(int dim)
{

    return __spirv_BuiltInWorkgroupId(dim) /
           __spirv_BuiltInNumWorkgroupsInRegionGroupINTEL(dim);
}

// SYCL: get_group_linear_id
size_t OVERLOADABLE __spirv_BuiltInRegionGroupIndexINTEL()
{
    size_t RegionGroupIdX = __spirv_BuiltInRegionGroupIdINTEL(0);
    size_t RegionGroupIdY = __spirv_BuiltInRegionGroupIdINTEL(1);
    size_t RegionGroupIdZ = __spirv_BuiltInRegionGroupIdINTEL(2);
    size_t NumRegionGroupsX = __spirv_BuiltInNumRegionGroupsINTEL(0);
    size_t NumRegionGroupsY = __spirv_BuiltInNumRegionGroupsINTEL(1);

    size_t RegionGroupIndex = NumRegionGroupsX * NumRegionGroupsY * RegionGroupIdZ +
                            NumRegionGroupsX * RegionGroupIdY + RegionGroupIdX;
    return RegionGroupIndex;
}

// SYCL: get_local_range
size_t OVERLOADABLE __spirv_BuiltInRegionGroupSizeINTEL(int dim)
{
    return __spirv_BuiltInNumWorkgroupsInRegionGroupINTEL(dim) *
           __spirv_BuiltInWorkgroupSize(dim);
}

// SYCL: get_local_id
size_t OVERLOADABLE __spirv_BuiltInRegionGroupLocalInvocationIdINTEL(int dim)
{
    return __spirv_BuiltInWorkgroupIdInRegionGroupINTEL(dim) *
           __spirv_BuiltInWorkgroupSize(dim) +
           __spirv_BuiltInLocalInvocationId(dim);
}

// SYCL: get_work_group_linear_id_in_region
size_t OVERLOADABLE __spirv_BuiltInWorkgroupIndexInRegionGroupINTEL()
{
    size_t WorkgroupIdX = __spirv_BuiltInWorkgroupIdInRegionGroupINTEL(0);
    size_t WorkgroupIdY = __spirv_BuiltInWorkgroupIdInRegionGroupINTEL(1);
    size_t WorkgroupIdZ = __spirv_BuiltInWorkgroupIdInRegionGroupINTEL(2);
    size_t NumWorkgroupsX = __spirv_BuiltInNumWorkgroupsInRegionGroupINTEL(0);
    size_t NumWorkgroupsY = __spirv_BuiltInNumWorkgroupsInRegionGroupINTEL(1);

    size_t WorkgroupIndex = NumWorkgroupsX * NumWorkgroupsY * WorkgroupIdZ +
                            NumWorkgroupsX * WorkgroupIdY + WorkgroupIdX;
    return WorkgroupIndex;
}

// SYCL: get_local_linear_id
size_t OVERLOADABLE __spirv_BuiltInRegionGroupLocalInvocationIndexINTEL()
{
    size_t WorkgroupId = __spirv_BuiltInWorkgroupIndexInRegionGroupINTEL();
    size_t WorkgroupSize = __spirv_BuiltInWorkgroupSize(0) *
                                __spirv_BuiltInWorkgroupSize(1) *
                                __spirv_BuiltInWorkgroupSize(2);
    size_t LocalId = __spirv_BuiltInLocalInvocationIndex();

    return WorkgroupId * WorkgroupSize + LocalId;
}
