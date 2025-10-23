;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt -S  --igc-vectorizer -dce  --regkey=VectorizerAllowADD=1 --regkey=VectorizerAllowMUL=1 --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 < %s 2>&1 | FileCheck %s

; CHECK: Candidate:   %32 = insertelement <8 x i16> %31, i16 %24, i64 7
; CHECK: Chain:   %32 = insertelement <8 x i16> %31, i16 %24, i64 7
; CHECK: --------------------------
; CHECK: Insert:   %32 = insertelement <8 x i16> %31, i16 %24, i64 7
; CHECK: fin:   %32 = insertelement <8 x i16> %31, i16 %24, i64 7
; CHECK: vec:   %25 = insertelement <8 x i16> zeroinitializer, i16 %17, i64 0
; CHECK: vec:   %26 = insertelement <8 x i16> %25, i16 %18, i64 1
; CHECK: vec:   %27 = insertelement <8 x i16> %26, i16 %19, i64 2
; CHECK: vec:   %28 = insertelement <8 x i16> %27, i16 %20, i64 3
; CHECK: vec:   %29 = insertelement <8 x i16> %28, i16 %21, i64 4
; CHECK: vec:   %30 = insertelement <8 x i16> %29, i16 %22, i64 5
; CHECK: vec:   %31 = insertelement <8 x i16> %30, i16 %23, i64 6
; CHECK: vec:   %32 = insertelement <8 x i16> %31, i16 %24, i64 7

; CHECK: Start:   %25 = insertelement <8 x i16> zeroinitializer, i16 %17, i64 0
; CHECK: Operand [0]:  Not an instruction
; CHECK: Operand [1]:  First:   %17 = mul i16 %9, %1
; CHECK: Not safe to vectorize

; CHECK-NOT: {{%vectorized.*}}


; ModuleID = 'reduced.ll'
source_filename = "initial_test.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @quux() {
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge, %0
  %1 = phi i16 [ 0, %0 ], [ %35, %._crit_edge ]
  %2 = phi i16 [ 1, %0 ], [ %36, %._crit_edge ]
  %3 = phi i16 [ 2, %0 ], [ %37, %._crit_edge ]
  %4 = phi i16 [ 3, %0 ], [ %38, %._crit_edge ]
  %5 = phi i16 [ 4, %0 ], [ %39, %._crit_edge ]
  %6 = phi i16 [ 5, %0 ], [ %40, %._crit_edge ]
  %7 = phi i16 [ 6, %0 ], [ %41, %._crit_edge ]
  %8 = phi i16 [ 7, %0 ], [ %42, %._crit_edge ]
  %9  = add i16 %1, 1
  %10 = add i16 %2, 2
  %11 = add i16 %3, 3
  %12 = add i16 %4, 4
  %13 = add i16 %5, 5
  %14 = add i16 %6, 6
  %15 = add i16 %7, 7
  %16 = add i16 %8, 8
  %17 = mul i16 %9, %1
  %18 = mul i16 %10, %2
  %19 = mul i16 %11, %3
  %20 = mul i16 %12, %4
  %21 = mul i16 %13, %5
  %22 = mul i16 %14, %6
  %23 = mul i16 %15, %7
  %24 = mul i16 %16, %8
  %25 = insertelement <8 x i16> zeroinitializer, i16 %17, i64 0
  %26 = insertelement <8 x i16> %25, i16 %18, i64 1
  %27 = insertelement <8 x i16> %26, i16 %19, i64 2
  %28 = insertelement <8 x i16> %27, i16 %20, i64 3
  %29 = insertelement <8 x i16> %28, i16 %21, i64 4
  %30 = insertelement <8 x i16> %29, i16 %22, i64 5
  %31 = insertelement <8 x i16> %30, i16 %23, i64 6
  %32 = insertelement <8 x i16> %31, i16 %24, i64 7
  %33 = call <8 x i16> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i16(<8 x i16> %32, <8 x i16> zeroinitializer, <8 x i16> zeroinitializer, i16 0, i16 0, i16 0, i16 0, i1 false)
  %34 = call <8 x i16> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i16(<8 x i16> %33, <8 x i16> zeroinitializer, <8 x i16> zeroinitializer, i16 0, i16 0, i16 0, i16 0, i1 false)
  %35 = extractelement <8 x i16> %34, i64 0
  %36 = extractelement <8 x i16> %34, i64 1
  %37 = extractelement <8 x i16> %34, i64 2
  %38 = extractelement <8 x i16> %34, i64 3
  %39 = extractelement <8 x i16> %34, i64 4
  %40 = extractelement <8 x i16> %34, i64 5
  %41 = extractelement <8 x i16> %34, i64 6
  %42 = extractelement <8 x i16> %34, i64 7
  br label %._crit_edge
}

; Function Attrs: convergent nounwind readnone willreturn
declare <8 x i16> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i16(<8 x i16>, <8 x i16>, <8 x i16>, i16, i16, i16, i16, i1) #1

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare i16 @llvm.exp2.i16(i16) #2

attributes #0 = { convergent nounwind }
attributes #1 = { convergent nounwind readnone willreturn }
attributes #2 = { nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!0}
!0 = !{void ()* @quux, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
