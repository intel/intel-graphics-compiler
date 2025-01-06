;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This is a test for vc-function-control option also available
; as IGC_FunctionControl environment

; RUN: %opt_legacy_typed %use_old_pass_manager% -GenXLinkageCorruptor -march=genx64 -vc-function-control=stackcall -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_legacy_opaque %use_old_pass_manager% -GenXLinkageCorruptor -march=genx64 -vc-function-control=stackcall -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

; RUN: %opt_new_pm_typed -passes=GenXLinkageCorruptor -march=genx64 -vc-function-control=stackcall -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_new_pm_opaque -passes=GenXLinkageCorruptor -march=genx64 -vc-function-control=stackcall -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

; Function Attrs: noinline nounwind
define internal spir_func void @foo(<8 x i32>* %vec.ref) {
  %vec.ref.ld = load <8 x i32>, <8 x i32>* %vec.ref
  ret void
}

; Function Attrs: noinline nounwind
define dllexport void @kernel() {
  %kernel.vec.ref = alloca <8 x i32>, align 32

  call spir_func void @foo(<8 x i32>* nonnull %kernel.vec.ref)
; CHECK-TYPED-PTRS: call spir_func void @foo(<8 x i32>* nonnull %kernel.vec.ref)
; CHECK-OPAQUE-PTRS: call spir_func void @foo(ptr nonnull %kernel.vec.ref)
; CHECK: CMStackCall

  ret void
}

!genx.kernels = !{!0}
!0 = !{void ()* @kernel}
