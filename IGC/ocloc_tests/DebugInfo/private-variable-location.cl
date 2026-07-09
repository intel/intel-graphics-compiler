//========================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
//=========================== end_copyright_notice =============================

// For private address space variables, verify location expression emitted (should contain special operations that IGC emits)
// We should verify DW_AT_location of kernel, function arguments as well as variables.

// UNSUPPORTED: sys32, lib-igc-clang
// REQUIRES: regkeys, oneapi-readelf, dg2-supported

// RUN: ocloc compile -file %s -options " -g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'" -device dg2
// RUN: oneapi-readelf --debug-dump %t_OCL_simd8_foo.elf |\
// RUN: FileCheck %s

constant float gb_float = 100500;
constant char gb_char_ar[4] = {0xbe, 0xef, 0xca, 0xfe};

private
int *f1(private int f1_arg1) {
private
  int f1_lc_int = gb_char_ar[1] + f1_arg1;
  return &f1_arg1;
}

__kernel void foo(private int in, local int *out) {
private
  char lc_ptr[] = {45, 56, 78};
private
  float lc_float;
  lc_float = out[4];
  *out += *f1(gb_float) + lc_ptr[1] + lc_float * in;
}

// CHECK: Abbrev Number: [[#]] (DW_TAG_compile_unit)
// CHECK-NEXT: DW_AT_producer : clang version
// CHECK-NEXT: DW_AT_language : 21 (OpenCL)
// CHECK-NEXT: DW_AT_name : private-variable-location.cl
// CHECK-NEXT: DW_AT_low_pc :
// CHECK-NEXT: DW_AT_high_pc :
// CHECK-NEXT: DW_AT_stmt_list :
// CHECK-NEXT: DW_AT_INTEL_simd_width: 8
// CHECK-NEXT: DW_AT_comp_dir :

// CHECK: Abbrev Number: [[#]] (DW_TAG_subprogram)
// CHECK-NEXT: DW_AT_name : foo

// CHECK-DAG: Abbrev Number: [[#]] (DW_TAG_formal_parameter)
// CHECK: DW_AT_name : in
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE:]]
// CHECK-NEXT: DW_AT_decl_line : 38
// CHECK-NEXT: DW_AT_type :
// CHECK-NEXT: DW_AT_location : {{(0x)?}}[[#%x,IN_LOC:]] (location list)

// CHECK: Abbrev Number: [[#]] (DW_TAG_formal_parameter)
// CHECK-NEXT: DW_AT_name : out
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 38
// CHECK-NEXT: DW_AT_type :
// CHECK-NEXT: DW_AT_location : {{(0x)?}}[[#%x,OUT_LOC:]] (location list)

// CHECK-DAG: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK: DW_AT_name : lc_ptr
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 40
// CHECK-NEXT: DW_AT_type :
// CHECK-NEXT: DW_AT_location : {{(0x)?}}[[#%x,LC_PTR_LOC:]] (location list)

// CHECK: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK-NEXT: DW_AT_name : lc_float
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 42
// CHECK-NEXT: DW_AT_type :
// CHECK-NEXT: DW_AT_location : {{(0x)?}}[[#%x,LC_FLOAT_LOC:]] (location list)

// CHECK: Abbrev Number: [[#]] (DW_TAG_subprogram)
// CHECK-NEXT: DW_AT_name : f1
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 32
// CHECK-NEXT: DW_AT_INTEL_simd_width: 8
// CHECK-NEXT: DW_AT_type :
// CHECK-NEXT: DW_AT_external :
// CHECK-NEXT: DW_AT_low_pc :
// CHECK-NEXT: DW_AT_high_pc :

// CHECK: Abbrev Number: [[#]] (DW_TAG_formal_parameter)
// CHECK-NEXT: DW_AT_name : f1_arg1
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 32
// CHECK-NEXT: DW_AT_type :
// CHECK-NEXT: DW_AT_location : {{(0x)?}}[[#%x,F1_ARG1_LOC:]] (location list)

// CHECK-DAG: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK: DW_AT_name : f1_lc_int
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 34
// CHECK-NEXT: DW_AT_type :
// CHECK-NEXT: DW_AT_location : {{(0x)?}}[[#%x,F1_LC_INT_LOC:]] (location list)

// CHECK: The File Name Table (offset {{.+}}):
// CHECK: [[#DECL_FILE]] [[#]] 0 0 private-variable-location.cl

// CHECK: Contents of the .debug_loc section:
// CHECK-DAG: {{0+}}[[#%x,IN_LOC]] {{.+}} (DW_OP_const1u: 143; DW_OP_const1u: 128; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: [[#]]; DW_OP_INTEL_push_simd_lane; DW_OP_lit4; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])
// CHECK-DAG: {{0+}}[[#%x,OUT_LOC]] {{.+}} (DW_OP_const1u: 143; DW_OP_const1u: 128; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: [[#]]; DW_OP_INTEL_push_simd_lane; DW_OP_lit8; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])
// CHECK-DAG: {{0+}}[[#%x,LC_PTR_LOC]] {{.+}} (DW_OP_const1u: 143; DW_OP_const1u: 128; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: [[#]]; DW_OP_INTEL_push_simd_lane; DW_OP_lit3; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])
// CHECK-DAG: {{0+}}[[#%x,LC_FLOAT_LOC]] {{.+}} (DW_OP_const1u: 143; DW_OP_const1u: 128; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: [[#]]; DW_OP_INTEL_push_simd_lane; DW_OP_lit4; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])
// CHECK-DAG: {{0+}}[[#%x,F1_ARG1_LOC]] {{.+}} (DW_OP_const1u: 143; DW_OP_const1u: 128; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: [[#]]; DW_OP_INTEL_push_simd_lane; DW_OP_lit4; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])
// CHECK-DAG: {{0+}}[[#%x,F1_LC_INT_LOC]] {{.+}} (DW_OP_const1u: 143; DW_OP_const1u: 128; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: [[#]]; DW_OP_INTEL_push_simd_lane; DW_OP_lit4; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])
