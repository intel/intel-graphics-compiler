/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef CM_CL_EXEC_H
#define CM_CL_EXEC_H

#include "detail/builtins.h"
#include "vector.h"

#include <opencl_def.h>

namespace cm {
namespace exec {

enum dimension : int { x = 0, y = 1, z = 2 };
enum scope : int {
  cross_device = 0,
  device = 1,
  workgroup = 2,
  subgroup = 3,
  invocation = 4
};

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
} // namespace detail

inline uint32_t get_local_id(int dim) {
  if (dim > dimension::z || dim < dimension::x)
    return 0;
  return cm::detail::get_local_id()[dim];
}

inline uint32_t get_local_size(int dim) {
  if (dim > dimension::z || dim < dimension::x)
    return 0;
  return cm::detail::get_local_size()[dim];
}

inline uint32_t get_group_count(int dim) {
  if (dim > dimension::z || dim < dimension::x)
    return 0;
  return cm::detail::get_group_count()[dim];
}

inline uint32_t get_group_id(int dim) {
  switch (dim) {
  case 0:
    return cm::detail::get_group_id_x();
  case 1:
    return cm::detail::get_group_id_y();
  case 2:
    return cm::detail::get_group_id_z();
  default:
    return 0;
  }
}

inline void barrier(int scope) {
  if (scope == scope::workgroup)
    cm::detail::__cm_cl_barrier();
}

inline void barrier_arrive(int scope) {
  if (scope == scope::workgroup)
    cm::detail::__cm_cl_sbarrier(1);
}

inline void barrier_wait(int scope) {
  if (scope == scope::workgroup)
    cm::detail::__cm_cl_sbarrier(0);
}

inline void fence(int scope, int semantics) {
  const uint8_t mode = detail::fence::global_coherent_fence |
                       detail::fence::local_barrier |
                       detail::fence::sw_barrier;

  if (semantics != 0)
    cm::detail::__cm_cl_fence(mode);
}

} // namespace exec
} // namespace cm

#endif // CM_CL_EXEC_H
