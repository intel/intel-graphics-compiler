;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Verify that simplifying a select chain under a known condition never rewrites values observable
; through another use after the walk reaches or crosses a shared node.
;
; RUN: igc_opt --opaque-pointers --igc-fold-implied-select-cond -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; An unknown condition on a shared node stops the walk before its nested select chain can
; be simplified under %A.
;
; CHECK-LABEL: define i32 @shared_select_chain(
; CHECK-NEXT:    %inner = select i1 %A, i32 %x, i32 %y
; CHECK-NEXT:    %middle = select i1 %B, i32 %inner, i32 %z
; CHECK-NEXT:    %outer = select i1 %A, i32 %w, i32 %middle
; CHECK-NEXT:    %result = add i32 %outer, %middle
; CHECK-NEXT:    ret i32 %result

define i32 @shared_select_chain(i1 %A, i1 %B, i32 %w, i32 %x, i32 %y, i32 %z) {
  %inner = select i1 %A, i32 %x, i32 %y
  %middle = select i1 %B, i32 %inner, i32 %z
  %outer = select i1 %A, i32 %w, i32 %middle
  %result = add i32 %outer, %middle
  ret i32 %result
}

; The condition of %shared is implied by %A. Crossing that shared node may
; simplify %outer under the known condition, but must not rewrite %middle for %shared's other user.
;
; CHECK-LABEL: define i32 @shared_implied_parent(
; CHECK-NEXT:    %A = icmp ult i32 %n, 5
; CHECK-NEXT:    %C = icmp ult i32 %n, 10
; CHECK-NEXT:    %inner = select i1 %A, i32 %x, i32 %y
; CHECK-NEXT:    %middle = select i1 %B, i32 %inner, i32 %z
; CHECK-NEXT:    %shared = select i1 %C, i32 %middle, i32 %w
; CHECK-NEXT:    %outer = select i1 %A, i32 %middle, i32 0
; CHECK-NEXT:    %result = add i32 %outer, %shared
; CHECK-NEXT:    ret i32 %result

define i32 @shared_implied_parent(i32 %n, i1 %B, i32 %x, i32 %y, i32 %z, i32 %w) {
  %A = icmp ult i32 %n, 5
  %C = icmp ult i32 %n, 10
  %inner = select i1 %A, i32 %x, i32 %y
  %middle = select i1 %B, i32 %inner, i32 %z
  %shared = select i1 %C, i32 %middle, i32 %w
  %outer = select i1 %A, i32 %shared, i32 0
  %result = add i32 %outer, %shared
  ret i32 %result
}

; A shared node below a single-use prefix stops in-place modification of its
; nested select chain after its implied condition lets the walk cross it.
;
; CHECK-LABEL: define i32 @shared_below_single_use_prefix(
; CHECK-NEXT:    %A = icmp ult i32 %n, 5
; CHECK-NEXT:    %C = icmp ult i32 %n, 10
; CHECK-NEXT:    %inner = select i1 %A, i32 %x, i32 %y
; CHECK-NEXT:    %middle = select i1 %B, i32 %inner, i32 %z
; CHECK-NEXT:    %shared = select i1 %C, i32 %middle, i32 %w
; CHECK-NEXT:    %single_use2 = select i1 %D, i32 %middle, i32 %v
; CHECK-NEXT:    %single_use1 = select i1 %E, i32 %single_use2, i32 %u
; CHECK-NEXT:    %outer = select i1 %A, i32 %single_use1, i32 0
; CHECK-NEXT:    %result = add i32 %outer, %shared
; CHECK-NEXT:    ret i32 %result

define i32 @shared_below_single_use_prefix(i32 %n, i1 %B, i1 %D, i1 %E, i32 %x, i32 %y,
                                        i32 %z, i32 %w, i32 %v, i32 %u) {
  %A = icmp ult i32 %n, 5
  %C = icmp ult i32 %n, 10
  %inner = select i1 %A, i32 %x, i32 %y
  %middle = select i1 %B, i32 %inner, i32 %z
  %shared = select i1 %C, i32 %middle, i32 %w
  %single_use2 = select i1 %D, i32 %shared, i32 %v
  %single_use1 = select i1 %E, i32 %single_use2, i32 %u
  %outer = select i1 %A, i32 %single_use1, i32 0
  %result = add i32 %outer, %shared
  ret i32 %result
}

; Sharing with the other incoming value of the same root also prevents in-place simplification
; below the implied shared node.
;
; CHECK-LABEL: define i32 @shared_with_other_root_value(
; CHECK-NEXT:    %A = icmp ult i32 %n, 5
; CHECK-NEXT:    %C = icmp ult i32 %n, 10
; CHECK-NEXT:    %inner = select i1 %A, i32 %x, i32 %y
; CHECK-NEXT:    %middle = select i1 %B, i32 %inner, i32 %z
; CHECK-NEXT:    %shared = select i1 %C, i32 %middle, i32 %w
; CHECK-NEXT:    %single_use = select i1 %D, i32 %middle, i32 %v
; CHECK-NEXT:    %outer = select i1 %A, i32 %single_use, i32 %shared
; CHECK-NEXT:    ret i32 %outer

define i32 @shared_with_other_root_value(i32 %n, i1 %B, i1 %D, i32 %x, i32 %y, i32 %z,
                                       i32 %w, i32 %v) {
  %A = icmp ult i32 %n, 5
  %C = icmp ult i32 %n, 10
  %inner = select i1 %A, i32 %x, i32 %y
  %middle = select i1 %B, i32 %inner, i32 %z
  %shared = select i1 %C, i32 %middle, i32 %w
  %single_use = select i1 %D, i32 %shared, i32 %v
  %outer = select i1 %A, i32 %single_use, i32 %shared
  ret i32 %outer
}
