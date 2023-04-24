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

declare void ()* @llvm.ssa.copy.p0f_isVoidf(void ()*)

define dllexport spir_kernel void @just_indir_call() {
  %func.ptr = call void ()* @llvm.ssa.copy.p0f_isVoidf(void ()* @empty)
  call void %func.ptr()
  ret void
}
; COM: Should stay the same (except privBase arg).
; CHECK: define dllexport spir_kernel void @just_indir_call(
; CHECK-NEXT: %func.ptr = call void ()* @llvm.ssa.copy.p0f_isVoidf(void ()* @empty)
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
