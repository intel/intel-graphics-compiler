/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file defines SPIRV vload/vstore built-ins.
//
//===----------------------------------------------------------------------===//

//*****************************************************************************/
// vload/vstore macros
//*****************************************************************************/

#define VLOAD_MACRO(addressSpace, scalarType, numElements, offsetType, mangle)                                                                                \
INLINE scalarType##numElements SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(vload, numElements##_##mangle, n_R##scalarType##numElements)(offsetType offset, addressSpace scalarType *p)                             \
{                                                                                                                                                             \
  const addressSpace scalarType *pOffset = p + offset * numElements;                                                                                          \
  scalarType##numElements ret;                                                                                                                                \
  __builtin_IB_memcpy_##addressSpace##_to_private((private uchar*)&ret, (addressSpace uchar*)pOffset, sizeof(scalarType) * numElements, sizeof(scalarType));  \
  return ret;                                                                                                                                                 \
}

#define VSTORE_MACRO(addressSpace, scalarType, numElements, offsetType, mangle)                                                                               \
INLINE void __builtin_spirv_OpenCL_vstore##numElements##_##mangle(scalarType##numElements data, offsetType offset, addressSpace scalarType *p)                       \
{                                                                                                                                                             \
  addressSpace scalarType *pOffset = p + offset * numElements;                                                                                                \
  scalarType##numElements ret = data;                                                                                                                         \
  __builtin_IB_memcpy_private_to_##addressSpace((addressSpace uchar*)pOffset, (private uchar*)&ret, sizeof(scalarType) * numElements, sizeof(scalarType));    \
}

