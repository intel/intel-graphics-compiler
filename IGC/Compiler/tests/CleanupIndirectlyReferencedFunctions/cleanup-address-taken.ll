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
; preserved when the function address is still taken (stored to a pointer).
;

@fptr = internal global void (i32)* undef

; CHECK-LABEL: define spir_func void @func_address_taken(
; CHECK-SAME:  #[[ATTR:[0-9]+]]
define spir_func void @func_address_taken(i32 %a) #0 {
  ret void
}

define spir_kernel void @kernel(i32 %a) {
  store void (i32)* @func_address_taken, void (i32)** @fptr
  ret void
}

; CHECK: attributes #[[ATTR]] = { "referenced-indirectly" "visaStackCall" }

attributes #0 = { "referenced-indirectly" "visaStackCall" }

!igc.functions = !{!0}
!0 = !{void (i32)* @kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
