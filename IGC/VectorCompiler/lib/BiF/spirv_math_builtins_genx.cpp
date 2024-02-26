/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2024 Intel Corporation

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
