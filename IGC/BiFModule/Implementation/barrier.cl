/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../Languages/OpenCL/IBiF_SPIRV_Utils.h"
#include "include/atomic_work_item_fence.h"

// Barrier Instructions

void __intel_workgroup_barrier(int Memory, int Semantics) {
  __intel_atomic_work_item_fence(Memory, Semantics);
  __builtin_IB_thread_group_barrier();
}

void __attribute__((overloadable)) __spirv_ControlBarrier(int Execution, int Memory, int Semantics) {
  if (Execution == Device) {
    global_barrier();
  } else if (Execution == Subgroup) {
    // nothing will be emited but we need to prevent optimization splitting control flow
    __builtin_IB_sub_group_barrier();
  } else if (Execution == Region) {
    region_barrier(0);
  } else if (Execution <= Workgroup) {
    __intel_workgroup_barrier(Memory, Semantics);
  }
}

__global volatile uchar *__builtin_IB_get_sync_buffer();

bool __attribute__((overloadable)) __attribute__((const)) __spirv_BuiltInDeviceBarrierValidINTEL() {
  // The sync buffer is set to null if the device barrier isn't valid.
  return __builtin_IB_get_sync_buffer();
}

void __attribute__((overloadable)) __spirv_SubRegionControlBarrierINTEL(int SubRegionSize, int Memory, int Semantics) {
  region_barrier(SubRegionSize);
}

void __attribute__((overloadable)) __spirv_ControlBarrierArriveINTEL(int Execution, int Memory, int Semantics) {
  if (Execution == Workgroup) {
    __intel_atomic_work_item_fence(Memory, Semantics);
    __builtin_IB_thread_group_barrier_signal();
  }
}

void __attribute__((overloadable)) __spirv_ControlBarrierWaitINTEL(int Execution, int Memory, int Semantics) {
  if (Execution == Workgroup) {
    __intel_atomic_work_item_fence(Memory, Semantics);
    __builtin_IB_thread_group_barrier_wait();
  }
}

bool __intel_is_first_work_group_item(void);
uint __intel_get_local_linear_id(void);
uint __intel_get_global_linear_id(void);
uint __intel_get_local_size(void);

// Implementation of global_barrier using atomic instructions.
// First two integers in the sync buffer are used for global synchronization, they are used interchangeably:
// first call to global_barrier() will use syncBuffer + 0, second call will use syncBuffer + 1, third call will use
// syncBuffer + 0 again, etc. This is to prevent a race condition, where one of the workgroups returned from the barrier
// and some other still wait for the value of the synchronization variable.
void __global_barrier_atomic() {
  __intel_workgroup_barrier(Device, AcquireRelease | CrossWorkgroupMemory);

  bool firstThreadPerWg = (__builtin_IB_get_local_id_x() == 0) && (__builtin_IB_get_local_id_y() == 0) &&
                          (__builtin_IB_get_local_id_z() == 0);
  size_t numGroups = __builtin_IB_get_num_groups(0) * __builtin_IB_get_num_groups(1) * __builtin_IB_get_num_groups(2);

  __global volatile int *syncBuffer = (__global volatile int *)__builtin_IB_get_sync_buffer();
  __global volatile int *offsetVar = syncBuffer + 2;

  if (firstThreadPerWg) {
    int offset = atomic_or(offsetVar, 0);
    __global volatile int *syncVar = syncBuffer + offset;

    if (__intel_get_global_linear_id() == 0) {
      atomic_sub(syncVar, numGroups - 1);
    } else {
      atomic_inc(syncVar);
    }

    // Bypass L1 to observe updates in L3 without invalidating the whole L1 each spin.
    // The _se_ load prevents the compiler from hoisting or eliminating polls.
    // The surrounding workgroup barriers provide memory ordering.
    while (__builtin_IB_lsc_load_se_global_uint((const __global uint *)syncVar, 0, LSC_LDCC_L1UC_L3C) != 0) {
    }

    if (offset) {
      atomic_and(offsetVar, 0);
    } else {
      atomic_or(offsetVar, 1);
    }
  }

  __intel_workgroup_barrier(Device, AcquireRelease | CrossWorkgroupMemory);
}

