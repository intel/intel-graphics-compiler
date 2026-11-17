/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, dg2-supported
// RUN: ocloc compile -file %s -options "-cl-opt-disable -igc_opts 'VISAOptions=-maxRAIterations 2 -asmToConsole'" -device dg2 | FileCheck %s

// Companion to ra-iteration-limit-graceful-failure.cl, which covers
// -maxRAIterations 1. Both invocations end the GRF RA loop the same way:
// GlobalRA::setupFailSafeIfNeeded() enables fail-safe RA for iteration
// maxRAIterations - 1, that iteration still has to insert spill code, and
// iterationNo reaches maxRAIterations with no iteration left. The two cases
// differ in what that leaves behind.
//
// With -maxRAIterations 1 the fail-safe iteration is iteration 0 and it leaves
// operands whose G4_RegVar has a null physical register, so the post-RA passes
// dereference null. That must be reported as an RA failure.
//
// With -maxRAIterations 2 the fail-safe iteration is iteration 1 and it
// allocates everything: every operand has a physical register and the kernel
// is complete and correct. Fail-safe RA is designed to produce exactly that -
// it assigns the spill/fill temporaries it creates out of the reserved GRFs on
// the spot, which is why it needs no follow-up iteration. Running the
// iteration counter out is not by itself evidence that anything is wrong, so
// this result must be kept rather than thrown away.
//
// If the out-of-iterations report is raised unconditionally here,
// coloringRegAlloc() returns VISA_SPILL, Optimizer::optimization() bails out,
// and CEncoder::Compile() records SIMD_SKIP_SPILL and produces no kernel.
// intel_reqd_sub_group_size(32) rules out SIMD16 and SIMD8, so every attempt
// fails the same way and IGC emits a program containing no kernel at all
// (a zebin with no .text.k, no .symtab and no .ze_info) while ocloc still
// prints "Build succeeded". -asmToConsole makes that visible: a compile that
// emits the kernel dumps its assembly, a compile that discarded it dumps
// nothing.
//
// Test setup is identical to ra-iteration-limit-graceful-failure.cl:
// 8 simultaneously live predicates is the point at which the allocator first
// has to spill on this target, -cl-opt-disable keeps them from being optimized
// away, and intel_reqd_sub_group_size(32) pins SIMD32 so the pressure is not
// relieved by a narrower dispatch.

// CHECK: //.kernel k
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
