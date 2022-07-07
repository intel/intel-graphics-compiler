/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <cm-cl/math.h>
#include <cm-cl/vector.h>

using namespace cm;

// Scalar IGC builtins use this flag to switch between "__builtin_spirv" and "__spirv" form of builtins.
// If it's defined, below wrappers are not required anymore. This is just a transitionary solution.
// Once Scalar IGC is switched to Khronos SPIRV Translator by default, below wrappers can just get removed.
#ifndef __USE_KHRONOS_SPIRV_TRANSLATOR_IN_SC__

extern "C" {
#include "spirv_math.h"
}

#define IGC_OCL_BUILTIN_NAME_1ARG_SCALAR(FUNC_NAME, POSTFIX_TYPE)              \
  __builtin_spirv_OpenCL_##FUNC_NAME##_##POSTFIX_TYPE

#define SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(FUNC_NAME, TYPE, OCL_POSTFIX)      \
  CM_NODEBUG CM_INLINE TYPE __spirv_ocl_##FUNC_NAME(TYPE x) {                  \
    return IGC_OCL_BUILTIN_NAME_1ARG_SCALAR(FUNC_NAME, OCL_POSTFIX)(x);        \
  }

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(exp, float, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(exp, double, f64);

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(exp10, float, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(exp10, double, f64);

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(log, float, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(log, double, f64);

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(log10, float, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR(log10, double, f64);

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

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp10, float, 2, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp10, float, 3, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp10, float, 4, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp10, float, 8, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp10, float, 16, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp10, double, 2, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp10, double, 3, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp10, double, 4, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp10, double, 8, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(exp10, double, 16, f64);

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

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log10, float, 2, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log10, float, 3, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log10, float, 4, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log10, float, 8, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log10, float, 16, f32);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log10, double, 2, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log10, double, 3, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log10, double, 4, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log10, double, 8, f64);
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR(log10, double, 16, f64);

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

#endif // __USE_KHRONOS_SPIRV_TRANSLATOR_IN_SC__

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

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(ceil, float, cm::math::ceil)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(floor, float, cm::math::floor)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(trunc, float, cm::math::truncate)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(roundne, float, cm::math::roundne)

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(s_abs, char, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(s_abs, short, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(s_abs, int, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(s_abs, long, cm::math::absolute)

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(fabs, float, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(fabs, double, cm::math::absolute)

#define SPIRV_MATH_BUILTIN_DECL_2ARG_SCALAR_CUSTOM(FUNC_NAME, TYPE,            \
                                                   CUSTOM_NAME)                \
  CM_NODEBUG CM_INLINE TYPE __spirv_ocl_##FUNC_NAME(TYPE x1, TYPE x2) {        \
    return CUSTOM_NAME(x1, x2);                                                \
  }

SPIRV_MATH_BUILTIN_DECL_2ARG_SCALAR_CUSTOM(fmin, float, cm::math::minimum)
SPIRV_MATH_BUILTIN_DECL_2ARG_SCALAR_CUSTOM(fmin, double, cm::math::minimum)

SPIRV_MATH_BUILTIN_DECL_2ARG_SCALAR_CUSTOM(fmax, float, cm::math::maximum)
SPIRV_MATH_BUILTIN_DECL_2ARG_SCALAR_CUSTOM(fmax, double, cm::math::maximum)

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(sqrt, float, cm::math::square_root)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM(sqrt, double, cm::math::square_root)

#define SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM_FAST(FUNC_NAME, TYPE,       \
                                                        CUSTOM_NAME)           \
  CM_NODEBUG CM_INLINE TYPE __spirv_ocl_##FUNC_NAME(TYPE x) {                  \
    return CUSTOM_NAME(x, cm::tag::fast);                                      \
  }

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM_FAST(native_sqrt, float,
                                                cm::math::square_root)

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM_FAST(native_log2, float,
                                                cm::math::log_base_2)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM_FAST(native_exp2, float,
                                                cm::math::exp_base_2)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM_FAST(native_sin, float,
                                                cm::math::sine)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CUSTOM_FAST(native_cos, float,
                                                cm::math::cosine)

#define SPIRV_MATH_BUILTIN_DECL_2ARG_SCALAR_CUSTOM_FAST(FUNC_NAME, TYPE,       \
                                                        CUSTOM_NAME)           \
  CM_NODEBUG CM_INLINE TYPE __spirv_ocl_##FUNC_NAME(TYPE x0, TYPE x1) {        \
    return CUSTOM_NAME(x0, x1, cm::tag::fast);                                 \
  }

SPIRV_MATH_BUILTIN_DECL_2ARG_SCALAR_CUSTOM_FAST(native_powr, float,
                                                cm::math::power_absolute_base)

