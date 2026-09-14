;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-fold-implied-select-cond --igc-fold-implied-select-cond-max-depth=1 -S %s | FileCheck %s --check-prefix=DEPTH1
; RUN: igc_opt --opaque-pointers --igc-fold-implied-select-cond -S %s | FileCheck %s --check-prefix=DEFAULT

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; DEPTH1-LABEL: define i32 @depth_past_limit(
; DEPTH1-NEXT:    %inner = select i1 %A, i32 %x, i32 %y
; DEPTH1-NEXT:    %middle = select i1 %B, i32 %inner, i32 %z
; DEPTH1-NEXT:    %outer = select i1 %A, i32 %middle, i32 %w
; DEPTH1-NEXT:    ret i32 %outer

define i32 @depth_past_limit(i1 %A, i1 %B, i32 %x, i32 %y, i32 %z, i32 %w) {
  %inner = select i1 %A, i32 %x, i32 %y
  %middle = select i1 %B, i32 %inner, i32 %z
  %outer = select i1 %A, i32 %middle, i32 %w
  ret i32 %outer
}

; %A proves %C and %T folds to %x. %T and %C are then dead and the pass deletes them itself.
;
; DEFAULT-LABEL: define i32 @default_depth_at_limit(
; DEFAULT-NEXT:    %A = icmp ult i32 %n, 5
; DEFAULT-NEXT:    %p0 = select i1 %u0, i32 %x, i32 %z
; DEFAULT-NEXT:    %p1 = select i1 %u1, i32 %p0, i32 %z
; DEFAULT-NEXT:    %p2 = select i1 %u2, i32 %p1, i32 %z
; DEFAULT-NEXT:    %p3 = select i1 %u3, i32 %p2, i32 %z
; DEFAULT-NEXT:    %p4 = select i1 %u4, i32 %p3, i32 %z
; DEFAULT-NEXT:    %p5 = select i1 %u5, i32 %p4, i32 %z
; DEFAULT-NEXT:    %p6 = select i1 %u6, i32 %p5, i32 %z
; DEFAULT-NEXT:    %outer = select i1 %A, i32 %p6, i32 %w
; DEFAULT-NEXT:    ret i32 %outer

define i32 @default_depth_at_limit(i32 %n, i1 %u0, i1 %u1, i1 %u2, i1 %u3, i1 %u4, i1 %u5, i1 %u6,
                                   i32 %x, i32 %y, i32 %z, i32 %w) {
  %A = icmp ult i32 %n, 5
  %C = icmp ult i32 %n, 10
  %T = select i1 %C, i32 %x, i32 %y
  %p0 = select i1 %u0, i32 %T, i32 %z
  %p1 = select i1 %u1, i32 %p0, i32 %z
  %p2 = select i1 %u2, i32 %p1, i32 %z
  %p3 = select i1 %u3, i32 %p2, i32 %z
  %p4 = select i1 %u4, i32 %p3, i32 %z
  %p5 = select i1 %u5, i32 %p4, i32 %z
  %p6 = select i1 %u6, i32 %p5, i32 %z
  %outer = select i1 %A, i32 %p6, i32 %w
  ret i32 %outer
}

; %T above default limit, nothing folds.
;
; DEFAULT-LABEL: define i32 @default_depth_past_limit(
; DEFAULT-NEXT:    %A = icmp ult i32 %n, 5
; DEFAULT-NEXT:    %C = icmp ult i32 %n, 10
; DEFAULT-NEXT:    %T = select i1 %C, i32 %x, i32 %y
; DEFAULT-NEXT:    %p0 = select i1 %u0, i32 %T, i32 %z
; DEFAULT-NEXT:    %p1 = select i1 %u1, i32 %p0, i32 %z
; DEFAULT-NEXT:    %p2 = select i1 %u2, i32 %p1, i32 %z
; DEFAULT-NEXT:    %p3 = select i1 %u3, i32 %p2, i32 %z
; DEFAULT-NEXT:    %p4 = select i1 %u4, i32 %p3, i32 %z
; DEFAULT-NEXT:    %p5 = select i1 %u5, i32 %p4, i32 %z
; DEFAULT-NEXT:    %p6 = select i1 %u6, i32 %p5, i32 %z
; DEFAULT-NEXT:    %p7 = select i1 %u7, i32 %p6, i32 %z
; DEFAULT-NEXT:    %outer = select i1 %A, i32 %p7, i32 %w
; DEFAULT-NEXT:    ret i32 %outer

define i32 @default_depth_past_limit(i32 %n, i1 %u0, i1 %u1, i1 %u2, i1 %u3, i1 %u4, i1 %u5, i1 %u6, i1 %u7,
                                     i32 %x, i32 %y, i32 %z, i32 %w) {
  %A = icmp ult i32 %n, 5
  %C = icmp ult i32 %n, 10
  %T = select i1 %C, i32 %x, i32 %y
  %p0 = select i1 %u0, i32 %T, i32 %z
  %p1 = select i1 %u1, i32 %p0, i32 %z
  %p2 = select i1 %u2, i32 %p1, i32 %z
  %p3 = select i1 %u3, i32 %p2, i32 %z
  %p4 = select i1 %u4, i32 %p3, i32 %z
  %p5 = select i1 %u5, i32 %p4, i32 %z
  %p6 = select i1 %u6, i32 %p5, i32 %z
  %p7 = select i1 %u7, i32 %p6, i32 %z
  %outer = select i1 %A, i32 %p7, i32 %w
  ret i32 %outer
}
