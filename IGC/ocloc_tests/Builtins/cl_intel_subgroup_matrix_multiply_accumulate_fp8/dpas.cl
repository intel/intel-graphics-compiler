/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, cri-supported

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -DTYPE_A=short -DTYPE_RES=float" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_matrix_multiply_accumulate_fp8" \
// RUN: | FileCheck %s --check-prefix=CHECK-VISAASM-SIZE-1_OUTPUT-FLOAT

// CHECK-VISAASM-SIZE-1_OUTPUT-FLOAT-DAG: dpas.bf8.bf8.8.1 (M1, 16) [[OUT1:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-1_OUTPUT-FLOAT-DAG: .decl [[OUT1]] v_type=G type=f num_elts=16 align=wordx32
// CHECK-VISAASM-SIZE-1_OUTPUT-FLOAT-DAG: dpas.hf8.hf8.8.1 (M1, 16) [[OUT2:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-1_OUTPUT-FLOAT-DAG: .decl [[OUT2]] v_type=G type=f num_elts=16 align=wordx32
// CHECK-VISAASM-SIZE-1_OUTPUT-FLOAT-DAG: dpas.hf8.bf8.8.1 (M1, 16) [[OUT3:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-1_OUTPUT-FLOAT-DAG: .decl [[OUT3]] v_type=G type=f num_elts=16 align=wordx32
// CHECK-VISAASM-SIZE-1_OUTPUT-FLOAT-DAG: dpas.bf8.hf8.8.1 (M1, 16) [[OUT4:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-1_OUTPUT-FLOAT-DAG: .decl [[OUT4]] v_type=G type=f num_elts=16 align=wordx32

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -DTYPE_A=short2 -DTYPE_RES=float2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_matrix_multiply_accumulate_fp8" \
// RUN: | FileCheck %s --check-prefix=CHECK-VISAASM-SIZE-2_OUTPUT-FLOAT

// CHECK-VISAASM-SIZE-2_OUTPUT-FLOAT-DAG: dpas.bf8.bf8.8.2 (M1, 16) [[OUT1:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-2_OUTPUT-FLOAT-DAG: .decl [[OUT1]] v_type=G type=f num_elts=32 align=wordx32
// CHECK-VISAASM-SIZE-2_OUTPUT-FLOAT-DAG: dpas.hf8.hf8.8.2 (M1, 16) [[OUT2:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-2_OUTPUT-FLOAT-DAG: .decl [[OUT2]] v_type=G type=f num_elts=32 align=wordx32
// CHECK-VISAASM-SIZE-2_OUTPUT-FLOAT-DAG: dpas.hf8.bf8.8.2 (M1, 16) [[OUT3:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-2_OUTPUT-FLOAT-DAG: .decl [[OUT3]] v_type=G type=f num_elts=32 align=wordx32
// CHECK-VISAASM-SIZE-2_OUTPUT-FLOAT-DAG: dpas.bf8.hf8.8.2 (M1, 16) [[OUT4:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-2_OUTPUT-FLOAT-DAG: .decl [[OUT4]] v_type=G type=f num_elts=32 align=wordx32

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -DTYPE_A=short4 -DTYPE_RES=float4" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_matrix_multiply_accumulate_fp8" \
// RUN: | FileCheck %s --check-prefix=CHECK-VISAASM-SIZE-4_OUTPUT-FLOAT

// CHECK-VISAASM-SIZE-4_OUTPUT-FLOAT-DAG: dpas.bf8.bf8.8.4 (M1, 16) [[OUT1:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-4_OUTPUT-FLOAT-DAG: .decl [[OUT1]] v_type=G type=f num_elts=64 align=wordx32
// CHECK-VISAASM-SIZE-4_OUTPUT-FLOAT-DAG: dpas.hf8.hf8.8.4 (M1, 16) [[OUT2:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-4_OUTPUT-FLOAT-DAG: .decl [[OUT2]] v_type=G type=f num_elts=64 align=wordx32
// CHECK-VISAASM-SIZE-4_OUTPUT-FLOAT-DAG: dpas.hf8.bf8.8.4 (M1, 16) [[OUT3:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-4_OUTPUT-FLOAT-DAG: .decl [[OUT3]] v_type=G type=f num_elts=64 align=wordx32
// CHECK-VISAASM-SIZE-4_OUTPUT-FLOAT-DAG: dpas.bf8.hf8.8.4 (M1, 16) [[OUT4:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-4_OUTPUT-FLOAT-DAG: .decl [[OUT4]] v_type=G type=f num_elts=64 align=wordx32

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -DTYPE_A=short8 -DTYPE_RES=float8" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_matrix_multiply_accumulate_fp8" \
// RUN: | FileCheck %s --check-prefix=CHECK-VISAASM-SIZE-8_OUTPUT-FLOAT

