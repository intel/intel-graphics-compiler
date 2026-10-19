//========================== begin_copyright_notice ============================
//
// Copyright (C) 2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
//=========================== end_copyright_notice =============================

// A variable declared right before an inlined call keeps its location after
// the call returns.

// UNSUPPORTED: sys32
// REQUIRES: dg2-supported

// RUN: ocloc compile -file %s -options " -g -cl-opt-disable -igc_opts 'ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'" -device dg2
// RUN: llvm-dwarfdump --debug-info %t_OCL_simd8_foo.elf | FileCheck %s

// bar has no parameters or local variables, so the declare of e is followed by
// the call to get_sub_group_size.
__attribute__((always_inline)) int bar(void) { return get_sub_group_size(); }

// "e" is declared right before the inlined bar. Check if its location ends where the
// location of "c" ends, not where bar() ends.
// CHECK:      DW_AT_name ("c")
// CHECK:      DW_AT_location
// CHECK-NEXT: [0x{{[0-9a-f]+}}, [[END:0x[0-9a-f]+]])
// CHECK:      DW_AT_name ("e")
// CHECK:      DW_AT_location
// CHECK-NEXT: [0x{{[0-9a-f]+}}, [[END]])
kernel void foo(global int *out) {
  int c = 0;
  int e = bar();
  out[0] = c + e;
}
