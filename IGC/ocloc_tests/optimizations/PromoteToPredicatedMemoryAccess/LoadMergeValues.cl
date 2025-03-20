/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Tests verify different cases of merge value (splat vector, constant vector, live, dead, etc...)

// REQUIRES: regkeys,pvc-supported,llvm-14-plus

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'EnablePromoteToPredicatedMemoryAccess=1 VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefixes=CHECK-ASM --implicit-check-not jmpi

// CHECK-ASM: kernel test_splat_vector
// CHECK-ASM-DAG:         cmp (32|M0)   (le)[[F1:f[0-9\.]+]]   null<1>:d     r{{[\.;,0-9<>:a-z]+}}
// CHECK-ASM-DAG: (W)     mov (8|M0)               [[R1:r[0-9]+]].0<1>:d     5:w
// CHECK-ASM:         mov (32|M0)              [[L1:r[0-9]+]].0<1>:f    [[R1]].0<0;1,0>:f
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R1]].1<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R1]].2<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R1]].3<0;1,0>:d
// CHECK-ASM: ([[F1]])  load.ugm.d32x4.a64 (32|M0)  [[L1]]:8
// CHECK-ASM:         mov (32|M0)              [[L2:r[0-9]+]].0<1>:d    [[R1]].4<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R1]].5<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R1]].6<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R1]].7<0;1,0>:d
// CHECK-ASM: ([[F1]])  load.ugm.d32x4.a64 (32|M0)  [[L2]]:8
// CHECK-ASM:         mov (32|M0)              [[L3:r[0-9]+]].0<1>:f    [[R1]].0<0;1,0>:f
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R1]].1<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R1]].2<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R1]].3<0;1,0>:d
// CHECK-ASM: ([[F1]])  load.ugm.d32x4.a64 (32|M0)  [[L3]]:8
// CHECK-ASM:         mov (32|M0)              [[L4:r[0-9]+]].0<1>:d    [[R1]].4<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R1]].5<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R1]].6<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R1]].7<0;1,0>:d
// CHECK-ASM: ([[F1]])  load.ugm.d32x4.a64 (32|M0)  [[L4]]:8

__kernel void test_splat_vector(__global const int16* in, __global int16* out, const int predicate) {
    int gid = get_global_id(0);
    int16 val = (int16){5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5};
    if (gid <= predicate)
        val = in[gid];
    out[gid] = val;
}

// CHECK-ASM: kernel test_const_vector
// CHECK-ASM-DAG:         cmp (32|M0)   (le)[[F2:f[0-9\.]+]]   null<1>:d     r{{[\.;,0-9<>:a-z]+}}
// CHECK-ASM-DAG:         mov (32|M0)              [[L5:r[0-9]+]].0<1>:d    1:w
// CHECK-ASM-DAG:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    2:w
// CHECK-ASM-DAG:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    3:w
// CHECK-ASM-DAG:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    4:w
// CHECK-ASM: ([[F2]])  load.ugm.d32x4.a64 (32|M0)  [[L5]]:8
// CHECK-ASM-DAG:         mov (32|M0)              [[L6:r[0-9]+]].0<1>:d    5:w
// CHECK-ASM-DAG:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    6:w
// CHECK-ASM-DAG:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    7:w
// CHECK-ASM-DAG:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    8:w
// CHECK-ASM: ([[F2]])  load.ugm.d32x4.a64 (32|M0)  [[L6]]:8
// CHECK-ASM-DAG:         mov (32|M0)              [[L7:r[0-9]+]].0<1>:d    9:w
// CHECK-ASM-DAG:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    10:w
// CHECK-ASM-DAG:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    11:w
// CHECK-ASM-DAG:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    12:w
// CHECK-ASM: ([[F2]])  load.ugm.d32x4.a64 (32|M0)  [[L7]]:8
// CHECK-ASM-DAG:         mov (32|M0)              [[L8:r[0-9]+]].0<1>:d    13:w
// CHECK-ASM-DAG:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    14:w
// CHECK-ASM-DAG:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    15:w
// CHECK-ASM-DAG:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    16:w
// CHECK-ASM: ([[F2]])  load.ugm.d32x4.a64 (32|M0)  [[L8]]:8

__kernel void test_const_vector(__global const int16* in, __global int16* out, const int predicate) {
    int gid = get_global_id(0);
    int16 val = (int16){1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    if (gid <= predicate)
        val = in[gid];
    out[gid] = val;
}

// CHECK-ASM: kernel test_dead_merge_value
// CHECK-ASM-DAG:        cmp (32|M0)   (le)[[F3:f[0-9\.]+]]   null<1>:d
// CHECK-ASM-DAG:        mul (32|M0)              [[L9:r[0-9]+]].0<1>:d     r{{[\.;,0-9<>:a-z]+}}     10:w
// CHECK-ASM:([[F3]])  load.ugm.d32.a64 (32|M0)  [[L9]]:2
// CHECK-ASM:        store.ugm.d32.a64 (32|M0)  [r{{[\.;,0-9<>:a-z]+}}]      [[L9]]:2

// test verifies predicated load with non-constant dead merge value
__kernel void test_dead_merge_value(__global const int* in, __global int* out, const int predicate) {
    int gid = get_global_id(0);
    int val = gid * 10;
    if (gid <= predicate)
        val = in[gid];
    out[gid] = val;
}
