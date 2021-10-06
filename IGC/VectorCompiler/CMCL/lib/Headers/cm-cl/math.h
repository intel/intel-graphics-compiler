/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef CM_CL_MATH_H
#define CM_CL_MATH_H

#include "detail/builtins.h"
#include "vector.h"

#include <opencl_def.h>
#include <opencl_utility.h>

namespace cm {
namespace math {

/*================ Count leading zeros =======================*/

inline uint32_t count_leading_zeros(uint32_t src) { return detail::lzd(src); }

template <int width>
vector<uint32_t, width> count_leading_zeros(vector<uint32_t, width> src) {
  return detail::lzd(src.cl_vector());
}

// FIXME: replace char with bool.
inline cl::pair<uint32_t, char> add_with_carry(uint32_t src0, uint32_t src1) {
  return detail::addc(src0, src1);
}

/*====================== Add with carry ======================*/

template <int width>
cl::pair<vector<uint32_t, width>, vector<char, width>>
add_with_carry(vector<uint32_t, width> src0, vector<uint32_t, width> src1) {
#if __clang_major__ > 9
  return detail::addc(src0.cl_vector(), src1.cl_vector());
#else  // __clang_major__ > 9
  // clang-9 has some issues with type deduction, it needs help.
  auto res = detail::addc(src0.cl_vector(), src1.cl_vector());
  return {res.first, res.second};
#endif // __clang_major__ > 9
}

/*======================= Is ordered ========================*/

inline bool is_ordered(float src0, float src1) {
  return (src0 == src0) && (src1 == src1);
}

inline bool is_ordered(double src0, double src1) {
  return (src0 == src0) && (src1 == src1);
}

/*====================== Is unordered =======================*/

inline bool is_unordered(float src0, float src1) {
  return (src0 != src0) || (src1 != src1);
}

inline bool is_unordered(double src0, double src1) {
  return (src0 != src0) || (src1 != src1);
}

/*====================== Reverse bit =======================*/

inline uint32_t reverse_bits(uint32_t src) { return detail::bfrev(src); }

template <int width>
vector<uint32_t, width> reverse_bits(vector<uint32_t, width> src) {
  return detail::bfrev(src.cl_vector());
}

/*================== Count trailing zeros ==================*/

inline uint32_t count_trailing_zeros(uint32_t src) {
  uint32_t src_reverse = reverse_bits(src);
  return count_leading_zeros(src_reverse);
}

inline uint32_t count_trailing_zeros(int32_t src) {
  return count_trailing_zeros(static_cast<uint32_t>(src));
}

inline uint32_t count_trailing_zeros(uint64_t src) {
  vector<uint64_t, 1> src_vec = src;
  vector<uint32_t, 2> src_vec_32 = src_vec.format<uint32_t>();
  uint32_t src_vec_lo_ctz =
      count_trailing_zeros(static_cast<uint32_t>(src_vec_32[0]));
  uint32_t src_vec_hi_ctz =
      count_trailing_zeros(static_cast<uint32_t>(src_vec_32[1]));
  if (src_vec_32[0] == 0)
    src_vec_lo_ctz += src_vec_hi_ctz;
  return src_vec_lo_ctz;
}

inline uint32_t count_trailing_zeros(int64_t src) {
  return count_trailing_zeros(static_cast<uint64_t>(src));
}

template <typename T, cl::enable_if_t<(sizeof(T) < sizeof(uint32_t)), int> = 0>
inline uint32_t count_trailing_zeros(T src) {
  static_assert(cl::is_integral<T>::value && !cl::is_bool<T>::value,
                "Count trailing zeros expects integer and not bool type");
  uint32_t src_32 = static_cast<uint32_t>(src) | (1U << (sizeof(T) * 8));
  return count_trailing_zeros(src_32);
}

template <int width>
vector<uint32_t, width> count_trailing_zeros(vector<uint32_t, width> src) {
  vector<uint32_t, width> src_reverse = reverse_bits(src);
  return count_leading_zeros(src_reverse);
}

template <int width>
vector<uint32_t, width> count_trailing_zeros(vector<int32_t, width> src) {
  return count_trailing_zeros(static_cast<vector<uint32_t, width>>(src));
}

template <int width>
vector<uint32_t, width> count_trailing_zeros(vector<uint64_t, width> src) {
  vector<uint32_t, (width * 2)> src_vec_32 = src.template format<uint32_t>();
  vector<uint32_t, width> src_vec_32_lo =
      src_vec_32.template select<width, 2>(0);
  vector<uint32_t, width> src_vec_32_hi =
      src_vec_32.template select<width, 2>(1);
  vector<uint32_t, width> src_vec_32_lo_ctz =
      count_trailing_zeros(src_vec_32_lo);
  vector<uint32_t, width> src_vec_32_hi_ctz =
      count_trailing_zeros(src_vec_32_hi);
  vector<uint32_t, width> res = src_vec_32_lo_ctz;
  vector<uint32_t, width> res_with_hi = src_vec_32_lo_ctz + src_vec_32_hi_ctz;
  res.merge(res_with_hi, src_vec_32_lo == vector<uint32_t, width>{0});
  return res;
}

template <int width>
vector<uint32_t, width> count_trailing_zeros(vector<int64_t, width> src) {
  return count_trailing_zeros(static_cast<vector<uint64_t, width>>(src));
}

template <typename T, int width,
          cl::enable_if_t<(sizeof(T) < sizeof(uint32_t)), int> = 0>
vector<uint32_t, width> count_trailing_zeros(vector<T, width> src) {
  static_assert(cl::is_integral<T>::value && !cl::is_bool<T>::value,
                "Count trailing zeros expects integer and not bool type");
  vector<uint32_t, width> src_32 =
      static_cast<vector<uint32_t, width>>(src) | (1U << (sizeof(T) * 8));
  return count_trailing_zeros(src_32);
}

/*=================== Count population =====================*/

template <typename T, int width>
vector<uint32_t, width> count_population(vector<T, width> src) {
  return detail::cbit(src.cl_vector());
};

template <int width>
vector<uint32_t, width> count_population(vector<uint64_t, width> src) {
  vector<uint32_t, (width * 2)> src_vec_32 = src.template format<uint32_t>();
  vector<uint32_t, width> src_vec_32_lo =
      src_vec_32.template select<width, 2>(0);
  vector<uint32_t, width> src_vec_32_hi =
      src_vec_32.template select<width, 2>(1);
  return count_population(src_vec_32_lo) + count_population(src_vec_32_hi);
}

template <int width>
vector<uint32_t, width> count_population(vector<int64_t, width> src) {
  return count_population(static_cast<vector<uint64_t, width>>(src));
}

// FIXME: add scalar overload functionallity
template <typename T> inline uint32_t count_population(T src) {
  vector<T, 1> __src_vec = src;
  return detail::cbit(__src_vec.cl_vector())[0];
}

inline uint32_t count_population(uint64_t src) {
  vector<uint64_t, 1> src_vec = src;
  vector<uint32_t, 2> src_vec_32 = src_vec.format<uint32_t>();
  return count_population(static_cast<uint32_t>(src_vec_32[0])) +
         count_population(static_cast<uint32_t>(src_vec_32[1]));
}

inline uint32_t count_population(int64_t src) {
  return count_population(static_cast<uint64_t>(src));
}

/*==========================================================*/

} // namespace math
} // namespace cm

#endif // CM_CL_MATH_H
