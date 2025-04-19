/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Tests verify predicated load for subDW types

// REQUIRES: regkeys,pvc-supported,llvm-14-plus

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'EnablePromoteToPredicatedMemoryAccess=1 VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefixes=CHECK-ASM

// CHECK-ASM: kernel test_i8_0
// CHECK-ASM:             cmp (32|M0)   (lt)[[F1:f[0-9\.]+]]
// CHECK-ASM:   ([[F1]]) goto (32|M0)
// CHECK-ASM: {{[_a-z0-9A-Z]+}}:
// CHECK-ASM-DAG:         cmp (32|M0)   (le)[[F2:f[0-9\.]+]]   null<1>:d     r{{[0-9\.]+}}
// CHECK-ASM-DAG:         cmp (32|M0)   (ge)[[F3:f[0-9\.]+]]   null<1>:d     r{{[0-9\.]+}}
// CHECK-ASM-DAG:         mov (32|M0)              [[L1:r[0-9]+]].0<1>:d    0:w
// CHECK-ASM-DAG:         mov (32|M0)              [[L2:r[0-9]+]].0<4>:b    0:w
// CHECK-ASM:   ([[F2]])  load.ugm.d32.a64 (32|M0)    [[L1]]:2       [{{[a-z0-9:]+}}]
// CHECK-ASM:   ([[F3]])  load.ugm.d8u32.a64 (32|M0)  [[L2]]:2       [{{[a-z0-9:]+}}]
// CHECK-ASM:             store.ugm.d32x2.a64 (32|M0)  [{{[a-z0-9:]+}}]    {{[a-z0-9:]+}}

// Kernel with constant 0 merge value
__attribute__((intel_reqd_sub_group_size(32)))
__kernel void test_i8_0(__global const char4* in0, __global const char* in1, __global int2* out, const int predicate) {
  int gid = get_global_id(0);

  int readIdx1 = gid % 3; // 0, 1, 2
  int readIdx2 = gid % 4; // 0, 1, 2, 3

  int fLoad1 = gid <= predicate;
  int fLoad2 = gid >= predicate;
  int fStore = (gid <= predicate + 5) && (gid >= predicate - 5);

  char4 val1 = 0;
  if(fLoad1)
    val1 = in0[readIdx1];

  char i1 = val1.x;
  char i2 = val1.y;
  char i3 = val1.z;
  char i4 = val1.w;

  char val2 = 0;
  if(fLoad2)
    val2 = in1[readIdx2];

  i1 += val2;
  i2 += val2;
  i3 += val2;
  i4 += val2;

  if(fStore) {
    short2 outVal = (short2)(i1, i2);
    int outVali = as_int(outVal);
    short2 outVal2 = (short2)(i3, i4);
    int outVali2 = as_int(outVal2);
    out[gid] = (int2)(outVali, outVali2);
  }
}

// CHECK-ASM: kernel test_i8_splat_vector
// CHECK-ASM-DAG: (W)     mov (2|M0)               r{{[0-9]+}}.0<2>:b     5:w
// CHECK-ASM-DAG:         cmp (32|M0)   (lt)[[F4:f[0-9\.]+]]   null<1>:d     r{{[0-9:\.]+}}<1;1,0>:d
// CHECK-ASM: ([[F4]])  load.ugm.d16u32.a64 (32|M0)  r{{[0-9]+}}:2      [r{{[0-9:]+}}]

// Kernel with constant splat merge vector
__kernel void test_i8_splat_vector(__global const char2* in, __global char2* out, const int predicate) {
    int gid = get_global_id(0);
    char2 val = (char2){5, 5};
    if (gid < predicate)
        val = in[gid];
    out[gid] = val;
}

// CHECK-ASM: kernel test_i8_const_vector
// CHECK-ASM-DAG:         cmp (32|M0)   (lt)[[F5:f[0-9\.]+]]   null<1>:d     r{{[0-9:\.]+}}<1;1,0>:d
// CHECK-ASM-DAG: (W)     mov (1|M0)               r{{[0-9]+}}.0<1>:hf    0x302:hf
// CHECK-ASM: ([[F5]])  load.ugm.d16u32.a64 (32|M0)  r{{[0-9]+}}:2      [r{{[0-9:]+}}]

// Kernel with constant non-0 merge vector
__kernel void test_i8_const_vector(__global const char2* in, __global char2* out, const int predicate) {
    int gid = get_global_id(0);
    char2 val = (char2){2, 3};
    if (gid < predicate)
        val = in[gid];
    out[gid] = val;
}

// CHECK-ASM: kernel test_dead_merge_value
// CHECK-ASM:        cmp (32|M0)   (le)[[F6:f[0-9\.]+]]   null<1>:d
// CHECK-ASM:([[F6]])  load.ugm.d8u32.a64 (32|M0)  [[L3:r[0-9]+]]:2
// CHECK-ASM:        add (32|M0)              [[D1:r[0-9]+]].0<1>:w
// CHECK-ASM:        mov (32|M0)              [[D2:r[0-9]+]].0<1>:b    [[D1]].0<2;1,0>:b
// CHECK-ASM:([[F6]])  mov (32|M0)              [[D2]].0<1>:b    [[L3]].0<4;1,0>:b
// CHECK-ASM:        mov (32|M0)              [[L4:r[0-9]+]].0<1>:ud   [[D2]].0<1;1,0>:ub
// CHECK-ASM:        store.ugm.d8u32.a64 (32|M0)  [r{{[0-9:]+}}]    [[L4]]:2

// test verifies predicated load with non-constant dead merge value
__kernel void test_dead_merge_value(__global const char* in, __global char* out, const int predicate) {
    int gid = get_global_id(0);
    char val = gid - 10;
    if (gid <= predicate)
        val = in[gid];
    out[gid] = val;
}
