/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, xe2-supported, llvm-16-plus
// UNSUPPORTED: sys32

// RUN: ocloc compile -file %s -options " -igc_opts 'EnableOpaquePointersBackend=1 DumpVISAASMToConsole=1'" -device bmg | FileCheck %s

// CHECK: .kernel "test_i32"
// CHECK: lsc_load.ugm (M1, 32)  [[A:V[0-9]+]]:d32  flat[{{V[0-9]+}}]:a64
// CHECK: lsc_load.ugm (M1, 32)  [[B:V[0-9]+]]:d32  flat[{{V[0-9]+}}]:a64
// CHECK: lsc_load.ugm (M1, 32)  [[C:V[0-9]+]]:d32  flat[{{V[0-9]+}}]:a64
// CHECK: bfn.xd8 (M1, 32) [[RESULT:V[0-9]+]](0,0)<1> [[C]](0,0)<1;1,0> [[B]](0,0)<1;1,0> [[A]](0,0)<1;1,0>
// CHECK: lsc_store.ugm (M1, 32)  flat[{{V[0-9]+}}]:a64  [[RESULT]]:d32
__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_i32(global int* a, global int* b, global int* c) {
    int i = get_global_id(0);
    a[i] = bitselect(a[i], b[i], c[i]);
}

// CHECK: .kernel "test_i64"
// CHECK: lsc_load.ugm (M1, 32)  [[A_LOAD:V[0-9]+]]:d64  flat[{{V[0-9]+}}]:a64
// CHECK: lsc_load.ugm (M1, 32)  [[B_LOAD:V[0-9]+]]:d64  flat[{{V[0-9]+}}]:a64
// CHECK: lsc_load.ugm (M1, 32)  [[C_LOAD:V[0-9]+]]:d64  flat[{{V[0-9]+}}]:a64

// Extract high dwords (feed 2nd bfn.xd8)
// CHECK-DAG: mov (M1, 16) [[A_HI:V[0-9]+]](0,0)<1> {{V[0-9]+}}(0,1)<2;1,0>
// CHECK-DAG: mov (M1, 16) [[B_HI:V[0-9]+]](0,0)<1> {{V[0-9]+}}(0,1)<2;1,0>
// CHECK-DAG: mov (M1, 16) [[C_HI:V[0-9]+]](0,0)<1> {{V[0-9]+}}(0,1)<2;1,0>

// Extract low dwords (feed 1st bfn.xd8)
// CHECK-DAG: mov (M1, 32) [[C_LO:V[0-9]+]](0,0)<1> [[C_LOAD]](0,0)<1;1,0>
// CHECK-DAG: mov (M1, 32) [[B_LO:V[0-9]+]](0,0)<1> [[B_LOAD]](0,0)<1;1,0>
// CHECK-DAG: mov (M1, 32) [[A_LO:V[0-9]+]](0,0)<1> [[A_LOAD]](0,0)<1;1,0>

// CHECK: bfn.xd8 (M1, 32) [[VEC:[A-Za-z0-9_]+]](0,0)<1> [[C_LO]](0,0)<1;1,0> [[B_LO]](0,0)<1;1,0> [[A_LO]](0,0)<1;1,0>
// CHECK: bfn.xd8 (M1, 32) [[VEC]](2,0)<1> [[C_HI]](0,0)<1;1,0> [[B_HI]](0,0)<1;1,0> [[A_HI]](0,0)<1;1,0>
// CHECK: lsc_store.ugm (M1, 32)  flat[{{V[0-9]+}}]:a64  [[VEC]]:d32x2

__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_i64(global long* a, global long* b, global long* c) {
    int i = get_global_id(0);
    a[i] = bitselect(a[i], b[i], c[i]);
}

// CHECK: .kernel "test_const_i32v2"
// CHECK: mov (M1_NM, 1) [[RESULT:V[0-9]+]](0,0)<1> 0xffff89da:d
// CHECK: mov (M1_NM, 1) [[RESULT]](0,1)<1> 0x348:d
// CHECK: lsc_store.ugm (M1_NM, 1)  flat[{{.*}}]:a64  [[RESULT]]:d32x2t
kernel void test_const_i32v2(global int2* ptr) {
    int2 a = { -30373, -6389 };
    int2 b = { -19240,   832 };
    int2 c = { -32631, -4381 };
    *ptr = bitselect(a, b, c);
}
