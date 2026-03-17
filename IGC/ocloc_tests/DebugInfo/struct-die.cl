//========================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
//=========================== end_copyright_notice =============================

// UNSUPPORTED: sys32
// REQUIRES: oneapi-readelf

// For this scenario we should verify that all members of a struct are emitted to DWARF

struct struct1 {
  int struct1_int_field;
  float struct1_float_field;
  float4 struct1_float4_field;
  const int struct1_const_int_field;
  int *struct1_int_ptr_field;
  float4 struct1_float4_array_field[5];
};

struct struct2;

typedef struct {
  struct struct2 *struct3_ptr_field;
} struct3;

struct struct4 {
  struct struct4 *struct4_self_ptr;
};

__kernel void test(int i) {
  struct struct1 s1;
  global struct3 *s3;
  local struct struct4 s4;
  s4.struct4_self_ptr = 0;
}

// RUN: %if dg2-supported %{ ocloc compile -file %s -options " -g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_dg2_'" -device dg2 %}
// RUN: %if dg2-supported %{ oneapi-readelf --debug-dump %t_dg2_OCL_simd8_test.elf |\
// RUN: FileCheck %s --check-prefixes=CHECK,CHECK-DG2 %}

// RUN: %if cri-supported %{ ocloc compile -file %s -options " -g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_cri_'" -device cri %}
// RUN: %if cri-supported %{ oneapi-readelf --debug-dump %t_cri_OCL_simd16_test.elf |\
// RUN: FileCheck %s --check-prefixes=CHECK,CHECK-CRI %}

// CHECK: Abbrev Number: [[#]] (DW_TAG_compile_unit)
// CHECK-NEXT: DW_AT_producer : clang version
// CHECK-NEXT: DW_AT_language : 21 (OpenCL)
// CHECK-NEXT: DW_AT_name : struct-die.cl
// CHECK-NEXT: DW_AT_low_pc :
// CHECK-NEXT: DW_AT_high_pc :
// CHECK-NEXT: DW_AT_stmt_list :
// CHECK-DG2-NEXT: DW_AT_INTEL_simd_width: 8
// CHECK-CRI-NEXT: DW_AT_INTEL_simd_width: 16

// CHECK: Abbrev Number: [[#]] (DW_TAG_subprogram)
// CHECK-NEXT: DW_AT_name : test
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE:]]
// CHECK-NEXT: DW_AT_decl_line : 42
// CHECK-DG2-NEXT: DW_AT_INTEL_simd_width: 8
// CHECK-CRI-NEXT: DW_AT_INTEL_simd_width: 16
// CHECK-NEXT: DW_AT_external : 1
// CHECK-NEXT: DW_AT_low_pc :
// CHECK-NEXT: DW_AT_high_pc :

// CHECK: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK-NEXT: DW_AT_name : s4
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 45
// CHECK-NEXT: DW_AT_type : <0x[[#%x,STRUCT4_TYPE:]]>
// CHECK-NEXT: DW_AT_address_class: 1
// CHECK-NEXT: DW_AT_location : [[#]] byte block: 3 {{.+}} (DW_OP_addr: [[#]])

// CHECK: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK-NEXT: DW_AT_name : s1
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 43
// CHECK-NEXT: DW_AT_type : <0x[[#%x,STRUCT1_TYPE:]]>

// CHECK: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK-NEXT: DW_AT_name : s3
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 44
// CHECK-NEXT: DW_AT_type : <0x[[#%x,STRUCT3_PTR_TYPE:]]>

// CHECK: <[[#%x,INT_TYPE:]]>: Abbrev Number: [[#]] (DW_TAG_base_type)
// CHECK-NEXT: DW_AT_name : int
// CHECK-NEXT: DW_AT_encoding : [[#]] (signed)
// CHECK-NEXT: DW_AT_byte_size : [[#INT_TYPE_SIZE:4]]

// CHECK: <[[#LV:1]]><[[#STRUCT4_TYPE]]>: Abbrev Number: [[#]] (DW_TAG_structure_type)
// CHECK-NEXT: DW_AT_name : struct4
// CHECK-NEXT: DW_AT_byte_size : 8
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 38

