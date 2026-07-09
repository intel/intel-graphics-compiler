//========================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
//=========================== end_copyright_notice =============================

// Verify that variables with constant values have correct constant value emitted
// In OpenCL, verify that `const uint lc_const_uint = 1357` results in either constant value (-O2) or a DW_AT_location expression (in -O0).

// UNSUPPORTED: sys32, lib-igc-clang
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
// RUN: FileCheck %s --check-prefixes=CHECK,CHECKO0,CHECKO0-DG2,%if llvm-22-plus %{CHECKO0-LLVM22%} %else %{CHECKO0-PRELLVM22%} %}

// RUN: %if dg2-supported %{ ocloc compile -file %s -options " -g -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_dg2_'" -device dg2 %}
// RUN: %if dg2-supported %{ oneapi-readelf --debug-dump %t_dg2_OCL_simd32_foo.elf | \
// RUN: FileCheck %s --check-prefixes=CHECK,CHECKO2 %}

// RUN: %if cri-supported %{ ocloc compile -file %s -options " -g -cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_cri_, ShortImplicitPayloadHeader=0, RemoveUnusedIdImplicitArguments=0'" -device cri %}
// RUN: %if cri-supported %{ oneapi-readelf --debug-dump %t_cri_OCL_simd16_foo.elf | \
// RUN: FileCheck %s --check-prefixes=CHECK,CHECKO0,CHECKO0-CRI,%if llvm-22-plus %{CHECKO0-LLVM22%} %else %{CHECKO0-PRELLVM22%} %}

// RUN: %if cri-supported %{ ocloc compile -file %s -options " -g -igc_opts 'EnableOpaquePointersBackend=1, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_cri_, ShortImplicitPayloadHeader=0, RemoveUnusedIdImplicitArguments=0'" -device cri %}
// RUN: %if cri-supported %{ oneapi-readelf --debug-dump %t_cri_OCL_simd32_foo.elf | \
// RUN: FileCheck %s --check-prefixes=CHECK,CHECKO2 %}

// CHECK: Abbrev Number: [[#]] (DW_TAG_compile_unit)
// CHECK: DW_AT_name : constant-variable-location.cl
// CHECKO0-DG2: DW_AT_INTEL_simd_width: 8
// CHECKO0-CRI: DW_AT_INTEL_simd_width: 16
// CHECKO2: DW_AT_INTEL_simd_width: 32

// CHECK: DW_TAG_subprogram
// CHECK: DW_AT_name : foo
// CHECK: DW_AT_decl_file : [[#DECL_FILE:]]

// The file-scope constants appear as children of the kernel entry and,
// at -O0, again in the __clang_ocl_kern_imp_foo body; their order and duplication
// vary by opt level, so match each by name and value independently.

// CHECK-DAG: DW_AT_name : gb_float
// CHECK-DAG: DW_AT_const_value : 0x47c44a00
// CHECK-DAG: DW_AT_name : gb_char_ar
// CHECK-DAG: DW_AT_const_value : 4 byte block: be ef ca fe

// Note: lc_int gets no DIE, as clang folds every use to an immediate

// CHECKO2-DAG: DW_AT_name : lc_const_uint
// CHECKO2-DAG: DW_AT_const_value : 0x54d

// CHECKO0: DW_AT_name : lc_ptr
// CHECKO0-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECKO0-NEXT: DW_AT_decl_line : 29
// CHECKO0-NEXT: DW_AT_type : <0x[[#%x,LC_PTR_TYPE:]]>
// CHECKO0-NEXT: DW_AT_location : 0x[[#%x,LC_PTR_LOC:]] (location list)
// CHECKO0-NEXT: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECKO0-NEXT: DW_AT_name : lc_const_uint
// CHECKO0-NEXT: DW_AT_decl_file : [[#DECL_FILE]]
// CHECKO0-NEXT: DW_AT_decl_line : 30
// CHECKO0-NEXT: DW_AT_type : <0x[[#%x,LC_CONST_UINT_TYPE:]]>
// CHECKO0-NEXT: DW_AT_location : 0x[[#%x,LC_CONST_UINT_LOC:]] (location list)

// CHECK: The File Name Table (offset {{.+}}):
// CHECK: [[#DECL_FILE]] [[#]] 0 0 constant-variable-location.cl

// CHECKO0: Contents of the .debug_loc section:
// On LLVM 22, bacause of the impl stackcall, it's a stack based location
// on older LLVM there's no stackcall and it's a private-base location
// CHECKO0-LLVM22-DAG: {{0+}}[[#%x,LC_PTR_LOC]] {{.+}} (DW_OP_const1u: 143; DW_OP_const[[#]]u: [[#]]; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: [[#]]; DW_OP_INTEL_push_simd_lane; DW_OP_lit8; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])
// CHECKO0-LLVM22-DAG: {{0+}}[[#%x,LC_CONST_UINT_LOC]] {{.+}} (DW_OP_const1u: 143; DW_OP_const[[#]]u: [[#]]; DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: [[#]]; DW_OP_INTEL_push_simd_lane; DW_OP_lit4; DW_OP_mul; DW_OP_plus; DW_OP_plus_uconst: [[#]])
// CHECKO0-PRELLVM22-DAG: {{0+}}[[#%x,LC_PTR_LOC]] {{.+}} (DW_OP_lit[[#]]; DW_OP_const1u: 128; DW_OP_INTEL_regval_bits: 64; DW_OP_constu: [[#]]; DW_OP_const[[#]]u: [[#]]; DW_OP_INTEL_regval_bits: 32; DW_OP_plus; DW_OP_plus_uconst: [[#]]; DW_OP_INTEL_push_simd_lane; DW_OP_lit8; DW_OP_mul; DW_OP_plus)
// CHECKO0-PRELLVM22-DAG: {{0+}}[[#%x,LC_CONST_UINT_LOC]] {{.+}} (DW_OP_lit[[#]]; DW_OP_const1u: 128; DW_OP_INTEL_regval_bits: 64; DW_OP_constu: [[#]]; DW_OP_const[[#]]u: [[#]]; DW_OP_INTEL_regval_bits: 32; DW_OP_plus; DW_OP_plus_uconst: [[#]]; DW_OP_INTEL_push_simd_lane; DW_OP_lit4; DW_OP_mul; DW_OP_plus)
