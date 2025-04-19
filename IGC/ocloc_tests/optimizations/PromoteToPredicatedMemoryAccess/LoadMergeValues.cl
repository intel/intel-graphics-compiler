/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Tests verify different cases of merge value (splat vector, constant vector, live, dead, etc...)

// REQUIRES: regkeys,pvc-supported,llvm-14-plus

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'EnablePromoteToPredicatedMemoryAccess=1 VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefixes=CHECK-ASM --implicit-check-not jmpi

// CHECK-ASM: kernel test_split_zero_vector
// CHECK-ASM-DAG: (W)     mov (8|M0)               [[R2:r[0-9]+]].0<1>:d     0:w
// CHECK-ASM-DAG:         cmp (32|M0)   (le)[[F5:f[0-9\.]+]]   null<1>:d
// CHECK-ASM:         mov (32|M0)              [[L12:r[0-9]+]].0<1>:f    [[R2]].0<0;1,0>:f
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R2]].1<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R2]].2<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R2]].3<0;1,0>:d
// CHECK-ASM: ([[F5]])  load.ugm.d32x4.a64 (32|M0)  [[L12]]:8
// CHECK-ASM:         mov (32|M0)              [[L13:r[0-9]+]].0<1>:d    [[R2]].4<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R2]].5<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R2]].6<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R2]].7<0;1,0>:d
// CHECK-ASM: ([[F5]])  load.ugm.d32x4.a64 (32|M0)  [[L13]]:8
// CHECK-ASM:         mov (32|M0)              [[L14:r[0-9]+]].0<1>:f    [[R2]].0<0;1,0>:f
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R2]].1<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R2]].2<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R2]].3<0;1,0>:d
// CHECK-ASM: ([[F5]])  load.ugm.d32x4.a64 (32|M0)  [[L14]]:8
// CHECK-ASM:         mov (32|M0)              [[L15:r[0-9]+]].0<1>:d    [[R2]].4<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R2]].5<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R2]].6<0;1,0>:d
// CHECK-ASM:         mov (32|M0)              r{{[0-9]+}}.0<1>:d    [[R2]].7<0;1,0>:d
// CHECK-ASM: ([[F5]])  load.ugm.d32x4.a64 (32|M0)  [[L15]]:8

// test verifies zero vector merge value split
__kernel void test_split_zero_vector(__global const int16* in, __global int16* out, const int predicate) {
    int gid = get_global_id(0);
    int16 val = 0;
    if (gid <= predicate)
        val = in[gid];
    out[gid] = val;
}

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

// test verifies predicated load with splat vector merge value
// it makes a copy of the merge value (vector) before the load
// also verifies split of large vectors into smaller vectors
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

// CHECK-ASM: kernel test_non_const_vector
// CHECK-ASM-DAG:         cmp (32|M0)   (le)[[F6:f[0-9\.]+]]   null<1>:d
// CHECK-ASM-DAG:         add3 (32|M0)             [[L16:r[0-9]+]].0<1>:d    [[R3:r[0-9]+]].0<1;0>:d       [[R4:r[0-9]+]].0<0;0>:d       1:w
// CHECK-ASM-DAG:         add3 (32|M0)             r{{[0-9]+}}.0<1>:d    [[R3]].0<1;0>:d       [[R4]].0<0;0>:d       2:w
// CHECK-ASM-DAG:         add3 (32|M0)             r{{[0-9]+}}.0<1>:d    [[R3]].0<1;0>:d       [[R4]].0<0;0>:d       3:w
// CHECK-ASM-DAG:         add3 (32|M0)             r{{[0-9]+}}.0<1>:d    [[R3]].0<1;0>:d       [[R4]].0<0;0>:d       4:w
// CHECK-ASM: ([[F6]])  load.ugm.d32x4.a64 (32|M0)  [[L16]]:8
// CHECK-ASM-DAG:         add3 (32|M0)             [[L17:r[0-9]+]].0<1>:d    [[R3]].0<1;0>:d       [[R4]].0<0;0>:d       5:w
// CHECK-ASM-DAG:         add3 (32|M0)             r{{[0-9]+}}.0<1>:d    [[R3]].0<1;0>:d       [[R4]].0<0;0>:d       6:w
// CHECK-ASM-DAG:         add3 (32|M0)             r{{[0-9]+}}.0<1>:d    [[R3]].0<1;0>:d       [[R4]].0<0;0>:d       7:w
// CHECK-ASM-DAG:         add3 (32|M0)             r{{[0-9]+}}.0<1>:d    [[R3]].0<1;0>:d       [[R4]].0<0;0>:d       8:w
// CHECK-ASM: ([[F6]])  load.ugm.d32x4.a64 (32|M0)  [[L17]]:8
// CHECK-ASM-DAG:         add3 (32|M0)             [[L18:r[0-9]+]].0<1>:d    [[R3]].0<1;0>:d       [[R4]].0<0;0>:d       9:w
// CHECK-ASM-DAG:         add3 (32|M0)             r{{[0-9]+}}.0<1>:d    [[R3]].0<1;0>:d       [[R4]].0<0;0>:d       10:w
// CHECK-ASM-DAG:         add3 (32|M0)             r{{[0-9]+}}.0<1>:d    [[R3]].0<1;0>:d       [[R4]].0<0;0>:d       11:w
// CHECK-ASM-DAG:         add3 (32|M0)             r{{[0-9]+}}.0<1>:d    [[R3]].0<1;0>:d       [[R4]].0<0;0>:d       12:w
// CHECK-ASM: ([[F6]])  load.ugm.d32x4.a64 (32|M0)  [[L18]]:8
// CHECK-ASM-DAG:         add3 (32|M0)             [[L19:r[0-9]+]].0<1>:d    [[R3]].0<1;0>:d       [[R4]].0<0;0>:d       13:w
// CHECK-ASM-DAG:         add3 (32|M0)             r{{[0-9]+}}.0<1>:d    [[R3]].0<1;0>:d       [[R4]].0<0;0>:d       14:w
// CHECK-ASM-DAG:         add3 (32|M0)             r{{[0-9]+}}.0<1>:d    [[R3]].0<1;0>:d       [[R4]].0<0;0>:d       15:w
// CHECK-ASM-DAG:         add3 (32|M0)             r{{[0-9]+}}.0<1>:d    [[R3]].0<1;0>:d       [[R4]].0<0;0>:d       16:w
// CHECK-ASM: ([[F6]])  load.ugm.d32x4.a64 (32|M0)  [[L19]]:8

