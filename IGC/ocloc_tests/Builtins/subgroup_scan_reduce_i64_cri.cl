/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: regkeys, cri-supported, llvm-16-plus

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'EnableOpaquePointersBackend=1,DumpVISAASMToConsole=1'" | FileCheck %s

// i64 sub-group add reduce and scan must be emitted as SIMD-wide instructions, not as a serial chain of SIMD1 adds
// or a per-channel loop.

__attribute__((intel_reqd_sub_group_size(32))) kernel void scan_incl32(global long *out, global long *in) {
  // CHECK-LABEL: .kernel "scan_incl32"
  // First three steps are split in two instructions to fit in 2 GRFs per region.
  // CHECK: add (M1_NM, 8) [[T:[A-z0-9_]*]](0,1)<2> [[T]](0,0)<2;1,0> [[T]](0,1)<2;1,0>
  // CHECK: add (M1_NM, 8) [[T]](2,1)<2> [[T]](2,0)<2;1,0> [[T]](2,1)<2;1,0>
  // CHECK-COUNT-2: add (M1_NM, 4) [[T]]({{[0-9]}},2)<4>
  // CHECK-COUNT-2: add (M1_NM, 4) [[T]]({{[0-9]}},3)<4>
  // CHECK-COUNT-4: add (M1_NM, 4) [[T]]({{[0-9]}},4)<1>
  // CHECK-COUNT-2: add (M1_NM, 8) [[T]]({{[0-9]}},0)<1>
  // CHECK: add (M1_NM, 16) [[T]](2,0)<1>
  // CHECK-NOT: add (M1_NM, 1)
  // CHECK: lsc_store
  size_t i = get_global_id(0);
  out[i] = sub_group_scan_inclusive_add(in[i]);
}

__attribute__((intel_reqd_sub_group_size(32))) kernel void scan_excl32(global long *out, global long *in) {
  // CHECK-LABEL: .kernel "scan_excl32"
  // The input is shifted by one lane; a 16-lane copy at an offset would span 3 GRFs, so it is split.
  // CHECK: mov (M1_NM, 1) [[T:[A-z0-9_]*]](0,0)<1> 0x0:q
  // CHECK: mov (M1_NM, 8) [[T]](0,1)<1> {{.*}}(0,0)<1;1,0>
  // CHECK: mov (M1_NM, 8) [[T]](1,1)<1> {{.*}}(1,0)<1;1,0>
  // CHECK: add (M1_NM, 8) [[T]](0,1)<2>
  // CHECK: add (M1_NM, 8) [[T]](2,1)<2>
  // CHECK-NOT: add (M1_NM, 1)
  // CHECK: lsc_store
  size_t i = get_global_id(0);
  out[i] = sub_group_scan_exclusive_add(in[i]);
}

__attribute__((intel_reqd_sub_group_size(16))) kernel void scan_incl16(global long *out, global long *in) {
  // CHECK-LABEL: .kernel "scan_incl16"
  // The vector 64-bit scan is implemented for SIMD32 only. SIMD16 must stay on the scalar path: the legacy vector code
  // assumes a 32B GRF and would leave lanes 8-15 unprocessed.
  // CHECK-NOT: add (M1_NM, 4)
  // CHECK-NOT: add (M1_NM, 8)
  // CHECK: add (M1_NM, 1)
  // CHECK: lsc_store
  size_t i = get_global_id(0);
  out[i] = sub_group_scan_inclusive_add(in[i]);
}

__attribute__((intel_reqd_sub_group_size(32))) kernel void reduce_add32(global long *out, global long *in) {
  // CHECK-LABEL: .kernel "reduce_add32"
  // A halving tree 16 -> 8 -> 4 -> 2 -> 1 lanes, each level reading the previous one.
  // CHECK: add (M1_NM, 16) [[D16:[A-z0-9_]+]](0,0)<1> [[S:[A-z0-9_]+]](0,0)<1;1,0> [[S]](2,0)<1;1,0>
  // CHECK: add (M1_NM, 8) [[D8:[A-z0-9_]+]](0,0)<1> [[D16]](0,0)<1;1,0> [[D16]](1,0)<1;1,0>
  // CHECK: add (M1_NM, 4) [[D4:[A-z0-9_]+]](0,0)<1> [[D8]](0,0)<1;1,0> [[D8]](0,4)<1;1,0>
  // CHECK: add (M1_NM, 2) [[D2:[A-z0-9_]+]](0,0)<1> [[D4]](0,0)<1;1,0> [[D4]](0,2)<1;1,0>
  // CHECK: add (M1_NM, 1) {{[A-z0-9_]+}}(0,0)<1> [[D2]](0,0)<0;1,0> [[D2]](0,1)<0;1,0>
  size_t i = get_global_id(0);
  out[i] = sub_group_reduce_add(in[i]);
}

__attribute__((intel_reqd_sub_group_size(16))) kernel void reduce_add16(global long *out, global long *in) {
  // CHECK-LABEL: .kernel "reduce_add16"
  // A halving tree 8 -> 4 -> 2 -> 1 lanes, each level reading the previous one.
  // CHECK: add (M1_NM, 8) [[E8:[A-z0-9_]+]](0,0)<1> [[T:[A-z0-9_]+]](0,0)<1;1,0> [[T]](1,0)<1;1,0>
  // CHECK: add (M1_NM, 4) [[E4:[A-z0-9_]+]](0,0)<1> [[E8]](0,0)<1;1,0> [[E8]](0,4)<1;1,0>
  // CHECK: add (M1_NM, 2) [[E2:[A-z0-9_]+]](0,0)<1> [[E4]](0,0)<1;1,0> [[E4]](0,2)<1;1,0>
  // CHECK: add (M1_NM, 1) {{[A-z0-9_]+}}(0,0)<1> [[E2]](0,0)<0;1,0> [[E2]](0,1)<0;1,0>
  size_t i = get_global_id(0);
  out[i] = sub_group_reduce_add(in[i]);
}
