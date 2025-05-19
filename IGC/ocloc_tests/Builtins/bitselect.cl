/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, xe2-supported
// UNSUPPORTED: sys32

// RUN: ocloc compile -file %s -options " -igc_opts 'DumpVISAASMToConsole=1'" -device bmg | FileCheck %s

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
// CHECK: lsc_load.ugm (M1, 32)  [[A:V[0-9]+]]:d32x2  flat[{{V[0-9]+}}]:a64
// CHECK: lsc_load.ugm (M1, 32)  [[B:V[0-9]+]]:d32x2  flat[{{V[0-9]+}}]:a64
// CHECK: lsc_load.ugm (M1, 32)  [[C:V[0-9]+]]:d32x2  flat[{{V[0-9]+}}]:a64
// CHECK: bfn.xd8 (M1, 32) [[RESULT:.+]](0,0)<1> [[C]](0,0)<1;1,0> [[B]](0,0)<1;1,0> [[A]](0,0)<1;1,0>
// CHECK: bfn.xd8 (M1, 32) [[RESULT]](2,0)<1> [[C]](2,0)<1;1,0> [[B]](2,0)<1;1,0> [[A]](2,0)<1;1,0>
// CHECK: lsc_store.ugm (M1, 32)  flat[{{V[0-9]+}}]:a64  [[RESULT]]:d32x2
__attribute__((intel_reqd_sub_group_size(32)))
kernel void test_i64(global long* a, global long* b, global long* c) {
    int i = get_global_id(0);
    a[i] = bitselect(a[i], b[i], c[i]);
}
