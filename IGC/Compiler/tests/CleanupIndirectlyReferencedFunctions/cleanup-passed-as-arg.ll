;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-cleanup-indirectly-referenced-functions -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
;
; Verifies that "referenced-indirectly" and "visaStackCall" attributes are
; preserved when the function's address is passed as an argument to another
; call (i.e. the value appears as a call operand but is NOT the callee).
;

declare void @takes_fptr(void (i32)*)

; CHECK-LABEL: define spir_func void @func_passed_as_arg(
; CHECK-SAME:  #[[ATTR:[0-9]+]]
define spir_func void @func_passed_as_arg(i32 %a) #0 {
  ret void
}

define spir_kernel void @kernel(i32 %a) {
  call void @takes_fptr(void (i32)* @func_passed_as_arg)
  ret void
}

; CHECK: attributes #[[ATTR]] = { "referenced-indirectly" "visaStackCall" }

attributes #0 = { "referenced-indirectly" "visaStackCall" }

!igc.functions = !{!0}
!0 = !{void (i32)* @kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
