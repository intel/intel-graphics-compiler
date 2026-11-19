/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Verify that the spill warning is reported exactly once per spilling kernel,
// for the kernel version that is shipped, under different retry decisions taken
// per kernel in one module:
//   - k_no_retry: required sub-group size + sub-group shuffles (indirect
//     operands) make IGC skip the retry for this kernel even though it spills,
//   - k_retry:    spills and is retried (and still spills),
//   - k_clean:    does not spill.
// The spill size in the warning must match spill_size of the shipped binary.
//
// TotalGRFNum=32 forces an artificially low register file, guaranteeing spills.
// ForceSIMDRPELimit=2000 prevents the compiler from avoiding spills by
// switching to a smaller SIMD width.

// UNSUPPORTED: release
// ForceSIMDRPELimit, ForceRecompilation and the RetryRevert*
// flags used below are unavailable in Linux Release builds.
// REQUIRES: bmg-supported, regkeys

// Default: k_no_retry ships its first try, k_retry ships its retry.
// RUN: ocloc compile -file %s -device bmg \
// RUN:   -options "-igc_opts 'TotalGRFNum=32, ForceSIMDRPELimit=2000, DumpZEInfoToConsole=1'" 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DEFAULT \
// RUN:   --implicit-check-not="spilled around" --implicit-check-not="in kernel 'k_no_retry': [RetryManager]" --implicit-check-not="in kernel 'k_clean'"

// zeinfo is printed before the build log.
// DEFAULT: - name: k_no_retry
// DEFAULT: spill_size: [[#NORETRY:]]
// DEFAULT: - name: k_clean
// DEFAULT-NOT: spill_size
// DEFAULT: - name: k_retry
// DEFAULT: spill_size: [[#RETRY:]]
// The retry message is deduplicated by text, so seeing it for k_retry also
// proves k_no_retry (compiled first) did not request a retry.
// DEFAULT: in kernel 'k_no_retry': compiled SIMD{{[0-9]+}} allocated {{[0-9]+}} regs and spilled around [[#div(NORETRY,64)]]
// DEFAULT: in kernel 'k_retry': [RetryManager] Start recompilation of the kernel
// DEFAULT: in kernel 'k_retry': compiled SIMD{{[0-9]+}} allocated {{[0-9]+}} regs and spilled around [[#div(RETRY,64)]]
// DEFAULT: Build succeeded.

// Retry disabled: every spilling kernel ships its first try.
// RUN: ocloc compile -file %s -device bmg \
// RUN:   -options "-igc_opts 'TotalGRFNum=32, ForceSIMDRPELimit=2000, DisableRecompilation=1, DumpZEInfoToConsole=1'" 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DISABLE \
// RUN:   --implicit-check-not="spilled around" --implicit-check-not="[RetryManager]" --implicit-check-not="in kernel 'k_clean'"

// DISABLE: - name: k_no_retry
// DISABLE: spill_size: [[#NORETRY:]]
// DISABLE: - name: k_clean
// DISABLE-NOT: spill_size
// DISABLE: - name: k_retry
// DISABLE: spill_size: [[#RETRY:]]
// DISABLE: in kernel 'k_no_retry': compiled SIMD{{[0-9]+}} allocated {{[0-9]+}} regs and spilled around [[#div(NORETRY,64)]]
// DISABLE: in kernel 'k_retry': compiled SIMD{{[0-9]+}} allocated {{[0-9]+}} regs and spilled around [[#div(RETRY,64)]]
// DISABLE: Build succeeded.

// Forced retry: every kernel is retried, so first-try spills must not be
// reported; each spilling kernel is reported once, for its retry.
// RUN: ocloc compile -file %s -device bmg \
// RUN:   -options "-igc_opts 'TotalGRFNum=32, ForceSIMDRPELimit=2000, ForceRecompilation=1, DumpZEInfoToConsole=1'" 2>&1 \
// RUN:   | FileCheck %s --check-prefix=FORCE \
// RUN:   --implicit-check-not="spilled around" --implicit-check-not="in kernel 'k_clean'"

