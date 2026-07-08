/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys
// RUN: %if ptl-supported %{ ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'VISAOptions=-asmToConsole'" -device ptl | FileCheck %s %}
// RUN: %if cri-supported %{ ocloc compile -file %s -options "-cl-std=CL3.0 -igc_opts 'VISAOptions=-asmToConsole'" -device cri | FileCheck %s %}

// A send/atomic-dominated kernel with very low register pressure and almost no
// bank-conflict-prone ALU ops. It takes the low-register-pressure BCR path
// (enableBCR is on), but the ALU-density guard must not turn on forceBCR here:
// there is nothing for bank-conflict reduction to reduce, so forceBCR would only
// perturb the schedule.

// clang-format off
__kernel void atomic_lowrp(__global int *out, __local atomic_int *slm) {
  __attribute__((opencl_unroll_hint(64)))
  for (int i = 0; i < 64; i++)
    atomic_fetch_add_explicit(&slm[0], 1, memory_order_relaxed, memory_scope_device);
  out[get_global_id(0)] = atomic_load_explicit(&slm[0], memory_order_relaxed, memory_scope_device);
}
// clang-format on

// CHECK-LABEL: .kernel atomic_lowrp
// CHECK: -enableBCR
// CHECK-NOT: -forceBCR
