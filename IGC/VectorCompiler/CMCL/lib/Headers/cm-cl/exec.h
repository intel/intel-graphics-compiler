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

} // namespace exec
} // namespace cm

#endif // CM_CL_EXEC_H
