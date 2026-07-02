//========================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
//=========================== end_copyright_notice =============================

// Verify that variables with constant values have correct constant value emitted
// In OpenCL, verify that const int x = 100 results in either constant value (-O2) or a DW_AT_location expression (in -O0).

// UNSUPPORTED: sys32, lib-igc-clang, llvm-22-plus
// REQUIRES: oneapi-readelf, llvm-16-plus

constant float gb_float = 100500;
constant char gb_char_ar[4] = {0xbe, 0xef, 0xca, 0xfe};

__kernel void foo(global int *res) {
  constant int lc_int = 50;
  constant char *lc_ptr = gb_char_ar;
  const uint lc_const_uint = 1357;
  *res += lc_const_uint + gb_float + gb_char_ar[2] + lc_int + lc_ptr[1];
}

// RUN: %if dg2-supported %{ ocloc compile -file %s -options " -g -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_dg2_'" -device dg2 %}
// RUN: %if dg2-supported %{ oneapi-readelf --debug-dump %t_dg2_OCL_simd8_foo.elf | \
// RUN: FileCheck %s --check-prefixes=CHECK,CHECKO0,CHECKO0-DG2 %}

// RUN: %if dg2-supported %{ ocloc compile -file %s -options " -g -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_dg2_'" -device dg2 %}
// RUN: %if dg2-supported %{ oneapi-readelf --debug-dump %t_dg2_OCL_simd32_foo.elf | \
// RUN: FileCheck %s --check-prefixes=CHECK,CHECKO2 %}

// RUN: %if cri-supported %{ ocloc compile -file %s -options " -g -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_cri_, ShortImplicitPayloadHeader=0, RemoveUnusedIdImplicitArguments=0'" -device cri %}
// RUN: %if cri-supported %{ oneapi-readelf --debug-dump %t_cri_OCL_simd16_foo.elf | \
// RUN: FileCheck %s --check-prefixes=CHECK,CHECKO0,CHECKO0-CRI %}

// RUN: %if cri-supported %{ ocloc compile -file %s -options " -g -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_cri_, ShortImplicitPayloadHeader=0, RemoveUnusedIdImplicitArguments=0'" -device cri %}
// RUN: %if cri-supported %{ oneapi-readelf --debug-dump %t_cri_OCL_simd32_foo.elf | \
// RUN: FileCheck %s --check-prefixes=CHECK,CHECKO2 %}

// CHECK: Abbrev Number: [[#]] (DW_TAG_compile_unit)
// CHECK: DW_AT_name : constant-variable-location.cl
// CHECKO0-DG2: DW_AT_INTEL_simd_width: 8
// CHECKO0-CRI: DW_AT_INTEL_simd_width: 16
// CHECKO2: DW_AT_INTEL_simd_width: 32

// CHECK-DAG: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK: DW_AT_name : gb_char_ar
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE:]]
// CHECK-NEXT: DW_AT_decl_line : 25
// CHECK-NEXT: DW_AT_type : <0x[[#%x,GB_CHAR_AR_TYPE:]]>
// CHECK-NEXT: DW_AT_const_value : 4 byte block: be ef ca fe

// CHECK-DAG: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK: DW_AT_name : gb_float
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 24
// CHECK-NEXT: DW_AT_type : <0x[[#%x,GB_FLOAT_TYPE:]]>
// CHECK-NEXT: DW_AT_const_value : 0x47c44a00

// Note: lc_int gets no DIE, as clang folds every use to an immediate

// CHECKO0-DAG: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECKO0: DW_AT_name : lc_ptr
// CHECKO0-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECKO0-NEXT: DW_AT_decl_line : 29
// CHECKO0-NEXT: DW_AT_type : <0x[[#%x,LC_PTR_TYPE:]]>
// CHECKO0-NEXT: DW_AT_location : 0x[[#%x,LC_PTR_LOC:]] (location list)

// CHECK-DAG: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK: DW_AT_name : lc_const_uint
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 30
// CHECK-NEXT: DW_AT_type : <0x[[#%x,LC_CONST_UINT_TYPE:]]>
// CHECKO0-NEXT: DW_AT_location : 0x[[#%x,LC_CONST_UINT_LOC:]] (location list)
// CHECKO2-NEXT: DW_AT_const_value : 0x54d

// CHECK: The File Name Table (offset {{.+}}):
// CHECK: [[#DECL_FILE]] [[#]] 0 0 constant-variable-location.cl

// CHECKO0: Contents of the .debug_loc section:
// CHECKO0-DAG: {{0+}}[[#%x,LC_PTR_LOC]] {{.+}} (DW_OP_lit[[#]]; DW_OP_const1u: 128; DW_OP_INTEL_regval_bits: 64; DW_OP_constu: [[#]]; DW_OP_const[[#]]u: [[#]]; DW_OP_INTEL_regval_bits: 32; DW_OP_plus; DW_OP_plus_uconst: [[#]]; DW_OP_INTEL_push_simd_lane; DW_OP_lit8; DW_OP_mul; DW_OP_plus)
// CHECKO0-DAG: {{0+}}[[#%x,LC_CONST_UINT_LOC]] {{.+}} (DW_OP_lit[[#]]; DW_OP_const1u: 128; DW_OP_INTEL_regval_bits: 64; DW_OP_constu: [[#]]; DW_OP_const[[#]]u: [[#]]; DW_OP_INTEL_regval_bits: 32; DW_OP_plus; DW_OP_plus_uconst: [[#]]; DW_OP_INTEL_push_simd_lane; DW_OP_lit4; DW_OP_mul; DW_OP_plus)
