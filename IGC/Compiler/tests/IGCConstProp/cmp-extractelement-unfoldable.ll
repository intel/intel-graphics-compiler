;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-const-prop -S < %s | FileCheck %s

; Lane 1 compare cannot be folded; must not crash and must keep the icmp.

@g = external addrspace(3) global i8

; CHECK-LABEL: @f(
; CHECK: icmp eq i32 %e, 7
define i1 @f(i32 %i) {
  %e = extractelement <2 x i32> <i32 7, i32 ptrtoint (ptr addrspace(3) @g to i32)>, i32 %i
  %c = icmp eq i32 %e, 7
  ret i1 %c
}
