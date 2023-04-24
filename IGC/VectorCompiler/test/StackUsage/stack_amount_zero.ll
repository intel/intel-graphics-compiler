;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXStackUsage -march=genx64 -mcpu=Gen9 -mtriple=spir64-unkonwn-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s | FileCheck %s

; COM: test for VC.Stack.Amount attr with zero amount of stack

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"

define internal spir_func void @foo() {
  ret void
}

define dllexport spir_kernel void @main() #0 {
  call spir_func void @foo()
; CHECK: "VC.Stack.Amount"="0"
  ret void
}

attributes #0 = { "CMGenxMain" }

!genx.kernel.internal = !{!0}
!0 = !{void ()* @main}
