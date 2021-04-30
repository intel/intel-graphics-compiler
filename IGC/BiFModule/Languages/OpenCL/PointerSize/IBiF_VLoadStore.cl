/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===- IBiF_VLoadStore.cl - OpenCL explicit conversion functions -====//
//
// This file defines OpenCL vload/vstore built-ins.
//
//===----------------------------------------------------------------------===//

//*****************************************************************************/
// vload/vstore macros
//*****************************************************************************/

#define VLOAD_MACRO(addressSpace, scalarType, numElements)                                                                                                    \
INLINE scalarType##numElements OVERLOADABLE vload##numElements(size_t offset, const addressSpace scalarType *p)                                               \
{                                                                                                                                                             \
  const addressSpace scalarType *pOffset = p + offset * numElements;                                                                                          \
  scalarType##numElements ret;                                                                                                                                \
  __builtin_IB_memcpy_##addressSpace##_to_private((private uchar*)&ret, (addressSpace uchar*)pOffset, sizeof(scalarType) * numElements, sizeof(scalarType)); \
  return ret;                                                                                                                                                 \
}

#define VLOAD3_MACRO(addressSpace, scalarType, numElements)                                                              \
INLINE scalarType##numElements OVERLOADABLE vload##numElements(size_t offset, const addressSpace scalarType *p)                                               \
{                                                                                                                                                             \
  const addressSpace scalarType *pOffset = p + offset * numElements;                                                                                          \
  scalarType##numElements ret;                                                                                                                                \
  __builtin_IB_memcpy_##addressSpace##_to_private((private uchar*)&ret, (addressSpace uchar*)pOffset, sizeof(scalarType) * numElements, sizeof(scalarType)); \
  return ret;                                                                                                                                                 \
}

#define VSTORE_MACRO(addressSpace, scalarType, numElements)                                                                                                   \
INLINE void OVERLOADABLE vstore##numElements(scalarType##numElements data, size_t offset, addressSpace scalarType *p)                                         \
{                                                                                                                                                             \
  addressSpace scalarType *pOffset = p + offset * numElements;                                                                                                \
  scalarType##numElements ret = data;                                                                                                                         \
  __builtin_IB_memcpy_private_to_##addressSpace((addressSpace uchar*)pOffset, (private uchar*)&ret, sizeof(scalarType) * numElements, sizeof(scalarType));   \
}

#define VSTORE3_MACRO(addressSpace, scalarType, numElements)                                                             \
INLINE void OVERLOADABLE vstore##numElements(scalarType##numElements data, size_t offset, addressSpace scalarType *p)                                         \
{                                                                                                                                                             \
  addressSpace scalarType *pOffset = p + offset * numElements;                                                                                                \
  scalarType##numElements ret = data;                                                                                                                         \
  __builtin_IB_memcpy_private_to_##addressSpace((addressSpace uchar*)pOffset, (private uchar*)&ret, sizeof(scalarType) * numElements, sizeof(scalarType));   \
}

#define ELEM_ARG(addressSpace, scalarType) \
VLOAD_MACRO(addressSpace, scalarType, 2)   \
VLOAD3_MACRO(addressSpace, scalarType, 3)   \
VLOAD_MACRO(addressSpace, scalarType, 4)   \
VLOAD_MACRO(addressSpace, scalarType, 8)   \
VLOAD_MACRO(addressSpace, scalarType, 16)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define TYPE_ARG(TYPE)   \
ELEM_ARG(global, TYPE)   \
ELEM_ARG(constant, TYPE) \
ELEM_ARG(local, TYPE)    \
ELEM_ARG(private, TYPE)  \
ELEM_ARG(generic, TYPE)
#else
#define TYPE_ARG(TYPE)   \
ELEM_ARG(global, TYPE)   \
ELEM_ARG(constant, TYPE) \
ELEM_ARG(local, TYPE)    \
ELEM_ARG(private, TYPE)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

TYPE_ARG(char)
TYPE_ARG(uchar)
TYPE_ARG(short)
TYPE_ARG(ushort)
TYPE_ARG(int)
TYPE_ARG(uint)
TYPE_ARG(long)
TYPE_ARG(ulong)
#if defined(cl_khr_fp16)
TYPE_ARG(half)
#endif
TYPE_ARG(float)
#if defined(cl_khr_fp64)
TYPE_ARG(double)
#endif

