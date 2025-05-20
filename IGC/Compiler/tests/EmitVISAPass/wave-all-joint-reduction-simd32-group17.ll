;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -platformbmg -igc-emit-visa %s -inputcs -simd-mode 32 -regkey DumpVISAASMToConsole | FileCheck %s
; ------------------------------------------------
; EmitVISAPass: Compare group of 17 WaveAll reductions participating in a joint reduction tree to a single WaveAll reduction
;               Joint reduction emits 75 instructions in total after EmitVISAPass
;               - includes 17 potentially unnecessary mov instructions to get the inputs into the GRF aligned space
;               - includes 2 potentially unnecessary mov instructions to move the reduction tree results to the destination
;               Compared to 7 (non-joint WaveAll reduction) * 17 = 119 for 17 consecutive non-joint WaveAll instructions if they were not merged
; ------------------------------------------------
target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-v64:32:32-v128:32:32-a0:0:32-n8:16:32-S32"
target triple = "dxil-ms-dx"

@ThreadGroupSize_X = constant i32 1
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 32

; Function Attrs: null_pointer_is_valid
define void @CSMain(i32 %runtime_value_0, i32 %runtime_value_1, i32 %runtime_value_2) #0 {
  %src = inttoptr i32 %runtime_value_0 to <4 x float> addrspace(2490368)*
  %dst = inttoptr i32 %runtime_value_2 to <4 x float> addrspace(2490369)*
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane32 = zext i16 %lane to i32
  %shuffle_0 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %runtime_value_1, i32 0, i32 0)
  %add_0 = add i32 %shuffle_0, %lane32
  %shl_0 = shl i32 %add_0, 2
  %a = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2490368v4f32(<4 x float> addrspace(2490368)* %src, i32 %shl_0, i32 4, i1 false)
  %shuffle_1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %runtime_value_1, i32 1, i32 0)
  %add_1 = add i32 %shuffle_1, %lane32
  %shl_1 = shl i32 %add_1, 2
  %b = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2490368v4f32(<4 x float> addrspace(2490368)* %src, i32 %shl_1, i32 4, i1 false)
  %shuffle_2 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %runtime_value_1, i32 2, i32 0)
  %add_2 = add i32 %shuffle_2, %lane32
  %shl_2 = shl i32 %add_2, 2
  %c = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2490368v4f32(<4 x float> addrspace(2490368)* %src, i32 %shl_2, i32 4, i1 false)
  %shuffle_3 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %runtime_value_1, i32 0, i32 0)
  %add_3 = add i32 %shuffle_3, %lane32
  %shl_3 = shl i32 %add_3, 2
  %d = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2490368v4f32(<4 x float> addrspace(2490368)* %src, i32 %shl_3, i32 4, i1 false)
  %shuffle_4 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %runtime_value_1, i32 0, i32 0)
  %add_4 = add i32 %shuffle_4, %lane32
  %shl_4 = shl i32 %add_4, 2
  %e = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2490368v4f32(<4 x float> addrspace(2490368)* %src, i32 %shl_4, i32 4, i1 false)
  %shuffle_5 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %runtime_value_1, i32 1, i32 0)
  %add_5 = add i32 %shuffle_5, %lane32
  %shl_5 = shl i32 %add_5, 2
  %f = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2490368v4f32(<4 x float> addrspace(2490368)* %src, i32 %shl_5, i32 4, i1 false)
  %waveAllSrc0 = insertelement <17 x i32> undef, i32 %add_0, i64 0
  %waveAllSrc1 = insertelement <17 x i32> %waveAllSrc0, i32 %shl_0, i64 1
  %waveAllSrc2 = insertelement <17 x i32> %waveAllSrc1, i32 %a, i64 2
  %waveAllSrc3 = insertelement <17 x i32> %waveAllSrc2, i32 %add_1, i64 3
  %waveAllSrc4 = insertelement <17 x i32> %waveAllSrc3, i32 %shl_1, i64 4
  %waveAllSrc5 = insertelement <17 x i32> %waveAllSrc4, i32 %b, i64 5
  %waveAllSrc6 = insertelement <17 x i32> %waveAllSrc5, i32 %add_2, i64 6
  %waveAllSrc7 = insertelement <17 x i32> %waveAllSrc6, i32 %shl_2, i64 7
  %waveAllSrc8 = insertelement <17 x i32> %waveAllSrc7, i32 %c, i64 8
  %waveAllSrc9 = insertelement <17 x i32> %waveAllSrc8, i32 %add_3, i64 9
  %waveAllSrc10 = insertelement <17 x i32> %waveAllSrc9, i32 %shl_3, i64 10
  %waveAllSrc11 = insertelement <17 x i32> %waveAllSrc10, i32 %d, i64 11
  %waveAllSrc12 = insertelement <17 x i32> %waveAllSrc11, i32 %add_4, i64 12
  %waveAllSrc13 = insertelement <17 x i32> %waveAllSrc12, i32 %shl_4, i64 13
  %waveAllSrc14 = insertelement <17 x i32> %waveAllSrc13, i32 %e, i64 14
  %waveAllSrc15 = insertelement <17 x i32> %waveAllSrc14, i32 %add_5, i64 15
  %waveAllSrc16 = insertelement <17 x i32> %waveAllSrc15, i32 %shl_5, i64 16
