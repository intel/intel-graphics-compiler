/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/exec.h>

extern "C" {
#include "spirv_atomics_common.h"
}

using namespace cm;
using namespace exec;
using namespace detail;

template <memory_scope OCLScope>
static CM_NODEBUG CM_INLINE void spirvFenceHelperWithKnownScope(int semantics) {
  switch (semantics) {
  default:
  case SequentiallyConsistent:
    return fence<memory_order_seq_cst, OCLScope>();
  case Relaxed:
    return;
  case Acquire:
    return fence<memory_order_acquire, OCLScope>();
  case Release:
    return fence<memory_order_release, OCLScope>();
  case AcquireRelease:
    return fence<memory_order_acq_rel, OCLScope>();
  }
}

static CM_NODEBUG CM_INLINE void spirvFenceHelper(int scope, int semantics) {
  switch (scope) {
  default:
  case CrossDevice:
    return spirvFenceHelperWithKnownScope<memory_scope_all_devices>(semantics);
  case Device:
    return spirvFenceHelperWithKnownScope<memory_scope_device>(semantics);
  case Workgroup:
    return spirvFenceHelperWithKnownScope<memory_scope_work_group>(semantics);
  case Subgroup:
    return spirvFenceHelperWithKnownScope<memory_scope_sub_group>(semantics);
  case Invocation:
    return spirvFenceHelperWithKnownScope<memory_scope_work_item>(semantics);
  }
}

CM_NODEBUG CM_INLINE void __spirv_MemoryBarrier(int scope, int semantics) {
  spirvFenceHelper(scope, semantics);
}

static CM_NODEBUG CM_INLINE void local_barrier() { __cm_cl_barrier(); }

static CM_NODEBUG CM_INLINE void global_barrier() {
  fence<memory_order_acq_rel, memory_scope_device>();
  local_barrier();

  __global uint8_t *sync_buff = sync_buffer();

  bool is_first_item =
      (get_local_id(0) | get_local_id(1) | get_local_id(2)) == 0;

  uint32_t group_id = get_group_linear_id();

  // Signal that a group hit the global barrier.
  if (is_first_item) {
    sync_buff[group_id] = 1;
    fence<memory_order_release, memory_scope_device>();
  }

  uint32_t num_groups = get_group_linear_count();

  // The last group controls that the others hit
  // the global barrier.
  if (group_id == (num_groups - 1)) {
    uint32_t local_size = get_local_linear_size();
    uint8_t Value;
    do {
      fence<memory_order_acquire, memory_scope_device>();
      Value = 1;
      for (uint32_t local_id = get_local_linear_id(); local_id < num_groups;
           local_id += local_size)
        Value = Value & sync_buff[local_id];
    } while (Value == 0);

    fence<memory_order_acq_rel, memory_scope_device>();
    local_barrier();

    // Global barrier is complete.
    for (uint32_t local_id = get_local_linear_id(); local_id < num_groups;
         local_id += local_size)
      sync_buff[local_id] = 0;
    fence<memory_order_release, memory_scope_device>();
  }

  // The first items wait for the last group.
  if (is_first_item)
    while (sync_buff[group_id] != 0)
      fence<memory_order_acquire, memory_scope_device>();

  // Other items wait for the first ones.
  fence<memory_order_acq_rel, memory_scope_device>();
  local_barrier();
}

CM_NODEBUG CM_INLINE void __spirv_ControlBarrier(int scope, int memory_scope,
                                                 int memory_semantics) {
  if (scope == Workgroup) {
    spirvFenceHelper(memory_scope, memory_semantics);
    local_barrier();
  } else if (scope == Device)
    global_barrier();
}

CM_NODEBUG CM_INLINE void
__spirv_ControlBarrierArriveINTEL(int scope, int memory_scope,
                                  int memory_semantics) {
  spirvFenceHelper(memory_scope, memory_semantics);
  if (scope == Workgroup)
    __cm_cl_sbarrier(1);
}

CM_NODEBUG CM_INLINE void
__spirv_ControlBarrierWaitINTEL(int scope, int memory_scope,
                                int memory_semantics) {
  spirvFenceHelper(memory_scope, memory_semantics);
  if (scope == Workgroup)
    __cm_cl_sbarrier(0);
}
