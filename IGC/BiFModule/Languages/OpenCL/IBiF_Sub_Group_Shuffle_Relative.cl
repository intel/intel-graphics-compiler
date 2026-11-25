/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define DEFN_SUB_GROUP_SHUFFLE_RELATIVE(TYPE, SPV_TYPE, TYPE_ABBR)                                                     \
  INLINE TYPE OVERLOADABLE sub_group_shuffle_up(TYPE value, uint delta) {                                              \
    return __spirv_GroupNonUniformShuffleUp(Subgroup, as_##SPV_TYPE(value), delta);                                    \
  }                                                                                                                    \
  INLINE TYPE OVERLOADABLE sub_group_shuffle_down(TYPE value, uint delta) {                                            \
    return __spirv_GroupNonUniformShuffleDown(Subgroup, as_##SPV_TYPE(value), delta);                                  \
  }

#if defined(cl_khr_subgroup_shuffle_relative)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(char, char, i8)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(uchar, char, i8)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(short, short, i16)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(ushort, short, i16)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(int, int, i32)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(uint, int, i32)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(long, long, i64)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(ulong, long, i64)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(float, float, f32)
#endif

#if defined(cl_khr_subgroup_shuffle_relative)
#if defined(cl_khr_fp64)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(double, double, f64)
#endif
#endif

#if defined(cl_khr_subgroup_shuffle_relative)
#if defined(cl_khr_fp16)
DEFN_SUB_GROUP_SHUFFLE_RELATIVE(half, half, f16)
#endif
#endif
