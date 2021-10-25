/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/atomic.h>
#include <cm-cl/exec.h>
#include <cm-cl/math.h>
#include <cm-cl/vector.h>
#include <opencl_def.h>

using namespace cm;

extern "C" {
#include "spirv_atomics_common.h"
#include "spirv_math.h"
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInGlobalInvocationId(int dim) {
  return cm::exec::get_local_id(dim) +
         static_cast<ulong>(cm::exec::get_group_id(dim)) *
             cm::exec::get_local_size(dim);
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInWorkgroupSize(int dim) {
  return cm::exec::get_local_size(dim);
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInLocalInvocationId(int dim) {
  return cm::exec::get_local_id(dim);
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInWorkgroupId(int dim) {
  return cm::exec::get_group_id(dim);
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInGlobalSize(int dim) {
  return static_cast<ulong>(cm::exec::get_local_size(dim)) *
         cm::exec::get_group_count(dim);
}

CM_NODEBUG CM_INLINE ulong __spirv_BuiltInNumWorkgroups(int dim) {
  return cm::exec::get_group_count(dim);
}

#define IGC_OCL_BUILTIN_NAME_1ARG_SCALAR(FUNC_NAME, POSTFIX_TYPE)              \
  __builtin_spirv_OpenCL_##FUNC_NAME##_##POSTFIX_TYPE

#define SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(FUNC_NAME, TYPE, OCL_POSTFIX)      \
  CM_NODEBUG CM_INLINE TYPE __spirv_ocl_##FUNC_NAME(TYPE x) {                  \
    return IGC_OCL_BUILTIN_NAME_1ARG_SCALAR(FUNC_NAME, OCL_POSTFIX)(x);        \
  }

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(exp, float, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(exp, double, f64);

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(log, float, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(log, double, f64);

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(sin, float, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(sin, double, f64);

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(asin, float, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(asin, double, f64);

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(cos, float, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(cos, double, f64);

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(acos, float, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(acos, double, f64);

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(tan, float, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(tan, double, f64);

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(atan, float, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(atan, double, f64);

// First type is also a return type
#define IGC_OCL_BUILTIN_NAME_2ARG_SCALAR(FUNC_NAME, FIRST_POSTFIX_TYPE,        \
                                         SECOND_POSTFIX_TYPE)                  \
  __builtin_spirv_OpenCL_##FUNC_NAME##_##FIRST_POSTFIX_TYPE##_##SECOND_POSTFIX_TYPE

#define SPIRV_MATH_BUILTIN_DECL_2ARG_SCALAR(                                   \
    FUNC_NAME, FIRST_TYPE, SECOND_TYPE, FIRST_OCL_POSTFIX, SECOND_OCL_POSTFIX) \
  CM_NODEBUG CM_INLINE FIRST_TYPE __spirv_ocl_##FUNC_NAME(FIRST_TYPE x,        \
                                                          SECOND_TYPE y) {     \
    return IGC_OCL_BUILTIN_NAME_2ARG_SCALAR(FUNC_NAME, FIRST_OCL_POSTFIX,      \
                                            SECOND_OCL_POSTFIX)(x, y);         \
  }

#define SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_SCALAR(FUNC_NAME, ARGS_TYPE,       \
                                                   OCL_POSTFIX)                \
  SPIRV_MATH_BUILTIN_DECL_2ARG_SCALAR(FUNC_NAME, ARGS_TYPE, ARGS_TYPE,         \
                                      OCL_POSTFIX, OCL_POSTFIX)

SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_SCALAR(pow, float, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_SCALAR(pow, double, f64);

SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_SCALAR(atan2, float, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_SCALAR(atan2, double, f64);

SPIRV_MATH_BUILTIN_DECL_2ARG_SCALAR(sincos, float, __private float *, f32,
                                    p0f32);
SPIRV_MATH_BUILTIN_DECL_2ARG_SCALAR(sincos, float, __global float *, f32,
                                    p1f32);
SPIRV_MATH_BUILTIN_DECL_2ARG_SCALAR(sincos, float, __local float *, f32, p3f32);
SPIRV_MATH_BUILTIN_DECL_2ARG_SCALAR(sincos, double, __private double *, f64,
                                    p0f64);
SPIRV_MATH_BUILTIN_DECL_2ARG_SCALAR(sincos, double, __global double *, f64,
                                    p1f64);
SPIRV_MATH_BUILTIN_DECL_2ARG_SCALAR(sincos, double, __local double *, f64,
                                    p3f64);

#define IGC_OCL_BUILTIN_NAME_1ARG_VECTOR(FUNC_NAME, N, POSTFIX_TYPE)           \
  __builtin_spirv_OpenCL_##FUNC_NAME##_v##N##POSTFIX_TYPE

#define SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(FUNC_NAME, ELEMENT_TYPE, N,        \
                                            OCL_POSTFIX)                       \
  CM_NODEBUG CM_INLINE cl_vector<ELEMENT_TYPE, N> __spirv_ocl_##FUNC_NAME(     \
      cl_vector<ELEMENT_TYPE, N> x) {                                          \
    return IGC_OCL_BUILTIN_NAME_1ARG_VECTOR(FUNC_NAME, N, OCL_POSTFIX)(x);     \
  }

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp, float, 2, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp, float, 3, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp, float, 4, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp, float, 8, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp, float, 16, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp, double, 2, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp, double, 3, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp, double, 4, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp, double, 8, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp, double, 16, f64);

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log, float, 2, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log, float, 3, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log, float, 4, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log, float, 8, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log, float, 16, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log, double, 2, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log, double, 3, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log, double, 4, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log, double, 8, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log, double, 16, f64);

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(sin, float, 2, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(sin, float, 3, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(sin, float, 4, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(sin, float, 8, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(sin, float, 16, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(sin, double, 2, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(sin, double, 3, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(sin, double, 4, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(sin, double, 8, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(sin, double, 16, f64);

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(asin, float, 2, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(asin, float, 3, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(asin, float, 4, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(asin, float, 8, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(asin, float, 16, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(asin, double, 2, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(asin, double, 3, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(asin, double, 4, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(asin, double, 8, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(asin, double, 16, f64);

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(cos, float, 2, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(cos, float, 3, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(cos, float, 4, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(cos, float, 8, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(cos, float, 16, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(cos, double, 2, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(cos, double, 3, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(cos, double, 4, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(cos, double, 8, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(cos, double, 16, f64);

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(acos, float, 2, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(acos, float, 3, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(acos, float, 4, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(acos, float, 8, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(acos, float, 16, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(acos, double, 2, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(acos, double, 3, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(acos, double, 4, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(acos, double, 8, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(acos, double, 16, f64);

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(tan, float, 2, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(tan, float, 3, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(tan, float, 4, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(tan, float, 8, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(tan, float, 16, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(tan, double, 2, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(tan, double, 3, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(tan, double, 4, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(tan, double, 8, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(tan, double, 16, f64);

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(atan, float, 2, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(atan, float, 3, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(atan, float, 4, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(atan, float, 8, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(atan, float, 16, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(atan, double, 2, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(atan, double, 3, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(atan, double, 4, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(atan, double, 8, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(atan, double, 16, f64);

#define IGC_OCL_BUILTIN_NAME_2ARG(FUNC_NAME, N, FIRST_OCL_POSTFIX,             \
                                  SECOND_OCL_POSTFIX)                          \
  __builtin_spirv_OpenCL_##FUNC_NAME##_##FIRST_OCL_POSTFIX##_##SECOND_OCL_POSTFIX

#define SPIRV_MATH_BUILTIN_DECL_2ARG(FUNC_NAME, FIRST_ELEMENT_TYPE,            \
                                     SECOND_ELEMENT_TYPE, N,                   \
                                     FIRST_OCL_POSTFIX, SECOND_OCL_POSTFIX)    \
  CM_NODEBUG CM_INLINE cl_vector<FIRST_ELEMENT_TYPE, N>                        \
      __spirv_ocl_##FUNC_NAME(cl_vector<FIRST_ELEMENT_TYPE, N> x,              \
                              cl_vector<SECOND_ELEMENT_TYPE, N> y) {           \
    return IGC_OCL_BUILTIN_NAME_2ARG(FUNC_NAME, N, v##N##FIRST_OCL_POSTFIX,    \
                                     v##N##SECOND_OCL_POSTFIX)(x, y);          \
  }

#define SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(FUNC_NAME, ARG_TYPE, N,     \
                                                   OCL_POSTFIX)                \
  SPIRV_MATH_BUILTIN_DECL_2ARG(FUNC_NAME, ARG_TYPE, ARG_TYPE, N, OCL_POSTFIX,  \
                               OCL_POSTFIX)

SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(pow, float, 2, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(pow, float, 3, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(pow, float, 4, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(pow, float, 8, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(pow, float, 16, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(pow, double, 2, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(pow, double, 3, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(pow, double, 4, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(pow, double, 8, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(pow, double, 16, f64);

SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(atan2, float, 2, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(atan2, float, 3, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(atan2, float, 4, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(atan2, float, 8, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(atan2, float, 16, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(atan2, double, 2, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(atan2, double, 3, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(atan2, double, 4, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(atan2, double, 8, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ARGS_VECTOR(atan2, double, 16, f64);

#define SPIRV_MATH_BUILTIN_DECL_2ARG_POINTER_VECTOR(                           \
    FUNC_NAME, FIRST_TYPE, ADDRESS_SPACE, SECOND_TYPE, N, FIRST_OCL_POSTFIX,   \
    ADDRESS_SPACE_PREFIX, SECOND_OCL_POSTFIX)                                  \
  CM_NODEBUG CM_INLINE cl_vector<FIRST_TYPE, N> __spirv_ocl_##FUNC_NAME(       \
      cl_vector<FIRST_TYPE, N> x,                                              \
      ADDRESS_SPACE cl_vector<SECOND_TYPE, N> *y) {                            \
    return IGC_OCL_BUILTIN_NAME_2ARG(                                          \
        FUNC_NAME, N, v##N##FIRST_OCL_POSTFIX,                                 \
        ADDRESS_SPACE_PREFIX##v##N##SECOND_OCL_POSTFIX)(x, y);                 \
  }

#define SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(           \
    FUNC_NAME, ELEMENT_TYPE, N, ADDRESS_SPACE, AS_PREFIX, OCL_POSTFIX)         \
  SPIRV_MATH_BUILTIN_DECL_2ARG_POINTER_VECTOR(                                 \
      FUNC_NAME, ELEMENT_TYPE, ADDRESS_SPACE, ELEMENT_TYPE, N, OCL_POSTFIX,    \
      AS_PREFIX, OCL_POSTFIX)

SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, float, 2,
                                                            __private, p0, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, float, 3,
                                                            __private, p0, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, float, 4,
                                                            __private, p0, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, float, 8,
                                                            __private, p0, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, float, 16,
                                                            __private, p0, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, double, 2,
                                                            __private, p0, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, double, 3,
                                                            __private, p0, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, double, 4,
                                                            __private, p0, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, double, 8,
                                                            __private, p0, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, double, 16,
                                                            __private, p0, f64);

SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, float, 2,
                                                            __global, p1, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, float, 3,
                                                            __global, p1, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, float, 4,
                                                            __global, p1, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, float, 8,
                                                            __global, p1, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, float, 16,
                                                            __global, p1, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, double, 2,
                                                            __global, p1, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, double, 3,
                                                            __global, p1, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, double, 4,
                                                            __global, p1, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, double, 8,
                                                            __global, p1, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, double, 16,
                                                            __global, p1, f64);

SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, float, 2,
                                                            __local, p3, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, float, 3,
                                                            __local, p3, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, float, 4,
                                                            __local, p3, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, float, 8,
                                                            __local, p3, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, float, 16,
                                                            __local, p3, f32);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, double, 2,
                                                            __local, p3, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, double, 3,
                                                            __local, p3, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, double, 4,
                                                            __local, p3, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, double, 8,
                                                            __local, p3, f64);
SPIRV_MATH_BUILTIN_DECL_2_SAME_ELEMENT_TYPES_POINTER_VECTOR(sincos, double, 16,
                                                            __local, p3, f64);

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

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(popcount, char,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(popcount, uchar,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(popcount, short,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(popcount, ushort,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(popcount, int,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(popcount, uint,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(popcount, long,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(popcount, ulong,
                                           cm::math::count_population)

#define SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(FUNC_NAME, ELEMENT_TYPE, N, \
                                                   CUSTOM_NAME)                \
  CM_NODEBUG CM_INLINE cl_vector<ELEMENT_TYPE, N> __spirv_ocl_##FUNC_NAME(     \
      cl_vector<ELEMENT_TYPE, N> x) {                                          \
    vector<ELEMENT_TYPE, N> x_vec = x;                                         \
    return static_cast<vector<ELEMENT_TYPE, N>>(CUSTOM_NAME(x_vec))            \
        .cl_vector();                                                          \
  }

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, char, 2,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, char, 3,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, char, 4,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, char, 8,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, char, 16,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, uchar, 2,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, uchar, 3,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, uchar, 4,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, uchar, 8,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, uchar, 16,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, short, 2,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, short, 3,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, short, 4,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, short, 8,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, short, 16,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, ushort, 2,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, ushort, 3,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, ushort, 4,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, ushort, 8,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, ushort, 16,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, int, 2,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, int, 3,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, int, 4,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, int, 8,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, int, 16,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, uint, 2,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, uint, 3,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, uint, 4,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, uint, 8,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, uint, 16,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, long, 2,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, long, 3,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, long, 4,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, long, 8,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, long, 16,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, ulong, 2,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, ulong, 3,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, ulong, 4,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, ulong, 8,
                                           cm::math::count_population)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(popcount, ulong, 16,
                                           cm::math::count_population)

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(ctz, char,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(ctz, uchar,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(ctz, short,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(ctz, ushort,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(ctz, int,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(ctz, uint,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(ctz, long,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(ctz, ulong,
                                           cm::math::count_trailing_zeros)

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, char, 2,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, char, 3,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, char, 4,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, char, 8,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, char, 16,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, uchar, 2,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, uchar, 3,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, uchar, 4,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, uchar, 8,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, uchar, 16,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, short, 2,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, short, 3,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, short, 4,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, short, 8,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, short, 16,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, ushort, 2,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, ushort, 3,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, ushort, 4,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, ushort, 8,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, ushort, 16,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, int, 2,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, int, 3,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, int, 4,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, int, 8,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, int, 16,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, uint, 2,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, uint, 3,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, uint, 4,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, uint, 8,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, uint, 16,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, long, 2,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, long, 3,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, long, 4,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, long, 8,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, long, 16,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, ulong, 2,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, ulong, 3,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, ulong, 4,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, ulong, 8,
                                           cm::math::count_trailing_zeros)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(ctz, ulong, 16,
                                           cm::math::count_trailing_zeros)

namespace {

template <typename PtrT, atomic::operation op, memory_order OCLSemantics,
          memory_scope OCLScope, typename... OpT>
constexpr auto invokeConcreteAtomic(PtrT *ptr, OpT... operands) {
  return atomic::execute<op, OCLSemantics, OCLScope, PtrT>(ptr, operands...);
}

template <typename PtrT, atomic::operation operation, memory_scope OCLScope,
          typename... OpT>
static CM_NODEBUG CM_INLINE auto
spirvAtomicHelperWithKnownScope(PtrT *ptr, int Semantics, OpT... operands) {
  switch (Semantics) {
  default:
  case SequentiallyConsistent:
    return invokeConcreteAtomic<PtrT, operation, memory_order_seq_cst,
                                OCLScope>(ptr, operands...);
  case Relaxed:
    return invokeConcreteAtomic<PtrT, operation, memory_order_relaxed,
                                OCLScope>(ptr, operands...);
  case Acquire:
    return invokeConcreteAtomic<PtrT, operation, memory_order_acquire,
                                OCLScope>(ptr, operands...);
  case Release:
    return invokeConcreteAtomic<PtrT, operation, memory_order_release,
                                OCLScope>(ptr, operands...);
  case AcquireRelease:
    return invokeConcreteAtomic<PtrT, operation, memory_order_acq_rel,
                                OCLScope>(ptr, operands...);
  }
}

// Iterate through all possible values of non-constant semantics and scope.
// Use the strict possible values if unknown: sequential-consistency semantics
// and cross-device scope.
template <atomic::operation operation, typename PtrT, typename... OpT>
static CM_NODEBUG CM_INLINE auto spirvAtomicHelper(PtrT *ptr, int Semantics,
                                                   int Scope, OpT... operands) {
  switch (Scope) {
  default:
  case CrossDevice:
    return spirvAtomicHelperWithKnownScope<PtrT, operation,
                                           memory_scope_all_devices>(
        ptr, Semantics, operands...);
  case Device:
    return spirvAtomicHelperWithKnownScope<PtrT, operation,
                                           memory_scope_device>(ptr, Semantics,
                                                                operands...);
  case Workgroup:
    return spirvAtomicHelperWithKnownScope<PtrT, operation,
                                           memory_scope_work_group>(
        ptr, Semantics, operands...);
  case Subgroup:
    return spirvAtomicHelperWithKnownScope<PtrT, operation,
                                           memory_scope_sub_group>(
        ptr, Semantics, operands...);
  case Invocation:
    return spirvAtomicHelperWithKnownScope<PtrT, operation,
                                           memory_scope_work_item>(
        ptr, Semantics, operands...);
  }
}

} // namespace

#define SPIRV_ATOMIC_BUILTIN_LOAD(ADDRESS_SPACE, TYPE)                         \
  CM_NODEBUG CM_INLINE TYPE __spirv_AtomicLoad(ADDRESS_SPACE TYPE *ptr,        \
                                               int Scope, int Semantics) {     \
    return spirvAtomicHelper<atomic::operation::load, ADDRESS_SPACE TYPE>(     \
        ptr, Semantics, Scope);                                                \
  }

// FIXME: strict aliasing violation.
#define SPIRV_ATOMIC_BUILTIN_FLOATS(ADDRESS_SPACE)                             \
  CM_NODEBUG CM_INLINE float __spirv_AtomicLoad(ADDRESS_SPACE float *ptr,      \
                                                int Scope, int Semantics) {    \
    ADDRESS_SPACE int *int_ptr = reinterpret_cast<ADDRESS_SPACE int *>(ptr);   \
    return as_float(__spirv_AtomicLoad(int_ptr, Scope, Semantics));            \
  }                                                                            \
  CM_NODEBUG CM_INLINE double __spirv_AtomicLoad(ADDRESS_SPACE double *ptr,    \
                                                 int Scope, int Semantics) {   \
    ADDRESS_SPACE long *long_ptr =                                             \
        reinterpret_cast<ADDRESS_SPACE long *>(ptr);                           \
    return as_double(__spirv_AtomicLoad(long_ptr, Scope, Semantics));          \
  }

SPIRV_ATOMIC_BUILTIN_LOAD(__global, int)
SPIRV_ATOMIC_BUILTIN_LOAD(__global, long)
SPIRV_ATOMIC_BUILTIN_FLOATS(__global)
SPIRV_ATOMIC_BUILTIN_LOAD(__local, int)
SPIRV_ATOMIC_BUILTIN_LOAD(__local, long)
SPIRV_ATOMIC_BUILTIN_FLOATS(__local)
SPIRV_ATOMIC_BUILTIN_LOAD(__generic, int)
SPIRV_ATOMIC_BUILTIN_LOAD(__generic, long)
SPIRV_ATOMIC_BUILTIN_FLOATS(__generic)

#define SPIRV_ATOMIC_BUILTIN_STORE(ADDRESS_SPACE, TYPE)                        \
  CM_NODEBUG CM_INLINE void __spirv_AtomicStore(                               \
      ADDRESS_SPACE TYPE *ptr, int Scope, int Semantics, TYPE Value) {         \
    spirvAtomicHelper<atomic::operation::store, ADDRESS_SPACE TYPE>(           \
        ptr, Semantics, Scope, Value);                                         \
  }

SPIRV_ATOMIC_BUILTIN_STORE(__global, int)
SPIRV_ATOMIC_BUILTIN_STORE(__global, long)
SPIRV_ATOMIC_BUILTIN_STORE(__global, float)
SPIRV_ATOMIC_BUILTIN_STORE(__global, double)
SPIRV_ATOMIC_BUILTIN_STORE(__local, int)
SPIRV_ATOMIC_BUILTIN_STORE(__local, long)
SPIRV_ATOMIC_BUILTIN_STORE(__local, float)
SPIRV_ATOMIC_BUILTIN_STORE(__local, double)
SPIRV_ATOMIC_BUILTIN_STORE(__generic, int)
SPIRV_ATOMIC_BUILTIN_STORE(__generic, long)
SPIRV_ATOMIC_BUILTIN_STORE(__generic, float)
SPIRV_ATOMIC_BUILTIN_STORE(__generic, double)

#define SPIRV_ATOMIC_BUILTIN_BINARY(SPIRV_ATOMIC_OP, GEN_ATOMIC_OP,            \
                                    ADDRESS_SPACE, TYPE)                       \
  CM_NODEBUG CM_INLINE TYPE __spirv_Atomic##SPIRV_ATOMIC_OP(                   \
      ADDRESS_SPACE TYPE *ptr, int Scope, int Semantics, TYPE Value) {         \
    return spirvAtomicHelper<GEN_ATOMIC_OP, ADDRESS_SPACE TYPE>(               \
        ptr, Semantics, Scope, Value);                                         \
  }

SPIRV_ATOMIC_BUILTIN_BINARY(SMin, atomic::operation::minsint, __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY(SMin, atomic::operation::minsint, __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY(SMin, atomic::operation::minsint, __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY(SMin, atomic::operation::minsint, __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY(SMin, atomic::operation::minsint, __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY(SMin, atomic::operation::minsint, __generic, long)

SPIRV_ATOMIC_BUILTIN_BINARY(SMax, atomic::operation::maxsint, __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY(SMax, atomic::operation::maxsint, __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY(SMax, atomic::operation::maxsint, __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY(SMax, atomic::operation::maxsint, __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY(SMax, atomic::operation::maxsint, __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY(SMax, atomic::operation::maxsint, __generic, long)

SPIRV_ATOMIC_BUILTIN_BINARY(UMin, atomic::operation::min, __global, uint)
SPIRV_ATOMIC_BUILTIN_BINARY(UMin, atomic::operation::min, __global, ulong)
SPIRV_ATOMIC_BUILTIN_BINARY(UMin, atomic::operation::min, __local, uint)
SPIRV_ATOMIC_BUILTIN_BINARY(UMin, atomic::operation::min, __local, ulong)
SPIRV_ATOMIC_BUILTIN_BINARY(UMin, atomic::operation::min, __generic, uint)
SPIRV_ATOMIC_BUILTIN_BINARY(UMin, atomic::operation::min, __generic, ulong)

SPIRV_ATOMIC_BUILTIN_BINARY(UMax, atomic::operation::max, __global, uint)
SPIRV_ATOMIC_BUILTIN_BINARY(UMax, atomic::operation::max, __global, ulong)
SPIRV_ATOMIC_BUILTIN_BINARY(UMax, atomic::operation::max, __local, uint)
SPIRV_ATOMIC_BUILTIN_BINARY(UMax, atomic::operation::max, __local, ulong)
SPIRV_ATOMIC_BUILTIN_BINARY(UMax, atomic::operation::max, __generic, uint)
SPIRV_ATOMIC_BUILTIN_BINARY(UMax, atomic::operation::max, __generic, ulong)

SPIRV_ATOMIC_BUILTIN_BINARY(IAdd, atomic::operation::add, __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY(IAdd, atomic::operation::add, __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY(IAdd, atomic::operation::add, __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY(IAdd, atomic::operation::add, __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY(IAdd, atomic::operation::add, __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY(IAdd, atomic::operation::add, __generic, long)

SPIRV_ATOMIC_BUILTIN_BINARY(ISub, atomic::operation::sub, __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY(ISub, atomic::operation::sub, __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY(ISub, atomic::operation::sub, __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY(ISub, atomic::operation::sub, __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY(ISub, atomic::operation::sub, __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY(ISub, atomic::operation::sub, __generic, long)

SPIRV_ATOMIC_BUILTIN_BINARY(Or, atomic::operation::orl, __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY(Or, atomic::operation::orl, __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY(Or, atomic::operation::orl, __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY(Or, atomic::operation::orl, __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY(Or, atomic::operation::orl, __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY(Or, atomic::operation::orl, __generic, long)

SPIRV_ATOMIC_BUILTIN_BINARY(Xor, atomic::operation::xorl, __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY(Xor, atomic::operation::xorl, __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY(Xor, atomic::operation::xorl, __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY(Xor, atomic::operation::xorl, __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY(Xor, atomic::operation::xorl, __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY(Xor, atomic::operation::xorl, __generic, long)

SPIRV_ATOMIC_BUILTIN_BINARY(And, atomic::operation::andl, __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY(And, atomic::operation::andl, __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY(And, atomic::operation::andl, __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY(And, atomic::operation::andl, __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY(And, atomic::operation::andl, __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY(And, atomic::operation::andl, __generic, long)

SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __global, float)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __global, double)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __local, float)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __local, double)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __generic, long)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __generic, float)
SPIRV_ATOMIC_BUILTIN_BINARY(Exchange, atomic::operation::xchg, __generic,
                            double)

#define SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(                              \
    SPIRV_ATOMIC_OP, GEN_ATOMIC_OP, OPVALUE, ADDRESS_SPACE, TYPE)              \
  CM_NODEBUG CM_INLINE TYPE __spirv_Atomic##SPIRV_ATOMIC_OP(                   \
      ADDRESS_SPACE TYPE *ptr, int Scope, int Semantics) {                     \
    TYPE Value = OPVALUE;                                                      \
    return spirvAtomicHelper<GEN_ATOMIC_OP, ADDRESS_SPACE TYPE>(               \
        ptr, Semantics, Scope, Value);                                         \
  }

SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IIncrement, atomic::operation::add, 1,
                                         __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IIncrement, atomic::operation::add, 1,
                                         __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IIncrement, atomic::operation::add, 1,
                                         __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IIncrement, atomic::operation::add, 1,
                                         __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IIncrement, atomic::operation::add, 1,
                                         __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IIncrement, atomic::operation::add, 1,
                                         __generic, long)

SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IDecrement, atomic::operation::sub, 1,
                                         __global, int)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IDecrement, atomic::operation::sub, 1,
                                         __global, long)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IDecrement, atomic::operation::sub, 1,
                                         __local, int)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IDecrement, atomic::operation::sub, 1,
                                         __local, long)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IDecrement, atomic::operation::sub, 1,
                                         __generic, int)
SPIRV_ATOMIC_BUILTIN_BINARY_WITH_OPVALUE(IDecrement, atomic::operation::sub, 1,
                                         __generic, long)

// FIXME: Unequal semantics will be eventually merged with
// Equal semantics which will result in Equal semantics,
// so we're skipping here for now.
#define SPIRV_ATOMIC_BUILTIN_TERNARY(SPIRV_ATOMIC_OP, GEN_ATOMIC_OP,           \
                                     ADDRESS_SPACE, TYPE)                      \
  CM_NODEBUG CM_INLINE TYPE __spirv_Atomic##SPIRV_ATOMIC_OP(                   \
      ADDRESS_SPACE TYPE *ptr, int Scope, int Equal, int Unequal, TYPE Value1, \
      TYPE Value2) {                                                           \
    return spirvAtomicHelper<GEN_ATOMIC_OP, ADDRESS_SPACE TYPE>(               \
        ptr, Equal, Scope, Value1, Value2);                                    \
  }

SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchange, atomic::operation::cmpxchg,
                             __global, int)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchange, atomic::operation::cmpxchg,
                             __global, long)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchange, atomic::operation::cmpxchg,
                             __local, int)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchange, atomic::operation::cmpxchg,
                             __local, long)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchange, atomic::operation::cmpxchg,
                             __generic, int)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchange, atomic::operation::cmpxchg,
                             __generic, long)

SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchangeWeak, atomic::operation::cmpxchg,
                             __global, int)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchangeWeak, atomic::operation::cmpxchg,
                             __global, long)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchangeWeak, atomic::operation::cmpxchg,
                             __local, int)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchangeWeak, atomic::operation::cmpxchg,
                             __local, long)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchangeWeak, atomic::operation::cmpxchg,
                             __generic, int)
SPIRV_ATOMIC_BUILTIN_TERNARY(CompareExchangeWeak, atomic::operation::cmpxchg,
                             __generic, long)
