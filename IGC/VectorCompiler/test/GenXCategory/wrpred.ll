;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXCategoryWrapper \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s

declare void @llvm.genx.svm.scatter.v16i1.v16i64.v16i32(<16 x i1>, i32, <16 x i64>, <16 x i32>) #0
declare !genx_intrinsic_id !6 <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1>, <8 x i1>, i32) #1

define dllexport spir_kernel void @kernel(<8 x i16> %mask, <16 x i64> %dst, <16 x i32> %data) local_unnamed_addr #2 {
  %maskv8 = icmp ne <8 x i16> %mask, zeroinitializer
; CHECK: [[CONST:%[^ ]+]] = call <16 x i1> @llvm.genx.constantpred.v16i1(<16 x i1> zeroinitializer)
; CHECK: %maskv16 = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1> [[CONST]], <8 x i1> %maskv8, i32 0)
  %maskv16 = call <16 x i1> @llvm.genx.wrpredregion.v16i1.v8i1(<16 x i1> zeroinitializer, <8 x i1> %maskv8, i32 0)
  tail call void @llvm.genx.svm.scatter.v16i1.v16i64.v16i32(<16 x i1> %maskv16, i32 0, <16 x i64> %dst, <16 x i32> %data)
  ret void
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readnone }
attributes #2 = { noinline nounwind "CMGenxMain" "VC.Stack.Amount"="0" "target-cpu"="XeHPC" }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}

!0 = !{void (<8 x i16>, <16 x i64>, <16 x i32>)* @kernel, !"kernel", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 0, i32 0, i32 0}
!2 = !{i32 64, i32 128, i32 196}
!3 = !{i32 0}
!4 = !{!""}
!5 = !{void (<8 x i16>, <16 x i64>, <16 x i32>)* @kernel, null, null, null, null}
!6 = !{i32 11177}
