;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXModule -FunctionGroupAnalysis -march=genx64 \
; RUN: -print-function-group-info -disable-output -mcpu=Gen9 -S < %s | FileCheck \
; RUN: %s --match-full-lines

target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; COM: wobble
; COM:  /  \
; COM:  \  bar
; COM:   \ /
; COM:   foo
; COM:    |
; COM:    K

; CHECK: Number of Groups = 1
; CHECK-NEXT: GR[0] = <
; CHECK-NEXT: {K}
; CHECK-NEXT: K
; CHECK-NEXT: bar
; CHECK-NEXT: foo
; CHECK-NEXT: wobble
; CHECK-NEXT: >
; CHECK: Number of SubGroups = 0

; CHECK-DAG: Users of K:
; CHECK-DAG: Users of bar: foo
; CHECK-DAG: Users of foo: K
; CHECK-DAG: Users of wobble: bar foo

define dllexport spir_kernel void @K() #0 {
  call spir_func void @foo() #1
  ret void
}

define internal spir_func void @foo() #1 {
  call spir_func void @wobble() #1
  call spir_func void @bar() #1
  ret void
}

define internal spir_func void @bar() #1 {
  call spir_func void @wobble() #1
  ret void
}

define internal spir_func void @wobble() #1 {
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" }
attributes #1 = { noinline nounwind }

!genx.kernels = !{!2}
!genx.kernel.internal = !{!6}

!0 = !{i32 0, i32 0}
!1 = !{}
!2 = !{void ()* @K, !"K", !3, i32 0, !4, !0, !5, i32 0}
!3 = !{i32 2, i32 2, i32 96}
!4 = !{i32 72, i32 80, i32 64}
!5 = !{!"buffer_t read_write", !"buffer_t read_write"}
!6 = !{void ()* @K, !7, !8, !1, !9}
!7 = !{i32 0, i32 0, i32 0}
!8 = !{i32 0, i32 1, i32 2}
!9 = !{i32 1, i32 2, i32 1}
