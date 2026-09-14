;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-fold-implied-select-cond -disable-output \
; RUN:   -debug-pass=Executions %s 2>&1 | FileCheck %s --check-prefix=REPORT
; RUN: igc_opt --opaque-pointers --igc-fold-implied-select-cond -S %s | \
; RUN:   FileCheck %s --check-prefix=IR

; Deleting an otherwise-unused select tree still changes the function. The
; pass-manager trace verifies that the pass reports the mutation.
;
; REPORT: Made Modification 'Fold Implied Select Condition' on Function 'f'
;
; IR-LABEL: define i32 @f(
; IR-NEXT:    ret i32 %w
; IR-NEXT:  }

define i32 @f(i1 %A, i32 %x, i32 %y, i32 %w) {
  %t = select i1 %A, i32 %x, i32 %y
  %f = select i1 %A, i32 %y, i32 %x
  %dead = select i1 %A, i32 %t, i32 %f
  ret i32 %w
}
