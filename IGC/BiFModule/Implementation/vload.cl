/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file defines SPIRV vloadn, vload_half, vload_halfn and vloada_halfn built-ins.
//

#include "include/vload_vstore_impl.h"

//*****************************************************************************/
// helper functions
//*****************************************************************************/

static OVERLOADABLE float __intel_spirv_half2float(short h)
{
    return __spirv_FConvert_Rfloat(as_half(h));
}

GENERATE_VECTOR_FUNCTIONS_1ARG_NO_MANG(__intel_spirv_half2float, float, short)

VLOADN_TYPE(char,   i8)
VLOADN_TYPE(short,  i16)
VLOADN_TYPE(int,    i32)
VLOADN_TYPE(long,   i64)
VLOADN_TYPE(half,   f16)
VLOADN_TYPE(float,  f32)
#if defined(cl_khr_fp64)
VLOADN_TYPE(double, f64)
#endif

// "When extended by the cl_khr_fp16 extension, the generic type gentypen is extended to include half"
#define VLOADN_SCALAR_HALF_DEF(addressSpace, offsetType, mangle)                                 \
INLINE half __attribute__((overloadable)) __spirv_ocl_vloadn_Rhalf(               \
    offsetType offset, addressSpace half * p, int n) {                                           \
  const addressSpace half *pOffset = p + offset;                                                 \
  half ret;                                                                                      \
  __builtin_IB_memcpy_##addressSpace##_to_private(                                               \
      (private uchar *)&ret, (addressSpace uchar *)pOffset, sizeof(half), sizeof(half));         \
  return ret;                                                                                    \
}

#define VLOADN_SCALAR_HALF_AS(addressSpace, mang)            \
VLOADN_SCALAR_HALF_DEF(addressSpace, long, i64_##mang##f16)  \
VLOADN_SCALAR_HALF_DEF(addressSpace, int,  i32_##mang##f16)  \

VLOADN_SCALAR_HALF_AS(private,  p0)
VLOADN_SCALAR_HALF_AS(global,   p1)
VLOADN_SCALAR_HALF_AS(constant, p2)
VLOADN_SCALAR_HALF_AS(local,    p3)
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
VLOADN_SCALAR_HALF_AS(generic,  p4)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

//*****************************************************************************/
// vload_half
// "Reads a half value from the address computed as (p + (offset)) and converts it to a float result value."
//*****************************************************************************/

#define VLOAD_HALF(addressSpace, ASNUM)                                                                                \
INLINE float __attribute__((overloadable)) __spirv_ocl_vload_half_Rfloat(long offset,                \
                                                                                           addressSpace half *p) {     \
    return __intel_spirv_half2float(as_short(*(p + offset)));                                                          \
}                                                                                                                      \
INLINE float __attribute__((overloadable)) __spirv_ocl_vload_half_Rfloat(int offset,                 \
                                                                                           addressSpace half *p) {     \
    return __intel_spirv_half2float(as_short(*(p + offset)));                                                          \
}

VLOAD_HALF(__private,  0)
VLOAD_HALF(__global,   1)
VLOAD_HALF(__constant, 2)
VLOAD_HALF(__local,    3)
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
VLOAD_HALF(__generic,  4)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

//*****************************************************************************/
// vload_halfn
// "Reads n half components from the address (p + (offset * n)), converts to n float components,
//  and creates a float vector result value from the n float components."
//*****************************************************************************/

#define VLOAD_HALFX_DEF(addressSpace, ASNUM, MANGSIZE, SIZETYPE, numElements)                                                  \
INLINE float##numElements __attribute__((overloadable)) __spirv_ocl_vload_halfn_Rfloat##numElements                            \
    (SIZETYPE offset, addressSpace half * p, int n) {                                                                          \
    return __intel_spirv_half2float(__spirv_ocl_vloadn_Rshort##numElements(offset, (addressSpace short *)p, numElements));     \
}

#define VLOAD_HALFX_AS(addressSpace, ASNUM)           \
VLOAD_HALFX_DEF(addressSpace, ASNUM, i64, long, 2)    \
VLOAD_HALFX_DEF(addressSpace, ASNUM, i64, long, 3)    \
VLOAD_HALFX_DEF(addressSpace, ASNUM, i64, long, 4)    \
VLOAD_HALFX_DEF(addressSpace, ASNUM, i64, long, 8)    \
VLOAD_HALFX_DEF(addressSpace, ASNUM, i64, long, 16)   \
VLOAD_HALFX_DEF(addressSpace, ASNUM, i32, int,  2)    \
VLOAD_HALFX_DEF(addressSpace, ASNUM, i32, int,  3)    \
VLOAD_HALFX_DEF(addressSpace, ASNUM, i32, int,  4)    \
VLOAD_HALFX_DEF(addressSpace, ASNUM, i32, int,  8)    \
VLOAD_HALFX_DEF(addressSpace, ASNUM, i32, int,  16)

VLOAD_HALFX_AS(__private,  0)
VLOAD_HALFX_AS(__global,   1)
VLOAD_HALFX_AS(__constant, 2)
VLOAD_HALFX_AS(__local,    3)
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
VLOAD_HALFX_AS(__generic,  4)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

//*****************************************************************************/
// vloada_halfn
// "Reads a vector of n half values from aligned memory and converts it to a float vector result value."
//*****************************************************************************/

#define VLOADA_HALFX_DEF(addressSpace, ASNUM, MANGSIZE, SIZETYPE, step, numElements)                              \
INLINE float##numElements __attribute__((overloadable)) __spirv_ocl_vloada_halfn_Rfloat##numElements                  \
    (SIZETYPE offset, addressSpace half * p, int n) {                                                             \
    const addressSpace short##numElements *pHalf = (const addressSpace short##numElements *)(p + offset * step);  \
    return __intel_spirv_half2float(*pHalf);                                                                      \
}

#define VLOADA_HALFX_AS(addressSpace, ASNUM)              \
VLOADA_HALFX_DEF(addressSpace, ASNUM, i64, long, 1,  )    \
VLOADA_HALFX_DEF(addressSpace, ASNUM, i64, long, 2,  2)   \
VLOADA_HALFX_DEF(addressSpace, ASNUM, i64, long, 4,  3)   \
VLOADA_HALFX_DEF(addressSpace, ASNUM, i64, long, 4,  4)   \
VLOADA_HALFX_DEF(addressSpace, ASNUM, i64, long, 8,  8)   \
VLOADA_HALFX_DEF(addressSpace, ASNUM, i64, long, 16, 16)  \
VLOADA_HALFX_DEF(addressSpace, ASNUM, i32, int,  1,  )    \
VLOADA_HALFX_DEF(addressSpace, ASNUM, i32, int,  2,  2)   \
VLOADA_HALFX_DEF(addressSpace, ASNUM, i32, int,  4,  3)   \
VLOADA_HALFX_DEF(addressSpace, ASNUM, i32, int,  4,  4)   \
VLOADA_HALFX_DEF(addressSpace, ASNUM, i32, int,  8,  8)   \
VLOADA_HALFX_DEF(addressSpace, ASNUM, i32, int,  16, 16)

VLOADA_HALFX_AS(__private,  0)
VLOADA_HALFX_AS(__global,   1)
VLOADA_HALFX_AS(__constant, 2)
VLOADA_HALFX_AS(__local,    3)
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
VLOADA_HALFX_AS(__generic,  4)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
