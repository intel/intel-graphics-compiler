/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

__global volatile uchar* __builtin_IB_get_sync_buffer();
bool __attribute__((overloadable)) __attribute__((const)) intel_is_device_barrier_valid()
{
    // The sync buffer is set to null if the device barrier isn't valid.
    return __builtin_IB_get_sync_buffer();
}

void __attribute__((overloadable)) intel_device_barrier(cl_mem_fence_flags flags) {
    global_barrier();
}

void __attribute__((overloadable))
intel_device_barrier(cl_mem_fence_flags flags, memory_scope memscope) {
    global_barrier();
}
