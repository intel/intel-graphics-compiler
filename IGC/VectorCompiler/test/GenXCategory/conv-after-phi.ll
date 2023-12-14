;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXCategoryWrapper \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Gen9 -S < %s | FileCheck %s

declare void @llvm.genx.oword.st.v8f32(i32, i32, <8 x float>) #0
declare !genx_intrinsic_id !17 i16 @llvm.genx.rdregioni.i16.v3i16.i16(<3 x i16>, i32, i32, i32, i16, i32) #1

define dllexport spir_kernel void @kernel(<3 x i16> %impl.arg.llvm.genx.local.id16, i64 %impl.arg.private.base) local_unnamed_addr #2 {
entry:
  %rdregioni = call i16 @llvm.genx.rdregioni.i16.v3i16.i16(<3 x i16> %impl.arg.llvm.genx.local.id16, i32 0, i32 1, i32 1, i16 0, i32 0)
  %tobool.not = icmp eq i16 %rdregioni, 0
  br i1 %tobool.not, label %if, label %if.end

if:
  br label %if.end

if.end:
; CHECK: %.dst.1 = phi i32
; CHECK-NEXT: %.dst.2 = phi i32
; CHECK-NEXT: call i32 @llvm.genx.convert
; CHECK-NEXT: call i32 @llvm.genx.convert
  %.dst.1 = phi i32 [ 4, %if ], [ 7, %entry ]
  %.dst.2 = phi i32 [ 2, %if ], [ 5, %entry ]
  tail call void @llvm.genx.oword.st.v8f32(i32 %.dst.1, i32 0, <8 x float> zeroinitializer)
  tail call void @llvm.genx.oword.st.v8f32(i32 %.dst.2, i32 0, <8 x float> zeroinitializer)
  ret void
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readnone }
attributes #2 = { noinline nounwind "CMGenxMain" "oclrt"="1" }

!genx.kernels = !{!6}
!genx.kernel.internal = !{!13}

!4 = !{}
!6 = !{void (<3 x i16>, i64)* @kernel, !"kernel", !7, i32 0, !8, !9, !10, i32 0}
!7 = !{i32 24, i32 96}
!8 = !{i32 32, i32 64}
!9 = !{i32 0, i32 0}
!10 = !{!""}
!13 = !{void (<3 x i16>, i64)* @kernel, !14, !15, !4, !16}
!14 = !{i32 0, i32 0}
!15 = !{i32 7, i32 8}
!16 = !{i32 255, i32 255}
!17 = !{i32 10973}