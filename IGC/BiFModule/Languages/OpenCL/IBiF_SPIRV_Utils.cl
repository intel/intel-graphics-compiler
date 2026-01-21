/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//*****************************************************************************/
// Functions to convert enum of memory_fence and cl_mem_fence_flags to
// to corresponding SPIRV equivalents
//*****************************************************************************/

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
static inline Scope_t get_spirv_mem_scope(memory_scope scope)
{
    switch (scope)
    {
    case memory_scope_work_item:
        return Invocation;
    case memory_scope_sub_group:
        return Subgroup;
    case memory_scope_work_group:
        return Workgroup;
    case memory_scope_device:
        return Device;
    case memory_scope_all_svm_devices:
        return CrossDevice;
    default:
        return CrossDevice;
    }
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

static inline uint get_spirv_mem_fence(cl_mem_fence_flags flag)
{
    uint result = 0;

    if (flag & CLK_GLOBAL_MEM_FENCE)
    {
        result |= CrossWorkgroupMemory;
    }

    if (flag & CLK_LOCAL_MEM_FENCE)
    {
        result |= WorkgroupMemory;
    }

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
    if (flag & CLK_IMAGE_MEM_FENCE)
    {
        result |= ImageMemory;
    }
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

    return result;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
static inline uint get_spirv_mem_order(memory_order order)
{
    switch (order)
    {
    case memory_order_relaxed:
        return Relaxed;
    case memory_order_acquire:
        return Acquire;
    case memory_order_release:
        return Release;
    case memory_order_acq_rel:
        return AcquireRelease;
    case memory_order_seq_cst:
        return SequentiallyConsistent;
    default:
        return 0;
    }
}
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
