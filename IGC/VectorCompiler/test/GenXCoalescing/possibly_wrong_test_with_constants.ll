;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: Check that GenXCoalescing does not fail on bitcast with constants

; RUN: opt %use_old_pass_manager% -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXCoalescingWrapper \
; RUN:  -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s

; ModuleID = 'reduced.bc'
source_filename = "before_coalesc_0.ll"
target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare void @llvm.genx.raw.sends2.noresult.v16i1.v16i32.v16i32(i8, i8, <16 x i1>, i8, i8, i8, i32, i32, <16 x i32>, <16 x i32>)

define dllexport spir_kernel void @kernel_A(i32 %_I_extent_0, i32 %_I_extent_1, i32 %_K_extent_0, i32 %_K_extent_1, i32 %_Out_extent_0, i32 %_Out_extent_1, i32* %__I, i32* %__K, i32* %__Out, i64 %privBase, <3 x i16> %__arg_llvm.genx.local.id16) local_unnamed_addr #0 {
for.end148:
  %cast14.i370.decomp.6 = bitcast <16 x float> zeroinitializer to <16 x i32>
  tail call void @llvm.genx.raw.sends2.noresult.v16i1.v16i32.v16i32(i8 0, i8 0, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 1, i8 1, i8 15, i32 0, i32 34472967, <16 x i32> undef, <16 x i32> %cast14.i370.decomp.6)
  ret void
}

attributes #0 = { "CMGenxMain" }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}

!0 = !{void (i32, i32, i32, i32, i32, i32, i32*, i32*, i32*, i64, <3 x i16>)* @kernel_A, !"kernel_A", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 96, i32 24}
!2 = !{i32 136, i32 140, i32 144, i32 148, i32 152, i32 156, i32 160, i32 168, i32 176, i32 128, i32 64}
!3 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!4 = !{!"", !"", !"", !"", !"", !"", !"svmptr_t", !"svmptr_t", !"svmptr_t"}
!5 = !{void (i32, i32, i32, i32, i32, i32, i32*, i32*, i32*, i64, <3 x i16>)* @kernel_A, !6, !7, !8, !9}
!6 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!7 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10}
!8 = !{}
!9 = !{i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 -1, i32 0, i32 1, i32 2, i32 3, i32 4}
