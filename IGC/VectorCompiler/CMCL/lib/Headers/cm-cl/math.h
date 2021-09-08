/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef CM_CL_MATH_H
#define CM_CL_MATH_H

#include "detail/builtins.h"
#include "vector.h"

#include <opencl_def.h>

namespace cm {
namespace math {

inline uint32_t count_leading_zeros(uint32_t src) { return detail::lzd(src); }

template <int width>
vector<uint32_t, width> count_leading_zeros(vector<uint32_t, width> src) {
  return detail::lzd(src.cl_vector());
}

} // namespace math
} // namespace cm

#endif // CM_CL_MATH_H
