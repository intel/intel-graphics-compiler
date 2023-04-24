;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPrologEpilogInsertion -dce -mattr=+ocl_runtime -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare void @llvm.genx.oword.st.v4i32(i32, i32, <4 x i32>)

; CHECK-LABEL: @kernel
; COM: the test just checks that the kernel can be compiled, there was a bug
; COM: causing an assertion to be triggered in case of *undef* ret
define internal spir_func { <4 x i32> } @_Z4funcu2CMmr2x2_i(<4 x i32> %0) #0 {
  ret { <4 x i32> } undef
}

define dllexport spir_kernel void @kernel(i32 %0, <8 x i32> %1, i64 %privBase) #1 {
  %3 = call spir_func { <4 x i32> } @_Z4funcu2CMmr2x2_i(<4 x i32> zeroinitializer)
  call void @llvm.genx.oword.st.v4i32(i32 1, i32 0, <4 x i32> <i32 1, i32 1, i32 1, i32 1>)
  ret void
}

attributes #0 = { noinline nounwind readnone "CMStackCall" }
attributes #1 = { noinline nounwind "CMGenxMain" "oclrt"="1" }

!opencl.enable.FP_CONTRACT = !{}
!genx.kernels = !{!2}
!genx.kernel.internal = !{!6}


!0 = !{}
!1 = !{i32 0, i32 0}
!2 = !{void (i32, <8 x i32>, i64)* @kernel, !"kernel", !3, i32 0, !4, !1, !5, i32 0}
!3 = !{i32 2, i32 0, i32 96}
!4 = !{i32 72, i32 96, i32 64}
!5 = !{!"buffer_t read_write", !""}
!6 = !{void (i32, <8 x i32>, i64)* @kernel, !7, !8, !0, !9}
!7 = !{i32 0, i32 0, i32 0}
!8 = !{i32 0, i32 1, i32 2}
!9 = !{i32 1, i32 -1, i32 2}
