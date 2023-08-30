;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXCategoryWrapper \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s

; Function Attrs: nofree nounwind readonly
declare <32 x i32> @llvm.genx.oword.ld.unaligned.v32i32(i32, i32, i32) #1

; Function Attrs: nounwind
declare void @llvm.genx.oword.st.v32i32(i32, i32, <32 x i32>) #2

define dllexport void @kernel(i32 %offset, i64 %impl.arg.private.base) #0 {
entry:
  ; CHECK: call i32 @llvm.genx.convert.i32(i32 2)
  %ld1 = tail call <32 x i32> @llvm.genx.oword.ld.unaligned.v32i32(i32 0, i32 2, i32 %offset)
  ; CHECK: call i32 @llvm.genx.convert.i32(i32 3)
  %ld3 = tail call <32 x i32> @llvm.genx.oword.ld.unaligned.v32i32(i32 0, i32 3, i32 %offset)
  ; CHECK-NOT: call i32 @llvm.genx.convert.i32(i32 3)
  call void @llvm.genx.oword.st.v32i32(i32 3, i32 0, <32 x i32> %ld1)
  ; CHECK-NOT: call i32 @llvm.genx.convert.i32(i32 2)
  call void @llvm.genx.oword.st.v32i32(i32 2, i32 0, <32 x i32> %ld3)
  br label %BB
BB:
  ; CHECK: BB
  ; CHECK: call i32 @llvm.genx.convert.i32(i32 2)
  %ld7 = tail call <32 x i32> @llvm.genx.oword.ld.unaligned.v32i32(i32 0, i32 2, i32 %offset)
  ; CHECK: call i32 @llvm.genx.convert.i32(i32 3)
  %ld9 = tail call <32 x i32> @llvm.genx.oword.ld.unaligned.v32i32(i32 0, i32 3, i32 %offset)
  ; CHECK-NOT: call i32 @llvm.genx.convert.i32(i32 3)
  call void @llvm.genx.oword.st.v32i32(i32 3, i32 128, <32 x i32> %ld7)
  ; CHECK-NOT: call i32 @llvm.genx.convert.i32(i32 2)
  call void @llvm.genx.oword.st.v32i32(i32 2, i32 128, <32 x i32> %ld9)
  ret void
}

attributes #0 = { "CMGenxMain" }
attributes #1 = { nofree nounwind readonly }
attributes #2 = { nounwind }

!genx.kernels = !{!6}
!genx.kernel.internal = !{!11}

!0 = !{i32 2, i32 2}
!1 = !{i32 0, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{i32 2, i32 0}
!4 = !{}
!5 = !{i16 6, i16 14}
!6 = !{void (i32, i64)* @kernel, !"kernel", !7, i32 0, !8, !9, !10, i32 0}
!7 = !{i32 0, i32 96}
!8 = !{i32 72, i32 64}
!9 = !{i32 0}
!10 = !{!""}
!11 = !{void (i32, i64)* @kernel, !1, !12, !4, !13}
!12 = !{i32 0, i32 1}
!13 = !{i32 -1, i32 1}