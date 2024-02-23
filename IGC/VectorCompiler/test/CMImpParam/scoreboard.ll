;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: not %opt %use_old_pass_manager% -cmimpparam -march=genx64 -mcpu=Gen9 -S \
; RUN:    < %s 2>&1 | FileCheck --check-prefix=OCL-CHECK %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; OCL-CHECK: scoreboarding intrinsics are not supported

declare i32 @llvm.genx.get.scoreboard.bti()
declare i32 @llvm.genx.get.scoreboard.depcnt()
declare <16 x i8> @llvm.genx.get.scoreboard.deltas()

define dllexport spir_kernel void @kernel() {
  %k.bti = call i32 @llvm.genx.get.scoreboard.bti()
  %k.depcnt = call i32 @llvm.genx.get.scoreboard.depcnt()
  %k.deltas = call <16 x i8> @llvm.genx.get.scoreboard.deltas()
  ret void
}

!genx.kernels = !{!1}

!0 = !{}
!1 = !{void ()* @kernel, !"kernel", !0, i32 0, !0, !0, !0, i32 0, i32 0}