#undef TYPE_ARG
#undef ELEM_ARG

#define ELEM_ARG(addressSpace, scalarType) \
VSTORE_MACRO(addressSpace, scalarType, 2)  \
VSTORE3_MACRO(addressSpace, scalarType, 3)  \
VSTORE_MACRO(addressSpace, scalarType, 4)  \
VSTORE_MACRO(addressSpace, scalarType, 8)  \
VSTORE_MACRO(addressSpace, scalarType, 16)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define TYPE_ARG(TYPE)   \
ELEM_ARG(global, TYPE)   \
ELEM_ARG(local, TYPE)    \
ELEM_ARG(private, TYPE)  \
ELEM_ARG(generic, TYPE)
#else
#define TYPE_ARG(TYPE)   \
ELEM_ARG(global, TYPE)   \
ELEM_ARG(local, TYPE)    \
ELEM_ARG(private, TYPE)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

TYPE_ARG(char)
TYPE_ARG(uchar)
TYPE_ARG(short)
TYPE_ARG(ushort)
TYPE_ARG(int)
TYPE_ARG(uint)
TYPE_ARG(long)
TYPE_ARG(ulong)
#if defined(cl_khr_fp16)
TYPE_ARG(half)
#endif
TYPE_ARG(float)
#if defined(cl_khr_fp64)
TYPE_ARG(double)
#endif

#undef TYPE_ARG
#undef ELEM_ARG

//*****************************************************************************/
// vload macros
//*****************************************************************************/
static OVERLOADABLE float __intel_half2float(ushort h)
{
    return convert_float(as_half(h));
}

#define VLOAD_SHORT(addressSpace)                                             \
static OVERLOADABLE ushort vload(size_t offset, const addressSpace ushort* p) \
{                                                                             \
    return *(p + offset);                                                     \
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
VLOAD_SHORT(__generic)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
VLOAD_SHORT(__global)
VLOAD_SHORT(__local)
VLOAD_SHORT(__constant)
VLOAD_SHORT(__private)

GENERATE_VECTOR_FUNCTIONS_1ARG(__intel_half2float, float, ushort)

#define __CLFN_DEF_F_VLOAD_SCALAR_HALF(addressSpace)                        \
INLINE half OVERLOADABLE vload(size_t offset, const addressSpace half* p) { \
  return *(p + offset);                                                     \
}

#define __CLFN_DEF_F_VLOAD_HALFX(addressSpace, numElements)                                                 \
INLINE float##numElements OVERLOADABLE vload_half##numElements(size_t offset, const addressSpace half* p) { \
  return __intel_half2float(vload##numElements(offset, (const addressSpace ushort*)p));                     \
}

#define __CLFN_DEF_F_VLOADA_HALFX(addressSpace, step, numElements)                                              \
INLINE float##numElements OVERLOADABLE vloada_half##numElements(size_t offset, const addressSpace half* p) {    \
  const addressSpace ushort##numElements* pHalf = (const addressSpace ushort##numElements*)(p + offset * step); \
  return __intel_half2float(*pHalf);                                                                            \
}

#define __CLFN_DEF_F_VLOAD_HALFX_AS(addressSpace) \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, )          \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, 2)         \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, 3)         \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, 4)         \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, 8)         \
__CLFN_DEF_F_VLOAD_HALFX(addressSpace, 16)

