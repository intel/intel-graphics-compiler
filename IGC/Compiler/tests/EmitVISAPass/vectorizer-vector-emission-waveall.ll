;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; RUN: igc_opt -S -dce -platformbmg -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 < %s | FileCheck %s

; CHECK: .decl vector331 v_type=G type=f num_elts=128
; CHECK: .decl vectorized_cast v_type=G type=hf num_elts=128

; CHECK: exp (M1, 16) vectorized_intrinsic(0,0)<1> vector331(0,0)<1;1,0>
; CHECK: exp (M1, 16) vectorized_intrinsic(1,0)<1> vector331(1,0)<1;1,0>
; CHECK: exp (M1, 16) vectorized_intrinsic(2,0)<1> vector331(2,0)<1;1,0>
; CHECK: exp (M1, 16) vectorized_intrinsic(3,0)<1> vector331(3,0)<1;1,0>
; CHECK: exp (M1, 16) vectorized_intrinsic(4,0)<1> vector331(4,0)<1;1,0>
; CHECK: exp (M1, 16) vectorized_intrinsic(5,0)<1> vector331(5,0)<1;1,0>
; CHECK: exp (M1, 16) vectorized_intrinsic(6,0)<1> vector331(6,0)<1;1,0>
; CHECK: exp (M1, 16) vectorized_intrinsic(7,0)<1> vector331(7,0)<1;1,0>

; CHECK: max (M1_NM, 8) vectorized_joint_waveall(0,0)<1> reduceSrc_vectorized_intrinsic(0,0)<2;1,1> reduceSrc_vectorized_intrinsic(0,1)<2;1,1>
; CHECK: max (M1, 16) vectorized_intrinsic2(0,0)<1> vectorized_joint_waveall(0,0)<0;1,0> vectorized_intrinsic(0,0)<1;1,0>
; CHECK: max (M1, 16) vectorized_intrinsic2(1,0)<1> vectorized_joint_waveall(0,1)<0;1,0> vectorized_intrinsic(1,0)<1;1,0>
; CHECK: max (M1, 16) vectorized_intrinsic2(2,0)<1> vectorized_joint_waveall(0,2)<0;1,0> vectorized_intrinsic(2,0)<1;1,0>
; CHECK: max (M1, 16) vectorized_intrinsic2(3,0)<1> vectorized_joint_waveall(0,3)<0;1,0> vectorized_intrinsic(3,0)<1;1,0>
; CHECK: max (M1, 16) vectorized_intrinsic2(4,0)<1> vectorized_joint_waveall(0,4)<0;1,0> vectorized_intrinsic(4,0)<1;1,0>
; CHECK: max (M1, 16) vectorized_intrinsic2(5,0)<1> vectorized_joint_waveall(0,5)<0;1,0> vectorized_intrinsic(5,0)<1;1,0>
; CHECK: max (M1, 16) vectorized_intrinsic2(6,0)<1> vectorized_joint_waveall(0,6)<0;1,0> vectorized_intrinsic(6,0)<1;1,0>
; CHECK: max (M1, 16) vectorized_intrinsic2(7,0)<1> vectorized_joint_waveall(0,7)<0;1,0> vectorized_intrinsic(7,0)<1;1,0>


define spir_kernel void @foo() {
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge, %6
  %tmp7 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %tmp8 = extractelement <8 x float> %tmp7, i64 0
  %vector331 = insertelement <8 x float> zeroinitializer, float %tmp8, i32 0
  %vectorized_intrinsic = call <8 x float> @llvm.exp2.v8f32(<8 x float> %vector331)
  %vectorized_joint_waveall = call <8 x float> @llvm.genx.GenISA.WaveAll.v8f32(<8 x float> %vectorized_intrinsic, i8 12, i1 true, i32 0)
  %vectorized_intrinsic2 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %vectorized_joint_waveall, <8 x float> %vectorized_intrinsic)
  %vectorized_cast = fptrunc <8 x float> %vectorized_intrinsic2 to <8 x half>
  %tmp9 = bitcast <8 x half> %vectorized_cast to <8 x i16>
  %tmp10 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %tmp9, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  br label %._crit_edge
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

declare <8 x float> @llvm.genx.GenISA.WaveAll.v8f32(<8 x float>, i8, i1, i32) #1

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare <8 x float> @llvm.exp2.v8f32(<8 x float>) #1

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare <8 x float> @llvm.maxnum.v8f32(<8 x float>, <8 x float>) #1

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }
!igc.functions = !{!0}

!0 = distinct !{void ()* @foo, !1}
!1 = distinct !{!2, !3}
!2 = distinct !{!"function_type", i32 0}
!3 = distinct !{!"sub_group_size", i32 16}


