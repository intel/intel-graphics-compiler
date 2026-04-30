;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; RUN: igc_opt -S -dce -platformbmg -igc-emit-visa --regkey=DumpVISAASMToConsole=1 -simd-mode 16 < %s | FileCheck %s

; ============================================================
; Memory layout shared by all passing tests
; ============================================================
;
; <8 x float> vectorized at SIMD16 = 128 floats = 8 GRF registers.
; One GRF holds 16 floats (XE2: 64 bytes / 4 bytes per float = 16 elements).
; Vector element k occupies GRF row k; its 16 slots hold values for 16 SIMD lanes.
;
;  vec index:   [ 0  ]  [  1  ]  [  2  ]  ...  [  7  ]
;  VISA row:   |(0-15) |(0-15) |( 0-15) | ... |(0-15)|
;               16 floats per row  (SIMD lanes 0..15)
;  current iteration of joint wave broadcast allows to extract 8 consecutive ascending subvalues out of 16
;  foo_elem0 --> checks that, pattern is probably degenerative but we support it for free

; CHECK-LABEL: .kernel "foo"
; CHECK: .decl tmp7 v_type=G type=f num_elts=128 align=wordx32
; CHECK:    dpas.?.?.0.0 (M1, 16) tmp7.0 %null.0 Broadcast_0v.0 Broadcast_1v(0,0)
; CHECK:    mov (M1, 16) tmp8(0,0)<1> tmp7(2,0)<1;1,0>
; CHECK:    mul (M1, 16) vectorized_intrinsic(0,0)<1> tmp8(0,0)<0;1,0> tmp7(0,0)<1;1,0>
; CHECK:    mul (M1, 16) vectorized_intrinsic(1,0)<1> tmp8(0,1)<0;1,0> tmp7(1,0)<1;1,0>
; CHECK:    mul (M1, 16) vectorized_intrinsic(2,0)<1> tmp8(0,2)<0;1,0> tmp7(2,0)<1;1,0>
; CHECK:    mul (M1, 16) vectorized_intrinsic(3,0)<1> tmp8(0,3)<0;1,0> tmp7(3,0)<1;1,0>
; CHECK:    mul (M1, 16) vectorized_intrinsic(4,0)<1> tmp8(0,4)<0;1,0> tmp7(4,0)<1;1,0>
; CHECK:    mul (M1, 16) vectorized_intrinsic(5,0)<1> tmp8(0,5)<0;1,0> tmp7(5,0)<1;1,0>
; CHECK:    mul (M1, 16) vectorized_intrinsic(6,0)<1> tmp8(0,6)<0;1,0> tmp7(6,0)<1;1,0>
; CHECK:    mul (M1, 16) vectorized_intrinsic(7,0)<1> tmp8(0,7)<0;1,0> tmp7(7,0)<1;1,0>

