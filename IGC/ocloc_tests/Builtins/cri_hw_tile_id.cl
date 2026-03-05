/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The test checks if "hw_tile_id" intrinsic is used by platforms
// that support HW TileID (>=XE3P). Before XE3P platform,
// physical TileID was missing, and it had to be patched by SW (additional memory reads).

// REQUIRES: cri-supported

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-igc_opts 'DumpVISAASMToConsole=1'" | FileCheck %s

// CHECK: and (M1_NM, 1) [[HWTILEID:.*]](0,0)<1> %[[SR0:.*]](0,1)<0;1,0> 0xf00:d
// CHECK: and (M1_NM, 1) [[HWTILEID1:.*]](0,0)<1> [[HWTILEID]](0,0)<0;1,0> 0x0:d
// CHECK: asr (M1_NM, 1) [[HWTILEID0:.*]](0,0)<1> [[HWTILEID]](0,0)<0;1,0> 0x8:d
// CHECK: bfn.xf8 (M1_NM, 1) [[HWTILEID]](0,0)<1> [[HWTILEID0]](0,0)<0;1,0> 0xffffffff:d [[HWTILEID1]](0,0)<0;1,0>
// CHECK: mov (M1, 32) [[A:.*]](0,0)<1> [[HWTILEID]](0,0)<0;1,0>

uint __attribute__((overloadable)) intel_get_tile_id(void);

kernel void test_kernel(global uint *output) {
  output[0] = intel_get_tile_id();
};
