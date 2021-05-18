/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENERAL_STL_EXTRAS_H
#define VC_UTILS_GENERAL_STL_EXTRAS_H

#include <iterator>

namespace vc {

namespace ranges {

template <typename Range>
using iterator_t = decltype(std::begin(std::declval<Range &>()));

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

} // namespace vc

#endif // VC_UTILS_GENERAL_STL_EXTRAS_H