#define SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CAST(FUNC_NAME, TYPE, CAST_TYPE,   \
                                                 CUSTOM_NAME)                  \
  CM_NODEBUG CM_INLINE TYPE __spirv_ocl_##FUNC_NAME(TYPE x) {                  \
    return CUSTOM_NAME(static_cast<CAST_TYPE>(x));                             \
  }

SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CAST(s_abs, uchar, char, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CAST(s_abs, ushort, short,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CAST(s_abs, uint, int, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_SCALAR_CAST(s_abs, ulong, long, cm::math::absolute)

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

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, char, 2, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, char, 3, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, char, 4, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, char, 8, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, char, 16, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, short, 2, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, short, 3, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, short, 4, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, short, 8, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, short, 16, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, int, 2, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, int, 3, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, int, 4, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, int, 8, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, int, 16, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, long, 2, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, long, 3, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, long, 4, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, long, 8, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(s_abs, long, 16, cm::math::absolute)

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(fabs, float, 2, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(fabs, float, 3, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(fabs, float, 4, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(fabs, float, 8, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(fabs, float, 16, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(fabs, double, 2, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(fabs, double, 3, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(fabs, double, 4, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(fabs, double, 8, cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(fabs, double, 16, cm::math::absolute)

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

SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmin, float, 2, cm::math::minimum)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmin, float, 3, cm::math::minimum)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmin, float, 4, cm::math::minimum)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmin, float, 8, cm::math::minimum)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmin, float, 16, cm::math::minimum)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmin, double, 2, cm::math::minimum)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmin, double, 3, cm::math::minimum)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmin, double, 4, cm::math::minimum)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmin, double, 8, cm::math::minimum)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmin, double, 16, cm::math::minimum)

SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmax, float, 2, cm::math::maximum)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmax, float, 3, cm::math::maximum)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmax, float, 4, cm::math::maximum)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmax, float, 8, cm::math::maximum)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmax, float, 16, cm::math::maximum)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmax, double, 2, cm::math::maximum)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmax, double, 3, cm::math::maximum)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmax, double, 4, cm::math::maximum)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmax, double, 8, cm::math::maximum)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM(fmax, double, 16, cm::math::maximum)

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(sqrt, float, 2,
                                           cm::math::square_root)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(sqrt, float, 3,
                                           cm::math::square_root)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(sqrt, float, 4,
                                           cm::math::square_root)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(sqrt, float, 8,
                                           cm::math::square_root)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(sqrt, float, 16,
                                           cm::math::square_root)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(sqrt, double, 2,
                                           cm::math::square_root)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(sqrt, double, 3,
                                           cm::math::square_root)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(sqrt, double, 4,
                                           cm::math::square_root)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(sqrt, double, 8,
                                           cm::math::square_root)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM(sqrt, double, 16,
                                           cm::math::square_root)

#define SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(                       \
    FUNC_NAME, ELEMENT_TYPE, N, CUSTOM_NAME)                                   \
  CM_NODEBUG CM_INLINE cl_vector<ELEMENT_TYPE, N> __spirv_ocl_##FUNC_NAME(     \
      cl_vector<ELEMENT_TYPE, N> x) {                                          \
    vector<ELEMENT_TYPE, N> x_vec = x;                                         \
    return static_cast<vector<ELEMENT_TYPE, N>>(                               \
               CUSTOM_NAME(x_vec, cm::tag::fast))                              \
        .cl_vector();                                                          \
  }

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_sqrt, float, 2,
                                                cm::math::square_root)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_sqrt, float, 3,
                                                cm::math::square_root)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_sqrt, float, 4,
                                                cm::math::square_root)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_sqrt, float, 8,
                                                cm::math::square_root)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_sqrt, float, 16,
                                                cm::math::square_root)

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_log2, float, 2,
                                                cm::math::log_base_2)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_log2, float, 3,
                                                cm::math::log_base_2)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_log2, float, 4,
                                                cm::math::log_base_2)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_log2, float, 8,
                                                cm::math::log_base_2)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_log2, float, 16,
                                                cm::math::log_base_2)

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_exp2, float, 2,
                                                cm::math::exp_base_2)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_exp2, float, 3,
                                                cm::math::exp_base_2)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_exp2, float, 4,
                                                cm::math::exp_base_2)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_exp2, float, 8,
                                                cm::math::exp_base_2)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_exp2, float, 16,
                                                cm::math::exp_base_2)

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_sin, float, 2,
                                                cm::math::sine)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_sin, float, 3,
                                                cm::math::sine)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_sin, float, 4,
                                                cm::math::sine)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_sin, float, 8,
                                                cm::math::sine)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_sin, float, 16,
                                                cm::math::sine)

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_cos, float, 2,
                                                cm::math::cosine)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_cos, float, 3,
                                                cm::math::cosine)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_cos, float, 4,
                                                cm::math::cosine)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_cos, float, 8,
                                                cm::math::cosine)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CUSTOM_FAST(native_cos, float, 16,
                                                cm::math::cosine)

