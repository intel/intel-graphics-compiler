/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// This test checks that when StackOverflowDetection is enabled, the compiled
// binary contains a DWARF DW_TAG_subprogram entry for stack overflow detection subroutine
// This allows gdb-oneapi to show a message that software exception
// was triggered due to stack overflow detection

// UNSUPPORTED: sys32
// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -options " -g -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_, StackOverflowDetection=1'" -device dg2
// RUN: llvm-dwarfdump --debug-info %t_OCL_simd8_test_stackoverflow_dwarf.elf | FileCheck %s

// Verify that the stack overflow detection subroutine has a DW_TAG_subprogram
// entry with name "__stackoverflow_detection" and proper attributes in DWARF.
// CHECK: DW_TAG_subprogram
// CHECK:   DW_AT_linkage_name      ("__stackoverflow_detection{{.*}}")
// CHECK:   DW_AT_name      ("__stackoverflow_detection")
// CHECK:   DW_AT_artificial        (true)
// CHECK:   DW_AT_low_pc    (0x{{[0-9a-f]+}})
// CHECK:   DW_AT_high_pc   (0x{{[0-9a-f]+}})

int fact(int n) {
  return n < 2 ? 1 : n * fact(n - 1);
}

kernel void test_stackoverflow_dwarf(global int *out, int n) {
  out[0] = fact(n);
}
