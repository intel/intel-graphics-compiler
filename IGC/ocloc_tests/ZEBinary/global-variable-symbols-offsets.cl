/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// UNSUPPORTED: sys32
// REQUIRES: pvc-supported, oneapi-readelf
// RUN: ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'ProgbinDumpFileName=%t.progbin'" -device pvc
// RUN: oneapi-readelf -s %t.progbin | FileCheck %s

// This test verifies whether the offsets of global variables that require more than
// 32 bits to represent are correctly passed to the ELF symbol table.

// CHECK: 0000000000000000 0x80000000 OBJECT  GLOBAL DEFAULT    3 array0
// CHECK: 0000000080000000 0x80000000 OBJECT  GLOBAL DEFAULT    3 array1
// CHECK: 0000000100000000 0x80000000 OBJECT  GLOBAL DEFAULT    3 array2
// CHECK: 0000000180000000 0x80000000 OBJECT  GLOBAL DEFAULT    3 array3
// CHECK: 0000000200000000 0x80000000 OBJECT  GLOBAL DEFAULT    3 array4

global long array0[268435456];     // <-- offset = 0
global long array1[268435456];     // <-- offset = 0          + sizeof(long) * 268435456 = 2147483648  (0x080000000)
global long array2[268435456];     // <-- offset = 2147483648 + sizeof(long) * 268435456 = 4294967296  (0x100000000)
global long array3[268435456];     // <-- offset = 4294967296 + sizeof(long) * 268435456 = 6442450944  (0x180000000)
global long array4[268435456];     // <-- offset = 6442450944 + sizeof(long) * 268435456 = 8589934592  (0x200000000)

kernel void test_simple(global long* out) {
  array0[268435455] = 111;
  array1[268435455] = 222;
  array2[268435455] = 333;
  array3[268435455] = 444;
  array4[268435455] = 555;
  out[0] = array0[268435455];
  out[1] = array1[268435455];
  out[2] = array2[268435455];
  out[3] = array3[268435455];
  out[4] = array4[268435455];
}