; CHECK-LABEL: .kernel "foo_long"
; CHECK: .decl tmp7 v_type=G type=f num_elts=128 align=wordx32
; CHECK: .decl vector v_type=G type=f num_elts=256 align=wordx32
; CHECK: dpas.?.?.0.0 (M1, 16) tmp7.0 %null.0 Broadcast_0v.0 Broadcast_1v(0,0)
; CHECK: mov (M1_NM, 8) V0034(0,0)<1> 0x0:f
; CHECK: mov (M1_NM, 8) V0034(0,8)<1> 0x0:f
; CHECK: mov (M1, 16) vector(0,0)<1> V0034(0,0)<0;1,0>
; CHECK: mov (M1, 16) vector(1,0)<1> V0034(0,1)<0;1,0>
; CHECK: mov (M1, 16) vector(2,0)<1> V0034(0,2)<0;1,0>
; CHECK: mov (M1, 16) vector(3,0)<1> V0034(0,3)<0;1,0>
; CHECK: mov (M1, 16) vector(4,0)<1> V0034(0,4)<0;1,0>
; CHECK: mov (M1, 16) vector(5,0)<1> V0034(0,5)<0;1,0>
; CHECK: mov (M1, 16) vector(6,0)<1> V0034(0,6)<0;1,0>
; CHECK: mov (M1, 16) vector(7,0)<1> V0034(0,7)<0;1,0>
; CHECK: mov (M1, 16) vector(8,0)<1> V0034(0,8)<0;1,0>
; CHECK: mov (M1, 16) vector(9,0)<1> V0034(0,9)<0;1,0>
; CHECK: mov (M1, 16) vector(10,0)<1> V0034(0,10)<0;1,0>
; CHECK: mov (M1, 16) vector(11,0)<1> V0034(0,11)<0;1,0>
; CHECK: mov (M1, 16) vector(12,0)<1> V0034(0,12)<0;1,0>
; CHECK: mov (M1, 16) vector(13,0)<1> V0034(0,13)<0;1,0>
; CHECK: mov (M1, 16) vector(14,0)<1> V0034(0,14)<0;1,0>
; CHECK: mov (M1, 16) vector(15,0)<1> V0034(0,15)<0;1,0>
; CHECK: mov (M1, 16) vector(0,0)<1> tmp7(2,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(0,0)<1> tmp7(2,0)<0;1,0> vector(0,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(1,0)<1> tmp7(2,1)<0;1,0> vector(1,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(2,0)<1> tmp7(2,2)<0;1,0> vector(2,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(3,0)<1> tmp7(2,3)<0;1,0> vector(3,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(4,0)<1> tmp7(2,4)<0;1,0> vector(4,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(5,0)<1> tmp7(2,5)<0;1,0> vector(5,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(6,0)<1> tmp7(2,6)<0;1,0> vector(6,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(7,0)<1> tmp7(2,7)<0;1,0> vector(7,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(8,0)<1> tmp7(2,8)<0;1,0> vector(8,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(9,0)<1> tmp7(2,9)<0;1,0> vector(9,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(10,0)<1> tmp7(2,10)<0;1,0> vector(10,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(11,0)<1> tmp7(2,11)<0;1,0> vector(11,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(12,0)<1> tmp7(2,12)<0;1,0> vector(12,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(13,0)<1> tmp7(2,13)<0;1,0> vector(13,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(14,0)<1> tmp7(2,14)<0;1,0> vector(14,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(15,0)<1> tmp7(2,15)<0;1,0> vector(15,0)<1;1,0>

; CHECK-LABEL: .kernel "foo_ugly"
; CHECK: .decl tmp7 v_type=G type=f num_elts=128 align=wordx32
; CHECK: .decl vector v_type=G type=f num_elts=208 align=wordx32
; CHECK: mul (M1, 16) vectorized_intrinsic(0,0)<1> tmp7(2,0)<0;1,0> vector(0,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(1,0)<1> tmp7(2,1)<0;1,0> vector(1,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(2,0)<1> tmp7(2,2)<0;1,0> vector(2,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(3,0)<1> tmp7(2,3)<0;1,0> vector(3,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(4,0)<1> tmp7(2,4)<0;1,0> vector(4,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(5,0)<1> tmp7(2,5)<0;1,0> vector(5,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(6,0)<1> tmp7(2,6)<0;1,0> vector(6,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(7,0)<1> tmp7(2,7)<0;1,0> vector(7,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(8,0)<1> tmp7(2,8)<0;1,0> vector(8,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(9,0)<1> tmp7(2,9)<0;1,0> vector(9,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(10,0)<1> tmp7(2,10)<0;1,0> vector(10,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(11,0)<1> tmp7(2,11)<0;1,0> vector(11,0)<1;1,0>
; CHECK: mul (M1, 16) vectorized_intrinsic(12,0)<1> tmp7(2,12)<0;1,0> vector(12,0)<1;1,0>

; CHECK-LABEL: .kernel "foo_elem0"
; CHECK: .decl tmp_e0 v_type=G type=f num_elts=128 align=wordx32
; CHECK:    mul (M1, 16) mul_e0(0,0)<1> tmp_e0_s(0,2)<0;1,0> tmp_e0(0,0)<1;1,0>
; CHECK:    mul (M1, 16) mul_e0(1,0)<1> tmp_e0_s(0,3)<0;1,0> tmp_e0(1,0)<1;1,0>
; CHECK:    mul (M1, 16) mul_e0(2,0)<1> tmp_e0_s(0,4)<0;1,0> tmp_e0(2,0)<1;1,0>
; CHECK:    mul (M1, 16) mul_e0(3,0)<1> tmp_e0_s(0,5)<0;1,0> tmp_e0(3,0)<1;1,0>
; CHECK:    mul (M1, 16) mul_e0(4,0)<1> tmp_e0_s(0,6)<0;1,0> tmp_e0(4,0)<1;1,0>
; CHECK:    mul (M1, 16) mul_e0(5,0)<1> tmp_e0_s(0,7)<0;1,0> tmp_e0(5,0)<1;1,0>
; CHECK:    mul (M1, 16) mul_e0(6,0)<1> tmp_e0_s(0,8)<0;1,0> tmp_e0(6,0)<1;1,0>
; CHECK:    mul (M1, 16) mul_e0(7,0)<1> tmp_e0_s(0,9)<0;1,0> tmp_e0(7,0)<1;1,0>

; CHECK-LABEL: .kernel "foo_elem7"
; CHECK: .decl tmp_e7 v_type=G type=f num_elts=128 align=wordx32
; CHECK:    mul (M1, 16) mul_e7(0,0)<1> tmp_e7_s(0,0)<0;1,0> tmp_e7(0,0)<1;1,0>
; CHECK:    mul (M1, 16) mul_e7(1,0)<1> tmp_e7_s(0,1)<0;1,0> tmp_e7(1,0)<1;1,0>
; CHECK:    mul (M1, 16) mul_e7(2,0)<1> tmp_e7_s(0,2)<0;1,0> tmp_e7(2,0)<1;1,0>
; CHECK:    mul (M1, 16) mul_e7(3,0)<1> tmp_e7_s(0,3)<0;1,0> tmp_e7(3,0)<1;1,0>
; CHECK:    mul (M1, 16) mul_e7(4,0)<1> tmp_e7_s(0,4)<0;1,0> tmp_e7(4,0)<1;1,0>
; CHECK:    mul (M1, 16) mul_e7(5,0)<1> tmp_e7_s(0,5)<0;1,0> tmp_e7(5,0)<1;1,0>
; CHECK:    mul (M1, 16) mul_e7(6,0)<1> tmp_e7_s(0,6)<0;1,0> tmp_e7(6,0)<1;1,0>
; CHECK:    mul (M1, 16) mul_e7(7,0)<1> tmp_e7_s(0,7)<0;1,0> tmp_e7(7,0)<1;1,0>

; CHECK-LABEL: .kernel "foo_fadd"
; CHECK: .decl tmp_fa v_type=G type=f num_elts=128 align=wordx32
; CHECK:    add (M1, 16) add_fa(0,0)<1> tmp_fa_s(0,0)<0;1,0> tmp_fa(0,0)<1;1,0>
; CHECK:    add (M1, 16) add_fa(1,0)<1> tmp_fa_s(0,1)<0;1,0> tmp_fa(1,0)<1;1,0>
; CHECK:    add (M1, 16) add_fa(2,0)<1> tmp_fa_s(0,2)<0;1,0> tmp_fa(2,0)<1;1,0>
; CHECK:    add (M1, 16) add_fa(3,0)<1> tmp_fa_s(0,3)<0;1,0> tmp_fa(3,0)<1;1,0>
; CHECK:    add (M1, 16) add_fa(4,0)<1> tmp_fa_s(0,4)<0;1,0> tmp_fa(4,0)<1;1,0>
; CHECK:    add (M1, 16) add_fa(5,0)<1> tmp_fa_s(0,5)<0;1,0> tmp_fa(5,0)<1;1,0>
; CHECK:    add (M1, 16) add_fa(6,0)<1> tmp_fa_s(0,6)<0;1,0> tmp_fa(6,0)<1;1,0>
; CHECK:    add (M1, 16) add_fa(7,0)<1> tmp_fa_s(0,7)<0;1,0> tmp_fa(7,0)<1;1,0>


define spir_kernel void @foo() {
  br label %._crit_edge

._crit_edge:
  %tmp7 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %tmp8 = extractelement <8 x float> %tmp7, i64 2
  %vectorized_joint_wavebroadcast = call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float %tmp8, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>, i32 0); visa id: 2117
  %vectorized_intrinsic = fmul <8 x float> %vectorized_joint_wavebroadcast, %tmp7
  %vectorized_intrinsic2 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %vectorized_intrinsic, <8 x float> %vectorized_intrinsic)
  %tmp10_el = extractelement <8 x float> %vectorized_intrinsic2, i32 0
  %sink = inttoptr i64 0 to float*
  store float %tmp10_el, float* %sink
  br label %._crit_edge
}

define spir_kernel void @foo_long() {
  br label %._crit_edge

._crit_edge:
  %tmp7 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %tmp8 = extractelement <8 x float> %tmp7, i64 2
  %vector = insertelement <16 x float> zeroinitializer, float %tmp8 , i32 0
  %vectorized_joint_wavebroadcast = call <16 x float> @llvm.genx.GenISA.JointWaveBroadcast.v16f32.f32.v16i32(float %tmp8, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>, i32 0); visa id: 2117
  %vectorized_intrinsic = fmul <16 x float> %vectorized_joint_wavebroadcast, %vector
  %vectorized_intrinsic2 = call <16 x float> @llvm.maxnum.v16f32(<16 x float> %vectorized_intrinsic, <16 x float> %vectorized_intrinsic)
  %tmp10_el = extractelement <16 x float> %vectorized_intrinsic2, i32 0
  %sink = inttoptr i64 0 to float*
  store float %tmp10_el, float* %sink
  br label %._crit_edge
}


define spir_kernel void @foo_ugly() {
  br label %._crit_edge

._crit_edge:
  %tmp7 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %tmp8 = extractelement <8 x float> %tmp7, i64 2
  %vector = insertelement <13 x float> zeroinitializer, float %tmp8 , i32 0
  %vectorized_joint_wavebroadcast = call <13 x float> @llvm.genx.GenISA.JointWaveBroadcast.v13f32.f32.v13i32(float %tmp8, <13 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12>, i32 0); visa id: 2117
  %vectorized_intrinsic = fmul <13 x float> %vectorized_joint_wavebroadcast, %vector
  %vectorized_intrinsic2 = call <13 x float> @llvm.maxnum.v13f32(<13 x float> %vectorized_intrinsic, <13 x float> %vectorized_intrinsic)
  %tmp10_el = extractelement <13 x float> %vectorized_intrinsic2, i32 0
  %sink = inttoptr i64 0 to float*
  store float %tmp10_el, float* %sink
  br label %._crit_edge
}


; Broadcast from first element (row 0) — lower boundary check
define spir_kernel void @foo_elem0() {
  br label %._crit_edge

._crit_edge:
  %tmp_e0 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %tmp_e0_s = extractelement <8 x float> %tmp_e0, i64 0
  %bcast_e0 = call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float %tmp_e0_s, <8 x i32> < i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9>, i32 0)
  %mul_e0 = fmul <8 x float> %bcast_e0, %tmp_e0
  %max_e0 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %mul_e0, <8 x float> %mul_e0)
  %el_e0 = extractelement <8 x float> %max_e0, i32 0
  %sink_e0 = inttoptr i64 0 to float*
  store float %el_e0, float* %sink_e0
  br label %._crit_edge
}

; Broadcast from last element (row 7) — upper boundary check
define spir_kernel void @foo_elem7() {
  br label %._crit_edge

._crit_edge:
  %tmp_e7 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %tmp_e7_s = extractelement <8 x float> %tmp_e7, i64 7
  %bcast_e7 = call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float %tmp_e7_s, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>, i32 0)
  %mul_e7 = fmul <8 x float> %bcast_e7, %tmp_e7
  %max_e7 = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %mul_e7, <8 x float> %mul_e7)
  %el_e7 = extractelement <8 x float> %max_e7, i32 0
  %sink_e7 = inttoptr i64 0 to float*
  store float %el_e7, float* %sink_e7
  br label %._crit_edge
}

; Broadcast element 2 with fadd — same row selection, different operation
define spir_kernel void @foo_fadd() {
  br label %._crit_edge

._crit_edge:
  %tmp_fa = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %tmp_fa_s = extractelement <8 x float> %tmp_fa, i64 2
  %bcast_fa = call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float %tmp_fa_s, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>, i32 0)
  %add_fa = fadd <8 x float> %bcast_fa, %tmp_fa
  %max_fa = call <8 x float> @llvm.maxnum.v8f32(<8 x float> %add_fa, <8 x float> %add_fa)
  %el_fa = extractelement <8 x float> %max_fa, i32 0
  %sink_fa = inttoptr i64 0 to float*
  store float %el_fa, float* %sink_fa
  br label %._crit_edge
}

declare <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float, <8 x i32>, i32)
declare <16 x float> @llvm.genx.GenISA.JointWaveBroadcast.v16f32.f32.v16i32(float, <16 x i32>, i32)
declare <13 x float> @llvm.genx.GenISA.JointWaveBroadcast.v13f32.f32.v13i32(float, <13 x i32>, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare <16 x float> @llvm.genx.GenISA.sub.group.dpas.v16f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare <8 x float> @llvm.maxnum.v8f32(<8 x float>, <8 x float>) #1
declare <16 x float> @llvm.maxnum.v16f32(<16 x float>, <16 x float>) #1
declare <13 x float> @llvm.maxnum.v13f32(<13 x float>, <13 x float>) #1

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }
!igc.functions = !{!0, !4, !6, !8, !10, !12}

!0 = distinct !{void ()* @foo, !1}
!1 = distinct !{!2, !3}
!2 = distinct !{!"function_type", i32 0}
!3 = distinct !{!"sub_group_size", i32 16}
!4 = distinct !{void ()* @foo_elem0, !5}
!5 = distinct !{!2, !3}
!6 = distinct !{void ()* @foo_elem7, !7}
!7 = distinct !{!2, !3}
!8 = distinct !{void ()* @foo_fadd, !9}
!9 = distinct !{!2, !3}
!10 = distinct !{void ()* @foo_long, !11}
!11 = distinct !{!2, !3}
!12 = distinct !{void ()* @foo_ugly, !13}
!13 = distinct !{!2, !3}
