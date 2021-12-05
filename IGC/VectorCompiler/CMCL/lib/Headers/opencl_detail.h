/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef OPENCL_DETAIL
#define OPENCL_DETAIL

namespace cl {
namespace detail {

template <typename T, int width>
using vector_impl = T __attribute__((ext_vector_type(width)));

template <bool B, typename T, typename F> struct conditional {
    using type = T;
};
template <typename T, typename F> struct conditional<false, T, F> {
    using type = F;
};

} // namespace detail
} // namespace cl

#endif // OPENCL_DETAIL
