;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers --igc-reuse-predicated-load -S %s | FileCheck %s

; Both loads must be in the same basic block, so neither function is changed.
; @dominating_block is the case a dominance-based version could optimize;
; @backedge is the case it must not - %b is re-executed after a store on every
; iteration.

target datalayout = "e-p:64:64:64-i16:16:16-i32:32:32-n8:16:32"
target triple = "spir64-unknown-unknown"

; CHECK-LABEL: define i32 @dominating_block(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    br i1 %c, label %then, label %exit
; CHECK-EMPTY:
; CHECK-NEXT:  then:
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    br label %exit
; CHECK-EMPTY:
; CHECK-NEXT:  exit:
; CHECK-NEXT:    %phi = phi i32 [ 0, %entry ], [ %b, %then ]
; CHECK-NEXT:    %r = add i32 %a, %phi
; CHECK-NEXT:    ret i32 %r

define i32 @dominating_block(ptr addrspace(1) %p, i1 %q, i1 %c) {
entry:
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  br i1 %c, label %then, label %exit

then:
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  br label %exit

exit:
  %phi = phi i32 [ 0, %entry ], [ %b, %then ]
  %r = add i32 %a, %phi
  ret i32 %r
}

; CHECK-LABEL: define i32 @backedge(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    br label %loop
; CHECK-EMPTY:
; CHECK-NEXT:  loop:
; CHECK-NEXT:    %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
; CHECK-NEXT:    %acc = phi i32 [ %a, %entry ], [ %sum, %loop ]
; CHECK-NEXT:    store i32 %i, ptr addrspace(1) %p, align 4
; CHECK-NEXT:    %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
; CHECK-NEXT:    %sum = add i32 %acc, %b
; CHECK-NEXT:    %i.next = add i32 %i, 1
; CHECK-NEXT:    %done = icmp eq i32 %i.next, %n
; CHECK-NEXT:    br i1 %done, label %exit, label %loop
; CHECK-EMPTY:
; CHECK-NEXT:  exit:
; CHECK-NEXT:    ret i32 %sum

define i32 @backedge(ptr addrspace(1) %p, i1 %q, i32 %n) {
entry:
  %a = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %acc = phi i32 [ %a, %entry ], [ %sum, %loop ]
  store i32 %i, ptr addrspace(1) %p, align 4
  %b = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %p, i64 4, i1 %q, i32 0)
  %sum = add i32 %acc, %b
  %i.next = add i32 %i, 1
  %done = icmp eq i32 %i.next, %n
  br i1 %done, label %exit, label %loop

exit:
  ret i32 %sum
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32) #0
attributes #0 = { nounwind willreturn readonly }
