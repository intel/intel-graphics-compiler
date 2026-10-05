;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -S --igc-ocl-merge-allocas %s | FileCheck %s

; An allocation escaping inside an endless loop makes its live range cyclic, which used to hit the
; "No lifetime start block?" assertion. Keep it unused in entry - a user there hides the problem.

define void @single_escaping_alloca(ptr addrspace(1) %out) nounwind {
; CHECK-LABEL: @single_escaping_alloca(
; CHECK: alloca [4 x i32]
entry:
  %buf = alloca [4 x i32]
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %inc, %loop ]
  %gep = getelementptr inbounds [4 x i32], ptr %buf, i32 0, i32 %i
  %p2i = ptrtoint ptr %gep to i32
  store i32 %p2i, ptr addrspace(1) %out
  %inc = add i32 %i, 1
  br label %loop
}

; Both live at the same time inside the loop - must not be merged.
define void @two_escaping_allocas(ptr addrspace(1) %out) nounwind {
; CHECK-LABEL: @two_escaping_allocas(
; CHECK: alloca [4 x i32]
; CHECK: alloca [4 x i32]
entry:
  %a = alloca [4 x i32]
  %b = alloca [4 x i32]
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %inc, %loop ]
  %gepa = getelementptr inbounds [4 x i32], ptr %a, i32 0, i32 %i
  %gepb = getelementptr inbounds [4 x i32], ptr %b, i32 0, i32 %i
  %p2ia = ptrtoint ptr %gepa to i32
  %p2ib = ptrtoint ptr %gepb to i32
  %sum = add i32 %p2ia, %p2ib
  store i32 %sum, ptr addrspace(1) %out
  %inc = add i32 %i, 1
  br label %loop
}

; %a dies before the loop is entered - merging is still allowed.
define void @dead_before_loop(ptr addrspace(1) %out) nounwind {
; CHECK-LABEL: @dead_before_loop(
; CHECK: alloca [4 x i32]
; CHECK-NOT: alloca [4 x i32]
entry:
  %a = alloca [4 x i32]
  %b = alloca [4 x i32]
  %gepa = getelementptr inbounds [4 x i32], ptr %a, i32 0, i32 0
  store i32 7, ptr %gepa
  %loada = load i32, ptr %gepa
  store i32 %loada, ptr addrspace(1) %out
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %inc, %loop ]
  %gepb = getelementptr inbounds [4 x i32], ptr %b, i32 0, i32 %i
  %p2ib = ptrtoint ptr %gepb to i32
  store i32 %p2ib, ptr addrspace(1) %out
  %inc = add i32 %i, 1
  br label %loop
}
