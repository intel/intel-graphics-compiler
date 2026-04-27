;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer --regkey=VectorizerAllowWAVEBROADCAST=1 --dce < %s 2>&1 | FileCheck %s

; Verify that two independent WaveBroadcast groups using the same lane indices
; (0-7) but different source values are each vectorized independently into
; their own JointWaveBroadcast call.  This covers the case where multiple
; distinct broadcast patterns coexist in the same function body.
;
; CHECK-NOT: llvm.genx.GenISA.WaveBroadcast.f32

; CHECK: [[SRC_A:%.*]] = call float @llvm.exp2.f32(float 0.000000e+00)
; CHECK: call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float [[SRC_A]], <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>, i32 0)
; CHECK: [[SRC_B:%.*]] = call float @llvm.exp2.f32(float 1.000000e+00)
; CHECK: call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float [[SRC_B]], <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>, i32 0)
; CHECK-NOT: llvm.genx.GenISA.WaveBroadcast.f32

; only lingering declare allowed
; CHECK: declare float @llvm.genx.GenISA.WaveBroadcast.f32(float, i32, i32)

source_filename = "reduced.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_two_srcs() {

bb:
  br label %loop

loop:                                             ; preds = %loop, %bb
  %acc_a = phi <8 x float> [ zeroinitializer, %bb ], [ %res_b, %loop ]
  %acc_b = phi <8 x float> [ zeroinitializer, %bb ], [ %res_a, %loop ]

  ; ---- Group A: source = exp2(0.0), lanes 0-7 ----
  %src_a = call float @llvm.exp2.f32(float 0.000000e+00)
  %a0 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src_a, i32 0, i32 0)
  %ea0 = extractelement <8 x float> %acc_a, i64 0
  %ma0 = fmul float %ea0, %a0
  %va0 = insertelement <8 x float> %acc_a, float %ma0, i64 0
  %a1 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src_a, i32 1, i32 0)
  %ea1 = extractelement <8 x float> %acc_a, i64 1
  %ma1 = fmul float %ea1, %a1
  %va1 = insertelement <8 x float> %va0, float %ma1, i64 1
  %a2 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src_a, i32 2, i32 0)
  %ea2 = extractelement <8 x float> %acc_a, i64 2
  %ma2 = fmul float %ea2, %a2
  %va2 = insertelement <8 x float> %va1, float %ma2, i64 2
  %a3 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src_a, i32 3, i32 0)
  %ea3 = extractelement <8 x float> %acc_a, i64 3
  %ma3 = fmul float %ea3, %a3
  %va3 = insertelement <8 x float> %va2, float %ma3, i64 3
  %a4 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src_a, i32 4, i32 0)
  %ea4 = extractelement <8 x float> %acc_a, i64 4
  %ma4 = fmul float %ea4, %a4
  %va4 = insertelement <8 x float> %va3, float %ma4, i64 4
  %a5 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src_a, i32 5, i32 0)
  %ea5 = extractelement <8 x float> %acc_a, i64 5
  %ma5 = fmul float %ea5, %a5
  %va5 = insertelement <8 x float> %va4, float %ma5, i64 5
  %a6 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src_a, i32 6, i32 0)
  %ea6 = extractelement <8 x float> %acc_a, i64 6
  %ma6 = fmul float %ea6, %a6
  %va6 = insertelement <8 x float> %va5, float %ma6, i64 6
  %a7 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src_a, i32 7, i32 0)
  %ea7 = extractelement <8 x float> %acc_a, i64 7
  %ma7 = fmul float %ea7, %a7
  %va7 = insertelement <8 x float> %va6, float %ma7, i64 7

  ; ---- Group B: source = exp2(1.0), lanes 0-7 (same lane range, different src) ----
  %src_b = call float @llvm.exp2.f32(float 1.000000e+00)
  %b0 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src_b, i32 0, i32 0)
  %eb0 = extractelement <8 x float> %acc_b, i64 0
  %mb0 = fmul float %eb0, %b0
  %vb0 = insertelement <8 x float> %acc_b, float %mb0, i64 0
  %b1 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src_b, i32 1, i32 0)
  %eb1 = extractelement <8 x float> %acc_b, i64 1
  %mb1 = fmul float %eb1, %b1
  %vb1 = insertelement <8 x float> %vb0, float %mb1, i64 1
  %b2 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src_b, i32 2, i32 0)
  %eb2 = extractelement <8 x float> %acc_b, i64 2
  %mb2 = fmul float %eb2, %b2
  %vb2 = insertelement <8 x float> %vb1, float %mb2, i64 2
  %b3 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src_b, i32 3, i32 0)
  %eb3 = extractelement <8 x float> %acc_b, i64 3
  %mb3 = fmul float %eb3, %b3
  %vb3 = insertelement <8 x float> %vb2, float %mb3, i64 3
  %b4 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src_b, i32 4, i32 0)
  %eb4 = extractelement <8 x float> %acc_b, i64 4
  %mb4 = fmul float %eb4, %b4
  %vb4 = insertelement <8 x float> %vb3, float %mb4, i64 4
  %b5 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src_b, i32 5, i32 0)
  %eb5 = extractelement <8 x float> %acc_b, i64 5
  %mb5 = fmul float %eb5, %b5
  %vb5 = insertelement <8 x float> %vb4, float %mb5, i64 5
  %b6 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src_b, i32 6, i32 0)
  %eb6 = extractelement <8 x float> %acc_b, i64 6
  %mb6 = fmul float %eb6, %b6
  %vb6 = insertelement <8 x float> %vb5, float %mb6, i64 6
  %b7 = call float @llvm.genx.GenISA.WaveBroadcast.f32(float %src_b, i32 7, i32 0)
  %eb7 = extractelement <8 x float> %acc_b, i64 7
  %mb7 = fmul float %eb7, %b7
  %vb7 = insertelement <8 x float> %vb6, float %mb7, i64 7

  %res_a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v16i16(<8 x float> %va7, <8 x i16> zeroinitializer, <16 x i16> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %res_b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v16i16(<8 x float> %vb7, <8 x i16> zeroinitializer, <16 x i16> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  br label %loop

}

declare float @llvm.genx.GenISA.WaveBroadcast.f32(float, i32, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v16i16(<8 x float>, <8 x i16>, <16 x i16>, i32, i32, i32, i32, i1)
declare float @llvm.exp2.f32(float)

!igc.functions = !{!0}

!0 = distinct !{ptr @test_two_srcs, !1}
!1 = distinct !{!2, !3}
!2 = distinct !{!"function_type", i32 0}
!3 = distinct !{!"sub_group_size", i32 16}
