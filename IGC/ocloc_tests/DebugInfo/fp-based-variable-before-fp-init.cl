//========================== begin_copyright_notice ============================
//
// Copyright (C) 2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
//=========================== end_copyright_notice =============================

// Test that an FP-based variable in a non-outermost scope is not inlined to the
// DIE when instructions of its scope are scheduled before FP is set.
//
// With stack calls and optimizations, the scheduler places instructions of the
// inlined "body" before FP = SP in the kernel or stack-called function prologue.
// The location of "vals" must be emitted to .debug_loc and must not cover the
// start of the scope, where FP is not set yet.

// UNSUPPORTED: sys32
// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -options " -g -igc_opts 'FunctionControl=3, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'" -device dg2
// RUN: llvm-dwarfdump --debug-info %t_OCL_simd8_fp_kernel.elf | FileCheck %s
// RUN: llvm-dwarfdump --debug-info %t_OCL_simd8_stack_fp_kernel.elf | FileCheck %s --check-prefixes=STACK,CHECK

// STACK:      DW_AT_name ("stack_body")

// CHECK:      DW_AT_abstract_origin {{.*}} "body")
// CHECK:      DW_AT_ranges
// CHECK-NEXT: [[[SCOPE_START:0x[0-9a-f]+]],

// CHECK:      DW_TAG_variable
// CHECK-NEXT: DW_AT_abstract_origin {{.*}} "vals")
// CHECK-NEXT: DW_AT_location {{.*}}(0x{{[0-9a-f]+}}:
// CHECK-NOT:  [[[SCOPE_START]],
// CHECK:      [0x{{[0-9a-f]+}}, 0x{{[0-9a-f]+}}): DW_OP_const1u 0x8f, DW_OP_const1u 0x80
// CHECK-NOT:  [[[SCOPE_START]],
// CHECK:      NULL

int check(int *vals, int n) {
  for (int i = 0; i < n; i++) {
    if (vals[i] != i)
      return 0;
  }
  return 1;
}

__attribute__((always_inline)) inline void body(global int *out) {
  int vals[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  out[1] = 7;
  out[0] = check(vals, out[2]);
}

__kernel void fp_kernel(global int *out) { body(out); }

__attribute__((noinline)) void stack_body(global int *out) { body(out); }

__kernel void stack_fp_kernel(global int *out) { stack_body(out); }
