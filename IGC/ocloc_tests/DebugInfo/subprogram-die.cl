//========================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
//=========================== end_copyright_notice =============================

// For this scenario we want to check whether following are present in DWARF (kernel+stack call functions):
// <1><5c>: Abbrev Number: 2 (DW_TAG_subprogram)
//     <5d>   DW_AT_name        : GodRays
//     <65>   DW_AT_decl_file   : 1
//     <66>   DW_AT_decl_line   : 646
//     <68>   DW_AT_INTEL_simd_width: 8
//     <6a>   DW_AT_external    : 1
//     <6a>   DW_AT_low_pc      : 0x0
//     <72>   DW_AT_high_pc     : 0xe40
// ...
//  <1><156>: Abbrev Number: 5 (DW_TAG_subprogram)
//     <157>   DW_AT_name        : EvaluateRay
//     <163>   DW_AT_decl_file   : 1
//     <164>   DW_AT_decl_line   : 27
//     <165>   DW_AT_INTEL_simd_width: 8
//     <167>   DW_AT_external    : 1
//     <167>   DW_AT_low_pc      : 0xe40
//     <16f>   DW_AT_high_pc     : 0x13340
// DW_AT_low/high_pc can be checked for presence, their values need not be checked.
// Along with DW_TAG_subprogram we should also check for presence of formal arguments and local variables.
// No need to check for actual location, just presence of all DIEs and presence of DW_AT_location is enough.

// UNSUPPORTED: sys32
// REQUIRES: oneapi-readelf

extern int extern_func_decl(global int *in);

private int *f1(private int *f1_arg1) {
  private int f1_local_int = 10;
  *f1_arg1 += f1_local_int;
  return f1_arg1;
}

static void f2(local char *f2_arg1) {
  const char f2_local_char = 42;
  f2_arg1[4] += f2_local_char;
}

__kernel void foo(local int *out) {
  const int foo_const_int = extern_func_decl(0);
  private int foo_int_array[] = {0, foo_const_int, 0};
  f2(0);
  *out = *f1(foo_int_array);
}

// RUN: %if dg2-supported %{ ocloc compile -file %s -options " -g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_dg2_'" -device dg2 %}
// RUN: %if dg2-supported %{ oneapi-readelf --debug-dump %t_dg2_OCL_simd8_foo.elf &> %t_dg2_OCL_simd8_foo.dwarf %}
// RUN: %if dg2-supported %{ FileCheck %s --input-file %t_dg2_OCL_simd8_foo.dwarf --check-prefixes=CHECK,CHECK-DG2,CHECK-KERNEL,CHECK-DG2-KERNEL %}
// RUN: %if dg2-supported %{ FileCheck %s --input-file %t_dg2_OCL_simd8_foo.dwarf --check-prefixes=CHECK,CHECK-DG2,CHECK-F1,CHECK-DG2-F1 %}
// RUN: %if dg2-supported %{ FileCheck %s --input-file %t_dg2_OCL_simd8_foo.dwarf --check-prefixes=CHECK,CHECK-DG2,CHECK-F2,CHECK-DG2-F2 %}

// RUN: %if cri-supported %{ ocloc compile -file %s -options " -g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_cri_'" -device cri %}
// RUN: %if cri-supported %{ oneapi-readelf --debug-dump %t_cri_OCL_simd16_foo.elf &> %t_cri_OCL_simd16_foo.dwarf %}
// RUN: %if cri-supported %{ FileCheck %s --input-file %t_cri_OCL_simd16_foo.dwarf --check-prefixes=CHECK,CHECK-CRI,CHECK-KERNEL,CHECK-CRI-KERNEL %}
// RUN: %if cri-supported %{ FileCheck %s --input-file %t_cri_OCL_simd16_foo.dwarf --check-prefixes=CHECK,CHECK-CRI,CHECK-F1,CHECK-CRI-F1 %}
// RUN: %if cri-supported %{ FileCheck %s --input-file %t_cri_OCL_simd16_foo.dwarf --check-prefixes=CHECK,CHECK-CRI,CHECK-F2,CHECK-CRI-F2 %}

// CHECK: Abbrev Number: [[#]] (DW_TAG_compile_unit)
// CHECK-NEXT: DW_AT_producer : clang version
// CHECK-NEXT: DW_AT_language : 21 (OpenCL)
// CHECK-NEXT: DW_AT_name : subprogram-die.cl
// CHECK-DG2: DW_AT_INTEL_simd_width: 8
// CHECK-CRI: DW_AT_INTEL_simd_width: 16
// CHECK-NOT: Abbrev Number

