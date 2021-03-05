/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#ifndef CM_CL_SVM_H
#define CM_CL_SVM_H

#include "detail/builtins.h"
#include "vector.h"

#include <opencl_def>

namespace cm {
namespace svm {

template <typename T, int width, int src_width>
void scatter(vector<uintptr_t, width> address, vector<T, src_width> src) {
  static_assert(src_width % width == 0,
                "src width must be a multiple of address width");
  constexpr int num_blocks = src_width / width;
  static_assert(num_blocks == 1, "only one block is yet supported");
  detail::svm_scatter<num_blocks>(address.cl_vector(), src.cl_vector());
}

enum class operation {
  add = 0x0,
  sub = 0x1,
  inc = 0x2,
  dec = 0x3,
  min = 0x4,
  max = 0x5,
  xchg = 0x6,
  cmpxchg = 0x7,
  andl = 0x8,
  orl = 0x9,
  xorl = 0xa,
  minsint = 0xb,
  maxsint = 0xc,
  fmax = 0x10,
  fmin = 0x11,
  fcmpwr = 0x12,
  predec = 0xff
};

template <enum operation op, typename T, int width>
vector<T, width> atomic(vector<uintptr_t, width> address,
                        vector<T, width> src) {
  static_assert(op == operation::add, "only add is yet supported");
  return detail::svm_atomic_add(address.cl_vector(), src.cl_vector());
}

} // namespace svm
} // namespace cm

#endif // CM_CL_SVM_H