; move operands to consecutive GRF space (generated from insertelement instructions, will likely be optimized away in the end)
; CHECK: mov (M1, 32) waveAllSrc0(0,0)<1> add_0(0,0)<1;1,0>
; CHECK: mov (M1, 32) waveAllSrc0(2,0)<1> shl_0(0,0)<1;1,0>
; CHECK: mov (M1, 32) waveAllSrc0(4,0)<1> a(0,0)<1;1,0>
; CHECK: mov (M1, 32) waveAllSrc0(6,0)<1> add_1(0,0)<1;1,0>
; CHECK: mov (M1, 32) waveAllSrc0(8,0)<1> shl_1(0,0)<1;1,0>
; CHECK: mov (M1, 32) waveAllSrc0(10,0)<1> b(0,0)<1;1,0>
; CHECK: mov (M1, 32) waveAllSrc0(12,0)<1> add_2(0,0)<1;1,0>
; CHECK: mov (M1, 32) waveAllSrc0(14,0)<1> shl_2(0,0)<1;1,0>
; CHECK: mov (M1, 32) waveAllSrc0(16,0)<1> c(0,0)<1;1,0>
; CHECK: mov (M1, 32) waveAllSrc0(18,0)<1> add_3(0,0)<1;1,0>
; CHECK: mov (M1, 32) waveAllSrc0(20,0)<1> shl_3(0,0)<1;1,0>
; CHECK: mov (M1, 32) waveAllSrc0(22,0)<1> d(0,0)<1;1,0>
; CHECK: mov (M1, 32) waveAllSrc0(24,0)<1> add_4(0,0)<1;1,0>
; CHECK: mov (M1, 32) waveAllSrc0(26,0)<1> shl_4(0,0)<1;1,0>
; CHECK: mov (M1, 32) waveAllSrc0(28,0)<1> e(0,0)<1;1,0>
; CHECK: mov (M1, 32) waveAllSrc0(30,0)<1> add_5(0,0)<1;1,0>
; CHECK: mov (M1, 32) waveAllSrc0(32,0)<1> shl_5(0,0)<1;1,0>

