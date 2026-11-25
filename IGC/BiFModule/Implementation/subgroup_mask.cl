/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

uint4 __attribute__((overloadable)) __spirv_BuiltInSubgroupEqMaskKHR(void) {
  uint id = __spirv_BuiltInSubgroupLocalInvocationId();
  uint4 v = 0;

  v.x = 1 << id;

  return v;
}

uint4 __attribute__((overloadable)) __spirv_BuiltInSubgroupGeMaskKHR(void) {
  uint id = __spirv_BuiltInSubgroupLocalInvocationId();
  uint4 v = 0;

  v.x = as_uint(as_int(1 << 31) >> (31 - id));

  return v;
}

uint4 __attribute__((overloadable)) __spirv_BuiltInSubgroupLeMaskKHR(void) {
  uint id = __spirv_BuiltInSubgroupLocalInvocationId();
  uint4 v = 0;

  uint bitIdx = 1 << id;

  v.x = (bitIdx - 1) | bitIdx;

  return v;
}

uint4 __attribute__((overloadable)) __spirv_BuiltInSubgroupGtMaskKHR(void) {
  uint4 v = 0;

  v.x = ~__spirv_BuiltInSubgroupLeMaskKHR().x;

  return v;
}

uint4 __attribute__((overloadable)) __spirv_BuiltInSubgroupLtMaskKHR(void) {
  uint id = __spirv_BuiltInSubgroupLocalInvocationId();
  uint4 v = 0;

  v.x = (1 << id) - 1;

  return v;
}

#if defined(cl_khr_subgroup_ballot)
uint4 __attribute__((overloadable)) __spirv_BuiltInSubgroupEqMask(void) { return __spirv_BuiltInSubgroupEqMaskKHR(); }

uint4 __attribute__((overloadable)) __spirv_BuiltInSubgroupGeMask(void) { return __spirv_BuiltInSubgroupGeMaskKHR(); }

uint4 __attribute__((overloadable)) __spirv_BuiltInSubgroupGtMask(void) { return __spirv_BuiltInSubgroupGtMaskKHR(); }

uint4 __attribute__((overloadable)) __spirv_BuiltInSubgroupLeMask(void) { return __spirv_BuiltInSubgroupLeMaskKHR(); }

uint4 __attribute__((overloadable)) __spirv_BuiltInSubgroupLtMask(void) { return __spirv_BuiltInSubgroupLtMaskKHR(); }
#endif