#define __CLFN_DEF_F_VLOADA_HALFX_AS(addressSpace)    \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, 1, )          \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, 2, 2)         \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, 4, 3)         \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, 4, 4)         \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, 8, 8)         \
__CLFN_DEF_F_VLOADA_HALFX(addressSpace, 16, 16)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define __CLFN_DEF_F_VLOAD_HALF_ALL()              \
__CLFN_DEF_F_VLOAD_HALFX_AS(__generic)             \
__CLFN_DEF_F_VLOAD_HALFX_AS(__global)              \
__CLFN_DEF_F_VLOAD_HALFX_AS(__local)               \
__CLFN_DEF_F_VLOAD_HALFX_AS(__constant)            \
__CLFN_DEF_F_VLOAD_HALFX_AS(__private)
#else
#define __CLFN_DEF_F_VLOAD_HALF_ALL()              \
__CLFN_DEF_F_VLOAD_HALFX_AS(__global)              \
__CLFN_DEF_F_VLOAD_HALFX_AS(__local)               \
__CLFN_DEF_F_VLOAD_HALFX_AS(__constant)            \
__CLFN_DEF_F_VLOAD_HALFX_AS(__private)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define __CLFN_DEF_F_VLOADA_HALF_ALL()              \
__CLFN_DEF_F_VLOADA_HALFX_AS(__generic)             \
__CLFN_DEF_F_VLOADA_HALFX_AS(__global)              \
__CLFN_DEF_F_VLOADA_HALFX_AS(__local)               \
__CLFN_DEF_F_VLOADA_HALFX_AS(__constant)            \
__CLFN_DEF_F_VLOADA_HALFX_AS(__private)
#else
#define __CLFN_DEF_F_VLOADA_HALF_ALL()              \
__CLFN_DEF_F_VLOADA_HALFX_AS(__global)              \
__CLFN_DEF_F_VLOADA_HALFX_AS(__local)               \
__CLFN_DEF_F_VLOADA_HALFX_AS(__constant)            \
__CLFN_DEF_F_VLOADA_HALFX_AS(__private)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

//*****************************************************************************/
// vstore macros (half)
//*****************************************************************************/
// These helper functions are used to macro-ize the rounding mode built-ins.
static OVERLOADABLE half __intel_float2half_rtz(float f)
{
    return convert_half_rtz(f);
}

static OVERLOADABLE half __intel_float2half_rte(float f)
{
    return convert_half_rte(f);
}

static OVERLOADABLE half __intel_float2half_rtp(float f)
{
    return convert_half_rtp(f);
}

static OVERLOADABLE half __intel_float2half_rtn(float f)
{
    return convert_half_rtn(f);
}

static OVERLOADABLE half __intel_float2half(float f)
{
    return __intel_float2half_rte(f);
}

#if defined(cl_khr_fp64)
static OVERLOADABLE half __intel_double2half_rtz(double a)
{
    return convert_half_rtz(a);
}

static OVERLOADABLE half __intel_double2half_rte(double a)
{
    return convert_half_rte(a);
}

static OVERLOADABLE half __intel_double2half_rtp(double a)
{
    return convert_half_rtp(a);
}

static OVERLOADABLE half __intel_double2half_rtn(double a)
{
    return convert_half_rtn(a);
}

static OVERLOADABLE half __intel_double2half(double a)
{
    return __intel_double2half_rte(a);
}

GENERATE_VECTOR_FUNCTIONS_1ARG(__intel_double2half_rtz, half, double)
GENERATE_VECTOR_FUNCTIONS_1ARG(__intel_double2half_rte, half, double)
GENERATE_VECTOR_FUNCTIONS_1ARG(__intel_double2half_rtp, half, double)
GENERATE_VECTOR_FUNCTIONS_1ARG(__intel_double2half_rtn, half, double)
GENERATE_VECTOR_FUNCTIONS_1ARG(__intel_double2half,     half, double)
#endif

GENERATE_VECTOR_FUNCTIONS_1ARG(__intel_float2half_rtz, half, float)
GENERATE_VECTOR_FUNCTIONS_1ARG(__intel_float2half_rte, half, float)
GENERATE_VECTOR_FUNCTIONS_1ARG(__intel_float2half_rtp, half, float)
GENERATE_VECTOR_FUNCTIONS_1ARG(__intel_float2half_rtn, half, float)
GENERATE_VECTOR_FUNCTIONS_1ARG(__intel_float2half,     half, float)

#if defined(cl_khr_fp64)
#define __CLFN_DEF_D_VSTORE_HALFX(addressSpace, rnd, numElements)                \
INLINE void OVERLOADABLE vstore_half##numElements##rnd(double##numElements data, \
                                                       size_t offset,            \
                                                       addressSpace half* p) {   \
  vstore##numElements(__intel_double2half##rnd(data), offset, p);                \
}

#define __CLFN_DEF_D_VSTOREA_HALFX(addressSpace, rnd, step, numElements)                          \
INLINE void OVERLOADABLE vstorea_half##numElements##rnd(double##numElements data,                 \
                                                        size_t offset,                            \
                                                        addressSpace half* p) {                   \
  addressSpace half##numElements *pHalf = (addressSpace half##numElements *)(p + offset * step);  \
  *pHalf = __intel_double2half##rnd(data);                                                        \
}
#endif

