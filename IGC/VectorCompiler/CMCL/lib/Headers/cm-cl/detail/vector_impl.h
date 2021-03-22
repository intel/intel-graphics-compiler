/*========================== begin_copyright_notice ============================

Copyright (c) 2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#ifndef CM_CL_DETAIL_VECTOR_IMPL_H
#define CM_CL_DETAIL_VECTOR_IMPL_H

#include <opencl_type_traits>

namespace cm {
namespace detail {

template<typename T, int width>
using vector_impl = T __attribute__((ext_vector_type(width)));

// offset into vector
using vector_offset_type = short;

template <typename To, typename From, int width>
constexpr int calc_bit_cast_width() {
  if constexpr (sizeof(To) <= sizeof(From))
    return width * (sizeof(From) / sizeof(To));
  else {
    constexpr auto ratio = sizeof(To) / sizeof(From);
    static_assert(width % ratio == 0,
                  "cannot bitcast the provided vector to the requested type");
    return width / ratio;
  }
}

template <typename To, typename From, int width>
constexpr auto bit_cast(vector_impl<From, width> vec) {
  using ret_t = vector_impl<To, calc_bit_cast_width<To, From, width>()>;
  if constexpr (cl::is_same<To, From>::value)
    return vec;
  else
    return __builtin_bit_cast(ret_t, vec);
}

// clang-9 somehow doesn't consider this function constexpr.
#if __clang_major__ > 9
template <typename T, int width>
constexpr int get_width(vector_impl<T, width>) {
  return width;
}
#endif // __clang_major__ > 9

template <typename T> struct width_getter {};

template <typename T, int width>
struct width_getter<__private vector_impl<T, width>>
    : public cl::integral_constant<int, width> {};
template <typename T, int width>
struct width_getter<__global vector_impl<T, width>>
    : public cl::integral_constant<int, width> {};
template <typename T, int width>
struct width_getter<__constant vector_impl<T, width>>
    : public cl::integral_constant<int, width> {};
template <typename T, int width>
struct width_getter<__local vector_impl<T, width>>
    : public cl::integral_constant<int, width> {};
template <typename T, int width>
struct width_getter<__generic vector_impl<T, width>>
    : public cl::integral_constant<int, width> {};

} // namespace detail
} // namespace cm

#endif // CM_CL_DETAIL_VECTOR_IMPL_H
