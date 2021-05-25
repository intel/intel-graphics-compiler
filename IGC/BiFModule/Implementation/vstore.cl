/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file defines SPIRV vstoren, vstore_half, vstore_half_r, vstore_halfn,
// vstore_halfn_r, vstorea_halfn and vstorea_halfn_r built-ins.
//

//*****************************************************************************/
// helper functions
//*****************************************************************************/

static OVERLOADABLE half __intel_spirv_float2half(float f, RoundingMode_t r)
{
    switch (r)
    {
    case rte:
        return SPIRV_BUILTIN(FConvert, _RTE_f16_f32, _Rhalf_rte)(f);
    case rtz:
        return SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _Rhalf_rtz)(f);
    case rtp:
        return SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _Rhalf_rtp)(f);
    case rtn:
        return SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _Rhalf_rtn)(f);
    }
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VS_NO_MANG(__intel_spirv_float2half, half, float, RoundingMode_t)

#if defined(cl_khr_fp64)

static OVERLOADABLE half __intel_spirv_double2half(double d, RoundingMode_t r)
{
    switch (r)
    {
    case rte:
        return SPIRV_BUILTIN(FConvert, _RTE_f16_f64, _Rhalf_rte)(d);
    case rtz:
        return SPIRV_BUILTIN(FConvert, _RTZ_f16_f64, _Rhalf_rtz)(d);
    case rtp:
        return SPIRV_BUILTIN(FConvert, _RTP_f16_f64, _Rhalf_rtp)(d);
    case rtn:
        return SPIRV_BUILTIN(FConvert, _RTN_f16_f64, _Rhalf_rtn)(d);
    }
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VS_NO_MANG(__intel_spirv_double2half, half, double, RoundingMode_t)

#endif //defined(cl_khr_fp64)

//*****************************************************************************/
// vstoren
// "Writes n components from the data vector value to the address computed as (p + (offset * n)),
//  where n is equal to the component count of the vector data."
//*****************************************************************************/

#define VSTOREN_DEF(addressSpace, scalarType, numElements, offsetType, mangle)                              \
INLINE void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(vstore, numElements##_##mangle, n)(                        \
    scalarType##numElements data, offsetType offset, addressSpace scalarType *p) {                          \
    addressSpace scalarType *pOffset = p + offset * numElements;                                            \
    scalarType##numElements ret = data;                                                                     \
    __builtin_IB_memcpy_private_to_##addressSpace((addressSpace uchar *)pOffset, (private uchar *)&ret,     \
                                                  sizeof(scalarType) * numElements, sizeof(scalarType));    \
}

