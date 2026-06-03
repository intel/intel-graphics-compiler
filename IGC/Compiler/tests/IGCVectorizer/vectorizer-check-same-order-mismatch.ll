;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 --dce --platformbmg < %s 2>&1 | FileCheck %s

; Exercises the rejection branch of checkIsSameOrder.
;
; Like the matching test, slice A's op-1 packs %s0..%s7 into an InsertElement
; chain at indices 0..7.  Slice B picks the very same scalars but in a
; rotated lane order ([%s3,%s4,%s5,%s6,%s7,%s0,%s1,%s2]).  The old code only
; checked that every op-1 mapped to the same vector and would have wrongly
; reused chain A; checkIsSameOrder must now detect the mismatch and emit
; "Not the same order" so the slice falls back to building a fresh chain.

; CHECK: Not the same order

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_mismatched_order() {
bb:
  br label %loop

loop:                                             ; preds = %loop, %bb
  %p0 = phi float [ 0.000000e+00, %bb ], [ %e0, %loop ]
  %p1 = phi float [ 0.000000e+00, %bb ], [ %e1, %loop ]
  %p2 = phi float [ 0.000000e+00, %bb ], [ %e2, %loop ]
  %p3 = phi float [ 0.000000e+00, %bb ], [ %e3, %loop ]
  %p4 = phi float [ 0.000000e+00, %bb ], [ %e4, %loop ]
  %p5 = phi float [ 0.000000e+00, %bb ], [ %e5, %loop ]
  %p6 = phi float [ 0.000000e+00, %bb ], [ %e6, %loop ]
  %p7 = phi float [ 0.000000e+00, %bb ], [ %e7, %loop ]

  ; cos is not in the vectorizer's safe-list, so these stay scalar and the
  ; first fmul slice will pack them via createVector.
  %s0 = call float @llvm.cos.f32(float 0.000000e+00)
  %s1 = call float @llvm.cos.f32(float 1.000000e+00)
  %s2 = call float @llvm.cos.f32(float 2.000000e+00)
  %s3 = call float @llvm.cos.f32(float 3.000000e+00)
  %s4 = call float @llvm.cos.f32(float 4.000000e+00)
  %s5 = call float @llvm.cos.f32(float 5.000000e+00)
  %s6 = call float @llvm.cos.f32(float 6.000000e+00)
  %s7 = call float @llvm.cos.f32(float 7.000000e+00)

  ; Slice A: op-1 collected as [%s0..%s7] in slice order -> chain has
  ; %s_i inserted at index i.
  %a0 = fmul float %p0, %s0
  %a1 = fmul float %p1, %s1
  %a2 = fmul float %p2, %s2
  %a3 = fmul float %p3, %s3
  %a4 = fmul float %p4, %s4
  %a5 = fmul float %p5, %s5
  %a6 = fmul float %p6, %s6
  %a7 = fmul float %p7, %s7

  ; Slice B: op-1 collected as [%s3,%s4,%s5,%s6,%s7,%s0,%s1,%s2] - all map
  ; to chain A but the lane ordering does not match the chain.
  %b0 = fmul float %a0, %s3
  %b1 = fmul float %a1, %s4
  %b2 = fmul float %a2, %s5
  %b3 = fmul float %a3, %s6
  %b4 = fmul float %a4, %s7
  %b5 = fmul float %a5, %s0
  %b6 = fmul float %a6, %s1
  %b7 = fmul float %a7, %s2

  %vb0 = insertelement <8 x float> zeroinitializer, float %b0, i64 0
  %vb1 = insertelement <8 x float> %vb0, float %b1, i64 1
  %vb2 = insertelement <8 x float> %vb1, float %b2, i64 2
  %vb3 = insertelement <8 x float> %vb2, float %b3, i64 3
  %vb4 = insertelement <8 x float> %vb3, float %b4, i64 4
  %vb5 = insertelement <8 x float> %vb4, float %b5, i64 5
  %vb6 = insertelement <8 x float> %vb5, float %b6, i64 6
  %vb7 = insertelement <8 x float> %vb6, float %b7, i64 7

  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %vb7, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)

  %e0 = extractelement <8 x float> %dpas, i64 0
  %e1 = extractelement <8 x float> %dpas, i64 1
  %e2 = extractelement <8 x float> %dpas, i64 2
  %e3 = extractelement <8 x float> %dpas, i64 3
  %e4 = extractelement <8 x float> %dpas, i64 4
  %e5 = extractelement <8 x float> %dpas, i64 5
  %e6 = extractelement <8 x float> %dpas, i64 6
  %e7 = extractelement <8 x float> %dpas, i64 7

  br label %loop
}

declare float @llvm.cos.f32(float)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

!igc.functions = !{!0}

!0 = distinct !{ptr @test_mismatched_order, !1}
!1 = distinct !{!2, !3}
!2 = distinct !{!"function_type", i32 0}
!3 = distinct !{!"sub_group_size", i32 16}
