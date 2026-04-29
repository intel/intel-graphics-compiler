;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: Regression test for a bug where Func was not set for phi nodes and
; COM: regular instructions in rebuildLiveRangeForValue. This caused
; COM: CG->getNode(nullptr) to return an empty node, skipping weak liveness
; COM: segments for values live across subroutine calls.
; COM:
; COM: We check that the live range for %input (a regular instruction in the
; COM: kernel that is live across the subroutine call) includes weak liveness
; COM: segments through the subroutine body.

; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXModule -GenXNumberingWrapper \
; RUN:   -GenXLiveRangesWrapper -march=genx64 \
; RUN:   -mcpu=Xe2 -mtriple=spir64-unknown-unknown \
; RUN:   -print-after=GenXLiveRangesWrapper -S < %s 2>&1 | FileCheck %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXModule -GenXNumberingWrapper \
; RUN:   -GenXLiveRangesWrapper -march=genx64 \
; RUN:   -mcpu=Xe2 -mtriple=spir64-unknown-unknown \
; RUN:   -print-after=GenXLiveRangesWrapper -S < %s 2>&1 | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare <16 x i32> @llvm.genx.oword.ld.v16i32(i32, i32, i32) #0
declare void @llvm.genx.oword.st.v16i32(i32, i32, <16 x i32>) #1

define internal spir_func <16 x i32> @subroutine(<16 x i32> %arg) #2 {
  %add = add <16 x i32> %arg, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  ret <16 x i32> %add
}

; %input is passed as a call argument to the subroutine and also used after
; the call, so it is live across the subroutine call. As a regular instruction
; (not an Argument), its weak liveness depends on the fix setting Func.
; CHECK: input; :{{.*}}[w
define dllexport spir_kernel void @kernel(i32 %bti_in, i32 %bti_out, i64 %privBase) #3 {
entry:
  %input = tail call <16 x i32> @llvm.genx.oword.ld.v16i32(i32 0, i32 %bti_in, i32 0)
  %res = call spir_func <16 x i32> @subroutine(<16 x i32> %input)
  tail call void @llvm.genx.oword.st.v16i32(i32 %bti_out, i32 0, <16 x i32> %input)
  tail call void @llvm.genx.oword.st.v16i32(i32 %bti_out, i32 128, <16 x i32> %res)
  ret void
}

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }
attributes #2 = { noinline nounwind "target-cpu"="Xe2" }
attributes #3 = { noinline nounwind "CMGenxMain" "oclrt"="1" "target-cpu"="Xe2" }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}

!0 = !{void (i32, i32, i64)* @kernel, !"kernel", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 2, i32 2, i32 96}
!2 = !{i32 72, i32 80, i32 64}
!3 = !{i32 0, i32 0}
!4 = !{!"buffer_t read_write", !"buffer_t read_write", !""}
!5 = !{void (i32, i32, i64)* @kernel, !6, !7, !8, !9}
!6 = !{i32 0, i32 0, i32 0}
!7 = !{i32 0, i32 1, i32 2}
!8 = !{}
!9 = !{i32 0, i32 1, i32 2}
