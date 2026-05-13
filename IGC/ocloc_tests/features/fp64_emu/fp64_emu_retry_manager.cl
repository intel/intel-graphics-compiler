/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Regression test for the FP64 emulation silent-exit through the retry manager.
//
// Background:
//   On platforms without native FP64 (e.g. DG2), IGC uses DP emulation via PreCompiledFuncImport.
//   For slow DP emulation builtins (e.g. __igcbuiltin_dp_add) PreCompiledFuncImport calls
//   m_retryManager->Disable(true) to skip retry and save compile time.
//
//   shouldForceEarlyRecompile() method didn't consider the information about disabling
//   retry manger in context (what was set by DP emulation in PreCompiledFuncImport)
//   and allows to trigger early recompilation, which is not expected for  DP emulation.
//   In result, the kernel finished with silent exit without emitting asmembly and without any error.
//
// Expected behavior:
//   With the fix, EmitPass checks context and sees IsEnabled()==false (DP-emu disabled the
//   retry manager) and skips the early-recompile path.

// UNSUPPORTED: system-windows
// REQUIRES: regkeys, dg2-supported, llvm-16-plus

// RUN: ocloc compile -file %s -device dg2 \
// RUN: -options "-igc_opts 'EarlyRetryDefaultGRFThreshold=0,EarlyRetryLargeGRFThreshold=0,EnableOpaquePointersBackend=1,DumpASMToConsole=1' -cl-fp64-gen-emu" \
// RUN: -internal_options "-cl-ext=-all,+cl_khr_fp64" 2>&1 | FileCheck %s

// CHECK: .kernel fadd_kernel

#pragma OPENCL EXTENSION cl_khr_fp64 : enable
__kernel void fadd_kernel(__global double *in, __global double *out, int N) {
  double sum = 0.0;
  for (int i = 0; i < N; i++) {
    sum += in[i];
  }
  out[get_global_id(0)] = sum;
}
