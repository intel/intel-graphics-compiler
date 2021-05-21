/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/* Shader clock extension */
__attribute__((always_inline))
ulong __builtin_spirv_OpReadClockKHR_i64_i32(uint scope) {
  (void) scope;
  return as_ulong(__builtin_IB_read_cycle_counter());
}

__attribute__((always_inline))
uint __builtin_spirv_OpReadClockKHR_i32_i32(uint scope) {
  return (uint) __builtin_spirv_OpReadClockKHR_i64_i32(scope);
}

