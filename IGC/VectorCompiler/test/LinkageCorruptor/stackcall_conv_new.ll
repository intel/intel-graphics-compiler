;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This is a test for vc-function-control option also available
; as IGC_FunctionControl environment

; RUN: opt %use_old_pass_manager% -GenXLinkageCorruptor -march=genx64 -vc-function-control=stackcall -save-stack-call-linkage=true -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

; Function Attrs: noinline nounwind
define spir_func void @foo(<8 x i32>* %vec.ref) {
  %vec.ref.ld = load <8 x i32>, <8 x i32>* %vec.ref
  ret void
}

; Function Attrs: noinline nounwind
define dllexport void @kernel() {
  %kernel.vec.ref = alloca <8 x i32>, align 32

  call spir_func void @foo(<8 x i32>* nonnull %kernel.vec.ref)
; CHECK: call spir_func void @foo
; CHECK-SAME: <8 x i32>* nonnull
; CHECK: CMStackCall

  ret void
}

!genx.kernels = !{!0}
!0 = !{void ()* @kernel}
