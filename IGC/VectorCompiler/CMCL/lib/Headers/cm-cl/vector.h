/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef CM_CL_VECTOR_H
#define CM_CL_VECTOR_H

#include "define.h"
#include "detail/builtins.h"
#include "detail/vector_impl.h"

#include <opencl_type_traits.h>

namespace cm {

template <typename T, int width>
using cl_vector = detail::vector_impl<T, width>;

template <typename DT, int d_width, int stride, typename OrigT, int orig_width>
class vector_slice;

template <typename T, int width> class vector;

// FIXME: mask should be a separate class, bool type should be utilized.
template <int width> using mask = vector<char, width>;

template<typename T, int width>
class vector {
  using vector_impl = detail::vector_impl<T, width>;
  vector_impl impl;

  template <typename DT, int d_width> friend class vector;

  template <typename DT, int d_width, int stride, typename OrigT,
            int orig_width>
  friend class vector_slice;

public:
  using offset_type = detail::vector_offset_type;

  vector() {}
  vector(T splat) : impl{static_cast<vector_impl>(splat)} {}
  template <typename G>
  vector(cl_vector<G, width> v)
      : impl{__builtin_convertvector(v, vector_impl)} {}
  template <typename G> vector(vector<G, width> v) : vector{v.impl} {}

  vector(const vector &) = default;
  vector(vector &&) = default;
  vector &operator=(const vector &) = default;
  vector &operator=(vector &&) = default;
  ~vector() = default;

  cl_vector<T, width> cl_vector() const { return impl; }

  vector operator+=(vector rhs) {
    impl += rhs.impl;
    return *this;
  }

  vector operator-=(vector rhs) {
    impl -= rhs.impl;
    return *this;
  }

  vector operator*=(vector rhs) {
    impl *= rhs.impl;
    return *this;
  }

  vector operator/=(vector rhs) {
    impl /= rhs.impl;
    return *this;
  }

  vector operator<<=(vector rhs) {
    impl <<= rhs.impl;
    return *this;
  }

  vector operator>>=(vector rhs) {
    impl >>= rhs.impl;
    return *this;
  }

  vector operator~() const {
    return ~impl;
  }

  vector operator-() const {
    return -impl;
  }

  vector operator+() const {
    return impl;
  }

  vector operator&=(vector rhs) {
    impl &= rhs.impl;
    return *this;
  }

  vector operator|=(vector rhs) {
    impl |= rhs.impl;
    return *this;
  }

  vector operator^=(vector rhs) {
    impl ^= rhs.impl;
    return *this;
  }

  vector_slice<T, 1, 1, T, width> operator[](offset_type idx) {
    return {*this, idx};
  }

  template <typename To>
  vector_slice<To, detail::calc_bit_cast_width<To, T, width>(), 1, T, width>
  format() {
    return {*this, 0};
  }

  mask<width> equal(vector rhs) const { return impl == rhs.impl; }

  mask<width> not_equal(vector rhs) const { return impl != rhs.impl; }

  mask<width> less(vector rhs) const { return impl < rhs.impl; }

  mask<width> greater(vector rhs) const { return impl > rhs.impl; }

  mask<width> less_equal(vector rhs) const { return impl <= rhs.impl; }

  mask<width> greater_equal(vector rhs) const { return impl >= rhs.impl; }

  vector merge(vector vec, mask<width> msk) {
    impl = detail::select(msk.impl, vec.impl, impl);
    return *this;
  }

  template <int sub_width, int stride>
  vector_slice<T, sub_width, stride, T, width> select(offset_type offset) {
    return {*this, offset};
  }

  bool all() {
    mask<width> msk = *this != 0;
    return detail::__cm_cl_all(msk.cl_vector());
  }

  bool any() {
    mask<width> msk = *this != 0;
    return detail::__cm_cl_any(msk.cl_vector());
  }

