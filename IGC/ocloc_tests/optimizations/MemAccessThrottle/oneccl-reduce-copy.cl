/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// The kernel the workaround exists for: compute-benchmarks p2p_benchmark
// UsmEUReduceCopyRing, the oneCCL ring reduce-copy step, with its hand-written
// throttle removed so the pass has to supply one. Measured on 2x CRI: 30 GB/s
// without the throttle, 112 with it, against a 115 full duplex ceiling.
//
// Two things here defeated earlier versions of the pass, so keep them exactly
// as the benchmark has them:
//   - the reload of out1 is volatile, which is what keeps it alive at all
//   - the loop starts at get_group_id(0) * get_local_size(0), which WIAnalysis
//     reports as UNIFORM_THREAD because the group id comes out of R0

// REQUIRES: regkeys, cri-supported

// RUN: ocloc compile -file %s -device cri -internal_options "-ze-opt-mem-access-throttle" -options " -igc_opts 'VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefix=CHECK-ASM
// RUN: ocloc compile -file %s -device cri -options " -igc_opts 'MemAccessThrottle=1 VISAOptions=-asmToConsole'" 2>&1 | FileCheck %s --check-prefix=CHECK-ASM

// Opt-in only.
// RUN: ocloc compile -file %s -device cri -options " -igc_opts 'DumpVISAASMToConsole=1'" 2>&1 | FileCheck %s --check-prefix=CHECK-OFF

// The peer store sits under the bounds guard, so the throttle has to go after
// the join that reconverges it, at the loop latch. A pause inside the guard
// would hang the last, partly populated work-group.
// CHECK-ASM: load.ugm.d32x4.a64
// CHECK-ASM: load.ugm.d32x4.a64
// CHECK-ASM: store.ugm.d32x4.a64
// CHECK-ASM: load.ugm.d32x4.a64
// CHECK-ASM: store.ugm.d32x4.a64
// CHECK-ASM: join
// CHECK-ASM: mov (1|M0) tm0.4<1>:ud 0x3E0:uw
// CHECK-ASM: signal barrier
// CHECK-ASM: sync.bar

// CHECK-OFF: lsc_store.ugm
// CHECK-OFF-NOT: %tsc(0,4)

__attribute__((intel_reqd_sub_group_size(16))) kernel void reduce_copy_throttled(global const float4 *in1,
                                                                                 global const float4 *in2,
                                                                                 global float4 *out1,
                                                                                 global float4 *out2, ulong count) {
  const size_t lid = get_local_id(0);
  for (size_t base = get_group_id(0) * get_local_size(0); base < count; base += get_global_size(0)) {
    const size_t idx = base + lid;
    if (idx < count) {
      out1[idx] = in1[idx] + in2[idx];
      out2[idx] = ((volatile global float4 *)out1)[idx];
    }
  }
}
