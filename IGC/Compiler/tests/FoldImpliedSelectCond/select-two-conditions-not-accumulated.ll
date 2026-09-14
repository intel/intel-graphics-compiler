;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-fold-implied-select-cond -dce -S %s | FileCheck %s
;
; XFAIL: *

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; Starting at %outer the walk knows %A is true. It steps into %mid's true incoming value, so %B is true
; there too - but it does not record that. By the time it reaches %inner it still knows only
; %A, and %A alone does not prove `and i1 %A, %B`, so %inner is not folded to %x.
;
; TODO: Accumulate a list of assumptions through the recursion instead of a single
; Cond, adding each select's condition as the walk follows the selected incoming value.
;
; CHECK-LABEL: define i32 @two_conditions_not_accumulated(
; CHECK-NEXT:    %A = icmp ult i32 %n, 5
; CHECK-NEXT:    %B = icmp ult i32 %m, 5
; CHECK-NEXT:    %pad = select i1 %u, i32 %x, i32 %z
; CHECK-NEXT:    %mid = select i1 %B, i32 %pad, i32 %z
; CHECK-NEXT:    %outer = select i1 %A, i32 %mid, i32 %w
; CHECK-NEXT:    ret i32 %outer

define i32 @two_conditions_not_accumulated(i32 %n, i32 %m, i1 %u, i32 %x, i32 %y, i32 %z, i32 %w) {
  %A     = icmp ult i32 %n, 5
  %B     = icmp ult i32 %m, 5
  %mask  = and i1 %A, %B
  %inner = select i1 %mask, i32 %x, i32 %y
  %pad   = select i1 %u, i32 %inner, i32 %z
  %mid   = select i1 %B, i32 %pad, i32 %z
  %outer = select i1 %A, i32 %mid, i32 %w
  ret i32 %outer
}
