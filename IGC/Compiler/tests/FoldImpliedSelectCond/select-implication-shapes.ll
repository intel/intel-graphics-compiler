;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-22-plus
; LLVM 22's isImpliedCondition adds the reasoning needed for the logical-and
; decomposition and mixed-sign comparison implications exercised below.
;
; RUN: igc_opt --opaque-pointers --igc-fold-implied-select-cond -dce -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; The implied condition is an `and` containing the assumed one, and the assumption is that it
; is false - so the implication runs in the negative direction: %A false proves %and false.
;
; CHECK-LABEL: define i32 @not_a_implies_not_and(
; CHECK-NEXT:    %mid = select i1 %C, i32 %y, i32 %z
; CHECK-NEXT:    %outer = select i1 %A, i32 %w, i32 %mid
; CHECK-NEXT:    ret i32 %outer

define i32 @not_a_implies_not_and(i1 %A, i1 %B, i1 %C, i32 %x, i32 %y, i32 %z, i32 %w) {
  %and   = and i1 %B, %A
  %inner = select i1 %and, i32 %x, i32 %y
  %mid   = select i1 %C, i32 %inner, i32 %z
  %outer = select i1 %A, i32 %w, i32 %mid
  ret i32 %outer
}

; The two conditions differ only in signedness. An unsigned comparison against a non-negative
; constant proves the signed comparison against that same constant.
;
; CHECK-LABEL: define i32 @ult_implies_slt(
; CHECK-NEXT:    %A = icmp ult i32 %n, 5
; CHECK-NEXT:    %mid = select i1 %C, i32 %x, i32 %z
; CHECK-NEXT:    %outer = select i1 %A, i32 %mid, i32 %w
; CHECK-NEXT:    ret i32 %outer

define i32 @ult_implies_slt(i32 %n, i1 %C, i32 %x, i32 %y, i32 %z, i32 %w) {
  %A     = icmp ult i32 %n, 5
  %S     = icmp slt i32 %n, 5
  %inner = select i1 %S, i32 %x, i32 %y
  %mid   = select i1 %C, i32 %inner, i32 %z
  %outer = select i1 %A, i32 %mid, i32 %w
  ret i32 %outer
}

; The same pair in the opposite direction, which does not hold: at %n = -1 the signed
; comparison is true and the unsigned one is false. Nothing may fold.
;
; CHECK-LABEL: define i32 @slt_does_not_imply_ult(
; CHECK-NEXT:    %S = icmp slt i32 %n, 5
; CHECK-NEXT:    %A = icmp ult i32 %n, 5
; CHECK-NEXT:    %inner = select i1 %A, i32 %x, i32 %y
; CHECK-NEXT:    %mid = select i1 %C, i32 %inner, i32 %z
; CHECK-NEXT:    %outer = select i1 %S, i32 %mid, i32 %w
; CHECK-NEXT:    ret i32 %outer

define i32 @slt_does_not_imply_ult(i32 %n, i1 %C, i32 %x, i32 %y, i32 %z, i32 %w) {
  %S     = icmp slt i32 %n, 5
  %A     = icmp ult i32 %n, 5
  %inner = select i1 %A, i32 %x, i32 %y
  %mid   = select i1 %C, i32 %inner, i32 %z
  %outer = select i1 %S, i32 %mid, i32 %w
  ret i32 %outer
}
