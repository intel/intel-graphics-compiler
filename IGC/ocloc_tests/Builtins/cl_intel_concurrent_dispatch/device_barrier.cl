/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys

// global_barrier() dispatches on PlatformType (see barrier.cl): pre-NVL platforms get
// __global_barrier_nonatomic(), NVL and later get __global_barrier_atomic(). Cover both.
// RUN: %if dg2-supported %{ ocloc compile -file %s -device dg2 -options "-cl-std=CL3.0 -igc_opts 'DumpVISAASMToConsole=1'" -internal_options "-cl-ext=-all,+cl_intel_concurrent_dispatch" | FileCheck %s %}
// RUN: %if cri-supported %{ ocloc compile -file %s -device cri -options "-cl-std=CL3.0 -igc_opts 'DumpVISAASMToConsole=1'" -internal_options "-cl-ext=-all,+cl_intel_concurrent_dispatch" | FileCheck %s %}

// The barrier builtins must be fully inlined - no vISA subroutine (call) or stack call
// (fcall) may survive. The positive checks keep that from passing vacuously on a kernel
// whose barrier body was optimized away: both dispatch paths expand to at least a UGM
// fence and a workgroup barrier (dg2 scopes the fence .tile and cri .gpu, so no suffix).
// CHECK-LABEL: .kernel "test"
// CHECK-NOT: call
// CHECK: lsc_fence.ugm
// CHECK: barrier
// CHECK-NOT: call
kernel void test() {
  if (intel_is_device_barrier_valid()) {
    intel_device_barrier(CLK_LOCAL_MEM_FENCE);
    intel_device_barrier(CLK_LOCAL_MEM_FENCE, memory_scope_device);
  }
}
