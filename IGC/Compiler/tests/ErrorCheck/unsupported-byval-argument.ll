;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-error-check -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; ErrorCheck
; ------------------------------------------------

; CHECK: error: ByVal argument with addrspace different than Generic/Private is not supported.

define void @test(i32 addrspace(3)* byval (i32) %p1) {
  ret void
}

!0 = !{void (i32 addrspace(3)*)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
