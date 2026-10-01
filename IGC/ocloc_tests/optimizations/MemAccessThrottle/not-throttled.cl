/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Shapes the throttle must stay out of, even with the regkey forced on. The
// pass cannot tell own-device memory from peer memory, so matching too widely
// slows down kernels that have nothing to do with peer traffic.

// REQUIRES: regkeys, cri-supported

// RUN: ocloc compile -file %s -device cri -options " -igc_opts 'MemAccessThrottle=1 VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --implicit-check-not=tm0.4

// CHECK: Build succeeded.

// A constant offset from the first store is the same buffer, not a peer.
kernel void self_copy(global const float4 *in1, global const float4 *in2, global float4 *out1) {
  const size_t i = get_global_id(0);
  out1[i] = in1[i] + in2[i];
  out1[i + 64] = ((volatile global float4 *)out1)[i];
}

// Below one d32x4 message: too small to be the bandwidth problem.
kernel void too_small(global const float *in1, global const float *in2, global float *out1, global float *out2) {
  const size_t i = get_global_id(0);
  out1[i] = in1[i] + in2[i];
  out2[i] = ((volatile global float *)out1)[i];
}

// Two independent stores, no staged copy through out1.
kernel void no_reload(global const float4 *in1, global const float4 *in2, global float4 *out1, global float4 *out2) {
  const size_t i = get_global_id(0);
  out1[i] = in1[i] + in2[i];
  out2[i] = in1[i] + in2[i];
}

// The staging buffer is SLM, so nothing leaves the device until the last store.
kernel void through_slm(global const float4 *in1, global const float4 *in2, global float4 *out2,
                        local float4 *scratch) {
  const size_t i = get_global_id(0);
  const size_t l = get_local_id(0);
  scratch[l] = in1[i] + in2[i];
  out2[i] = ((volatile local float4 *)scratch)[l];
}

// A barrier needs the whole work-group, which a callee cannot guarantee.
__attribute__((noinline)) void step(global const float4 *in1, global const float4 *in2, global float4 *out1,
                                    global float4 *out2, size_t idx) {
  out1[idx] = in1[idx] + in2[idx];
  out2[idx] = ((volatile global float4 *)out1)[idx];
}

kernel void in_callee(global const float4 *in1, global const float4 *in2, global float4 *out1, global float4 *out2,
                      ulong count) {
  const size_t idx = get_global_id(0);
  if (idx < count)
    step(in1, in2, out1, out2, idx);
}
