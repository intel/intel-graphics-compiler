/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// -ze-exp-register-file-size fixes the GRF number, so register pressure passes
// plan for that number instead of the automatic selection's budgets. Loop
// sinking skips the VRT targets and its threshold is the size plus the 30-GRF
// LoopSinkThresholdDelta.

// UNSUPPORTED: system-windows, release
// REQUIRES: regkeys, cri-supported

// RUN: rm -rf %t_128 %t_160
// RUN: ocloc compile -file %s -device cri -options "-ze-exp-register-file-size 128 \
// RUN:   -igc_opts 'ShaderDumpEnable=1, DumpToCustomDir=%t_128, DumpLoopSink=1, CodeLoopSinkingMinSize=1'"
// RUN: cat %t_128/*loopsink.txt | FileCheck %s --check-prefix=CHECK128

// RUN: ocloc compile -file %s -device cri -options "-ze-exp-register-file-size 160 \
// RUN:   -igc_opts 'ShaderDumpEnable=1, DumpToCustomDir=%t_160, DumpLoopSink=1, CodeLoopSinkingMinSize=1'"
// RUN: cat %t_160/*loopsink.txt | FileCheck %s --check-prefix=CHECK160

// CHECK128: Checking loop with preheader
// CHECK128-NOT: Trying VRT target
// CHECK128: Threshold to sink = 158

// CHECK160: Checking loop with preheader
// CHECK160-NOT: Trying VRT target
// CHECK160: Threshold to sink = 190

__kernel void foo(__global float *in, __global float *out, int n) {
  int gid = get_global_id(0);
  float x = in[gid];
  float a = x * 2.0f;
  float b = x * 3.0f;
  float c = x * 5.0f;
  float acc = 0.0f;
  for (int i = 0; i < n; ++i)
    acc = (acc + a) * b - c;
  out[gid] = acc;
}
