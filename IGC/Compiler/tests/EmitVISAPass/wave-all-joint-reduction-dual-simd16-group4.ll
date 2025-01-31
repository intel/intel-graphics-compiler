;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -platformdg2 -igc-emit-visa %s -inputcs -simd-mode 32 -regkey DumpVISAASMToConsole | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
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
  %shl_runtime_value_1 = shl i32 %runtime_value_1, 2
  %shuffle_0 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %shl_runtime_value_1, i32 0, i32 0)
  %shl_lane32 = shl i32 %lane32, 2
  %add_0 = add i32 %shuffle_0, %shl_lane32
  %a = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2490368v4f32(<4 x float> addrspace(2490368)* %src, i32 %add_0, i32 4, i1 false)
  %shuffle_1 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %shl_runtime_value_1, i32 1, i32 0)
  %add_1 = add i32 %shuffle_1, %shl_lane32
  %b = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2490368v4f32(<4 x float> addrspace(2490368)* %src, i32 %add_1, i32 4, i1 false)
  %shuffle_2 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %shl_runtime_value_1, i32 2, i32 0)
  %add_2 = add i32 %shuffle_2, %shl_lane32
  %c = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2490368v4f32(<4 x float> addrspace(2490368)* %src, i32 %add_2, i32 4, i1 false)
  %shuffle_3 = call i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32 %shl_runtime_value_1, i32 3, i32 0)
  %add_3 = add i32 %shuffle_3, %shl_lane32
  %d = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2490368v4f32(<4 x float> addrspace(2490368)* %src, i32 %add_3, i32 4, i1 false)
  %waveAllSrc0 = insertelement <4 x i32> undef, i32 %a, i64 0
  %waveAllSrc1 = insertelement <4 x i32> %waveAllSrc0, i32 %b, i64 1
  %waveAllSrc2 = insertelement <4 x i32> %waveAllSrc1, i32 %c, i64 2
  %waveAllSrc3 = insertelement <4 x i32> %waveAllSrc2, i32 %d, i64 3
; move operands to consecutive GRF space (generated from insertelement instructions)
; CHECK: mov (M1, 16) waveAllSrc0(0,0)<1> a(0,0)<1;1,0>
; CHECK: mov (M1, 16) waveAllSrc0(2,0)<1> b(0,0)<1;1,0>
; CHECK: mov (M1, 16) waveAllSrc0(4,0)<1> c(0,0)<1;1,0>
; CHECK: mov (M1, 16) waveAllSrc0(6,0)<1> d(0,0)<1;1,0>
; move operands (secondHalf) to consecutive GRF space (one-time use space for first reduction layer)
; CHECK: mov (M5, 16) waveAllSrc0_0(0,0)<1> a_0(0,0)<1;1,0>
; CHECK: mov (M5, 16) waveAllSrc0_0(2,0)<1> b_0(0,0)<1;1,0>
; CHECK: mov (M5, 16) waveAllSrc0_0(4,0)<1> c_0(0,0)<1;1,0>
; CHECK: mov (M5, 16) waveAllSrc0_0(6,0)<1> d_0(0,0)<1;1,0>

