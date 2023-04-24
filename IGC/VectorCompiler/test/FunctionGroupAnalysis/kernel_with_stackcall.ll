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

; COM: wobbler
; COM:    |
; COM:   SC
; COM:    |
; COM:    K

; CHECK: Number of Groups = 1
; CHECK-NEXT: GR[0] = <
; CHECK-NEXT: {K}
; CHECK-NEXT: K
; CHECK-NEXT: --SGR[0]: <SC>
; CHECK-NEXT: >
; CHECK: Number of SubGroups = 1
; CHECK-NEXT: SGR[0] = <
; CHECK-NEXT: {SC}
; CHECK-NEXT: SC
; CHECK-NEXT: wobbler
; CHECK-NEXT: >

define internal spir_func void @wobbler(i32 %0, i32 %1) #0 {
  ret void
}

define internal spir_func void @SC(i32 %0) #1 {
  call spir_func void @wobbler(i32 %0, i32 1) #0
  ret void
}

define dllexport spir_kernel void @K(i32 %0, i64 %privBase) #2 {
  call spir_func void @SC(i32 0) #1
  ret void
}

attributes #0 = { noinline nounwind }
attributes #1 = { noinline nounwind readnone "CMStackCall" }
attributes #2 = { noinline nounwind "CMGenxMain" }

!genx.kernels = !{!2}
!genx.kernel.internal = !{!7}

!0 = !{i32 0, i32 0}
!1 = !{}
!2 = !{void (i32, i64)* @K, !"K", !3, i32 0, !4, !5, !6, i32 0}
!3 = !{i32 2, i32 96}
!4 = !{i32 72, i32 64}
!5 = !{i32 0}
!6 = !{!"buffer_t read_write"}
!7 = !{void (i32, i64)* @K, !0, !8, !1, !8}
!8 = !{i32 0, i32 1}
