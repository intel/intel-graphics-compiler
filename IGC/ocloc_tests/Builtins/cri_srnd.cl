/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// REQUIRES: cri-supported

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'DumpVISAASMToConsole=1'" | FileCheck %s

// f32->f16 vec1
half  __attribute__((overloadable)) intel_convert_half_srnd(float source, ushort  random);
// f16->bf8 vec2
uchar2 __attribute__((overloadable)) intel_convert_bfloat82_as_uchar2_srnd(half2 source, uchar2 random);
// f16->hf8 vec3
uchar3 __attribute__((overloadable)) intel_convert_hfloat83_as_uchar3_srnd(half3 source, uchar3 random);
// bf16->bf8 vec4
uchar4 __attribute__((overloadable)) intel_convert_as_bfloat164_bfloat84_as_uchar4_srnd(ushort4 source, uchar4 random);
// bf16->hf8 vec8 & vec16
uchar8 __attribute__((overloadable)) intel_convert_as_bfloat168_hfloat88_as_uchar8_srnd(ushort8 source, uchar8 random);
uchar16 __attribute__((overloadable)) intel_convert_as_bfloat1616_hfloat816_as_uchar16_srnd(ushort16 source, uchar16 random);

kernel void test(global int *in, global int *out)
{
    // CHECK-DAG: srnd (M1_NM, 1) [[T0_DST:[A-z0-9_]*]](0,0)<1> [[T0_SRC0:[A-z0-9_]*]](0,0)<1;1,0> [[T0_SRC1:[A-z0-9_]*]](0,0)<1;1,0>
    int adv = 1;
    *(global half *)out = intel_convert_half_srnd(*(global float *)in, *(global ushort *)(in + adv));
    in += 2*adv;
    out += adv;

    // CHECK-DAG: srnd (M1_NM, 2) [[T1_DST:[A-z0-9_]*]](0,0)<1> [[T1_SRC0:[A-z0-9_]*]](0,0)<1;1,0> [[T1_SRC1:[A-z0-9_]*]](0,0)<1;1,0>
    adv = 1;
    *(global uchar2 *)out = intel_convert_bfloat82_as_uchar2_srnd(*(global half2 *)in, *(global uchar2 *)(in + adv));
    in += 2*adv;
    out += adv;

    // CHECK-DAG: srnd (M1_NM, 2) [[T2_DST:[A-z0-9_]*]](0,0)<1> [[T2_SRC0:[A-z0-9_]*]](0,0)<1;1,0> [[T2_SRC1:[A-z0-9_]*]](0,0)<1;1,0>
    // CHECK-DAG: srnd (M1_NM, 1) [[T2_DST]](0,2)<1> [[T2_SRC0]](0,2)<1;1,0> [[T2_SRC1]](0,2)<1;1,0>
    adv = 2;
    *(global uchar3 *)out = intel_convert_hfloat83_as_uchar3_srnd(*(global half3 *)in, *(global uchar3 *)(in + adv));
    in += 2*adv;
    out += adv;

    // CHECK-DAG: srnd (M1_NM, 4) [[T3_DST:[A-z0-9_]*]](0,0)<1> [[T3_SRC0:[A-z0-9_]*]](0,0)<1;1,0> [[T3_SRC1:[A-z0-9_]*]](0,0)<1;1,0>
    adv = 2;
    *(global uchar4 *)out = intel_convert_as_bfloat164_bfloat84_as_uchar4_srnd(*(global ushort4 *)in, *(global uchar4 *)(in + adv));
    in += 2*adv;
    out += adv;

    // CHECK-DAG: srnd (M1_NM, 8) [[T4_DST:[A-z0-9_]*]](0,0)<1> [[T4_SRC0:[A-z0-9_]*]](0,0)<1;1,0> [[T4_SRC1:[A-z0-9_]*]](0,0)<1;1,0>
    adv = 4;
    *(global uchar8 *)out = intel_convert_as_bfloat168_hfloat88_as_uchar8_srnd(*(global ushort8 *)in, *(global uchar8 *)(in + adv));
    in += 2*adv;
    out += adv;

    // CHECK-DAG: srnd (M1_NM, 16) [[T5_DST:[A-z0-9_]*]](0,0)<1> [[T5_SRC0:[A-z0-9_]*]](0,0)<1;1,0> [[T5_SRC1:[A-z0-9_]*]](0,0)<1;1,0>
    adv = 8;
    *(global uchar16 *)out = intel_convert_as_bfloat1616_hfloat816_as_uchar16_srnd(*(global ushort16 *)in, *(global uchar16 *)(in + adv));
    in += 2*adv;
    out += adv;
}

// CHECK-DAG: .decl [[T0_DST]] v_type=G type=hf num_elts=1
// CHECK-DAG: .decl [[T0_SRC0]] v_type=G type=f num_elts=1
// CHECK-DAG: .decl [[T0_SRC1]] v_type=G type=uw num_elts=1

// CHECK-DAG: .decl [[T1_DST]] v_type=G type=ub num_elts=2
// CHECK-DAG: .decl [[T1_SRC0]] v_type=G type=hf num_elts=2
// CHECK-DAG: .decl [[T1_SRC1]] v_type=G type=ub num_elts=2

// CHECK-DAG: .decl [[T2_DST]] v_type=G type=b num_elts=3
// CHECK-DAG: .decl [[T2_SRC0]] v_type=G type=hf num_elts=3
// CHECK-DAG: .decl [[T2_SRC1]] v_type=G type=ub num_elts=3

// CHECK-DAG: .decl [[T3_DST]] v_type=G type=ub num_elts=4
// CHECK-DAG: .decl [[T3_SRC0]] v_type=G type=bf num_elts=4
// CHECK-DAG: .decl [[T3_SRC1]] v_type=G type=ub num_elts=4

// CHECK-DAG: .decl [[T4_DST]] v_type=G type=b num_elts=8
// CHECK-DAG: .decl [[T4_SRC0]] v_type=G type=bf num_elts=8
// CHECK-DAG: .decl [[T4_SRC1]] v_type=G type=ub num_elts=8

// CHECK-DAG: .decl [[T5_DST]] v_type=G type=b num_elts=16
// CHECK-DAG: .decl [[T5_SRC0]] v_type=G type=bf num_elts=16
// CHECK-DAG: .decl [[T5_SRC1]] v_type=G type=ub num_elts=16
