/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef CM_CL_DETAIL_BUILTINS_H
#define CM_CL_DETAIL_BUILTINS_H

#include "vector_impl.h"
#include <cm-cl/define.h>

#include <opencl_def.h>
#include <opencl_utility.h>

namespace cm {
namespace detail {

//=========================== builtin declarations ===========================//

template <typename T, int width>
vector_impl<T, width> __cm_cl_select(vector_impl<char, width> cond,
                                     vector_impl<T, width> true_val,
                                     vector_impl<T, width> false_val);

template <int dst_width, typename T, int src_width>
vector_impl<T, dst_width>
__cm_cl_rdregion_int(vector_impl<T, src_width> src, int vstride, int width,
                     int stride, vector_offset_type offset);

template <int dst_width, typename T, int src_width>
vector_impl<T, dst_width>
__cm_cl_rdregion_float(vector_impl<T, src_width> src, int vstride, int width,
                       int stride, vector_offset_type offset);

template <int dst_width, typename T, int src_width>
vector_impl<T, dst_width>
__cm_cl_wrregion_int(vector_impl<T, dst_width> dst,
                     vector_impl<T, src_width> src, int vstride, int width,
                     int stride, vector_offset_type offset);

template <int dst_width, typename T, int src_width>
vector_impl<T, dst_width>
__cm_cl_wrregion_float(vector_impl<T, dst_width> dst,
                       vector_impl<T, src_width> src, int vstride, int width,
                       int stride, vector_offset_type offset);
// FIXME: For legacy issues 64-bit pointer is always returned.
uint64_t __cm_cl_printf_buffer();

int __cm_cl_printf_format_index(__constant const char *str);
// DPC++ tend to place constant strings in global address space.
int __cm_cl_printf_format_index(__global const char *str);
// FIXME: Need this overload as a workaround for some frontends that didn't
//        switch to using addrspaces.
int __cm_cl_printf_format_index(__private const char *str);

template <int width> bool __cm_cl_all(vector_impl<char, width> src);
template <int width> bool __cm_cl_any(vector_impl<char, width> src);

uint32_t __cm_cl_lzd(uint32_t src);
template <int width>
vector_impl<uint32_t, width> __cm_cl_lzd(vector_impl<uint32_t, width> src);

uint32_t __cm_cl_addc(uint32_t *sum, uint32_t src0, uint32_t src1);
template <int width>
vector_impl<uint32_t, width> __cm_cl_addc(vector_impl<uint32_t, width> *sum,
                                          vector_impl<uint32_t, width> src0,
                                          vector_impl<uint32_t, width> src1);

template <typename T> uint32_t __cm_cl_cbit(T src);
template <typename T, int width>
vector_impl<uint32_t, width> __cm_cl_cbit(vector_impl<T, width> src);

template <typename T> T __cm_cl_fma(T src0, T src1, T src2);

uint32_t __cm_cl_bfrev(uint32_t src);
template <int width>
vector_impl<uint32_t, width> __cm_cl_bfrev(vector_impl<uint32_t, width> src);

template <typename T> T __cm_cl_abs_int(T src);
template <typename T> T __cm_cl_abs_float(T src);

template <typename T> T __cm_cl_ceil(T src);
template <typename T> T __cm_cl_floor(T src);
template <typename T> T __cm_cl_trunc(T src);
template <typename T> T __cm_cl_roundne(T src);

template <typename T> T __cm_cl_minnum(T src0, T src1);
template <typename T> T __cm_cl_maxnum(T src0, T src1);

template <typename T> T __cm_cl_sqrt(T src, bool use_fast);
template <typename T> T __cm_cl_log2(T src, bool use_fast);
template <typename T> T __cm_cl_exp2(T src, bool use_fast);
template <typename T> T __cm_cl_powr(T src0, T src1, bool use_fast);
template <typename T> T __cm_cl_sin(T src, bool use_fast);
template <typename T> T __cm_cl_cos(T src, bool use_fast);

template <typename T> T __cm_cl_rsqrt(T src);

vector_impl<uint32_t, 3> __cm_cl_local_id();
vector_impl<uint32_t, 3> __cm_cl_local_size();
vector_impl<uint32_t, 3> __cm_cl_group_count();
uint32_t __cm_cl_group_id_x();
uint32_t __cm_cl_group_id_y();
uint32_t __cm_cl_group_id_z();

void __cm_cl_barrier();
void __cm_cl_sbarrier(uint8_t);
void __cm_cl_fence(uint8_t);

template <typename T>
T __cm_cl_atomicrmw(__global T *ptr, atomic::operation operation, T operand,
                    memory_order semantics, memory_scope scope);

template <typename T>
T __cm_cl_atomicrmw(__local T *ptr, atomic::operation operation, T operand,
                    memory_order semantics, memory_scope scope);

template <typename T>
T __cm_cl_atomicrmw(__generic T *ptr, atomic::operation operation, T operand,
                    memory_order semantics, memory_scope scope);

template <typename T>
T __cm_cl_cmpxchg(__global T *ptr, T operand0, T operand1,
                  memory_order semantics_on_success,
                  memory_order semantics_on_failure, memory_scope scope);

template <typename T>
T __cm_cl_cmpxchg(__local T *ptr, T operand0, T operand1,
                  memory_order semantics_on_success,
                  memory_order semantics_on_failure, memory_scope scope);

template <typename T>
T __cm_cl_cmpxchg(__generic T *ptr, T operand0, T operand1,
                  memory_order semantics_on_success,
                  memory_order semantics_on_failure, memory_scope scope);

//========================= soft implementation part =========================//
//
// This implementations are enabled via CM_CL_SOFT_BUILTINS option.
// The idea is to implement all builtins using only vanilla clang means, without
// requirment of cm-cl builtins support.

template <typename T, int width>
vector_impl<T, width> select_impl(vector_impl<char, width> cond,
                                  vector_impl<T, width> true_val,
                                  vector_impl<T, width> false_val) {
  vector_impl<T, width> res;
  for (int i = 0; i != width; ++i)
    res[i] = cond[i] ? true_val[i] : false_val[i];
  return res;
}

//============================= helper functions =============================//

constexpr bool is_legal_region(int vwidth, int vstride, int width, int stride) {
  return vwidth > 0 && width > 0 && vstride >= 0 && stride >= 0;
}

constexpr int encode_num_blocks(int num_blocks) {
  switch (num_blocks) {
  case 1:
    return 0;
  case 2:
    return 1;
  case 4:
    return 2;
  case 8:
    return 3;
  default:
    return -1;
  }
}

//============================= builtin wrappers =============================//

template <typename T, int width>
vector_impl<T, width> select(vector_impl<char, width> cond,
                             vector_impl<T, width> true_val,
                             vector_impl<T, width> false_val) {
#ifdef CM_CL_SOFT_BUILTINS
  return select_impl(cond, true_val, false_val);
#else  // CM_CL_SOFT_BUILTINS
  return __cm_cl_select(cond, true_val, false_val);
#endif // CM_CL_SOFT_BUILTINS
}

// Unlike __cm_cl_rdregion \p offset here is in T elements, not bytes.
template <int vwidth, int vstride, int width, int stride, typename T,
          int src_width>
vector_impl<T, vwidth * width> read_region(vector_impl<T, src_width> src,
                                           vector_offset_type offset) {
  static_assert(is_legal_region(vwidth, vstride, width, stride),
                "provided region is illegal");
  if constexpr (width == 1 && vwidth == 1)
    return src[offset];
  else {
    if constexpr (cl::is_floating_point<T>::value)
      return __cm_cl_rdregion_float<vwidth * width>(src, vstride, width, stride,
                                                    offset * sizeof(T));
    else
      return __cm_cl_rdregion_int<vwidth * width>(src, vstride, width, stride,
                                                  offset * sizeof(T));
  }
}

// Unlike __cm_cl_wrregion \p offset here is in T elements, not bytes.
template <int vstride, int width, int stride, typename T, int dst_width,
          int src_width>
void write_region(vector_impl<T, dst_width> &dst, vector_impl<T, src_width> src,
                  vector_offset_type offset) {
  static_assert(is_legal_region((src_width / width), vstride, width, stride) &&
                    src_width % width == 0,
                "provided region is illegal");
  if constexpr (src_width == 1 && width == 1)
    dst[offset] = src[0];
  else {
    if constexpr (cl::is_floating_point<T>::value)
      dst = __cm_cl_wrregion_float(dst, src, vstride, width, stride,
                                   offset * sizeof(T));
    else
      dst = __cm_cl_wrregion_int(dst, src, vstride, width, stride,
                                 offset * sizeof(T));
  }
}

inline __global void *printf_buffer() {
  // FIXME: for legacy issues 64-bit pointer is always returned.
  auto ptr = static_cast<uintptr_t>(__cm_cl_printf_buffer());
  return reinterpret_cast<__global void *>(ptr);
}

inline int printf_format_index(__constant const char *str) {
  return __cm_cl_printf_format_index(str);
}

inline int printf_format_index(__global const char *str) {
  return __cm_cl_printf_format_index(str);
}

inline int printf_format_index(__private const char *str) {
  return __cm_cl_printf_format_index(str);
}

inline uint32_t lzd(uint32_t src) { return __cm_cl_lzd(src); }

template <int width>
vector_impl<uint32_t, width> lzd(vector_impl<uint32_t, width> src) {
  return __cm_cl_lzd(src);
}

// Sum is the first output, carry - the second.
inline cl::pair<uint32_t, char> addc(uint32_t src0, uint32_t src1) {
  uint32_t res;
  uint32_t carry = __cm_cl_addc(&res, src0, src1);
  return {res, carry};
}

template <int width>
cl::pair<vector_impl<uint32_t, width>, vector_impl<char, width>>
addc(vector_impl<uint32_t, width> src0, vector_impl<uint32_t, width> src1) {
  vector_impl<uint32_t, width> res;
  vector_impl<uint32_t, width> carry = __cm_cl_addc(&res, src0, src1);
  return {res, __builtin_convertvector(carry, vector_impl<char, width>)};
}

inline vector_impl<uint32_t, 3> get_local_id() { return __cm_cl_local_id(); }

inline vector_impl<uint32_t, 3> get_local_size() {
  return __cm_cl_local_size();
}

inline vector_impl<uint32_t, 3> get_group_count() {
  return __cm_cl_group_count();
}

inline uint32_t get_group_id_x() { return __cm_cl_group_id_x(); }

inline uint32_t get_group_id_y() { return __cm_cl_group_id_y(); }

inline uint32_t get_group_id_z() { return __cm_cl_group_id_z(); }

template <typename T, int width>
vector_impl<uint32_t, width> cbit(vector_impl<T, width> src) {
  static_assert(cl::is_integral<T>::value && !cl::is_bool<T>::value &&
                    sizeof(T) <= sizeof(uint32_t),
                "illegal type provided in cbit");
  return __cm_cl_cbit(src);
}

template <typename T> uint32_t cbit(T src) {
  static_assert(cl::is_integral<T>::value && !cl::is_bool<T>::value &&
                    sizeof(T) <= sizeof(uint32_t),
                "illegal type provided in cbit");
  return __cm_cl_cbit(src);
}

template <typename T> T fma(T src0, T src1, T src2) {
  static_assert(cl::is_floating_point<T>::value,
                "illegal type provided in fma");
  return __cm_cl_fma(src0, src1, src2);
}

inline uint32_t bfrev(uint32_t src) { return __cm_cl_bfrev(src); }

template <int width>
vector_impl<uint32_t, width> bfrev(vector_impl<uint32_t, width> src) {
  return __cm_cl_bfrev(src);
}

// Usigned values are returned without a change.
template <typename T> T absolute(T src) {
  if constexpr (cl::is_floating_point<T>::value)
    return __cm_cl_abs_float(src);
  else
    static_assert(cl::is_integral<T>::value && !cl::is_bool<T>::value,
                  "Absolute function expects integer or floating point type.");

  if constexpr (cl::is_signed<T>::value)
    return __cm_cl_abs_int(src);
  return src;
}

template <typename T> T ceil(T src) {
  static_assert(cl::is_floating_point<T>::value,
                "Ceil function expects floating poing type.");
  return __cm_cl_ceil(src);
}

template <typename T> T floor(T src) {
  static_assert(cl::is_floating_point<T>::value,
                "Floor function expects floating poing type.");
  return __cm_cl_floor(src);
}

template <typename T> T trunc(T src) {
  static_assert(cl::is_floating_point<T>::value,
                "Trunc function expects floating poing type.");
  return __cm_cl_trunc(src);
}

template <typename T> T roundne(T src) {
  static_assert(cl::is_floating_point<T>::value,
    "Roundne function expects floating poing type.");
  return __cm_cl_roundne(src);
}

template <typename T> T min_float(T src0, T src1) {
  static_assert(cl::is_floating_point<T>::value,
                "illegal type provided in min_float");
  return __cm_cl_minnum(src0, src1);
}

template <typename T> T max_float(T src0, T src1) {
  static_assert(cl::is_floating_point<T>::value,
                "illegal type provided in max_float");
  return __cm_cl_maxnum(src0, src1);
}

template <bool use_fast, typename T> T sqrt(T src) {
  static_assert(cl::is_floating_point<T>::value,
                "illegal type provided in sqrt");
  return __cm_cl_sqrt(src, use_fast);
}
template <bool use_fast, typename T> T log2(T src) {
  static_assert(cl::is_floating_point<T>::value,
                "illegal type provided in log2");
  return __cm_cl_log2(src, use_fast);
}
template <bool use_fast, typename T> T exp2(T src) {
  static_assert(cl::is_floating_point<T>::value,
                "illegal type provided in exp2");
  return __cm_cl_exp2(src, use_fast);
}
template <bool use_fast, typename T> T powr(T src0, T src1) {
  static_assert(cl::is_floating_point<T>::value,
                "illegal type provided in powr");
  return __cm_cl_powr(src0, src1, use_fast);
}
template <bool use_fast, typename T> T sin(T src) {
  static_assert(cl::is_floating_point<T>::value,
                "illegal type provided in sin");
  return __cm_cl_sin(src, use_fast);
}
template <bool use_fast, typename T> T cos(T src) {
  static_assert(cl::is_floating_point<T>::value,
                "illegal type provided in cos");
  return __cm_cl_cos(src, use_fast);
}

template <atomic::operation operation, memory_order semantics,
          memory_scope scope, typename T>
T atomicrmw(__global T *ptr, T operand) {
  static_assert(cl::is_arithmetic<T>::value,
                "illegal type provided in atomicrmw");
  return __cm_cl_atomicrmw(ptr, operation, operand, semantics, scope);
}

template <atomic::operation operation, memory_order semantics,
          memory_scope scope, typename T>
T atomicrmw(__local T *ptr, T operand) {
  static_assert(cl::is_arithmetic<T>::value,
                "illegal type provided in atomicrmw");
  return __cm_cl_atomicrmw(ptr, operation, operand, semantics, scope);
}

template <atomic::operation operation, memory_order semantics,
          memory_scope scope, typename T>
T atomicrmw(__generic T *ptr, T operand) {
  static_assert(cl::is_arithmetic<T>::value,
                "illegal type provided in atomicrmw");
  return __cm_cl_atomicrmw(ptr, operation, operand, semantics, scope);
}

template <memory_order semantics_on_success, memory_order semantics_on_failure,
          memory_scope scope, typename T>
T cmpxchg(__global T *ptr, T operand0, T operand1) {
  static_assert(cl::is_integral<T>::value, "illegal type provided in cmpxchg");
  return __cm_cl_cmpxchg(ptr, operand0, operand1, semantics_on_success,
                         semantics_on_failure, scope);
}

template <memory_order semantics_on_success, memory_order semantics_on_failure,
          memory_scope scope, typename T>
T cmpxchg(__local T *ptr, T operand0, T operand1) {
  static_assert(cl::is_integral<T>::value, "illegal type provided in cmpxchg");
  return __cm_cl_cmpxchg(ptr, operand0, operand1, semantics_on_success,
                         semantics_on_failure, scope);
}

template <memory_order semantics_on_success, memory_order semantics_on_failure,
          memory_scope scope, typename T>
T cmpxchg(__generic T *ptr, T operand0, T operand1) {
  static_assert(cl::is_integral<T>::value, "illegal type provided in cmpxchg");
  return __cm_cl_cmpxchg(ptr, operand0, operand1, semantics_on_success,
                         semantics_on_failure, scope);
}

} // namespace detail
} // namespace cm

#endif // CM_CL_DETAIL_BUILTINS_H
