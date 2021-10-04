/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef CM_CL_MATH_H
#define CM_CL_MATH_H

#include "detail/builtins.h"
#include "vector.h"

#include <opencl_def.h>
#include <opencl_utility.h>

namespace cm {
namespace math {

inline uint32_t count_leading_zeros(uint32_t src) { return detail::lzd(src); }

template <int width>
vector<uint32_t, width> count_leading_zeros(vector<uint32_t, width> src) {
  return detail::lzd(src.cl_vector());
}

// FIXME: replace char with bool.
inline cl::pair<uint32_t, char> add_with_carry(uint32_t src0, uint32_t src1) {
  return detail::addc(src0, src1);
}

template <int width>
cl::pair<vector<uint32_t, width>, vector<char, width>>
add_with_carry(vector<uint32_t, width> src0, vector<uint32_t, width> src1) {
#if __clang_major__ > 9
  return detail::addc(src0.cl_vector(), src1.cl_vector());
#else  // __clang_major__ > 9
  // clang-9 has some issues with type deduction, it needs help.
  auto res = detail::addc(src0.cl_vector(), src1.cl_vector());
  return {res.first, res.second};
#endif // __clang_major__ > 9
}

inline bool is_ordered(float src0, float src1) {
  return (src0 == src0) && (src1 == src1);
}

inline bool is_ordered(double src0, double src1) {
  return (src0 == src0) && (src1 == src1);
}

} // namespace math
} // namespace cm

#endif // CM_CL_MATH_H
