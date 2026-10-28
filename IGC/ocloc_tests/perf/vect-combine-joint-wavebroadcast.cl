/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, bmg-supported
// RUN: ocloc compile -file %s -device bmg -options "-igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s

// End to end: IGCVectorizer turns each group of sub_group_broadcasts into a
// JointWaveBroadcast, IGCVectorCoalescer concatenates the two halves for a
// wider multiply, and IGCVectCombine folds the concatenation into one wider
// JointWaveBroadcast. The multiply then reads the lanes of x directly instead
// of from a copy assembled one element at a time. The destination is matched
// by any name because release builds don't carry LLVM value names into vISA.

// Lanes 0..7 and 8..15 -> one 16-lane region starting at lane 0.
// CHECK-LABEL: .kernel "full"
// CHECK: lsc_load.ugm (M1, 16) [[X:[A-Za-z0-9_]+]]:d32
// CHECK-NOT: mov (M1_NM, 1) {{[A-Za-z0-9_]+}}(0,15)<1> [[X]](0,15)<0;1,0>
// CHECK: mul (M1_NM, 16) {{[A-Za-z0-9_]+}}(0,0)<1> [[X]](0,0)<1;1,0>
__attribute__((intel_reqd_sub_group_size(16))) kernel void full(global const float *in, global float *out) {
  float x = in[get_global_id(0)];
  float8 a, b;
  a.s0 = sub_group_broadcast(x, 8) * 2.0f;
  a.s1 = sub_group_broadcast(x, 9) * 2.0f;
  a.s2 = sub_group_broadcast(x, 10) * 2.0f;
  a.s3 = sub_group_broadcast(x, 11) * 2.0f;
  a.s4 = sub_group_broadcast(x, 12) * 2.0f;
  a.s5 = sub_group_broadcast(x, 13) * 2.0f;
  a.s6 = sub_group_broadcast(x, 14) * 2.0f;
  a.s7 = sub_group_broadcast(x, 15) * 2.0f;
  b.s0 = sub_group_broadcast(x, 0) * 2.0f;
  b.s1 = sub_group_broadcast(x, 1) * 2.0f;
  b.s2 = sub_group_broadcast(x, 2) * 2.0f;
  b.s3 = sub_group_broadcast(x, 3) * 2.0f;
  b.s4 = sub_group_broadcast(x, 4) * 2.0f;
  b.s5 = sub_group_broadcast(x, 5) * 2.0f;
  b.s6 = sub_group_broadcast(x, 6) * 2.0f;
  b.s7 = sub_group_broadcast(x, 7) * 2.0f;
  size_t g = get_group_id(0) * 32;
  vstore8(a, 0, out + g);
  vstore8(b, 0, out + g + 16);
}

// Lanes 6..9 and 10..13 -> one 8-lane region starting at lane 6.
// CHECK-LABEL: .kernel "offset"
// CHECK: lsc_load.ugm (M1, 16) [[X:[A-Za-z0-9_]+]]:d32
// CHECK-NOT: mov (M1_NM, 1) {{[A-Za-z0-9_]+}}(0,7)<1> [[X]](0,13)<0;1,0>
// CHECK: mul (M1_NM, 8) {{[A-Za-z0-9_]+}}(0,0)<1> [[X]](0,6)<1;1,0>
__attribute__((intel_reqd_sub_group_size(16))) kernel void offset(global const float *in, global float *out) {
  float x = in[get_global_id(0)];
  float4 a, b;
  a.s0 = sub_group_broadcast(x, 10) * 2.0f;
  a.s1 = sub_group_broadcast(x, 11) * 2.0f;
  a.s2 = sub_group_broadcast(x, 12) * 2.0f;
  a.s3 = sub_group_broadcast(x, 13) * 2.0f;
  b.s0 = sub_group_broadcast(x, 6) * 2.0f;
  b.s1 = sub_group_broadcast(x, 7) * 2.0f;
  b.s2 = sub_group_broadcast(x, 8) * 2.0f;
  b.s3 = sub_group_broadcast(x, 9) * 2.0f;
  size_t g = get_group_id(0) * 16;
  vstore4(a, 0, out + g);
  vstore4(b, 0, out + g + 8);
}
