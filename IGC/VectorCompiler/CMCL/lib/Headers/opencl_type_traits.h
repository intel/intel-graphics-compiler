/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef OPENCL_TYPE_TRAITS
#define OPENCL_TYPE_TRAITS
#include <opencl_detail.h>

namespace cl {

namespace detail {

template <typename T> struct type_identity { using type = T; };

template <typename T> auto try_add_lvalue_reference(int) -> type_identity<T &>;
template <typename T, typename G>
auto try_add_lvalue_reference(G) -> type_identity<T>;

template <typename T> auto try_add_rvalue_reference(int) -> type_identity<T &&>;
template <typename T, typename G>
auto try_add_rvalue_reference(G) -> type_identity<T>;

} // namespace detail

template <typename T>
struct add_lvalue_reference : decltype(detail::try_add_lvalue_reference<T>(0)) {
};

template <typename T>
struct add_rvalue_reference : decltype(detail::try_add_rvalue_reference<T>(0)) {
};

template <typename T> struct remove_reference { using type = T; };

template <typename T> struct remove_reference<T &> { using type = T; };

template <typename T> struct remove_reference<T &&> { using type = T; };

template <typename T>
using remove_reference_t = typename remove_reference<T>::type;

template <typename...> using void_t = void;

template <typename T> typename add_rvalue_reference<T>::type declval() noexcept;

template <typename T, T v> struct integral_constant {
  static __constant constexpr T value = v;
  using value_type = T;
  using type = integral_constant;
  constexpr operator value_type() const noexcept { return value; }
  constexpr value_type operator()() const noexcept { return value; }
};

template <bool B> using bool_constant = integral_constant<bool, B>;
using true_type = bool_constant<true>;
using false_type = bool_constant<false>;

template <typename T, typename U> struct is_same : false_type {};

template <typename T> struct is_same<T, T> : true_type {};

template <bool B, typename T, typename F>
using conditional = detail::conditional<B, T, F>;

template <typename T> struct remove_cv { using type = T; };
template <typename T> struct remove_cv<const T> { using type = T; };
template <typename T> struct remove_cv<volatile T> { using type = T; };
template <typename T> struct remove_cv<const volatile T> { using type = T; };

template <typename T> struct pointer_traits;

template <typename T> struct pointer_traits<__generic T *> {
  using pointer = __generic T *;
  using element_type = T;
};

template <typename T> struct pointer_traits<__local T *> {
  using pointer = __local T *;
  using element_type = T;
};

template <typename T> struct pointer_traits<__global T *> {
  using pointer = __global T *;
  using element_type = T;
};

template <typename T> struct pointer_traits<__private T *> {
  using pointer = __private T *;
  using element_type = T;
};

template <typename T> struct pointer_traits<__constant T *> {
  using pointer = __constant T *;
  using element_type = T;
};

template <typename T>
struct is_floating_point
    : integral_constant<
          bool, is_same<float, typename remove_cv<T>::type>::value ||
                    is_same<double, typename remove_cv<T>::type>::value ||
                    is_same<long double, typename remove_cv<T>::type>::value> {
};

template <typename T>
struct is_integral
    : integral_constant<
          bool,
          is_same<bool, typename remove_cv<T>::type>::value ||
              is_same<char, typename remove_cv<T>::type>::value ||
              is_same<unsigned char, typename remove_cv<T>::type>::value ||
              is_same<short, typename remove_cv<T>::type>::value ||
              is_same<unsigned short, typename remove_cv<T>::type>::value ||
              is_same<int, typename remove_cv<T>::type>::value ||
              is_same<unsigned int, typename remove_cv<T>::type>::value ||
              is_same<long, typename remove_cv<T>::type>::value ||
              is_same<unsigned long, typename remove_cv<T>::type>::value ||
              is_same<long long, typename remove_cv<T>::type>::value ||
              is_same<unsigned long long, typename remove_cv<T>::type>::value> {
};

template <typename T>
struct is_arithmetic
    : integral_constant<bool, is_integral<T>::value ||
                                  is_floating_point<T>::value> {};

template <typename T>
struct is_bool
    : integral_constant<bool,
                        is_same<bool, typename remove_cv<T>::type>::value> {};

template <typename T>
struct is_signed
    : integral_constant<bool, is_arithmetic<T>::value && T(-1) < T(0)> {};

// Specializations for vector_impl
// --------------------------------------------------------
template <typename T, int width>
struct is_integral<cl::detail::vector_impl<T, width>>
    : integral_constant<bool, is_integral<T>::value> {};

template <typename T, int width>
struct is_signed<cl::detail::vector_impl<T, width>>
    : integral_constant<bool, is_signed<T>::value> {};

template <typename T, int width>
struct is_floating_point<cl::detail::vector_impl<T, width>>
    : integral_constant<bool, is_floating_point<T>::value> {};
// --------------------------------------------------------

} // namespace cl

#endif // OPENCL_TYPE_TRAITS
