/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef OPENCL_TYPE_TRAITS
#define OPENCL_TYPE_TRAITS

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

template<bool B, typename T, typename F>
struct conditional { using type = T; };

template<typename T, typename F>
struct conditional<false, T, F> { using type = F; };

} // namespace cl

#endif // OPENCL_TYPE_TRAITS
