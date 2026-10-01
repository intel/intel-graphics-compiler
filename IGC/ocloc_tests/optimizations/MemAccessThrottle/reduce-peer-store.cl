/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The fence is what keeps the reload of out1 alive.

// REQUIRES: regkeys, cri-supported

// The option is the primary control.
// RUN: ocloc compile -file %s -device cri -internal_options "-ze-opt-mem-access-throttle" -options " -igc_opts 'VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefix=CHECK-ASM

// The regkey can force it on without the option, and off despite it.
// RUN: ocloc compile -file %s -device cri -options " -igc_opts 'MemAccessThrottle=1 VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefix=CHECK-ASM
// RUN: ocloc compile -file %s -device cri -internal_options "-ze-opt-mem-access-throttle" -options " -igc_opts 'MemAccessThrottle=0 DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-OFF

// Opt-in only.
// RUN: ocloc compile -file %s -device cri -options " -igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-OFF

// The pause is a constant, so tm0.4 takes the immediate path: 0x3E0, ~992 clocks.
// CHECK-ASM: load.ugm.d32x4.a64
// CHECK-ASM: load.ugm.d32x4.a64
// CHECK-ASM: store.ugm.d32x4.a64
// CHECK-ASM: load.ugm.d32x4.a64
// CHECK-ASM: cmp {{.*}}(eq)f0.0 {{.*}} 0:w
// CHECK-ASM: store.ugm.d32x4.a64
// CHECK-ASM: mov (1|M0) tm0.4<1>:ud 0x3E0:uw
// CHECK-ASM: signal barrier
// CHECK-ASM: sync.bar

// CHECK-OFF: lsc_store.ugm
// CHECK-OFF-NOT: %tsc(0,4)

__kernel void allreduce_step(__global float4 *in1, __global float4 *in2, __global float4 *out1, __global float4 *out2) {
  size_t i = get_global_id(0);
  out1[i] = in1[i] + in2[i];
  mem_fence(CLK_GLOBAL_MEM_FENCE);
  out2[i] = out1[i];
}
