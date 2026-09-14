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

; When %s2 is visited as a root, incoming value %s1 is shared (%use reads it too) and %D is not
; implied by %A, so the walk bails without descending. Later, %s3's walk bypasses %s1 under
; %D and rewrites %s2's true incoming value to %s0. %s2's own condition %A implies %s0's condition %C,
; so the next fixed-point iteration must revisit %s2 and fold its true incoming value to %x.

; CHECK-LABEL: define i32 @two_runs(
; CHECK-NEXT:    %A = icmp ult i32 %n, 5
; CHECK-NEXT:    %C = icmp ult i32 %n, 10
; CHECK-NEXT:    %s0 = select i1 %C, i32 %x, i32 %y
; CHECK-NEXT:    %s1 = select i1 %D, i32 %s0, i32 %z
; CHECK-NEXT:    %s2 = select i1 %A, i32 %x, i32 %w
; CHECK-NEXT:    %s3 = select i1 %D, i32 %s2, i32 %v
; CHECK-NEXT:    %use = add i32 %s1, %s3
; CHECK-NEXT:    ret i32 %use

define i32 @two_runs(i32 %n, i1 %D, i32 %x, i32 %y, i32 %z, i32 %w, i32 %v) {
  %A = icmp ult i32 %n, 5
  %C = icmp ult i32 %n, 10
  %s0 = select i1 %C, i32 %x, i32 %y
  %s1 = select i1 %D, i32 %s0, i32 %z
  %s2 = select i1 %A, i32 %s1, i32 %w
  %s3 = select i1 %D, i32 %s2, i32 %v
  %use = add i32 %s1, %s3
  ret i32 %use
}
