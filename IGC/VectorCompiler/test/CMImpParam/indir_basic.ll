;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -CMImpParam -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_legacy_opaque %use_old_pass_manager% -CMImpParam -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

; RUN: %opt_new_pm_typed -passes=CMImpParam -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_new_pm_opaque -passes=CMImpParam -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare void ()* @llvm.ssa.copy.p0f_isVoidf(void ()*)

define dllexport spir_kernel void @just_indir_call() {
  %func.ptr = call void ()* @llvm.ssa.copy.p0f_isVoidf(void ()* @empty)
  call void %func.ptr()
  ret void
}
; COM: Should stay the same (except privBase arg).
; CHECK: define dllexport spir_kernel void @just_indir_call(
; CHECK-TYPED-PTRS-NEXT: %func.ptr = call void ()* @llvm.ssa.copy.p0f_isVoidf(void ()* @empty)
; CHECK-OPAQUE-PTRS-NEXT: %func.ptr = call ptr @llvm.ssa.copy.p0(ptr @empty)
; CHECK-NEXT: call void %func.ptr()
; CHECK-NEXT: ret void

define internal spir_func void @empty() {
  ret void
}
; CHECK: define internal spir_func void @empty()
; CHECK-NEXT: ret void

!genx.kernels = !{!0}

!0 = !{void ()* @just_indir_call, !"just_indir_call", !1, i32 0, !1, !1, !1, i32 0, i32 0}
!1 = !{}