#define SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM_FAST(                       \
    FUNC_NAME, ELEMENT_TYPE, N, CUSTOM_NAME)                                   \
  CM_NODEBUG CM_INLINE cl_vector<ELEMENT_TYPE, N> __spirv_ocl_##FUNC_NAME(     \
      cl_vector<ELEMENT_TYPE, N> x0, cl_vector<ELEMENT_TYPE, N> x1) {          \
    vector<ELEMENT_TYPE, N> x0_vec = x0;                                       \
    vector<ELEMENT_TYPE, N> x1_vec = x1;                                       \
    return static_cast<vector<ELEMENT_TYPE, N>>(                               \
               CUSTOM_NAME(x0_vec, x1_vec, cm::tag::fast))                     \
        .cl_vector();                                                          \
  }

SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM_FAST(native_powr, float, 2,
                                                cm::math::power_absolute_base)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM_FAST(native_powr, float, 3,
                                                cm::math::power_absolute_base)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM_FAST(native_powr, float, 4,
                                                cm::math::power_absolute_base)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM_FAST(native_powr, float, 8,
                                                cm::math::power_absolute_base)
SPIRV_MATH_BUILTIN_DECL_2ARG_VECTOR_CUSTOM_FAST(native_powr, float, 16,
                                                cm::math::power_absolute_base)

#define SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(FUNC_NAME, TYPE, CAST_TYPE,   \
                                                 N, CUSTOM_NAME)               \
  CM_NODEBUG CM_INLINE cl_vector<CAST_TYPE, N> __spirv_ocl_##FUNC_NAME(        \
      cl_vector<TYPE, N> x) {                                                  \
    vector<CAST_TYPE, N> x_vec{x};                                             \
    return static_cast<vector<CAST_TYPE, N>>(CUSTOM_NAME(x_vec)).cl_vector();  \
  }

SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, uchar, char, 2,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, uchar, char, 3,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, uchar, char, 4,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, uchar, char, 8,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, uchar, char, 16,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, ushort, short, 2,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, ushort, short, 3,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, ushort, short, 4,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, ushort, short, 8,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, ushort, short, 1,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, uint, int, 2,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, uint, int, 3,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, uint, int, 4,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, uint, int, 8,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, uint, int, 16,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, ulong, long, 2,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, ulong, long, 3,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, ulong, long, 4,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, ulong, long, 8,
                                         cm::math::absolute)
SPIRV_MATH_BUILTIN_DECL_1ARG_VECTOR_CAST(s_abs, ulong, long, 16,
                                         cm::math::absolute)

#define SPIRV_MATH_BUILTIN_DECL_3ARG_SCALAR_CUSTOM(FUNC_NAME, TYPE,            \
                                                   CUSTOM_NAME)                \
  CM_NODEBUG CM_INLINE TYPE __spirv_ocl_##FUNC_NAME(TYPE x1, TYPE x2,          \
                                                    TYPE x3) {                 \
    return CUSTOM_NAME(x1, x2, x3);                                            \
  }

SPIRV_MATH_BUILTIN_DECL_3ARG_SCALAR_CUSTOM(mad, float, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_SCALAR_CUSTOM(mad, double, cm::math::mad)

SPIRV_MATH_BUILTIN_DECL_3ARG_SCALAR_CUSTOM(fma, float, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_SCALAR_CUSTOM(fma, double, cm::math::mad)

#define SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(FUNC_NAME, ELEMENT_TYPE, N, \
                                                   CUSTOM_NAME)                \
  CM_NODEBUG CM_INLINE cl_vector<ELEMENT_TYPE, N> __spirv_ocl_##FUNC_NAME(     \
      cl_vector<ELEMENT_TYPE, N> x1, cl_vector<ELEMENT_TYPE, N> x2,            \
      cl_vector<ELEMENT_TYPE, N> x3) {                                         \
    vector<ELEMENT_TYPE, N> x_vec1 = x1;                                       \
    vector<ELEMENT_TYPE, N> x_vec2 = x2;                                       \
    vector<ELEMENT_TYPE, N> x_vec3 = x3;                                       \
    return static_cast<vector<ELEMENT_TYPE, N>>(                               \
               CUSTOM_NAME(x_vec1, x_vec2, x_vec3))                            \
        .cl_vector();                                                          \
  }

SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(mad, float, 2, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(mad, float, 3, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(mad, float, 4, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(mad, float, 8, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(mad, float, 16, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(mad, double, 2, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(mad, double, 3, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(mad, double, 4, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(mad, double, 8, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(mad, double, 16, cm::math::mad)

SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(fma, float, 2, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(fma, float, 3, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(fma, float, 4, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(fma, float, 8, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(fma, float, 16, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(fma, double, 2, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(fma, double, 3, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(fma, double, 4, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(fma, double, 8, cm::math::mad)
SPIRV_MATH_BUILTIN_DECL_3ARG_VECTOR_CUSTOM(fma, double, 16, cm::math::mad)
