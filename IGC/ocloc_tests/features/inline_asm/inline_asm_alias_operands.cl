/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, pvc-supported
// RUN: ocloc compile -file %s -options "-igc_opts 'DumpVISAASMToConsole=1'" -device pvc | FileCheck %s

// This is to test the handling of alias operands used in inline asm. For each alias operands,
// a vISA decl with non-zero alias offset shall be generated.

// CHECK-LABEL: .kernel
// CHECK: .decl [[ROOTVAR:.*]] v_type=G type=w num_elts=128 align=wordx32
// CHECK: .decl [[V0:.*]] v_type=G type=w num_elts=96 align=wordx32 alias=<[[ROOTVAR]], 64>
// CHECK: .decl [[V1:.*]] v_type=G type=w num_elts=64 align=wordx32 alias=<[[ROOTVAR]], 128>
// CHECK: .decl [[V2:.*]] v_type=G type=w num_elts=32 align=wordx32 alias=<[[ROOTVAR]], 192>
//
// CHECK-LABEL: .function
//
// 1st inline asm
//
// CHECK-LABEL: lsc_load_block2d.ugm
//
// 2nd inline asm
//
// CHECK: mov (M1_NM, 16) [[ROOTVAR]](0,0)<2> [[T0:.*]](0,0)<4;1,0>
// CHECK: mov (M1_NM, 16) [[ROOTVAR]](0,1)<2> [[T0]](0,1)<4;1,0>
// CHECK: mov (M1_NM, 16) [[V0]](0,0)<2>      [[T0]](0,2)<4;1,0>
// CHECK: mov (M1_NM, 16) [[V0]](0,1)<2>      [[T0]](0,3)<4;1,0>
//
// 3rd inline asm
//
// CHECK: mov (M1_NM, 16) [[V1]](0,0)<2> [[T1:.*]](0,0)<4;1,0>
// CHECK: mov (M1_NM, 16) [[V1]](0,1)<2> [[T1]](0,1)<4;1,0>
// CHECK: mov (M1_NM, 16) [[V2]](0,0)<2> [[T1]](0,2)<4;1,0>
// CHECK: mov (M1_NM, 16) [[V2]](0,1)<2> [[T1]](0,3)<4;1,0>
//
// CHECK-LABEL: ret (M1, 1)

__attribute__((convergent)) void __spirv_Subgroup2DBlockStoreINTEL(
  int ElementSize, int BlockWidth, int BlockHeight, int BlockCount,
  void *src_pointer, global const void *dst_base_pointer, int memory_width,
  int memory_height, int memory_pitch, int2 coordinate);

__attribute__((intel_reqd_sub_group_size(16)))
kernel void test(global uchar *a, global ushort *b)
{
    uchar x[16];
    ushort y[16];

    __asm__ (
        "lsc_load_block2d.ugm (M1, 1) %0:d8.16x16nt flat[%1,15,15,15,0,0]"
        : "=rw"(*(uchar16 *)(&x[0]))
        : "rw.u"(a)
    );

    __asm__ (
      "mov (M1_NM, 16) %0(0,0)<2> %2(0,0)<4;1,0>\n"
      "mov (M1_NM, 16) %0(0,1)<2> %2(0,1)<4;1,0>\n"
      "mov (M1_NM, 16) %1(0,0)<2> %2(0,2)<4;1,0>\n"
      "mov (M1_NM, 16) %1(0,1)<2> %2(0,3)<4;1,0>\n"
      : "=rw"(*(ushort2 *)(&y[0])), "=rw"(*(ushort2 *)(&y[2]))
      : "rw"(*(uchar4 *)(&x[0]))
    );

    __asm__ (
      "mov (M1_NM, 16) %0(0,0)<2> %2(0,0)<4;1,0>\n"
      "mov (M1_NM, 16) %0(0,1)<2> %2(0,1)<4;1,0>\n"
      "mov (M1_NM, 16) %1(0,0)<2> %2(0,2)<4;1,0>\n"
      "mov (M1_NM, 16) %1(0,1)<2> %2(0,3)<4;1,0>\n"
      : "=rw"(*(ushort2 *)(&y[4])), "=rw"(*(ushort2 *)(&y[6]))
      : "rw"(*(uchar4 *)(&x[4]))
    );

    int2 xy = 0;
    __spirv_Subgroup2DBlockStoreINTEL(2, 16, 8, 1, &y[0], b, 16, 8, 16, xy);
}
