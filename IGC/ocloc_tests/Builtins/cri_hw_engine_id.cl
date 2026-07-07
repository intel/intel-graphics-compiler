/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The test checks if "hw_engine_id" intrinsic is used by platforms
// that support HW EngineID (>=XE3P). This value is set by HW.
// For other platforms, we should set 0x0 value.

// REQUIRES: cri-supported

// RUN: ocloc compile -file %s -device cri \
// RUN: -options "-igc_opts 'DumpVISAASMToConsole=1'" | FileCheck %s

// CHECK: and (M1_NM, 1) [[HWENGINEID:.*]](0,0)<1> %[[SR0:.*]](0,1)<0;1,0> 0x3f0000:d
// CHECK: and (M1_NM, 1) [[HWENGINEID1:.*]](0,0)<1> [[HWENGINEID]](0,0)<0;1,0> 0x0:d
// CHECK: asr (M1_NM, 1) [[HWENGINEID0:.*]](0,0)<1> [[HWENGINEID]](0,0)<0;1,0> 0x10:d
// CHECK: bfn.xf8 (M1_NM, 1) [[HWENGINEID]](0,0)<1> [[HWENGINEID0]](0,0)<0;1,0> 0xffffffff:d [[HWENGINEID1]](0,0)<0;1,0>
// CHECK: mov (M1, 32) [[A:.*]](0,0)<1> [[HWENGINEID]](0,0)<0;1,0>

uint __builtin_IB_hw_engine_id(void) __attribute__((const));

kernel void test_kernel(global uint *output) {
  output[0] = __builtin_IB_hw_engine_id();
};