#define __CLFN_DEF_VSTORE_SCALAR_HALF(addressSpace)              \
INLINE void OVERLOADABLE vstore(half data,                       \
                                size_t offset,                   \
                                addressSpace half* p) {          \
  addressSpace half *pHalf = (addressSpace half *)(p + offset);  \
  *pHalf = data;                                                 \
}

#define __CLFN_DEF_F_VSTORE_HALFX(addressSpace, rnd, numElements)               \
INLINE void OVERLOADABLE vstore_half##numElements##rnd(float##numElements data, \
                                                       size_t offset,           \
                                                       addressSpace half* p) {  \
  vstore##numElements(__intel_float2half##rnd(data), offset, p);                \
}

#define __CLFN_DEF_F_VSTOREA_HALFX(addressSpace, rnd, step, numElements)                          \
INLINE void OVERLOADABLE vstorea_half##numElements##rnd(float##numElements data,                  \
                                                        size_t offset,                            \
                                                        addressSpace half* p) {                   \
  addressSpace half##numElements *pHalf = (addressSpace half##numElements *)(p + offset * step);  \
  *pHalf = __intel_float2half##rnd(data);                                                         \
}

#if defined(cl_khr_fp64)
#define __CLFN_DEF_VSTORE_HALF(addressSpace, rnd)                  \
  __CLFN_DEF_F_VSTORE_HALFX(addressSpace, rnd, )                   \
  __CLFN_DEF_F_VSTORE_HALFX(addressSpace, rnd, 2)                  \
  __CLFN_DEF_F_VSTORE_HALFX(addressSpace, rnd, 3)                  \
  __CLFN_DEF_F_VSTORE_HALFX(addressSpace, rnd, 4)                  \
  __CLFN_DEF_F_VSTORE_HALFX(addressSpace, rnd, 8)                  \
  __CLFN_DEF_F_VSTORE_HALFX(addressSpace, rnd, 16)                 \
  __CLFN_DEF_D_VSTORE_HALFX(addressSpace, rnd, )                   \
  __CLFN_DEF_D_VSTORE_HALFX(addressSpace, rnd, 2)                  \
  __CLFN_DEF_D_VSTORE_HALFX(addressSpace, rnd, 3)                  \
  __CLFN_DEF_D_VSTORE_HALFX(addressSpace, rnd, 4)                  \
  __CLFN_DEF_D_VSTORE_HALFX(addressSpace, rnd, 8)                  \
  __CLFN_DEF_D_VSTORE_HALFX(addressSpace, rnd, 16)
#else
#define __CLFN_DEF_VSTORE_HALF(addressSpace, rnd)                  \
  __CLFN_DEF_F_VSTORE_HALFX(addressSpace, rnd, )                   \
  __CLFN_DEF_F_VSTORE_HALFX(addressSpace, rnd, 2)                  \
  __CLFN_DEF_F_VSTORE_HALFX(addressSpace, rnd, 3)                  \
  __CLFN_DEF_F_VSTORE_HALFX(addressSpace, rnd, 4)                  \
  __CLFN_DEF_F_VSTORE_HALFX(addressSpace, rnd, 8)                  \
  __CLFN_DEF_F_VSTORE_HALFX(addressSpace, rnd, 16)
#endif


#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define __CLFN_DEF_VSTORE_HALF_AS(rnd)                    \
  __CLFN_DEF_VSTORE_HALF(generic, rnd)                    \
  __CLFN_DEF_VSTORE_HALF(global, rnd)                     \
  __CLFN_DEF_VSTORE_HALF(local, rnd)                      \
  __CLFN_DEF_VSTORE_HALF(private, rnd)
#else
#define __CLFN_DEF_VSTORE_HALF_AS(rnd)                    \
  __CLFN_DEF_VSTORE_HALF(global, rnd)                     \
  __CLFN_DEF_VSTORE_HALF(local, rnd)                      \
  __CLFN_DEF_VSTORE_HALF(private, rnd)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#define __CLFN_DEF_VSTORE_HALF_ALL()                          \
  __CLFN_DEF_VSTORE_HALF_AS()                                 \
  __CLFN_DEF_VSTORE_HALF_AS(_rte)                             \
  __CLFN_DEF_VSTORE_HALF_AS(_rtz)                             \
  __CLFN_DEF_VSTORE_HALF_AS(_rtp)                             \
  __CLFN_DEF_VSTORE_HALF_AS(_rtn)

