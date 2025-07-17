/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: dg2-supported
// RUN: ocloc compile -file %s -options "-igc_opts 'DumpVISAASMToConsole=1'" -device dg2 | FileCheck --check-prefix CHECK-VISA %s

// This test verifies that function calls as subroutines are properly handled when
// intel_reqd_sub_group_size is equal to 32 on DG2. The final call instruction must be
// in the SIMD width required by the user (32), but instructions that are responsible
// for passing parameters to the subroutine and returning values from the subroutine
// must be split into two SIMD16 instructions.

// CHECK-VISA: mov (M1, 16) [[A_PARAM_LO:.*]](0,0)<1> {{.*}}(0,0)<0;1,0>
// CHECK-VISA: mov (M5, 16) [[A_PARAM_HI:.*]](0,0)<1> {{.*}}(0,0)<0;1,0>
// CHECK-VISA: mov (M1, 16) [[B_PARAM_LO:.*]](0,0)<1> {{.*}}(0,0)<0;1,0>
// CHECK-VISA: mov (M5, 16) [[B_PARAM_HI:.*]](0,0)<1> {{.*}}(0,0)<0;1,0>
// CHECK-VISA: call (M1, 32) compute_sum_1
// CHECK-VISA: mov (M1, 16) {{.*}}(0,0)<1> [[COMPUTE_SUM_RETVAL_LO:.*]](0,0)<1;1,0>
// CHECK-VISA: mov (M5, 16) {{.*}}(0,0)<1> [[COMPUTE_SUM_RETVAL_HI:.*]](0,0)<1;1,0>

// CHECK-VISA: .function "compute_sum_1"
// CHECK-VISA: compute_sum_1:
// CHECK-VISA: add (M1, 16) [[RESULT_LO:.*]](0,0)<1> [[A_PARAM_LO]](0,0)<1;1,0> [[B_PARAM_LO]](0,0)<1;1,0>
// CHECK-VISA: add (M5, 16) [[RESULT_HI:.*]](0,0)<1> [[A_PARAM_HI]](0,0)<1;1,0> [[B_PARAM_HI]](0,0)<1;1,0>
// CHECK-VISA: mov (M1, 16) [[COMPUTE_SUM_RETVAL_LO]](0,0)<1> [[RESULT_LO]](0,0)<1;1,0>
// CHECK-VISA: mov (M5, 16) [[COMPUTE_SUM_RETVAL_HI]](0,0)<1> [[RESULT_HI]](0,0)<1;1,0>

__attribute__((noinline))
int compute_sum(int a, int b)
{
    return a + b;
}

__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_sum(global int* out, int x, int y)
{
    int result = compute_sum(x, y);
    out[0] = result;
}