  bool none() {
    mask<width> msk = *this != 0;
    return !detail::__cm_cl_any(msk.cl_vector());
  }
};

#define VECTOR_OPEQ_BASED_BINOP(OP)                                            \
  template <typename LT, typename RT, int width>                               \
  auto operator OP(vector<LT, width> lhs, vector<RT, width> rhs) {             \
    using CT = decltype(cl::declval<LT>() OP cl::declval<RT>());               \
    vector<CT, width> clhs = lhs;                                              \
    vector<CT, width> crhs = rhs;                                              \
    return clhs OP## = crhs;                                                   \
  }                                                                            \
                                                                               \
  template <typename LT, typename RT, int width>                               \
  auto operator OP(vector<LT, width> lhs, RT srhs) {                           \
    vector<RT, width> rhs{srhs};                                               \
    return lhs OP rhs;                                                         \
  }                                                                            \
                                                                               \
  template <typename LT, typename RT, int width>                               \
  auto operator OP(LT slhs, vector<RT, width> rhs) {                           \
    vector<LT, width> lhs{slhs};                                               \
    return lhs OP rhs;                                                         \
  }

VECTOR_OPEQ_BASED_BINOP(+)
VECTOR_OPEQ_BASED_BINOP(-)
VECTOR_OPEQ_BASED_BINOP(*)
VECTOR_OPEQ_BASED_BINOP(/)
VECTOR_OPEQ_BASED_BINOP(<<)
VECTOR_OPEQ_BASED_BINOP(>>)
VECTOR_OPEQ_BASED_BINOP(&)
VECTOR_OPEQ_BASED_BINOP(|)
VECTOR_OPEQ_BASED_BINOP(^)

#define VECTOR_FUNC_BASED_BINOP(FUNC, OP)                                      \
  template <typename LT, typename RT, int width>                               \
  auto operator OP(vector<LT, width> lhs, vector<RT, width> rhs) {             \
    using CT = decltype(cl::declval<LT>() + cl::declval<RT>());                \
    vector<CT, width> clhs = lhs;                                              \
    vector<CT, width> crhs = rhs;                                              \
    return clhs.FUNC(crhs);                                                    \
  }                                                                            \
                                                                               \
  template <typename LT, typename RT, int width>                               \
  auto operator OP(vector<LT, width> lhs, RT srhs) {                           \
    vector<RT, width> rhs{srhs};                                               \
    return lhs OP rhs;                                                         \
  }                                                                            \
                                                                               \
  template <typename LT, typename RT, int width>                               \
  auto operator OP(LT slhs, vector<RT, width> rhs) {                           \
    vector<LT, width> lhs{slhs};                                               \
    return lhs OP rhs;                                                         \
  }

VECTOR_FUNC_BASED_BINOP(equal, ==)
VECTOR_FUNC_BASED_BINOP(not_equal, !=)
VECTOR_FUNC_BASED_BINOP(less, <)
VECTOR_FUNC_BASED_BINOP(greater, >)
VECTOR_FUNC_BASED_BINOP(less_equal, <=);
VECTOR_FUNC_BASED_BINOP(greater_equal, >=);

template <typename T, int width>
vector<T, width> merge(vector<T, width> true_val, vector<T, width> false_val,
                       mask<width> msk) {
  return false_val.merge(true_val, msk);
}

template <typename T, int width>
vector<T, width> merge(T scl_true_val, vector<T, width> false_val,
                       mask<width> msk) {
  vector<T, width> true_val = scl_true_val;
  return merge(true_val, false_val, msk);
}

template <typename T, int width>
vector<T, width> merge(vector<T, width> true_val, T scl_false_val,
                       mask<width> msk) {
  vector<T, width> false_val = scl_false_val;
  return merge(true_val, false_val, msk);
}

template <typename T, int width>
vector<T, width> merge(T scl_true_val, T scl_false_val, mask<width> msk) {
  vector<T, width> true_val = scl_true_val;
  vector<T, width> false_val = scl_false_val;
  return merge(true_val, false_val, msk);
}

// \p width, \p stride and \p offset is in \p T elements.
// TODO: preserve a stack of transformations to make chains of select and
//       format possible.
template <typename T, int width, int stride, typename OrigT, int orig_width>
struct vector_slice {
  static_assert(stride > 0, "stride must be a positive value");
  using offset_type = typename vector<OrigT, orig_width>::offset_type;

private:
  using orig_vector_impl = detail::vector_impl<OrigT, orig_width>;
  // Reference removes default constructor and assignment, but considering
  // that vector_slice is reference-like type, it should have those two.
  orig_vector_impl &orig_vec;
  offset_type offset = 0;
  // Whether whole vector is selected.
  static constexpr bool whole_vector =
      width * sizeof(T) == orig_width * sizeof(OrigT) && stride == 1;

