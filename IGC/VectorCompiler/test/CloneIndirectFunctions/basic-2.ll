;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXCloneIndirectFunctions -vc-enable-clone-indirect-functions=true -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

; COM: indirect with external linkage type
; CHECK: declare spir_func void @foo
; CHECK-SAME: #[[IndirectAttrs:[0-9]]]

declare spir_func void @foo(<8 x i32>* %vec.ref)

define dllexport void @kernel() {
  %kernel.vec.ref = alloca <8 x i32>, align 32

  call spir_func void @foo(<8 x i32>* nonnull %kernel.vec.ref)
; COM: call of indirect-only function. It will be replaced later by global value
; COM: lowering to use gaddr instruction.
; CHECK: call spir_func void @foo
  ret void
}

; CHECK: attributes #[[IndirectAttrs]] = { "CMStackCall" }

!genx.kernels = !{!0}
!0 = !{void ()* @kernel}
