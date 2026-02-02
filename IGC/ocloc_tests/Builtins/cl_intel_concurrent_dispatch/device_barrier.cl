/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// RUN: ocloc compile -file %s -device dg2 -options "-cl-std=CL3.0 -igc_opts 'DumpVISAASMToConsole=1'" -internal_options "-cl-ext=-all,+cl_intel_concurrent_dispatch" | FileCheck %s

// CHECK-LABEL: .kernel "test"
// CHECK-NOT: call
kernel void test() {
  if (intel_is_device_barrier_valid()) {
    intel_device_barrier(CLK_LOCAL_MEM_FENCE);
    intel_device_barrier(CLK_LOCAL_MEM_FENCE, memory_scope_device);
  }
}
