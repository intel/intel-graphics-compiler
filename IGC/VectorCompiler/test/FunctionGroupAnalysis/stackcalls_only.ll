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

; COM:    SC1
; COM:   /    \
; COM: SC2 <- SC3

; CHECK: Number of Groups = 0
; CHECK: Number of SubGroups = 3
; CHECK-NEXT: SGR[0] = <
; CHECK-NEXT: {SC1}
; CHECK-NEXT: SC1
; CHECK-NEXT: >
; CHECK-NEXT: SGR[1] = <
; CHECK-NEXT: {SC2}
; CHECK-NEXT: SC2
; CHECK-NEXT: --SGR[0]: <SC1>
; CHECK-NEXT: >
; CHECK-NEXT: SGR[2] = <
; CHECK-NEXT: {SC3}
; CHECK-NEXT: SC3
; CHECK-NEXT: --SGR[0]: <SC1>
; CHECK-NEXT: --SGR[1]: <SC2>
; CHECK-NEXT: >

define internal spir_func void @SC1(i32 %0, i32 %1) #0 {
  ret void
}

define internal spir_func void @SC2(i32 %0) #0 {
  call spir_func void @SC1(i32 %0, i32 1) #0
  ret void
}

define internal spir_kernel void @SC3(i32 %0) #0 {
  call spir_func void @SC1(i32 %0, i32 2) #0
  call spir_func void @SC2(i32 %0) #0
  ret void
}

attributes #0 = { noinline nounwind readnone "CMStackCall" }
