/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define SHUFFLE_AS_INTEGER_2_1(FNAME)                                                                                  \
  INLINE half OVERLOADABLE FNAME(half x, half y, uint c) { return as_half(FNAME(as_ushort(x), as_ushort(y), c)); }

#define SHUFFLE_AS_INTEGER_2_2(FNAME)                                                                                  \
  INLINE half2 OVERLOADABLE FNAME(half2 x, half2 y, uint c) { return as_half2(FNAME(as_uint(x), as_uint(y), c)); }

#define SHUFFLE_AS_INTEGER_2_4(FNAME)                                                                                  \
  INLINE half4 OVERLOADABLE FNAME(half4 x, half4 y, uint c) { return as_half4(FNAME(as_uint2(x), as_uint2(y), c)); }

#define SHUFFLE_AS_INTEGER_2_8(FNAME)                                                                                  \
  INLINE half8 OVERLOADABLE FNAME(half8 x, half8 y, uint c) { return as_half8(FNAME(as_uint4(x), as_uint4(y), c)); }

#define SHUFFLE_AS_INTEGER_2_16(FNAME)                                                                                 \
  INLINE half16 OVERLOADABLE FNAME(half16 x, half16 y, uint c) { return as_half16(FNAME(as_uint8(x), as_uint8(y), c)); }