// CHECK-VISAASM-SIZE-8_OUTPUT-FLOAT-DAG: dpas.bf8.bf8.8.8 (M1, 16) [[OUT1:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-8_OUTPUT-FLOAT-DAG: .decl [[OUT1]] v_type=G type=f num_elts=128 align=wordx32
// CHECK-VISAASM-SIZE-8_OUTPUT-FLOAT-DAG: dpas.hf8.hf8.8.8 (M1, 16) [[OUT2:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-8_OUTPUT-FLOAT-DAG: .decl [[OUT2]] v_type=G type=f num_elts=128 align=wordx32
// CHECK-VISAASM-SIZE-8_OUTPUT-FLOAT-DAG: dpas.hf8.bf8.8.8 (M1, 16) [[OUT3:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-8_OUTPUT-FLOAT-DAG: .decl [[OUT3]] v_type=G type=f num_elts=128 align=wordx32
// CHECK-VISAASM-SIZE-8_OUTPUT-FLOAT-DAG: dpas.bf8.hf8.8.8 (M1, 16) [[OUT4:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-8_OUTPUT-FLOAT-DAG: .decl [[OUT4]] v_type=G type=f num_elts=128 align=wordx32


// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -DTYPE_A=short -DTYPE_RES=short" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_matrix_multiply_accumulate_fp8" \
// RUN: | FileCheck %s --check-prefix=CHECK-VISAASM-SIZE-1_OUTPUT-BFLOAT

// CHECK-VISAASM-SIZE-1_OUTPUT-BFLOAT-DAG: dpas.bf8.bf8.8.1 (M1, 16) [[OUT1:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-1_OUTPUT-BFLOAT-DAG: .decl [[OUT1]] v_type=G type=bf num_elts=16 align=wordx32
// CHECK-VISAASM-SIZE-1_OUTPUT-BFLOAT-DAG: dpas.hf8.hf8.8.1 (M1, 16) [[OUT2:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-1_OUTPUT-BFLOAT-DAG: .decl [[OUT2]] v_type=G type=bf num_elts=16 align=wordx32
// CHECK-VISAASM-SIZE-1_OUTPUT-BFLOAT-DAG: dpas.hf8.bf8.8.1 (M1, 16) [[OUT3:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-1_OUTPUT-BFLOAT-DAG: .decl [[OUT3]] v_type=G type=bf num_elts=16 align=wordx32
// CHECK-VISAASM-SIZE-1_OUTPUT-BFLOAT-DAG: dpas.bf8.hf8.8.1 (M1, 16) [[OUT4:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-1_OUTPUT-BFLOAT-DAG: .decl [[OUT4]] v_type=G type=bf num_elts=16 align=wordx32

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -DTYPE_A=short2 -DTYPE_RES=short2" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_matrix_multiply_accumulate_fp8" \
// RUN: | FileCheck %s --check-prefix=CHECK-VISAASM-SIZE-2_OUTPUT-BFLOAT

// CHECK-VISAASM-SIZE-2_OUTPUT-BFLOAT-DAG: dpas.bf8.bf8.8.2 (M1, 16) [[OUT1:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-2_OUTPUT-BFLOAT-DAG: .decl [[OUT1]] v_type=G type=bf num_elts=32 align=wordx32
// CHECK-VISAASM-SIZE-2_OUTPUT-BFLOAT-DAG: dpas.hf8.hf8.8.2 (M1, 16) [[OUT2:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-2_OUTPUT-BFLOAT-DAG: .decl [[OUT2]] v_type=G type=bf num_elts=32 align=wordx32
// CHECK-VISAASM-SIZE-2_OUTPUT-BFLOAT-DAG: dpas.hf8.bf8.8.2 (M1, 16) [[OUT3:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-2_OUTPUT-BFLOAT-DAG: .decl [[OUT3]] v_type=G type=bf num_elts=32 align=wordx32
// CHECK-VISAASM-SIZE-2_OUTPUT-BFLOAT-DAG: dpas.bf8.hf8.8.2 (M1, 16) [[OUT4:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-2_OUTPUT-BFLOAT-DAG: .decl [[OUT4]] v_type=G type=bf num_elts=32 align=wordx32

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -DTYPE_A=short4 -DTYPE_RES=short4" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_matrix_multiply_accumulate_fp8" \
// RUN: | FileCheck %s --check-prefix=CHECK-VISAASM-SIZE-4_OUTPUT-BFLOAT

