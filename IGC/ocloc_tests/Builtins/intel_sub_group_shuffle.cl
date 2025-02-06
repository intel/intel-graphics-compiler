/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// UNSUPPORTED: sys32
// REQUIRES: regkeys, pvc-supported

// RUN: ocloc compile -file %s -device pvc -options "-igc_opts 'DumpVISAASMToConsole=1'" | FileCheck %s

// CHECK-LABEL: .kernel "test_intel_sub_group_shuffle_immediate_index_simd32"
__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_intel_sub_group_shuffle_immediate_index_simd32(global int* in, global int* ids, global int* out) {
    size_t gid = get_global_id(0);
    int x = in[gid];

// CHECK: mov (M5_NM, 1) simdShuffle(0,0)<1> V0039(1,15)<0;1,0>

// CHECK: mov (M1, 32) simdShuffleBroadcast(0,0)<1> simdShuffle(0,0)<0;1,0>
// CHECK: lsc_store.ugm (M1, 32)  flat[V0041]:a64  simdShuffleBroadcast:d32
    out[gid] = intel_sub_group_shuffle(x, 31);
}

// CHECK-LABEL: .kernel "test_intel_sub_group_shuffle_uniform_non_immediate_index_simd32"
__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_intel_sub_group_shuffle_uniform_non_immediate_index_simd32(global int* in, global int* ids, uint which_sub_group_local_id, global int* out) {
    size_t gid = get_global_id(0);
    int x = in[gid];

// CHECK: shl (M1_NM, 1) ShuffleTmp(0,0)<1> which_sub_group_local_id_0(0,0)<0;1,0> 0x2:uw
// CHECK-NEXT: addr_add (M1_NM, 1) A0(0)<1> &{{V[0-9]+}} ShuffleTmp(0,0)<0;1,0>
// CHECK-NEXT: mov (M1_NM, 1) simdShuffle(0,0)<1> r[A0(0),0]<0;1,0>:d

// CHECK: mov (M1, 32) simdShuffleBroadcast(0,0)<1> simdShuffle(0,0)<0;1,0>
// CHECK: lsc_store.ugm (M1, 32)  flat[{{.+}}]:a64  simdShuffleBroadcast:d32
    out[gid] = intel_sub_group_shuffle(x, which_sub_group_local_id);
}

// CHECK-LABEL: .kernel "test_intel_sub_group_shuffle_non_uniform_non_immediate_index_simd32"
__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_intel_sub_group_shuffle_non_uniform_non_immediate_index_simd32(global int* in, global int* ids, global int* out) {
    size_t gid = get_global_id(0);
    int x = in[gid];
    uint which_sub_group_local_id = ids[gid];

// CHECK: shl (M1, 32) ShuffleTmp(0,0)<1> {{V[0-9]+}}(0,0)<16;8,2> 0x2:uw
// CHECK-NEXT: mov (M1_NM, 16) A0(0)<1> 0x0:uw
// CHECK-NEXT: addr_add (M1, 16) A0(0)<1> &[[X:V[0-9]+]] ShuffleTmp(0,0)<1;1,0>
// CHECK-NEXT: mov (M1, 16) simdShuffle(0,0)<1> r[A0(0),0]<1,0>:d
// CHECK-NEXT: mov (M5_NM, 16) A0(0)<1> 0x0:uw
// CHECK-NEXT: addr_add (M5, 16) A0(0)<1> &[[X]] ShuffleTmp(0,16)<1;1,0>
// CHECK-NEXT: mov (M5, 16) simdShuffle(1,0)<1> r[A0(0),0]<1,0>:d

// CHECK: lsc_store.ugm (M1, 32)  flat[{{.+}}]:a64  simdShuffle:d32
    out[gid] = intel_sub_group_shuffle(x, which_sub_group_local_id);
}

// CHECK-LABEL: .kernel "test_intel_sub_group_shuffle_non_uniform_non_immediate_index_src_the_same_as_dst_simd32"
__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_intel_sub_group_shuffle_non_uniform_non_immediate_index_src_the_same_as_dst_simd32(global int* in, global int* ids, uint num_iterations, global int* out) {
    size_t gid = get_global_id(0);
    int x = in[gid];
    uint which_sub_group_local_id = ids[gid];

    for (uint i = 0; i < num_iterations; ++i)
    {
// CHECK: shl (M1, 32) ShuffleTmp(0,0)<1> {{V[0-9]+}}(0,0)<16;8,2> 0x2:uw
// CHECK-NEXT: mov (M1_NM, 16) A0(0)<1> 0x0:uw
// CHECK-NEXT: addr_add (M1, 16) A0(0)<1> &[[X:V[0-9]+]] ShuffleTmp(0,0)<1;1,0>
// CHECK-NEXT: mov (M1, 16) first16LanesResult(0,0)<1> r[A0(0),0]<1,0>:d
// CHECK-NEXT: mov (M5_NM, 16) A0(0)<1> 0x0:uw
// CHECK-NEXT: addr_add (M5, 16) A0(0)<1> &[[X]] ShuffleTmp(0,16)<1;1,0>
// CHECK-NEXT: mov (M5, 16) [[X]](1,0)<1> r[A0(0),0]<1,0>:d
// CHECK-NEXT: mov (M1, 16) [[X]](0,0)<1> first16LanesResult(0,0)<1;1,0>
        x = intel_sub_group_shuffle(x, which_sub_group_local_id);
    }

// CHECK: lsc_store.ugm (M1, 32)  flat[{{.+}}]:a64  [[X]]:d32
    out[gid] = x;
}
