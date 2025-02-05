/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/math.h>
#include <cm-cl/vector.h>

using namespace cm;


CM_NODEBUG CM_INLINE int __spirv_Unordered(float src0, float src1) {
  return math::is_unordered(src0, src1);
}
CM_NODEBUG CM_INLINE int __spirv_Unordered(double src0, double src1) {
  return math::is_unordered(src0, src1);
}
CM_NODEBUG CM_INLINE int __spirv_Ordered(float src0, float src1) {
  return math::is_ordered(src0, src1);
}
CM_NODEBUG CM_INLINE int __spirv_Ordered(double src0, double src1) {
  return math::is_ordered(src0, src1);
}

#define SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(FUNC_NAME, TYPE,            \
                                                   CUSTOM_NAME)                \
  CM_NODEBUG CM_INLINE TYPE __spirv_ocl_##FUNC_NAME(TYPE x) {                  \
    return CUSTOM_NAME(x);                                                     \
  }

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(ceil, float, cm::math::ceil)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(floor, float, cm::math::floor)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(trunc, float, cm::math::truncate)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(roundne, float, cm::math::roundne)

#define SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(FUNC_NAME, ELEMENT_TYPE, N, \
                                                   CUSTOM_NAME)                \
  CM_NODEBUG CM_INLINE cl_vector<ELEMENT_TYPE, N> __spirv_ocl_##FUNC_NAME(     \
      cl_vector<ELEMENT_TYPE, N> x) {                                          \
    vector<ELEMENT_TYPE, N> x_vec = x;                                         \
    return static_cast<vector<ELEMENT_TYPE, N>>(CUSTOM_NAME(x_vec))            \
        .cl_vector();                                                          \
  }

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ceil, float, 2, cm::math::ceil)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ceil, float, 3, cm::math::ceil)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ceil, float, 4, cm::math::ceil)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ceil, float, 8, cm::math::ceil)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ceil, float, 16, cm::math::ceil)

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(floor, float, 2, cm::math::floor)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(floor, float, 3, cm::math::floor)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(floor, float, 4, cm::math::floor)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(floor, float, 8, cm::math::floor)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(floor, float, 16, cm::math::floor)

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(trunc, float, 2, cm::math::truncate)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(trunc, float, 3, cm::math::truncate)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(trunc, float, 4, cm::math::truncate)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(trunc, float, 8, cm::math::truncate)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(trunc, float, 16, cm::math::truncate)

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(roundne, float, 2, cm::math::roundne)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(roundne, float, 3, cm::math::roundne)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(roundne, float, 4, cm::math::roundne)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(roundne, float, 8, cm::math::roundne)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(roundne, float, 16, cm::math::roundne)

#define SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(FUNC_NAME, ELEMENT_TYPE, N, \
                                                   CUSTOM_NAME)                \
  CM_NODEBUG CM_INLINE cl_vector<ELEMENT_TYPE, N> __spirv_ocl_##FUNC_NAME(     \
      cl_vector<ELEMENT_TYPE, N> x1, cl_vector<ELEMENT_TYPE, N> x2) {          \
    vector<ELEMENT_TYPE, N> x_vec1 = x1;                                       \
    vector<ELEMENT_TYPE, N> x_vec2 = x2;                                       \
    return static_cast<vector<ELEMENT_TYPE, N>>(CUSTOM_NAME(x_vec1, x_vec2))   \
        .cl_vector();                                                          \
  }
