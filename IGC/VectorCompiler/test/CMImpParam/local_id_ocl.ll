;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmimpparam -cmimpparam-cmrt=false -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare <3 x i32> @llvm.genx.local.id.v3i32()

define dllexport spir_kernel void @kernel() {
; CHECK: @kernel(
; CHECK-SAME: <3 x i16> %impl.arg.llvm.genx.local.id16
  %ids = call <3 x i32> @llvm.genx.local.id.v3i32()
; CHECK: store <3 x i16> %impl.arg.llvm.genx.local.id16, <3 x i16>* @__imparg_llvm.genx.local.id16
; CHECK: %ids.i16 = load <3 x i16>, <3 x i16>* @__imparg_llvm.genx.local.id16
; CHECK: %ids = zext <3 x i16> %ids.i16 to <3 x i32>
  ret void
}

!genx.kernels = !{!0}

!0 = !{void ()* @kernel, !"kernel", !1, i32 0, !1, !1, !1, i32 0, i32 0}
!1 = !{}
