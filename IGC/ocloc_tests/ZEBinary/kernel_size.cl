/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This test checks if the size of kernel in debug info and .symtab match when there is a stack call function call in the kernel.

// UNSUPPORTED: sys32
// REQUIRES: pvc-supported, oneapi-readelf

// RUN: ocloc compile -file %s -options "-cl-opt-disable -g" -device pvc -output %t -output_no_suffix
// RUN: oneapi-readelf %t.bin -Ws --sym-base=16 > %t.out
// RUN: oneapi-readelf %t.bin --debug-dump --sym-base=16 >> %t.out
// RUN: cat %t.out | FileCheck %s

// CHECK: Symbol table '.symtab'
// CHECK: {{0+}}[[KERNEL_SIZE:[0-9a-fA-F]+]] 0x[[HELPER_SIZE:[0-9a-fA-F]+]] FUNC    LOCAL  DEFAULT    1 helper
// CHECK-NEXT:  0000000000000000 0x{{0*}}[[KERNEL_SIZE]] FUNC    LOCAL  DEFAULT    1 _kernel

// CHECK: Abbrev Number: 1 (DW_TAG_compile_unit)
// CHECK-NEXT: DW_AT_producer
// CHECK-NEXT: DW_AT_language
// CHECK-NEXT: DW_AT_name
// CHECK-NEXT: DW_AT_low_pc       : 0
// CHECK-NEXT: DW_AT_high_pc      : 0x[[WHOLE_SIZE:[0-9a-fA-F]+]]

// CHECK: Abbrev Number: {{[0-9]+}} (DW_TAG_subprogram)
// CHECK-NEXT: DW_AT_name        :  _kernel
// CHECK-NEXT: DW_AT_decl_file
// CHECK-NEXT: DW_AT_decl_line
// CHECK-NEXT: DW_AT_INTEL_simd_width
// CHECK-NEXT: DW_AT_external
// CHECK-NEXT: DW_AT_low_pc      :  0
// CHECK-NEXT: DW_AT_high_pc     :  0x[[KERNEL_SIZE]]

// CHECK: Abbrev Number: {{[0-9]+}} (DW_TAG_subprogram)
// CHECK-NEXT: DW_AT_name        :  helper
// CHECK-NEXT: DW_AT_decl_file
// CHECK-NEXT: DW_AT_decl_line
// CHECK-NEXT: DW_AT_INTEL_simd_width
// CHECK-NEXT: DW_AT_type
// CHECK-NEXT: DW_AT_external
// CHECK-NEXT: DW_AT_low_pc      :  0x[[KERNEL_SIZE]]
// CHECK-NEXT: DW_AT_high_pc     :  0x[[WHOLE_SIZE]]

int helper(int x) {
    return x + 1;
}

kernel void _kernel(global int* out) {
    out[0] = helper(out[0]);
}
