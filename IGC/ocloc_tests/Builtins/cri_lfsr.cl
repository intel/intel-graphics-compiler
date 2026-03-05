/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: cri-supported

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1'" | FileCheck %s

uint    __attribute__((overloadable)) intel_lfsr(uint seed, uint polynomial);
ushort2 __attribute__((overloadable)) intel_lfsr(ushort2 seed, ushort2 polynomial);
uchar4  __attribute__((overloadable)) intel_lfsr(uchar4 seed, uchar4 polynomial);

kernel void test_lfsr(global uint    *in_uint,    global uint    *out_uint,
                      global ushort2 *in_ushort2, global ushort2 *out_ushort2,
                      global uchar4  *in_uchar4,  global uchar4  *out_uchar4)
{
    // lfsr with int argument type
    // CHECK-DAG: lfsr.b32 (M1_NM, 1) [[DstInt:[A-z0-9_]*]](0,0)<1> [[SrcInt:[A-z0-9_]*]](0,0)<0;1,0> [[SrcInt]](0,1)<0;1,0>
    // CHECK-DAG: .decl [[DstInt]] v_type=G type=ud num_elts=1
    // CHECK-DAG: .decl [[SrcInt]] v_type=G type=ud num_elts=2
    out_uint[0] = intel_lfsr(in_uint[0], in_uint[1]);

    // lfsr with short2 argument type
    // CHECK-DAG: lfsr.b16v2 (M1_NM, 1) [[DstShort:[A-z0-9_]*]](0,0)<1> [[Src0Short:[A-z0-9_]*]](0,0)<0;1,0> [[Src0Short]](0,1)<0;1,0>
    // CHECK-DAG: .decl [[DstShort]] v_type=G type=ud num_elts=1
    // CHECK-DAG: .decl [[Src0Short]] v_type=G type=ud num_elts=2
    out_ushort2[0] = intel_lfsr(in_ushort2[0], in_ushort2[1]);

    // lfsr with char4 argument type and non-uniform input
    // CHECK-DAG: lfsr.b8v4 (M1, 32) [[DstChar:[A-z0-9_]*]](0,0)<1> [[Src0Char:[A-z0-9_]*]](0,0)<1;1,0> [[Src1Char:[A-z0-9_]*]](0,0)<1;1,0>
    // CHECK-DAG: .decl [[DstChar]] v_type=G type=ud num_elts=32
    // CHECK-DAG: .decl [[Src0Char]] v_type=G type=ud num_elts=32
    // CHECK-DAG: .decl [[Src1Char]] v_type=G type=ud num_elts=32
    const int tid = get_global_id(0);
    const int tsize = get_global_size(0);
    out_uchar4[tid] = intel_lfsr(in_uchar4[tid], in_uchar4[tsize + tid]);
}