// CHECK-VISAASM-SIZE-4_OUTPUT-BFLOAT-DAG: dpas.bf8.bf8.8.4 (M1, 16) [[OUT1:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-4_OUTPUT-BFLOAT-DAG: .decl [[OUT1]] v_type=G type=bf num_elts=64 align=wordx32
// CHECK-VISAASM-SIZE-4_OUTPUT-BFLOAT-DAG: dpas.hf8.hf8.8.4 (M1, 16) [[OUT2:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-4_OUTPUT-BFLOAT-DAG: .decl [[OUT2]] v_type=G type=bf num_elts=64 align=wordx32
// CHECK-VISAASM-SIZE-4_OUTPUT-BFLOAT-DAG: dpas.hf8.bf8.8.4 (M1, 16) [[OUT3:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-4_OUTPUT-BFLOAT-DAG: .decl [[OUT3]] v_type=G type=bf num_elts=64 align=wordx32
// CHECK-VISAASM-SIZE-4_OUTPUT-BFLOAT-DAG: dpas.bf8.hf8.8.4 (M1, 16) [[OUT4:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-4_OUTPUT-BFLOAT-DAG: .decl [[OUT4]] v_type=G type=bf num_elts=64 align=wordx32

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1' -DTYPE_A=short8 -DTYPE_RES=short8" \
// RUN: -internal_options "-cl-ext=-all,+cl_intel_subgroup_matrix_multiply_accumulate_fp8" \
// RUN: | FileCheck %s --check-prefix=CHECK-VISAASM-SIZE-8_OUTPUT-BFLOAT

// CHECK-VISAASM-SIZE-8_OUTPUT-BFLOAT-DAG: dpas.bf8.bf8.8.8 (M1, 16) [[OUT1:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-8_OUTPUT-BFLOAT-DAG: .decl [[OUT1]] v_type=G type=bf num_elts=128 align=wordx32
// CHECK-VISAASM-SIZE-8_OUTPUT-BFLOAT-DAG: dpas.hf8.hf8.8.8 (M1, 16) [[OUT2:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-8_OUTPUT-BFLOAT-DAG: .decl [[OUT2]] v_type=G type=bf num_elts=128 align=wordx32
// CHECK-VISAASM-SIZE-8_OUTPUT-BFLOAT-DAG: dpas.hf8.bf8.8.8 (M1, 16) [[OUT3:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-8_OUTPUT-BFLOAT-DAG: .decl [[OUT3]] v_type=G type=bf num_elts=128 align=wordx32
// CHECK-VISAASM-SIZE-8_OUTPUT-BFLOAT-DAG: dpas.bf8.hf8.8.8 (M1, 16) [[OUT4:V[0-9]+]].0 {{.*}} {{.*}} {{.*}}
// CHECK-VISAASM-SIZE-8_OUTPUT-BFLOAT-DAG: .decl [[OUT4]] v_type=G type=bf num_elts=128 align=wordx32


__attribute__((intel_reqd_sub_group_size(16)))
kernel void kernelTest(__global TYPE_A* partA, __global int8* partB, __global TYPE_RES* acc, __global TYPE_RES* res1, __global TYPE_RES* res2, __global TYPE_RES* res3, __global TYPE_RES* res4)
{
    int lid = get_sub_group_local_id();
    *res1 = intel_sub_group_e5m2_e5m2_matrix_mad_k32(partA[lid], partB[lid], acc[lid]);
    *res2 = intel_sub_group_e4m3_e4m3_matrix_mad_k32(partA[lid], partB[lid], acc[lid]);
    *res3 = intel_sub_group_e5m2_e4m3_matrix_mad_k32(partA[lid], partB[lid], acc[lid]);
    *res4 = intel_sub_group_e4m3_e5m2_matrix_mad_k32(partA[lid], partB[lid], acc[lid]);
}
