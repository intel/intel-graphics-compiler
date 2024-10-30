/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: dg2-supported
// RUN: ocloc compile -file %s -options "-igc_opts 'DumpVISAASMToConsole=1'" -device dg2 | FileCheck --check-prefix CHECK-VISA %s
// RUN: ocloc compile -file %s -options "-igc_opts 'DumpASMToConsole=1'" -device dg2 | FileCheck %s

// Test verifies SIMD32 code is generated.
//
// IGC enables EU Fusion CallWA for DG2 when nested stackcalls or indirect calls
// are present in a module. The workaround is not supported in SIMD32. This test
// verifies if a proper warning message is printed if EU fusion is disabled
// because SIMD32 mode specified by intel_reqd_sub_group_size(32).
// The test verifies that CallWA is not applied (CallWA_BigB0 BB label is not used).

// CHECK-VISA: .kernel_attr SimdSize=32
// CHECK-NOT: {{^}}{{[_A-z0-9]*}}_CallWA_BigB0:{{$}}
// CHECK: warning: EU fusion is disabled, it does not work on the current platform if SIMD32 mode specified by intel_reqd_sub_group_size(32)
// CHECK-NEXT: in kernel: 'test_simple'

int fact(int n) {
    return n < 2 ? 1 : n * fact(n - 1);
}

__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_simple(global int* out, int n) {
    out[0] = fact(n);
}