// CHECK: <[[#LV+1]]><[[#%x,]]>: Abbrev Number: [[#]] (DW_TAG_member)
// CHECK-NEXT: DW_AT_name : struct4_self_ptr
// CHECK-NEXT: DW_AT_type : <0x[[#%x,STRUCT4_PTR_TYPE:]]>
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 39
// CHECK-NEXT: DW_AT_data_member_location: 0
// CHECK-NEXT: DW_AT_accessibility: 1 (public)

// CHECK: <[[#STRUCT4_PTR_TYPE]]>: Abbrev Number: [[#]] (DW_TAG_pointer_type)
// CHECK-NEXT: DW_AT_type : <0x[[#STRUCT4_TYPE]]>

// CHECK: <[[#LV:1]]><[[#STRUCT1_TYPE]]>: Abbrev Number: [[#]] (DW_TAG_structure_type)
// CHECK-NEXT: DW_AT_name : struct1
// CHECK-NEXT: DW_AT_byte_size : 128
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 23

// CHECK: <[[#LV+1]]><[[#%x,]]>: Abbrev Number: [[#]] (DW_TAG_member)
// CHECK-NEXT: DW_AT_name : struct1_int_field
// CHECK-NEXT: DW_AT_type : <0x[[#INT_TYPE]]>
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 24
// CHECK-NEXT: DW_AT_data_member_location: 0
// CHECK-NEXT: DW_AT_accessibility: 1 (public)

// CHECK: <[[#LV+1]]><[[#%x,]]>: Abbrev Number: [[#]] (DW_TAG_member)
// CHECK-NEXT: DW_AT_name : struct1_float_field
// CHECK-NEXT: DW_AT_type : <0x[[#%x,FLOAT_TYPE:]]>
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 25
// CHECK-NEXT: DW_AT_data_member_location: 4
// CHECK-NEXT: DW_AT_accessibility: 1 (public)

// CHECK: <[[#LV+1]]><[[#%x,]]>: Abbrev Number: [[#]] (DW_TAG_member)
// CHECK-NEXT: DW_AT_name : struct1_float4_field
// CHECK-NEXT: DW_AT_type : <0x[[#%x,FLOAT4_TYPE:]]>
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 26
// CHECK-NEXT: DW_AT_data_member_location: 16
// CHECK-NEXT: DW_AT_accessibility: 1 (public)

// CHECK: <[[#LV+1]]><[[#%x,]]>: Abbrev Number: [[#]] (DW_TAG_member)
// CHECK-NEXT: DW_AT_name : struct1_const_int_field
// CHECK-NEXT: DW_AT_type : <0x[[#%x,CONST_INT_TYPE:]]>
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 27
// CHECK-NEXT: DW_AT_data_member_location: 32
// CHECK-NEXT: DW_AT_accessibility: 1 (public)

// CHECK: <[[#LV+1]]><[[#%x,]]>: Abbrev Number: [[#]] (DW_TAG_member)
// CHECK-NEXT: DW_AT_name : struct1_int_ptr_field
// CHECK-NEXT: DW_AT_type : <0x[[#%x,INT_PTR_TYPE:]]>
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 28
// CHECK-NEXT: DW_AT_data_member_location: 40
// CHECK-NEXT: DW_AT_accessibility: 1 (public)

// CHECK: <[[#LV+1]]><[[#%x,]]>: Abbrev Number: [[#]] (DW_TAG_member)
// CHECK-NEXT: DW_AT_name : struct1_float4_array_field
// CHECK-NEXT: DW_AT_type : <0x[[#%x,FLOAT4_ARRAY_TYPE:]]>
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 29
// CHECK-NEXT: DW_AT_data_member_location: 48
// CHECK-NEXT: DW_AT_accessibility: 1 (public)

// CHECK: <[[#FLOAT_TYPE]]>: Abbrev Number: [[#]] (DW_TAG_base_type)
// CHECK-NEXT: DW_AT_name : float
// CHECK-NEXT: DW_AT_encoding : [[#]] (float)
// CHECK-NEXT: DW_AT_byte_size : [[#FLOAT_TYPE_SIZE:4]]

