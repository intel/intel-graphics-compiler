/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef CM_CL_BARRIER_H
#define CM_CL_BARRIER_H

#include "detail/builtins.h"
#include "exec.h"
#include "vector.h"

#include <opencl_def.h>

namespace cm {
namespace exec {

namespace detail {
enum fence : uint8_t {
  global_coherent_fence = 1,
  l3_flush_instructions = 2,
  l3_flush_texture_data = 4,
  l3_flush_constant_data = 8,
  l3_flush_rw_data = 16,
  local_barrier = 32,
  l1_flush_ro_data = 64,
  sw_barrier = 128,
};

enum semantics : int {
  relaxed = 0,
  acquire = 1 << 1,
  release = 1 << 2,
  acquire_release = 1 << 3
};
} // namespace detail

enum scope : int {
  cross_device = 0,
  device = 1,
  workgroup = 2,
  subgroup = 3,
  invocation = 4
};

inline void fence_global(int semantics) {
  uint8_t mode = detail::fence::global_coherent_fence;
  bool invalidate_L1 = semantics & (detail::semantics::acquire |
                                    detail::semantics::acquire_release);
  if (invalidate_L1)
    mode |= detail::fence::l1_flush_ro_data;
  if (semantics != detail::semantics::relaxed)
    cm::detail::__cm_cl_fence(mode);
}

inline void fence_local(int semantics) {
  const uint8_t mode = detail::fence::global_coherent_fence |
                       detail::fence::local_barrier | detail::fence::sw_barrier;

  if (semantics != detail::semantics::relaxed)
    cm::detail::__cm_cl_fence(mode);
}

inline void fence(int scope, int semantics) {
  if (scope == scope::workgroup)
    fence_local(semantics);
  else
    fence_global(semantics);
}

inline void barrier_arrive(int scope) {
  if (scope == scope::workgroup)
    cm::detail::__cm_cl_sbarrier(1);
}

inline void barrier_wait(int scope) {
  if (scope == scope::workgroup)
    cm::detail::__cm_cl_sbarrier(0);
}

inline void local_barrier() { cm::detail::__cm_cl_barrier(); }

inline void global_barrier() {
  fence_global(detail::semantics::acquire_release);
  local_barrier();

  __global uint8_t *sync_buffer = cm::detail::sync_buffer();

  bool is_first_item =
      (get_local_id(0) | get_local_id(1) | get_local_id(2)) == 0;

  uint32_t group_id = get_group_linear_id();

  // Signal that a group hit the global barrier.
  if (is_first_item) {
    sync_buffer[group_id] = 1;
    fence_global(detail::semantics::release); // write fence
  }

  uint32_t num_groups = get_group_linear_count();

  // The last group controls that the others hit
  // the global barrier.
  if (group_id == (num_groups - 1)) {
    uint32_t local_size = get_local_linear_size();
    uint8_t Value;
    do {
      fence_global(detail::semantics::acquire); // read fence
      Value = 1;
      for (uint32_t local_id = get_local_linear_id(); local_id < num_groups;
           local_id += local_size)
        Value = Value & sync_buffer[local_id];
    } while (Value == 0);

    fence_global(detail::semantics::acquire_release);
    local_barrier();

    // Global barrier is complete.
    for (uint32_t local_id = get_local_linear_id(); local_id < num_groups;
         local_id += local_size)
      sync_buffer[local_id] = 0;
    fence_global(detail::semantics::release); // write fence
  }

  // The first items wait for the last group.
  if (is_first_item)
    while (sync_buffer[group_id] != 0)
      fence_global(detail::semantics::acquire); // read fence

  // Other items wait for the first ones.
  fence_global(detail::semantics::acquire_release);
  local_barrier();
}

} // namespace exec
} // namespace cm

#endif // CM_CL_BARRIER_H
