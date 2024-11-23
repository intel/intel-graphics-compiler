;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% %pass_pref%CMImpParam -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% %pass_pref%CMImpParam -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare <3 x i32> @llvm.genx.local.id.v3i32()

define dllexport spir_kernel void @kernel() {
; CHECK: @kernel(
; CHECK-SAME: <3 x i16> %impl.arg.llvm.genx.local.id16
  %ids = call <3 x i32> @llvm.genx.local.id.v3i32()
; CHECK-TYPED-PTRS: store <3 x i16> %impl.arg.llvm.genx.local.id16, <3 x i16>* @__imparg_llvm.genx.local.id16
; CHECK-TYPED-PTRS: %ids.i16 = load <3 x i16>, <3 x i16>* @__imparg_llvm.genx.local.id16
; CHECK-OPAQUE-PTRS: store <3 x i16> %impl.arg.llvm.genx.local.id16, ptr @__imparg_llvm.genx.local.id16
; CHECK-OPAQUE-PTRS: %ids.i16 = load <3 x i16>, ptr @__imparg_llvm.genx.local.id16
; CHECK: %ids = zext <3 x i16> %ids.i16 to <3 x i32>
  ret void
}

!genx.kernels = !{!0}

!0 = !{void ()* @kernel, !"kernel", !1, i32 0, !1, !1, !1, i32 0, i32 0}
!1 = !{}
