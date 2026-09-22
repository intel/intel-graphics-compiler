;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-22-plus, !llvm-23-plus
; RUN: igc_opt --opaque-pointers %s -S -o - -diamond-chain-merge | FileCheck %s

; A later triangle PHI consumes a value defined in the preceding diamond's
; merge block. Splitting that incoming edge must use each path's mapped value
; before the triangle itself is merged.

define i32 @diamond_merge_to_triangle(i1 %c, i32 %x) {
; CHECK-LABEL: define i32 @diamond_merge_to_triangle(
; CHECK:       entry:
; CHECK-NEXT:    br i1 %c, label %true0, label %false0
; CHECK:       true0:
; CHECK:         [[TRUE:%.*]] = add i32 %x, 1
; CHECK:         [[TAIL:%.*]] = add i32 [[TRUE]], 3
; CHECK-NEXT:    ret i32 [[TAIL]]
; CHECK:       false0:
; CHECK:         [[FALSE:%.*]] = add i32 %x, 2
; CHECK-NEXT:    ret i32 [[FALSE]]
entry:
  br i1 %c, label %true0, label %false0
true0:
  br label %merge0
false0:
  br label %merge0
merge0:
  br i1 %c, label %true1, label %false1
true1:
  %t = add i32 %x, 1
  br label %merge1
false1:
  %f = add i32 %x, 2
  br label %merge1
merge1:
  %v = phi i32 [ %t, %true1 ], [ %f, %false1 ]
  br i1 %c, label %true2, label %merge2
true2:
  %tail = add i32 %v, 3
  br label %merge2
merge2:
  %result = phi i32 [ %tail, %true2 ], [ %v, %merge1 ]
  ret i32 %result
}
