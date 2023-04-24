;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: Test that regalloc dump does not crash with unified return.

; RUN: llc %s -march=genx64 -mcpu=Gen9 \
; RUN: -vc-fg-dump-prefix=%basename_t_ -genx-dump-regalloc \
; RUN: -o /dev/null

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Function Attrs: nounwind readnone
declare <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1) #2
; Function Attrs: nounwind
declare void @llvm.genx.media.st.v8i32(i32, i32, i32, i32, i32, i32, <8 x i32>) #3
; Function Attrs: nounwind readnone
declare i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32>, i32, i32, i32, i16, i32) #2


; Function Attrs: noinline nounwind readnone
define internal spir_func i32 @S1(<8 x i32> %v_out) unnamed_addr #0 !FuncArgSize !15 !FuncRetSize !16 {
entry:
  %sev.cast.1.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v8i32.i16(<8 x i32> %v_out, i32 0, i32 1, i32 1, i16 0, i32 undef)
  ret i32 %sev.cast.1.regioncollapsed
}

; Function Attrs: noinline nounwind
define dllexport spir_kernel void @K1(i32 %indx) local_unnamed_addr #1 {
entry:
  %call = tail call spir_func i32 @S1(<8 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>) #5, !FuncArgSize !15, !FuncRetSize !16
  %add = add nsw i32 %call, 1
  %sev.cast.1 = insertelement <1 x i32> undef, i32 %add, i64 0
  %wrregion = tail call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>, <1 x i32> %sev.cast.1, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
  tail call void @llvm.genx.media.st.v8i32(i32 0, i32 %indx, i32 0, i32 32, i32 0, i32 0, <8 x i32> %wrregion)
  ret void
}

attributes #0 = { noinline nounwind readnone "CMStackCall" }
attributes #1 = { noinline nounwind "CMGenxMain" }
attributes #2 = { nounwind readnone }
attributes #3 = { nounwind }
attributes #4 = { nounwind readnone speculatable willreturn }
attributes #5 = { noinline nounwind }

!genx.kernels = !{!8}
!genx.kernel.internal = !{!14}

!8 = !{void (i32)* @K1, !"K1", !9, i32 0, !10, !11, !12, i32 0}
!9 = !{i32 2}
!10 = !{i32 32}
!11 = !{i32 0}
!12 = !{!"buffer_t read_write"}
!14 = !{void (i32)* @K1, !11, !11, null, null}
!15 = !{i32 1}
!16 = !{i32 1}
