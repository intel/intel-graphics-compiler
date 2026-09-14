;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-fold-implied-select-cond -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; freeze(%A) being true does not prove %A is true: if %A is poison, freeze may yield true while
; the select on %A takes either incoming value. Nothing may fold.
;
; CHECK-LABEL: define i32 @frozen_cond_does_not_prove_raw(
; CHECK-NEXT:    %A = icmp ult i32 %n, 5
; CHECK-NEXT:    %fA = freeze i1 %A
; CHECK-NEXT:    %inner = select i1 %A, i32 %x, i32 %y
; CHECK-NEXT:    %mid = select i1 %B, i32 %inner, i32 %z
; CHECK-NEXT:    %outer = select i1 %fA, i32 %mid, i32 %w
; CHECK-NEXT:    ret i32 %outer

define i32 @frozen_cond_does_not_prove_raw(i32 %n, i1 %B, i32 %x, i32 %y, i32 %z, i32 %w) {
  %A  = icmp ult i32 %n, 5
  %fA = freeze i1 %A
  %inner = select i1 %A, i32 %x, i32 %y
  %mid   = select i1 %B, i32 %inner, i32 %z
  %outer = select i1 %fA, i32 %mid, i32 %w
  ret i32 %outer
}

; The other direction is unsound for the same reason.
;
; CHECK-LABEL: define i32 @raw_cond_does_not_prove_frozen(
; CHECK-NEXT:    %A = icmp ult i32 %n, 5
; CHECK-NEXT:    %fA = freeze i1 %A
; CHECK-NEXT:    %inner = select i1 %fA, i32 %x, i32 %y
; CHECK-NEXT:    %mid = select i1 %B, i32 %inner, i32 %z
; CHECK-NEXT:    %outer = select i1 %A, i32 %mid, i32 %w
; CHECK-NEXT:    ret i32 %outer

define i32 @raw_cond_does_not_prove_frozen(i32 %n, i1 %B, i32 %x, i32 %y, i32 %z, i32 %w) {
  %A  = icmp ult i32 %n, 5
  %fA = freeze i1 %A
  %inner = select i1 %fA, i32 %x, i32 %y
  %mid   = select i1 %B, i32 %inner, i32 %z
  %outer = select i1 %A, i32 %mid, i32 %w
  ret i32 %outer
}