// CHECK: <[[#FLOAT4_TYPE]]>: Abbrev Number: [[#]] (DW_TAG_typedef)
// CHECK-NEXT: DW_AT_type : <0x[[#%x,FLOAT_ARRAY_TYPE:]]>
// CHECK-NEXT: DW_AT_name : float4

// CHECK: <[[#FLOAT_ARRAY_TYPE]]>: Abbrev Number: [[#]] (DW_TAG_array_type)
// CHECK-NEXT: DW_AT_GNU_vector : 1
// CHECK-NEXT: DW_AT_type : <0x[[#FLOAT_TYPE]]>
// CHECK-NEXT: Abbrev Number: [[#]] (DW_TAG_subrange_type)
// CHECK-NEXT: DW_AT_type : <0x[[#%x,RANGE_INT_TYPE:]]>
// CHECK-NEXT: DW_AT_lower_bound : 0
// CHECK-NEXT: DW_AT_count : 4

// CHECK: <[[#RANGE_INT_TYPE]]>: Abbrev Number: [[#]] (DW_TAG_base_type)
// CHECK-NEXT: DW_AT_name : int
// CHECK-NEXT: DW_AT_byte_size : 4
// CHECK-NEXT: DW_AT_encoding : 5 (signed)

// CHECK: <[[#CONST_INT_TYPE]]>: Abbrev Number: [[#]] (DW_TAG_const_type)
// CHECK-NEXT: DW_AT_type : <0x[[#INT_TYPE]]>

// CHECK: <[[#INT_PTR_TYPE]]>: Abbrev Number: [[#]] (DW_TAG_pointer_type)
// CHECK-NEXT: DW_AT_type : <0x[[#INT_TYPE]]>

// CHECK: <[[#FLOAT4_ARRAY_TYPE]]>: Abbrev Number: [[#]] (DW_TAG_array_type)
// CHECK-NEXT: DW_AT_type : <0x[[#FLOAT4_TYPE]]>
// CHECK-NEXT: Abbrev Number: [[#]] (DW_TAG_subrange_type)
// CHECK-NEXT: DW_AT_type : <0x[[#RANGE_INT_TYPE]]>
// CHECK-NEXT: DW_AT_lower_bound : 0
// CHECK-NEXT: DW_AT_count : 5

// CHECK: <[[#STRUCT3_PTR_TYPE]]>: Abbrev Number: [[#]] (DW_TAG_pointer_type)
// CHECK-NEXT: DW_AT_type : <0x[[#%x,STRUCT3_TYPEDEF:]]>

// CHECK: <[[#STRUCT3_TYPEDEF]]>: Abbrev Number: [[#]] (DW_TAG_typedef)
// CHECK-NEXT: DW_AT_type : <0x[[#%x,STRUCT3_TYPE:]]>
// CHECK-NEXT: DW_AT_name : struct3

// CHECK: <[[#LV:1]]><[[#STRUCT3_TYPE]]>: Abbrev Number: [[#]] (DW_TAG_structure_type)
// CHECK-NEXT: DW_AT_byte_size : 8
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 34

// CHECK: <[[#LV+1]]><[[#%x,]]>: Abbrev Number: [[#]] (DW_TAG_member)
// CHECK-NEXT: DW_AT_name : struct3_ptr_field
// CHECK-NEXT: DW_AT_type : <0x[[#%x,STRUCT2_PTR_TYPE:]]>
// CHECK-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECK-NEXT: DW_AT_decl_line : 35
// CHECK-NEXT: DW_AT_data_member_location: 0
// CHECK-NEXT: DW_AT_accessibility: 1 (public)

// CHECK: <[[#STRUCT2_PTR_TYPE]]>: Abbrev Number: [[#]] (DW_TAG_pointer_type)
// CHECK-NEXT: DW_AT_type : <0x[[#%x,STRUCT2_TYPE:]]>

// CHECK: <1><[[#STRUCT2_TYPE]]>: Abbrev Number: [[#]] (DW_TAG_structure_type)
// CHECK-NEXT: DW_AT_name : struct2
// CHECK-NEXT: DW_AT_declaration : 1

// CHECK: The File Name Table (offset {{.+}}):
// CHECK: [[#DECL_FILE]] [[#]] 0 0 struct-die.cl
