/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

uint __intel_get_local_size(void);
uint __intel_get_enqueued_local_size(void);
uint __intel_get_local_linear_id(void);

INLINE uint OVERLOADABLE get_max_sub_group_size(void) { return __builtin_IB_get_simd_size(); }

INLINE uint OVERLOADABLE get_num_sub_groups(void) {
  uint totalWorkGroupSize = __intel_get_local_size() + get_max_sub_group_size() - 1;
  return totalWorkGroupSize / get_max_sub_group_size();
}

INLINE uint OVERLOADABLE get_sub_group_id(void) { return __intel_get_local_linear_id() / get_max_sub_group_size(); }

INLINE uint OVERLOADABLE get_sub_group_size(void) {
  uint remainder = __intel_get_local_size() & (get_max_sub_group_size() - 1);
  bool fullSubGroup = (remainder == 0) || (get_sub_group_id() < get_num_sub_groups() - 1);

  return fullSubGroup ? get_max_sub_group_size() : remainder;
}

INLINE uint OVERLOADABLE get_enqueued_num_sub_groups(void) {
  uint totalEnqueuedWorkGroupSize = __intel_get_enqueued_local_size() + get_max_sub_group_size() - 1;
  return totalEnqueuedWorkGroupSize / get_max_sub_group_size();
}

INLINE uint OVERLOADABLE get_sub_group_local_id(void) { return __builtin_IB_get_simd_id(); }

#if defined(cl_khr_subgroup_ballot)
INLINE uint4 OVERLOADABLE get_sub_group_eq_mask() { return as_uint4(__spirv_BuiltInSubgroupEqMask()); }

INLINE uint4 OVERLOADABLE get_sub_group_ge_mask() { return as_uint4(__spirv_BuiltInSubgroupGeMask()); }

INLINE uint4 OVERLOADABLE get_sub_group_gt_mask() { return as_uint4(__spirv_BuiltInSubgroupGtMask()); }

INLINE uint4 OVERLOADABLE get_sub_group_le_mask() { return as_uint4(__spirv_BuiltInSubgroupLeMask()); }

INLINE uint4 OVERLOADABLE get_sub_group_lt_mask() { return as_uint4(__spirv_BuiltInSubgroupLtMask()); }
#endif
