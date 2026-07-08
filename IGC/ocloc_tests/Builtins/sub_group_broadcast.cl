/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// UNSUPPORTED: sys32
// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1'" | FileCheck %s

__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_sub_group_broadcast_non_immediate_sub_group_local_id_simd16(global int* in, global int* ids, global int* out) {
    size_t gid = get_global_id(0);
    int x = in[gid];
    uint which_sub_group_local_id = ids[gid];
// CHECK-LABEL: .kernel "test_sub_group_broadcast_non_immediate_sub_group_local_id_simd16"
// CHECK: shl (M1_NM, 1) ShuffleTmp(0,0)<1> {{.+}}(0,0)<0;1,0> 0x2:uw
// CHECK: addr_add (M1_NM, 1) A0(0)<1> &{{V[0-9]+}} ShuffleTmp(0,0)<0;1,0>
// CHECK: mov (M1, 16) simdBroadcast(0,0)<1> r[A0(0),0]<0;1,0>:d
// CHECK: lsc_store.ugm (M1, 16)  flat[{{.+}}]:a64  simdBroadcast:d32
    out[gid] = sub_group_broadcast(x, which_sub_group_local_id);
}

__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_sub_group_broadcast_non_immediate_sub_group_local_id_simd32(global int* in, global int* ids, global int* out) {
    size_t gid = get_global_id(0);
    int x = in[gid];
    uint which_sub_group_local_id = ids[gid];
// CHECK-LABEL: .kernel "test_sub_group_broadcast_non_immediate_sub_group_local_id_simd32"
// CHECK: shl (M1_NM, 1) ShuffleTmp(0,0)<1> {{.+}}(0,0)<0;1,0> 0x2:uw
// CHECK: addr_add (M1_NM, 1) A0(0)<1> &{{V[0-9]+}} ShuffleTmp(0,0)<0;1,0>
// CHECK: mov (M1, 32) simdBroadcast(0,0)<1> r[A0(0),0]<0;1,0>:d
// CHECK: lsc_store.ugm (M1, 32)  flat[{{.+}}]:a64  simdBroadcast:d32
    out[gid] = sub_group_broadcast(x, which_sub_group_local_id);
}

__attribute__((intel_reqd_sub_group_size(16)))
kernel void test_sub_group_broadcast_immediate_sub_group_local_id_simd16(global int* in, global int* ids, global int* out) {
    size_t gid = get_global_id(0);
    int x = in[gid];
    uint which_sub_group_local_id = 15;
// CHECK-LABEL: .kernel "test_sub_group_broadcast_immediate_sub_group_local_id_simd16"
// CHECK: mov (M1_NM, 1) simdBroadcast(0,0)<1> {{V[0-9]+}}(0,15)<0;1,0>
// CHECK: mov (M1, 16) simdBroadcastBroadcast(0,0)<1> simdBroadcast(0,0)<0;1,0>
// CHECK: lsc_store.ugm (M1, 16)  flat[{{.+}}]:a64  simdBroadcastBroadcast:d32
    out[gid] = sub_group_broadcast(x, which_sub_group_local_id);
}

__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_sub_group_broadcast_immediate_sub_group_local_id_simd32(global int* in, global int* ids, global int* out) {
    size_t gid = get_global_id(0);
    int x = in[gid];
    uint which_sub_group_local_id = 31;
// CHECK-LABEL: .kernel "test_sub_group_broadcast_immediate_sub_group_local_id_simd32"
// CHECK: mov (M5_NM, 1) simdBroadcast(0,0)<1> {{V[0-9]+}}(1,15)<0;1,0>
// CHECK: mov (M1, 32) simdBroadcastBroadcast(0,0)<1> simdBroadcast(0,0)<0;1,0>
// CHECK: lsc_store.ugm (M1, 32)  flat[{{.+}}]:a64  simdBroadcastBroadcast:d32
    out[gid] = sub_group_broadcast(x, which_sub_group_local_id);
}