#if defined(cl_khr_fp64)
#define __CLFN_DEF_VSTOREA_HALF(addressSpace, rnd)               \
  __CLFN_DEF_F_VSTOREA_HALFX(addressSpace, rnd, 1, )             \
  __CLFN_DEF_F_VSTOREA_HALFX(addressSpace, rnd, 2, 2)            \
  __CLFN_DEF_F_VSTOREA_HALFX(addressSpace, rnd, 4, 3)            \
  __CLFN_DEF_F_VSTOREA_HALFX(addressSpace, rnd, 4, 4)            \
  __CLFN_DEF_F_VSTOREA_HALFX(addressSpace, rnd, 8, 8)            \
  __CLFN_DEF_F_VSTOREA_HALFX(addressSpace, rnd, 16, 16)          \
  __CLFN_DEF_D_VSTOREA_HALFX(addressSpace, rnd, 1, )             \
  __CLFN_DEF_D_VSTOREA_HALFX(addressSpace, rnd, 2, 2)            \
  __CLFN_DEF_D_VSTOREA_HALFX(addressSpace, rnd, 4, 3)            \
  __CLFN_DEF_D_VSTOREA_HALFX(addressSpace, rnd, 4, 4)            \
  __CLFN_DEF_D_VSTOREA_HALFX(addressSpace, rnd, 8, 8)            \
  __CLFN_DEF_D_VSTOREA_HALFX(addressSpace, rnd, 16, 16)
#else
#define __CLFN_DEF_VSTOREA_HALF(addressSpace, rnd)               \
  __CLFN_DEF_F_VSTOREA_HALFX(addressSpace, rnd, 1, )             \
  __CLFN_DEF_F_VSTOREA_HALFX(addressSpace, rnd, 2, 2)            \
  __CLFN_DEF_F_VSTOREA_HALFX(addressSpace, rnd, 4, 3)            \
  __CLFN_DEF_F_VSTOREA_HALFX(addressSpace, rnd, 4, 4)            \
  __CLFN_DEF_F_VSTOREA_HALFX(addressSpace, rnd, 8, 8)            \
  __CLFN_DEF_F_VSTOREA_HALFX(addressSpace, rnd, 16, 16)
#endif

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
#define __CLFN_DEF_VSTOREA_HALF_AS(rnd)                 \
  __CLFN_DEF_VSTOREA_HALF(generic, rnd)                 \
  __CLFN_DEF_VSTOREA_HALF(global, rnd)                  \
  __CLFN_DEF_VSTOREA_HALF(local, rnd)                   \
  __CLFN_DEF_VSTOREA_HALF(private, rnd)
#else
#define __CLFN_DEF_VSTOREA_HALF_AS(rnd)                 \
__CLFN_DEF_VSTOREA_HALF(global, rnd)                    \
__CLFN_DEF_VSTOREA_HALF(local, rnd)                     \
__CLFN_DEF_VSTOREA_HALF(private, rnd)
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
__CLFN_DEF_F_VLOAD_SCALAR_HALF(__generic)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
__CLFN_DEF_F_VLOAD_SCALAR_HALF(__global)
__CLFN_DEF_F_VLOAD_SCALAR_HALF(__local)
__CLFN_DEF_F_VLOAD_SCALAR_HALF(__constant)
__CLFN_DEF_F_VLOAD_SCALAR_HALF(__private)

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
__CLFN_DEF_VSTORE_SCALAR_HALF(__generic)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
__CLFN_DEF_VSTORE_SCALAR_HALF(__global)
__CLFN_DEF_VSTORE_SCALAR_HALF(__local)
__CLFN_DEF_VSTORE_SCALAR_HALF(__private)
//*****************************************************************************/
// vload/vstore HALF functions
//*****************************************************************************/
__CLFN_DEF_F_VLOAD_HALF_ALL()
__CLFN_DEF_F_VLOADA_HALF_ALL()

__CLFN_DEF_VSTORE_HALF_ALL()
__CLFN_DEF_VSTOREA_HALF_ALL()
