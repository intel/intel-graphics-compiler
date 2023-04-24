;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmabi -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@global.int = internal global i32 0

; COM: the test checks that if no align was specified, the respected alloca
; COM: should not have one too. Starting from LLVM 11, this semantic is
; COM: represented through 'align 1'.
; FIXME: Make 'align 1' an unconditional part of the check after LLVM 9-10
;        gets abolished.
; CHECK: %global.int.local = alloca i32{{(, align 1)?}}
; CHECK: store i32 0, i32* %global.int.local{{(, align 1)?}}
define dllexport void @kernel(float %kernel.value) {
  %1 = load i32, i32* @global.int, align 4
  ret void
}

!genx.kernels = !{!0}
!0 = !{void (float)* @kernel}
