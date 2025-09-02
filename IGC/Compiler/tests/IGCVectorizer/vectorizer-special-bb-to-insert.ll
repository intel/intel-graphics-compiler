;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -S  --igc-vectorizer -dce --regkey=VectorizerDepWindowMultiplier=6 < %s 2>&1 | FileCheck %s

; CHECK-LABEL: bb3:
; CHECK-NEXT: [[PHI:%.*]] = phi float
; CHECK-NEXT: [[VECTOR_0:%.*]] = insertelement <8 x float> undef, float [[PHI]], i32 0
; CHECK-NEXT: [[VECTOR_1:%.*]] = insertelement <8 x float> [[VECTOR_0]], float [[PHI]], i32 1
; CHECK-NEXT: [[VECTOR_2:%.*]] = insertelement <8 x float> [[VECTOR_1]], float [[PHI]], i32 2
; CHECK-NEXT: [[VECTOR_3:%.*]] = insertelement <8 x float> [[VECTOR_2]], float [[PHI]], i32 3
; CHECK-NEXT: [[VECTOR_4:%.*]] = insertelement <8 x float> [[VECTOR_3]], float [[PHI]], i32 4
; CHECK-NEXT: [[VECTOR_5:%.*]] = insertelement <8 x float> [[VECTOR_4]], float [[PHI]], i32 5
; CHECK-NEXT: [[VECTOR_6:%.*]] = insertelement <8 x float> [[VECTOR_5]], float [[PHI]], i32 6
; CHECK-NEXT: [[VECTOR_7:%.*]] = insertelement <8 x float> [[VECTOR_6]], float [[PHI]], i32 7
; CHECK-NEXT: br i1 {{%.*}}, label {{%.*}}, label {{%.*}}


define spir_kernel void @barney() {
bb:
  %tmp = fcmp une float 0.000000e+00, 0.000000e+00
  br label %bb1

bb1:                                              ; preds = %bb
  br i1 false, label %bb3, label %bb2

bb2:                                              ; preds = %bb1
  br label %bb3

bb3:                                              ; preds = %bb2, %bb1
  %tmp4 = phi float [ 0.000000e+00, %bb1 ], [ 0.000000e+00, %bb2 ]
  br i1 %tmp, label %bb5, label %bb6

bb5:                                              ; preds = %bb3
  br label %bb6

bb6:                                              ; preds = %bb5, %bb3
  %tmp7 = fmul reassoc nsz arcp contract float 0.000000e+00, %tmp4
  %tmp8 = fmul reassoc nsz arcp contract float 0.000000e+00, %tmp4
  %tmp9 = fmul reassoc nsz arcp contract float 0.000000e+00, %tmp4
  %tmp10 = fmul reassoc nsz arcp contract float 0.000000e+00, %tmp4
  %tmp11 = fmul reassoc nsz arcp contract float 0.000000e+00, %tmp4
  %tmp12 = fmul reassoc nsz arcp contract float 0.000000e+00, %tmp4
  %tmp13 = fmul reassoc nsz arcp contract float 0.000000e+00, %tmp4
  %tmp14 = fmul reassoc nsz arcp contract float 0.000000e+00, %tmp4
  %tmp15 = insertelement <8 x float> zeroinitializer, float %tmp7, i64 0
  %tmp16 = insertelement <8 x float> %tmp15, float %tmp8, i64 1
  %tmp17 = insertelement <8 x float> %tmp16, float %tmp9, i64 2
  %tmp18 = insertelement <8 x float> %tmp17, float %tmp10, i64 3
  %tmp19 = insertelement <8 x float> %tmp18, float %tmp11, i64 4
  %tmp20 = insertelement <8 x float> %tmp19, float %tmp12, i64 5
  %tmp21 = insertelement <8 x float> %tmp20, float %tmp13, i64 6
  %tmp22 = insertelement <8 x float> %tmp21, float %tmp14, i64 7
  %tmp23 = bitcast <8 x float> %tmp22 to <8 x i32>
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0, <8 x i32> %tmp23)
  ret void
}

declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)

!igc.functions = !{!0}

!0 = distinct !{void ()* @barney, !1}
!1 = distinct !{!2}
!2 = distinct !{!"sub_group_size", i32 16}
