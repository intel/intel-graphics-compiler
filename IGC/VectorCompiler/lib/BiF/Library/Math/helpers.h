/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef BIF_LIBRARY_MATH_F64_HELPERS_H
#define BIF_LIBRARY_MATH_F64_HELPERS_H

#include <cm-cl/vector.h>

template <unsigned N>
CM_NODEBUG CM_INLINE cm::vector<uint64_t, N>
__impl_combineLoHi(cm::vector<uint32_t, N> Lo, cm::vector<uint32_t, N> Hi) {
  cm::vector<uint32_t, 2 * N> Res;
  Res.template select<N, 2>(1) = Hi;
  Res.template select<N, 2>(0) = Lo;
  return Res.template format<uint64_t>();
}

#endif // BIF_LIBRARY_MATH_F64_HELPERS_H
