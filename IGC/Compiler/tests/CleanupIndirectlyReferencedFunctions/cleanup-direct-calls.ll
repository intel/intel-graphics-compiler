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
; removed and linkage restored to internal when the only remaining uses of the
; function are direct calls (i.e. the address is no longer taken).
;
; Case 1: @func_direct_only has "referenced-indirectly" but its only use is a
;         direct call from @kernel. Attributes must be stripped.
;
; Case 2: @func_dead has "referenced-indirectly" but is completely unused.
;         Attributes must also be stripped (no indirect use exists).
;

; CHECK-LABEL: define internal spir_func void @func_direct_only(
define spir_func void @func_direct_only(i32 %a) #0 {
  ret void
}

define spir_kernel void @kernel(i32 %a) {
  call spir_func void @func_direct_only(i32 %a)
  ret void
}

; CHECK-LABEL: define internal spir_func void @func_dead(
define spir_func void @func_dead(i32 %a) #0 {
  ret void
}

; CHECK-NOT: "referenced-indirectly"
; CHECK-NOT: "visaStackCall"

attributes #0 = { "referenced-indirectly" "visaStackCall" }

!igc.functions = !{!0}
!0 = !{void (i32)* @kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
