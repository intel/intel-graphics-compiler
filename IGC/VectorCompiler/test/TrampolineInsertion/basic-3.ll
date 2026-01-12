;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -GenXTrampolineInsertion -vc-enable-trampoline-insertion=true -march=genx64 -mcpu=XeLPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_legacy_opaque %use_old_pass_manager% -GenXTrampolineInsertion -vc-enable-trampoline-insertion=true -march=genx64 -mcpu=XeLPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

; RUN: %opt_new_pm_typed -passes=GenXTrampolineInsertion -vc-enable-trampoline-insertion=true -march=genx64 -mcpu=XeLPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_new_pm_opaque -passes=GenXTrampolineInsertion -vc-enable-trampoline-insertion=true -march=genx64 -mcpu=XeLPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

; COM: direct with internal linkage type
; CHECK: define internal spir_func void @foo_direct

define internal spir_func void @foo(<8 x i32>* %vec.ref) {
  %vec.ref.ld = load <8 x i32>, <8 x i32>* %vec.ref
  ret void
}

define dllexport void @kernel() {
  %kernel.vec.ref = alloca <8 x i32>, align 32

; CHECK: call spir_func void @foo_direct
  call spir_func void @foo(<8 x i32>* nonnull %kernel.vec.ref)

; CHECK-TYPED-PTRS: %fptr = ptrtoint void (<8 x i32>*)* @foo to i64
; CHECK-OPAQUE-PTRS: %fptr = ptrtoint ptr @foo to i64
  %fptr = ptrtoint void (<8 x i32>*)* @foo to i64
  ret void
}

; COM: indirect with internal linkage type (as originaly foo was internal)
; CHECK: define internal spir_func void @foo
; CHECK-SAME: #[[IndirectAttrs:[0-9]]]
; CHECK: call spir_func void @foo_direct


; CHECK: attributes #[[IndirectAttrs]] = { "CMStackCall" }

!genx.kernels = !{!0}
!0 = !{void ()* @kernel}