  template <typename DT, int d_width> friend class vector;

public:
  vector_slice(orig_vector_impl &orig_vec_in, offset_type offset_in = 0)
      : orig_vec{orig_vec_in}, offset{offset_in} {}

  vector_slice(vector<OrigT, orig_width> &orig_vec_in,
               offset_type offset_in = 0)
      : orig_vec{orig_vec_in.impl}, offset{offset_in} {}

  operator vector<T, width>() const {
    auto typed_orig = detail::bit_cast<T>(orig_vec);
    if constexpr (whole_vector) {
      // When the whole vector is selected, offset can only be zero.
      // Otherwise access is out of bound.
      // assert(offset == 0)
      return typed_orig;
    } else
      return detail::read_region</* vwidth */ 1, /* vstride */ 0, width,
                                 stride>(typed_orig, offset);
  }

  operator T() const {
    // FIXME: disable this overload for all width except 1
    static_assert(width == 1, "only single element stride can assign scalar");
    auto typed_orig = detail::bit_cast<T>(orig_vec);
    return typed_orig[offset];
  }

  vector_slice operator=(vector<T, width> rhs) {
    auto typed_orig = detail::bit_cast<T>(orig_vec);
#if __clang_major__ > 9
    detail::write_region</* vstride */ 0, width, stride>(typed_orig, rhs.impl,
                                                         offset);
#else  // __clang_major__ > 9
    // clang-9 has some issues with type deduction, it needs help.
    detail::write_region</* vstride */ 0, width, stride, T,
                         detail::width_getter<decltype(typed_orig)>::value>(
        typed_orig, rhs.impl, offset);
#endif // __clang_major__ > 9
    auto back_typed_orig = detail::bit_cast<OrigT>(typed_orig);
    orig_vec = back_typed_orig;
    return *this;
  }

  vector_slice operator=(T rhs) {
    // FIXME: disable this overload for all width except 1
    static_assert(width == 1, "only single element stride can assign scalar");
    auto typed_orig = detail::bit_cast<T>(orig_vec);
    typed_orig[offset] = rhs;
    auto back_typed_orig = detail::bit_cast<OrigT>(typed_orig);
    orig_vec = back_typed_orig;
    return *this;
  }

  // FIXME: this overload is here to disambiguate slice<T, 1,...> assignment,
  //        as both conversion to T and to vector<T, 1> is applicable.
  // FIXME: for now it covers any width.
  template <int d_stride, typename DOrigT, int d_orig_width>
  vector_slice
  operator=(vector_slice<T, width, d_stride, DOrigT, d_orig_width> rhs) {
    vector<T, width> tmp{rhs};
    *this = tmp;
    return *this;
  }

  // Even though we have a template version of opertor=, standard assignment
  // will win the overload. And it's implicitly deleted.
  vector_slice operator=(vector_slice rhs) {
    return operator=<stride, OrigT, orig_width>(rhs);
  }

  template <int new_sub_width, int new_stride>
  vector_slice<T, new_sub_width, stride * new_stride, OrigT, orig_width>
  select(offset_type new_offset) {
    offset_type off = offset + new_offset * stride;
    return {orig_vec, off};
  }

  vector_slice merge(vector<T, width> vec, mask<width> msk) {
    vector<T, width> tmp = *this;
    tmp.merge(vec, msk);
    *this = tmp;
    return *this;
  }
};

} // namespace cm

#endif // CM_CL_VECTOR_H
