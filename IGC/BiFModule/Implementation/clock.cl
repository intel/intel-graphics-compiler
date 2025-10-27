/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/* Shader clock extension */
ulong __attribute__((overloadable)) __spirv_ReadClockKHR_Rulong(int scope) {
  (void) scope;
  return __builtin_IB_read_cycle_counter();
}

uint2 __attribute__((overloadable)) __spirv_ReadClockKHR_Ruint2(int scope) {
  (void) scope;
  return as_uint2(__builtin_IB_read_cycle_counter());
}
