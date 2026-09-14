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

; CHECK-LABEL: define i32 @false_value(
; CHECK-NEXT:    %middle = select i1 %B, i32 %y, i32 %z
; CHECK-NEXT:    %result = select i1 %A, i32 %x, i32 %middle
; CHECK-NEXT:    ret i32 %result

define i32 @false_value(ptr addrspace(1) %p, i1 %A, i1 %B, i32 %x, i32 %y, i32 %z) {
  %dead = load i32, ptr addrspace(1) %p, align 4
  %inner = select i1 %A, i32 %dead, i32 %y
  %middle = select i1 %B, i32 %inner, i32 %z
  %result = select i1 %A, i32 %x, i32 %middle
  ret i32 %result
}
