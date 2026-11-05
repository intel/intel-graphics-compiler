;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-add-implicit-args -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

; Two functions pass each other's address to a call. Opaque pointers have no bitcast around the address, so
; the address operand is a direct use of the function by a call. This must not send hasStackCallInCG into
; endless recursion.

; CHECK: define spir_func void @ping()
; CHECK: call void @sink(ptr @pong)

define spir_func void @ping() {
  call void @sink(ptr @pong)
  ret void
}

; CHECK: define spir_func void @pong()
; CHECK: call void @sink(ptr @ping)

define spir_func void @pong() {
  call void @sink(ptr @ping)
  ret void
}

declare void @sink(ptr)

!igc.functions = !{!0, !3}

!0 = !{ptr @ping, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @pong, !1}