// test verifies non-constant vector merge value split
__kernel void test_non_const_vector(__global const int16* in, __global int16* out, const int predicate) {
    int gid = get_global_id(0);
    int16 val = (int16){gid + 1, gid + 2, gid + 3, gid + 4, gid + 5, gid + 6, gid + 7, gid + 8, gid + 9, gid + 10, gid + 11, gid + 12, gid + 13, gid + 14, gid + 15, gid + 16};
    if (gid <= predicate)
        val = in[gid];
    out[gid] = val;
}

// CHECK-ASM: kernel test_dead_merge_value_DW
// CHECK-ASM-DAG:        cmp (32|M0)   (le)[[F3:f[0-9\.]+]]   null<1>:d
// CHECK-ASM-DAG:        mul (32|M0)              [[L9:r[0-9]+]].0<1>:d     r{{[\.;,0-9<>:a-z]+}}     10:w
// CHECK-ASM:([[F3]])  load.ugm.d32.a64 (32|M0)  [[L9]]:2
// CHECK-ASM:        store.ugm.d32.a64 (32|M0)  [r{{[\.;,0-9<>:a-z]+}}]      [[L9]]:2

// test verifies predicated load with non-constant dead merge value
__kernel void test_dead_merge_value_DW(__global const int* in, __global int* out, const int predicate) {
    int gid = get_global_id(0);
    int val = gid * 10;
    if (gid <= predicate)
        val = in[gid];
    out[gid] = val;
}

// CHECK-ASM: kernel test_live_merge_value
// CHECK-ASM-DAG:        cmp (32|M0)   (le)[[F4:f[0-9\.]+]]   null<1>:d
// CHECK-ASM-DAG:        mul (32|M0)              [[L10:r[0-9]+]].0<1>:d     r{{[\.;,0-9<>:a-z]+}}     10:w
// CHECK-ASM:            mov (32|M0)              [[L11:r[0-9]+]].0<1>:f     [[L10]].0<1;1,0>:f
// CHECK-ASM:([[F4]])  load.ugm.d32.a64 (32|M0)  [[L11]]:2
// CHECK-ASM-DAG:      store.ugm.d32.a64 (32|M0)  [r{{[\.;,0-9<>:a-z]+}}]      [[L11]]:2
// CHECK-ASM-DAG:      store.ugm.d32.a64 (32|M0)  [r{{[\.;,0-9<>:a-z]+}}]      [[L10]]:2

// test verifies predicated load with non-constant live merge value
// it makes a copy of the merge value (with number of elements = 1) before the load
__kernel void test_live_merge_value(__global const int* in, __global int* out0, __global int* out1, const int predicate) {
    int gid = get_global_id(0);
    int val0 = gid * 10;
    int val1 = val0;
    if (gid <= predicate)
        val0 = in[gid];
    out0[gid] = val0;
    out1[gid] = val1;
}

// tests below verify different cases of transforming merge value for predicated loads
// like load <8xi8> -> load <2 x i32>

// CHECK-ASM: kernel test_vec_process_load_0
// CHECK-ASM-DAG: (W)     mov (2|M0)               [[R5:r[0-9]+]].0<1>:d     0:w
// CHECK-ASM-DAG:         cmp (32|M0)   (le)[[F7:f[0-9\.]+]]   null<1>:d
// CHECK-ASM:         mov (32|M0)              [[L20:r[0-9]+]].0<1>:f    [[R5]].0<0;1,0>:f
// CHECK-ASM:         mov (32|M0)              r{{[\.;,0-9<>:a-z]+}}    [[R5]].1<0;1,0>:d
// CHECK-ASM: ([[F7]])  load.ugm.d32x2.a64 (32|M0)  [[L20]]:4

__kernel void test_vec_process_load_0(__global const char8* in, __global char8* out, const int predicate) {
    int gid = get_global_id(0);
    char8 val = (char8){0, 0, 0, 0, 0, 0, 0, 0};
    if (gid <= predicate)
        val = in[gid];
    out[gid] = val;
}
