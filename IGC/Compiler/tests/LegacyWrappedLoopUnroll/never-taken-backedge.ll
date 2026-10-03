;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers -loop-unroll-legacy-wrapped -unroll-threshold=1 -S < %s | FileCheck %s
; RUN: igc_opt --opaque-pointers -loop-unroll-legacy-wrapped -unroll-threshold=1 -licm-legacy-wrapped -S < %s | FileCheck %s --check-prefix=LICM
; RUN: igc_opt --opaque-pointers -loop-unroll-legacy-wrapped -unroll-threshold=1 -loop-simplify -S < %s | FileCheck %s

; The last RUN checks that LoopSimplify, which uses the legacy LoopInfo, does
; not see loops that the wrapper removed.

; The loop body is over the unroll threshold, so the loop is not unrolled. Its
; backedge is never taken, so the wrapper breaks it.

; CHECK-LABEL: define void @single_trip(
; CHECK: loop:
; CHECK: %m4 = mul i32 %m3, %a
; CHECK-NEXT: br label %exit

; LICM does not sink the body of the former loop into its exit block.

; LICM-LABEL: define void @single_trip(
; LICM: loop:
; LICM: %a = load i32
; LICM: %m4 = mul i32 %m3, %a
; LICM-NEXT: br label %exit
; LICM: exit:
; LICM-NEXT: %m4.lcssa = phi i32 [ %m4, %loop ]

define void @single_trip(ptr addrspace(1) %in, ptr addrspace(1) %out) {
entry:
  br label %loop

loop:
  %a = load i32, ptr addrspace(1) %in
  %p = getelementptr i32, ptr addrspace(1) %in, i64 1
  %b = load i32, ptr addrspace(1) %p
  %m0 = mul i32 %a, %b
  %m1 = mul i32 %m0, %b
  %m2 = mul i32 %m1, %a
  %m3 = mul i32 %m2, %b
  %m4 = mul i32 %m3, %a
  br i1 false, label %loop, label %exit

exit:
  %m4.lcssa = phi i32 [ %m4, %loop ]
  store i32 %m4.lcssa, ptr addrspace(1) %out
  ret void
}

; A loop that runs twice keeps its backedge.

; CHECK-LABEL: define void @two_trips(
; CHECK: br i1 %cmp, label %loop, label %exit

define void @two_trips(ptr addrspace(1) %in, ptr addrspace(1) %out) {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %a = load i32, ptr addrspace(1) %in
  %p = getelementptr i32, ptr addrspace(1) %in, i64 1
  %b = load i32, ptr addrspace(1) %p
  %m0 = mul i32 %a, %b
  %m1 = mul i32 %m0, %b
  %m2 = mul i32 %m1, %a
  %m3 = mul i32 %m2, %b
  %m4 = mul i32 %m3, %a
  %i.next = add i32 %i, 1
  %cmp = icmp ult i32 %i.next, 2
  br i1 %cmp, label %loop, label %exit

exit:
  %m4.lcssa = phi i32 [ %m4, %loop ]
  store i32 %m4.lcssa, ptr addrspace(1) %out
  ret void
}

; The outer loop runs once and loses its backedge. The inner loop with an
; unknown trip count is kept.

; CHECK-LABEL: define void @nested(
; CHECK: inner:
; CHECK: br i1 %cmp, label %inner, label %outer.latch
; CHECK: outer.latch:
; CHECK-NEXT: %acc.lcssa = phi i32 [ %acc.next, %inner ]
; CHECK-NEXT: br label %exit

define void @nested(ptr addrspace(1) %in, ptr addrspace(1) %out, i32 %n) {
entry:
  br label %outer

outer:
  br label %inner

inner:
  %i = phi i32 [ 0, %outer ], [ %i.next, %inner ]
  %acc = phi i32 [ 0, %outer ], [ %acc.next, %inner ]
  %p = getelementptr i32, ptr addrspace(1) %in, i32 %i
  %v = load i32, ptr addrspace(1) %p
  %acc.next = add i32 %acc, %v
  %i.next = add i32 %i, 1
  %cmp = icmp ult i32 %i.next, %n
  br i1 %cmp, label %inner, label %outer.latch

outer.latch:
  %acc.lcssa = phi i32 [ %acc.next, %inner ]
  br i1 false, label %outer, label %exit

exit:
  %acc.lcssa.lcssa = phi i32 [ %acc.lcssa, %outer.latch ]
  store i32 %acc.lcssa.lcssa, ptr addrspace(1) %out
  ret void
}

; A fully unrolled loop is gone.

; CHECK-LABEL: define void @full_unroll(
; CHECK-NOT: br i1
; CHECK: ret void

define void @full_unroll(ptr addrspace(1) %in, ptr addrspace(1) %out) {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %acc = phi i32 [ 0, %entry ], [ %acc.next, %loop ]
  %p = getelementptr i32, ptr addrspace(1) %in, i32 %i
  %v = load i32, ptr addrspace(1) %p
  %acc.next = add i32 %acc, %v
  %i.next = add i32 %i, 1
  %cmp = icmp ult i32 %i.next, 4
  br i1 %cmp, label %loop, label %exit, !llvm.loop !0

exit:
  store i32 %acc.next, ptr addrspace(1) %out
  ret void
}

!0 = distinct !{!0, !1}
!1 = !{!"llvm.loop.unroll.full"}
