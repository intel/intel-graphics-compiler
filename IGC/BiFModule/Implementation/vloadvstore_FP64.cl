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
TYPE_ARG(double, f64)

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
TYPE_ARG(double, f64)

#undef TYPE_ARG
#undef ELEM_ARG

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

#define __CLFN_DEF_VSTORE_HALF(addressSpace, ASNUM, rnd)                                  \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, ,    rnd, f64, double, )        \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 2)       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 3)       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 4)       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 8)       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 16)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  ,    rnd, f64, double, )       \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 2)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 3)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 4)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 8)      \
  __CLFN_DEF_VSTORE_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 16)

  #define __CLFN_DEF_VSTOREA_HALF(addressSpace, ASNUM, rnd)                                     \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, ,    rnd, f64, double, 1, )        \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 2, 2)       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 4, 3)       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 4, 4)       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 8, 8)       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i64, ulong, v,   rnd, f64, double, 16, 16)     \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  ,    rnd, f64, double, 1, )       \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 2, 2)      \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 4, 3)      \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 4, 4)      \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 8, 8)      \
  __CLFN_DEF_VSTOREA_HALFX(addressSpace, ASNUM, i32, uint,  v,   rnd, f64, double, 16, 16)

  #if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
__CLFN_DEF_VSTORE_SCALAR_HALF(__generic, 4)
#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0
__CLFN_DEF_VSTORE_SCALAR_HALF(__global,  1)
__CLFN_DEF_VSTORE_SCALAR_HALF(__local,   3)
__CLFN_DEF_VSTORE_SCALAR_HALF(__private, 0)

__CLFN_DEF_VSTORE_HALF_ALL()
__CLFN_DEF_VSTOREA_HALF_ALL()