; Identity operations
; CHECK: mov (M1_NM, 32) reduceSrc_waveAllSrc0(0,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 32) reduceSrc_waveAllSrc0(0,0)<1> waveAllSrc0(0,0)<1;1,0>
; CHECK: mov (M1_NM, 32) reduceSrc_waveAllSrc0(2,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 32) reduceSrc_waveAllSrc0(2,0)<1> waveAllSrc0(2,0)<1;1,0>
; CHECK: mov (M1_NM, 32) reduceSrc_waveAllSrc0(4,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 32) reduceSrc_waveAllSrc0(4,0)<1> waveAllSrc0(4,0)<1;1,0>
; CHECK: mov (M1_NM, 32) reduceSrc_waveAllSrc0(6,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 32) reduceSrc_waveAllSrc0(6,0)<1> waveAllSrc0(6,0)<1;1,0>
; CHECK: mov (M1_NM, 32) reduceSrc_waveAllSrc0(8,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 32) reduceSrc_waveAllSrc0(8,0)<1> waveAllSrc0(8,0)<1;1,0>
; CHECK: mov (M1_NM, 32) reduceSrc_waveAllSrc0(10,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 32) reduceSrc_waveAllSrc0(10,0)<1> waveAllSrc0(10,0)<1;1,0>
; CHECK: mov (M1_NM, 32) reduceSrc_waveAllSrc0(12,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 32) reduceSrc_waveAllSrc0(12,0)<1> waveAllSrc0(12,0)<1;1,0>
; CHECK: mov (M1_NM, 32) reduceSrc_waveAllSrc0(14,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 32) reduceSrc_waveAllSrc0(14,0)<1> waveAllSrc0(14,0)<1;1,0>
; CHECK: mov (M1_NM, 32) reduceSrc_waveAllSrc0(16,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 32) reduceSrc_waveAllSrc0(16,0)<1> waveAllSrc0(16,0)<1;1,0>
; CHECK: mov (M1_NM, 32) reduceSrc_waveAllSrc0(18,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 32) reduceSrc_waveAllSrc0(18,0)<1> waveAllSrc0(18,0)<1;1,0>
; CHECK: mov (M1_NM, 32) reduceSrc_waveAllSrc0(20,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 32) reduceSrc_waveAllSrc0(20,0)<1> waveAllSrc0(20,0)<1;1,0>
; CHECK: mov (M1_NM, 32) reduceSrc_waveAllSrc0(22,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 32) reduceSrc_waveAllSrc0(22,0)<1> waveAllSrc0(22,0)<1;1,0>
; CHECK: mov (M1_NM, 32) reduceSrc_waveAllSrc0(24,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 32) reduceSrc_waveAllSrc0(24,0)<1> waveAllSrc0(24,0)<1;1,0>
; CHECK: mov (M1_NM, 32) reduceSrc_waveAllSrc0(26,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 32) reduceSrc_waveAllSrc0(26,0)<1> waveAllSrc0(26,0)<1;1,0>
; CHECK: mov (M1_NM, 32) reduceSrc_waveAllSrc0(28,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 32) reduceSrc_waveAllSrc0(28,0)<1> waveAllSrc0(28,0)<1;1,0>
; CHECK: mov (M1_NM, 32) reduceSrc_waveAllSrc0(30,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 32) reduceSrc_waveAllSrc0(30,0)<1> waveAllSrc0(30,0)<1;1,0>
; CHECK: mov (M1_NM, 32) reduceSrc_waveAllSrc0(32,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 32) reduceSrc_waveAllSrc0(32,0)<1> waveAllSrc0(32,0)<1;1,0>
; Joint Reduction Tree (16-wide)
; layer 1
; CHECK: add (M1_NM, 32) reduceSrc_waveAllSrc0(0,0)<1> reduceSrc_waveAllSrc0(0,0)<32;16,1> reduceSrc_waveAllSrc0(1,0)<32;16,1>
; CHECK: add (M1_NM, 32) reduceSrc_waveAllSrc0(2,0)<1> reduceSrc_waveAllSrc0(4,0)<32;16,1> reduceSrc_waveAllSrc0(5,0)<32;16,1>
; CHECK: add (M1_NM, 32) reduceSrc_waveAllSrc0(4,0)<1> reduceSrc_waveAllSrc0(8,0)<32;16,1> reduceSrc_waveAllSrc0(9,0)<32;16,1>
; CHECK: add (M1_NM, 32) reduceSrc_waveAllSrc0(6,0)<1> reduceSrc_waveAllSrc0(12,0)<32;16,1> reduceSrc_waveAllSrc0(13,0)<32;16,1>
; CHECK: add (M1_NM, 32) reduceSrc_waveAllSrc0(8,0)<1> reduceSrc_waveAllSrc0(16,0)<32;16,1> reduceSrc_waveAllSrc0(17,0)<32;16,1>
; CHECK: add (M1_NM, 32) reduceSrc_waveAllSrc0(10,0)<1> reduceSrc_waveAllSrc0(20,0)<32;16,1> reduceSrc_waveAllSrc0(21,0)<32;16,1>
; CHECK: add (M1_NM, 32) reduceSrc_waveAllSrc0(12,0)<1> reduceSrc_waveAllSrc0(24,0)<32;16,1> reduceSrc_waveAllSrc0(25,0)<32;16,1>
; CHECK: add (M1_NM, 32) reduceSrc_waveAllSrc0(14,0)<1> reduceSrc_waveAllSrc0(28,0)<32;16,1> reduceSrc_waveAllSrc0(29,0)<32;16,1>
; layer 2
; CHECK: add (M1_NM, 32) reduceSrc_waveAllSrc0(0,0)<1> reduceSrc_waveAllSrc0(0,0)<16;8,1> reduceSrc_waveAllSrc0(0,8)<16;8,1>
; CHECK: add (M1_NM, 32) reduceSrc_waveAllSrc0(2,0)<1> reduceSrc_waveAllSrc0(4,0)<16;8,1> reduceSrc_waveAllSrc0(4,8)<16;8,1>
; CHECK: add (M1_NM, 32) reduceSrc_waveAllSrc0(4,0)<1> reduceSrc_waveAllSrc0(8,0)<16;8,1> reduceSrc_waveAllSrc0(8,8)<16;8,1>
; CHECK: add (M1_NM, 32) reduceSrc_waveAllSrc0(6,0)<1> reduceSrc_waveAllSrc0(12,0)<16;8,1> reduceSrc_waveAllSrc0(12,8)<16;8,1>
; layer 3
; CHECK: add (M1_NM, 32) reduceSrc_waveAllSrc0(0,0)<1> reduceSrc_waveAllSrc0(0,0)<8;4,1> reduceSrc_waveAllSrc0(0,4)<8;4,1>
; CHECK: add (M1_NM, 32) reduceSrc_waveAllSrc0(2,0)<1> reduceSrc_waveAllSrc0(4,0)<8;4,1> reduceSrc_waveAllSrc0(4,4)<8;4,1>
; layer 4
; CHECK: add (M1_NM, 32) reduceSrc_waveAllSrc0(0,0)<1> reduceSrc_waveAllSrc0(0,0)<4;2,1> reduceSrc_waveAllSrc0(0,2)<4;2,1>
; layer 5
; CHECK: add (M1_NM, 16) waveAllJoint(0,0)<1> reduceSrc_waveAllSrc0(0,0)<2;1,1> reduceSrc_waveAllSrc0(0,1)<2;1,1>
; Joint Reduction Tree (1-wide, leftover from splitting the 17-wide vector into 16 and 1, almost identical to existing non-joint reduction tree generated from scalar WaveAll intrinsic further below)
; CHECK: add (M1_NM, 16) reduceSrc_waveAllSrc0(32,0)<1> reduceSrc_waveAllSrc0(32,0)<32;16,1> reduceSrc_waveAllSrc0(33,0)<32;16,1>
; CHECK: add (M1_NM, 8) reduceSrc_waveAllSrc0(32,0)<1> reduceSrc_waveAllSrc0(32,0)<16;8,1> reduceSrc_waveAllSrc0(32,8)<16;8,1>
; CHECK: add (M1_NM, 4) reduceSrc_waveAllSrc0(32,0)<1> reduceSrc_waveAllSrc0(32,0)<8;4,1> reduceSrc_waveAllSrc0(32,4)<8;4,1>
; CHECK: add (M1_NM, 2) reduceSrc_waveAllSrc0(32,0)<1> reduceSrc_waveAllSrc0(32,0)<4;2,1> reduceSrc_waveAllSrc0(32,2)<4;2,1>
; CHECK: add (M1_NM, 1) waveAllJoint(1,0)<1> reduceSrc_waveAllSrc0(32,0)<2;1,1> reduceSrc_waveAllSrc0(32,1)<2;1,1>
  %waveAllJoint = call <17 x i32> @llvm.genx.GenISA.WaveAll.v17i32.i8.i32(<17 x i32> %waveAllSrc16, i8 0, i32 0)
  %res_f = call i32 @llvm.genx.GenISA.WaveAll.i32.i8.i32(i32 %f, i8 0, i32 0)
  %res_add_0 = extractelement <17 x i32> %waveAllJoint, i32 0
  %res_shl_0 = extractelement <17 x i32> %waveAllJoint, i32 1
  %res_a = extractelement <17 x i32> %waveAllJoint, i32 2
  %res_add_1 = extractelement <17 x i32> %waveAllJoint, i32 3
  %res_shl_1 = extractelement <17 x i32> %waveAllJoint, i32 4
  %res_b = extractelement <17 x i32> %waveAllJoint, i32 5
  %res_add_2 = extractelement <17 x i32> %waveAllJoint, i32 6
  %res_shl_2 = extractelement <17 x i32> %waveAllJoint, i32 7
  %res_c = extractelement <17 x i32> %waveAllJoint, i32 8
  %res_add_3 = extractelement <17 x i32> %waveAllJoint, i32 9
  %res_shl_3 = extractelement <17 x i32> %waveAllJoint, i32 10
  %res_d = extractelement <17 x i32> %waveAllJoint, i32 11
  %res_add_4 = extractelement <17 x i32> %waveAllJoint, i32 12
  %res_shl_4 = extractelement <17 x i32> %waveAllJoint, i32 13
  %res_e = extractelement <17 x i32> %waveAllJoint, i32 14
  %res_add_5 = extractelement <17 x i32> %waveAllJoint, i32 15
  %res_shl_5 = extractelement <17 x i32> %waveAllJoint, i32 16
