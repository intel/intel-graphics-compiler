;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-legalize-function-signatures -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LegalizeFunctionSignatures
; ------------------------------------------------

; Test that functions with illegal types referenced by metadata are handled correctly.

define spir_kernel void @test_kernel() {
  call void @foo(i1 true)
  ret void
}

define internal void @foo(i1) {
    ret void
}

!md = !{!0}
!0 = !{ptr @foo}

; CHECK: !0 = !{ptr poison}