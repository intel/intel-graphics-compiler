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

; This test guards triangle-orientation handling.
; The second branch has reversed triangle orientation and must still map
; operations to semantically correct destination paths.

define spir_kernel void @triangle_orientation_flip(
    i1 %c, ptr addrspace(1) align 4 captures(none) %out) {
; CHECK-LABEL: define spir_kernel void @triangle_orientation_flip(
; CHECK-SAME: i1 [[C:%.*]], ptr addrspace(1) align 4 captures(none) [[OUT:%.*]]) {
; CHECK:       entry:
; CHECK-NEXT:    br i1 [[C]], label %[[CRIT:.*]], label %[[BODY:.*]]
; CHECK-NOT:   merge:
; CHECK-NOT:   mid:
;
; Path for c=true: must only do the tail store.
; CHECK:       [[CRIT]]:
; CHECK-NOT:     store i32 11
; CHECK:         store i32 22, ptr addrspace(1) [[OUT]], align 4
; CHECK-NEXT:    ret void
;
; Path for c=false: must keep body payload plus mid+tail stores.
; CHECK:       [[BODY]]:
; CHECK:         store i32 7, ptr addrspace(1) [[OUT]], align 4
; CHECK:         store i32 11, ptr addrspace(1) %{{.*}}, align 4
; CHECK:         store i32 22, ptr addrspace(1) [[OUT]], align 4
; CHECK-NEXT:    ret void
;
entry:
  br i1 %c, label %crit, label %body

crit:
  br label %merge

body:
  store i32 7, ptr addrspace(1) %out, align 4
  br label %merge

merge:
  %x = phi i32 [ 0, %crit ], [ 1, %body ]
  %idx = sext i32 %x to i64
  %p = getelementptr inbounds i32, ptr addrspace(1) %out, i64 %idx
  ; Reverse triangle orientation: succ1 unconditionally flows to succ0.
  br i1 %c, label %tail, label %mid

mid:
  store i32 11, ptr addrspace(1) %p, align 4
  br label %tail

tail:
  store i32 22, ptr addrspace(1) %out, align 4
  ret void
}