// gentype intel_sub_group_shuffle(gentype data, uint sub_group_local_id )
//
// Allows data to be arbitrarily transferred between work items in a subgroup.
// The data that is returned for this work item is the value of data for the work item
// identified by sub_group_local_id.
#define DEFN_INTEL_SUB_GROUP_SHUFFLE(TYPE, SPIRV_TYPE, TYPE_ABBR)                                                      \
  INLINE TYPE OVERLOADABLE intel_sub_group_shuffle(TYPE x, uint c) {                                                   \
    return __spirv_SubgroupShuffleINTEL(as_##SPIRV_TYPE(x), c);                                                        \
  }

// gentype intel_sub_group_shuffle_down( gentype current, gentype next, uint delta )
//
// Allows data to be transferred from a work item in the subgroup with a higher sub_group_local_id
// down to a work item in the subgroup with a lower sub_group_local_id.
#define DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(TYPE, SPIRV_TYPE, TYPE_ABBR)                                                 \
  INLINE TYPE OVERLOADABLE intel_sub_group_shuffle_down(TYPE cur, TYPE next, uint c) {                                 \
    return __spirv_SubgroupShuffleDownINTEL(as_##SPIRV_TYPE(cur), as_##SPIRV_TYPE(next), c);                           \
  }

// gentype intel_sub_group_shuffle_up(gentype previous, gentype current, uint delta )
//
// Allows data to be transferred from a work item in the subgroup with a lower sub_group_local_id
// up to a work item in the subgroup with a higher sub_group_local_id.
#define DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(TYPE, SPIRV_TYPE, TYPE_ABBR)                                                   \
  INLINE TYPE OVERLOADABLE intel_sub_group_shuffle_up(TYPE prev, TYPE cur, uint c) {                                   \
    return __spirv_SubgroupShuffleUpINTEL(as_##SPIRV_TYPE(prev), as_##SPIRV_TYPE(cur), c);                             \
  }

// gentype intel_sub_group_shuffle_xor( gentype data, uint value )
//
// The data that is returned for this work item is the value of data for the work item with
// sub_group_local_id equal to this work item's sub_group_local_id XOR'd with the specified value.
#define DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(TYPE, SPIRV_TYPE, TYPE_ABBR)                                                  \
  INLINE TYPE OVERLOADABLE intel_sub_group_shuffle_xor(TYPE x, uint c) {                                               \
    return __spirv_SubgroupShuffleXorINTEL(as_##SPIRV_TYPE(x), c);                                                     \
  }

#define OVERLOAD_AS_INTEGERS_2(FNAME)                                                                                  \
  SHUFFLE_AS_INTEGER_2_1(FNAME)                                                                                        \
  SHUFFLE_AS_INTEGER_2_2(FNAME)                                                                                        \
  SHUFFLE_AS_INTEGER_2_4(FNAME)                                                                                        \
  SHUFFLE_AS_INTEGER_2_8(FNAME)                                                                                        \
  SHUFFLE_AS_INTEGER_2_16(FNAME)

#define DEFN_SUB_GROUP_SHUFFLE(TYPE, SPV_TYPE, TYPE_ABBR)                                                              \
  INLINE TYPE OVERLOADABLE sub_group_shuffle(TYPE value, uint index) {                                                 \
    return __spirv_GroupNonUniformShuffle(Subgroup, as_##SPV_TYPE(value), index);                                      \
  }                                                                                                                    \
  INLINE TYPE OVERLOADABLE sub_group_shuffle_xor(TYPE value, uint mask) {                                              \
    return __spirv_GroupNonUniformShuffleXor(Subgroup, as_##SPV_TYPE(value), mask);                                    \
  }

#ifdef cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_SHUFFLE(char, char, i8)
DEFN_INTEL_SUB_GROUP_SHUFFLE(uchar, char, i8)
#endif

DEFN_INTEL_SUB_GROUP_SHUFFLE(short, short, i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE(ushort, short, i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE(int, int, i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE(uint, int, i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE(long, long, i64)
DEFN_INTEL_SUB_GROUP_SHUFFLE(ulong, long, i64)

#ifdef cl_khr_fp16
DEFN_INTEL_SUB_GROUP_SHUFFLE(half, half, f16)
#endif

DEFN_INTEL_SUB_GROUP_SHUFFLE(float, float, f32)

#ifdef cl_khr_fp64
DEFN_INTEL_SUB_GROUP_SHUFFLE(double, double, f64)
#endif

#ifdef cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(intel_sub_group_shuffle, char, char, uint)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(intel_sub_group_shuffle, uchar, uchar, uint)
#endif

GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(intel_sub_group_shuffle, short, short, uint)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(intel_sub_group_shuffle, ushort, ushort, uint)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(intel_sub_group_shuffle, int, int, uint)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(intel_sub_group_shuffle, uint, uint, uint)

#ifdef cl_khr_fp16
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(intel_sub_group_shuffle, half, half, uint)
#endif

GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(intel_sub_group_shuffle, float, float, uint)

#ifdef cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(char, char, i8)
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(uchar, char, i8)
#endif

DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(short, short, i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(ushort, short, i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(int, int, i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(uint, int, i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(long, long, i64)
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(ulong, long, i64)
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(float, float, f32)

#ifdef cl_khr_fp64
DEFN_INTEL_SUB_GROUP_SHUFFLE_DOWN(double, double, f64)
#endif

#ifdef cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS(intel_sub_group_shuffle_down, char, char, uint)
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS(intel_sub_group_shuffle_down, uchar, uchar, uint)
#endif

GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS(intel_sub_group_shuffle_down, short, short, uint)
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS(intel_sub_group_shuffle_down, ushort, ushort, uint)
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS(intel_sub_group_shuffle_down, int, int, uint)
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS(intel_sub_group_shuffle_down, uint, uint, uint)
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS(intel_sub_group_shuffle_down, float, float, uint)

#ifdef cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(char, char, i8)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(uchar, char, i8)
#endif

DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(short, short, i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(ushort, short, i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(int, int, i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(uint, int, i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(long, long, i64)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(ulong, long, i64)
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(float, float, f32)

#ifdef cl_khr_fp64
DEFN_INTEL_SUB_GROUP_SHUFFLE_UP(double, double, f64)
#endif

#ifdef cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS(intel_sub_group_shuffle_up, char, char, uint)
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS(intel_sub_group_shuffle_up, uchar, uchar, uint)
#endif

GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS(intel_sub_group_shuffle_up, short, short, uint)
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS(intel_sub_group_shuffle_up, ushort, ushort, uint)
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS(intel_sub_group_shuffle_up, int, int, uint)
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS(intel_sub_group_shuffle_up, uint, uint, uint)
GENERATE_VECTOR_FUNCTIONS_3ARGS_VVS(intel_sub_group_shuffle_up, float, float, uint)

#ifdef cl_intel_subgroups_char
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(char, char, i8)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(uchar, char, i8)
#endif

DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(short, short, i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(ushort, short, i16)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(int, int, i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(uint, int, i32)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(long, long, i64)
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(ulong, long, i64)

#ifdef cl_khr_fp16
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(half, half, f16)
#endif

DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(float, float, f32)

#ifdef cl_khr_fp64
DEFN_INTEL_SUB_GROUP_SHUFFLE_XOR(double, double, f64)
#endif

#ifdef cl_intel_subgroups_char
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(intel_sub_group_shuffle_xor, char, char, uint)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(intel_sub_group_shuffle_xor, uchar, uchar, uint)
#endif

GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(intel_sub_group_shuffle_xor, short, short, uint)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(intel_sub_group_shuffle_xor, ushort, ushort, uint)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(intel_sub_group_shuffle_xor, int, int, uint)
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(intel_sub_group_shuffle_xor, uint, uint, uint)

#ifdef cl_khr_fp16
GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(intel_sub_group_shuffle_xor, half, half, uint)
#endif

GENERATE_VECTOR_FUNCTIONS_2ARGS_VS(intel_sub_group_shuffle_xor, float, float, uint)

#ifdef cl_khr_fp16
OVERLOAD_AS_INTEGERS_2(intel_sub_group_shuffle_down)
OVERLOAD_AS_INTEGERS_2(intel_sub_group_shuffle_up)
#endif

#if defined(cl_khr_subgroup_shuffle)
DEFN_SUB_GROUP_SHUFFLE(char, char, i8)
DEFN_SUB_GROUP_SHUFFLE(uchar, char, i8)
DEFN_SUB_GROUP_SHUFFLE(short, short, i16)
DEFN_SUB_GROUP_SHUFFLE(ushort, short, i16)
DEFN_SUB_GROUP_SHUFFLE(int, int, i32)
DEFN_SUB_GROUP_SHUFFLE(uint, int, i32)
DEFN_SUB_GROUP_SHUFFLE(long, long, i64)
DEFN_SUB_GROUP_SHUFFLE(ulong, long, i64)
DEFN_SUB_GROUP_SHUFFLE(float, float, f32)
#endif

#if defined(cl_khr_subgroup_shuffle)
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_SHUFFLE(double, double, f64)
#endif
#endif

#if defined(cl_khr_subgroup_shuffle)
#if defined(cl_khr_fp16)
DEFN_SUB_GROUP_SHUFFLE(half, half, f16)
#endif
#endif
