/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, dg2-supported
// RUN: ocloc compile -file %s -options "-cl-opt-disable -igc_opts 'VISAOptions=-maxRAIterations 1'" -device dg2 | FileCheck %s

// This test verifies that the GRF register allocator reports failure, rather
// than returning success with unallocated variables, when it runs out of RA
// iterations while fail-safe RA is enabled.
//
// GlobalRA::setupFailSafeIfNeeded() turns fail-safe RA on when
// getIterNo() == maxRAIterations - 1. With -maxRAIterations 1 that is
// iteration 0, so fail-safe RA is enabled on the very first iteration. If that
// iteration still has to insert spill code it creates new spill/fill
// temporaries and bumps iterationNo to maxRAIterations, and there is no further
// iteration in which to allocate them. The post-loop failure check in
// coloringRegAlloc() used to be guarded by !reserveSpillReg, so it skipped the
// report, returned VISA_SUCCESS, and left G4_RegVars with a null physical
// register. Post-RA passes then dereferenced it: SWSB's
// G4_BB_SB::getFootprintForOperand() and the local scheduler's
// DDD::getBucketsForOperand() both do
//   phyReg = base->asRegVar()->getPhyReg(); switch (phyReg->getKind())
// with no null check, so the compiler segfaulted.
//
// With the failure correctly reported, RA returns VISA_SPILL, Optimizer
// bails out before those passes, and IGC's retry logic recompiles the kernel
// successfully. So the expected result is a clean successful build; before the
// fix this invocation died with SIGSEGV.
//
// Test setup:
// - 8 simultaneously live predicates is the point at which flag/GRF pressure
//   first forces the allocator to spill on this target; 7 does not trigger it.
// - -cl-opt-disable keeps the predicates from being optimized away.
// - intel_reqd_sub_group_size(32) pins SIMD32 so pressure is not reduced by
//   selecting a narrower dispatch.

// CHECK: Build succeeded

__attribute__((intel_reqd_sub_group_size(32)))
kernel void k(global const int *in, global int *out, global const uint *off) {
  int gid = get_global_id(0);
  bool p0 = in[gid + off[0]] > 0;
  bool p1 = in[gid + off[1]] > 1;
  bool p2 = in[gid + off[2]] > 2;
  bool p3 = in[gid + off[3]] > 3;
  bool p4 = in[gid + off[4]] > 4;
  bool p5 = in[gid + off[5]] > 5;
  bool p6 = in[gid + off[6]] > 6;
  bool p7 = in[gid + off[7]] > 7;
  int acc = 0;
  if (p0) acc += 1; else acc -= 1;
  if (p1) acc += 2; else acc -= 2;
  if (p2) acc += 3; else acc -= 3;
  if (p3) acc += 4; else acc -= 4;
  if (p4) acc += 5; else acc -= 5;
  if (p5) acc += 6; else acc -= 6;
  if (p6) acc += 7; else acc -= 7;
  if (p7) acc += 8; else acc -= 8;
  out[gid] = acc;
}
