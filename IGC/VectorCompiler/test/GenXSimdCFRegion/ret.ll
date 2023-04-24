;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -simdcf-region -enable-simdcf-transform -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXSimdCFRegion
; ------------------------------------------------
; This test checks that GenXSimdCFRegion do not generate
; if-then simd llvm-ir for region with break (based on ispc test)
; COM: TODO: Support breaks from simdcf if-then and loop

; Function Attrs: nounwind
; CHECK: f_fu
; CHECK-NOT: [[GOTO:%[A-z0-9.]*]] = call { <32 x i1>, <16 x i1>, i1 } @llvm.genx.simdcf.goto

; Function Attrs: nounwind
declare void @llvm.genx.svm.scatter.v16i1.v16i64.v16f32(<16 x i1>, i32, <16 x i64>, <16 x float>) #0

; Function Attrs: nounwind readonly
declare <16 x float> @llvm.genx.svm.gather.v16f32.v16i1.v16i64(<16 x i1>, i32, <16 x i64>, <16 x float>) #1

; Function Attrs: nounwind readnone
declare i1 @llvm.genx.any.v16i1(<16 x i1>) #2

; Function Attrs: nounwind readnone
declare i1 @llvm.genx.all.v16i1(<16 x i1>) #2

; Function Attrs: nounwind
define dllexport spir_kernel void @f_fu(i8* nocapture %0, i8* %1, float %2, i64 %impl.arg.private.base) #3 {
  %4 = ptrtoint i8* %1 to i64
  %5 = insertelement <16 x i64> undef, i64 %4, i32 0
  %6 = shufflevector <16 x i64> %5, <16 x i64> undef, <16 x i32> zeroinitializer
  %7 = add <16 x i64> %6, <i64 0, i64 4, i64 8, i64 12, i64 0, i64 4, i64 8, i64 12, i64 0, i64 4, i64 8, i64 12, i64 0, i64 4, i64 8, i64 12>
  %8 = call <16 x float> @llvm.genx.svm.gather.v16f32.v16i1.v16i64(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i32 0, <16 x i64> %7, <16 x float> undef)
  %9 = fcmp oge <16 x float> %8, <float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00, float 4.000000e+00>
  %10 = call i1 @llvm.genx.any.v16i1(<16 x i1> %9)
  br i1 %10, label %16, label %12

11:                                               ; preds = %16, %12
  ret void

12:                                               ; preds = %16, %3
  %13 = phi <16 x i1> [ zeroinitializer, %3 ], [ %9, %16 ]
  %14 = xor <16 x i1> %9, <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>
  %15 = call i1 @llvm.genx.any.v16i1(<16 x i1> %14)
  br label %11

16:                                               ; preds = %3
  %17 = ptrtoint i8* %0 to i64
  %18 = insertelement <16 x i64> undef, i64 %17, i32 0
  %19 = shufflevector <16 x i64> %18, <16 x i64> undef, <16 x i32> zeroinitializer
  %20 = add <16 x i64> %19, <i64 0, i64 4, i64 8, i64 12, i64 16, i64 20, i64 24, i64 28, i64 32, i64 36, i64 40, i64 44, i64 48, i64 52, i64 56, i64 60>
  call void @llvm.genx.svm.scatter.v16i1.v16i64.v16f32(<16 x i1> %9, i32 0, <16 x i64> %20, <16 x float> zeroinitializer)
  %21 = bitcast <16 x i1> %9 to i16
  %22 = icmp eq i16 %21, -1
; COM: This branch must be catched in algorithm
  br i1 %22, label %11, label %12
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind readnone }
attributes #3 = { nounwind "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }
attributes #5 = { nounwind writeonly }

!spirv.Source = !{!0}
!opencl.spir.version = !{!1, !2, !2, !2}
!opencl.ocl.version = !{!0, !2, !2, !2}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!3}
!spirv.Generator = !{!4}
!genx.kernels = !{!5}
!llvm.ident = !{!15, !15, !15}
!llvm.module.flags = !{!16}
!genx.kernel.internal = !{!17}

!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{i32 2, i32 0}
!3 = !{}
!4 = !{i16 6, i16 14}
!5 = !{void (i8*, i8*, float, i64)* @f_fu, !"f_fu", !6, i32 0, !7, !8, !9, i32 0}
!6 = !{i32 0, i32 0, i32 0, i32 96}
!7 = !{i32 72, i32 80, i32 88, i32 64}
!8 = !{i32 0, i32 0, i32 0}
!9 = !{!"svmptr_t", !"svmptr_t", !""}
!15 = !{!"clang version 10.0.0-4ubuntu1 "}
!16 = !{i32 1, !"wchar_size", i32 4}
!17 = !{void (i8*, i8*, float, i64)* @f_fu, !18, !19, !3, !20}
!18 = !{i32 0, i32 0, i32 0, i32 0}
!19 = !{i32 0, i32 1, i32 2, i32 3}
!20 = !{i32 255, i32 255, i32 -1, i32 255}
