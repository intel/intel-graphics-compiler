/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported

// PVC has no SIMD8, so disabling SIMD16 and SIMD32 leaves no SIMD size to
// compile. The build must fail instead of leaving the kernel out of the binary.

// RUN: not ocloc compile -file %s -device pvc -options "-igc_opts 'EnableOCLSIMD16=0,EnableOCLSIMD32=0'" 2>&1 | FileCheck %s
// RUN: not ocloc compile -file %s -device pvc -options "-igc_opts 'EnableOCLSIMD16=0,EnableOCLSIMD32=0,DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s

// CHECK: error: in kernel 'test': Failed to compile the kernel in any SIMD size

__kernel void test(__global int *dst, __global int *src) {
  int id = get_global_id(0);
  dst[id] = src[id] + 1;
}