// FORCE: - name: k_no_retry
// FORCE: spill_size: [[#NORETRY:]]
// FORCE: - name: k_clean
// FORCE-NOT: spill_size
// FORCE: - name: k_retry
// FORCE: spill_size: [[#RETRY:]]
// FORCE: [RetryManager] Start recompilation of the kernel
// FORCE: in kernel 'k_no_retry': compiled SIMD{{[0-9]+}} allocated {{[0-9]+}} regs and spilled around [[#div(NORETRY,64)]]
// FORCE: in kernel 'k_retry': compiled SIMD{{[0-9]+}} allocated {{[0-9]+}} regs and spilled around [[#div(RETRY,64)]]
// FORCE: Build succeeded.

// First version chosen: the retry of k_retry is rejected as worse, so the
// first try is shipped and its spill (not the retry's) must be reported.
// RUN: ocloc compile -file %s -device bmg \
// RUN:   -options "-igc_opts 'TotalGRFNum=32, ForceSIMDRPELimit=2000, RetryRevertExcessiveSpillingKernelThreshold=1, RetryRevertExcessiveSpillingKernelCoefficient=10, DumpZEInfoToConsole=1'" 2>&1 \
// RUN:   | FileCheck %s --check-prefix=PICKPREV \
// RUN:   --implicit-check-not="spilled around" --implicit-check-not="in kernel 'k_clean'"

// PICKPREV: - name: k_no_retry
// PICKPREV: spill_size: [[#NORETRY:]]
// PICKPREV: - name: k_clean
// PICKPREV-NOT: spill_size
// PICKPREV: - name: k_retry
// PICKPREV: spill_size: [[#RETRY:]]
// PICKPREV: in kernel 'k_no_retry': compiled SIMD{{[0-9]+}} allocated {{[0-9]+}} regs and spilled around [[#div(NORETRY,64)]]
// PICKPREV: in kernel 'k_retry': [RetryManager] Start recompilation of the kernel
// PICKPREV: in kernel 'k_retry': [RetryManager] Used previous version of the kernel, reason : NO_Retry_Pick_Prv
// PICKPREV: in kernel 'k_retry': compiled SIMD{{[0-9]+}} allocated {{[0-9]+}} regs and spilled around [[#div(RETRY,64)]]
// PICKPREV: Build succeeded.

#define def(N)                                                                                                         \
  float16 v##N = {1 + i, 2 + i,  3 + i,  4 + i,  5 + i,  6 + i,  7 + i,  8 + i,                                        \
                  9 + i, 10 + i, 11 + i, 12 + i, 13 + i, 14 + i, 15 + i, 16 + i};
#define shf(N) v##N += intel_sub_group_shuffle(v##N, (uint)(j + N) & 15);
#define inc(N) v##N += (float16){16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
#define wrt(N) result[N] = v##N;

__attribute__((intel_reqd_sub_group_size(16))) kernel void k_no_retry(float16 global *result) {
  int i = get_global_id(0);

  def(0);
  def(1);
  def(2);
  def(3);
  def(4);
  def(5);
  def(6);
  def(7);

#pragma nounroll
  for (int j = 0; j < 1500; j++) {
    shf(0);
    shf(1);
    shf(2);
    shf(3);
    shf(4);
    shf(5);
    shf(6);
    shf(7);
  }

  wrt(0);
  wrt(1);
  wrt(2);
  wrt(3);
  wrt(4);
  wrt(5);
  wrt(6);
  wrt(7);
}

kernel void k_clean(int global *result) { result[get_global_id(0)] = 1; }

kernel void k_retry(float16 global *result) {
  int i = get_global_id(0);

  def(0);
  def(1);
  def(2);
  def(3);
  def(4);
  def(5);

#pragma nounroll
  for (int j = 0; j < 1500; j++) {
    inc(0);
    inc(1);
    inc(2);
    inc(3);
    inc(4);
    inc(5);
  }

  wrt(0);
  wrt(1);
  wrt(2);
  wrt(3);
  wrt(4);
  wrt(5);
}
