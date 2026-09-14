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

; %G's walk reaches %T at depth 8 and stops at the cap. %P's walk then bypasses %n2 and %n3,
; shortening the chain by two, which brings %T within %G's reach - but that rewrite lands on
; %SI, so only %SI and %P are re-queued, never %G. Running the pass twice folds %T to %tv.
;
; TODO: Fold in one pass.
;
; CHECK-LABEL: define i32 @grandparent_not_requeued(
; CHECK:         %n7 = select i1 %I, i32 %tv, i32 %x7
; CHECK-NEXT:    %n6 = select i1 %H, i32 %n7, i32 %x6
; CHECK-NEXT:    %n5 = select i1 %F, i32 %n6, i32 %x5
; CHECK-NEXT:    %n4 = select i1 %E, i32 %n5, i32 %x4
; CHECK-NEXT:    %SI = select i1 %D, i32 %n4, i32 %s
; CHECK-NEXT:    %P = select i1 %B, i32 %SI, i32 %p
; CHECK-NEXT:    %G = select i1 %A, i32 %P, i32 %g
; CHECK-NEXT:    ret i32 %G

define i32 @grandparent_not_requeued(i32 %n, i1 %B, i1 %D, i1 %E, i1 %F, i1 %H, i1 %I,
                                     i32 %tv, i32 %fv, i32 %x2, i32 %x3, i32 %x4, i32 %x5,
                                     i32 %x6, i32 %x7, i32 %s, i32 %p, i32 %g) {
  %A  = icmp ult i32 %n, 5
  %C  = icmp ult i32 %n, 10
  %T  = select i1 %C, i32 %tv, i32 %fv
  %n7 = select i1 %I, i32 %T,  i32 %x7
  %n6 = select i1 %H, i32 %n7, i32 %x6
  %n5 = select i1 %F, i32 %n6, i32 %x5
  %n4 = select i1 %E, i32 %n5, i32 %x4
  %n3 = select i1 %B, i32 %n4, i32 %x3
  %n2 = select i1 %B, i32 %n3, i32 %x2
  %SI = select i1 %D, i32 %n2, i32 %s
  %P  = select i1 %B, i32 %SI, i32 %p
  %G  = select i1 %A, i32 %P,  i32 %g
  ret i32 %G
}
