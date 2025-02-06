/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// UNSUPPORTED: sys32
// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1'" | FileCheck %s

__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_sub_group_non_uniform_broadcast_non_immediate_sub_group_local_id_simd16(global int* in, global int* ids, global int* out) {
    size_t gid = get_global_id(0);
    int x = in[gid];
    uint which_sub_group_local_id = ids[gid];
// CHECK-LABEL: .kernel "test_sub_group_non_uniform_broadcast_non_immediate_sub_group_local_id_simd16"
// CHECK: shl (M1, 16) ShuffleTmp(0,0)<1> {{.+}}(0,0)<16;8,2> 0x2:uw
// CHECK: addr_add (M1, 16) A0(0)<1> &{{V[0-9]+}} ShuffleTmp(0,0)<1;1,0>
// CHECK: mov (M1, 16) simdShuffle(0,0)<1> r[A0(0),0]<1,0>:d
// CHECK: lsc_store.ugm (M1, 16)  flat[{{.+}}]:a64  simdShuffle:d32
    bool isOddLane = get_sub_group_local_id() % 2 == 1;
    if (isOddLane)
    {
        out[gid] = sub_group_non_uniform_broadcast(x, which_sub_group_local_id);
    }
}

__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_sub_group_non_uniform_broadcast_non_immediate_sub_group_local_id_simd32(global int* in, global int* ids, global int* out) {
    size_t gid = get_global_id(0);
    int x = in[gid];
    uint which_sub_group_local_id = ids[gid];
// CHECK-LABEL: .kernel "test_sub_group_non_uniform_broadcast_non_immediate_sub_group_local_id_simd32"
// CHECK: shl (M1, 32) ShuffleTmp(0,0)<1> {{.+}}(0,0)<16;8,2> 0x2:uw
// CHECK: addr_add (M1, 16) A0(0)<1> &{{V[0-9]+}} ShuffleTmp(0,0)<1;1,0>
// CHECK: mov (M1, 16) simdShuffle(0,0)<1> r[A0(0),0]<1,0>:d
// CHECK: addr_add (M5, 16) A0(0)<1> &{{V[0-9]+}} ShuffleTmp(0,16)<1;1,0>
// CHECK: mov (M5, 16) simdShuffle(1,0)<1> r[A0(0),0]<1,0>:d
// CHECK: lsc_store.ugm (M1, 32)  flat[V0046]:a64  simdShuffle:d32
    bool isOddLane = get_sub_group_local_id() % 2 == 1;
    if (isOddLane)
    {
        out[gid] = sub_group_non_uniform_broadcast(x, which_sub_group_local_id);
    }
}

__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_sub_group_non_uniform_broadcast_immediate_sub_group_local_id_simd16(global int* in, global int* ids, global int* out) {
    size_t gid = get_global_id(0);
    int x = in[gid];
    uint which_sub_group_local_id = 15;
// CHECK-LABEL: .kernel "test_sub_group_non_uniform_broadcast_immediate_sub_group_local_id_simd16"
// CHECK: mov (M1_NM, 1) simdShuffle(0,0)<1> {{V[0-9]+}}(0,15)<0;1,0>
// CHECK: mov (M1, 16) simdShuffleBroadcast(0,0)<1> simdShuffle(0,0)<0;1,0>
// CHECK: lsc_store.ugm (M1, 16)  flat[{{.+}}]:a64  simdShuffleBroadcast:d32
    bool isOddLane = get_sub_group_local_id() % 2 == 1;
    if (isOddLane)
    {
        out[gid] = sub_group_non_uniform_broadcast(x, which_sub_group_local_id);
    }
}

__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_sub_group_non_uniform_broadcast_immediate_sub_group_local_id_simd32(global int* in, global int* ids, global int* out) {
    size_t gid = get_global_id(0);
    int x = in[gid];
    uint which_sub_group_local_id = 31;
// CHECK-LABEL: .kernel "test_sub_group_non_uniform_broadcast_immediate_sub_group_local_id_simd32"
// CHECK: mov (M5_NM, 1) simdShuffle(0,0)<1> {{V[0-9]+}}(1,15)<0;1,0>
// CHECK: mov (M1, 32) simdShuffleBroadcast(0,0)<1> simdShuffle(0,0)<0;1,0>
// CHECK: lsc_store.ugm (M1, 32)  flat[{{.+}}]:a64  simdShuffleBroadcast:d32
    bool isOddLane = get_sub_group_local_id() % 2 == 1;
    if (isOddLane)
    {
        out[gid] = sub_group_non_uniform_broadcast(x, which_sub_group_local_id);
    }
}
