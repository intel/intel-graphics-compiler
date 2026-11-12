;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; An empty block whose phi moves only copy constants is laid out right before
; its successor. Right after the branch it would make the phi results span the
; loop in layout order, which vISA augmentation treats as live across the loop.
; A block that copies a variable or undef keeps its place, and so does a
; block leaving a loop and a block with no loop between it and its successor.

; RUN: igc_opt -S --opaque-pointers --igc-layout < %s | FileCheck %s

; CHECK-LABEL: @constant_moves(
; CHECK:       entry:
; CHECK:       preheader:
; CHECK:       loop:
; CHECK:       exit:
; CHECK:       skip:
; CHECK-NEXT:    br label %merge
; CHECK:       merge:

define spir_kernel void @constant_moves(ptr addrspace(1) %out, float %a, i32 %n) {
entry:
  %has_work = icmp sgt i32 %n, 0
  br i1 %has_work, label %preheader, label %skip

skip:
  br label %merge

preheader:
  br label %loop

loop:
  %i = phi i32 [ 0, %preheader ], [ %i.next, %loop ]
  %acc = phi float [ 0.0, %preheader ], [ %acc.next, %loop ]
  %acc.next = fadd float %acc, %a
  %i.next = add i32 %i, 1
  %cont = icmp slt i32 %i.next, %n
  br i1 %cont, label %loop, label %exit

exit:
  br label %merge

merge:
  %res = phi float [ 0.0, %skip ], [ %acc.next, %exit ]
  store float %res, ptr addrspace(1) %out
  ret void
}

; CHECK-LABEL: @variable_moves(
; CHECK:       entry:
; CHECK:       skip:
; CHECK-NEXT:    br label %merge
; CHECK:       preheader:
; CHECK:       loop:
; CHECK:       exit:
; CHECK:       merge:

define spir_kernel void @variable_moves(ptr addrspace(1) %out, float %a, i32 %n) {
entry:
  %has_work = icmp sgt i32 %n, 0
  br i1 %has_work, label %preheader, label %skip

skip:
  br label %merge

preheader:
  br label %loop

loop:
  %i = phi i32 [ 0, %preheader ], [ %i.next, %loop ]
  %acc = phi float [ 0.0, %preheader ], [ %acc.next, %loop ]
  %acc.next = fadd float %acc, %a
  %i.next = add i32 %i, 1
  %cont = icmp slt i32 %i.next, %n
  br i1 %cont, label %loop, label %exit

exit:
  br label %merge

merge:
  %res = phi float [ %a, %skip ], [ %acc.next, %exit ]
  store float %res, ptr addrspace(1) %out
  ret void
}

; CHECK-LABEL: @undef_moves(
; CHECK:       entry:
; CHECK:       skip:
; CHECK-NEXT:    br label %merge
; CHECK:       preheader:

define spir_kernel void @undef_moves(ptr addrspace(1) %out, float %a, i32 %n) {
entry:
  %has_work = icmp sgt i32 %n, 0
  br i1 %has_work, label %preheader, label %skip

skip:
  br label %merge

preheader:
  br label %loop

loop:
  %i = phi i32 [ 0, %preheader ], [ %i.next, %loop ]
  %acc = phi float [ 0.0, %preheader ], [ %acc.next, %loop ]
  %acc.next = fadd float %acc, %a
  %i.next = add i32 %i, 1
  %cont = icmp slt i32 %i.next, %n
  br i1 %cont, label %loop, label %exit

exit:
  br label %merge

merge:
  %res = phi float [ undef, %skip ], [ %acc.next, %exit ]
  store float %res, ptr addrspace(1) %out
  ret void
}

; The break block stays next to the loop, where the break-block fix-up put it.
; CHECK-LABEL: @brk(
; CHECK:       header:
; CHECK:       brk:
; CHECK-NEXT:    br label %merge
; CHECK:       latch:
; CHECK:       exit2:
; CHECK:       merge:

define spir_kernel void @brk(ptr addrspace(1) %out, ptr addrspace(1) %in, i32 %n) {
entry:
  br label %header

header:
  %i = phi i32 [ 0, %entry ], [ %i.next, %latch ]
  %acc = phi float [ 0.0, %entry ], [ %acc.next, %latch ]
  %p = getelementptr float, ptr addrspace(1) %in, i32 %i
  %x = load float, ptr addrspace(1) %p
  %c1 = fcmp olt float %x, 0.0
  br i1 %c1, label %brk, label %latch

brk:
  br label %merge

latch:
  %acc.next = fadd float %acc, %x
  %i.next = add i32 %i, 1
  %c2 = icmp slt i32 %i.next, %n
  br i1 %c2, label %header, label %exit2

exit2:
  br label %merge

merge:
  %r = phi float [ 0.0, %brk ], [ %acc.next, %exit2 ]
  store float %r, ptr addrspace(1) %out
  ret void
}

; No loop between A and M, so A keeps its place.
; CHECK-LABEL: @churn(
; CHECK:       after:
; CHECK:       A:
; CHECK-NEXT:    br label %M
; CHECK:       B:
; CHECK:       B2:
; CHECK:       M:

define spir_kernel void @churn(ptr addrspace(1) %out, float %a, i32 %n, i1 %c, i1 %d) {
entry:
  br label %loop
loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %i.next = add i32 %i, 1
  %cont = icmp slt i32 %i.next, %n
  br i1 %cont, label %loop, label %after
after:
  br i1 %c, label %A, label %B
A:
  br label %M
B:
  %x = fmul float %a, %a
  store float %x, ptr addrspace(1) %out
  br i1 %d, label %B2, label %M
B2:
  %y = fadd float %x, %a
  br label %M
M:
  %r = phi float [ 1.0, %A ], [ %x, %B ], [ %y, %B2 ]
  store float %r, ptr addrspace(1) %out
  ret void
}
