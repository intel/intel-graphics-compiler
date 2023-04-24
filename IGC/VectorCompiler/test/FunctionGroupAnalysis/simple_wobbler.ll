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

; COM:  wobbler
; COM:     |
; COM:    foo
; COM:   /   \
; COM: K1     K2

; CHECK: Number of Groups = 2
; CHECK-NEXT: GR[0] = <
; CHECK-NEXT: {K1}
; CHECK-NEXT: K1
; CHECK-NEXT: foo.K1
; CHECK-NEXT: wobbler.K1
; CHECK-NEXT: >
; CHECK-NEXT: GR[1] = <
; CHECK-NEXT: {K2}
; CHECK-NEXT: K2
; CHECK-NEXT: foo.K2
; CHECK-NEXT: wobbler.K2
; CHECK-NEXT: >
; CHECK: Number of SubGroups = 0

define internal spir_func void @wobbler(i32 %0, i32 %1) #0 {
  ret void
}

define internal spir_func void @foo(i32 %0) #0 {
  call spir_func void @wobbler(i32 %0, i32 1) #0
  ret void
}

define dllexport spir_kernel void @K1(i32 %0, i64 %privBase) #1 {
  call spir_func void @foo(i32 0) #0
  ret void
}

define dllexport spir_kernel void @K2(i32 %0, i64 %privBase) #1 {
  call spir_func void @foo(i32 0) #0
  ret void
}

attributes #0 = { noinline nounwind }
attributes #1 = { noinline nounwind "CMGenxMain" }

!genx.kernels = !{!2, !7}
!genx.kernel.internal = !{!8, !10}

!0 = !{i32 0, i32 0}
!1 = !{}
!2 = !{void (i32, i64)* @K1, !"K1", !3, i32 0, !4, !5, !6, i32 0}
!3 = !{i32 2, i32 96}
!4 = !{i32 72, i32 64}
!5 = !{i32 0}
!6 = !{!"buffer_t read_write"}
!7 = !{void (i32, i64)* @K2, !"K2", !3, i32 0, !4, !5, !6, i32 0}
!8 = !{void (i32, i64)* @K1, !0, !9, !1, !9}
!9 = !{i32 0, i32 1}
!10 = !{void (i32, i64)* @K2, !0, !9, !1, !9}
