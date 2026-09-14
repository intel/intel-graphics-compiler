;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-fold-implied-select-cond -dce -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; The select that gets rewritten sits in a different basic block from the one supplying the
; assumption. The walk is purely use-based and needs no dominance information: %mid's single
; use is %outer's true incoming value, so %mid is only ever observed where %A holds, whichever block it
; is defined in.
;
; CHECK-LABEL: define i32 @cross_block(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %mid = select i1 %B, i32 %x, i32 %z
; CHECK-NEXT:    br i1 %p, label %bb, label %exit
; CHECK-EMPTY:
; CHECK-NEXT:  bb:                                               ; preds = %entry
; CHECK-NEXT:    %outer = select i1 %A, i32 %mid, i32 %z
; CHECK-NEXT:    br label %exit
; CHECK-EMPTY:
; CHECK-NEXT:  exit:                                             ; preds = %bb, %entry
; CHECK-NEXT:    %r = phi i32 [ 0, %entry ], [ %outer, %bb ]
; CHECK-NEXT:    ret i32 %r

define i32 @cross_block(i1 %A, i1 %B, i32 %x, i32 %y, i32 %z, i1 %p) {
entry:
  %inner = select i1 %A, i32 %x, i32 %y
  %mid   = select i1 %B, i32 %inner, i32 %z
  br i1 %p, label %bb, label %exit
bb:
  %outer = select i1 %A, i32 %mid, i32 %z
  br label %exit
exit:
  %r = phi i32 [ 0, %entry ], [ %outer, %bb ]
  ret i32 %r
}
