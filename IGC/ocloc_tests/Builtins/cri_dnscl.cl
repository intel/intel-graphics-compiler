/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: cri-supported

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1'" | FileCheck %s

uint intel_downscale_as_bf16_i4_mode_0(short2, short2);
uint intel_downscale_as_bf16_e2m1_mode_2_srnd(short2, short2, ushort2);
uint intel_downscale_e2m1_mode_3_srnd(half2, half2, ushort2);

kernel void test_dnscl(global uchar *in, global uint *out)
{
    {
        // test of non-uniform input/output
        const int tid = get_global_id(0);
        const int tsize = get_global_size(0);

        // CHECK-DAG: dnscl.bftoint4.mode0.rne (M1, 32) [[T0_Dst:[A-z0-9]*]].0 [[T0_Src0:[A-z0-9]*]].0 [[T0_Src1:[A-z0-9]*]].0 %null.0
        // CHECK-DAG: .decl [[T0_Dst]] v_type=G type=ud num_elts=32
        // CHECK-DAG: .decl [[T0_Src0]] v_type=G type=ud num_elts=32
        // CHECK-DAG: .decl [[T0_Src1]] v_type=G type=ud num_elts=32
        short2 src0 = ((global short2 *)in)[tid];
        in += sizeof(short2)*tsize;
        short2 src1 = ((global short2 *)in)[tid];
        in += sizeof(short2)*tsize;

        uint dst = intel_downscale_as_bf16_i4_mode_0(src0, src1);
        out[tid] = dst;
        out += tsize;
    }

    {
        // CHECK-DAG: dnscl.bftoe2m1.mode2.srnd (M1_NM, 1) [[T2_Dst:[A-z0-9]*]].0 [[T2_Args:[A-z0-9]*]].0 [[T2_Args]].4 [[T2_Args]].8
        // CHECK-DAG: .decl [[T2_Dst]] v_type=G type=ud num_elts=1
        // CHECK-DAG: .decl [[T2_Args]] v_type=G type=ud num_elts=3
        short2 src0 = *(global short2 *)in;
        in += sizeof(short2);
        short2 src1 = *(global short2 *)in;
        in += sizeof(short2);
        ushort2 bias = *(global ushort2 *)in;
        in += sizeof(ushort2);

        uint dst = intel_downscale_as_bf16_e2m1_mode_2_srnd(src0, src1, bias);
        *out = dst;
        out += 1;
    }

    {
        // CHECK-DAG: dnscl.hftoe2m1.mode3.srnd (M1_NM, 1) [[T3_Dst:[A-z0-9]*]].0 [[T3_Args:[A-z0-9]*]].0 [[T3_Args]].4 [[T3_Args]].8
        // CHECK-DAG: .decl [[T3_Dst]] v_type=G type=ud num_elts=1
        // CHECK-DAG: .decl [[T3_Args]] v_type=G type=ud num_elts=3
        half2 src0 = *(global half2 *)in;
        in += sizeof(half2);
        half2 src1 = *(global half2 *)in;
        in += sizeof(half2);
        ushort2 bias = *(global ushort2 *)in;
        in += sizeof(ushort2);

        uint dst = intel_downscale_e2m1_mode_3_srnd(src0, src1, bias);
        *out = dst;
        out += 1;
    }
}
