/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IBiF_SPIRV_Utils.h"

//*****************************************************************************/
// Synchronization functions
//*****************************************************************************/

INLINE void OVERLOADABLE work_group_barrier(cl_mem_fence_flags flags) {
  __spirv_ControlBarrier(Workgroup, Device, AcquireRelease | get_spirv_mem_fence(flags));
}

INLINE void OVERLOADABLE barrier(cl_mem_fence_flags flags)
{
  // 2.0 barrier is able to accept CLK_IMAGE_MEM_FENCE
  work_group_barrier(flags);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE void OVERLOADABLE work_group_barrier(cl_mem_fence_flags flags, memory_scope scope) {
  __spirv_ControlBarrier(get_spirv_mem_scope(scope), Device, AcquireRelease | get_spirv_mem_fence(flags));
}

INLINE void OVERLOADABLE read_mem_fence(cl_mem_fence_flags flags) {
  atomic_work_item_fence(flags & (CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE), memory_order_acq_rel,
                         memory_scope_work_group);
}

INLINE void OVERLOADABLE write_mem_fence(cl_mem_fence_flags flags) {
  atomic_work_item_fence(flags & (CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE), memory_order_acq_rel,
                         memory_scope_work_group);
}

INLINE void OVERLOADABLE mem_fence(cl_mem_fence_flags flags) {
  // Notice that per 2.0 CLK_IMAGE_MEM_FENCE is not supported as a flag argument for a mem_fence
  atomic_work_item_fence(flags & (CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE), memory_order_acq_rel,
                         memory_scope_work_group);
}

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

//*****************************************************************************/
// wait_group_events()
//*****************************************************************************/
INLINE void OVERLOADABLE wait_group_events(int num_events, __private event_t *event_list) {
  barrier(CLK_GLOBAL_MEM_FENCE);
  barrier(CLK_LOCAL_MEM_FENCE);
}

// here for backwards compatibility with clang/llvm 3.0
// wait_group_events().
INLINE void OVERLOADABLE wait_group_events(int num_events, __private uint *event_list) {
  barrier(CLK_GLOBAL_MEM_FENCE);
  barrier(CLK_LOCAL_MEM_FENCE);
}
