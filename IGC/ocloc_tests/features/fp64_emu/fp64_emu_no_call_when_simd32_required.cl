/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Regression test for github issue #397: wrong FP64 results when a kernel
// requires SIMD32 on a platform without native FP64.
//
// Background:
//   On platforms without native FP64 (e.g. DG2, TGLLP) PreCompiledFuncImport
//   rewrites double arithmetic into __igcbuiltin_dp_* calls and, for the slow DP
//   emulation builtins, keeps them as subroutines/stack calls instead of inlining
//   them.
//
//   IGC otherwise avoids SIMD32 whenever the function group contains calls on
//   platforms that need the fused-EU call workaround: ForceLowestSIMDForStackCalls
//   and the requireCallWA() SIMD32 bail-out in COpenCLKernel::checkSIMDCompileConds().
//   Both of those guards only apply when no sub-group size is required, so a kernel
//   with intel_reqd_sub_group_size(32) (or SPIR-V OpExecutionMode SubgroupSize 32)
//   reached SIMD32 codegen with the emulation still in call form. The generated code
//   is wrong with no diagnostic: the double returned by an emulated call is replaced
//   by an unrelated live value of the caller, so e.g. rsqrt() yields another local.
//
// Expected behavior:
//   With the fix the emulation is inlined for such kernels, so no call to any
//   __igcbuiltin_dp_* function is emitted and no stack call is created.

// UNSUPPORTED: system-windows
// REQUIRES: regkeys, dg2-supported, llvm-16-plus

// RUN: ocloc compile -file %s -device dg2 \
// RUN: -options "-igc_opts 'DumpASMToConsole=1' -cl-fp64-gen-emu" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp64,+cl_intel_subgroups" 2>&1 \
// RUN:   | FileCheck %s --implicit-check-not="Stack call has been detected" \
// RUN:                  --implicit-check-not="___igcbuiltin_dp_"

// COM: The two patterns above are checked over the whole output rather than with
// COM: leading CHECK-NOT directives, which would only guard the region ahead of
// COM: the first positive match. The kernel must still be generated, so the
// COM: absence of the emulation calls cannot be satisfied by an empty dump.
// CHECK: .kernel dp_sg32

#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__attribute__((intel_reqd_sub_group_size(32))) __kernel void dp_sg32(__global double *in, __global double *out) {
  int lid = get_local_id(0) & 31;
  double x = in[lid];
  double acc = 0.0;
  for (uint j = 0; j < 32; j++) {
    double s = intel_sub_group_shuffle(x, j);
    double d = s - x;
    double r2 = d * d;
    double inv = rsqrt(r2);
    double t = 0.0;
    if (j != (uint)lid)
      t = x * s * inv * inv;
    acc -= d * t;
  }
  out[lid] = acc;
}