#define VSTOREN_AS(addressSpace, scalarType, typemang, mang)                \
VSTOREN_DEF(addressSpace, scalarType, 2,  long, v2##typemang##_i64_##mang)  \
VSTOREN_DEF(addressSpace, scalarType, 2,  int,  v2##typemang##_i32_##mang)  \
VSTOREN_DEF(addressSpace, scalarType, 3,  long, v3##typemang##_i64_##mang)  \
VSTOREN_DEF(addressSpace, scalarType, 3,  int,  v3##typemang##_i32_##mang)  \
VSTOREN_DEF(addressSpace, scalarType, 4,  long, v4##typemang##_i64_##mang)  \
VSTOREN_DEF(addressSpace, scalarType, 4,  int,  v4##typemang##_i32_##mang)  \
VSTOREN_DEF(addressSpace, scalarType, 8,  long, v8##typemang##_i64_##mang)  \
VSTOREN_DEF(addressSpace, scalarType, 8,  int,  v8##typemang##_i32_##mang)  \
VSTOREN_DEF(addressSpace, scalarType, 16, long, v16##typemang##_i64_##mang) \
VSTOREN_DEF(addressSpace, scalarType, 16, int,  v16##typemang##_i32_##mang)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define VSTOREN_TYPE(TYPE, TYPEMANG)               \
VSTOREN_AS(global,   TYPE, TYPEMANG, p1##TYPEMANG) \
VSTOREN_AS(local,    TYPE, TYPEMANG, p3##TYPEMANG) \
VSTOREN_AS(private,  TYPE, TYPEMANG, p0##TYPEMANG) \
VSTOREN_AS(generic,  TYPE, TYPEMANG, p4##TYPEMANG)
#else
#define VSTOREN_TYPE(TYPE, TYPEMANG)               \
VSTOREN_AS(global,   TYPE, TYPEMANG, p1##TYPEMANG) \
VSTOREN_AS(local,    TYPE, TYPEMANG, p3##TYPEMANG) \
VSTOREN_AS(private,  TYPE, TYPEMANG, p0##TYPEMANG)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

VSTOREN_TYPE(char,   i8)
VSTOREN_TYPE(short,  i16)
VSTOREN_TYPE(int,    i32)
VSTOREN_TYPE(long,   i64)
VSTOREN_TYPE(half,   f16)
VSTOREN_TYPE(float,  f32)
#if defined(cl_khr_fp64)
VSTOREN_TYPE(double, f64)
#endif

//*****************************************************************************/
// vstore_half
// "Converts the data float or double value to a half value using the default rounding mode
//  and writes the half value to the address computed as (p + offset)."
// vstore_half_r
// "Converts the data float or double value to a half value using the specified rounding mode
//  mode and writes the half value to the address computed as (p + offset)."
//*****************************************************************************/

#define VSTORE_HALF_DEF(addressSpace, ASNUM, MANGSIZE, SIZETYPE, MANGSRC, srcType)                               \
INLINE void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(vstore_half, _##MANGSRC##_##MANGSIZE##_p##ASNUM##f16, )(        \
    srcType data, SIZETYPE offset, addressSpace half * p) {                                                      \
    addressSpace half *pHalf = (addressSpace half *)(p + offset);                                                \
    *pHalf = __intel_spirv_##srcType##2half(data, rte);                                                          \
}

#define VSTORE_HALF_R_DEF(addressSpace, ASNUM, MANGSIZE, SIZETYPE, MANGSRC, srcType)                             \
INLINE void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(vstore_half_r, _##MANGSRC##_##MANGSIZE##_p##ASNUM##f16_i32, )(  \
    srcType data, SIZETYPE offset, addressSpace half * p, RoundingMode_t r) {                                    \
    addressSpace half *pHalf = (addressSpace half *)(p + offset);                                                \
    *pHalf = __intel_spirv_##srcType##2half(data, r);                                                            \
}

#define VSTORE_HALF_BOTH(addressSpace, ASNUM, MANGSIZE, SIZETYPE, MANGSRC, srcType)  \
VSTORE_HALF_DEF(addressSpace, ASNUM, MANGSIZE, SIZETYPE, MANGSRC, srcType)           \
VSTORE_HALF_R_DEF(addressSpace, ASNUM, MANGSIZE, SIZETYPE, MANGSRC, srcType)

#if defined(cl_khr_fp64)
#define VSTORE_HALF_ALL_TYPES(addressSpace, ASNUM)              \
VSTORE_HALF_BOTH(addressSpace, ASNUM, i64, long, f32, float)    \
VSTORE_HALF_BOTH(addressSpace, ASNUM, i64, long, f64, double)   \
VSTORE_HALF_BOTH(addressSpace, ASNUM, i32, int,  f32, float)    \
VSTORE_HALF_BOTH(addressSpace, ASNUM, i32, int,  f64, double)
#else
#define VSTORE_HALF_ALL_TYPES(addressSpace, ASNUM)              \
VSTORE_HALF_BOTH(addressSpace, ASNUM, i64, long, f32, float)    \
VSTORE_HALF_BOTH(addressSpace, ASNUM, i32, int,  f32, float)
#endif

VSTORE_HALF_ALL_TYPES(private, 0)
VSTORE_HALF_ALL_TYPES(global,  1)
VSTORE_HALF_ALL_TYPES(local,   3)
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
VSTORE_HALF_ALL_TYPES(generic, 4)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

//*****************************************************************************/
// vstore_halfn
// "Converts the data vector of float or vector of double values to a vector of half values
//  using the default rounding mode and writes the half values to memory."
// vstore_halfn_r
// "Converts the data vector of float or vector of double values to a vector of half values
//  using the specified rounding mode mode and writes the half values to memory."
//*****************************************************************************/

#define VSTORE_HALFN_DEF(addressSpace, ASNUM, MANGSIZE, SIZETYPE, MANGSRC, srcType, numElements)              \
INLINE void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(                                                             \
    vstore_half, numElements##_v##numElements##MANGSRC##_##MANGSIZE##_p##ASNUM##f16,                          \
    n)(srcType##numElements data, SIZETYPE offset, addressSpace half * p) {                                   \
    SPIRV_OCL_BUILTIN(vstore, numElements##_v##numElements##f16##_##MANGSIZE##_p##ASNUM##f16, n)              \
    (__intel_spirv_##srcType##2half(data, rte), offset, p);                                                   \
}

#define VSTORE_HALFN_R_DEF(addressSpace, ASNUM, MANGSIZE, SIZETYPE, MANGSRC, srcType, numElements)            \
INLINE void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(                                                             \
    vstore_half, numElements##_r_v##numElements##MANGSRC##_##MANGSIZE##_p##ASNUM##f16_i32,                    \
    n_r)(srcType##numElements data, SIZETYPE offset, addressSpace half * p, RoundingMode_t r) {               \
    SPIRV_OCL_BUILTIN(vstore, numElements##_v##numElements##f16##_##MANGSIZE##_p##ASNUM##f16, n)              \
    (__intel_spirv_##srcType##2half(data, r), offset, p);                                                     \
}

#define VSTORE_HALFN_BOTH(addressSpace, ASNUM, MANGSIZE, SIZETYPE, MANGSRC, srcType, numElements)  \
VSTORE_HALFN_DEF(addressSpace, ASNUM, MANGSIZE, SIZETYPE, MANGSRC, srcType, numElements)           \
VSTORE_HALFN_R_DEF(addressSpace, ASNUM, MANGSIZE, SIZETYPE, MANGSRC, srcType, numElements)


#if defined(cl_khr_fp64)
#define VSTORE_HALFN_ALL_TYPES(addressSpace, ASNUM)                 \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float,  2)   \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float,  3)   \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float,  4)   \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float,  8)   \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float,  16)  \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i64, long, f64, double, 2)   \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i64, long, f64, double, 3)   \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i64, long, f64, double, 4)   \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i64, long, f64, double, 8)   \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i64, long, f64, double, 16)  \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float,  2)   \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float,  3)   \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float,  4)   \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float,  8)   \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float,  16)  \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f64, double, 2)   \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f64, double, 3)   \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f64, double, 4)   \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f64, double, 8)   \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f64, double, 16)
#else
#define VSTORE_HALFN_ALL_TYPES(addressSpace, ASNUM)                 \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float, 2)    \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float, 3)    \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float, 4)    \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float, 8)    \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float, 16)   \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float, 2)    \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float, 3)    \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float, 4)    \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float, 8)    \
VSTORE_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float, 16)
#endif //defined(cl_khr_fp64)

VSTORE_HALFN_ALL_TYPES(private, 0)
VSTORE_HALFN_ALL_TYPES(global,  1)
VSTORE_HALFN_ALL_TYPES(local,   3)
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
VSTORE_HALFN_ALL_TYPES(generic, 4)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

//*****************************************************************************/
// vstorea_halfn
// "Converts the data vector of float or vector of double values to a vector of half values using the default rounding mode,
//  and then writes the converted vector of half values to aligned memory."
// vstorea_halfn_r
// "Converts the data vector of float or vector of double values to a vector of half values using the specified rounding mode mode,
//  and then write the converted vector of half values to aligned memory."
//*****************************************************************************/

#define VSTOREA_HALFN_DEF(addressSpace, ASNUM, MANGSIZE, SIZETYPE, MANGSRC, srcType, step, numElements)       \
INLINE void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(                                                             \
    vstorea_half, numElements##_v##numElements##MANGSRC##_##MANGSIZE##_p##ASNUM##f16,                         \
    n)(srcType##numElements data, SIZETYPE offset, addressSpace half * p) {                                   \
    addressSpace half##numElements *pHalf = (addressSpace half##numElements *)(p + offset * step);            \
    *pHalf = __intel_spirv_##srcType##2half(data, rte);                                                       \
}

#define VSTOREA_HALFN_R_DEF(addressSpace, ASNUM, MANGSIZE, SIZETYPE, MANGSRC, srcType, step, numElements)     \
INLINE void SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(                                                             \
    vstorea_half, numElements##_r_v##numElements##MANGSRC##_##MANGSIZE##_p##ASNUM##f16_i32,                   \
    n_r)(srcType##numElements data, SIZETYPE offset, addressSpace half * p, RoundingMode_t r) {               \
    addressSpace half##numElements *pHalf = (addressSpace half##numElements *)(p + offset * step);            \
    *pHalf = __intel_spirv_##srcType##2half(data, r);                                                         \
}

#define VSTOREA_HALFN_BOTH(addressSpace, ASNUM, MANGSIZE, SIZETYPE, MANGSRC, srcType, step, numElements) \
VSTOREA_HALFN_DEF(addressSpace, ASNUM, MANGSIZE, SIZETYPE, MANGSRC, srcType, step, numElements)          \
VSTOREA_HALFN_R_DEF(addressSpace, ASNUM, MANGSIZE, SIZETYPE, MANGSRC, srcType, step, numElements)

#if defined(cl_khr_fp64)
#define VSTOREA_HALFN_ALL_TYPES(addressSpace, ASNUM)                      \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float,  2,  2)    \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float,  4,  3)    \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float,  4,  4)    \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float,  8,  8)    \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float,  16, 16)   \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i64, long, f64, double, 2,  2)    \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i64, long, f64, double, 4,  3)    \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i64, long, f64, double, 4,  4)    \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i64, long, f64, double, 8,  8)    \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i64, long, f64, double, 16, 16)   \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float,  2,  2)    \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float,  4,  3)    \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float,  4,  4)    \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float,  8,  8)    \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float,  16, 16)   \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f64, double, 2,  2)    \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f64, double, 4,  3)    \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f64, double, 4,  4)    \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f64, double, 8,  8)    \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f64, double, 16, 16)
#else
#define VSTOREA_HALFN_ALL_TYPES(addressSpace, ASNUM)                      \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float, 2,  2)     \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float, 4,  3)     \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float, 4,  4)     \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float, 8,  8)     \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i64, long, f32, float, 16, 16)    \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float, 2,  2)     \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float, 4,  3)     \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float, 4,  4)     \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float, 8,  8)     \
VSTOREA_HALFN_BOTH(addressSpace, ASNUM, i32, int,  f32, float, 16, 16)
#endif //defined(cl_khr_fp64)

VSTOREA_HALFN_ALL_TYPES(private, 0)
VSTOREA_HALFN_ALL_TYPES(global,  1)
VSTOREA_HALFN_ALL_TYPES(local,   3)
#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
VSTOREA_HALFN_ALL_TYPES(generic, 4)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