; Proper replacement in subsequent instructions
; CHECK: add (M1_NM, 1) join_a_0(0,0)<1> waveAllJoint(0,0)<0;1,0> waveAllJoint(0,1)<0;1,0>
  %join_a_0 = add i32 %res_add_0, %res_shl_0
  %join_a_1 = add i32 %join_a_0, %res_a
; CHECK: add3 (M1_NM, 1) join_b_1(0,0)<1> waveAllJoint(0,3)<0;1,0> waveAllJoint(0,4)<0;1,0> waveAllJoint(0,5)<0;1,0>
  %join_b_0 = add i32 %res_add_1, %res_shl_1
  %join_b_1 = add i32 %join_b_0, %res_b
; CHECK: add (M1_NM, 1) join_c_0(0,0)<1> waveAllJoint(0,6)<0;1,0> waveAllJoint(0,7)<0;1,0>
  %join_c_0 = add i32 %res_add_2, %res_shl_2
  %join_c_1 = add i32 %join_c_0, %res_c
; CHECK: add3 (M1_NM, 1) join_d_1(0,0)<1> waveAllJoint(0,9)<0;1,0> waveAllJoint(0,10)<0;1,0> waveAllJoint(0,11)<0;1,0>
  %join_d_0 = add i32 %res_add_3, %res_shl_3
  %join_d_1 = add i32 %join_d_0, %res_d
