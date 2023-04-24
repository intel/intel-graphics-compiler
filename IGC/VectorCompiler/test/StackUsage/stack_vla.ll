;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXStackUsage -march=genx64 -mcpu=Gen9 -mtriple=spir64-unkonwn-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s | FileCheck %s

; COM: test for VC.Stack.Amount attr with variable length array

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"

define dllexport spir_kernel void @main(i32 %n) #0 {
  %ptr = alloca i32, i32 %n, align 4
; CHECK-NOT: "VC.Stack.Amount"
  ret void
}

attributes #0 = { "CMGenxMain" }

!genx.kernel.internal = !{!0}
!0 = !{void (i32)* @main}
