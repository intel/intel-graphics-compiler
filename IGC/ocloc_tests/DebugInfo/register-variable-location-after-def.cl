//========================== begin_copyright_notice ============================
//
// Copyright (C) 2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
//=========================== end_copyright_notice =============================

// UNSUPPORTED: sys32
// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -device dg2 -options " -g -igc_opts 'VISAOptions=-asmToConsole -printInstOffsetInAsm, ElfDumpEnable=1, DumpUseShorterName=0, DebugDumpNamePrefix=%t_'" > %t.txt
// RUN: llvm-dwarfdump --debug-info %t_OCL_simd16_foo.elf >> %t.txt
// RUN: FileCheck %s --input-file %t.txt

// Second load writes vb.
// CHECK: load.ugm
// CHECK: load.ugm{{.*}} inst_offset=[[#LOAD:]]

// LLVM 16 describes vb in an inlined instance via DW_AT_abstract_origin.
// CHECK:      {{DW_AT_name|DW_AT_abstract_origin}} {{.*}}"vb")
// CHECK:      DW_AT_location
// CHECK-NEXT: [0x[[#%.16x,LOAD+16]],

__attribute__((intel_reqd_sub_group_size(16))) __kernel void foo(__global float *a, __global float *b,
                                                                 __global float *out) {
  int i = get_global_id(0);
  float va = a[i];
  float vb = b[i];
  out[i] = va * vb;
}
