/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


// REQUIRES: bmg-supported
// RUN: ocloc compile -device bmg -options "-igc_opts 'DumpVISAASMToConsole=1'" -file %s > %t
// RUN: FileCheck %s < %t

// Make sure we set and reset the rounding mode when there is unused conversion intrinsic.
// This is a regression test for the following bug:
// after switching to newer LLVM version, LLVM stopped optimizing out unused rounding intrinsics,
// while VISA ignored them, resulting in not setting or unsetting rounding mode.

__kernel void test( __global uint *src, __global float *dest )
{
  size_t i = get_global_id(0);

  uint2 in;
  float2 out;

  in.x = src[i];
  if (i % 2 == 0)
    in.y = src[i+1];

  out = convert_float2_rtn(in);
  dest[i] = out.x;

  if (i % 2 == 0)
    dest[i+1] = out.y;
}

// The xors set/unset the rtn rounding mode.

// CHECK-LABEL: _main_0:

// converting in.x
// CHECK:      _{{[A-Za-z0-9]+}}:
// CHECK:      xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud
// CHECK-NEXT: mov
// CHECK-NEXT: xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud

// With the bug the previous and next xors were missing, keeping the non-default rounding mode for instructions in between.

// converting both in.x and in.y
// CHECK:      _{{[A-Za-z0-9]+}}:
// CHECK:      xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud
// CHECK-NEXT: mov
// CHECK-NEXT: mov
// CHECK-NEXT: xor (M1_NM, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x20:ud
