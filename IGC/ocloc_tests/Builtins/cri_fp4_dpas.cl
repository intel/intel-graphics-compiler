/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: regkeys, cri-supported

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1,EnableFP4Dpas=1'" | FileCheck %s

short8 __attribute__((overloadable)) intel_sub_group_e2m1_e2m1_matrix_mad_k64_bf16(short8 a, int8 b, float8 acc);
kernel void test_e2m1_e2m1_bf16_vec8(global short8 *out, short8 a, int8 b, float8 acc)
{
    // CHECK-LABEL: .kernel "test_e2m1_e2m1_bf16_vec8"
    // CHECK: dpas.e2m1.e2m1.8.8 (M1, 16) {{.*}}.0 {{.*}}.0 {{.*}}.0 {{.*}}(0,0)
    out[0] = intel_sub_group_e2m1_e2m1_matrix_mad_k64_bf16(a, b, acc);
}

short __attribute__((overloadable)) intel_sub_group_e2m1_e2m1_matrix_mad_k64_bf16(short a, int8 b, float acc);
kernel void test_e2m1_e2m1_bf16_scalar(global short *out, short a, int8 b, float acc)
{
    // CHECK-LABEL: .kernel "test_e2m1_e2m1_bf16_scalar"
    // CHECK: dpas.e2m1.e2m1.8.1 (M1, 16) {{.*}}.0 {{.*}}.0 {{.*}}.0 {{.*}}(0,0)
    out[0] = intel_sub_group_e2m1_e2m1_matrix_mad_k64_bf16(a, b, acc);
}
