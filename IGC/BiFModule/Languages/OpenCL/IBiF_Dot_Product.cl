/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifdef cl_khr_integer_dot_product
//===-  IBiF_Dot_Product.cl -===============================================//
//
// This file defines OpenCL dot product functions.
// These functions are part of the KHR cl_khr_integer_dot_product extension.
//
//===--------------------------------------------------------------------===//

// Supported Caps related to the cl_khr_integer_dot_product extension
#define __opencl_c_dot_product_signed
#define __opencl_c_dot_product_mixed_signedness
#define __opencl_c_integer_dot_product_saturation_accumulation
#define __opencl_c_integer_dot_product_input_4x8bit_packed

// Currently support CL_DEVICE_INTEGER_DOT_PRODUCT_INPUT_4x8BIT_KHR only

#define DEFN_INTEL_DOT_PRODUCT(TYPE_RET, TYPE_ARG1, TYPE_ARG2, TYPE_SUFFIX, MANGLING_OLD, MANGLING_NEW)     \
INLINE TYPE_RET OVERLOADABLE dot(TYPE_ARG1 a, TYPE_ARG2 b)                                                  \
{                                                                                                           \
    return SPIRV_BUILTIN(TYPE_SUFFIX##DotKHR, MANGLING_OLD, MANGLING_NEW)(a, b);                            \
}

#define DEFN_INTEL_DOT_PRODUCT_SAT(TYPE_RET, TYPE_ARG1, TYPE_ARG2, TYPE_SUFFIX, MANGLING_OLD, MANGLING_NEW)     \
INLINE TYPE_RET OVERLOADABLE dot_acc_sat(TYPE_ARG1 a, TYPE_ARG2 b, TYPE_RET acc)                                \
{                                                                                                               \
    return SPIRV_BUILTIN(TYPE_SUFFIX##DotAccSatKHR, MANGLING_OLD, MANGLING_NEW)(a, b, acc);                     \
}

#define DEFN_INTEL_DOT_PRODUCT_BUILTIN_SPIRV(TYPE_RET, TYPE_ARG1, TYPE_ARG2, TYPE_SUFFIX, MANGLING_OLD, MANGLING_NEW, TYPE_SUFFIX_IB)     \
TYPE_RET SPIRV_OVERLOADABLE SPIRV_BUILTIN(TYPE_SUFFIX##DotKHR, MANGLING_OLD, MANGLING_NEW)(TYPE_ARG1 a, TYPE_ARG2 b)                      \
{                                                                                                                                         \
    return __builtin_IB_dp4a_##TYPE_SUFFIX_IB(0, as_int(a), as_int(b));                                                               \
}

#define DEFN_INTEL_DOT_PRODUCT_SAT_BUILTIN_SPIRV(TYPE_RET, TYPE_ARG1, TYPE_ARG2, TYPE_SUFFIX, MANGLING_OLD, MANGLING_NEW, TYPE_SUFFIX_IB, SAT_PREFIX)     \
TYPE_RET SPIRV_OVERLOADABLE SPIRV_BUILTIN(TYPE_SUFFIX##DotAccSatKHR, MANGLING_OLD, MANGLING_NEW)(TYPE_ARG1 a, TYPE_ARG2 b, TYPE_RET acc)                  \
{                                                                                                                                                         \
    TYPE_RET product = __builtin_IB_dp4a_##TYPE_SUFFIX_IB(0, as_int(a), as_int(b));                                                                   \
    return SPIRV_OCL_BUILTIN(SAT_PREFIX##_add_sat, _i32_i32,)(product, acc);                                                                              \
}

#define DEFN_INTEL_DOT_PRODUCT_US(TYPE_RET, TYPE_ARG, MANGLING_OLD, MANGLING_NEW)     \
INLINE TYPE_RET OVERLOADABLE dot(u##TYPE_ARG a, TYPE_ARG b)                           \
{                                                                                     \
    return SPIRV_BUILTIN(SUDotKHR, MANGLING_OLD, MANGLING_NEW)(b, a);                 \
}

#define DEFN_INTEL_DOT_PRODUCT_SAT_US(TYPE_RET, TYPE_ARG, MANGLING_OLD, MANGLING_NEW)        \
INLINE TYPE_RET OVERLOADABLE dot_acc_sat(u##TYPE_ARG a, TYPE_ARG b, TYPE_RET acc)            \
{                                                                                            \
    return SPIRV_BUILTIN(SUDotAccSatKHR, MANGLING_OLD, MANGLING_NEW)(b, a, acc);             \
}

#define DEFN_INTEL_DOT_PRODUCT_PACKED_BUILTIN_SPIRV(TYPE_RET, TYPE_ARG1, TYPE_ARG2, TYPE_SUFFIX, MANGLING_OLD, MANGLING_NEW, TYPE_SUFFIX_IB)     \
TYPE_RET SPIRV_OVERLOADABLE SPIRV_BUILTIN(TYPE_SUFFIX##DotKHR, MANGLING_OLD, MANGLING_NEW)(TYPE_ARG1 a, TYPE_ARG2 b, TYPE_ARG1 packed)           \
{                                                                                                                                                \
    return __builtin_IB_dp4a_##TYPE_SUFFIX_IB(0, as_int(a), as_int(b));                                                                      \
}

#define DEFN_INTEL_DOT_PRODUCT_SAT_PACKED_BUILTIN_SPIRV(TYPE_RET, TYPE_ARG1, TYPE_ARG2, TYPE_SUFFIX, MANGLING_OLD, MANGLING_NEW, TYPE_SUFFIX_IB, SAT_PREFIX)     \
TYPE_RET SPIRV_OVERLOADABLE SPIRV_BUILTIN(TYPE_SUFFIX##DotAccSatKHR, MANGLING_OLD, MANGLING_NEW)(TYPE_ARG1 a, TYPE_ARG2 b, TYPE_RET acc, TYPE_ARG1 packed)       \
{                                                                                                                                                                \
    TYPE_RET product = __builtin_IB_dp4a_##TYPE_SUFFIX_IB(0, as_int(a), as_int(b));                                                                          \
    return SPIRV_OCL_BUILTIN(SAT_PREFIX##_add_sat, _i32_i32,)(product, acc);                                                                                     \
}

#define DEFN_INTEL_DOT_PRODUCT_PACKED(TYPE_RET, ARG_TYPES, TYPE_SUFFIX, MANGLING_OLD, MANGLING_NEW)     \
INLINE TYPE_RET OVERLOADABLE dot_4x8packed_##ARG_TYPES##_##TYPE_RET(uint a, uint b)                     \
{                                                                                                       \
    return SPIRV_BUILTIN(TYPE_SUFFIX##DotKHR, MANGLING_OLD, MANGLING_NEW)(a, b, 0);                     \
}

#define DEFN_INTEL_DOT_PRODUCT_SAT_PACKED(TYPE_RET, TYPE_SUFFIX, ARG_TYPES, MANGLING_OLD, MANGLING_NEW)        \
INLINE TYPE_RET OVERLOADABLE dot_acc_sat_4x8_packed_##ARG_TYPES##_##TYPE_RET(uint a, uint b, TYPE_RET acc)     \
{                                                                                                              \
    return SPIRV_BUILTIN(TYPE_SUFFIX##DotAccSatKHR, MANGLING_OLD, MANGLING_NEW)(a, b, acc, 0);                 \
}

#define DEFN_INTEL_DOT_PRODUCT_PACKED_US                              \
INLINE int OVERLOADABLE dot_4x8packed_us_int(uint a, uint b)          \
{                                                                     \
    return SPIRV_BUILTIN(SUDotKHR, _i32_i32_i32, _Rint)(b, a, 0);     \
}

#define DEFN_INTEL_DOT_PRODUCT_SAT_PACKED_US                                         \
INLINE int OVERLOADABLE dot_acc_sat_4x8packed_us_int(uint a, uint b, int acc)        \
{                                                                                    \
    return SPIRV_BUILTIN(SUDotAccSatKHR, _i32_i32_i32_i32, _Rint)(b, a, acc, 0);     \
}

DEFN_INTEL_DOT_PRODUCT_BUILTIN_SPIRV(uint, uchar4, uchar4, U, _v4i8_v4i8, _Ruint, uu)
DEFN_INTEL_DOT_PRODUCT(uint, uchar4, uchar4, U, _v4i8_v4i8, _Ruint)
DEFN_INTEL_DOT_PRODUCT_BUILTIN_SPIRV(uint, ushort2, ushort2, U, _v2i16_v2i16, _Ruint, uu)
DEFN_INTEL_DOT_PRODUCT(uint, ushort2, ushort2, U, _v2i16_v2i16, _Ruint)
#ifdef __opencl_c_integer_dot_product_input_4x8bit_packed
DEFN_INTEL_DOT_PRODUCT_PACKED_BUILTIN_SPIRV(uint, uint, uint, U, _i32_i32_i32, _Ruint, uu)
DEFN_INTEL_DOT_PRODUCT_PACKED(uint, uu, U, _i32_i32_i32, _Ruint)
#endif // __opencl_c_integer_dot_product_input_4x8bit_packed
#ifdef __opencl_c_integer_dot_product_saturation_accumulation
DEFN_INTEL_DOT_PRODUCT_SAT_BUILTIN_SPIRV(uint, uchar4, uchar4, U, _v4i8_v4i8_i32, _Ruint, uu, u)
DEFN_INTEL_DOT_PRODUCT_SAT(uint, uchar4, uchar4, U, _v4i8_v4i8_i32, _Ruint)
DEFN_INTEL_DOT_PRODUCT_SAT_BUILTIN_SPIRV(uint, ushort2, ushort2, U, _v2i16_v2i16_i32, _Ruint, uu, u)
DEFN_INTEL_DOT_PRODUCT_SAT(uint, ushort2, ushort2, U, _v2i16_v2i16_i32, _Ruint)
#ifdef __opencl_c_integer_dot_product_input_4x8bit_packed
DEFN_INTEL_DOT_PRODUCT_SAT_PACKED_BUILTIN_SPIRV(uint, uint, uint, U, _i32_i32_i32_i32, _Ruint, uu, u)
DEFN_INTEL_DOT_PRODUCT_SAT_PACKED(uint, U, uu, _i32_i32_i32_i32, _Ruint)
#endif // __opencl_c_integer_dot_product_input_4x8bit_packed
#endif // __opencl_c_integer_dot_product_saturation_accumulation

#ifdef __opencl_c_dot_product_signed
DEFN_INTEL_DOT_PRODUCT_BUILTIN_SPIRV(int, char4, char4, S, _v4i8_v4i8, _Rint, ss)
DEFN_INTEL_DOT_PRODUCT(int, char4, char4, S, _v4i8_v4i8, _Rint)
DEFN_INTEL_DOT_PRODUCT_BUILTIN_SPIRV(int, short2, short2, S, _v2i16_v2i16, _Rint, ss)
DEFN_INTEL_DOT_PRODUCT(int, short2, short2, S, _v2i16_v2i16, _Rint)
#ifdef __opencl_c_integer_dot_product_input_4x8bit_packed
DEFN_INTEL_DOT_PRODUCT_PACKED_BUILTIN_SPIRV(int, int, int, S, _i32_i32_i32, _Rint, ss)
DEFN_INTEL_DOT_PRODUCT_PACKED(int, ss, S, _i32_i32_i32, _Rint)
#endif // __opencl_c_integer_dot_product_input_4x8bit_packed
#ifdef __opencl_c_integer_dot_product_saturation_accumulation
DEFN_INTEL_DOT_PRODUCT_SAT_BUILTIN_SPIRV(int, char4, char4, S, _v4i8_v4i8_i32, _Rint, ss, s)
DEFN_INTEL_DOT_PRODUCT_SAT(int, char4, char4, S, _v4i8_v4i8_i32, _Rint)
DEFN_INTEL_DOT_PRODUCT_SAT_BUILTIN_SPIRV(int, short2, short2, S, _v2i16_v2i16_i32, _Rint, ss, s)
DEFN_INTEL_DOT_PRODUCT_SAT(int, short2, short2, S, _v2i16_v2i16_i32, _Rint)
#ifdef __opencl_c_integer_dot_product_input_4x8bit_packed
DEFN_INTEL_DOT_PRODUCT_SAT_PACKED_BUILTIN_SPIRV(int, int, int, S, _i32_i32_i32_i32, _Rint, ss, s)
DEFN_INTEL_DOT_PRODUCT_SAT_PACKED(int, S, ss, _i32_i32_i32_i32, _Rint)
#endif // __opencl_c_integer_dot_product_input_4x8bit_packed
#endif // __opencl_c_integer_dot_product_saturation_accumulation
#endif // __opencl_c_dot_product_signed

#ifdef __opencl_c_dot_product_mixed_signedness
DEFN_INTEL_DOT_PRODUCT_BUILTIN_SPIRV(int, char4, uchar4, SU, _v4i8_v4i8, _Rint, su)
DEFN_INTEL_DOT_PRODUCT(int, char4, uchar4, SU, _v4i8_v4i8, _Rint)
DEFN_INTEL_DOT_PRODUCT_US(int, char4, _v4i8_v4i8, _Rint)
DEFN_INTEL_DOT_PRODUCT_BUILTIN_SPIRV(int, short2, ushort2, SU, _v2i16_v2i16, _Rint, su)
DEFN_INTEL_DOT_PRODUCT(int, short2, ushort2, SU, _v2i16_v2i16, _Rint)
DEFN_INTEL_DOT_PRODUCT_US(int, short2, _v2i16_v2i16, _Rint)
#ifdef __opencl_c_integer_dot_product_input_4x8bit_packed
DEFN_INTEL_DOT_PRODUCT_PACKED_BUILTIN_SPIRV(int, int, uint, SU, _i32_i32_i32, _Rint, su)
DEFN_INTEL_DOT_PRODUCT_PACKED(int, su, SU, _i32_i32_i32, _Rint)
DEFN_INTEL_DOT_PRODUCT_PACKED_US
#endif // __opencl_c_integer_dot_product_input_4x8bit_packed
#ifdef __opencl_c_integer_dot_product_saturation_accumulation
DEFN_INTEL_DOT_PRODUCT_SAT_BUILTIN_SPIRV(int, char4, uchar4, SU, _v4i8_v4i8_i32, _Rint, su, s)
DEFN_INTEL_DOT_PRODUCT_SAT(int, char4, uchar4, SU, _v4i8_v4i8_i32, _Rint)
DEFN_INTEL_DOT_PRODUCT_SAT_US(int, char4, _v4i8_v4i8_i32, _Rint)
DEFN_INTEL_DOT_PRODUCT_SAT_BUILTIN_SPIRV(int, short2, ushort2, SU, _v2i16_v2i16_i32, _Rint, su, s)
DEFN_INTEL_DOT_PRODUCT_SAT(int, short2, ushort2, SU, _v2i16_v2i16_i32, _Rint)
DEFN_INTEL_DOT_PRODUCT_SAT_US(int, short2, _v2i16_v2i16_i32, _Rint)
#ifdef __opencl_c_integer_dot_product_input_4x8bit_packed
DEFN_INTEL_DOT_PRODUCT_SAT_PACKED_BUILTIN_SPIRV(int, int, uint, SU, _i32_i32_i32_i32, _Rint, su, s)
DEFN_INTEL_DOT_PRODUCT_SAT_PACKED(int, SU, su, _i32_i32_i32_i32, _Rint)
DEFN_INTEL_DOT_PRODUCT_SAT_PACKED_US
#endif // __opencl_c_integer_dot_product_input_4x8bit_packed
#endif // __opencl_c_integer_dot_product_saturation_accumulation
#endif // __opencl_c_dot_product_mixed_signedness

// For possible future support of CL_DEVICE_INTEGER_DOT_PRODUCT_INPUT_ALL_KHR
// extension, one needs to:
// - Add arguments of 'short/ushort' type and 'long' result type
// - Add different vector sizes such as 2,3,8,16
#endif // cl_khr_integer_dot_product
