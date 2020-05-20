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

#ifndef LLVM_GENXOPTS_UTILS_GENXSTLEXTRAS_H
#define LLVM_GENXOPTS_UTILS_GENXSTLEXTRAS_H

#include <iterator>

namespace llvm {
namespace genx {

namespace ranges {

template <typename Range>
using iterator_t = decltype(std::begin(std::declval<Range&>()));

template <typename Range>
using range_pointer_t =
    typename std::iterator_traits<iterator_t<Range>>::pointer;

template <typename Range>
using range_reference_t =
    typename std::iterator_traits<iterator_t<Range>>::reference;

} // namespace ranges

/* Returns the first iterator (let's name it RetIt) such that
 * std::accumulate(First, RetIt, 0) > Bound (not full truth, read below).
 *
 * Arguments:
 * \p First, \p Last - considered range
 * \p Bound - considered Bound
 * \p Op - functor that returns T, takes T and decltype(*First)
 *    respectively as arguments. It is meant to increment current partial sum.
 *    First argument is previous partial sum, second argument is upcoming value
 *    from the range, new partial sum is returned.
 *
 * Arguments of \p PlusEqualOp may not be equal, so the range may possibly point
 * not to T type. In this case partial sum is calculated for transformed range
 * (transformation is hidden in \p Op).
 */
template <typename ForwardIt, typename PlusEqualOp, typename T>
ForwardIt upper_partial_sum_bound(ForwardIt First, ForwardIt Last, T Bound,
                                  PlusEqualOp Op) {
  T CurSum = 0;
  for (; First != Last; ++First) {
    CurSum = Op(CurSum, *First);
    if (CurSum > Bound)
      return First;
  }
  return Last;
}

} // namespace genx
} // namespace llvm

#endif // LLVM_GENXOPTS_UTILS_GENXSTLEXTRAS_H
