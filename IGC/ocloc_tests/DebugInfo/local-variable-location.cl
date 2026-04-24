//========================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
//=========================== end_copyright_notice =============================

// For variables in local address space, verify location expression emitted (should contain special operations that IGC emits)
// We should verify presence of DW_AT_address_class and DW_AT_location along with DW_OP_addr operation.
// We verify actual value passed to DW_OP_addr to confirm that it has 0x10000000 high bit added.

// UNSUPPORTED: sys32
// REQUIRES: oneapi-readelf

constant float gb_float = 100500;
constant char gb_char_ar[4] = {0xbe, 0xef, 0xca, 0xfe};

local int *f1(int f1_arg1) {
  local int * private f1_ptr = 0;
  return 0;
}

__kernel void foo(local int *foo_param, global char *foo_param1) {
  local char lc_ptr[10];
  local float lc_float;
  lc_float = foo_param1[4];
  global int * local foo_ptr;
  *foo_ptr = foo_param1[1];
  *foo_param += *f1(gb_float) + lc_ptr[1] + lc_float * foo_param[1] + *foo_ptr;
}

// RUN: %if dg2-supported %{ ocloc compile -file %s -options " -g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_dg2_'" -device dg2 %}
// RUN: %if dg2-supported %{ oneapi-readelf --debug-dump %t_dg2_OCL_simd8_foo.elf | \
// RUN: FileCheck %s --check-prefixes=CHECK,CHECK-DG2 %}

// RUN: %if cri-supported %{ ocloc compile -file %s -options " -g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_cri_'" -device cri -internal_options "'-ze-intel-64bit-addressing'" %}
// RUN: %if cri-supported %{ oneapi-readelf --debug-dump %t_cri_OCL_simd16_foo.elf | \
// RUN: FileCheck %s --check-prefixes=CHECK,CHECK-CRI %}

// CHECK: Abbrev Number: [[#]] (DW_TAG_compile_unit)
// CHECK-NEXT: DW_AT_producer : clang version
// CHECK-NEXT: DW_AT_language : 21 (OpenCL)
// CHECK-NEXT: DW_AT_name : local-variable-location.cl
// CHECK-NEXT: DW_AT_low_pc :
// CHECK-NEXT: DW_AT_high_pc :
// CHECK-NEXT: DW_AT_stmt_list :
// CHECK-DG2-NEXT: DW_AT_INTEL_simd_width: 8
// CHECK-CRI-NEXT: DW_AT_INTEL_simd_width: 16

// CHECK: Abbrev Number: [[#]] (DW_TAG_subprogram)
// CHECK-NEXT: DW_AT_name : foo
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE:]]
// CHECK-NEXT: DW_AT_decl_line : 33
// CHECK-DG2-NEXT: DW_AT_INTEL_simd_width: 8
// CHECK-CRI-NEXT: DW_AT_INTEL_simd_width: 16
// CHECK-NEXT: DW_AT_external : 1
// CHECK-NEXT: DW_AT_low_pc :
// CHECK-NEXT: DW_AT_high_pc :

// FP based foo_param.
// CHECK: Abbrev Number: [[#]] (DW_TAG_formal_parameter)
// CHECK-NEXT: DW_AT_name : foo_param
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 33
// CHECK-NEXT: DW_AT_type : <0x[[#%x,SLMPTR_TYPE:]]>
// CHECK-NEXT: DW_AT_location : {{.+}} (location list)

// FP based foo_param1.
// CHECK: Abbrev Number: [[#]] (DW_TAG_formal_parameter)
// CHECK-NEXT: DW_AT_name : foo_param1
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 33
// CHECK-NEXT: DW_AT_type :
// CHECK-NEXT: DW_AT_location : {{.+}} (location list)

// Const gb_float.
// CHECK: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK-NEXT: DW_AT_name : gb_float
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 25
// CHECK-NEXT: DW_AT_type :
// CHECK-NEXT: DW_AT_const_value : 0x47c44a00

// Add check for gb_char_ar when supported.

// SLM lc_ptr variable.
// CHECK: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK-NEXT: DW_AT_name : lc_ptr
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 34
// CHECK-NEXT: DW_AT_type :
// CHECK-NEXT: DW_AT_address_class: 1
// CHECK-NEXT: DW_AT_location : [[#]] byte block: 3 {{.+}} (DW_OP_addr: 1{{.+}})

// SLM lc_float variable.
// CHECK: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK-NEXT: DW_AT_name : lc_float
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 35
// CHECK-NEXT: DW_AT_type :
// CHECK-NEXT: DW_AT_address_class: 1
// CHECK-NEXT: DW_AT_location : [[#]] byte block: 3 {{.+}} (DW_OP_addr: 1{{.+}})

// SLM foo_ptr variable.
// CHECK: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK-NEXT: DW_AT_name : foo_ptr
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 37
// CHECK-NEXT: DW_AT_type :
// CHECK-NEXT: DW_AT_address_class: 1
// CHECK-NEXT: DW_AT_location : [[#]] byte block: 3 {{.+}} (DW_OP_addr: 1{{.+}})

// CHECK: Abbrev Number: [[#]] (DW_TAG_subprogram)
// CHECK-NEXT: DW_AT_name : f1
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 28
// CHECK-DG2-NEXT: DW_AT_INTEL_simd_width: 8
// CHECK-CRI-NEXT: DW_AT_INTEL_simd_width: 16
// CHECK-NEXT: DW_AT_type : <0x[[#SLMPTR_TYPE]]>

// FP based f1_arg1.
// CHECK: Abbrev Number: [[#]] (DW_TAG_formal_parameter)
// CHECK-NEXT: DW_AT_name : f1_arg1
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 28
// CHECK-NEXT: DW_AT_type :
// CHECK-NEXT: DW_AT_location : {{.+}} (location list)

// Const gb_float.
// CHECK: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK-NEXT: DW_AT_name : gb_float
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 25
// CHECK-NEXT: DW_AT_type :
// CHECK-NEXT: DW_AT_const_value : 0x47c44a00

// FP based f1_ptr.
// CHECK-DAG: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK: DW_AT_name : f1_ptr
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 29
// CHECK-NEXT: DW_AT_type : <0x[[#SLMPTR_TYPE]]>
// CHECK-NEXT: DW_AT_location : {{.+}} (location list)

// Expect SLM pointer type to has DW_AT_address_class flag.
// CHECK: <[[#]]><[[#SLMPTR_TYPE]]>: Abbrev Number: [[#]] (DW_TAG_pointer_type)
// CHECK-NEXT: DW_AT_type :
// CHECK-NEXT: DW_AT_address_class: 1

// CHECK: The File Name Table (offset {{.+}}):
// CHECK: [[#DECL_FILE]] [[#]] 0 0 local-variable-location.cl
//
// CHECK-DG2: .debug_loc
// CHECK-DG2: (DW_OP_const1u: 143; DW_OP_const1u: 128; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: 16; DW_OP_INTEL_push_simd_lane; DW_OP_lit8; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])
// CHECK-DG2: (DW_OP_const1u: 143; DW_OP_const1u: 128; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: 16; DW_OP_INTEL_push_simd_lane; DW_OP_lit4; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])
// CHECK-CRI: .debug_loc
// CHECK-CRI: (DW_OP_const1u: 143; DW_OP_const2u: 256; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: 16; DW_OP_INTEL_push_simd_lane; DW_OP_lit8; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])
// CHECK-CRI: (DW_OP_const1u: 143; DW_OP_const2u: 256; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: 16; DW_OP_INTEL_push_simd_lane; DW_OP_lit4; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])
