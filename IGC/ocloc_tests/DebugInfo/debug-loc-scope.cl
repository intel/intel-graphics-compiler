//========================== begin_copyright_notice ============================
//
// Copyright (C) 2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
//=========================== end_copyright_notice =============================

// Test FP-based variable locations in stack-call functions and kernel.
//
// Verifies that:
// 1. Outermost-scope FP-based variables ("arg", "*outer") are emitted to
//    .debug_loc
// 2. Non-outermost-scope FP-based variable ("*inner") is inlined directly
//    into the DIE, since FE_FP is valid throughout the scope.

// UNSUPPORTED: sys32
// REQUIRES: regkeys, oneapi-readelf, dg2-supported

// RUN: ocloc compile -file %s -options " -g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'" -device dg2
// RUN: oneapi-readelf --debug-dump %t_OCL_simd8_test_kernel.elf | FileCheck %s

int stackcall_func(int arg) {
    int func_outer = arg + 10;

    if (arg > 0) {
        int func_inner = arg * 2;
        func_outer = func_inner;
    }

    return func_outer;
}

__kernel void test_kernel(global int *out) {
    int kernel_outer = out[0] + 5;

    if (kernel_outer > 10) {
        int kernel_inner = kernel_outer * 3;
        out[1] = kernel_inner;
    }

    out[0] = stackcall_func(kernel_outer);
}

// CHECK: DW_TAG_formal_parameter
// CHECK-NEXT: DW_AT_name        : out
// CHECK-NEXT: DW_AT_decl_file   : {{.*}}
// CHECK-NEXT: DW_AT_decl_line   : {{.*}}
// CHECK-NEXT: DW_AT_type        : {{.*}}
// CHECK-NEXT: DW_AT_location    : {{.+}} (location list)

// CHECK: DW_TAG_variable
// CHECK-NEXT: DW_AT_name        : kernel_outer
// CHECK-NEXT: DW_AT_decl_file   : {{.*}}
// CHECK-NEXT: DW_AT_decl_line   : {{.*}}
// CHECK-NEXT: DW_AT_type        : {{.*}}
// CHECK-NEXT: DW_AT_location    : {{.+}} (location list)

// CHECK: DW_TAG_variable
// CHECK-NEXT: DW_AT_name        : kernel_inner
// CHECK-NEXT: DW_AT_decl_file   : {{.*}}
// CHECK-NEXT: DW_AT_decl_line   : {{.*}}
// CHECK-NEXT: DW_AT_type        : {{.*}}
// CHECK-NEXT: DW_AT_location    : {{[0-9]+}} byte block:{{.+}}DW_OP_plus_uconst

// CHECK: DW_TAG_formal_parameter
// CHECK-NEXT: DW_AT_name        : arg
// CHECK-NEXT: DW_AT_decl_file   : {{.*}}
// CHECK-NEXT: DW_AT_decl_line   : {{.*}}
// CHECK-NEXT: DW_AT_type        : {{.*}}
// CHECK-NEXT: DW_AT_location    : {{.+}} (location list)

// CHECK: DW_TAG_variable
// CHECK-NEXT: DW_AT_name        : func_outer
// CHECK-NEXT: DW_AT_decl_file   : {{.*}}
// CHECK-NEXT: DW_AT_decl_line   : {{.*}}
// CHECK-NEXT: DW_AT_type        : {{.*}}
// CHECK-NEXT: DW_AT_location    : {{.+}} (location list)

// CHECK: DW_TAG_variable
// CHECK-NEXT: DW_AT_name        : func_inner
// CHECK-NEXT: DW_AT_decl_file   : {{.*}}
// CHECK-NEXT: DW_AT_decl_line   : {{.*}}
// CHECK-NEXT: DW_AT_type        : {{.*}}
// CHECK-NEXT: DW_AT_location    : {{[0-9]+}} byte block:{{.+}}DW_OP_plus_uconst

// CHECK: .debug_loc
// CHECK: {{0+}} {{[0-9a-f]+}} {{[0-9a-f]+}} ({{.*}}DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: {{[0-9]+}}; DW_OP_INTEL_push_simd_lane{{.*}}DW_OP_plus_uconst: {{[0-9]+}})
// CHECK: {{0+}} {{[0-9a-f]+}} {{[0-9a-f]+}} ({{.*}}DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: {{[0-9]+}}; DW_OP_INTEL_push_simd_lane{{.*}}DW_OP_plus_uconst: {{[0-9]+}})
// CHECK: {{0+}} {{[0-9a-f]+}} {{[0-9a-f]+}} ({{.*}}DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: {{[0-9]+}}; DW_OP_INTEL_push_simd_lane{{.*}}DW_OP_plus_uconst: {{[0-9]+}})
// CHECK: {{0+}} {{[0-9a-f]+}} {{[0-9a-f]+}} ({{.*}}DW_OP_INTEL_regval_bits: 64; DW_OP_plus_uconst: {{[0-9]+}}; DW_OP_INTEL_push_simd_lane{{.*}}DW_OP_plus_uconst: {{[0-9]+}})
