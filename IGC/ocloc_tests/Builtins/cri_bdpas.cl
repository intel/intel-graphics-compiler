/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: regkeys, cri-supported, llvm-16-plus

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'EnableOpaquePointersBackend=1,DumpVISAASMToConsole=1,EnableFP4Dpas=1'" | FileCheck %s

__attribute__((intel_reqd_sub_group_size(16))) kernel void test_bdpas16b_1(global short8 *out, short8 a, int8 b, short8 acc, uchar scale_a, uchar scale_b)
{
    // CHECK-LABEL: .kernel "test_bdpas16b_1"
    // CHECK-DAG: bdpas.bf.bf.8.8 (M1, 16) [[Dst1:[A-z0-9_]*]].0 [[Acc1:[A-z0-9_]*]].0 [[SrcB1:[A-z0-9_]*]].0 [[SrcA1:[A-z0-9_]*]].0 [[ScaleB1:[A-z0-9_]*]](0,0) [[ScaleA1:[A-z0-9_]*]](0,0)
    // CHECK-DAG: .decl [[Dst1]] v_type=G type=bf num_elts=128
    // CHECK-DAG: .decl [[Acc1]] v_type=G type=bf num_elts=128
    // CHECK-DAG: .decl [[SrcB1]] v_type=G type=bf num_elts=256
    // CHECK-DAG: .decl [[SrcA1]] v_type=G type=bf num_elts=128
    // CHECK-DAG: .decl [[ScaleB1]] v_type=G type=ub num_elts=16
    // CHECK-DAG: .decl [[ScaleA1]] v_type=G type=ub num_elts=16
    out[0] = intel_sub_group_bf16_bf16_scaled_matrix_mad_k16(a, b, acc, scale_a, scale_b);
}

__attribute__((intel_reqd_sub_group_size(16))) kernel void test_bdpas_16b_2(global half8 *out, short8 a, int8 b, half8 acc, uchar scale_a, uchar scale_b)
{
    // CHECK-LABEL: .kernel "test_bdpas_16b_2"
    // CHECK: bdpas.hf.hf.8.8 (M1, 16) {{.*}}.0 {{.*}}.0 {{.*}}.0 {{.*}}.0 {{.*}}(0,0) {{.*}}(0,0)
    out[0] = intel_sub_group_f16_f16_scaled_matrix_mad_k16(a, b, acc, scale_a, scale_b);
}

__attribute__((intel_reqd_sub_group_size(16))) kernel void test_bdpas_8b_1(global short8 *out, short8 a, int8 b, float8 acc, uchar scale_a, uchar scale_b)
{
    // CHECK-LABEL: .kernel "test_bdpas_8b_1"
    // CHECK: bdpas.hf8.bf8.8.8 (M1, 16) {{.*}}.0 {{.*}}.0 {{.*}}.0 {{.*}}.0 {{.*}}(0,0) {{.*}}(0,0)
    out[0] = intel_sub_group_bf8_hf8_scaled_matrix_mad_k32_bf16(a, b, acc, scale_a, scale_b);
}

__attribute__((intel_reqd_sub_group_size(16))) kernel void test_bdpas_8b_2(global float8 *out, short8 a, int8 b, float8 acc, uchar scale_a, uchar scale_b)
{
    // CHECK-LABEL: .kernel "test_bdpas_8b_2"
    // CHECK: bdpas.bf8.hf8.8.8 (M1, 16) {{.*}}.0 {{.*}}.0 {{.*}}.0 {{.*}}.0 {{.*}}(0,0) {{.*}}(0,0)
    out[0] = intel_sub_group_hf8_bf8_scaled_matrix_mad_k32(a, b, acc, scale_a, scale_b);
}

__attribute__((intel_reqd_sub_group_size(16))) kernel void test_bdpas_8b_3(global short8 *out, short8 a, int8 b, short8 acc, uchar scale_a, uchar scale_b)
{
    // CHECK-LABEL: .kernel "test_bdpas_8b_3"
    // CHECK: bdpas.bf8.bf8.8.8 (M1, 16) {{.*}}.0 {{.*}}.0 {{.*}}.0 {{.*}}.0 {{.*}}(0,0) {{.*}}(0,0)
    out[0] = intel_sub_group_bf8_bf8_scaled_matrix_mad_k32(a, b, acc, scale_a, scale_b);
}

__attribute__((intel_reqd_sub_group_size(16))) kernel void test_bdpas_8b_4(global float8 *out, short8 a, int8 b, short8 acc, uchar scale_a, uchar scale_b)
{
    // CHECK-LABEL: .kernel "test_bdpas_8b_4"
    // CHECK: bdpas.bf8.bf8.8.8 (M1, 16) {{.*}}.0 {{.*}}.0 {{.*}}.0 {{.*}}.0 {{.*}}(0,0) {{.*}}(0,0)
    out[0] = intel_sub_group_bf8_bf8_scaled_matrix_mad_k32_f32(a, b, acc, scale_a, scale_b);
}

__attribute__((intel_reqd_sub_group_size(16))) kernel void test_bdpas_4b_3(global float8 *out, short8 a, int8 b, float8 acc, uchar scale_a, uchar scale_b)
{
    // CHECK-LABEL: .kernel "test_bdpas_4b_3"
    // CHECK: bdpas.e2m1.e2m1.8.8 (M1, 16) {{.*}}.0 {{.*}}.0 {{.*}}.0 {{.*}}.0 {{.*}}(0,0) {{.*}}(0,0)
    out[0] = intel_sub_group_e2m1_e2m1_scaled_matrix_mad_k64(a, b, acc, scale_a, scale_b);
}
