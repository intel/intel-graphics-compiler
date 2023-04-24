;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXModule -FunctionGroupAnalysis -march=genx64 \
; RUN: -print-function-group-info -disable-output -mcpu=Gen9 -S < %s | FileCheck \
; RUN: %s --match-full-lines

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; COM:   wobbler
; COM:   /    \
; COM: foo <- bar

; CHECK: Number of Groups = 0
; CHECK: Number of SubGroups = 0

define internal spir_func void @wobbler(i32 %0, i32 %1) #0 {
  ret void
}

define internal spir_func void @foo(i32 %0) #0 {
  call spir_func void @wobbler(i32 %0, i32 3) #0
  ret void
}

define internal spir_func void @bar(i32 %0) #0 {
  call spir_func void @foo(i32 0) #0
  call spir_func void @wobbler(i32 %0, i32 1) #0
  ret void
}

attributes #0 = { noinline nounwind }