; CHECK: add (M1_NM, 1) join_e_0(0,0)<1> waveAllJoint(0,12)<0;1,0> waveAllJoint(0,13)<0;1,0>
  %join_e_0 = add i32 %res_add_4, %res_shl_4
  %join_e_1 = add i32 %join_e_0, %res_e
; CHECK: add3 (M1_NM, 1) join_f_1(0,0)<1> waveAllJoint(0,15)<0;1,0> waveAllJoint(1,0)<0;1,0> res_f(0,0)<0;1,0>
  %join_f_0 = add i32 %res_add_5, %res_shl_5
  %join_f_1 = add i32 %join_f_0, %res_f
; CHECK: add3 (M1_NM, 1) join_ab(0,0)<1> join_a_0(0,0)<0;1,0> waveAllJoint(0,2)<0;1,0> join_b_1(0,0)<0;1,0>
  %join_ab = add i32 %join_a_1, %join_b_1
; CHECK: add3 (M1_NM, 1) join_cd(0,0)<1> join_c_0(0,0)<0;1,0> waveAllJoint(0,8)<0;1,0> join_d_1(0,0)<0;1,0>
  %join_cd = add i32 %join_c_1, %join_d_1
; CHECK: add3 (M1_NM, 1) join_ef(0,0)<1> join_e_0(0,0)<0;1,0> waveAllJoint(0,14)<0;1,0> join_f_1(0,0)<0;1,0>
  %join_ef = add i32 %join_e_1, %join_f_1
  %join_ab_cd = add i32 %join_ab, %join_cd
; CHECK: add3 (M1_NM, 1) join_ab_cd_ef(0,0)<1> join_ab(0,0)<0;1,0> join_cd(0,0)<0;1,0> join_ef(0,0)<0;1,0>
  %join_ab_cd_ef = add i32 %join_ab_cd, %join_ef
  %store = insertelement <1 x i32> undef, i32 %join_ab_cd_ef, i64 0
  call void @llvm.genx.GenISA.storerawvector.indexed.p2490377v4f32.v1i32(<4 x float> addrspace(2490369)* %dst, i32 0, <1 x i32> %store, i32 4, i1 false)
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId() #1

declare i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2490368v4f32(<4 x float> addrspace(2490368)*, i32, i32, i1) #2

declare <17 x i32> @llvm.genx.GenISA.WaveAll.v17i32.i8.i32(<17 x i32>, i8, i32) #3
declare <1 x i32> @llvm.genx.GenISA.WaveAll.v1i32.i8.i32(<1 x i32>, i8, i32) #3
declare i32 @llvm.genx.GenISA.WaveAll.i32.i8.i32(i32, i8, i32) #3

declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32) #4

declare void @llvm.genx.GenISA.storerawvector.indexed.p2490377v4f32.v1i32(<4 x float> addrspace(2490369)*, i32, <1 x i32>, i32, i1) #5

attributes #0 = { null_pointer_is_valid }
attributes #1 = { nounwind readnone }
attributes #2 = { argmemonly nounwind readonly }
attributes #3 = { convergent inaccessiblememonly nounwind }
attributes #4 = { convergent nounwind readnone }
attributes #5 = { argmemonly nounwind writeonly }

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{void (i32, i32, i32)* @CSMain, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", void (i32, i32, i32)* @CSMain}
!6 = !{!"FuncMDValue[0]"}