;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXModule -FunctionGroupAnalysis -march=genx64 \
; RUN: -print-function-group-info -disable-output -mcpu=Gen9 -S < %s | FileCheck  \
; RUN: %s --match-full-lines

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; COM:      wobbler
; COM:      /     \
; COM:   foo      bar
; COM:   /       /   \
; COM:  K1      K2   K3

; CHECK: Number of Groups = 3
; CHECK-NEXT: GR[0] = <
; CHECK-NEXT: {K1}
; CHECK-NEXT: K1
; CHECK-NEXT: foo
; CHECK-NEXT: wobbler.K1
; CHECK-NEXT: >
; CHECK-NEXT: GR[1] = <
; CHECK-NEXT: {K2}
; CHECK-NEXT: K2
; CHECK-NEXT: bar.K2
; CHECK-NEXT: wobbler.K2
; CHECK-NEXT: >
; CHECK-NEXT: GR[2] = <
; CHECK-NEXT: {K3}
; CHECK-NEXT: K3
; CHECK-NEXT: bar.K3
; CHECK-NEXT: wobbler.K3
; CHECK-NEXT: >
; CHECK: Number of SubGroups = 0

; CHECK-DAG: Users of K1:
; CHECK-DAG: Users of K2:
; CHECK-DAG: Users of K3:
; CHECK-DAG: Users of bar.K2: K2
; CHECK-DAG: Users of bar.K3: K3
; CHECK-DAG: Users of foo: K1
; CHECK-DAG: Users of wobbler.K1: foo
; CHECK-DAG: Users of wobbler.K2: bar.K2
; CHECK-DAG: Users of wobbler.K3: bar.K3

define internal spir_func void @wobbler(i32 %0, i32 %1) #0 {
  ret void
}

define internal spir_func void @foo(i32 %0) #0 {
  call spir_func void @wobbler(i32 %0, i32 3) #0
  ret void
}

define internal spir_func void @bar(i32 %0) #0 {
  call spir_func void @wobbler(i32 %0, i32 1) #0
  ret void
}

define dllexport spir_kernel void @K1(i32 %0, i64 %privBase) #1 {
  call spir_func void @foo(i32 0) #0
  ret void
}

define dllexport spir_kernel void @K2(i32 %0, i64 %privBase) #1 {
  call spir_func void @bar(i32 0) #0
  ret void
}

define dllexport spir_kernel void @K3(i32 %0, i64 %privBase) #1 {
  call spir_func void @bar(i32 0) #0
  ret void
}

attributes #0 = { noinline nounwind }
attributes #1 = { noinline nounwind "CMGenxMain" }

!genx.kernels = !{!2, !7, !8}
!genx.kernel.internal = !{!9, !11, !12}

!0 = !{i32 0, i32 0}
!1 = !{}
!2 = !{void (i32, i64)* @K1, !"K1", !3, i32 0, !4, !5, !6, i32 0}
!3 = !{i32 2, i32 96}
!4 = !{i32 72, i32 64}
!5 = !{i32 0}
!6 = !{!"buffer_t read_write"}
!7 = !{void (i32, i64)* @K2, !"K2", !3, i32 0, !4, !5, !6, i32 0}
!8 = !{void (i32, i64)* @K3, !"K3", !3, i32 0, !4, !5, !6, i32 0}
!9 = !{void (i32, i64)* @K1, !0, !10, !1, !10}
!10 = !{i32 0, i32 1}
!11 = !{void (i32, i64)* @K2, !0, !10, !1, !10}
!12 = !{void (i32, i64)* @K3, !0, !10, !1, !10}
