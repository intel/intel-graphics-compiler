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

; CHECK-LABEL: define <2 x i32> @vector_condition(
; CHECK-NEXT:    %inner = select <2 x i1> %A, <2 x i32> %x, <2 x i32> %y
; CHECK-NEXT:    %outer = select <2 x i1> %A, <2 x i32> %inner, <2 x i32> %z
; CHECK-NEXT:    ret <2 x i32> %outer

define <2 x i32> @vector_condition(<2 x i1> %A, <2 x i32> %x, <2 x i32> %y, <2 x i32> %z) {
  %inner = select <2 x i1> %A, <2 x i32> %x, <2 x i32> %y
  %outer = select <2 x i1> %A, <2 x i32> %inner, <2 x i32> %z
  ret <2 x i32> %outer
}
