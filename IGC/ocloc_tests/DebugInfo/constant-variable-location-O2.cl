//========================== begin_copyright_notice ============================
//
// Copyright (C) 2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
//=========================== end_copyright_notice =============================

// Verify that variable with constant value will have inlined location and parameter
// passsed in register will have an entry in location list for the O2 compilation.

// UNSUPPORTED: sys32
// REQUIRES: oneapi-readelf

__kernel void foo(__global int* result) {
    const int sum = 4;
    result[0] = sum;
}

// RUN: %if dg2-supported %{ ocloc compile -file %s -options " -g -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_dg2_'" -device dg2 %}
// RUN: %if dg2-supported %{ oneapi-readelf --debug-dump %t_dg2_OCL_simd32_foo.elf | \
// RUN: FileCheck %s --check-prefixes=CHECK,%if lib-igc-clang && !llvm-22-plus %{CHECK-IGC-CLANG%} %else %{CHECK-DIRECT%} %}

// RUN: %if cri-supported %{ ocloc compile -file %s -options " -g -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_cri_, ShortImplicitPayloadHeader=0'" -device cri %}
// RUN: %if cri-supported %{ oneapi-readelf --debug-dump %t_cri_OCL_simd32_foo.elf | \
// RUN: FileCheck %s --check-prefixes=CHECK,%if lib-igc-clang && !llvm-22-plus %{CHECK-IGC-CLANG%} %else %{CHECK-DIRECT%} %}

// CHECK: DW_AT_name : result
// CHECK-NEXT: DW_AT_decl_file :
// CHECK-NEXT: DW_AT_decl_line :
// CHECK-NEXT: DW_AT_type :
// CHECK-NEXT: DW_AT_location : {{(0x)?}}[[RESULT_LOC:[0-9a-f]+]] (location list)

// CHECK-DIRECT: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK-DIRECT-NEXT: DW_AT_name : sum
// CHECK-DIRECT-NEXT: DW_AT_decl_file :
// CHECK-DIRECT-NEXT: DW_AT_decl_line :
// CHECK-DIRECT-NEXT: DW_AT_type :
// CHECK-DIRECT-NEXT: DW_AT_const_value : 4

// On pre-LLVM-22 backends, newer clang front-ends describe the constant in an
// inlined instance. Verify that its value refers to the named abstract
// definition for sum. From LLVM 22 on, __clang_ocl_kern_imp_foo is inlined into
// the kernel entry and sum is emitted directly again, so CHECK-DIRECT applies.
// CHECK-IGC-CLANG: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK-IGC-CLANG-NEXT: DW_AT_abstract_origin: <0x[[SUM_DIE:[0-9a-f]+]]>
// CHECK-IGC-CLANG-NEXT: DW_AT_const_value : 4
// CHECK-IGC-CLANG: <[[#]]><[[SUM_DIE]]>: Abbrev Number: [[#]] (DW_TAG_variable)
// CHECK-IGC-CLANG-NEXT: DW_AT_name : sum
// CHECK-IGC-CLANG-NEXT: DW_AT_decl_file :
// CHECK-IGC-CLANG-NEXT: DW_AT_decl_line :
// CHECK-IGC-CLANG-NEXT: DW_AT_type :

// CHECK: Contents of the .debug_loc section:
// CHECK: [[RESULT_LOC]] {{[^(]+}}(DW_OP_lit[[#]]; DW_OP_{{lit|const1u: }}[[#]]; DW_OP_INTEL_regval_bits: 64; DW_OP_stack_value)
