/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// UNSUPPORTED: sys32
// REQUIRES: dg2-supported
// RUN: not ocloc compile -file %s -device dg2 2>&1 | FileCheck %s

// The exec size of the inline asm is wider than the operands the user gave it,
// so the LSC message would read and write registers past the end of the vISA
// declares IGC created for them. vISA used to abort the compiler here.

// CHECK: error: parsing vISA inline assembly failed:
// CHECK: address register dimensions don't fit data type
// CHECK: addr size is 64b x 32 elem(s)
// CHECK: data register dimensions don't fit data type

__attribute__((intel_reqd_sub_group_size(8))) kernel void test(global uint *in, global uint *out) {
  uint value;
  __asm__ volatile("lsc_load.ugm.uc.ca (M1, 32) %0:d32 flat[%1]:a64" : "=rw"(value) : "rw"(in));
  out[0] = value;
}
