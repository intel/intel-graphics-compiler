;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -FCmpPaternMatch -S < %s | FileCheck %s
; ------------------------------------------------
; FCmpPaternMatch
; ------------------------------------------------

; +0.0 has all bits zero, so bits == 0 is !%c and the select uses %c with swapped values.
define i32 @pos_zero(float %a, float %b, i32 %x, i32 %y) {
; CHECK-LABEL: @pos_zero(
; CHECK: select i1 %c, i32 %y, i32 %x
  %c = fcmp olt float %a, %b
  %s = select i1 %c, float 1.0, float 0.0
  %bits = bitcast float %s to i32
  %z = icmp eq i32 %bits, 0
  %r = select i1 %z, i32 %x, i32 %y
  ret i32 %r
}

; -0.0 is 0x80000000, so bits == 0 is false for both values. The select must keep the compare.
define i32 @neg_zero_false_value(float %a, float %b, i32 %x, i32 %y) {
; CHECK-LABEL: @neg_zero_false_value(
; CHECK: select i1 %z, i32 %x, i32 %y
  %c = fcmp olt float %a, %b
  %s = select i1 %c, float 1.0, float -0.0
  %bits = bitcast float %s to i32
  %z = icmp eq i32 %bits, 0
  %r = select i1 %z, i32 %x, i32 %y
  ret i32 %r
}

; -0.0 as the true value. The select must keep the compare.
define i32 @neg_zero_true_value(float %a, float %b, i32 %x, i32 %y) {
; CHECK-LABEL: @neg_zero_true_value(
; CHECK: select i1 %z, i32 %x, i32 %y
  %c = fcmp olt float %a, %b
  %s = select i1 %c, float -0.0, float 1.0
  %bits = bitcast float %s to i32
  %z = icmp eq i32 %bits, 0
  %r = select i1 %z, i32 %x, i32 %y
  ret i32 %r
}
