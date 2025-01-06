;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -GenXCloneIndirectFunctions -vc-enable-clone-indirect-functions=true -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_legacy_opaque %use_old_pass_manager% -GenXCloneIndirectFunctions -vc-enable-clone-indirect-functions=true -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

; RUN: %opt_new_pm_typed -passes=GenXCloneIndirectFunctions -vc-enable-clone-indirect-functions=true -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_new_pm_opaque -passes=GenXCloneIndirectFunctions -vc-enable-clone-indirect-functions=true -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

; COM: No direct calls, so it's enough to have only indirectly called version
; CHECK-NO: @foo_direct
; CHECK: @foo
; CHECK-SAME: #[[IndirectAttrs:[0-9]]]

define internal spir_func void @foo(<8 x i32>* %vec.ref) {
  %vec.ref.ld = load <8 x i32>, <8 x i32>* %vec.ref
  ret void
}

define dllexport void @kernel() {
  %kernel.vec.ref = alloca <8 x i32>, align 32

; CHECK-TYPED-PTRS: %fptr = ptrtoint void (<8 x i32>*)* @foo to i64
; CHECK-OPAQUE-PTRS: %fptr = ptrtoint ptr @foo to i64
  %fptr = ptrtoint void (<8 x i32>*)* @foo to i64
  ret void
}

; CHECK: attributes #[[IndirectAttrs]] = { "CMStackCall" }

!genx.kernels = !{!0}
!0 = !{void ()* @kernel}
