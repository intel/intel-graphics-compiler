/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef CM_CL_SVM_H
#define CM_CL_SVM_H

#include "define.h"
#include "detail/builtins.h"
#include "vector.h"

#include <opencl_def.h>

namespace cm {
namespace svm {

template <typename T, int width, int src_width>
void scatter(vector<uintptr_t, width> address, vector<T, src_width> src) {
  static_assert(src_width % width == 0,
                "src width must be a multiple of address width");
  constexpr int num_blocks = src_width / width;
  static_assert(num_blocks == 1, "only one block is yet supported");
  detail::svm_scatter<num_blocks>(
      static_cast<vector<uint64_t, width>>(address).cl_vector(),
      src.cl_vector());
}

template <atomic::operation op, typename T, int width>
vector<T, width> atomic(vector<uintptr_t, width> address,
                        vector<T, width> src) {
  static_assert(op == atomic::operation::add, "only add is yet supported");
  return detail::svm_atomic_add(
      static_cast<vector<uint64_t, width>>(address).cl_vector(),
      src.cl_vector());
}

} // namespace svm
} // namespace cm

#endif // CM_CL_SVM_H
