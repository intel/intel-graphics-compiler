;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-16-plus, debug
; RUN: igc_opt --opaque-pointers -domtree -loops -licm-legacy-wrapped -verify-dom-info -verify-loop-info -S < %s \
; RUN:   | FileCheck %s

; With assertions, the legacy pass manager re-verifies every analysis a pass declares preserved. The wrapper's loop
; adaptor inserts header.preheader here, so a DominatorTree or LoopInfo claimed as preserved would be stale and fail
; verification.

; CHECK-LABEL: @no_preheader(
; CHECK:       header.preheader:
; CHECK-NEXT:    %inv = mul i32 %a, %b

define void @no_preheader(i32 %n, i32 %a, i32 %b, i1 %c, ptr addrspace(1) %p) {
entry:
  br i1 %c, label %header, label %other

other:
  br label %header

header:
  %i = phi i32 [ 0, %entry ], [ 0, %other ], [ %inc, %header ]
  %inv = mul i32 %a, %b
  %g = getelementptr i32, ptr addrspace(1) %p, i32 %i
  store i32 %inv, ptr addrspace(1) %g, align 4
  %inc = add i32 %i, 1
  %cmp = icmp slt i32 %inc, %n
  br i1 %cmp, label %header, label %exit

exit:
  ret void
}