// CHECK-DAG: <[[#LV:1]]><[[#%x,]]>: Abbrev Number: [[#]] (DW_TAG_subprogram)
// CHECK-KERNEL: DW_AT_name : foo
// CHECK-KERNEL-NEXT: DW_AT_decl_file : [[#DECL_FILE:]]
// CHECK-KERNEL-NEXT: DW_AT_decl_line : 56
// CHECK-DG2-KERNEL-NEXT: DW_AT_INTEL_simd_width: 8
// CHECK-CRI-KERNEL-NEXT: DW_AT_INTEL_simd_width: 16
// CHECK-KERNEL-NEXT: DW_AT_external : 1
// CHECK-KERNEL-NEXT: DW_AT_low_pc :
// CHECK-KERNEL-NEXT: DW_AT_high_pc :

// CHECK-KERNEL: <[[#LV+1]]><[[#%x,]]>: Abbrev Number: [[#]] (DW_TAG_formal_parameter)
// CHECK-KERNEL-NEXT: DW_AT_name : out
// CHECK-KERNEL-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-KERNEL-NEXT: DW_AT_decl_line : 56
// CHECK-KERNEL-NEXT: DW_AT_type :
// CHECK-KERNEL-NEXT: DW_AT_location :

// CHECK-KERNEL: <[[#LV+1]]><[[#%x,]]>: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK-KERNEL-NEXT: DW_AT_name : foo_const_int
// CHECK-KERNEL-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-KERNEL-NEXT: DW_AT_decl_line : 57
// CHECK-KERNEL-NEXT: DW_AT_type :
// CHECK-KERNEL-NEXT: DW_AT_location :

// CHECK-KERNEL: <[[#LV+1]]><[[#%x,]]>: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK-KERNEL-NEXT: DW_AT_name : foo_int_array
// CHECK-KERNEL-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-KERNEL-NEXT: DW_AT_decl_line : 58
// CHECK-KERNEL-NEXT: DW_AT_type :
// CHECK-KERNEL-NEXT: DW_AT_location :

// CHECK-F1-DAG: <[[#LV:1]]><[[#%x,]]>: Abbrev Number: [[#]] (DW_TAG_subprogram)
// CHECK-F1: DW_AT_name : f1
// CHECK-F1-NEXT: DW_AT_decl_file : [[#DECL_FILE:]]
// CHECK-F1-NEXT: DW_AT_decl_line : 45
// CHECK-DG2-F1-NEXT: DW_AT_INTEL_simd_width: 8
// CHECK-CRI-F1-NEXT: DW_AT_INTEL_simd_width: 16
// CHECK-F1-NEXT: DW_AT_type :
// CHECK-F1-NEXT: DW_AT_external : 1
// CHECK-F1-NEXT: DW_AT_low_pc :
// CHECK-F1-NEXT: DW_AT_high_pc :

// CHECK-F1: <[[#LV+1]]><[[#%x,]]>: Abbrev Number: [[#]] (DW_TAG_formal_parameter)
// CHECK-F1-NEXT: DW_AT_name : f1_arg1
// CHECK-F1-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-F1-NEXT: DW_AT_decl_line : 45
// CHECK-F1-NEXT: DW_AT_type :
// CHECK-F1-NEXT: DW_AT_location :

// CHECK-F1: <[[#LV+1]]><[[#%x,]]>: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK-F1-NEXT: DW_AT_name : f1_local_int
// CHECK-F1-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-F1-NEXT: DW_AT_decl_line : 46
// CHECK-F1-NEXT: DW_AT_type :
// CHECK-F1-NEXT: DW_AT_location :

// CHECK-F2-DAG: <[[#LV:1]]><[[#%x,]]>: Abbrev Number: [[#]] (DW_TAG_subprogram)
// CHECK-F2: DW_AT_name : f2
// CHECK-F2-NEXT: DW_AT_decl_file : [[#DECL_FILE:]]
// CHECK-F2-NEXT: DW_AT_decl_line : 51
// CHECK-DG2-F2-NEXT: DW_AT_INTEL_simd_width: 8
// CHECK-CRI-F2-NEXT: DW_AT_INTEL_simd_width: 16
// CHECK-F2-NEXT: DW_AT_low_pc :
// CHECK-F2-NEXT: DW_AT_high_pc :
// CHECK-F2-NOT: DW_AT_external

// CHECK-F2: <[[#LV+1]]><[[#%x,]]>: Abbrev Number: [[#]] (DW_TAG_formal_parameter)
// CHECK-F2-NEXT: DW_AT_name : f2_arg1
// CHECK-F2-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-F2-NEXT: DW_AT_decl_line : 51
// CHECK-F2-NEXT: DW_AT_type :
// CHECK-F2-NEXT: DW_AT_location :

// CHECK-F2: <[[#LV+1]]><[[#%x,]]>: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK-F2-NEXT: DW_AT_name : f2_local_char
// CHECK-F2-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-F2-NEXT: DW_AT_decl_line : 52
// CHECK-F2-NEXT: DW_AT_type :
// CHECK-F2-NEXT: DW_AT_location :

// CHECK: The File Name Table (offset {{.+}}):
// CHECK: [[#DECL_FILE]] [[#]] 0 0 subprogram-die.cl
