;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-const-prop -S < %s | FileCheck %s
; ------------------------------------------------
; IGCConstProp
; ------------------------------------------------

; Insert and extract indices of different integer types can point to the same lane.

define i32 @insert_idx_i64(i32 %x) {
; CHECK-LABEL: @insert_idx_i64(
; CHECK:         [[E:%.*]] = extractelement <4 x i32> {{%.*}}, i32 1
; CHECK-NEXT:    ret i32 [[E]]
;
  %v = insertelement <4 x i32> <i32 1, i32 2, i32 3, i32 4>, i32 %x, i64 1
  %e = extractelement <4 x i32> %v, i32 1
  ret i32 %e
}

define i32 @extract_idx_i64(i32 %x) {
; CHECK-LABEL: @extract_idx_i64(
; CHECK:         ret i32 7
;
  %v0 = insertelement <4 x i32> <i32 1, i32 2, i32 3, i32 4>, i32 %x, i64 0
  %v1 = insertelement <4 x i32> %v0, i32 7, i32 2
  %e = extractelement <4 x i32> %v1, i64 2
  ret i32 %e
}

; An index value that does not fit in 64 bits is out of range.
define i32 @extract_idx_i128(i32 %x) {
; CHECK-LABEL: @extract_idx_i128(
; CHECK:         ret i32 poison
;
  %v = insertelement <4 x i32> <i32 1, i32 2, i32 3, i32 4>, i32 %x, i32 0
  %e = extractelement <4 x i32> %v, i128 18446744073709551617
  ret i32 %e
}
