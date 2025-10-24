/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/* Shader clock extension */
ulong SPIRV_OVERLOADABLE SPIRV_BUILTIN(ReadClockKHR, _i64_i32, _Rulong)(int scope) {
  (void) scope;
  return __builtin_IB_read_cycle_counter();
}

uint2 SPIRV_OVERLOADABLE SPIRV_BUILTIN(ReadClockKHR, _v2i32_i32, _Ruint2)(int scope) {
  (void) scope;
  return as_uint2(__builtin_IB_read_cycle_counter());
}
