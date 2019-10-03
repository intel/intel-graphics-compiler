/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

//
// This file defines SPIRV vload/vstore built-ins.
//
//===----------------------------------------------------------------------===//

//*****************************************************************************/
// vload/vstore macros
//*****************************************************************************/

#define VLOAD_MACRO(addressSpace, scalarType, numElements, offsetType, mangle)                                                                                \
INLINE scalarType##numElements __builtin_spirv_OpenCL_vload##numElements##_##mangle(offsetType offset, const addressSpace scalarType *p)                             \
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
VLOAD_MACRO(addressSpace, scalarType, 2,  ulong, i64_##mang) \
VLOAD_MACRO(addressSpace, scalarType, 2,  uint,  i32_##mang) \
VLOAD_MACRO(addressSpace, scalarType, 3,  ulong, i64_##mang) \
VLOAD_MACRO(addressSpace, scalarType, 3,  uint,  i32_##mang) \
VLOAD_MACRO(addressSpace, scalarType, 4,  ulong, i64_##mang) \
VLOAD_MACRO(addressSpace, scalarType, 4,  uint,  i32_##mang) \
VLOAD_MACRO(addressSpace, scalarType, 8,  ulong, i64_##mang) \
VLOAD_MACRO(addressSpace, scalarType, 8,  uint,  i32_##mang) \
VLOAD_MACRO(addressSpace, scalarType, 16, ulong, i64_##mang) \
VLOAD_MACRO(addressSpace, scalarType, 16, uint,  i32_##mang)

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
static OVERLOADABLE float __intel_spirv_half2float(ushort h)
{
    return __builtin_spirv_OpFConvert_f32_f16(as_half(h));
}

#define VLOAD_SHORT(addressSpace, ASNUM)                                                                 \
INLINE static ushort __builtin_spirv_OpenCL_vload_i64_p##ASNUM##i16(ulong offset, const addressSpace ushort* p) \
{                                                                                                        \
    return *(p + offset);                                                                                \
}                                                                                                        \
INLINE static ushort __builtin_spirv_OpenCL_vload_i32_p##ASNUM##i16(uint offset, const addressSpace ushort* p)  \
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

GENERATE_VECTOR_FUNCTIONS_1ARG_NO_MANG(__intel_spirv_half2float, float, ushort)

// Two copies for the i32 and i64 size_t offsets.
#define __CLFN_DEF_F_VLOAD_SCALAR_HALF(addressSpace, ASNUM)                                      \
INLINE half __builtin_spirv_OpenCL_vload_i32_p##ASNUM##f16(uint offset, const addressSpace half* p) {   \
  return *(p + offset);                                                                          \
}                                                                                                \
INLINE half __builtin_spirv_OpenCL_vload_i64_p##ASNUM##f16(ulong offset, const addressSpace half* p) {  \
  return *(p + offset);                                                                          \
}

#define __CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, MANGSIZE, SIZETYPE, numElements)                                                        \
INLINE float##numElements __builtin_spirv_OpenCL_vload_half##numElements##_##MANGSIZE##_p##ASNUM##f16(SIZETYPE offset, const addressSpace half* p) { \
  return __intel_spirv_half2float(__builtin_spirv_OpenCL_vload##numElements##_##MANGSIZE##_p##ASNUM##i16(offset, (const addressSpace ushort*)p));          \
}

#define __CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, MANGSIZE, SIZETYPE, step, numElements)                                                 \
INLINE float##numElements __builtin_spirv_OpenCL_vloada_half##numElements##_##MANGSIZE##_p##ASNUM##f16(SIZETYPE offset, const addressSpace half* p) {  \
  const addressSpace ushort##numElements* pHalf = (const addressSpace ushort##numElements*)(p + offset * step);                               \
  return __intel_spirv_half2float(*pHalf);                                                                                                          \
}

#define __CLFN_DEF_F_VLOAD_HALFX_AS(addressSpace, ASNUM)             \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i64, ulong, )          \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i64, ulong, 2)         \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i64, ulong, 3)         \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i64, ulong, 4)         \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i64, ulong, 8)         \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i64, ulong, 16)        \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i32, uint, )           \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i32, uint, 2)          \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i32, uint, 3)          \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i32, uint, 4)          \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i32, uint, 8)          \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, ASNUM, i32, uint, 16)

#define __CLFN_DEF_F_VLOADA_HALFX_AS(addressSpace, ASNUM)           \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i64, ulong, 1, )     \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i64, ulong, 2, 2)    \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i64, ulong, 4, 3)    \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i64, ulong, 4, 4)    \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i64, ulong, 8, 8)    \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i64, ulong, 16, 16)  \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i32, uint, 1, )      \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i32, uint, 2, 2)     \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i32, uint, 4, 3)     \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i32, uint, 4, 4)     \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i32, uint, 8, 8)     \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, ASNUM, i32, uint, 16, 16)

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
    return __builtin_spirv_OpFConvert_RTZ_f16_f32(f);
}

static OVERLOADABLE half __intel_spirv_float2half_rte(float f)
{
    return __builtin_spirv_OpFConvert_RTE_f16_f32(f);
}

static OVERLOADABLE half __intel_spirv_float2half_rtp(float f)
{
    return __builtin_spirv_OpFConvert_RTP_f16_f32(f);
}

static OVERLOADABLE half __intel_spirv_float2half_rtn(float f)
{
    return __builtin_spirv_OpFConvert_RTN_f16_f32(f);
}

static OVERLOADABLE half __intel_spirv_float2half(float f)
{
    return __intel_spirv_float2half_rte(f);
}

#if defined(cl_khr_fp64)

static OVERLOADABLE half __intel_spirv_double2half_rtz(double a)
{
    return __builtin_spirv_OpFConvert_RTZ_f16_f64(a);
}

static OVERLOADABLE half __intel_spirv_double2half_rte(double a)
{
    return __builtin_spirv_OpFConvert_RTE_f16_f64(a);
}

static OVERLOADABLE half __intel_spirv_double2half_rtp(double a)
{
    return __builtin_spirv_OpFConvert_RTP_f16_f64(a);
}

static OVERLOADABLE half __intel_spirv_double2half_rtn(double a)
{
    return __builtin_spirv_OpFConvert_RTN_f16_f64(a);
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