; Identity operations + layer 0 (simd-16 reduction of a single variable across 32 lanes)
; CHECK: mov (M1_NM, 16) reduceSrc_waveAllSrc0(0,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 16) reduceSrc_waveAllSrc0(0,0)<1> waveAllSrc0(0,0)<1;1,0>
; CHECK-NEXT: mov (M5_NM, 16) reduceSrcSecondHalf_waveAllSrc0(0,0)<1> 0x0:d
; CHECK-NEXT: mov (M5, 16) reduceSrcSecondHalf_waveAllSrc0(0,0)<1> waveAllSrc0_0(0,0)<1;1,0>
; CHECK-NEXT: add (M1_NM, 16) reduceSrc_waveAllSrc0(0,0)<1> reduceSrc_waveAllSrc0(0,0)<1;1,0> reduceSrcSecondHalf_waveAllSrc0(0,0)<1;1,0>
; CHECK: mov (M1_NM, 16) reduceSrc_waveAllSrc0(2,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 16) reduceSrc_waveAllSrc0(2,0)<1> waveAllSrc0(2,0)<1;1,0>
; CHECK-NEXT: mov (M5_NM, 16) reduceSrcSecondHalf_waveAllSrc0(2,0)<1> 0x0:d
; CHECK-NEXT: mov (M5, 16) reduceSrcSecondHalf_waveAllSrc0(2,0)<1> waveAllSrc0_0(2,0)<1;1,0>
; CHECK-NEXT: add (M1_NM, 16) reduceSrc_waveAllSrc0(2,0)<1> reduceSrc_waveAllSrc0(2,0)<1;1,0> reduceSrcSecondHalf_waveAllSrc0(2,0)<1;1,0>
; CHECK: mov (M1_NM, 16) reduceSrc_waveAllSrc0(4,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 16) reduceSrc_waveAllSrc0(4,0)<1> waveAllSrc0(4,0)<1;1,0>
; CHECK-NEXT: mov (M5_NM, 16) reduceSrcSecondHalf_waveAllSrc0(4,0)<1> 0x0:d
; CHECK-NEXT: mov (M5, 16) reduceSrcSecondHalf_waveAllSrc0(4,0)<1> waveAllSrc0_0(4,0)<1;1,0>
; CHECK-NEXT: add (M1_NM, 16) reduceSrc_waveAllSrc0(4,0)<1> reduceSrc_waveAllSrc0(4,0)<1;1,0> reduceSrcSecondHalf_waveAllSrc0(4,0)<1;1,0>
; CHECK: mov (M1_NM, 16) reduceSrc_waveAllSrc0(6,0)<1> 0x0:d
; CHECK-NEXT: mov (M1, 16) reduceSrc_waveAllSrc0(6,0)<1> waveAllSrc0(6,0)<1;1,0>
; CHECK-NEXT: mov (M5_NM, 16) reduceSrcSecondHalf_waveAllSrc0(6,0)<1> 0x0:d
; CHECK-NEXT: mov (M5, 16) reduceSrcSecondHalf_waveAllSrc0(6,0)<1> waveAllSrc0_0(6,0)<1;1,0>
; CHECK-NEXT: add (M1_NM, 16) reduceSrc_waveAllSrc0(6,0)<1> reduceSrc_waveAllSrc0(6,0)<1;1,0> reduceSrcSecondHalf_waveAllSrc0(6,0)<1;1,0>

; Joint Reduction Tree
; layer 1
; CHECK: add (M1_NM, 16) reduceSrc_waveAllSrc0(0,0)<1> reduceSrc_waveAllSrc0(0,0)<16;8,1> reduceSrc_waveAllSrc0(1,0)<16;8,1>
; CHECK: add (M1_NM, 16) reduceSrc_waveAllSrc0(2,0)<1> reduceSrc_waveAllSrc0(4,0)<16;8,1> reduceSrc_waveAllSrc0(5,0)<16;8,1>
; layer 2
; CHECK: add (M1_NM, 16) reduceSrc_waveAllSrc0(0,0)<1> reduceSrc_waveAllSrc0(0,0)<8;4,1> reduceSrc_waveAllSrc0(0,4)<8;4,1>
; layer 3
; CHECK: add (M1_NM, 8) reduceSrc_waveAllSrc0(0,0)<1> reduceSrc_waveAllSrc0(0,0)<4;2,1> reduceSrc_waveAllSrc0(0,2)<4;2,1>
; layer 4
; CHECK: add (M1_NM, 4) waveAllJoint(0,0)<1> reduceSrc_waveAllSrc0(0,0)<2;1,1> reduceSrc_waveAllSrc0(0,1)<2;1,1>
  %waveAllJoint = call <4 x i32> @llvm.genx.GenISA.WaveAll.v4i32.i8.i32(<4 x i32> %waveAllSrc3, i8 0, i32 0)
  %res_a = extractelement <4 x i32> %waveAllJoint, i32 0
  %res_b = extractelement <4 x i32> %waveAllJoint, i32 1
  %res_c = extractelement <4 x i32> %waveAllJoint, i32 2
  %res_d = extractelement <4 x i32> %waveAllJoint, i32 3
; Proper replacement in subsequent instructions
; CHECK: add (M1_NM, 1) join_c_d(0,0)<1> waveAllJoint(0,2)<0;1,0> waveAllJoint(0,3)<0;1,0>
; CHECK: add3 (M1_NM, 1) join_a_b_c_d(0,0)<1> waveAllJoint(0,0)<0;1,0> waveAllJoint(0,1)<0;1,0> join_c_d(0,0)<0;1,0>
  %join_a_b = add i32 %res_a, %res_b
  %join_c_d = add i32 %res_c, %res_d
  %join_a_b_c_d = add i32 %join_a_b, %join_c_d
  %store = insertelement <1 x i32> undef, i32 %join_a_b_c_d, i64 0
  call void @llvm.genx.GenISA.storerawvector.indexed.p2490377v4f32.v1i32(<4 x float> addrspace(2490369)* %dst, i32 0, <1 x i32> %store, i32 4, i1 false)
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId() #1

declare i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2490368v4f32(<4 x float> addrspace(2490368)*, i32, i32, i1) #2

declare <4 x i32> @llvm.genx.GenISA.WaveAll.v4i32.i8.i32(<4 x i32>, i8, i32) #3

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