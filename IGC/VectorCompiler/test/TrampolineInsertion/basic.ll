;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXTrampolineInsertion -vc-enable-trampoline-insertion=true -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

; COM: direct with internal linkage type
; CHECK: define internal spir_func void @foo_direct

define spir_func void @foo(<8 x i32>* %vec.ref) {
  %vec.ref.ld = load <8 x i32>, <8 x i32>* %vec.ref
  ret void
}

define dllexport void @kernel() {
  %kernel.vec.ref = alloca <8 x i32>, align 32

; CHECK: call spir_func void @foo_direct
  call spir_func void @foo(<8 x i32>* nonnull %kernel.vec.ref)
  ret void
}

; COM: indirect with external linkage type
; CHECK: define dso_local spir_func void @foo
; CHECK-SAME: #[[IndirectAttrs:[0-9]]]
; COM: foo calls foo_direct
; CHECK: call spir_func void @foo_direct

; CHECK: attributes #[[IndirectAttrs]] = { "CMStackCall" }

!genx.kernels = !{!0}
!0 = !{void ()* @kernel}
