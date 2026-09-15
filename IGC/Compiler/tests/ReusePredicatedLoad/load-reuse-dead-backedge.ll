;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load --verify -S %s | FileCheck %s

; The fallback value is a PHI whose backedge operand is defined after the reused
; load, so deleting the dead PHI also deletes the instruction that follows it.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @dead_backedge(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br label %loop
; CHECK-EMPTY:
; CHECK-NEXT:  loop:
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    %r = select i1 %q, i32 %a, i32 0
; CHECK-NEXT:    %r2 = add i32 %r, %a
; CHECK-NEXT:    br i1 %again, label %loop, label %exit
; CHECK-EMPTY:
; CHECK-NEXT:  exit:
; CHECK-NEXT:    ret i32 %r2

define i32 @dead_backedge(ptr addrspace(1) %p, i1 noundef %q, i1 %again) {
entry:
  br label %loop

loop:
  %fallback = phi i32 [ 0, %entry ], [ %next, %loop ]
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 %fallback)
  %next = add i32 1, 2
  %r = select i1 %q, i32 %b, i32 0
  %r2 = add i32 %r, %a
  br i1 %again, label %loop, label %exit

exit:
  ret i32 %r2
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
