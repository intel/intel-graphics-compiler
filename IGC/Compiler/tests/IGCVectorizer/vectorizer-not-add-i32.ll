; REQUIRES: regkeys
; RUN: igc_opt -S  --igc-vectorizer -dce  --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 < %s 2>&1 | FileCheck %s

; CHECK: Start:   %25 = insertelement <8 x i32> zeroinitializer, i32 %17, i64 0
; CHECK: Operand [1]:  First:   %17 = mul i32 %9, %1
; CHECK:  Not safe to vectorize

; CHECK: some elements weren't even vectorized

; CHECK: %1 = phi i32 [ 0, %0 ], [ %35, %._crit_edge ]
; CHECK: %2 = phi i32 [ 1, %0 ], [ %36, %._crit_edge ]
; CHECK: %3 = phi i32 [ 2, %0 ], [ %37, %._crit_edge ]
; CHECK: %4 = phi i32 [ 3, %0 ], [ %38, %._crit_edge ]
; CHECK: %5 = phi i32 [ 4, %0 ], [ %39, %._crit_edge ]
; CHECK: %6 = phi i32 [ 5, %0 ], [ %40, %._crit_edge ]
; CHECK: %7 = phi i32 [ 6, %0 ], [ %41, %._crit_edge ]
; CHECK: %8 = phi i32 [ 7, %0 ], [ %42, %._crit_edge ]
; CHECK-NOT: %vectorized_phi


; ModuleID = 'reduced.ll'
source_filename = "initial_test.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @quux() {
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge, %0
  %1 = phi i32 [ 0, %0 ], [ %35, %._crit_edge ]
  %2 = phi i32 [ 1, %0 ], [ %36, %._crit_edge ]
  %3 = phi i32 [ 2, %0 ], [ %37, %._crit_edge ]
  %4 = phi i32 [ 3, %0 ], [ %38, %._crit_edge ]
  %5 = phi i32 [ 4, %0 ], [ %39, %._crit_edge ]
  %6 = phi i32 [ 5, %0 ], [ %40, %._crit_edge ]
  %7 = phi i32 [ 6, %0 ], [ %41, %._crit_edge ]
  %8 = phi i32 [ 7, %0 ], [ %42, %._crit_edge ]
  %9  = add i32 %1, 1
  %10 = add i32 %2, 2
  %11 = add i32 %3, 3
  %12 = add i32 %4, 4
  %13 = add i32 %5, 5
  %14 = add i32 %6, 6
  %15 = add i32 %7, 7
  %16 = add i32 %8, 8
  %17 = mul i32 %9, %1
  %18 = mul i32 %10, %2
  %19 = mul i32 %11, %3
  %20 = mul i32 %12, %4
  %21 = mul i32 %13, %5
  %22 = mul i32 %14, %6
  %23 = mul i32 %15, %7
  %24 = mul i32 %16, %8
  %25 = insertelement <8 x i32> zeroinitializer, i32 %17, i64 0
  %26 = insertelement <8 x i32> %25, i32 %18, i64 1
  %27 = insertelement <8 x i32> %26, i32 %19, i64 2
  %28 = insertelement <8 x i32> %27, i32 %20, i64 3
  %29 = insertelement <8 x i32> %28, i32 %21, i64 4
  %30 = insertelement <8 x i32> %29, i32 %22, i64 5
  %31 = insertelement <8 x i32> %30, i32 %23, i64 6
  %32 = insertelement <8 x i32> %31, i32 %24, i64 7
  %33 = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x i32> %32, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %34 = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x i32> %33, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %35 = extractelement <8 x i32> %34, i64 0
  %36 = extractelement <8 x i32> %34, i64 1
  %37 = extractelement <8 x i32> %34, i64 2
  %38 = extractelement <8 x i32> %34, i64 3
  %39 = extractelement <8 x i32> %34, i64 4
  %40 = extractelement <8 x i32> %34, i64 5
  %41 = extractelement <8 x i32> %34, i64 6
  %42 = extractelement <8 x i32> %34, i64 7
  br label %._crit_edge
}

; Function Attrs: convergent nounwind readnone willreturn
declare <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x i32>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #1

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare i32 @llvm.exp2.i32(i32) #2

attributes #0 = { convergent nounwind }
attributes #1 = { convergent nounwind readnone willreturn }
attributes #2 = { nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!0}
!0 = !{void ()* @quux, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"sub_group_size", i32 16}
