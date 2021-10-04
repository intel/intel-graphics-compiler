/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef OPENCL_UTILITY
#define OPENCL_UTILITY

#include <opencl_type_traits.h>

namespace cl {

template <typename T> constexpr T &&forward(remove_reference_t<T> &arg) {
  return static_cast<T &&>(arg);
}

template <typename T> constexpr T &&forward(remove_reference_t<T> &&arg) {
  return static_cast<T &&>(arg);
}

template <typename FirstT, typename SecondT> struct pair {
  using first_type = FirstT;
  using second_type = SecondT;
  first_type first;
  second_type second;

  constexpr pair() {}

  constexpr pair(const first_type &first_in, const second_type &second_in)
      : first{first_in}, second{second_in} {}

  template <typename FirstTIn, typename SecondTIn>
  constexpr pair(FirstTIn &&first_in, SecondTIn &&second_in)
      : first(forward<FirstTIn>(first_in)),
        second(forward<SecondTIn>(second_in)) {}

  constexpr pair(const pair &) = default;
  constexpr pair(pair &&) = default;

  template <typename FirstTIn, typename SecondTIn>
  constexpr pair(const pair<FirstTIn, SecondTIn> &in)
      : first(in.first), second(in.second) {}

  template <typename FirstTIn, typename SecondTIn>
  constexpr pair(pair<FirstTIn, SecondTIn> &&in)
      : first(forward<FirstTIn>(in.first)),
        second(forward<SecondTIn>(in.second)) {}

  constexpr pair &operator=(const pair &) = default;
  constexpr pair &operator=(pair &&) = default;

  template <typename FirstTIn, typename SecondTIn>
  constexpr pair &operator=(const pair<FirstTIn, SecondTIn> &in) {
    first = in.first;
    second = in.second;
    return *this;
  }

  template <typename FirstTIn, typename SecondTIn>
  constexpr pair &operator=(pair<FirstTIn, SecondTIn> &&in) {
    first = forward<FirstTIn>(in.first);
    second = forward<SecondTIn>(in.second);
    return *this;
  }

  friend bool operator==(const pair &lhs, const pair &rhs) {
    return lhs.first == rhs.first && lhs.second == rhs.second;
  }
};

template <bool B, class T = void> struct enable_if {};

template <class T> struct enable_if<true, T> { using type = T; };

template <bool B, class T = void>
using enable_if_t = typename enable_if<B, T>::type;

} // namespace cl

#endif // OPENCL_UTILITY