// Implementation of global_barrier without using atomic instructions except for fences.
void __global_barrier_nonatomic() {
  // Make sure each WKG item hit the barrier.
  __intel_workgroup_barrier(Device, AcquireRelease | CrossWorkgroupMemory);

  __global volatile uchar *syncBuffer = __builtin_IB_get_sync_buffer();
  bool firstThreadPerWg = __intel_is_first_work_group_item();
  uint groupLinearId =
      (__builtin_IB_get_group_id(2) * __builtin_IB_get_num_groups(1) * __builtin_IB_get_num_groups(0)) +
      (__builtin_IB_get_group_id(1) * __builtin_IB_get_num_groups(0)) + __builtin_IB_get_group_id(0);

  // Now first thread of each wkg writes to designated place in syncBuffer
  if (firstThreadPerWg) {
    syncBuffer[groupLinearId] = 1;
    atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_release,
                           memory_scope_device); // == write_mem_fence(CLK_GLOBAL_MEM_FENCE);
  }

  uint numGroups = __builtin_IB_get_num_groups(0) * __builtin_IB_get_num_groups(1) * __builtin_IB_get_num_groups(2);
  // Higher wkg ids tend to not have work to do in all cases, therefore I choose last wkg to wait for the others, as it
  // is most likely it will hit this code sooner.
  if (groupLinearId == (numGroups - 1)) {
    uint localSize = __intel_get_local_size();
    // 24 -48 case
    volatile uchar Value;
    do {
      atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_acquire,
                             memory_scope_device); // == read_mem_fence(CLK_GLOBAL_MEM_FENCE);
      Value = 1;
      for (uint i = __intel_get_local_linear_id(); i < numGroups; i += localSize) {
        Value = Value & syncBuffer[i];
      }

    } while (Value == 0);
    __intel_workgroup_barrier(Device, AcquireRelease | CrossWorkgroupMemory);

    for (uint i = __intel_get_local_linear_id(); i < numGroups; i += localSize) {
      syncBuffer[i] = 0;
    }
    atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_release,
                           memory_scope_device); // == write_mem_fence(CLK_GLOBAL_MEM_FENCE);
  }

  if (firstThreadPerWg) {
    while (syncBuffer[groupLinearId] != 0) {
      atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_acquire,
                             memory_scope_device); // == read_mem_fence(CLK_GLOBAL_MEM_FENCE);
    };
  }
  __intel_workgroup_barrier(Device, AcquireRelease | CrossWorkgroupMemory);
}

void global_barrier() {
  if (BIF_FLAG_CTRL_GET(PlatformType) >= IGFX_NVL) {
    __global_barrier_atomic();
    return;
  }
  __global_barrier_nonatomic();
}

void region_barrier(uint SubRegionSize) {
  uint regionSize = __builtin_IB_get_region_group_wg_count();
  if (SubRegionSize == 0) {
    SubRegionSize = regionSize;
  }
  uint barrierOffset = 0;

  uint workgroupX = __builtin_IB_get_group_id(0) % __builtin_IB_get_region_group_size(0);
  uint workgroupY = __builtin_IB_get_group_id(1) % __builtin_IB_get_region_group_size(1);
  uint workgroupZ = __builtin_IB_get_group_id(2) % __builtin_IB_get_region_group_size(2);
  uint numWorkgroupsX = __builtin_IB_get_region_group_size(0);
  uint numWorkgroupsY = __builtin_IB_get_region_group_size(1);
  uint workgroupIDinRegion = numWorkgroupsX * numWorkgroupsY * workgroupZ + numWorkgroupsX * workgroupY + workgroupX;
  if (SubRegionSize < regionSize) {
    uint subRegionID = workgroupIDinRegion / SubRegionSize;
    barrierOffset += 1 + subRegionID * SubRegionSize;
  }
  uint regionX = __builtin_IB_get_group_id(0) / __builtin_IB_get_region_group_size(0);
  uint regionY = __builtin_IB_get_group_id(1) / __builtin_IB_get_region_group_size(1);
  uint regionZ = __builtin_IB_get_group_id(2) / __builtin_IB_get_region_group_size(2);
  uint numRegionsX = __builtin_IB_get_num_groups(0) / __builtin_IB_get_region_group_size(0);
  uint numRegionsY = __builtin_IB_get_num_groups(1) / __builtin_IB_get_region_group_size(1);
  uint regionID = numRegionsX * numRegionsY * regionZ + numRegionsX * regionY + regionX;

  // regionSize + 1 sync elements per region (1 for region barrier and regionSize for subregion barriers)
  // 2 values per sync element
  __global volatile int *syncBuffer = (__global volatile int *)__builtin_IB_get_region_group_barrier_buffer();
  __global volatile int *mySync = &syncBuffer[2 * (regionID * (regionSize + 1) + barrierOffset)];
  const uint ctr = 0, signal = 1;
  __intel_workgroup_barrier(Region, AcquireRelease | CrossWorkgroupMemory);
  if (__intel_is_first_work_group_item()) {
    if (workgroupIDinRegion % SubRegionSize == 0) {
      while (mySync[ctr] != SubRegionSize - 1) {
        __intel_atomic_work_item_fence(Region, Acquire | CrossWorkgroupMemory);
      }
      mySync[signal] = 1;
      __intel_atomic_work_item_fence(Region, Release | CrossWorkgroupMemory);

      while (mySync[ctr] != 0) {
        __intel_atomic_work_item_fence(Region, Acquire | CrossWorkgroupMemory);
      }
      mySync[signal] = 0;
    } else {
      __builtin_IB_atomic_inc_global_i32(&mySync[ctr]);
      __intel_atomic_work_item_fence(Region, Release | CrossWorkgroupMemory);
      while (mySync[signal] != 1) {
        __intel_atomic_work_item_fence(Region, Acquire | CrossWorkgroupMemory);
      }
      __builtin_IB_atomic_dec_global_i32(&mySync[ctr]);
      __intel_atomic_work_item_fence(Region, Release | CrossWorkgroupMemory);
      while (mySync[signal] != 0) {
        __intel_atomic_work_item_fence(Region, Acquire | CrossWorkgroupMemory);
      }
    }
  }
  __intel_workgroup_barrier(Region, AcquireRelease | CrossWorkgroupMemory);
}

void system_memfence(char fence_typed_memory) { return __builtin_IB_system_memfence(fence_typed_memory); }
