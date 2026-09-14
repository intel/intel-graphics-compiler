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

; CHECK-LABEL: define i32 @root_identical_values(
; CHECK-NEXT:    ret i32 %x
; CHECK-NEXT:  }

define i32 @root_identical_values(i1 %A, i32 %x, i32 %y) {
  %t = select i1 %A, i32 %x, i32 %y
  %f = select i1 %A, i32 %y, i32 %x
  %outer = select i1 %A, i32 %t, i32 %f
  ret i32 %outer
}
