//========================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
//=========================== end_copyright_notice =============================

// For global address space variables, verify location expression emitted (should contain special operations that IGC emits)

// UNSUPPORTED: sys32
// REQUIRES: oneapi-readelf

global int *f1(global int *f1_arg1) {
  global int * private f1_ptr = 0;
  return f1_arg1;
}

__kernel void foo(global char *foo_param) {
  global int *foo_ptr;
  *foo_ptr = foo_param[1];
  *foo_param += *f1(foo_ptr) + foo_param[1] + *foo_ptr;
}

// RUN: %if dg2-supported %{ ocloc compile -file %s -options " -g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_dg2_'" -device dg2 %}
// RUN: %if dg2-supported %{ oneapi-readelf --debug-dump %t_dg2_OCL_simd8_foo.elf |\
// RUN: FileCheck %s --check-prefixes=CHECK,CHECK-DG2 %}

// RUN: %if cri-supported %{ ocloc compile -file %s -options " -g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_cri_'" -device cri -internal_options "'-ze-intel-64bit-addressing'" %}
// RUN: %if cri-supported %{ oneapi-readelf --debug-dump %t_cri_OCL_simd16_foo.elf |\
// RUN: FileCheck %s --check-prefixes=CHECK,CHECK-CRI %}

// CHECK: Abbrev Number: [[#]] (DW_TAG_compile_unit)
// CHECK-NEXT: DW_AT_producer : clang version
// CHECK-NEXT: DW_AT_language : 21 (OpenCL)
// CHECK-NEXT: DW_AT_name : global-variable-location.cl
// CHECK-NEXT: DW_AT_low_pc :
// CHECK-NEXT: DW_AT_high_pc :
// CHECK-NEXT: DW_AT_stmt_list : 0
// CHECK-DG2-NEXT: DW_AT_INTEL_simd_width: 8
// CHECK-CRI-NEXT: DW_AT_INTEL_simd_width: 16
// CHECK-NEXT: DW_AT_comp_dir :

// CHECK: Abbrev Number: [[#]] (DW_TAG_subprogram)
// CHECK-NEXT: DW_AT_name : foo
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE:]]
// CHECK-NEXT: DW_AT_decl_line : 28
// CHECK-DG2-NEXT: DW_AT_INTEL_simd_width: 8
// CHECK-CRI-NEXT: DW_AT_INTEL_simd_width: 16
// CHECK-NEXT: DW_AT_external : 1
// CHECK-NEXT: DW_AT_low_pc :
// CHECK-NEXT: DW_AT_high_pc :

// CHECK: Abbrev Number: [[#]] (DW_TAG_formal_parameter)
// CHECK-NEXT: DW_AT_name : foo_param
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 28
// CHECK-NEXT: DW_AT_type :
// CHECK-DG2-NEXT: DW_AT_location : {{.+}} (DW_OP_const1u: 143; DW_OP_const1u: 128; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: 16; DW_OP_INTEL_push_simd_lane; DW_OP_lit8; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])
// CHECK-CRI-NEXT: DW_AT_location : {{.+}} (DW_OP_const1u: 143; DW_OP_const2u: 256; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: 16; DW_OP_INTEL_push_simd_lane; DW_OP_lit8; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])

// CHECK: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK-NEXT: DW_AT_name : foo_ptr
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 29
// CHECK-NEXT: DW_AT_type :
// CHECK-DG2-NEXT: DW_AT_location : {{.+}} (DW_OP_const1u: 143; DW_OP_const1u: 128; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: 16; DW_OP_INTEL_push_simd_lane; DW_OP_lit8; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])
// CHECK-CRI-NEXT: DW_AT_location : {{.+}} (DW_OP_const1u: 143; DW_OP_const2u: 256; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: 16; DW_OP_INTEL_push_simd_lane; DW_OP_lit8; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])

// CHECK: Abbrev Number: [[#]] (DW_TAG_subprogram)
// CHECK-NEXT: DW_AT_name : f1
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 23
// CHECK-DG2-NEXT: DW_AT_INTEL_simd_width: 8
// CHECK-CRI-NEXT: DW_AT_INTEL_simd_width: 16

// CHECK: Abbrev Number: [[#]] (DW_TAG_formal_parameter)
// CHECK-NEXT: DW_AT_name : f1_arg1
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 23
// CHECK-NEXT: DW_AT_type :
// CHECK-DG2-NEXT: DW_AT_location : {{.+}} (DW_OP_const1u: 143; DW_OP_const1u: 128; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: 16; DW_OP_INTEL_push_simd_lane; DW_OP_lit8; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])
// CHECK-CRI-NEXT: DW_AT_location : {{.+}} (DW_OP_const1u: 143; DW_OP_const2u: 256; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: 16; DW_OP_INTEL_push_simd_lane; DW_OP_lit8; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])

// CHECK: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK-NEXT: DW_AT_name : f1_ptr
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 24
// CHECK-NEXT: DW_AT_type :
// CHECK-DG2-NEXT: DW_AT_location : {{.+}} (DW_OP_const1u: 143; DW_OP_const1u: 128; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: 16; DW_OP_INTEL_push_simd_lane; DW_OP_lit8; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])
// CHECK-CRI-NEXT: DW_AT_location : {{.+}} (DW_OP_const1u: 143; DW_OP_const2u: 256; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: 16; DW_OP_INTEL_push_simd_lane; DW_OP_lit8; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])

// CHECK: The File Name Table (offset {{.+}}):
// CHECK: [[#DECL_FILE]] [[#]] 0 0 global-variable-location.cl
