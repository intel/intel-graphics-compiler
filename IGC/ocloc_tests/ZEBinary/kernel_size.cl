/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This test checks if the size of kernel in debug info and .symtab match when there is a stack call function call in the kernel.

// On clang >= 22 a kernel is a thin wrapper that calls the actual
// implementation (__clang_ocl_kern_imp__kernel), so an extra FUNC symbol and
// DW_TAG_subprogram appear between _kernel and helper. Layout: [_kernel][impl][helper].
// Uses `FunctionControl=3` to ensure stackcalls.

// UNSUPPORTED: sys32
// REQUIRES: pvc-supported, oneapi-readelf, regkeys

// RUN: ocloc compile -file %s -options "-cl-opt-disable -g" -device pvc -output %t -output_no_suffix -options "-igc_opts 'FunctionControl=3'" | FileCheck %s --check-prefixes=CHECK-OCLOC
// RUN: oneapi-readelf %t.bin -Ws --sym-base=16 > %t.out
// RUN: oneapi-readelf %t.bin --debug-dump --sym-base=16 >> %t.out
// RUN: cat %t.out | FileCheck %s --check-prefixes=CHECK,%if llvm-22-plus || lib-igc-clang %{CHECK-LLVM22%} %else %{CHECK-PRE-LLVM22%}

// CHECK-OCLOC: Stack call has been detected

// Capture the start (Value) and size of each function symbol from .symtab.
// CHECK: Symbol table '.symtab'
// CHECK-LLVM22:      [[#%x,IMPL_START:]] 0x[[#%x,IMPL_SIZE:]]     FUNC    LOCAL  DEFAULT    1 __clang_ocl_kern_imp__kernel
// CHECK-LLVM22-NEXT: [[#%x,HELPER_START:]] 0x[[#%x,HELPER_SIZE:]] FUNC    LOCAL  DEFAULT    1 helper
// CHECK-PRE-LLVM22:  [[#%x,HELPER_START:]] 0x[[#%x,HELPER_SIZE:]] FUNC    LOCAL  DEFAULT    1 helper
// CHECK:             0000000000000000 0x[[#%x,KERNEL_SIZE:]]      FUNC    LOCAL  DEFAULT    1 _kernel

// Whole size (compile unit high_pc) == sum of all function sizes.
// CHECK: Abbrev Number: 1 (DW_TAG_compile_unit)
// CHECK-NEXT: DW_AT_producer
// CHECK-NEXT: DW_AT_language
// CHECK-NEXT: DW_AT_name
// CHECK-NEXT: DW_AT_low_pc       : 0
// CHECK-LLVM22-NEXT:     DW_AT_high_pc      : 0x[[#KERNEL_SIZE + IMPL_SIZE + HELPER_SIZE]]
// CHECK-PRE-LLVM22-NEXT: DW_AT_high_pc      : 0x[[#KERNEL_SIZE + HELPER_SIZE]]

// _kernel spans [0, KERNEL_SIZE).
// CHECK: Abbrev Number: {{[0-9]+}} (DW_TAG_subprogram)
// CHECK-NEXT: DW_AT_name        :  _kernel
// CHECK-NEXT: DW_AT_decl_file
// CHECK-NEXT: DW_AT_decl_line
// CHECK-NEXT: DW_AT_INTEL_simd_width
// CHECK-NEXT: DW_AT_external
// CHECK-NEXT: DW_AT_low_pc      :  0
// CHECK-NEXT: DW_AT_high_pc     :  0x[[#KERNEL_SIZE]]

// __clang_ocl_kern_imp__kernel spans [IMPL_START, IMPL_START + IMPL_SIZE).
// CHECK-LLVM22: Abbrev Number: {{[0-9]+}} (DW_TAG_subprogram)
// CHECK-LLVM22-NEXT: DW_AT_linkage_name: __clang_ocl_kern_imp__kernel
// CHECK-LLVM22-NEXT: DW_AT_name        :  _kernel
// CHECK-LLVM22-NEXT: DW_AT_decl_file
// CHECK-LLVM22-NEXT: DW_AT_decl_line
// CHECK-LLVM22-NEXT: DW_AT_INTEL_simd_width
// CHECK-LLVM22-NEXT: DW_AT_external
// CHECK-LLVM22-NEXT: DW_AT_low_pc      :  0x[[#IMPL_START]]
// CHECK-LLVM22-NEXT: DW_AT_high_pc     :  0x[[#IMPL_START + IMPL_SIZE]]

// helper spans [HELPER_START, HELPER_START + HELPER_SIZE).
// CHECK: Abbrev Number: {{[0-9]+}} (DW_TAG_subprogram)
// CHECK-NEXT: DW_AT_name        :  helper
// CHECK-NEXT: DW_AT_decl_file
// CHECK-NEXT: DW_AT_decl_line
// CHECK-NEXT: DW_AT_INTEL_simd_width
// CHECK-NEXT: DW_AT_type
// CHECK-NEXT: DW_AT_external
// CHECK-NEXT: DW_AT_low_pc      :  0x[[#HELPER_START]]
// CHECK-NEXT: DW_AT_high_pc     :  0x[[#HELPER_START + HELPER_SIZE]]

int helper(int x) {
    return x + 1;
}

kernel void _kernel(global int* out) {
    out[0] = helper(out[0]);
}