#define ELEM_ARG(addressSpace, scalarType, mang)             \
VLOAD_MACRO(addressSpace, scalarType, 2,  long, i64_##mang) \
VLOAD_MACRO(addressSpace, scalarType, 2,  int,  i32_##mang) \
VLOAD_MACRO(addressSpace, scalarType, 3,  long, i64_##mang) \
VLOAD_MACRO(addressSpace, scalarType, 3,  int,  i32_##mang) \
VLOAD_MACRO(addressSpace, scalarType, 4,  long, i64_##mang) \
VLOAD_MACRO(addressSpace, scalarType, 4,  int,  i32_##mang) \
VLOAD_MACRO(addressSpace, scalarType, 8,  long, i64_##mang) \
VLOAD_MACRO(addressSpace, scalarType, 8,  int,  i32_##mang) \
VLOAD_MACRO(addressSpace, scalarType, 16, long, i64_##mang) \
VLOAD_MACRO(addressSpace, scalarType, 16, int,  i32_##mang)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define TYPE_ARG(TYPE, TYPEMANG)       \
ELEM_ARG(global,   TYPE, p1##TYPEMANG) \
ELEM_ARG(constant, TYPE, p2##TYPEMANG) \
ELEM_ARG(local,    TYPE, p3##TYPEMANG) \
ELEM_ARG(private,  TYPE, p0##TYPEMANG) \
ELEM_ARG(generic,  TYPE, p4##TYPEMANG)
#else
#define TYPE_ARG(TYPE, TYPEMANG)       \
ELEM_ARG(global,   TYPE, p1##TYPEMANG) \
ELEM_ARG(constant, TYPE, p2##TYPEMANG) \
ELEM_ARG(local,    TYPE, p3##TYPEMANG) \
ELEM_ARG(private,  TYPE, p0##TYPEMANG)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

TYPE_ARG(char,  i8)
TYPE_ARG(short, i16)
TYPE_ARG(int,   i32)
TYPE_ARG(long,  i64)
TYPE_ARG(half,   f16)
TYPE_ARG(float,  f32)
#if defined(cl_khr_fp64)
TYPE_ARG(double, f64)
#endif

#undef TYPE_ARG
#undef ELEM_ARG

#define ELEM_ARG(addressSpace, scalarType, typemang, mang)                    \
VSTORE_MACRO(addressSpace, scalarType, 2,  ulong, v2##typemang##_i64_##mang)  \
VSTORE_MACRO(addressSpace, scalarType, 2,  uint,  v2##typemang##_i32_##mang)  \
VSTORE_MACRO(addressSpace, scalarType, 3,  ulong, v3##typemang##_i64_##mang)  \
VSTORE_MACRO(addressSpace, scalarType, 3,  uint,  v3##typemang##_i32_##mang)  \
VSTORE_MACRO(addressSpace, scalarType, 4,  ulong, v4##typemang##_i64_##mang)  \
VSTORE_MACRO(addressSpace, scalarType, 4,  uint,  v4##typemang##_i32_##mang)  \
VSTORE_MACRO(addressSpace, scalarType, 8,  ulong, v8##typemang##_i64_##mang)  \
VSTORE_MACRO(addressSpace, scalarType, 8,  uint,  v8##typemang##_i32_##mang)  \
VSTORE_MACRO(addressSpace, scalarType, 16, ulong, v16##typemang##_i64_##mang) \
VSTORE_MACRO(addressSpace, scalarType, 16, uint,  v16##typemang##_i32_##mang)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define TYPE_ARG(TYPE, TYPEMANG)                 \
ELEM_ARG(global,   TYPE, TYPEMANG, p1##TYPEMANG) \
ELEM_ARG(local,    TYPE, TYPEMANG, p3##TYPEMANG) \
ELEM_ARG(private,  TYPE, TYPEMANG, p0##TYPEMANG) \
ELEM_ARG(generic,  TYPE, TYPEMANG, p4##TYPEMANG)
#else
#define TYPE_ARG(TYPE, TYPEMANG)                 \
ELEM_ARG(global,   TYPE, TYPEMANG, p1##TYPEMANG) \
ELEM_ARG(local,    TYPE, TYPEMANG, p3##TYPEMANG) \
ELEM_ARG(private,  TYPE, TYPEMANG, p0##TYPEMANG)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

TYPE_ARG(uchar,  i8)
TYPE_ARG(ushort, i16)
TYPE_ARG(uint,   i32)
TYPE_ARG(ulong,  i64)
TYPE_ARG(half,   f16)
TYPE_ARG(float,  f32)
#if defined(cl_khr_fp64)
TYPE_ARG(double, f64)
#endif

#undef TYPE_ARG
#undef ELEM_ARG

//*****************************************************************************/
// vload macros
//*****************************************************************************/
static OVERLOADABLE float __intel_spirv_half2float(short h)
{
    return SPIRV_BUILTIN(FConvert, _f32_f16, _Rfloat)(as_half(h));
}

#define VLOAD_SHORT(addressSpace, ASNUM)                                                                 \
INLINE static short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(vload, _i64_p##ASNUM##i16, n_Rshort)(long offset, addressSpace short* p) \
{                                                                                                        \
    return *(p + offset);                                                                                \
}                                                                                                        \
INLINE static short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(vload, _i32_p##ASNUM##i16, n_Rshort)(int offset, addressSpace short* p)  \
{                                                                                                        \
    return *(p + offset);                                                                                \
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
VLOAD_SHORT(__generic,  4)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
VLOAD_SHORT(__global,   1)
VLOAD_SHORT(__local,    3)
VLOAD_SHORT(__constant, 2)
VLOAD_SHORT(__private,  0)

GENERATE_VECTOR_FUNCTIONS_1ARG_NO_MANG(__intel_spirv_half2float, float, short)

// Two copies for the i32 and i64 size_t offsets.
#define __CLFN_DEF_F_VLOAD_SCALAR_HALF(addressSpace, ASNUM)                                      \
INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(vload, _i32_p##ASNUM##f16, _Rhalf)(int offset, addressSpace half* p) {   \
  return *(p + offset);                                                                          \
}                                                                                                \
INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(vload, _i64_p##ASNUM##f16, _Rhalf)(long offset, addressSpace half* p) {  \
  return *(p + offset);                                                                          \
}

#define __CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, MANGSIZE, SIZETYPE, numElements, postfix)                                                        \
INLINE float##numElements SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(vload_half, numElements##_##MANGSIZE##_p##ASNUM##f16, postfix##_Rfloat##numElements)(SIZETYPE offset, addressSpace half* p) { \
  return __intel_spirv_half2float(SPIRV_OCL_BUILTIN(vload, numElements##_##MANGSIZE##_p##ASNUM##i16, n_Rshort##numElements)(offset, (addressSpace short*)p));          \
}

#define __CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, MANGSIZE, SIZETYPE, step, numElements)                                                 \
INLINE float##numElements SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(vloada_half, numElements##_##MANGSIZE##_p##ASNUM##f16, n_Rfloat##numElements)(SIZETYPE offset, addressSpace half* p) {  \
  const addressSpace short##numElements* pHalf = (const addressSpace short##numElements*)(p + offset * step);  \
  return __intel_spirv_half2float(*pHalf);                                                                     \
}

#define __CLFN_DEF_F_VLOAD_HALFX_AS(addressSpace, ASNUM)             \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i64, long, , )         \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i64, long, 2, n)       \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i64, long, 3, n)       \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i64, long, 4, n)       \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i64, long, 8, n)       \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i64, long, 16, n)      \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i32, int, , )          \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i32, int, 2, n)        \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i32, int, 3, n)        \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i32, int, 4, n)        \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i32, int, 8, n)        \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i32, int, 16, n)

#define __CLFN_DEF_F_VLOADA_HALFX_AS(addressSpace, ASNUM)          \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i64, long, 1, )     \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i64, long, 2, 2)    \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i64, long, 4, 3)    \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i64, long, 4, 4)    \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i64, long, 8, 8)    \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i64, long, 16, 16)  \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i32, int, 1, )      \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i32, int, 2, 2)     \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i32, int, 4, 3)     \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i32, int, 4, 4)     \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i32, int, 8, 8)     \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i32, int, 16, 16)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define __CLFN_DEF_F_VLOAD_HALF_ALL()       \
__CLFN_DEF_F_VLOAD_HALFX_AS(__generic,  4)  \
__CLFN_DEF_F_VLOAD_HALFX_AS(__global,   1)  \
__CLFN_DEF_F_VLOAD_HALFX_AS(__local,    3)  \
__CLFN_DEF_F_VLOAD_HALFX_AS(__constant, 2)  \
__CLFN_DEF_F_VLOAD_HALFX_AS(__private,  0)
#else
#define __CLFN_DEF_F_VLOAD_HALF_ALL()       \
__CLFN_DEF_F_VLOAD_HALFX_AS(__global,   1)  \
__CLFN_DEF_F_VLOAD_HALFX_AS(__local,    3)  \
__CLFN_DEF_F_VLOAD_HALFX_AS(__constant, 2)  \
__CLFN_DEF_F_VLOAD_HALFX_AS(__private,  0)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define __CLFN_DEF_F_VLOADA_HALF_ALL()      \
__CLFN_DEF_F_VLOADA_HALFX_AS(__generic,  4) \
__CLFN_DEF_F_VLOADA_HALFX_AS(__global,   1) \
__CLFN_DEF_F_VLOADA_HALFX_AS(__local,    3) \
__CLFN_DEF_F_VLOADA_HALFX_AS(__constant, 2) \
__CLFN_DEF_F_VLOADA_HALFX_AS(__private,  0)
#else
#define __CLFN_DEF_F_VLOADA_HALF_ALL()      \
__CLFN_DEF_F_VLOADA_HALFX_AS(__global,   1) \
__CLFN_DEF_F_VLOADA_HALFX_AS(__local,    3) \
__CLFN_DEF_F_VLOADA_HALFX_AS(__constant, 2) \
__CLFN_DEF_F_VLOADA_HALFX_AS(__private,  0)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

//*****************************************************************************/
// vstore macros (half)
//*****************************************************************************/
// These helper functions are used to macro-ize the rounding mode built-ins.
static OVERLOADABLE half __intel_spirv_float2half_rtz(float f)
{
    return SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _rtz_Rhalf)(f);
}

static OVERLOADABLE half __intel_spirv_float2half_rte(float f)
{
    return SPIRV_BUILTIN(FConvert, _RTE_f16_f32, _rte_Rhalf)(f);
}

static OVERLOADABLE half __intel_spirv_float2half_rtp(float f)
{
    return SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _rtp_Rhalf)(f);
}

static OVERLOADABLE half __intel_spirv_float2half_rtn(float f)
{
    return SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _rtn_Rhalf)(f);
}

static OVERLOADABLE half __intel_spirv_float2half(float f)
{
    return __intel_spirv_float2half_rte(f);
}

#if defined(cl_khr_fp64)

static OVERLOADABLE half __intel_spirv_double2half_rtz(double a)
{
    return SPIRV_BUILTIN(FConvert, _RTZ_f16_f64, _rtz_Rhalf)(a);
}

static OVERLOADABLE half __intel_spirv_double2half_rte(double a)
{
    return SPIRV_BUILTIN(FConvert, _RTE_f16_f64, _rte_Rhalf)(a);
}

static OVERLOADABLE half __intel_spirv_double2half_rtp(double a)
{
    return SPIRV_BUILTIN(FConvert, _RTP_f16_f64, _rtp_Rhalf)(a);
}

static OVERLOADABLE half __intel_spirv_double2half_rtn(double a)
{
    return SPIRV_BUILTIN(FConvert, _RTN_f16_f64, _rtn_Rhalf)(a);
}

static OVERLOADABLE half __intel_spirv_double2half(double a)
{
    return __intel_spirv_double2half_rte(a);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_NO_MANG(__intel_spirv_double2half_rtz, half, double)
GENERATE_VECTOR_FUNCTIONS_1ARG_NO_MANG(__intel_spirv_double2half_rte, half, double)
GENERATE_VECTOR_FUNCTIONS_1ARG_NO_MANG(__intel_spirv_double2half_rtp, half, double)
GENERATE_VECTOR_FUNCTIONS_1ARG_NO_MANG(__intel_spirv_double2half_rtn, half, double)
GENERATE_VECTOR_FUNCTIONS_1ARG_NO_MANG(__intel_spirv_double2half,     half, double)

#endif //defined(cl_khr_fp64)

GENERATE_VECTOR_FUNCTIONS_1ARG_NO_MANG(__intel_spirv_float2half_rtz, half, float)
GENERATE_VECTOR_FUNCTIONS_1ARG_NO_MANG(__intel_spirv_float2half_rte, half, float)
GENERATE_VECTOR_FUNCTIONS_1ARG_NO_MANG(__intel_spirv_float2half_rtp, half, float)
GENERATE_VECTOR_FUNCTIONS_1ARG_NO_MANG(__intel_spirv_float2half_rtn, half, float)
GENERATE_VECTOR_FUNCTIONS_1ARG_NO_MANG(__intel_spirv_float2half,     half, float)

#define __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, MANGSIZE, SIZETYPE, HASVEC, rnd, MANGSRC, srcType, numElements)                               \
INLINE void __builtin_spirv_OpenCL_vstore_half##numElements##rnd##_##HASVEC##numElements##MANGSRC##_##MANGSIZE##_p##ASNUM##f16(srcType##numElements data, \
                                                           SIZETYPE offset,                                                                        \
                                                           addressSpace half* p) {                                                                 \
  __builtin_spirv_OpenCL_vstore##numElements##_##HASVEC##numElements##f16##_##MANGSIZE##_p##ASNUM##f16(__intel_spirv_##srcType##2half##rnd(data), offset, p);   \
}

#define __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, MANGSIZE, SIZETYPE, HASVEC, rnd, MANGSRC, srcType, step, numElements)                         \
INLINE void __builtin_spirv_OpenCL_vstorea_half##numElements##rnd##_##HASVEC##numElements##MANGSRC##_##MANGSIZE##_p##ASNUM##f16(srcType##numElements data, \
                                                        SIZETYPE offset,                                                                            \
                                                        addressSpace half* p) {                                                                     \
  addressSpace half##numElements *pHalf = (addressSpace half##numElements *)(p + offset * step);                                                    \
  *pHalf = __intel_spirv_##srcType##2half##rnd(data);                                                                                                     \
}

// Two copies for the i32 and i64 size_t offsets.
#define __CLFN_DEF_VSTORE_SCALAR_HALF(addressSpace, ASNUM)       \
INLINE void __builtin_spirv_OpenCL_vstore_f16_i32_p##ASNUM##f16(        \
    half data,                                                   \
    uint offset,                                                 \
    addressSpace half* p) {                                      \
  addressSpace half *pHalf = (addressSpace half *)(p + offset);  \
  *pHalf = data;                                                 \
}                                                                \
INLINE void __builtin_spirv_OpenCL_vstore_f16_i64_p##ASNUM##f16(        \
    half data,                                                   \
    ulong offset,                                                \
    addressSpace half* p) {                                      \
  addressSpace half *pHalf = (addressSpace half *)(p + offset);  \
  *pHalf = data;                                                 \
}

#if defined(cl_khr_fp64)
#define __CLFN_DEF_VSTORE_HALF(addressSpace, ASNUM, rnd)                                  \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, ,    rnd, f32, float, )        \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 2)       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 3)       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 4)       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 8)       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 16)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, ,    rnd, f64, double, )        \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 2)       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 3)       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 4)       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 8)       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 16)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  ,    rnd, f32, float, )       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 2)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 3)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 4)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 8)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 16)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  ,    rnd, f64, double, )       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 2)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 3)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 4)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 8)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 16)
#else
#define __CLFN_DEF_VSTORE_HALF(addressSpace, ASNUM, rnd)                                  \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, ,    rnd, f32, float, )        \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 2)       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 3)       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 4)       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 8)       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 16)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  ,    rnd, f32, float, )       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 2)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 3)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 4)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 8)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 16)
#endif //defined(cl_khr_fp64)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define __CLFN_DEF_VSTORE_HALF_AS(rnd)          \
  __CLFN_DEF_VSTORE_HALF(generic, 4, rnd)       \
  __CLFN_DEF_VSTORE_HALF(global,  1, rnd)       \
  __CLFN_DEF_VSTORE_HALF(local,   3, rnd)       \
  __CLFN_DEF_VSTORE_HALF(private, 0, rnd)
#else
#define __CLFN_DEF_VSTORE_HALF_AS(rnd)          \
  __CLFN_DEF_VSTORE_HALF(global,  1, rnd)       \
  __CLFN_DEF_VSTORE_HALF(local,   3, rnd)       \
  __CLFN_DEF_VSTORE_HALF(private, 0, rnd)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#define __CLFN_DEF_VSTORE_HALF_ALL()                          \
  __CLFN_DEF_VSTORE_HALF_AS()                                 \
  __CLFN_DEF_VSTORE_HALF_AS(_rte)                             \
  __CLFN_DEF_VSTORE_HALF_AS(_rtz)                             \
  __CLFN_DEF_VSTORE_HALF_AS(_rtp)                             \
  __CLFN_DEF_VSTORE_HALF_AS(_rtn)

#if defined(cl_khr_fp64)
#define __CLFN_DEF_VSTOREA_HALF(addressSpace, ASNUM, rnd)                                     \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, ,    rnd, f32, float, 1, )        \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 2, 2)       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 4, 3)       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 4, 4)       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 8, 8)       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 16, 16)     \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, ,    rnd, f64, double, 1, )        \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 2, 2)       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 4, 3)       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 4, 4)       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 8, 8)       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 16, 16)     \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  ,    rnd, f32, float, 1, )       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 2, 2)      \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 4, 3)      \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 4, 4)      \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 8, 8)      \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 16, 16)     \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  ,    rnd, f64, double, 1, )       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 2, 2)      \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 4, 3)      \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 4, 4)      \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 8, 8)      \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 16, 16)
#else
#define __CLFN_DEF_VSTOREA_HALF(addressSpace, ASNUM, rnd)                                     \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, ,    rnd, f32, float, 1, )        \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 2, 2)       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 4, 3)       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 4, 4)       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 8, 8)       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f32, float, 16, 16)     \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  ,    rnd, f32, float, 1, )       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 2, 2)      \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 4, 3)      \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 4, 4)      \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 8, 8)      \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f32, float, 16, 16)
#endif //defined(cl_khr_fp64)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define __CLFN_DEF_VSTOREA_HALF_AS(rnd)      \
  __CLFN_DEF_VSTOREA_HALF(generic, 4, rnd)   \
  __CLFN_DEF_VSTOREA_HALF(global,  1, rnd)   \
  __CLFN_DEF_VSTOREA_HALF(local,   3, rnd)   \
  __CLFN_DEF_VSTOREA_HALF(private, 0, rnd)
#else
#define __CLFN_DEF_VSTOREA_HALF_AS(rnd)      \
  __CLFN_DEF_VSTOREA_HALF(global,  1, rnd)   \
  __CLFN_DEF_VSTOREA_HALF(local,   3, rnd)   \
  __CLFN_DEF_VSTOREA_HALF(private, 0, rnd)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#define __CLFN_DEF_VSTOREA_HALF_ALL()                       \
  __CLFN_DEF_VSTOREA_HALF_AS()                              \
  __CLFN_DEF_VSTOREA_HALF_AS(_rte)                          \
  __CLFN_DEF_VSTOREA_HALF_AS(_rtz)                          \
  __CLFN_DEF_VSTOREA_HALF_AS(_rtp)                          \
  __CLFN_DEF_VSTOREA_HALF_AS(_rtn)

//*****************************************************************************/
// vload/vstore HALF functions
//*****************************************************************************/
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
__CLFN_DEF_F_VLOAD_SCALAR_HALF(__generic,  4)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
__CLFN_DEF_F_VLOAD_SCALAR_HALF(__global,   1)
__CLFN_DEF_F_VLOAD_SCALAR_HALF(__local,    3)
__CLFN_DEF_F_VLOAD_SCALAR_HALF(__constant, 2)
__CLFN_DEF_F_VLOAD_SCALAR_HALF(__private,  0)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
__CLFN_DEF_VSTORE_SCALAR_HALF(__generic, 4)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
__CLFN_DEF_VSTORE_SCALAR_HALF(__global,  1)
__CLFN_DEF_VSTORE_SCALAR_HALF(__local,   3)
__CLFN_DEF_VSTORE_SCALAR_HALF(__private, 0)
//*****************************************************************************/
// vload/vstore HALF functions
//*****************************************************************************/
__CLFN_DEF_F_VLOAD_HALF_ALL()
__CLFN_DEF_F_VLOADA_HALF_ALL()

__CLFN_DEF_VSTORE_HALF_ALL()
__CLFN_DEF_VSTOREA_HALF_ALL()

