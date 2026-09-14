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

; CHECK-LABEL: define i32 @identical_values(
; CHECK-NEXT:    %outer = select i1 %A, i32 %x, i32 %z
; CHECK-NEXT:    ret i32 %outer

define i32 @identical_values(i1 %A, i1 %B, i32 %x, i32 %z) {
  %inner = select i1 %A, i32 %x, i32 %z
  %mid   = select i1 %B, i32 %inner, i32 %x
  %outer = select i1 %A, i32 %mid, i32 %z
  ret i32 %outer
}
