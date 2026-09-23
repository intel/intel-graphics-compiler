/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

This software and the related documents are Intel copyrighted materials,
and your use of them is governed by the express license under which they were
provided to you ("License"). Unless the License provides otherwise,
you may not use, modify, copy, publish, distribute, disclose or transmit this
software or the related documents without Intel's prior written permission.

This software and the related documents are provided as is, with no express or
implied warranties, other than those that are expressly stated in the License.


============================= end_copyright_notice ===========================*/

// Checks the VRT mode table SIMD16 drop. Every run keeps the static XE3
// threshold out of reach, so only the table-driven decision can fire.
// UNSUPPORTED: system-windows, release
// REQUIRES: regkeys, cri-supported

// Budget scaled down to 1% of the Xe3P top budget (512 GRFs) -> 5 GRFs, which
// SIMD32 is comfortably past, so SIMD16 is compiled instead.
// RUN: ocloc compile -file %s -device cri \
// RUN:   -options "-igc_opts 'ShaderDumpEnable=1, DumpToCustomDir=%t_drop, AllowSIMD16DropForXE2Plus=1, EarlySIMD16DropForXE3Threshold=10000, VRTSimd16DropBudgetPercent=1'"
// RUN: ls -al %t_drop | FileCheck %s --check-prefix=DROP

// Default budget: 170% of 512 GRFs is far above this kernel, so SIMD32 stays.
// RUN: ocloc compile -file %s -device cri \
// RUN:   -options "-igc_opts 'ShaderDumpEnable=1, DumpToCustomDir=%t_keep, AllowSIMD16DropForXE2Plus=1, EarlySIMD16DropForXE3Threshold=10000'"
// RUN: ls -al %t_keep | FileCheck %s --check-prefix=NO-DROP

// The GRF count is requested directly, so the VRT table does not describe what
// the kernel will get and the table-driven drop stays out of the decision.
// RUN: ocloc compile -file %s -device cri -options "-cl-intel-256-GRF-per-thread \
// RUN:   -igc_opts 'ShaderDumpEnable=1, DumpToCustomDir=%t_grf, AllowSIMD16DropForXE2Plus=1, EarlySIMD16DropForXE3Threshold=10000, VRTSimd16DropBudgetPercent=1'"
// RUN: ls -al %t_grf | FileCheck %s --check-prefix=NO-DROP

// Same for the switch being off.
// RUN: ocloc compile -file %s -device cri \
// RUN:   -options "-igc_opts 'ShaderDumpEnable=1, DumpToCustomDir=%t_off, AllowSIMD16DropForXE2Plus=1, EarlySIMD16DropForXE3Threshold=10000, EnableVRTSimd16Drop=0, VRTSimd16DropBudgetPercent=1'"
// RUN: ls -al %t_off | FileCheck %s --check-prefix=NO-DROP

// DROP: OCL_asm{{.*}}_simd16_entry_0001.asm
// DROP-NOT: OCL_asm{{.*}}_simd32_entry_0001.asm

// NO-DROP-NOT: OCL_asm{{.*}}_simd16_entry_0001.asm
// NO-DROP: OCL_asm{{.*}}_simd32_entry_0001.asm

#define def(N) float4 float_var_##N = {1 + tid, 2 + tid, 3 + tid, 4 + tid};
#define incf4(N) float_var_##N += (float4){4, 3, 2, 1};
#define wrt(N) result[N] = float_var_##N;

__kernel void foo(float4 __global *result) {
  int tid = get_global_id(0);

  def(1);
  def(2);
  def(3);
  def(4);
  def(5);
  def(6);
  def(7);
  def(8);
  def(11);
  def(12);
  def(13);
  def(14);
  def(15);
  def(16);
  def(17);
  def(18);
  def(21);
  def(22);
  def(23);
  def(24);
  def(25);
  def(26);
  def(27);
  def(28);

#pragma nounroll
  for (int i = 0; i < 1000; i++) {
    incf4(1);
    incf4(2);
    incf4(3);
    incf4(4);
    incf4(5);
    incf4(6);
    incf4(7);
    incf4(8);
    incf4(11);
    incf4(12);
    incf4(13);
    incf4(14);
    incf4(15);
    incf4(16);
    incf4(17);
    incf4(18);
    incf4(21);
    incf4(22);
    incf4(23);
    incf4(24);
    incf4(25);
    incf4(26);
    incf4(27);
    incf4(28);
  }

  wrt(1);
  wrt(2);
  wrt(3);
  wrt(4);
  wrt(5);
  wrt(6);
  wrt(7);
  wrt(8);
  wrt(11);
  wrt(12);
  wrt(13);
  wrt(14);
  wrt(15);
  wrt(16);
  wrt(17);
  wrt(18);
  wrt(21);
  wrt(22);
  wrt(23);
  wrt(24);
  wrt(25);
  wrt(26);
  wrt(27);
  wrt(28);
}
