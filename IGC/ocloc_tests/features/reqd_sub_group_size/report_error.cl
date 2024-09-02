/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: dg2-supported
// RUN: not ocloc compile -file %s -device dg2 | FileCheck %s

// IGC enables EUFusion CallWA for DG2 when nested stackcalls or indirect calls
// are present in a module. The workaround is not supported in SIMD32. This test
// verifies if a proper error message is printed if CallWA is required and
// intel_reqd_sub_group_size kernel attribute is set to 32.

// CHECK: error: Cannot compile a kernel in the SIMD mode specified by intel_reqd_sub_group_size(32)
// CHECK-NEXT: in kernel: 'test_simple'

int fact(int n) {
    return n < 2 ? 1 : n * fact(n - 1);
}

__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_simple(global int* out, int n) {
    out[0] = fact(n);
}
