;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -GenXDepressurizerWrapper \
; RUN:  -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

; Function Attrs: nofree nounwind readonly
declare <16 x i32> @llvm.genx.oword.ld.unaligned.v16i32(i32, i32, i32) #0

; Function Attrs: nofree nosync nounwind readnone
declare !genx_intrinsic_id !17 <32 x i32> @llvm.genx.wrregioni.v64i32.v32i32.i16.i1(<32 x i32>, <16 x i32>, i32, i32, i32, i16, i32, i1) #2

; Function Attrs: nounwind
declare void @llvm.genx.oword.st.v32i32(i32, i32, <32 x i32>) #3

; Function Attrs: nounwind
declare void @llvm.genx.oword.st.v16i32(i32, i32, <16 x i32>) #3

; Function Attrs: noinline nounwind
define dllexport void @convolution(i32 %val, i32 %idx) local_unnamed_addr #1 {
  %1 = tail call <16 x i32> @llvm.genx.oword.ld.unaligned.v16i32(i32 0, i32 2, i32 0)
  %2 = tail call <16 x i32> @llvm.genx.oword.ld.unaligned.v16i32(i32 0, i32 %idx, i32 64)
  %3 = tail call <16 x i32> @llvm.genx.oword.ld.unaligned.v16i32(i32 0, i32 %idx, i32 128)
  %cmp = icmp eq i32 %val, 8
  br i1 %cmp, label %BB, label %exit

BB:
  ; CHECK: BB
  ; CHECK: [[CATEGORY:[^ ]+]] = call i32 @llvm.genx.convert.i32(i32 2)
  ; CHECK: tail call <16 x i32> @llvm.genx.oword.ld.unaligned.v16i32(i32 0, i32 [[CATEGORY]], i32 0)
  %wrregioni1 = tail call <32 x i32> @llvm.genx.wrregioni.v64i32.v32i32.i16.i1(<32 x i32> undef, <16 x i32> %1, i32 0, i32 16, i32 1, i16 0, i32 undef, i1 true)
  %wrregioni2 = tail call <32 x i32> @llvm.genx.wrregioni.v64i32.v32i32.i16.i1(<32 x i32> %wrregioni1, <16 x i32> %3, i32 0, i32 16, i32 1, i16 64, i32 undef, i1 true)
  tail call void @llvm.genx.oword.st.v32i32(i32 3, i32 0, <32 x i32> %wrregioni2)
  tail call void @llvm.genx.oword.st.v16i32(i32 3, i32 128, <16 x i32> %2)
  br label %exit

exit:
  ret void
}

attributes #0 = { nofree nounwind readonly }
attributes #1 = { nounwind "CMGenxMain" "oclrt"="1" }
attributes #2 = { nofree nosync nounwind readnone }
attributes #3 = { nounwind }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}

!0 = !{void (i32, i32)* @convolution, !"convolution", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 64, i32 128}
!2 = !{i32 -1, i32 16}
!3 = !{i32 0}
!4 = !{}
!5 = !{void (i32, i32)* @convolution, !6, !7, !8, null}
!6 = !{i32 0, i32 0}
!7 = !{i32 0, i32 0}
!8 = !{!9}
!9 = !{i32 0, !10}
!10 = !{!11}
!11 = !{i32 1, i32 0}

!17 = !{i32 11177}
